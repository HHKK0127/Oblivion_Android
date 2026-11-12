# v201_opcode_gt_residuals3.md — opcode/GT 残点 第6束（V188）

- 作成日: 2026-10-29 / 担当: TODO serial 188 (V188・再試行)
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- スコープ: `TODO_re49.md` serial 188（V188）行
  1. %変数レコード +0x10（'f'/'s' 判定 byte）の初回書手（登録帯 GetPrivateProfileString L134586 帯に縮小済み）
  2. `base+0x58` bit7 と bit22 の正式フラグ名（bit16-18=ペイロード有・bit31=無効センチネルは確定済み）
  3. MagicItem slot[6] 値 4 の帰属クラス（base=pure-call / Enchant=6 / Alchemy=7 / Ingredient=8 済み）
- 契約: `tmp/re/SUBAGENT_CONTRACT.md`（読取のみ・成果物は本ファイルのみ・docs/・TODO・台帳・git・Supermemory・sync は編集禁止）
- 出発点: `tmp/re/v198_opcode_gt_residuals2.md` §4（残留 3 件）と §2（確定済み: TESSaveLoadGame / seta 消費 / +0x58 デュアリティ / slot[6] 型列挙 / MagicCaster）
- 元の残点表: `tmp/re/v178_runtime_residuals3.md` §2.5 / `tmp/re/v177_gt_slot_residuals2.md` §7（V-177b/V-177c）

## 1. 手法 / 入力

- capstone 5.0.7（detail 付き Cs）で逆アセンブル。`%TEMP%\opencode\probe_harness.py`（selftest 4/4 PASS）＋ `cache/`（opcode_table.json 533行 / call_targets.json / vtable_index.tsv / named_funcs.txt）。
- decompiled（29 MB）は該当行の部分読取のみ（GT §2 規則: `python enumerate(f,1)` 部分検索）。
- 中間スクリプト: `%TEMP%\opencode\v201_*.py` + 出力 `v201_*_out.txt`。
- 既存中間物: `%TEMP%\opencode\v198_1..7_*.py`（前タスク）を再利用。
- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時確認済: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。

## 2. 残点1: %変数レコード +0x10（'f'/'s' 判定 byte）の初回書手 — **[確定]**

### 2.1 初回書手の同定（型宣言側）

- **書手 = `FUN_004FF710` @`0x4FFB0C` `88 45 10  mov byte [ebp+0x10], al`**（`%TEMP%\opencode\v201_8_out.txt` / `v201_21_out.txt`）。直後の `0x4FFB0F call 0x4028d0`（ecx=`[ebp+0x18]` = name 格納先）と一对で、**レコード新規生成時の初期化**であることを disasm で実測:
  ```
  0x4FFABB  push 0x20 / call 0x401F00        ; malloc(0x20) — 0x20 セル（+0x10 flag / +0x18 name）
  0x4FFAD7  call 0x517A80 → ebp = eax        ; ctor
  0x4FFAE2  mov eax, [esi+0x410]             ; esi = param_3（prologue 0x4FF75D で第3スタック引数）
  0x4FFAE8  cmp eax, 0x14 / je → al=0
  0x4FFAF5  cmp eax, 0x1F / je → al=0
  0x4FFAFA  mov eax, 1                        ; それ以外 → al=1
  0x4FFB0C  mov byte [ebp+0x10], al           ; ★ +0x10 初回書手
  0x4FFB0F  call 0x4028d0                     ; [ebp+0x18] ← name（local_1150）
  ```

### 2.2 decompiled 直接対応（L198266–198299、1:1 一致）

