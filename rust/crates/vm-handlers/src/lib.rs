use capstone::prelude::*;
use pelite::pe32::{Pe, PeFile};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::path::Path;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum VmError {
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    #[error("pe parse: {0}")]
    Pe(String),
    #[error("vm table not found")]
    TableNotFound,
    #[error("capstone: {0}")]
    Capstone(String),
}

/// Common trampoline / dispatcher VAs (image base 0x400000).
pub const TRAMPOLINE: u64 = 0x4FAE80;
pub const DISPATCHER: u64 = 0x4FDE30;

/// EVMP base opcode. Table index i implements opcode (BASE_OPCODE + i).
pub const BASE_OPCODE: u32 = 0x1000;

/// Max slots to walk past the table base before giving up.
const MAX_SLOTS: usize = 0x400;

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct HandlerInfo {
    pub index: usize,
    pub opcode: u32,
    pub address: u64,
    /// Disassembled body size in bytes (None when handler pointer is 0).
    pub size: Option<usize>,
    pub name: String,
    /// True when word0 is a code pointer; false for unimplemented slots.
    pub valid: bool,
    /// Alternate handler stored at entry+8 (some slots dispatch via helper).
    pub alt_handler: Option<u64>,
}

/// Names recovered from earlier Ghidra/capstone sessions (annotate_db.json).
fn known_names() -> HashMap<u64, &'static str> {
    HashMap::from([
        // Quest
        (0x502CB0, "Script_SetStage"),
        (0x502BE0, "Script_GetStage"),
        (0x502C40, "Script_GetStageDone"),
        (0x502D30, "Script_StartQuest"),
        (0x502D80, "Script_StopQuest"),
        (0x502DD0, "Script_CompleteQuest"),
        // Actor
        (0x5016D0, "Script_SetActorValue"),
        (0x5017E0, "Script_ModActorValue"),
        (0x501960, "Script_KillActor"),
        (0x5150A0, "Script_ResurrectActor"),
        // Combat / Magic
        (0x501F50, "Script_Cast"),
        (0x50AF90, "Script_Evp"),
        // Item / Object
        (0x507320, "Script_AddItem"),
        (0x507650, "Script_Activate"),
        (0x50A150, "Script_Enable"),
        (0x50A240, "Script_Disable"),
        (0x504A10, "Script_Lock"),
        (0x504B00, "Script_UnLock"),
        // Crime / Faction
        (0x504D00, "Script_ModCrimeGold"),
        (0x50B4A0, "Script_SetCrimeGold"),
        (0x50AB80, "Script_SetFactionRank"),
        // UI / Sound
        (0x509490, "Script_PlaySound"),
        (0x509C50, "Script_Show1stPerson"),
        // Animation
        (0x5107A0, "PlayGroup"),
        (0x51AC70, "Anim_GroupID_Get"),
        // Misc
        (0x508950, "FastTravel_CheckHostile"),
        (0x510E60, "Path_CalcLowPathToPoint"),
        (0x511720, "SetCameraFOV_Console"),
        (0x517300, "Script_JumpTable"),
        (0x517320, "Script_ClassTable"),
        (0x518F00, "MapMarker_GetType"),
        (0x51C090, "Skill_IsMajor"),
        (0x5222D0, "CharGen_CalcStats"),
        (0x522760, "LevelUp_ApplyAttributes"),
        (0x523A10, "MapMarker_Discover"),
        // Quest internals
        (0x529AC0, "AdvanceStage_Helper"),
        (0x529B70, "QUST_UpdateMaxStage"),
        (0x529D20, "QUST_SaveGame"),
        (0x529FF0, "QUST_LoadSaveData"),
        (0x52A570, "QUST_LoadSubrecordsExt"),
        (0x52ACC0, "QuestStage_Constructor"),
        (0x52ADF0, "QuestStage_ResolveLogDate"),
        (0x52AF40, "QuestTarget_UpdateUI"),
        (0x52B0C0, "QuestStage_InitItem"),
        (0x52F010, "Topic_Resolve"),
        (0x52F330, "Topic_SortByPriority"),
        (0x52F770, "Topic_EvaluateAny"),
        (0x52F790, "Topic_FindOrCreateConversation"),
        (0x52F830, "Dialogue_CreateConversation"),
        (0x515AF0, "QuestTarget_DebugDump"),
    ])
}

struct SectionInfo {
    rva: u32,
    file_ptr: u32,
    raw_size: u32,
}

