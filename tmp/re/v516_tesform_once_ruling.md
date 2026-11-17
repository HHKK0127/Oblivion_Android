# v516 — `FUN_0041FA30`＝extra type10 getter・`TESForm` vtable 確定・`FUN_004D6DF0`＝once 委譲 wrapper の裁定（V-516-1..3 完遂・旧残点 V-406-3 継続第46弾）

- 採番: **v516**（旧残点群消化・probe prefix **v516_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-516-1..3（v515 残点）** — 1=`FUN_0041FA30` identity＋`PTR_FUN_00A3BE3C` RTTI＋`FUN_004D6DF0` machine／2=gated mask 源と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-516-1: `FUN_0041FA30`（CELL `+0xD4` getter source）identity** | **[確定相当]（extra type 10 getter）** | L22075 実読 [確定(decompiled)]: **`undefined4 FUN_0041fa30(void)`: `iVar1 = FUN_0041e210(10); if (iVar1 == 0) return 0; return *(undefined4 *)(iVar1 + 0xc);`** → **`FUN_0041FA30`＝`FUN_0041E210(10)`（**extra data type 10＝`GetExtraData(type=10)`・v504 `BaseExtraList::GetExtraData`**）の結果の `+0xc` を返す getter [確定相当]** → **CELL `+0xD4` getter＝「extra data type 10 の `+0xc` field を取得（null→`DAT_00B35C0C` default）[確定相当]」**（`FUN_0041E210(10)` の type 10＝特定 extra data type [推定]＝**extra type 番号が getter の identity**） |
| **V-516-1: `PTR_FUN_00A3BE3C`（`FUN_0046C460` の vtable）RTTI** | **[確定]（`.?AVTESForm@@`）** | RTTI 実読 [確定(実RTTI)]: **`rtti_name(0xA3BE3C) = .?AVTESForm@@` → `FUN_0046C460`（v515 ctor 候補）は `TESForm` vtable を store → TESForm 系 ctor [確定相当]（v515 ctor 候補の class 確定）**。vtable slots: **`[+0x0/+0x4]`＝`FUN_0060D0A0`（ctor/dtor pair [推定]）／`[+0x8/+0xC]`＝`FUN_0046B800/870`（`FUN_0046B7A0` 内）／`[+0x10]`＝`FUN_0046C620`／`[+0x14]`＝`FUN_0046C2E0`（`FUN_0046C280` 内・v512 bit14 family）[確定(機械)]** |
| **V-516-1: `FUN_004D6DF0`（REFR `+0xD4` impl・docs gap）machine** | **[確定相当]（once 委譲 wrapper）** | machine decode [確定(機械)]: **`cmp byte [0xb333f4], 0`（once flag）→ `push esi; mov esi,ecx` → `mov byte [0xb333f4], 1`（flag set）→ **`mov eax,[esi]; mov edx,[eax+0xd4]; call edx`（**`virtual+0xD4` の委譲 call！（self の `+0xD4` slot を呼ぶ）**）→ `test eax,eax; mov byte [0xb333f4], 0; je` → `lea edx,[eax+1]; jmp`（**result+1 の操作**）** → **`FUN_004D6DF0`＝「once flag（`0xB333F4`）付きで `virtual+0xD4`（self の slot）を委譲 call し、結果に `+1` を加える getter wrapper [確定相当]」**（**`+1`＝index+1 [推定]**）→ **REFR `+0xD4` impl＝once-guarded 委譲 wrapper [確定相当]**（**v514 docs gap を machine で補完**） |
| **V-516-2: gated mask 源／caller 分離** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-516-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v516_1）

### §2.1 `FUN_0041FA30`（v516_1a）
```
L22075: undefined4 FUN_0041fa30(void) {
L22080:   iVar1 = FUN_0041e210(10);          // GetExtraData(type=10)  (v504)
L22081:   if (iVar1 == 0) { return 0; }
L22084:   return *(undefined4 *)(iVar1 + 0xc); // +0xc field of the extra data
  => CELL +0xD4 getter source: extra type 10 -> +0xc  [確定相当]
```

