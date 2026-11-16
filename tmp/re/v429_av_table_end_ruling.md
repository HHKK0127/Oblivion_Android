# v429 — AV name table の終端と dispatcher case 範囲の裁定（V-429-1 完遂・派生残点第21弾）

- 採番: **v429**（派生残点群・probe prefix **v429_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v428 の残点 **V-429-1【低】（table 終端（`[70]Darkness` 以降）と最大 AV id（dispatcher case 上限 `0x1A+` との対応））** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **table の終端** | **[確定]（実読）** | window `+70..+73`: **`[70]Darkness` → `[71]ResistWaterDamage` → `[72]'Begin'` → `[73]=0xA2F7EC`（空文字列 ptr・run 終了）** → **AV name table ＝ `[0..71]` の72件 [確定]**。`[72]'Begin'` は AV 名として不自然＝**隣接構造の先頭 [推定高]**（`[73]`＝空文字列 default ptr と続く） |
| **dispatcher `FUN_00519F50` の case 範囲** | **[確定]** | decompiled body L210351-210436 の **switch case 全40件 ＝ `0..0x27`（0..39・全て連番）** — **case0..39 ⇔ table[0..39]（`[39]Infamy` まで）と完全一致 [確定]** |
| **最大 handled AV id と table の関係** | **[確定]（組合せ）** | **dispatcher が扱う AV id ＝ 0..39（Infamy まで）**／table は **0..71（`[71]ResistWaterDamage` まで）** → **effect AV 群（`[40]MagickaMultiplier`..`[71]ResistWaterDamage`）は dispatcher 外（magic/効果システム側で処理）[確定(否定側: dispatcher case なし)]**。attrs(0-7)/vitals(8-11)/skills(12-32)/behavior(33-36)/reputation(37-39) が dispatcher 管轄 |

## §2 証拠（v429_1）

### §2.1 table window（+70..+73）
```
[+70] 0xB0A2C0: str='Darkness'
[+71] 0xB0A2C4: str='ResistWaterDamage'
[+72] 0xB0A2C8: str='Begin'
[+73] 0xB0A2CC: 0x00A2F7EC (non-string -> RUN END)   ; 0xA2F7EC = 空文字列（v409 で実読済）
table entries: 73 (index 0..72) — 実名は [0..71] の72件
```

### §2.2 dispatcher cases（decompiled regex 全抽出）
- `FUN_00519F50` body L210351-210436・**case 40件: `0,1,...,0x20,0x25,0x26,0x27,0x21,0x22,0x23,0x24`**＝**0..39 連番（並びは末尾4件が0x25..0x27→0x21..0x24 の goto テーブル順）**。
- `0x27` = 39 ＝ table `[39]Infamy` ✓（v427/v428 の index=id 証明と接続）。

## §3 残点 **V-430-1..2**
1. 【低】`[72]'Begin'` の正体（隣接構造の用途＝`0xA2F7EC` 空文字列 ptr とセットで何の table か）。
2. 【低】effect AV（40..71）の書き手システム（dispatcher 外の magic/効果処理での setter 経路）— 任意の effect AV id（例 `[61]ResistFire`）の格納先。

## §4 検証・自己批評
- probe: **v429_1 全 EXIT=0**（1本で V-429-1 完遂）。
- **決定打の作り**: (i) **window 延長を run 破綻まで**（空文字列 ptr＝end 慣習を含めて実読）→ (ii) **dispatcher の decompiled switch を regex 全抽出**（部分列挙でなく40件全数）→ (iii) **case 上限と table 終端の突合**。
- 妥当性チェック: ①case0..39⇔table[0..39]（Infamy）— ②table 72件⇔effect AVs が dispatcher 外 — **二方向矛盾なし**。
- docs との不整合: **なし**（v428 の残点を確定）。docs/GT 反映は #47/#20 一括。
- **教訓**: **switch の case 範囲は「最後の case」でなく「regex 全抽出の集合」で見る**（本件は0x25..0x27 が 0x21..0x24 の前に並ぶため部分列挙だと上限を誤認する）／**table 終端は「空文字列 ptr」の出現で読む**（run break の2種類: 非 ptr と空文字列）。