fn parse_sections(map: &[u8]) -> Result<(u64, Vec<SectionInfo>), VmError> {
    let file = PeFile::from_bytes(map).map_err(|e| VmError::Pe(format!("{e}")))?;
    let image_base = file.optional_header().ImageBase as u64;
    let sections = file
        .section_headers()
        .iter()
        .map(|s| SectionInfo {
            rva: s.VirtualAddress,
            file_ptr: s.PointerToRawData,
            raw_size: s.SizeOfRawData,
        })
        .collect();
    Ok((image_base, sections))
}

fn section_range<'a>(
    map: &'a [u8],
    sections: &[SectionInfo],
    name: &str,
) -> Result<(u32, &'a [u8]), VmError> {
    // Match by RVA prefix of common names is fragile; find by known names.
    let want_rva = match name {
        ".text" => 0x1000u32,
        ".data" => 0x702000u32,
        _ => return Err(VmError::Pe(format!("unknown section {name}"))),
    };
    let s = sections
        .iter()
        .find(|s| s.rva == want_rva)
        .ok_or_else(|| VmError::Pe(format!("{name} not found")))?;
    let end = (s.file_ptr + s.raw_size) as usize;
    map.get(s.file_ptr as usize..end)
        .map(|b| (s.rva, b))
        .ok_or_else(|| VmError::Pe(format!("{name} out of file")))
}

/// Walk the VM handler table in .data and recover each handler body.
///
/// Entry layout (40 bytes, confirmed against opcode→address deltas):
///   +0x00 handler VA (0 = unimplemented)
///   +0x04 DISPATCHER (0x4FDE30)
///   +0x08 alternate/helper handler or 0
///   +0x18 next opcode
pub fn recover_handlers(exe_path: &Path) -> Result<Vec<HandlerInfo>, VmError> {
    let map = std::fs::read(exe_path)?;
    let (image_base, sections) = parse_sections(&map)?;

    let (text_rva, text_data) = section_range(&map, &sections, ".text")?;
    let text_va = text_rva as u64 + image_base;
    let (_data_rva, data_data) = section_range(&map, &sections, ".data")?;

    let table_off_in_data = find_table(data_data)?;

    let cs = Capstone::new()
        .x86()
        .mode(arch::x86::ArchMode::Mode32)
        .syntax(arch::x86::ArchSyntax::Intel)
        .detail(true)
        .build()
        .map_err(|e| VmError::Capstone(e.to_string()))?;

    let names = known_names();
    let mut handlers = Vec::new();
    let mut invalid_run = 0usize;

    for i in 0..MAX_SLOTS {
        let entry_off = table_off_in_data + i * 40;
        if entry_off + 0x1c > data_data.len() {
            break;
        }
        let handler = le32(&data_data[entry_off..]) as u64;
        let word1 = le32(&data_data[entry_off + 4..]) as u64;
        let alt = le32(&data_data[entry_off + 8..]) as u64;
        let next_op = le32(&data_data[entry_off + 0x18..]);

        let in_text = (text_va..text_va + text_data.len() as u64).contains(&handler);
        let valid = handler != 0 && in_text;
        let expects_next = next_op == BASE_OPCODE + i as u32 + 1;
        if valid {
            invalid_run = 0;
        } else {
            invalid_run += 1;
        }
        if i > 0 && !expects_next && word1 != DISPATCHER && invalid_run >= 4 {
            break;
        }

        let size = if valid {
            disasm_function(&cs, text_data, text_va, handler).map(|(s, _, _)| s)
        } else {
            None
        };

        let name = names
            .get(&handler)
            .map(|s| s.to_string())
            .unwrap_or_else(|| format!("FUN_{handler:08X}"));

        let alt_in_text = (text_va..text_va + text_data.len() as u64).contains(&alt);
        handlers.push(HandlerInfo {
            index: i,
            opcode: BASE_OPCODE + i as u32,
            address: handler,
            size,
            name,
            valid,
            alt_handler: if alt_in_text { Some(alt) } else { None },
        });
    }
    if handlers.is_empty() {
        return Err(VmError::TableNotFound);
    }
    Ok(handlers)
}

fn le32(b: &[u8]) -> u32 {
    u32::from_le_bytes([b[0], b[1], b[2], b[3]])
}

