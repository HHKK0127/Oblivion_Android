# v318 — v314 由来残点 5 件の裁定（次束H / ai-todo #83）

- 対象バイナリ: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam、ImageBase 0x400000)
- セクション: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` 0xA28000→0x627200 (0xD9A00) / `.data` 0xB02000→0x700C00 (0x30C00, **VirtualSize 0xA9C1C** → VA 0xB32C00..0xBABC1C は BSS)、`file = va - VA + fileOff`（re_util 経由は `pe.get_offset_from_rva(VA-0x400000)`）
- 手法: capstone 5.x 線形 disasm（md.detail 既定）+ CC ラン境界 walk + raw E8/E9 検索（`va+5+rel==target`、境界検証付き）+ find_dword 全セクション + IAT 解決（pefile import parse）+ BFS 再現スイープ（v314_05 と同一 roots/除外）
- 中間物: `%TEMP%\opencode\v318_01..07 系 .py/.out`（17 本・すべて再現可能。リポジトリ側は本 md のみ新規作成）
- 帯遵守（v319 lane と非重複）:
  - 深解析帯: `0x404000-0x406000` / `0x44F000-0x452000` / `0x51B000-0x51C000` / `0x570000-0x574000` / `0x500000-0x504000`
  - **明示調査項目のため帯外・単関数 read-only**: `0x44DCF0`（項目3。v319 禁止帯にも非該当。帯外であることを明記のうえ本項目のみ深読）
  - **「.data 0xB0BF78 / 0xB33E9C の参照は全区間読取可」条項で読取**: 0xB0AF50 系参照（0x4FF4C0 / 0x4FCCE0 / 0x4FCA70 帯外・直近文脈）、impl 列参照 0x5169A0（帯外・直近文脈）、0x9DC2A0/0x9DC2BC（CRT 初期化域）、0x47D170（書手、帯外浅層）、0xB33E90 参照 4 site（直近文脈のみ）
  - 帯外浅層（VA＋直近文脈のみ）: 0x447050 head 36 命令、0x4FD7C0 head、0x47D170 head/tail、0x450550/0x4507D0/0x450710 等の境界バイト（v313/v316/v317 既記数値の再確認）
  - **v319 禁止帯（0x430000 / 0x452000-0x467920 / 0x4D8000-0x4F2000 / 0x5E1000-0x5E2800 / 0x5F0000-0x5F1000）は深解析 0 件**。該当する off-by-pad 例（0x45B300 / 0x452330 / 0x45CC60 / 0x466D50）は**直前 16B＋先頭 1 命令の境界確認のみ**（浅層・既記数値の再現）。0x5AE190 も帯外・境界バイトのみ。
  - 機械走査（.text raw u32 スキャン、E8/E9 ターゲット集合構築）は全帯に適用したが、これは命令境界検証を伴うデータ走査であり深解析ではない。
- クリーンルーム維持（OpenTES4 / xOBSE / COEF 不使用）。ESM 禁止・git 禁止・`re_util.py` 変更なし・既存成果物の上書きなし。実 .ess 不在（動的要項目は [未確定] 明記）。
- 注記: `OPENCODE_REVERSE_ENGINEERING_INSTRUCTION.md` の「成果物は v309_*」は旧指示（本割当優先・指示ファイル不変更）。

---

## 1. 解析対象と VA 範囲

| 項目 | VA / 範囲 | 帯 |
|---|---|---|
| 項目1: 0x502500 ディスパッチ基址 | .data テーブル 0xB0AF50..0xB0C8A0（+ 第二グループ 0xB0C8C0..0xB1026C）、参照 fn 0x4FF4C0（帯外・条項）、0x4FCCE0（帯外・条項）、実行時 0x5169A0（帯外・条項）、帯内 0x50038F 周辺 | 条項/帯内 |
| 項目2: [0xB33E9C] | .text 参照 164 件全数（0x401000..0xA27E00 全域＝条項）、初期化 0x9DC2A0..0x9DC2CD、書手 0x47D170..0x47D253 | 条項 |
| 項目3: 0x44DCF0 | **0x44DCF0..0x44E69E**（帯外・明示項目・単関数 read-only）＋帯内 caller 0x44F200..0x44F240 | 帯外(明示)/帯内 |
| 項目4: 間接 call 595 | BFS roots 0x5723E0/0x57B8E0 の到達 723 fn（帯・禁止帯 cut は v314_05 と同一） | 再現 |
| 項目5: walker | 失敗型 7 例（うち禁止帯 4 例は境界浅層）＋帯内 head 全数スイープ（5 帯 303 head） | 帯内＋浅層 |

---

## 2. probe 一覧（17 本）

| probe | 内容 | out (B) |
|---|---|---|
| v318_01 | テーブル anchor find_dword / row グリッド検証 / セクションヘッダ / .text raw u32 窓走査 / call[reg+0x10] 探索 | 35772 |
| v318_01b | row グリッド歩行（T・行数・行内容）/ index テーブル dump / 0x50038F fn / 帯外 table 参照の直近文脈 / slot-call 探索 | 51477 |
| v318_01c | row-table consumer A/B head+窓 / helper callers / disp32 slot-call 探索 / anchor imm 検索 | 10323 |
| v318_01d | consumer B 0x4FF4C0 の head 起点クリーン decode / 0x5000C7・ジャンプテーブル 0x5000F4 | 9986 |
| v318_01e | 0x4FF7C0.. 連続 decode・helper call 確認 / callers / ジャンプテーブル entries | — |
| v318_02 | [0xB33E9C] 全 164 参照 census・命令分類・CRT 初期化文脈 | 13256 |
| v318_02b | 分類の境界厳格化（164/164 命令境界一致）/ 0xB33E90 参照 / FUN_0047D170 浅層 | — |
| v318_03 | 0x44DCF0 全文 disasm（1430 命令）・ret/eax 書手列挙 | 80407 |
| v318_04 | 595 間接 call 再現＋初回分類 | 3561 |
| v318_04b | 由来追跡付き分類（slot/global/imm/未解決）＋log-context deep context | 14020 |
| v318_05 | fn-head 規則 old vs new・fn-end walker old vs new・帯内 D スイープ初回 | 10218 |
| v318_05b | 境界 ambiguity dump / caller 確認付き refined walker | 17865 |
| v318_05c | **rule v3**（内部分岐ターゲット判別）ラベル検証＋D 再スイープ | 9894 |
| v318_05d | v3>old 差分の個別 spot-check | 9401 |
| v318_05e | 残差分の callers/bytes 検証 | — |
| v318_06 | 0x447050 head（浅層）/ 0x44E6D0 テーブル尾 / [0xB33A9C] 参照 / 0x44F21C 文脈 | — |
| v318_07 | **0x5169A0 impl 列 consumer** / timer 書手特定 / 0xB33E90 参照 call 先 / CRT 初期化登録 | — |

---

## 3. 5 項の判定表

### 項目1 — `0x502500` のディスパッチ基址（主判定 **[確定]**）

| # | 結論 | 根拠（VA/命令） | 判定 |
|---|---|---|---|
| 1-1 | **テーブル実体**: 命令/オペコード行は id 格子 **T=0xB0AF50 から stride 0x28**、コマンド行 **162 行 = [0xB0AF50, 0xB0C8A0)**。row103 = **0xB0BF68** = {id=0x148, name=0xA509E4 'LoadGame <filename>', flags=0x00020000, struct=0xB0A678, **impl=0x502500@+0x10**, helper=0x4FDE30@+0x14, +0x18/+0x1C=0, +0x20=0xA509D4, +0x24=0xA509D0}。id は疎（row102=0x147, row161=0x182）。エントリ格子（svm_495/v159 表記）は **0xB0AF48 + k×0x28**（id=entry+8、impl=entry+0x18）、第二グループ [0xB0C8C0..0xB1026C)（id 帯 0x1000..0x1170）を含め **532 エントリ**（svm_495「532 entries」・(0xB10268-0xB0AF48)/0x28=532 と整合） | v318_01b §1 row walk（T=0xB0AF50/rows=162/anchor idx=103）＋ v318_01 §2 行内容、u32[0xB0C8A0]=0x183, u32[0xB10270]=0x1171 | [確定] |
| 1-2 | **ガード**: フェンス fn **0x4FCCE0..0x4FCD18** が `cmp dword [0xb10270],0x1171`（0x4FCCE0）と `cmp dword [0xb0c8a0],0x183`（0x4FCCFC）を検査し、不一致なら `push 0xA4A0E0 / 0xA4A098; call 0x404EC0`（ch0 即終了）。＝v314 §3-7 の起動フェンスと同一 | v318_01d §1 全文 | [確定] |
| 1-3 | **.text 参照（基址）はグローバル経由ではなく直接 imm32**。「0xB0BF78 を指す imm」は存在しないが（行アドレス参照=0 件 find_dword）、**テーブル基址/列基址の imm は実在**: (a) コンパイラ fn **0x4FF4C0**: `mov [esp+0x14],0xb0af4c`@0x4FF803（探索カーソル、+0x28 刻み・limit 0xB0B424・strcmp 0x98262D×2）、**`mov ax, word [ebx+ebx+0xb0af50]`@0x4FF88E**（行 id 読出、ebx=idx×20→×2＝idx×0x28）、`lea eax,[ebx+0xb0af48]`@0x4FF904（entry ptr）、`cmp byte [eax+0x10],0`@0x4FF918（flags 検査）、`mov ebx,[eax+0x1c]`@0x4FF963 → **`call ebx`@0x4FF9B9（helper 0x4FDE30 を間接呼び出し、callers(0x4FDE30)=0 と整合）** (b) フェンス 0x4FCCFC | v318_01e §1 連続 decode、v318_01c §2 | [確定] |
| 1-4 | **実行時の impl transfer = FUN_005169A0**: `lea eax,[ecx+ecx*4]`@0x516A3E → `add eax,eax`×2（ecx×40）→ `mov ecx,[eax+eax+0xb0af5c]`@0x516A45（argdesc 列）→ `add eax,eax` → **`mov ecx,[eax+0xb0af60]`@0x516A5D（★impl 列 = 0xB0AF60+idx×0x28）** → **`call ecx`@0x516AF3**（8 引数 cdecl、`add esp,0x20`）。LoadGame: 0xB0AF60+103×0x28 = **0xB0BF78**（v314 find_dword の cell と完全一致） | v318_07 §1（独立再確認、v159 既記と一致） | [確定] |
| 1-5 | 0x502500 の**直接参照 = find_dword で 0xB0BF78 の 1 件のみ・E8 = 0 件** → impl は必ず表経由の間接 transfer（v159 と一致・独立再確認）。帯内 0x50038F の `[eax*4+0xb0a110]` は別テーブル（tblA）で本行系ではない | v318_01 §1/§4、v318_01d §3 | [確定] |
| 1-6 | FUN_005169A0 の正式役割名と ecx（エントリ index）の供給元（帯外浅層留保） | — | [未確定] |

### 項目2 — `[0xB33E9C] = 195.06` の意味（主判定 **[確定]**）

| # | 結論 | 根拠 | 判定 |
|---|---|---|---|
| 2-1 | **参照 site 全数 = 164 件（.text 164 / 他セクション 0）**。命令境界厳格分類で **fld 74 / fsub 52 / fadd 21 / fmul 14 / fdiv 2 / fstp 1 = 164、境界不一致 0** → **読手 163 + 書手 1** | v318_02 §1-3、v318_02b §8 | [確定] |
| 2-2 | **書手 = `fstp dword [0xb33e9c]`@0x9DC2BC の 1 件のみ**（imm32 書手はこれ以外 0）。文脈はタイマ初期化 fn **0x9DC2A0**: `call dword [0xa280d0]`(GetTickCount) → **`fldz`** → `fst [0xb33e94]` / `fst [0xb33e98]` / `mov byte [0xb33e90],cl` / `fstp [0xb33e9c]` / `mov [0xb33ea0],ecx` / `mov [0xb33ea4],eax` → `ret`。＝オブジェクト **0xB33E90（タイマ）の +0x0C = dt を 0.0 で初期化**し、+0x14 に GetTickCount を格納。CRT 初期化関数配列 .rdata **0xA2884C → 0x9DC2A0**（0xA28840.. = {0x9DC240,0x9DC260,0x9DC280,0x9DC2A0,...}）に登録 | v318_02 §5、v318_06 §5、v318_07 §4 | [確定] |
| 2-3 | **ファイル値 195.06265258789062 は実データではない**: .data VirtualSize=0xA9C1C で raw は 0x30C00 のみ → VA 0xB33E9C は BSS。`pe.get_offset_from_rva` は **fileOff=0x732A9C を返し、これは .rsrc raw(0x731A00..0x733200) のバイト**（bytes `0a104343`、周辺は PNG ライク列）。＝**v314「初期 f32 = 195.06」は訂正**（ロード後は BSS ゼロ＋CRT 初期化 0.0） | v318_02 §0 | [確定]（訂正） |
| 2-4 | **実行時書手 = FUN_0047D170**: `mov [ecx+0x10],edx`@0x47D23E → **`fstp dword [ecx+0xc]`@0x47D241** → `fld [ecx+0xc]`@0x47D244 → `fmul dword [0xb06704]`@0x47D247 → **`fstp [ecx+0xc]`@0x47D24D** → `ret 4`@0x47D253。ecx=0xB33E90 のメソッド（`mov ecx,0xb33e90` の 4 site は 0x47D0F0/0x47D120 を呼出＝Timer メソッド群）。倍率 **[0xB06704]=1.0f**（v228 の既記 [0xB06704] と一致）。IAT 0xA280D0=GetTickCount 解決済 | v318_07 §2-3、v318_02b §7 | [確定] |
| 2-5 | **用途 = タイマオブジェクト 0xB33E90 の +0x0C「dt（フレーム/処理デルタ）」**。163 読手の文脈: 期限 tick 0x5721B5/0x57230D（0x572170 のクロス判定）、レート制御 0x4FBE28/0x4FBE34（v193b: [+0x38] -= dt）、AI 検知累算 0x5F28C3/0x640B35、スリープ 0x65F8A2/0x65FF43/0x65FFFD、粒子 0x40D8B2/0x40DB8D（s80）、回転 0x5106C2（b21 ×dt）、ダイアログ/クエストタイマ 0x59F749/0x59F93A、フェード 0x583C46 等（全 164 地址は §8 に列挙） | v318_02/02b、先行成果物（v228/s80/s56/s58/s64/v193b）と整合 | [確定] |
| 2-6 | 単位 = 秒 [推定]（GetTickCount 差分×0.001 系の v228 既記＋倍率 1.0 の本束確認。動的実測なし） | — | [推定] |
| 2-7 | オブジェクト基址 0xB33E90 の参照 = 9 件（実在は `mov ecx,0xb33e90`@0x406169/0x4061AA/0x40EACD/0x494A13 の 4 + initer `mov [0xb33e90],cl`@0x9DC2B6、他 4 件は境界偽） | v318_02b §6 | [確定] |

### 項目3 — `0x44DCF0` の「0 条件」発生経路（主判定 **[確定]**）

| # | 結論 | 根拠 | 判定 |
|---|---|---|---|
| 3-1 | **範囲 = 0x44DCF0..0x44E69E**（SEH prologue、共通エピローグ `lea esp,[ebp-0x38]`@0x44E674 → cookie check → **`ret 8`@0x44E69B**（3B、終端 0x44E69E）→ 整列バイト `8B FF`@0x44E69E-9F → **fn 局所ジャンプテーブル 0x44E6A0..0x44E6DB（15 エントリ）** → 型マップ 0x44E6DC..0x44E71F（68B）→ 次 fn 0x44E720（callers 1=0x445E72）。**範囲内の ret は 0x44E69B の 1 本のみ** → 全経路が単一エピローグに収束 | v318_03 全文（1430 命令）、v318_05b §0、v318_05c §C | [確定] |
| 3-2 | **型スイッチ構造**: `lea eax,[ebx-1]`@0x44DE1B → `cmp eax,0x42; ja 0x44e5a5`（型-1>66 → デフォルト）→ `movzx ecx, byte [eax+0x44e6dc]`@0x44DE27（型→ケースマップ）→ `jmp dword [ecx*4+0x44e6a0]`@0x44DE2E（**15 ケース**: 0x44DE35/0x44E326/0x44E578/0x44E512/0x44E367/0x44DEF0/0x44E001/0x44E2A5/0x44E401/0x44E237/0x44DE83/0x44E474/0x44E4C9/0x44E556/**0x44E5A5=デフォルト**） | v318_03 line99・v318_06 §3 テーブル dump | [確定] |
| 3-3 | **成功 exit = 19 箇所**（`mov eax,1`×18 が直後に `jmp 0x44e674`、＋`movzx eax,al; jmp 0x44e674`@0x44E50D の 1）。他に eax を返す出口なし | v318_03 eax 列挙＋ jmp 目的全 grep（0x44e674/0x44e672 の全 25 参照） | [確定] |
| 3-4 | **return-0 経路 = 8 本（すべて 0x44E672 `xor eax,eax` または 0x44E674 直前 al を経由し ret 8）**: | 同上 | [確定] |
| 3-4a | **F1 @0x44DDA4**: 0x44DD69 到達時に `[edi+0x244] & 0x4000` 立ち ∧ **esi==0**（base form 検出失敗: masterfile NULL ／ 0x46ABD0 却下 je ／ 0x55E000 ルックアップ失敗・NULL）。`and [edi+0x244],0xffffbfff` → `push 0xA385D8; call 0x4A7A60`（'Partial Form (%08X) in file "%s" could not find base form.'）→ jmp 0x44E672 | v318_03 line54 + s72 head | [確定] |
| 3-4b | **F2 @0x44DDFC**: `[esi+4] != ebx`（マスタから引いたレコードの型バイト ≠ 0x4511C0 が返した型）→ 型名 2 文字列を 0xB05E04 テーブルから組立て → `push 0xA38598; call 0x4A7A60`（'Form (%08X) in file "%s" has wrong type.\n\nShould be:\t%s\nIs:\t\t%s'）→ jmp 0x44E672 | v318_03 line70-82 | [確定] |
| 3-4c | **F3 @0x44E008**（case 0x44E001）: `cmp dword [0xb33a9c],0; je 0x44e672` → **シングルトン [0xB33A9C]==NULL でログなし return 0** | v318_03 line242-244 | [確定] |
| 3-4d | **F4 @0x44E2BC**（case 0x44E2A5）: `[0xB33A9C]==NULL` → `push 0xA384A0; call 0x4A7A60`（'PathGrid not associated with cell.'）→ return 0 | v318_03 line458-466 | [確定] |
| 3-4e | **F5 @0x44E61A**（case 0x44E5A5=デフォルト/ConstructObject 経路）: 新規 obj（0x447DF0/new）が esi==0 ∧ `[edi+0x23c] != 0` → `[edi+0x23c..0x23f]` の 4 バイトを %s に `push 0xA3847C; call 0x4A7A60`（'Unknown %s_ID in ConstructObject.'）→ [0xb06b18] 復元 → return 0 | v318_03 line640-716 | [確定] |
| 3-4f | **F6 @0x44E636**: esi==0 ∧ `[edi+0x23c]==0` → `test ebx,ebx; jne 0x44e62d`（ebx≠0 はログ省略）→ `push 0xA38450; call 0x4A7A60`（'NO_FORM trying to load in ConstructObject.'）→ return 0 | 同上 | [確定] |
| 3-4g | **F7 @0x44E667→0x44E672**（case 0x44E638・esi≠0 経路）: `call 0x447050`(edi,esi) の **al==0** → `mov edx,[esi]; mov eax,[edx+0x10]; push 1; call eax`（vtable[+0x10] クリーンアップ）→ `xor eax,eax` → return 0 | v318_03 line700-745 | [確定] |
| 3-4h | **F8 @0x44E50A**: 同様に `call 0x447050` の結果を `movzx eax,al` でそのまま返却（al==0 → return 0、al==1 → return 1） | v318_03 line600-640 | [確定] |
| 3-5 | F7/F8 の深層（0x447050 の al=0 条件）: 0x447050 = **vtable[+0x1C] 呼び出し型**（`mov edx,[eax+0x1c]; call edx`@0x44705B/0x447069、callers 18 件）→ al はその戻り値。クラス別の成否定義は帯外浅層留保 | v318_06 §1-2 | [未確定] |
| 3-6 | 帯内 caller 文脈再確認: 0x44F21C `push [esp+0x134]; push esi; mov ecx,edi; call 0x44dcf0; test eax,eax; jne 0x44f22e; mov [esp+0x18],ebp`(=0) → v314 の契約（0 → [esp+0x18]=0）と完全整合 | v318_06 §6 | [確定] |

### 項目4 — 間接 call 595 件の先追跡（分類: ログ系のみ深追跡）（主判定 **[確定]**）

| # | 結論 | 根拠 | 判定 |
|---|---|---|---|
| 4-1 | **595 件を完全再現**（visited=723・indirect=595、v314_05 と一致）。**全件が `call reg` 形**（`call [reg+disp]` 形はスイープ内 0 件） | v318_04 §sweep | [確定] |
| 4-2 | **分類（由来追跡: 直前 14 命令・call/ret 境界で停止）**: vtable/メソッドスロット **348**（disp ヒストグラム上位: 0xc×39, 4×38, 0xd4×27, 8×25, 0x170×23, 0x10×18, 0x24×14, 0x4c×13…）／ vtable slot0（二重 deref 後 call）**179** ／ [global] 読替 **19**（global = IAT スロット: InterlockedDecrement×10, InterlockedIncrement×2, InterlockedCompareExchange×2, Sleep, TlsGetValue, lstrcatA, lstrcpyA + 0xB42EB4/0xB42E8C 各1）／ **imm32 解決 0** ／ 解決不可 **49**。合計 595 | v318_04b §classification | [確定] |
| 4-3 | **log-関連文脈 = 104 件 / 23 fn**（fn が LOG_SET={0x4941A0,0x404EC0,0x404EE0,0x494480,0x494730,0x4A7A60,0x494390,0x494658} を直接呼び出し）。代表 deep context: 0x5723E0(d0) の `call edx`=vtable[+0x88]、slot0 の `push 1; call eax`=Release 型、0x4390B0 の Release 等 — **全てゲームオブジェクトの vtable メソッド**で、logger への fn-ptr 呼び出しではない | v318_04/04b | [確定] |
| 4-4 | **ログ/警告系の間接呼出先 = 0 件**: imm 解決先に LOG_SET 0 件、[global] 読替 8 稳に logger スロット（[0xB34D90]/0xA280D4 系）なし、log-context 104 件も全てオブジェクト vtable。＝v314 の「間接 call 留保」を解消し、**Warnings 書出経路の間接的補強も否定方向で一致** | 同上 | [確定] |
| 4-5 | 残る不確実: 解決不可 49 件の先、vtable slot 先の実体（実 .ess/動的観測なし）、深さ 20 超の到達 | — | [未確定]（動的要項目） |

### 項目5 — fn-end walker の短 CC 境界改良（主判定: 失敗型再現＋改良案 **[確定]**／一般適用 **[部分確定]**）

| # | 結論 | 根拠 | 判定 |
|---|---|---|---|
| 5-1 | **失敗型の再現**: 既知 head 14 件に v314_05 の旧 head 規則（≥4B CC＋16 アライン＋非 CC、back-to-back ret/ret-imm）を適用 → **不合格 3 件 = 0x45CC60 / 0x571660 / 0x571820**（パターン: `ret imm` の終端が CC 1-2B を挟んで head に一致、pre4=`c20400cc`/`0400cccc`/`c20400cc`）。0x45B300/0x452330/0x466D50/0x5AE190 は v314_05 の back-to-back ルールで通過（＝v313 の旧 minrun≥1 規則時代の失敗は既に部分修正済み）、0x44F3D0/0x51B490 は CC15/12 で通過 | v318_05 §A | [確定] |
| 5-2 | **改良案 rule v3（命令根拠付き）**: ①**fn-head**: 16 アライン・非 CC かつ（CC ラン≥4 **OR**（CC ラン k≤3 で `C3` or `C2 imm16` が exactly A−k で終端））→ 上記 3 不合格を含む 14/14 通過。②**fn-end**: 線形 decode 中、ret で終端 E が生じたとき、**decode 済み jcc/jmp の直接ターゲットに E ≤ T < E+0x800 が存在すれば「早期 ret＋継続コード」として拒否し継続**、存在しなければ (E2') E から≤15B の CC を飛ばして 16 アライン非 CC に到達、または (E3) E から次 16 アライン非 CC（＝CC/データ領域の先）に到達 → **end=E**。E1（≥4B CC ラン）も同判別でゲート。根拠: 継続コードは必ず E より前の直接分岐から到達する（ret を越える fall-through は存在しない）一方、fn 局所ジャンプテーブル/データは直接分岐ターゲットを持たない | v318_05c §B（擬似コードは本節末） | [確定] |
| 5-3 | **ラベルペア 16/16 一致（NG 0）**: 0x44F0C0→0x44F3A8（旧 0x44F3C1 NG）、**0x44DCF0→0x44E69E**（旧 0x44F0B8 NG）、0x44F3D0→0x44FA44、0x404EC0→0x404ED5、**0x502500→0x50262F**（旧 0x502647＝融合 NG）、**0x51B490→0x51BE2B**（旧 0x51BE45 NG）、**0x571640→0x57165F**（旧 0x571716 NG）、0x571660→0x571716、0x571720→0x571782、**0x571800→0x57181E**（旧 0x5718FB NG）、0x571820→0x5718FB、**0x571DF0→0x571E7E**（旧 0x571F66 NG）、0x571E80→0x571F66、0x572170→0x5723DA、0x51BE50→0x51BE65、0x502630→0x502647 | v318_05c §B | [確定] |
| 5-4 | **分割 fn の外部証拠**: callers(0x51BE50)=2（0x596DAA/0x596DEC）、callers(0x44E720)=1、callers(0x450710)=1（0x4520B6）、callers(0x451A60)=4、callers(0x4040E0)=637 → rule v3 の分割判定を外部参照で補強 | v318_05c §C、v318_05e | [確定] |
| 5-5 | **v314 既知 end の訂正（旧値不削除・併記）**: 0x51B490 の end は 0x51BE65 ではなく **0x51BE2B**（0x51BE2D..0x51BE44 はデータ、**0x51BE50..0x51BE65 は別 fn**（`cmp eax,7; ja; jmp 0x565cc0` 型ディスパッチ）、v310「xor eax,eax;ret@0x51BE62」は同別 fn の epilogue）／0x502500 size 0x147 は **0x130（end 0x50262F）**＋0x502630..0x502647 は独立 thunk（`call 0x4f4800; ret`、callers 0）／0x450550 end は 0x450978 ではなく **0x45070E**（0x450710 が新 fn）／0x4507D0 end = 0x450978（旧値維持・v3 確認） | v318_05 §A/B、v318_05b §0、v318_05c §B | [確定]（訂正） |
| 5-6 | **帯内 D スイープ**: 旧 head 303 件で old≠v3 = **56 件**（v3<old 48＝旧 walker の融合/テーブル通過是正、v3>old 8＝旧 walker の早期 ret 停止是正）。**個別検証 11 件はすべて v3 側に境界根拠**（例: 0x4040A0 old=0x404143→v3=0x4040DF＝ret8+CC+新 fn callers637、0x451A40 old=0x451AB0→v3=0x451A5D＝ret8+CC3+prologue callers4、0x405440/0x451460 は旧 early-ret 停止）。残 45 件は同型と推定（全数目視未了） | v318_05c §D、v318_05d/05e | [部分確定] |

rule v3 擬似コード:

```
walk(head): decode linearly; collect direct jcc/jmp targets seen so far (B)
  on >=4B CC run at decode pos (E): if no t in B with E<=t<E+0x800: end=E else continue
  on ret ending at E:              if any t in B with E<=t<E+0x800: continue   # early-ret: code follows
    else: q=E; skip <=15 CC; if q16-aligned & nonCC(q): end=E                   # E2'
          else: H=first 16-aligned nonCC > E within E+0x400; end=E              # E3 (data/jump-table follows)