- `FUN_004ff710` の `switch (param_3[0x104])` のうち **`case 0x12 / 0x13 / 0x14 / 0x1f` が変数宣言（"Missing variable name in variable declaration."）**:
  ```c
  L198277  local_1158 = FUN_00401f00(0x20);            // malloc(0x20)
  L198283  piVar5     = FUN_00517a80();                // ctor
  L198286  if ((param_3[0x104] == 0x14) || (param_3[0x104] == 0x1f)) uVar2 = 0;
           else                                          uVar2 = 1;
  L198292  *(undefined1 *)(piVar5 + 4) = uVar2;         // piVar5+0x10 ← ★asm 0x4FFB0C と一致
  L198293  FUN_004028d0(local_1150, 0);                 // name コピー（0x4FFB0F と一致）
  L198294  FUN_004faa90(piVar5[6], piVar5);             // 'f'/'s' 重複確認（v193 の式と同一）
  L198296  新規のとき ID 採番 → FUN_0067b1e0 で挿入
  ```
- → **判定値: 現在の型トークン `param_3[0x104]` ∈ {0x14, 0x1F} なら 0（'f'=float）、{0x12, 0x13} なら 1（'s'=string）**。v193 の「+0x10 は boolean（0='f'/非0='s'、neg/sbb/and 0xD/add 0x66 の読手式）」と**完全整合**。

### 2.3 既存記載の訂正（2 件）

1. **v198 の「compiler band 0 件（C6 形）」は走査漏れ**: 本書手は **`88 45 10`（reg-source 形）**であり、v198/v193 の `C6 disp8` 即時走査には捕捉されない。`v201_6` の 88 形全走査では band 0x4F0000 に 2 件（`004FA95C`=clone 内の複写 / **`004FFB0C`=本初回書手**）を検出。「band 内 0 件」は形式限定の結果であり**在不在の判定には使えない** [訂正]。
2. **v198 の「登録帯 GetPrivateProfileString L134586 帯に縮小」は誤り**: L134586 帯 = `FUN_004a8800`（SettingCollection の INI 読取、switch(uVar1) 0..8）であり、`v201_4` の全文 disasm で byte store は **`[esi]`（value 本体）のみ**で `[reg+0x10]` は無い ＝ **INI 帯は %変数レコード +0x10 を書かない**。%変数の型宣言はスクリプトコンパイラ帯（`FUN_004ff710` case 0x12..0x1f）が担う [訂正]。

### 2.4 整合（v193 / v193b）

- v193 の 'f'/'s' 式（`FUN_004faa90` L194383 / `FUN_004fd5f0` L196975）は**読手**、本件 0x4FFB0C は**書き手**で、L198294 が生成直後に同一 `FUN_004faa90` を呼ぶことから同一 byte を扱うことが確定。
- v193b V-193-1 の `FUN_004FDAF0` type-code 体系（0x04/05/06/09/0A/20/39/3B）は**別系統**（ローカル式ノードの type-code）であり、+0x10 の boolean とは無関係（v193b の問いは本件で**分離**）。

## 3. 残点2: `base+0x58` bit7 と bit22 の正式フラグ名 — **[確定(機能) / ESM 定数名は B13・資料依存を除く]**

### 3.1 bit7 (0x80) = 「base フォーム無効化ビット」[確定(機能)]

- **bit7 は全 reader で同一文脈**: `base = [this+0x1C]` の `[base+0x58]` bit7 が立つと、このオブジェクトの**値操作・評価が閉じる**（本タスク `v201_5`/`v201_22` で全 reader 文脈 disasm）:

  | reader | bit7 立ちの挙動（実測） |
  |---|---|
  | `FUN_004133C0` / `FUN_004133E0`（getter/setter 対） | getter→**0 返し** / setter→**拒否**（v177 と一致） |
  | `FUN_00413FC0` @0x41405C | 0 返し → 直後の `[esi+0xC]` 格納が**スキップ**（`fst [esi+0x20]` も不発） |
  | `FUN_00413BE0` @0x413ECA/ED5 | 2 連続 bit7 テスト、`jne 0x413f2c` = **文字列出力パス除外**（この関数は `%s %s` / ` %d %s` の name 組立） |
  | `FUN_0068E670` @0x68E949 | 立ちなら float 比較（`[esi+4]` vs `[esi+0x1c]`）を**飛ばす**（= base 無効時は評価しない） |