### §2.2 `PTR_FUN_00A3BE3C` RTTI（v516_1b）
```
rtti_name(0xA3BE3C) = .?AVTESForm@@
  [+0x0] = 0x0060D0A0  owning=FUN_0060d0a0      (ctor/dtor pair [推定])
  [+0x4] = 0x0060D0A0  owning=FUN_0060d0a0
  [+0x8] = 0x0046B800  owning=FUN_0046b7a0
  [+0xC] = 0x0046B870  owning=FUN_0046b7a0
  [+0x10] = 0x0046C620 owning=FUN_0046c620
  [+0x14] = 0x0046C2E0 owning=FUN_0046c280      (v512 bit14 family)
=> FUN_0046C460 stores the TESForm vtable -> TESForm-family ctor  [確定相当]
```

### §2.3 `FUN_004D6DF0` machine (docs gap)（v516_1c）
```
4D6DF0: cmp byte ptr [0xb333f4], 0            ; once flag
4D6DF7: push esi / mov esi, ecx
4D6DFC: mov byte ptr [0xb333f4], 1            ; flag set
4D6E03: mov eax, [esi] / mov edx, [eax + 0xd4] / call edx   ; virtual+0xD4 delegate
4D6E0D: test eax, eax / mov byte ptr [0xb333f4], 0 / je
4D6E18: lea edx, [eax + 1] / jmp               ; result + 1  [推定]
=> once-guarded delegating getter wrapper  [確定相当]  (v514 docs gap filled)
```

## §3 残点 **V-517-1..3**
1. 【中】**`FUN_0041E210(10)` の extra data type 10 の正体**（`+0xc` field が何の値か）＋ **`FUN_0046C460` ctor の残り本文**（`formEnumString` 検証後の init 完遂）＋ **`FUN_0060D0A0`（TESForm ctor/dtor pair 候補）identity**。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v516_1 の1本 EXIT=0**（V-516-1 `FUN_0041FA30`＝extra type 10 getter を確定相当・`PTR_FUN_00A3BE3C`＝`.?AVTESForm@@` を RTTI 実読で確定・`FUN_004D6DF0`＝once 委譲 wrapper を machine で確定相当（v514 docs gap 補完）／V-516-2/3 は引継ぎ）。
- **決定打の作り**: (i) **`FUN_0041FA30` docs 12行**で `FUN_0041E210(10)`→`+0xc` を獲得 → **extra type 10 getter と即決**（v504 `BaseExtraList::GetExtraData` と接続）→ (ii) **`PTR_FUN_00A3BE3C` RTTI 実読**で `.?AVTESForm@@` → **v515 ctor class 確定** → (iii) **`FUN_004D6DF0` machine decode**で once flag＋`virtual+0xD4` 委譲＋`+1` を獲得 → docs gap 補完。
- 妥当性チェック: ①`FUN_0041E210(10)`⇔v504 `BaseExtraList::GetExtraData`（type byte 10）— ②`.?AVTESForm@@`⇔v515 `*param_1 = &PTR_FUN_00A3BE3C`（vtable store）— ③`virtual+0xD4` 委譲⇔v513 LoadGame `virtual[*piVar4+0xd4]`（同一 slot）— ④`DAT_00B35C0C` default⇔v515 `FUN_004CCBC0` null fallback — ⑤`0xB333F4` once flag⇔v462/v466 once-init pattern（`FUN_004040E0` 系）— **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_0041FA30`/`FUN_004D6DF0` は machine/docs とも整合・`FUN_004D6DF0` は docs gap を machine 補完）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **extra data getter は `FUN_0041E210(type)` の type 番号が identity**（**`GetExtraData(10)`⇔`+0xc`＝特定 extra data の値**＝v504 に接続）。
  2. **`PTR_FUN_xxx`（vtable ptr）の RTTI 逆引きが ctor の class を確定**（**`.?AVTESForm@@`＝v515 ctor 候補の class 完遂**＝vtable store 法の完成）。
  3. **once flag（`cmp byte [GLOBAL], 0; mov …,1; …; mov …,0`）＋`virtual+slot` 委譲が wrapper fn の定番**（**`FUN_004D6DF0`＝once-guarded delegate**＝docs gap を machine で補完）。
  4. **`lea edx,[eax+1]` の `+1` が「result+1」操作の目印**（index/counter 増分 [推定]）。
  5. **docs gap の補完は machine decode で**（v514 推定→v516 machine 確定＝**gap 対応の完成形**）。
