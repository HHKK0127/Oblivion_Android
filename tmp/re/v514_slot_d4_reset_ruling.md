# v514 — `virtual+0xD4` impl・`FUN_0046B2C0`＝reset 系・Player `+0x80` 共通 ptr の裁定（V-514-1..3 完遂・旧残点 V-406-3 継続第44弾）

- 採番: **v514**（旧残点群消化・probe prefix **v514_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-514-1..3（v513 残点）** — 1=`virtual+0xD4` slot impl と `FUN_0046B2C0`/`FUN_0046C460` identity／2=gated mask 源と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-514-1: `virtual+0xD4` slot impl（LoadGame name/version getter）** | **[推定相当]（REFR/CELL 別 impl）** | vtable 実読 [確定(機械)]: **`TESObjectREFR vt=0xA46C44 [+0xD4]=0x4D6DF0`（owning `FUN_004D6DB0`・L168240）／`TESObjectCELL vt=0xA469D4 [+0xD4]=0x4CCBC0`（owning `FUN_004CCBC0`・L160651）** → **`virtual+0xD4`＝REFR/CELL で別 impl（`FUN_004D6DF0`/`FUN_004CCBC0`）＝name/version getter 候補 [推定相当]**（v513 LoadGame の `uVar5 = virtual[*piVar4+0xd4](byte, u32)` 呼び出しと整合）。`+0xD8`（SetName・v474/v478）と対で **form の name/version 系 slot family [推定]** |
| **V-514-1: `FUN_0046B2C0`（FF id set caller 1）identity** | **[確定相当]（reset 系）** | docs head 実読 [確定(decompiled)]: **`void __fastcall FUN_0046b2c0(int param_1)`: **`if ((*(uint *)(param_1 + 8) >> 0xe & 1) == 0)`（**bit14 gate**）→ **`FUN_004524c0(*(undefined4 *)(param_1 + 0xc))`（**old remove（v505 remove fn）**）→ `uVar2 = 0; if (DAT_00b06158 != 0) { do { if (*(DAT_00b06150 + uVar2*4) == param_1) ... } }`（**global bucket `0xB06150` 走査で `param_1` を探す**）** → **`FUN_0046B2C0`＝「`+0x8` bit14 未立なら old remove＋global bucket から自身を探して処理する reset 系 fn [確定相当]」→ v509 `FUN_004492E0`（global clear/reset）の reset family に接続 [確定相当]**（**bit14 gate（v512 family）と reset（v509）の接続点**） |
| **V-514-1: `FUN_0046C460`（FF id set caller 2）identity** | **[部分確定]** | docs head [確定(decompiled)]: **`undefined4 * __fastcall FUN_0046c460(undefined4 *param_1)`（**return ptr**・locals 多（`pcVar1`/`ppuVar5`/`uVar6/uVar7`/`unaff_FS_OFFSET`/`local_8`））→ large fn で本文は cap 外＝**本文特定は残点 [部分確定]**（`param_1` が `undefined4*`＝ptr object [確定相当]） |
| **V-514-1: Player `+0x80` 共通 ptr（追加確認）** | **[確定相当]（複数 handler 共通）** | 別 band context 実読 [確定(decompiled)]: **`L28394`（`FUN_00426A90`・`"ExtraDataList.cpp"` line `0x1b6f`＝7023）／`L50367`（`FUN_00441280`・`"TES.cpp"` line `0x15f6`＝5622）→ いずれも `local != 0x4b4f4c42`（`"BLOB"` FourCC 判定）で error log、else で `FUN_0046b250(*puVar*)`（RTDynamicCast）** → **`Player+0x80`＝LoadGame buffer の block header ptr が複数の LoadGame handler（`EffectSettingCollection.cpp`/`ExtraDataList.cpp`/`TES.cpp`）で共通使用 [確定相当]**（v513 に追補＝**LoadGame 共通 state**） |
| **V-514-2: gated mask 源／caller 分離** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-514-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v514_1）

### §2.1 slot `+0xD4`（v514_1a）
```
TESObjectREFR vt=0xA46C44 [+0xD4] = 0x4D6DF0  owning=FUN_004d6db0 (L168240)
TESObjectCELL vt=0xA469D4 [+0xD4] = 0x4CCBC0  owning=FUN_004ccbc0 (L160651)
=> name/version getter candidates (distinct impls)  [推定相当]
```