- さらに **bit9 (0x200) が同型の対 exists**（`FUN_00413370/0x413390`: base bit9 立ちなら `[this+0x8]` を返さない/セットしない。`v201_22(f)`）→ bit7/bit9 は**「base 側 flags による値ゲート」ファミリ**であり、bit7 はその主系（+0xC = formID を守る、callers 9 件、v177/v163 GT と一致）。
- **bit7 が ESM 由来である裏付け**: `or/and [reg+0x58], imm` に bit7 マスクは**0 件**（v193b H.3 ＋本タスク `v201_9` §2 再走査で一致）→ 実行時に set されない＝レコード/データ読み込み時に埋まる値。
- **正式名の裁定**: 「base フォームが無効化されている間、リンク先フォームの formID ゲート（get/set）と派生値の適用を閉じる」＝ **base-disabled guard bit** [確定(機能)]。ESM record flag の定数名（0x80 の対応語）は**実機 .data ダンプ（B13）・外部資料依存**として本タスクの分析可能分から除外（GT §6 Blocked 指示と整合）。

### 3.2 bit22 (0x400000) = 「エフェクト判定バイパス／資格拒否ビット」[確定(機能)]

- **reader 全体像**（`v201_9` §5: 13 箇所）。うち 12 本は `0x414EB0..0x415306` の**同型リストチェッカ群**（`this+4/+8` の effect リストを `add ecx,-4` 歩行し、各 elem `[[elem+0x1C]+0x58]` の bit22 を探す）。代表 2 本の機能:
  - **`FUN_00414EB0`（毒判定）**: bit22 立ち要素は**素通し（判定対象外）**、非立なら `FUN_00413470` 述語で判定し、1 つでも偽なら即 return 0（v197 §2 / TODO R-4 と一致）。
  - **`FUN_00415180`（唯一 caller `0x69919E`）**: リスト内に bit22 フォームを含むなら **1** → `FUN_00699190` は `[esi+8]=5`（busy 拒否）（v193 と一致）。
  - 第 13 本 `FUN_00419B30` @L16556: エフェクト選択ループの除外条件 `((*(uint*)(piVar1[7]+0x58) >> 0x16 & 1) == 0)` = **bit22 立ちは選択から除外**。
- **bit22 は本 EXE に書手が存在しない**（供給源の確定）:
  - `or/and [reg+0x58], imm32` に bit22 マスク **0 件**（`v201_9` §2: 3 件は `and 0xFFDFFFFF` / `or 0x00201000` / `or 0x00200800`）。
  - `mov [reg+0x58], imm32` の notable 17 件は vtable ptr・float リテラル（別クラスの同 offset、混在注意）。
  - **MGEF 登録 flags 161 件に bit22 は 0 件**（`v201_17`: `flags with bit22: 0`）。
  - → **bit22 は ESM レコードデータ由来**（実行時コードでは決して立たない）＝ v193b H.3 の「高ビットは ESM 一括インポート由来」と完全一致 [確定(供給源)]。
- **v193 との接続（bit21/bit30/bit31）**:
  - **bit21 = known MGEF**: set = Player LoadGame `*(knownEffectSetting+0x58) |= 0x200000`（v193b L15470）。**clear = 本タスク新検出 `FUN_004167E0` @`0x416854  and dword [edi+0x58], 0xFFDFFFFF`**（MGEF レジストリ `0xB33508` を走査し、外れる form から known マークを落とす。`v201_20` §3）。set/clear 双方が揃い bit21 は**確定強化**。
  - **bit30** = `FUN_004AE590`（`shr eax,0x1e; and al,1`）／**bit31** = `FUN_004AE5A0`（`test 0x80000000; setne`）— v193b H.1 と一致（本タスクで callers 文脈の再計測なし・既存値を引用）。
  - 5 ビット（bit7/21/22/30/31）は**同一 `+0x58` dword に分散テスト**される（v177 §2.24 と本タスク走査の一致）。
