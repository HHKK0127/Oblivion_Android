use anyhow::Result;
use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "oblivion-re", about = "Oblivion reverse-engineering toolkit")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// Parse an Oblivion .ess savegame file
    Ess { path: std::path::PathBuf },
    /// Parse Oblivion ESM records
    Esm {
        path: std::path::PathBuf,
        /// Print the plugin summary as JSON
        #[arg(long)]
        json: bool,
        /// Dump the subrecords of a single record (decimal or 0x-prefixed hex)
        #[arg(long, value_parser = parse_form_id)]
        formid: Option<u32>,
        /// List the records of one type instead of the summary
        #[arg(long, value_parser = parse_rec_type)]
        rec_type: Option<[u8; 4]>,
        /// Maximum records listed by --rec-type
        #[arg(long, default_value_t = 20)]
        limit: usize,
        /// Number of record types listed in the summary
        #[arg(long, default_value_t = 20)]
        top: usize,
    },
    /// Recover script VM handlers from Oblivion.exe via capstone
    Vm {
        /// Path to Oblivion.exe
        exe: std::path::PathBuf,
        /// Emit JSON annotation list (for annotate_db.json)
        #[arg(long)]
        json: bool,
    },
}

fn parse_form_id(raw: &str) -> std::result::Result<u32, String> {
    let trimmed = raw.trim();
    let (digits, radix) = match trimmed
        .strip_prefix("0x")
        .or_else(|| trimmed.strip_prefix("0X"))
    {
        Some(hex) => (hex, 16),
        None => (trimmed, 10),
    };
    u32::from_str_radix(digits, radix)
        .map_err(|_| format!("expected a FormID as decimal or 0x-prefixed hex, got {raw:?}"))
}

fn parse_rec_type(raw: &str) -> std::result::Result<[u8; 4], String> {
    let bytes = raw.as_bytes();
    if bytes.len() != 4 || !bytes.iter().all(|b| b.is_ascii_graphic() || *b == b' ') {
        return Err(format!(
            "expected a 4-character record type such as WEAP, got {raw:?}"
        ));
    }
    Ok(bytes.try_into().unwrap())
}

fn print_summary(summary: &esm_parser::PluginSummary, top: usize) {
    println!("File        {}", summary.file_name);
    println!(
        "Kind        {}",
        if summary.is_master {
            "master (.esm)"
        } else {
            "plugin (.esp)"
        }
    );
    println!("Header size {}", summary.header_size);
    println!(
        "Masters     {}",
        if summary.masters.is_empty() {
            "-".to_string()
        } else {
            summary.masters.join(", ")
        }
    );
    println!("Records     {}", summary.record_count);
    println!("Groups      {}", summary.group_count);
    println!("Compressed  {}", summary.compressed_count);

    let total = summary.record_count.max(1) as f64;
    println!();
    println!("Record types (top {top}):");
    for entry in summary.type_counts.iter().take(top) {
        println!(
            "  {:>4}  {:>9}  {:>5.1}%",
            entry.rec_type,
            entry.count,
            entry.count as f64 / total * 100.0
        );
    }
}

fn print_record(
    plugin: &esm_parser::Plugin,
    data: &[u8],
    index: &esm_parser::RecordIndex,
) -> Result<()> {
    println!("FormID      0x{:08X}", index.form_id);
    println!("Type        {}", index.name());
    println!(
        "Flags       0x{:08X}{}{}",
        index.flags,
        if index.compressed { " compressed" } else { "" },
        if index.is_deleted() { " deleted" } else { "" }
    );
    println!("Data size   {}", index.data_size);
    println!("File offset 0x{:X}", index.file_offset);
    if index.compressed {
        println!("Inflated    {}", index.decomp_size);
    }
    if index.parent_form_id != 0 {
        println!("Cell        0x{:08X}", index.parent_form_id);
    }
    if index.worldspace_form_id != 0 {
        println!("Worldspace  0x{:08X}", index.worldspace_form_id);
    }

    let subrecords = plugin.decode_record(data, index)?;
    println!("Subrecords  {}", subrecords.len());
    for (i, sub) in subrecords.iter().enumerate() {
        let readable = printable_string(&sub.data)
            .map(|text| format!("  \"{text}\""))
            .unwrap_or_default();
        println!(
            "  [{i:3}]  {}  size={:5}  {}{}",
            sub.name(),
            sub.data.len(),
            hex_preview(&sub.data, 16),
            readable
        );
    }
    Ok(())
}

/// Render a payload as a quoted string only when it looks like real text.
/// Binary payloads otherwise leak a stray character before their first NUL.
fn printable_string(data: &[u8]) -> Option<String> {
    let end = data.iter().position(|&b| b == 0).unwrap_or(data.len());
    let text = &data[..end];
    if text.len() < 4 || !text.iter().all(|b| (0x20..0x7f).contains(b)) {
        return None;
    }
    Some(String::from_utf8_lossy(text).into_owned())
}

