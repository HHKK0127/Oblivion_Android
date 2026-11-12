# v288_misc_residuals.md — v60v76-bundle 依頼6 [P6]: 小物残件まとめ v60/v63/v64/v69（2026-11-04）

- 依頼元: `tmp/re/handoff_20261105_v60v76_residuals_bundle.md` 依頼6 [優先度 P6]
- 対象: Oblivion.exe v1.2.0416 (Steam)
- ハーネス: `probe_harness.py selftest` → **4/4 PASS**（本セッション実行）
- **判定: 9点中 7点 [確定] / 1点 [部分確定] / 1点 [確定（他レポートへ統合）]**

---

## 1. v60(a): startup-init 配列スキャンの未完部 → **[確定] 走査コードは `0x981C6B`（V93 で特定済み・本日再検証一致）**

- **v60 の未解決点**: 「`0xA28860`/`0xA2D184` ほか範囲内任意アドレスへの imm32 参照がファイル全体で0件」＝走査コードを特定できなかった。
- **本日実測**: 実際の境界値への abs32 参照は**存在する**:
  - `abs32(0xA283D0)` = `.text 0x981CB5`（`mov esi, 0xa283d0` @0x981CB4）
  - `abs32(0xA2F77C)` = `.text 0x981CBC`（`mov edi, 0xa2f77c` @0x981CBB）
- → **v60 が探していたアドレス（0xA28860/0xA2D184）が誤り**であり、正しい走査コード = **`0x981C6B`（無名 gap 関数、146 B）のインラインループ `0x981CC5..0x981CD4`**（`mov eax,[esi]; test; je; call eax; add esi,4` を begin/end で走査）。呼出し元 = `WinMainCRTStartup@0x98769C` 内 `0x9877E1`（**起動時に1回**）＝ V93 §c で確定済み・RE_EXE にも正典化済み。
- v60 の補足「判定には影響しない」は維持。`0x981FB4`（各 thunk の第2呼び出し）の役割 = **atexit ラッパ**（V93 補足3: `call 0x981F78` の真偽を `!x-1` に変換、戻り値0=成功）→ v60 の残点2もこれで解消。

---

## 2. v60(c): first-match bug 推定の確定 → **[確定] 機構を完全再現**

再現実測（raw PE `data.find`）:

| 手順 | 結果 |
|---|---|
| ファイル全体の `data.find(b"sSpecialization")` 最初のヒット | offset `0x65C75D` → **VA `0xA5D55D`**（文脈 `el\0Level: \0`**`sClassSpecialization`**`\0\0\0\0sSpecialization` = **部分文字列**） |
| その VA への abs32 参照（.text） | **0 件** → 「push 未検出」を**完全再現** |
| 独立文字列（前バイト NUL） | offset `0x65D394` → **VA `0xA5E194`** |
| `abs32(0xA5E194)` | **.text 1件 = `0x9F0D56`**（= `push 0xa5e194` @0x9F0D55 の imm32） |

- → **v60 の「推定」は確定**: 前回の未検出は first-match が `sClassSpecialization` 内部を返したため。補助要因（push imm32 が4非整列 `0x9F0D56 mod 4 = 2`）も実測どおり。

---

## 3. v63#1: `0x584D10` ラベル → **[確定] 役割（+ 命名案）**

- **役割（v63 §4.2 で証拠づけ済み・本日再確認）**: `0x584D10` = **文字列比較ラッパ（`__stricmp(a,b) == 0`）**。CRT 本体経路 `0x98262D = __stricmp` → `0x982525 = ___ascii_stricmp`。
- **所在**: vtable **`0xA40454`（設定マップ `0xB35574` の vtable）の `+0x08` スロット**。実行は `0x575D2F: mov eax,[edx+8]`（compare = vt[+8] = 0x584D10）で観測。同胞 vtable `0xA40410` / `0xA3BF1C` / `0xA53920` / `0xA64800` も**同一実装を共有**（`0x4A80A0`(hash) とペア）。
- **正典 NO HIT の理由**: Ghidra `decompiled.c` に**関数定義が存在しない**（`FUN_00584CC0` と `FUN_00584D50` の間に未関数化ギャップとして残存）→ 出典は生逆展開＋vtable スロット参照のみ。
- **命名案（根拠つき）**: `MapCompare_StrICase`（仮）— 「string-key map の compare スロット実装・大文字小文字非依存」。正式 `FUN_*` 付与は decompiled に定義が無いため不可。