- **正式名の裁定**: 機能名 = **「エフェクトリスト判定のバイパス（毒判定/選択から除外）と資格ゲート（busy 拒否）を兼ねる ESM 由来ビット」** [確定(機能)]。GECK/ESM の record-flag 定数名は B13・外部資料依存として除外（bit7 と同扱い）。
- ⚠️ **混在注意（本タスクで排除）**: `0x400000` は (a) MGEF/フォーム `+0x58` bit22、(b) ExtraDataList ストリームの 4B 読取フラグ（`0x4273CD`、v203）、(c) `FUN_005976B0` の `add [esi+0x58], edi` 等、別物が同数値を使う。本判定は (a) のみ。

### 3.3 本タスクでの補助実測

- `FUN_00413470`（毒判定の基準本体、TODO R-4 の残）= **bit0 述語**を特定（`v201_22(e)`）: `[ecx+0x18]` が有れば `[+0x14]` byte、無ければ `[base+0x58]` の **bit0** を返す（callers 15 件）。v197 の「毒判定基準」の正体はこれ [確定]（bit0 の対応する GECK 名は B13 依存で [残]）。

## 4. 残点3: MagicItem slot[6] 値 4 の帰属クラス ＋ 表示名3件突合

### 4.1 slot[6] 値 4 = **SpellItem の動的型フィールド経由のみ** [確定]

- **負証拠の二重確認**: (a) v193b: 全 1715 vtable の slot[6] に `mov eax,4; ret` = **0 件**。(b) 本タスク `v201_10` §1: 全 vtable slot[6] の定数 return スキャン（`B8 imm32 C3` / `6A imm8 58 C3`）→ 定数 60 件の内訳は `{0, 1, 6, 7, 8}` のみ、**`4` は 0 件**、`push 4; pop eax; ret` も **0 件**。
- **帰属クラスの同定**: MagicItem 系 16-slot の slot[6] impl を対比（v198 §2 表）:

  | クラス | slot[6] impl | 戻り値 |
  |---|---|---|
  | MagicItem (base) | `0x982D85` | pure-call（未定義） |
  | **SpellItem** | **`0x4D9B40`** = `mov eax,[ecx+0x1C]; ret` | **動的（型フィールド）** |
  | EnchantmentItem | `0x7AFE90` | `mov eax,6; ret` |
  | AlchemyItem | `0x4059C0` | `mov eax,7; ret` |
  | IngredientItem | `0x419750` | `mov eax,8; ret` |

  → **4 を返し得るのは SpellItem の `[MagicItem-sub +0x1C]`（＝ SpellItem 本体 +0x34）のみ**。そのフィールドの存在自体は **SpellItem ctor `FUN_0041D420` で実測**: vtable `0xA34C3C`（SpellItem primary）/ `[esi+0x18]=0xA34BF4`（MagicItem 16-slot sub、slot6=0x4D9B40）を代入した上で **`0x41D46D  mov [esi+0x34], eax`（eax=0、decompiled L19198 `param_1[0xd] = 0` と 1:1）＝ ctor は +0x34 を 0 に初期化**（`v201_23`）。命令境界も capstone で確認（非整列罠なし）[確定(構造)]。
  - ⚠️ 訂正（probe 中の仮説を棄却）: 0x41D46D は「引数から型コードを設定」ではなく **0 初期化**。型コード（1/4/5…）を書くのは ESM ロード経由の別書手で、その特定は本タスクで至らず [残（下記 §5-3）]。
- → **判定: 値 4 の帰属クラス = SpellItem（動的型コード）**。Enchant=6 / Alchemy=7 / Ingredient=8 が定数スタブであるのと対照し、4 は定数スタブを持たない＝**v193 の「1715 中 0 件」負証拠と完全整合**。

### 4.2 値 4 の消費者文脈（本タスク decompiled 実測 3 件）

