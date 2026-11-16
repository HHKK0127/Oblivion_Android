# v430 — AV table 直後の隣接構造（script キーワード/型レジストリ）の裁定（V-430-1 完遂・派生残点第22弾）

- 採番: **v430**（派生残点群・probe prefix **v430_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416（読み取りのみ）
- 依頼経緯: v429 の残点 **V-430-1【低】（`[72]'Begin'` の正体＝隣接構造（`0xA2F7EC` 空文字列 ptr とセットで何の table か））** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 内文字列・data refs のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`[72]'Begin'`（`0xB0A2C8`）の正体** | **[確定]（構造実読）＋[推定高]（用途）** | **AV name table は `[0..71]`（`0xB0A1A8..0xB0A2C4`）で終了し、`0xB0A2C8` から stride `0x28`（40B）の新テーブルが隣接** — レコード構造 **`{name ptr, name2 ptr, id u32, zeros×6}`**: **`{'Begin', 空(0xA2F7EC), id=0x10}` → `{'End', 空, 0x11}` → `{'Short', 'int', 0x12}` → …**（id 連番0x10,0x11,0x12...）＝ **script コンパイラのキーワード/型レジストリ [推定高]**（`Begin`/`End`＝スクリプトブロック境界・`Short`/`int`＝変数型＝TES4 script の型キーワードと整合） |
| **AV table との境界** | **[確定]** | `0xB0A1A8 + 72×4 = 0xB0A2C8` ＝ **キーワード table レコード0の name ptr セルと厳密に接する**（＝AV table＝72件で v429 の終端確定と整合・`[72]` は AV ではなくキーワードレコード0） |
| **refs 形式** | **[確定]** | `cell+72`（`0xB0A2C8`）への dword xref＝**0件**（index/offset アクセスのみ）／`'Begin'` 文字列（`0xA51C1C`）＝**2件（`.text 0x6CA5C2`・`.data 0xB0A2C8`＝レコード0自身）**／`cell+73`（空文字列 ptr セル）＝`.text 1件` |

## §2 証拠（v430_1）

### §2.1 window 実読（`0xB0A2C0..0xB0A32C`）
```
0xB0A2C0: 'Darkness'        <- AV table [70]
0xB0A2C4: 'ResistWaterDamage' <- AV table [71]（終端）
0xB0A2C8: ptr 'Begin'        <- ★キーワード rec0.name
0xB0A2CC: ptr 0xA2F7EC(空)   <- rec0.name2
0xB0A2D0: 0x10               <- rec0.id
0xB0A2D4..0xB0A2EC: 0×6
0xB0A2F0: 'End' / 0xB0A2F4: 空 / 0xB0A2F8: 0x11   <- rec1（stride 0x28 ✓）
0xB0A318: 'Short' / 0xB0A31C: 'int' / 0xB0A320: 0x12  <- rec2
```

### §2.2 refs（v430_1(b)）
- `cell+72`（`0xB0A2C8`）＝0件／`'Begin'`（`0xA51C1C`）＝2件（`.text 0x6CA5C2`＋`.data 0xB0A2C8` 自身）／`cell+73`＝`.text 1件`。

## §3 残点 **V-431-1..2**
1. 【低】キーワード/型レジストリの**消費者 identity**（`.text 0x6CA5C2` と `cell+73` 参照 `0x4FCA1A` 帯）＝script parser/コンパイラのどの pass か＋`name2`（`'int'` 等）の意味（alias？ C 型名？）。
2. 【低】id 連番 `0x10` 起点の意味（キーワード enum の基点＝`0x00..0x0F` は何か＝レコード0より前の enum 帯）。

## §4 検証・自己批評
- probe: **v430_1 全 EXIT=0**（1本で V-430-1 完遂）。
- **決定打の作り**: (i) **window を AV table 終端後も延長**（`'Begin'/'End'/'Short'` の出現と stride0x28 の反復を実読）→ (ii) **`0xB0A1A8 + 72×4 = 0xB0A2C8` の境界計算で2テーブルの接続を確認** → (iii) refs 形式の census。
- 妥当性チェック: ①AV 72件⇔境界 0xB0A2C8⇔rec0 — ②stride0x28 反復（Begin/End/Short）⇔id 連番 0x10..0x12 — ③refs=index アクセスのみ⇔v427/v428 の index 形 — **三方向矛盾なし**。
- docs との不整合: **なし**（v429 の [推定高]（隣接構造）を構造確定へ）。docs/GT 反映は #47/#20 一括。
- **教訓**: **隣接 table の境界は「base + N×stride」の計算一致で確定する**（AV table 72件×4B＝キーワード rec0 開始位置）／**レコード構造は「反復出現（stride）＋id 連番」で組み立てる**（1レコードの偶然でなく3連続の反復を確認）。