fn hex_preview(data: &[u8], limit: usize) -> String {
    let mut out = String::new();
    for byte in data.iter().take(limit) {
        out.push_str(&format!("{byte:02X} "));
    }
    if data.len() > limit {
        out.push_str("...");
    }
    out.trim_end().to_string()
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.command {
        Commands::Ess { path } => {
            let data = std::fs::read(&path)?;
            let header = ess_parser::parse_header(&data)?;
            println!("Save version: {}", header.version);
            println!("Player: {}", header.player_name);
        }
        Commands::Esm {
            path,
            json,
            formid,
            rec_type,
            limit,
            top,
        } => {
            let data = std::fs::read(&path)?;
            let file_name = path
                .file_name()
                .map(|name| name.to_string_lossy().into_owned())
                .unwrap_or_else(|| path.display().to_string());
            let plugin = esm_parser::Plugin::parse(&file_name, &data)?;

            if json {
                println!("{}", serde_json::to_string_pretty(&plugin.summary())?);
            } else if let Some(form_id) = formid {
                match plugin.record_by_form_id(form_id) {
                    Some(index) => print_record(&plugin, &data, index)?,
                    None => anyhow::bail!("no record with FormID 0x{form_id:08X} in {file_name}"),
                }
            } else if let Some(tag) = rec_type {
                let total = plugin.records_of_type(&tag).count();
                let mut listed = 0usize;
                for index in plugin.records_of_type(&tag).take(limit) {
                    println!(
                        "0x{:08X}  size={:6}  flags=0x{:08X}  file=0x{:08X}  cell=0x{:08X}  world=0x{:08X}{}",
                        index.form_id,
                        index.data_size,
                        index.flags,
                        index.file_offset,
                        index.parent_form_id,
                        index.worldspace_form_id,
                        if index.is_deleted() { "  deleted" } else { "" }
                    );
                    listed += 1;
                }
                eprintln!(
                    "{listed} of {total} {} records shown",
                    esm_parser::tag_name(&tag)
                );
            } else {
                print_summary(&plugin.summary(), top);
            }
        }
        Commands::Vm { exe, json } => {
            let handlers = vm_handlers::recover_handlers(&exe)?;
            let valid: Vec<_> = handlers.iter().filter(|h| h.valid).collect();
            if json {
                println!("{}", serde_json::to_string_pretty(&handlers)?);
            } else {
                for h in &handlers {
                    if !h.valid {
                        println!("opcode 0x{:04X}  [MISS]", h.opcode);
                        continue;
                    }
                    println!(
                        "opcode 0x{:04X}  0x{:06X}  size={:4}  {}",
                        h.opcode,
                        h.address,
                        h.size.unwrap_or(0),
                        h.name
                    );
                }
                eprintln!("{} entries, {} implemented", handlers.len(), valid.len());
            }
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn form_id_accepts_decimal_and_hex() {
        assert_eq!(parse_form_id("100").unwrap(), 100);
        assert_eq!(parse_form_id("0x1F").unwrap(), 31);
        assert_eq!(parse_form_id("0X00FF00FF").unwrap(), 0x00FF_00FF);
        assert_eq!(parse_form_id(" 0x10 ").unwrap(), 16);
        assert!(parse_form_id("0xZZ").is_err());
        assert!(parse_form_id("").is_err());
        assert!(parse_form_id("-1").is_err());
    }

    #[test]
    fn rec_type_requires_four_printable_characters() {
        assert_eq!(parse_rec_type("WEAP").unwrap(), *b"WEAP");
        assert_eq!(parse_rec_type("GRUP").unwrap(), *b"GRUP");
        assert!(parse_rec_type("WEA").is_err());
        assert!(parse_rec_type("WEAPX").is_err());
        assert!(parse_rec_type("W\nAP").is_err());
    }

    #[test]
    fn hex_preview_truncates_long_payloads() {
        assert_eq!(hex_preview(&[0x00, 0xAB], 4), "00 AB");
        assert_eq!(hex_preview(&[0x01, 0x02, 0x03], 2), "01 02 ...");
        assert_eq!(hex_preview(&[], 4), "");
    }

    #[test]
    fn printable_string_ignores_binary_payloads() {
        assert_eq!(printable_string(b"TestCell\0").as_deref(), Some("TestCell"));
        assert_eq!(printable_string(b"o\0\xF3\xC1"), None);
        assert_eq!(printable_string(b"abc\0"), None);
        assert_eq!(printable_string(&[0x00, 0x01, 0x02, 0x03]), None);
    }
}