---

## 4. v63#3: `0xA3BF1C` / `0xA53920` / `0xA64800` → **[確定] 3つとも vtable アドレス（オブジェクトではない）・install 元を特定**

| 値 | 正体 | install（vptr 書き込み）元 | インスタンス |
|---|---|---|---|
| **`0xA3BF1C`** | 文字列キー map の **vtable** | `FUN_0046C430`（@0x46C433）＋ **グローバル `0xB06164` への直接書込**: `mov dword [0xb06164], 0xa3bf1c` @`0x9DBFC2`（`FUN_009DBE90`）/ @`0xA18530`（`FUN_00A18530` = CRT exit リセット） | **グローバル map オブジェクト `0xB06164`**（+0 vtable / +8 bucket ptr=`0xB0616C` / +0x10 ownsKey=`0xB06174`=0 と、書込3本が map ヘッダと一致） |
| **`0xA53920`** | 同上（別クラス） | `FUN_00521450`（**14 callers**: `FUN_00622180`×2 / `FUN_006267A0`×2 / `FUN_006545E0`×2 / `FUN_00511C10` / `FUN_006239D0` …）＋ `FUN_00521590` / `FUN_00521950`（1 caller `FUN_00446D80`）/ `FUN_005219B0`（1 caller `FUN_004491D0`） | **埋め込み map メンバ**（多数のオブジェクトが ctor で inline 構築） |
| **`0xA64800`** | 同上（別系統） | `FUN_00551370` / `FUN_005512F0`（E8 caller 0）/ `FUN_005513A0`（1 caller `FUN_00551D20`） | 別系統の map インスタンス |

- 共通: 全 vtable が **hash `0x4A80A0`（文字列）+ compare `0x584D10`（stricmp）** を共有（v63 §5.4）。`0xA3BF1C` の .text abs32 は ctor 書込のほかに `0x9DBFC8`（init スタブ内）/ `0xA18536`（exit スタブ）の2系統（いずれも `mov [0xB06164], 0xA3BF1C` のストア本体）。
- → 「objects」ではなく **vtable**。正典 NO HIT は本レポートで初記載。

---

## 5. v63#4: `[0xB35580]` 経由のランタイム個数 → **[部分確定] フィールド意味は確定・実数は静的不可能（手許の静的近似値つき）**

- **`0xB35580` = map ヘッダの `+0x0C` = 要素数カウンタ（count）**（v63 §5.5: ヘッダ `+0x00` vtable / `+0x04` bucket 数 401 / `+0x08` bucket 配列 `0xB3557C` / **`+0x0C` count = `0xB35580`** / `+0x10` ownsKey `0xB35584` = 0）。
- **実行時の値 = 静的解析不可能**（依頼の許容条件どおり明記）: 値は「insert 成功回数」であり、(a) 重複キーが `FUN_00419A70` の `"Setting key '%s' already used in map."` エラーでスキップされる、(b) 登録が条件分岐（CRT init /遅延 thunk）経由、(c) 実行中に erase（`FUN_004055E0`）が走るため、静的に確定できない。
- **静的な近似値（参考）**:
  - registrar 呼出試行 = **2,304 件**（`FUN_00419A70` 1,312 + `FUN_0041BAE0` 992、`RE_EXE_REPORT` L9632 の全数解析）
  - 静的登録キー数 = **2,175**（V59 計測。`sSpecialization` の s 系漏れ訂正で **2,176** の可能性 — v60 §7-3 の注記どおり）
  - → **起動直後の count は ≤2,175〜2,176 が上限の近似**。厳密値は実行時計測のみで確定可能。

---

## 6. v64#3: `sRHand:DEFAULT` タイポ / blend パーサのトレランス → **[確定]**