| 消費者 | `==4` の挙動 | 出典 |
|---|---|---|
| `FUN_00699190` | `sub eax,1; je`（==1 → SpellItem dynamic_cast → vtable[1]）／**`sub eax,3; je`（==4 → 同 dynamic_cast → vtable[0]** = 別受入パス） | `v201_10` §4 |
| `FUN_00690a00` | `iVar5==4` → `fVar6=0`＝**`SpecialIdle_HitEffect` を使わない**（L459039-45） | `v201_12` 12.5 |
| `FUN_005239c0` | `iVar2==4` → `obj+0x24` の effect リストを走査して **fourcc `"VAMP"`（0x504D4156）** ノードを探す → `FUN_00413320`（L217630） | `v201_12` 12.5b |

- → 4 は「Spell(1) と並ぶ SpellItem 型コードの一つ」であり、消費者は**特殊型（ヒットエフェクト無効・Vampirism 照合）**として扱う。**`"VAMP"` fourcc との対応は「4 = Vampirism 系 disease 型 spell」を示唆 [有力]**（`FUN_005239c0` は `param_1[3]!=7` のときのみこの照合を行う）。厳密な型名（ESM SPIT 値 4 の対応語）は ESM データ/B13 依存で [残]。

### 4.3 表示名3件の突合（監視項目・読取のみ）

- 連託元が人間承認済みで適用済みとされる3 件を、**現行ソースから実読み**（`2026-10-31`）:

  | 箇所 | 現行内容 | 判定 |
  |---|---|---|
  | `native_scda_decoder.cpp` **L644** | `case 0x1083: return "GetPCIsSex";` | ✅ 適用済 |
  | `native_scda_decoder.cpp` **L706** | `case 0x10E3: return "HasVampireFed";` | ✅ 適用済 |
  | `native_scda_decoder.cpp` **L763** | `case 0x1132: return "IsActorUsingATorch";` | ✅ 適用済 |
  | `script_vm_tests.cpp` **L2424** | `getNativeOpcodeName(0x10E3) == "HasVampireFed" &&` | ✅ 適用済 |

- **(a) 適用状況**: 4 箇所すべてに指定の表示名が正しく適用されている。
- **(b) 二重修正の有無**: 本タスクの成果物は +0x10 初回書手 / +0x58 bit7・bit22 / slot[6] 値4 の 3 残点のみがスコープで、**opcode 表の表示名（opid ↔ name 対応）に触れる記述を一切含まない**（§2〜§3 の証拠は VA・decompiled 行・フラグビットのみ）。よって上記 4 箇所と**矛盾せず、二重修正は発生しない** [確認]。

### 4.4 結論（3行）

1. **%変数 +0x10 初回書手は確定**: `FUN_004FF710` case 0x12/0x13/0x14/0x1f（変数宣言）@`0x4FFB0C` が `param_3[0x104] ∈ {0x14,0x1F}` → 0('f') / それ以外 → 1('s') を書く（decompiled L198266-198299 と asm が 1:1）。v198 の「compiler band 0 件」は C6 形限定の走査漏れ、「INI L134586 帯」説は誤りで**ともに訂正**。
2. **bit7/bit22 は機能名で確定（ESM 定数名は B13 依存を除外）**: bit7=**base-disabled guard**（base bit7 立ちで formID getter=0 / setter 拒否 / 格納・列挙・float 評価が閉じる、bit9 と対のゲート系）、bit22=**エフェクト判定バイパス／資格拒否**（毒判定素通し・選択除外・busy 拒否、**書手 0 件＋登録 161 件に 0 件＝ESM 由来**を実測）。bit21 は set(Player LoadGame) ＋ **clear(`FUN_004167E0` @0x416854) の双対を本タスクで新検出**し確定強化。
3. **slot[6] 値4 = SpellItem の動的型コード**: 全 1715 vtable の定数スタブ（`B8 imm C3` / `push 4 pop ret`）に **4 は 0 件（二重走査）**、返し得るのは SpellItem の `[sub+0x1C]`(=本体+0x34、ctor `FUN_0041D420` で 0 初期化) のみ。消費者 3 件は 4 を「特殊型」として扱い（ヒット特殊 idle 無効・`"VAMP"` fourcc 照合）、v193 の負証拠と整合。表示名3件＋テスト1件は現行ソースで適用済・二重修正なし。