/// Locate the handler table in .data.
///
/// Entry layout (40 bytes):
///   +0x00 handler VA (0 = unimplemented)
///   +0x04 DISPATCHER (0x4FDE30) on most slots
///   +0x18 next opcode (= current opcode + 1)
///
/// Anchored on the known AddItem pointer (opcode 0x1002). Its +0x18 word
/// must equal 0x1003, and the slot two before it must carry 0x1001.
fn find_table(data: &[u8]) -> Result<usize, VmError> {
    const ENTRY: usize = 40;
    let add_item: u32 = 0x507320;
    for off in (0..data.len().saturating_sub(ENTRY * 4)).step_by(4) {
        if le32(&data[off..]) != add_item || off < ENTRY * 2 {
            continue;
        }
        let base = off - ENTRY * 2;
        let next_op = le32(&data[off + 0x18..]);
        let prev_op = le32(&data[off - ENTRY + 0x18..]);
        let base_op = le32(&data[base + 0x18..]);
        // +0x18 stores next-opcode; AddItem (0x1002) slot must show 0x1003.
        if next_op == 0x1003 && prev_op == 0x1002 && base_op == 0x1001 {
            return Ok(base);
        }
    }
    Err(VmError::TableNotFound)
}

/// Linearly disassemble from `addr` until a `ret`/`jmp` terminator.
/// Returns (body size, calls_trampoline, ends with `mov al, 1`).
fn disasm_function(
    cs: &Capstone,
    text: &[u8],
    text_va: u64,
    addr: u64,
) -> Option<(usize, bool, bool)> {
    let offset = (addr - text_va) as usize;
    if offset >= text.len() {
        return None;
    }
    let insns = cs.disasm_count(&text[offset..], addr, 512).ok()?;
    if insns.is_empty() {
        return None;
    }

    let mut calls_trampoline = false;
    let mut last_end = addr;
    let mut terminator_idx: Option<usize> = None;

    for (i, insn) in insns.iter().enumerate() {
        let mnemonic = insn.mnemonic().unwrap_or("");
        if mnemonic == "call" {
            let detail = cs.insn_detail(insn).ok()?;
            if let Some(capstone::arch::ArchOperand::X86Operand(x86)) =
                detail.arch_detail().operands().first()
            {
                if x86.op_type == capstone::arch::x86::X86OperandType::Imm(TRAMPOLINE as i64) {
                    calls_trampoline = true;
                }
            }
        }
        last_end = insn.address() + insn.len() as u64;
        if mnemonic == "ret" || mnemonic == "jmp" || mnemonic == "retn" {
            terminator_idx = Some(i);
            break;
        }
    }

    let ends_mov_al_1 = terminator_idx
        .and_then(|i| i.checked_sub(1))
        .map(|prev| insns[prev].bytes() == [0xB0, 0x01])
        .unwrap_or(false);

    Some(((last_end - addr) as usize, calls_trampoline, ends_mov_al_1))
}

/// Top-frequency handlers from the EVMP cross-reference (call counts).
pub fn top_frequency_handlers() -> Vec<HandlerInfo> {
    [
        (0x1039u32, 0x502CB0u64, "SetStage"),
        (0x105E, 0x50AF90, "Evp"),
        (0x1002, 0x507320, "AddItem"),
        (0x1021, 0x50A150, "Enable"),
        (0x100D, 0x507650, "Activate"),
    ]
    .iter()
    .map(|&(opcode, address, name)| HandlerInfo {
        index: (opcode - BASE_OPCODE) as usize,
        opcode,
        address,
        size: None,
        name: name.to_string(),
        valid: true,
        alt_handler: None,
    })
    .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn known_addresses_are_stable() {
        assert_eq!(TRAMPOLINE, 0x4FAE80);
        assert_eq!(DISPATCHER, 0x4FDE30);
        assert_eq!(BASE_OPCODE, 0x1000);
    }

    #[test]
    fn table_walk_recovers_real_exe() {
        let exe = Path::new(r"C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Oblivion.exe");
        if !exe.exists() {
            eprintln!("skipping: Oblivion.exe not present");
            return;
        }
        let handlers = recover_handlers(exe).expect("table walk");
        assert!(
            handlers.len() >= 360,
            "expected ~366 handlers, got {}",
            handlers.len()
        );

        let by_name: HashMap<&str, &HandlerInfo> =
            handlers.iter().map(|h| (h.name.as_str(), h)).collect();
        for expected in [
            "Script_SetStage",
            "Script_AddItem",
            "Script_Enable",
            "Script_Activate",
            "Script_Evp",
        ] {
            let h = by_name
                .get(expected)
                .unwrap_or_else(|| panic!("{expected} not found"));
            assert!(h.valid && h.size.unwrap_or(0) > 0, "{expected} invalid");
            assert_eq!(h.opcode, h.index as u32 + BASE_OPCODE);
        }
    }
}