### §2.2 `FUN_0046B2C0`（v514_1b）
```
L84780: void __fastcall FUN_0046b2c0(int param_1) {
L84786:   if ((*(uint *)(param_1 + 8) >> 0xe & 1) == 0) {        // bit14 gate (v512)
L84787:     FUN_004524c0(*(undefined4 *)(param_1 + 0xc));          // old remove (v505)
L84789:     uVar2 = 0; if (DAT_00b06158 != 0) {
L84791:       do { if (*(DAT_00b06150 + uVar2 * 4) == param_1) {   // global bucket walk
  ... body continues beyond cap
=> reset-family fn (global bucket walk + old remove)  [確定相当]
```

### §2.3 Player `+0x80` other bands（v514_1c）
```
L28394 (FUN_00426a90, "ExtraDataList.cpp" line 0x1b6f=7023):
  local_144 != 0x4b4f4c42 ("BLOB") -> puVar2 = *(DAT_00b33b00 + 0x80) -> null? log : RTDynamicCast
L50367 (FUN_00441280, "TES.cpp" line 0x15f6=5622): same shape
=> +0x80 = LoadGame block-header ptr shared across handlers  [確定相当]
```

## §3 残点 **V-515-1..3**
1. 【中】**`FUN_004D6DF0`/`FUN_004CCBC0`（slot `+0xD4` impl）本文 identity**（name/version getter 完遂）＋ **`FUN_0046C460`（large fn）本文**。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ `FUN_0045B670` gated の `param_1+0xC` key と `FUN_0046C300` の current ptr の caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v514_1 の1本 EXIT=0**（V-514-1 slot `+0xD4`＝REFR/CELL 別 impl を vtable 実読で推定相当・`FUN_0046B2C0`＝reset 系を確定相当・`FUN_0046C460` は large fn で部分確定・Player `+0x80` 共通 ptr を追補／V-514-2/3 は引継ぎ）。
- **決定打の作り**: (i) **slot `+0xD4` を form family vtable で実読** → REF/CELL 別 impl と name/version getter 候補を特定（**slot family `+0xD4`/`+0xD8`**）→ (ii) **`FUN_0046B2C0` docs head 16行**で bit14 gate→old remove→global bucket 走査を獲得 → **v509 reset family と接続** → (iii) **Player `+0x80` の別 band context（`ExtraDataList.cpp`/`TES.cpp`）**で共通 ptr を追補。
- 妥当性チェック: ①`FUN_004524C0`（old remove）⇔v505 remove（`count--`）— ②`DAT_00b06158`/`0x6150`⇔v509 `FUN_004492E0` global clear（同一 bucket）— ③bit14 gate⇔v512 family 12fn — ④`FUN_0046B250`⇔v501/v502/v513 RTDynamicCast wrapper（3度目）— ⑤`0x4b4f4c42`＝`"BLOB"`⇔v484/v485/v504 FourCC 法 — **五方向矛盾なし**。
- docs との不整合: **なし**（slot 実読・docs head とも一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **slot family（`+0xD4` getter ⇔ `+0xD8` setter）が form の name/version 系を語る**（**REFR/CELL で別 impl＝class ごとに getter が分岐**＝v513 LoadGame 呼び出しとの整合）。
  2. **`bit gate→old remove→global bucket walk` の3段が reset 系 fn の定番**（**v509 `FUN_004492E0` と `FUN_0046B2C0` の横断一致＝reset family の完成**）。
  3. **`Player+0x80` の「共通 state」性格は別 band context で確定**（**複数 LoadGame handler（`EffectSettingCollection`/`ExtraDataList`/`TES`）での同一 pattern**＝**ptr の中身は log context 法の追補**）。
  4. **large fn（locals 多）は signature までで残点化**（`FUN_0046C460`＝**budget 制約を明記**）。
  5. **RTDynamicCast wrapper（`FUN_0046B250`）が LoadGame 全 band で横断**（**cast wrapper の利用回数が増えるほど「LoadGame の form cast 標準経路」が確定**）。