## 5. 残留不明点

1. **型トークン {0x12,0x13,0x14,0x1F} のキーワード名**: 'f' 系 (0x14/0x1F) / 's' 系 (0x12/0x13) の対応語（スクリプト型宣言の具体語）は `FUN_004ff710` の block-type 表 `DAT_00B0AF48` 帯と未照合（"begin" 系とは別表）。→ 次手: 0x104 状態番号の名前テーブル照合。
2. **bit7 (0x80) / bit22 (0x400000) / bit0 の ESM・GECK record-flag 定数名**: 機能名は §3 で確定したが、対応する公式フラグ語は実機 .data ダンプ（B13・GT §6 Blocked）・GECK 文書依存。bit7 が立つフォームの formType 分布（B13）で命名を閉じる。
3. **slot[6] 値4 の型コード対応語と +0x34 書手**: 「4 = Vampirism 系 disease 型 spell」[有力]（`"VAMP"` fourcc 照合と `SpecialIdle_HitEffect` 無効の 2 文脈）だが厳密名は ESM SPIT 値依存。**SpellItem +0x34 へ型コード（1/4/5…）を書く本体**は `FUN_00447DF0` の dword-store **0 件**を実測済みで、ESM バッファ読出経由（store が別形式/別関数）と推定・未特定。
4. **`FUN_00419B30` の `type==1 → fourcc "DISE"` 分岐**: type1=Spell（v198 確定）と "DISE"（疾病）レジストリ対応の意味論（`0xB33508` レジストリ fourcc の役割）は本タスクで深掘りせず。
5. **v177 由来の他残点**（ExtraStartingPosition node+8 読手 / +0x11D 値5 / +0xE4..F0 元素型 / 馬警報トリガ）は本タスク範囲外（v193 §4 の継続）。

## 6. 自己検証

- **`probe_harness.py selftest` → 4/4 PASS**（着手時・**完成直前の 2 回**: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- **内部整合（一致 9 項目）**:
  1. `FUN_004ff710` の書手 asm（`[esi+0x410]` cmp 0x14/0x1F → al、`[ebp+0x10]`、name `0x4028D0`）と decompiled L198286-198293 が **1:1**（malloc 0x20 / ctor `0x517A80` / `param_3[0x104]` も一致）。
  2. `callers(0x4FF710)=1`（`0x500237` in `FUN_00500170`）・`callers(0x4133C0)=9`（v177 の 9 件と一致）。
  3. slot[6] 定数スキャン: `-> B8 4 / -> B0 4` を grep で **0 件**＋`push 4;pop eax;ret` 0 件＝v193b の 1715 行走査と二重一致。
  4. MGEF 登録 161 件の bit22 = **0 件**（v201_17）＋ `or/and [reg+0x58], imm` の bit22 マスク 0 件（v201_9 §2 再測定）→ ESM 由来判定の二重確認。
  5. `FUN_004167E0` @0x416854 の mask `0xFFDFFFFF` を ~`0x00200000` と分解して **bit21 クリア**と確定（v193b の bit21 set と対）。
  6. SpellItem ctor `FUN_0041D420`: vtable `0xA34C3C` / sub `0xA34BF4` が v198 §2 表と一致、`0x41D46D` は命令境界確認済（`eax=0`＝decompiled L19198 と一致）。
  7. bit7 reader 4 文脈（0x4133C0/E0・0x41405C・0x413ECA・0x68E949）が全て「base=+0x1C → [+0x58] >>7」同型。
  8. 表示名 4 箇所（L644/L706/L763＋test L2424）を**実読み**で一致確認（引用でなく現行ファイルの当該行）。
  9. slot[6] 値4 の消費者 3 件を decompiled 直読（L459039-45 / L217630）＋ asm（`sub eax,1 / sub eax,3`）で突合。