```

---

## 4. 根拠（主要命令・数値）

- **表**: T=0xB0AF50 / stride 0x28 / rows=162 / row103=0xB0BF68（id 0x148, impl 0x502500, helper 0x4FDE30）／エントリ格子 0xB0AF48+ k×0x28（532 entries → 0xB10268）／ガード [0xB0C8A0]=0x183, [0xB10270]=0x1171。
- **基址参照**: `0x4FF803 c7442414 4cafb000`（mov [esp+0x14],0xb0af4c）／`0x4FF88E 668b841b50afb000`（mov ax,[ebx+ebx+0xb0af50]）／`0x4FF904 8d8348afb000`（lea eax,[ebx+0xb0af48]）／`0x4FF9B9 ffd3`（call ebx=helper）／`0x4FCCFC 813da0c8b00083010000`（cmp [0xb0c8a0],0x183）／`0x516A45 8b8c005cafb000`＋`0x516A5D 8b8860afb000`＋`0x516AF3 ffd1`（call ecx, add esp,0x20）。
- **0xB33E9C**: 164 refs（fld74/fsub52/fadd21/fmul14/fdiv2/fstp1）／書手 `0x9DC2BC d91d9c3eb300`（fldz 由来）／初期化 fn 0x9DC2A0（`ff15d080a200`=GetTickCount → `d9ee` fldz → 4 フィールド初期化 → ret）／.rdata 0xA2884C → 0x9DC2A0／実行時書手 `0x47D241 d9590c` `0x47D24D d9590c`（fmul [0xb06704]=1.0f）／fileOff 0x732A9C ∈ .rsrc raw。
- **0x44DCF0**: 単一 `ret 8`@0x44E69B（`c20800`）／exit 全列挙（0x44e672 参照 7＋0x44e674 参照 18）／switch `ff248da0e64400`@0x44DE2E／マップ `0fb688dce64400`@0x44DE27（0x44E6DC, 68B）／F1-F8 の VA は §3-4a..h。
- **595 分類**: slot 348 / slot0 179 / IAT-経由 19 / 未解決 49 / imm 0、log-context 104（23 fn）、logger への間接 0。
- **walker**: 見出し検証表（§3-5-1）、ラベル 16/16（§3-5-3）、sweep 303 head・56 差分・11 検証（§3-5-6）。

---

## 5. v314 既知との整合・訂正（旧結論は削除せず併記）

| v314 記載 | 本束の裁定 | 理由 |
|---|---|---|
| 残点1「0x502500 ディスパッチ基址 [未確定]、グローバル経由と推定」 | **解消 [確定]／推定は訂正**: 基址は .text imm32 に実在（0x4FF88E の 0xB0AF50、0x516A5D の 0xB0AF60）。行アドレス 0xB0BF78 の imm が 0 件なのは事実だが、これは「行を直接指す必要がない」構造（列基址＋idx×0x28）による | v318_01/01b/01e/07 |
| 残点2「[0xB33E9C] 初期 f32=195.06、書手 CRT fstp のみ、意味 [未確定]」 | **解消 [確定]／初期値は訂正**: 195.06 は .rsrc file bytes（BSS 超域の読出ゴミ）。実初期値は fldz→0.0。用途 = タイマ+0x0C dt、実行時書手 = FUN_0047D170 | v318_02/02b/07 |
| 残点3「0x44DCF0 の 0 条件 [部分確定]」 | **解消 [確定]**（F1..F8 の 8 経路を特定）。深層のうち 0x447050 の al 成否だけを新残件へ分離 | v318_03/06 |
| 残点4「間接 call 595 件 [未確定]（留保）」 | **分類 [確定]**（4-2/4-3）。ログ系深追跡は方針どおり実施し、logger 間接 0 件を確認。49 未解決＋動的性を新残件へ | v318_04/04b |
| 残点5「fn-end walker 短 CC 境界欠損 [未確定]」 | **失敗型再現・改良案 [確定]**（rule v3、ラベル 16/16）。一般 sweep 完全性のみ [部分確定]。旧 walker の end 誤り（0x51BE45/0x502647/0x450978/0x44F0B8 等）を訂正 | v318_05 系 |
| v314 §1.2「0x502500 size 0x147」 | **訂正**: end=0x50262F（size 0x130）。0x502630..0x502647 は独立 thunk（ret+CC+16 アライン、callers 0） | v318_05b §0 |
| v314 §1.5「0x51B490..0x51BE64」 | **訂正**: 本 fn end=0x51BE2B。0x51BE50..0x51BE65 は別 fn（callers 2） | v318_05b §0 |
| svm_495「0x502500 160B」→ v314「0x147」 | さらに **0x130** へ（融合範囲の縮小）。impl アドレス・callee 列は従来どおり一致 | 同上 |
| **`v321_v318_next_bundle_H.md`（別成果物・非編集）との不一致**: (a) 195.06 を「raw f32 [部分確定]・単位不明」とする（ファイルゴミと断定できていない）、(b) 0x44DCF0 の 0 条件を `xor eax,eax`@0x44DEA6/0x44DF1A/0x44E2ED/0x44E4EF と列挙（これらは **SEH/取得失敗時の内部変数 esi 用で、ret に到達しない**。実際の return-0 は 0x44E672/0x44E674 経由の F1..F8）、(c) walker を「差分多数のまま [部分確定]」とする（ラベル 16/16 は未反映） | **裁定: 本束 v318_v314_residuals.md を正とする**（v321 は同一 #83 への別セッション出力とみられ、本束の probe 証拠＝fileOff・fldz・ret 到達性・ラベル検証が優先。v321 は削除・変更しない） | v318_02/03/05c |

先行成果物との整合（不一致なし）: GT_CHEATSHEET §3（フェンス 0x4FCCE0）、v13（0xB0AF48 基底・コンパイラ fn）、v159（impl 列 0xB0AF60 = 1 site・FUN_005169A0@0x516A5D→0x516AF3）、v17（0x4FF4C0 はコンパイラで実行時 dispatcher ではない）、v228（書手 FUN_0047D170・GetTickCount・[0xB06704]）、svm_495（532 entries stride 0x28）。

---

## 6. 新規残件（次束用）

1. **[未確定] 0x447050 の al=0 条件（F7/F8 の深層）**: vtable[+0x1C] 戻り値のクラス別定義（帯外 0x447050・浅層 head のみ）。移植に必要な「失敗の定義」の最終部分。
2. **[未確定] 間接 call 595 中 49 件の解決不可分＋vtable slot 先の実体**: 静的限定（imm 解決 0）＋実 .ess なし。動的要項目。
3. **[部分確定] rule v3 の帯内 sweep 全数検証**: 56 差分中 11 件検証済・残 45 件（同型推定）。特に v3>old 8 件の全数裏取り。
4. **[未確定] FUN_005169A0 の正式役割名と ecx（エントリ index）の供給元**（帯外 0x5169A0・浅層留保）。
5. **[未確定] 0x44DCF0 型スイッチの型名対応**: 型 1..0x43 → マップ 0x44E6DC → 15 ケースの各レコード型名（帯外の型名テーブル照合が要る）。
6. **[未確定] 単位=秒の動的確認**（2-6 の [推定] 解消用）。

---

## 7. 再現手順

```powershell
chcp 65001
python $env:TEMP\opencode\v318_01.py   *> $env:TEMP\opencode\v318_01.out    # 表 anchor・window 走査
python $env:TEMP\opencode\v318_01b.py  *> $env:TEMP\opencode\v318_01b.out   # row walk・index table・帯外文脈
python $env:TEMP\opencode\v318_01c.py  *> $env:TEMP\opencode\v318_01c.out   # consumer A/B・slot 探索
python $env:TEMP\opencode\v318_01d.py  *> $env:TEMP\opencode\v318_01d.out   # 0x4FF4C0 クリーン decode
python $env:TEMP\opencode\v318_01e.py  *> $env:TEMP\opencode\v318_01e.out   # helper call・0x5000F4 テーブル
python $env:TEMP\opencode\v318_02.py   *> $env:TEMP\opencode\v318_02.out    # 164 census・CRT 初期化
python $env:TEMP\opencode\v318_02b.py  *> $env:TEMP\opencode\v318_02b.out   # 境界厳格分類・0xB33E90・0x47D170
python $env:TEMP\opencode\v318_03.py   *> $env:TEMP\opencode\v318_03.out    # 0x44DCF0 全文
python $env:TEMP\opencode\v318_04.py   *> $env:TEMP\opencode\v318_04.out    # 595 再現・初回分類
python $env:TEMP\opencode\v318_04b.py  *> $env:TEMP\opencode\v318_04b.out   # 由来追跡分類・log-context
python $env:TEMP\opencode\v318_05.py   *> $env:TEMP\opencode\v318_05.out    # head/end old vs new・D sweep
python $env:TEMP\opencode\v318_05b.py  *> $env:TEMP\opencode\v318_05b.out   # 境界 dump・refined walker
python $env:TEMP\opencode\v318_05c.py  *> $env:TEMP\opencode\v318_05c.out   # rule v3 ラベル検証
python $env:TEMP\opencode\v318_05d.py  *> $env:TEMP\opencode\v318_05d.out   # 差分 spot-check 1
python $env:TEMP\opencode\v318_05e.py  *> $env:TEMP\opencode\v318_05e.out   # 差分 spot-check 2
python $env:TEMP\opencode\v318_06.py   *> $env:TEMP\opencode\v318_06.out    # 0x447050 浅層・0x44F21C 文脈
python $env:TEMP\opencode\v318_07.py   *> $env:TEMP\opencode\v318_07.out    # 0x5169A0 impl 列・timer 書手
```

---

## 8. selftest 結果

- 項目1: `T=0xB0AF50 rows=162 anchor idx=103`／`row103: 00000148 00A509E4 00020000 00B0A678 00502500 004FDE30 ...`／`find_dword(0xB0BF68)=[]`、`find_dword(0xB0AF50)=['004FF892']`／`u32[0xB0C8A0]=0x183, u32[0xB10270]=0x1171`／`0x516A5D mov ecx,[eax+0xb0af60]` → `0x516AF3 call ecx`／`callers(0x4FDE30)=0`
- 項目2: `refs(0xB33E9C)=164 (.text=164, other=0)`／分類 `fld74 fsub52 fadd21 fmul14 fdiv2 fstp1`（境界不一致 0）／`fileOff(0xB33E9C)=0x732A9C ∈ .rsrc raw 0x731A00..0x733200, bytes=0a104343`／`0x9DC2A0 call [0xa280d0]; fldz ... fstp [0xb33e9c]`／`.rdata 0xA2884C → 0x9DC2A0`／`0x47D241/0x47D24D fstp [ecx+0xc]`、`[0xB06704]=1.0f`
- 項目3: `range 0x44DCF0..0x44E69E, insns=1430, ret は範囲内 1 本 (ret8@0x44E69B)`／`jmp [ecx*4+0x44e6a0]` 15 エントリ、マップ 68B@0x44E6DC／0x44e672 参照 7・0x44e674 参照 18／文字列 0xA385D8='Partial Form …' 0xA38598='Form … wrong type' 0xA384A0='PathGrid not associated with cell.' 0xA3847C='Unknown %s_ID in ConstructObject.' 0xA38450='NO_FORM trying to load in ConstructObject.'
- 項目4: `visited=723, indirect=595`（v314_05 一致）／`slot348 / slot0 179 / global19 / unresolved49 / imm0`／`log-context=104 sites / 23 fn`／`LOG_SET 直接先 0・logger global 0`
- 項目5: `head 検証 14 件（old 不合格 3: 0x45CC60/0x571660/0x571820、new 全通過）`／`ラベル 16/16 OK, NG=0`／`sweep 303 head, old!=v3 うち 11 件検証すべて v3 正`／`callers(0x51BE50)=2, (0x44E720)=1, (0x450710)=1, (0x451A60)=4, (0x4040E0)=637`

**判定件数**: [確定] 22 / [部分確定] 1 / [推定] 1 / [未確定] 3 / [確定負] 0（合計 27 行）。**5 項目の主判定はすべて [確定]**（項目5は一般適用の完全性のみ [部分確定] 付き）。