### 6-1. タイポの実体

- `sRHand:DEFAULT`（slot `0xB11B94`、value ptr `0xA56A08`）の既定値文字列 = **`"1.0f, 1.0"`**。他の blend エントリはすべて **`"1.0, 1.0"`**（Ghidra のラベルすら `PTR_s_1_0f__1_0_00b11b94` とタイポを反映）→ **vanilla データの `'f'` 付き誤記**。

### 6-2. パーサとトレランス仕様（実測）

- 読手 = **`FUN_0053A720`**（127行、`sscanf` **22 箇所**、slot `0xB11B94` への参照 @`0x53A9C4` を含む）。本体 L73:
  ```c
  local_4 = local_8 = 0x3F800000;   // 既定 1.0f
  sscanf(PTR_s_1_0f__1_0_00b11b94, "%f, %f", &local_4, &local_8);
  _DAT_00b2e71c = local_8;          // → blend 用グローバルへ
  ```
- **トレランス判定**: `sscanf("%f, %f")` は `"1.0f, 1.0"` に対し、
  1. 1 つ目の `%f` は **`"1.0"` まで**を読む（`'f'` は浮動小数点書式に含まれないため消費されない）→ **第1値 = 1.0（タイポ無視で正しい）**。
  2. 続く書式文字 `,` と入力 `'f'` が**不一致 → 変換中断（戻り値1）** → **第2値は読めず既定 `1.0f` のまま**。
- → **結果は (1.0, 1.0) で結果的に正しい**（第2値が偶然1.0のため）。**トレランス = 部分的**: 第1値は `'f'` を許容、第2値は既定値にフォールバック。**移植時は `"1.0, 1.0"` へ正規化するのが安全**（`"1.0f, 0.5"` のような場合に第2値が silently 1.0 になるため）。

---

## 7. v64#4: thunk の文字列パス → **[確定] string 型の runtime 登録 thunk は存在しない**

- **共有 EH ハンドラ `0x9AB578` を持つ関数は全 .text で 5 件のみ**（`push 0x9ab578` 全走査）:

| 関数 | 役割 |
|---|---|
| `FUN_00404850` | **float 型 runtime 登録 thunk**（`fld dword [esp+0x20]` で float 引数を読む、`call 0x4040E0`。caller 1件 = `FUN_0049D2A0` @0x49D546 = Water 系初回） |
| `FUN_00444060` | **dword 型 runtime 登録 thunk**（`mov ecx,[esp+0x20]`、`call 0x4040E0`。caller 1件 = `FUN_0049DD00` @0x49DD72） |
| `FUN_00419A70` / `FUN_0041BAE0` | GMST/設定マップ registrar（key/value を push で受け取る汎用） |
| `FUN_004055E0` | eraser（`name[0]=='S'` 時のみ free） |

- **string 型の typed thunk はこのファミリに存在しない** → string の登録は (a) 汎用 registrar `FUN_00419A70/0x41BAE0`（静的 CRT モジュール `fun_009Exxxx` 帯の push 群）、(b) 静的初期化（V51 の572件に包含）のいずれか。**runtime 専用の string thunk は不要**（float/dword のみが遅延登録を必要とした）。
- → v64 §5-4 の主張を**独立系（EH ファミリ全数）で裏付け**。

---

## 8. v69#2: `0x66C580` callers ~22 件の文脈 → **[確定] 本バンドル依頼1（v283）で確定済み・統合**

handoff の許容「依頼 1-4 と重複する場合はそちらでまとめて可」に従い、**`tmp/re/v283_v62_residuals4.md` 項目4** に統合済み:

- E8 callers = **22 件全数分類**: A群12件（ecx に `[0xB333C4]` を明示ロード）＋ B群10件（同クラスのメソッド this 経由）。
- 本体 `FUN_0066C580(this, x)` = **`this+0x588 = (x==0)` の反転セット**＋副作用（`vcall +0x198` 時の float グローバル転送 / `FUN_0066B710` / `FUN_00664F70`）、戻り値 = 変化したか。
- 呼出しパターン5型（現在値の保存/復元・再適用・条件反転・明示0/1・ガード付き）を全数表化 → v69 §5-2 の残点解消。