- **制約遵守**: 成果物は本 1 件のみ（追記・§1 無改変）。docs/・`TODO_re49.md`・GT・台帳・plan・cache・git・Supermemory・sync は**編集 0 件**。移植ソースは**読取のみ 4 箇所**（L640-770 / L2418-2432 ほか）。ESM アクセス **0 回**。中間物は `%TEMP%\opencode/v201_1..23_*`。

## 7. 証拠一覧（本タスク生成・読取）

| 区分 | ファイル |
|---|---|
| probe（本セッション新規） | `%TEMP%\opencode/v201_20.py`（decompiled FUN_004ff710＋bit22 clear＋bit7 callers）/ `v201_21.py`（esi 同定＋case 0x11- 帯＋bit22 checker）/ `v201_22.py`（bit7 reader 4 文脈＋`FUN_00413470` 同定）/ `v201_23.py`（SpellItem ctor 境界確認）＋対応 `v201_20..23_out.txt` |
| probe（前セッション再利用） | `v201_1..19_*.py` ＋ `v201_*_out.txt`（19 本・371KB。特に 4=INI 帯全文 / 6=88 形全走査 / 8=`0x4FFB0C` disasm / 9=`[reg+0x58]` inventory / 10=全 vtable slot6 定数スキャン＋`FUN_00699190` / 11-12=型4 消費者 decompiled / 17=登録 161 件の flags 解析）、`v198_1..7_*.py`（前タスク） |
| decompiled（部分読取） | `docs/Oblivion_decompiled.c` L198130-198400（`FUN_004ff710`）/ L19176-19220（SpellItem ctor）/ L16519-16572（`FUN_00419B30`）/ L458990-459060（type4 消費者 A）/ L217590-217665（type4 消費者 B）/ L407950-408040 |
| 既存ノート（読取） | `v198_opcode_gt_residuals2.md` / `v193_opcode_gt_residuals.md` / `v193b_opid29_supplement.md` / `v178_runtime_residuals3.md` §2.5 / `v177_gt_slot_residuals2.md` §5・§7 / `v197_magic_residuals2.md` / `v192_havok_magic_residuals.md` / `GT_CHEATSHEET.md`（§4.1・§5・§6）/ `SUBAGENT_CONTRACT.md` |
| キャッシュ（読取） | `cache/named_funcs.txt` / `cache/vtable_index.tsv` / `cache/opcode_table.json`（既存物のみ・再走査なし） |
| 表示名突合（読取のみ） | `app/src/main/cpp/script/native_scda_decoder.cpp` L640-770 / `app/src/main/cpp/tests/script_vm_tests.cpp` L2418-2432 |

## 8. Android 移植への示唆（実装しない）

%変数レコードの型 byte は「変数宣言時（コンパイラ case 0x12-0x1f）に一度だけ決まり、それ以降読手式 `'f' + (flag?0x0D:0)` で分岐する」単純な 2 層設計なので、移植版では生成時 1 関数（型トークン → bool）に集約すればよく、INI 読取帯（SettingCollection）とは**完全に独立**であることを再現すべき。`base+0x58` の bit7/bit22/bit21/bit30/bit31 は「実行時コードでは一切 set されない ESM 由来ビット」であり、移植版でも読み取り専用の flags ワードとして扱い（bit21 のみ runtime set/clear 双対あり）、bit7 ゲートは formID get/set の入口、bit22 はエフェクトリスト判定のバイパス兼資格拒否として再現すれば原挙動と一致する。slot[6] は「base=未定義・派生は定数または動的型コード」の列挙 getter として再現し、値4（Vampirism 系と推定）は定数スタブを持たない**動的経由**であることを型で表現すると、1715 vtable 走査で得た負証拠（4 の定数スタブ不在）と矛盾しない移植になる。
