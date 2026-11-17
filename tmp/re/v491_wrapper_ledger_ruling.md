# v491 — notify wrapper・GameSetting 台帳ファイル・`FUN_004CBA80` callsites の裁定（V-491-1..3 完遂・旧残点 V-406-3 継続第21弾）

- 採番: **v491**（旧残点群消化・probe prefix **v491_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-491-1..3（v490 残点）** — 1=`FUN_0046ABF0`（slot `+0x40` notify 先）本文 identity／2=GameSetting key 台帳の row 化／3=V-490-3 引継ぎ（`FUN_004CBA80` `param_3`）を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-491-1: `FUN_0046ABF0`（slot `+0x40` notify 先）本文** | **[確定相当]（wrapper）＋[未確定]（本体）** | L84340 実読 [確定(decompiled)]: **`void __thiscall FUN_0046abf0(undefined4 param_1, undefined4 param_2) { FUN_0045b670(param_1, param_2); return; }`（**1行 wrapper**）[確定]** → **実行体は `FUN_0045B670`（docs annotation 別 fn）[確定]** → **`virtual+0x8C(flag)`（bit5 set/clear・v489）→ `virtual+0x40()`＝`FUN_0046ABF0` → `FUN_0045B670` が bit 変更後の実処理 [確定相当]**、**`FUN_0045B670` の本文 identity（bit5 semantic を含む）は [未確定]＝残点**（wrapper 2段（thunk `0x4D65D0`→`FUN_0046A9E0`→`FUN_0046ABF0`→`FUN_0045B670`）を追跡した結果 [確定]） |
| **V-491-2: GameSetting key 台帳の row 化** | **[確定]** | **`tmp/re/v491_gamesettings_table.md`（**122 rows**）を生成 [確定]**: md 形式（header＋方式説明＋`\| addr \| key \| value(push imm) \|` テーブル＋件数注記）。**方式は v490 の nearest-push（fn 境界 CC/C3 → `mov ecx` 直前）を明記**、型 prefix（`i/f/s` [推定相当]）も注記 → **v490 ruling の「資料は probe out のみ」を解消し台帳参照可能に [確定]** |
| **V-491-3: `FUN_004CBA80` の callsites（`param_3` 手掛かり）** | **[部分確定]** | docs callsites **2件 [確定(decompiled)]**: **L163835 `iVar6 = FUN_004cba80(DAT_00b35eb8, 1);`（NorthMarker singleton・v478 既知）**／**L424183 `DAT_00b3bad4 = FUN_004cba80(DAT_00b33aac, 1);`（**第2の singleton `DAT_00B33AAC` を渡す新経路**）** → **`FUN_004CBA80` は複数 singleton（`0xB35EB8`/`0xB33AAC`）を `+0x48` list 系へ処理する fn [推定相当]**（`param_3`（char）は **docs 表示で省略/未記載＝2引数形式で2件とも書かれており [確定（制約）] → 3rd arg は machine 側（push 回数）で要確認＝残点**）。`DAT_00B33AAC` の identity は [未確定]＝残点 |
| **V-491-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v491_1）

### §2.1 `FUN_0046ABF0` wrapper（v491_1a）
```
L84340: void __thiscall FUN_0046abf0(undefined4 param_1, undefined4 param_2) {
L84343:   FUN_0045b670(param_1, param_2);
L84344:   return; }
chain: FUN_004D65D0 (thunk jmp 0x46a9e0) -> FUN_0046A9E0 (bit5 toggle + slot+0x40 call)
       -> FUN_0046ABF0 (wrapper) -> FUN_0045B670 (body, residual)
```

### §2.2 ledger file（v491_1b）
```
written: C:\...\tmp\re\v491_gamesettings_table.md (122 rows)
header: v490 残点（key 台帳の row 化）／方式: nearest-push（fn 境界 CC/C3 -> mov ecx 直前）／
        value: push imm（0..0xFFFF）／型 prefix i/f/s [推定相当]
```

### §2.3 `FUN_004CBA80` callsites（v491_1c）
```
docs hits: 3 (incl. definition)
L159742: int * __thiscall FUN_004cba80(int param_1, int param_2, char param_3)   (3-arg sig)
L163835: iVar6 = FUN_004cba80(DAT_00b35eb8, 1);      (NorthMarker, 2-arg display)
L424183: DAT_00b3bad4 = FUN_004cba80(DAT_00b33aac, 1); (second singleton, 2-arg display)
```

## §3 残点 **V-492-1..3**
1. 【中】**`FUN_0045B670`（notify 本体）本文 identity** → bit5（`0x20`）semantic と slot `+0x40` 機能が確定する（V-491-1 の残り）。
2. 【低】**`FUN_004CBA80` の `param_3`（machine push 回数で確認）と `DAT_00B33AAC` identity** ＋**`DAT_00b3bad4`（L424183 戻り先）の意味**。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v491_1 の1本 EXIT=0**（V-491-1 wrapper 確定・本体残点／V-491-2 台帳ファイル生成を確定／V-491-3 callsites 2件を部分確定・arg3 は docs 制約で残点）。
- **決定打の作り**: (i) **`FUN_0046ABF0` decompiled が1行 wrapper**で `FUN_0045B670` へ → 2段 wrapper 連鎖を確定 → (ii) **nearest-push capture を再実行し md 台帳へ書き出し**（v490 の残点（資料が probe out のみ）を解消）→ (iii) **`FUN_004CBA80(` の docs grep 3件**で2つの singleton callsite を獲得、**signature 3引数 vs 表示2引数の矛盾を制約として記録**。
- 妥当性チェック: ①wrapper⇔本体 fn（`FUN_0045B670`）annotation 実在— ②122 rows⇔v490 の insert-style 122 と一致— ③`FUN_004CBA80` 2 callsite⇔v478 L163835＋新規 L424183— ④2引数表示⇔3引数 signature（docs の省略/implicit として制約記録）— ⑤台帳 md の header（方式/件数）⇔v490 教訓の反映 — **五方向矛盾なし**。
- docs との不整合: **①`FUN_004CBA80` は signature 3引数だが callsite は2引数表示（docs の欠落/省略）＝制約として記録 [確定（制約）]**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **notify chain は「thunk→toggle→wrapper→body」の4段になることがある** — **段ごとに landing を記録**（本裁定で wrapper 段を確定、body 残）。
  2. **台帳化＝「probe out の資料を md ファイルへ」も裁定の一部**（v490 の残点（資料所在）を v491 で解消＝**残点の形が次の作業を誘導**）。
  3. **signature と callsite 表示の引数欠落を制約として明記**（`param_3` が docs に無い＝machine push 回数で確認と次の手を固定）。
  4. **docs grep の hits（3件）から「定義＋既知＋新規」を分離**（L159742 定義／L163835 既知／L424183 新規 singleton）。
  5. **第2の singleton（`DAT_00B33AAC`）出現で fn の汎用性が分かる**（NorthMarker 専用でなく複数 singleton を扱う）。