---

## 9. v69#3: opcode table `entry+0x10` フラグの意味 → **[確定]（V83 構造 + V99 意味 + 本日のサブテーブル計測）**

- **構造（V83）**: `+0x10` は単一フィールドではなく **`{u8 flagA, u8 unused(+0x11)=0, u16 argn(+0x12)}` のパック**（u32 nonzero 408 = flagA 272 ∪ argn 281）。
- **意味（`tmp/re/b03_flaga_meaning.md` = V99）**: **flagA = 「参照必須（reference-required）」フラグ** — 1 のエントリは暗黙の宿主参照を必要とし、quest スクリプトでは明示 `ref.` が無いとコンパイラが拒否（エラー3種）。消費者は .text 全走査で **7 箇所**（`0x516AA1`, `0x56ACB8` 等、`b03` で全数特定）。
- **本日のサブテーブル計測**（v69 の対象 = `base 0xB0C8C0` = idx163..531 の **369 エントリ**、範囲 `0xB0C8C0–0xB10268`、`CTDA=entry+0x20` は既存確定）:

| 項目 | 分布 |
|---|---|
| **flagA（byte0）** | **1 = 235 件 / 0 = 134 件**（計369） |
| **argn（+0x12）** | **0:166 / 1:139 / 2:53 / 3:5 / 4:3 / 5:3**（計369） |
| サンプル | idx0 `MessageBox` +0x10=0x10000（flagA0, argn0）/ idx1 `GetDistance`=0x10001（flagA0, argn1）/ idx2 `AddItem`=0x20001（**flagA1**, argn1）/ idx368 `ClearOwnership`=0x1 |

- → v69 §5-3 の「部分推定のみ」は**確定に更新**（構造=V83、意味=V99、サブテーブル内訳=本計測）。

---

## 残点（記録）

1. v63#1 の命名案は仮（decompiled に定義が無く正式 `FUN_*` 付与不可）。
2. v63#4: 起動直後 count の**厳密値**（実行時計測が必要。静的近似 2,175〜2,176 まで）。
3. v63#3 の3 map の**各インスタンスの用途名**（`0xB06164` / 埋め込み群 / `FUN_00551D20` 系）。
4. v64#3: blend 値の移植正規化（`"1.0f, 1.0"` → `"1.0, 1.0"`）は移植側タスクで反映要。

## 証跡（すべて `%TEMP%\opencode\` または `tmp/re\`）

| スクリプト / 入力 | 内容 |
|---|---|
| `v288_p6.py` + `_out.txt` | v60(c) first-match 再現 / v60(a) 境界 abs32 再検証 / v63#3 vtable store・abs32 / v69#3 サブテーブル369件の flagA・argn 分布 |
| `v288_p6b.py` + `_out.txt` | v63#3 install ctor の caller / `0xA3BF1C` CRT スタブ窓 / v64#3 slot 実読・読手特定 / v64#4 thunk 本体 |
| `v288_p6c.py` + `_out.txt` | v64#3 `FUN_0053A720` 本文（sscanf 22 箇所・L73 の `PTR_s_1_0f__1_0_00b11b94`）/ v64#4 EH `0x9AB578` ファミリ5件 |
| inline probe | thunk → `FUN_004040E0`（637 callers）/ blend slot `0xB11B94` = `{0xA56A08="1.0f, 1.0", 0xA56A14}` / `FUN_0053A720` の `b11b94` 参照確認 |
| `tmp/re/v93_gmst_gt_residuals.md` §c / `v63_…md` §4-5 / `v64_…md` §1-5 / `v69_…md` §5-6 / `b03_flaga_meaning.md` | 既存成果物（読取・引用） |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。内部整合: first-match 再現（VA・abs32 0/1件）/ 境界 abs32 2/2 = V93 一致 / EH ファミリ5件 / サブテーブル 235+134=369・argn 合計369 ✓ / blend slot 値と Ghidra ラベル一致。
