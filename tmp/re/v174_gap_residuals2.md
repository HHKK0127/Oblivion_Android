# V174 — gap/E8 残点と read_ptr 防御修正束の裁定（tmp/re/v174_gap_residuals2.md）

タスク: TODO_re49.md serial **161（V161・中）** / 分析専任サブエージェント / 作成日 2026-10-28
スコープ: `tmp/re/v162_gap_residuals.md` 残点 R-a..R-j / **R-l** / **R-m**（R-k は v169 適用済・除外）＋
`tmp/re/v165_v140_recompute.md` 新規残点 **Rs1..Rs6**。

> 本ファイルは新規作成。中断対策のため「証拠収集→即書込み→追記」で随時更新する。

---

## §1 手法 / 入力 / 環境

- 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**
  （bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
- EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B）。
  VA→file off は probe_harness `CTX.rd` / セクション定数（.text 0x400C00 / .rdata 0x400E00 / .data 0x401400）経由。
- 入力（読取専用）: `%TEMP%\opencode\func_list_new.txt`（21,597 行）、`%TEMP%\opencode\v155_union.csv`（31,905 行）、
  **`%TEMP%\opencode\v170_ledger_append_candidates.csv`（10,870 行）**、
  `%TEMP%\opencode\v133_dup_results.csv`、`tmp/re\v96_e8_candidates.txt`、`tmp/re\v92_e8_214_triage.txt`、`tmp/re\v95_recall_gap.txt`。
  - **名称差異**: タスク記載「V157 の CSV 10,870 行」の実体は **serial 157（=V170）の成果物 `v170_ledger_append_candidates.csv`**（10,870 行 + ヘッダ、`v170_ledger_granularity.md` L15 と一致）。本ファイルでは v170 CSV として扱う。
  - `tmp/re\v96_e8_candidates.txt` / `v95_recall_gap.txt` / `v92_e8_214_triage.txt` は `%TEMP%\opencode\` ではなく **`tmp/re\` 配下**に所在（タスク記載と場所が異なる）。
- 新規スクリプト: `%TEMP%\opencode\v174_p1.py`（R-l + R-m）、出力 `v174_p1_out.txt`。
  成果物 CSV: `%TEMP%\opencode\v174_xcu_entries.csv`（7,402 行）/ `v174_xcu_candidates.csv`（1,199 行）。
- 共有台帳・docs/・TODO・既存 `tmp/re/*.md` は無編集。

---

## §2 Ghidra MCP 使用可否（R-f に影響）

- `execute` 内 `ghidra-mcp.list_instances()` 確認 → **Ghidra MCP 自体は稼働**（v7.0.0 / pid 29132 / http://127.0.0.1:8089 / endpoint 190）。
- ただし接続先インスタンスは **`d1tamers_analysis` プロジェクト（`D:\Cargo\WS\D1Tamers-RE\analysis\d1tamers_analysis`、open program = `/d1tamers_linear.img`）** の 1 件のみ。
- `connect_instance(project="OblivionRe")` → **失敗**（`No instance matching 'OblivionRe' (UDS: 1 found, none matched)`）。
  `list_open_programs()` も `d1tamers_linear.img` のみ（1 プログラム）。
- **裁定: OblivionRe プロジェクトは当該 Ghidra インスタンスに不在 → R-f の `Function.getBody()` 突合は本タスクでは使用不可**。
  新規 headless インスタンス起動手段は MCP ツール群に存在しない。→ R-f は §4 で代替証拠を示す。

---

## §3 R-l（中）: `.CRT$XCU` 登録配列 — **用途確定・台帳化候補抽出まで完了**

### 3-1 配列の再確認（`v174_p1.py`）

- 配列範囲 `.rdata 0xA283D4 .. 0xA2F77C`（hi exclusive / CRT 初期化ループ `mov edi,0xa2f77c` と一致）、**29,608 B / 7,402 エントリ**を再計測で一致。
- 値の一意数 **7,380**（うち **NULL = 23**、非 NULL 実関数 **7,357**）。値域 `0x9D7E40..0xA163C0` 帯 + NULL。
- 重複: 22（top: NULL x23 のみ実質。非 NULL はすべて slot_count=1）。

### 3-2 台帳・候補 CSV との重複チェック（タスク要件）

| 突合先 | 一意値 7,380 のうち該当 | 備考 |
|---|---:|---|
| `func_list_new.txt`（Ghidra 台帳 21,597） | **713** | decompiled ヘッダあり（union でも `dec` 列 713 と一致） |
| `v155_union.csv`（統合表 31,905） | **6,181** | 内訳: `V86+Cat3` 5,467 / `dec` 713 / `V86+Cat3+I1` 1 |
| **v170_append CSV（10,870）** | **1** | **タスクの「V157 CSV」との重複はほぼゼロ** |
| （補助）`v96_e8_candidates.txt` 内 VA 表記 | 2 | E8 経由では拾われないという v162 の主張を数値的支持 |
| 上記いずれにも不在 = **台帳化候補（完全未登録）** | **1,199** | `%TEMP%\opencode\v174_xcu_candidates.csv` に全件書き出し（列: fn_va, slot_count） |

- すなわち XCU 配列の関数 7,357 の大半（6,181）は **v155 統合表では Cat-3 / v86 帯として捕捉済み**だが、
  **台帳（func_list）登録は 713 のみ**。残り 1,199 は統合表にも v170 候補にも無い完全未登録層。
- 台帳化候補は **`v174_xcu_candidates.csv`（1,199 行）** を提出（本タスクは分析専任のため共有台帳への追記は行わない。V159/V170 の承認フローに合流する候補）。

### 3-3 用途確定（サンプル逆アセンブル 18 件等間隔）

識別子様文字列（`^[a-zA-Z][a-zA-Z0-9_]{3,}$`）を含むサンプル = **10/18**。分類は 4 型:

| 型 | 典型コード列 | 例（文字列 / 先頭アドレス） |
|---|---|---|
| **A: GMST/GameSetting 登録** | `push <name_str>; push <desc?/edid>; mov ecx, <obj>; call 0x419A70/0x41BAE0; push <ptr>; call 0x981FB4(atexit)` | `sHolidayNewLifeFestival`(0x9DF520) / `fMasserAngleFadeStart`(0x9E5C20) / `fPerkSneakAttackMeleeJourneymanMult`(0x9EA940) / `fMagicDiseaseTransferBase`(0x9EF520) / `fPersuasionReactionHate`(0x9F3040) / `fPathPointTurnDistanceInterior`(0x9FA8C0) / `sArmorWeightHeavy`(0x9E27B0, v162 済) |
| **B: Gamebryo クラス名登録/RTTI** | `push <class_str>; push <bss>; mov ecx, <obj>; call 0x70E220; ret` | `NiControllerSequence`(0xA02560) / `NiTransformInterpolator`(0xA041A0) / `NiPSysBoundUpdateModifier`(0xA0F380) / `NiD3DShaderConstantMap`(0xA163C0) |
| **C: シングルトン生成 + グローバル代入** | `call <factory>; mov [global], eax; ret` | `call 0x6BEFD0→[0xB3C464]`(0xA009E0) / `call 0x6C2650`(0xA05DE0, 0xA0BAF0) / `call 0x6C1BC0`(0xA07A30) / `call 0x6BF330`(0xA096A0) / `call 0x6C2D20`(0xA0D730) |
| **D: 既存オブジェクトの破棄登録（atexit のみ）** | `push <ptr>; call 0x981FB4; pop ecx; ret` | 0xA115C0（`push 0xa27730; call 0x981fb4`） |

- **裁定: 配列は MSVC の `.CRT$XCU`（C++ 動的初期化関数配列）で確定**。中身は GMST/設定登録（型 A）、Gamebryo クラス登録（型 B）、
  シングルトン生成（型 C）、atexit 破棄登録（型 D）が混在する **ゲーム固有の初期化フック群**。v162 §3-3(d) の「設定/GMST 登録の動的初期化関数」
  仮説をサンプルで拡張・確定した（GMST のみならず Ni クラス登録・singleton 生成を含む）。
- 消費者（ループ本体）: `0x981C6B` 起点（`v174_p1.py` 実測）。先頭は `cmp dword [0xaa3e84],0 / je / call 0x98d8a2 / call dword [0xaa3e84]`（onexit テーブル系）→
  `call 0x98d805`（_mtinit 相当）→ **`mov esi,0xa283d0 / mov edi,0xa2f77c / mov eax,[esi] / test eax,eax / je / call eax / add esi,4 / cmp / jb`** = `_initterm` 本体。
  E8 caller は **1 件のみ `0x9877E1`**（= `_DllMainCRTStartup` 相当からの CRT 初期化）。`0x981C6B` は `v155_union.csv` に登録済（func_list には無い）。

### 3-4 判定

**R-l 解明**。配列の用途（`.CRT$XCU` = ゲーム固有動的初期化: GMST/クラス/singleton/atexit）、消費ループ、caller、
台帳化候補 1,199 件の抽出（CSV 提出）まで完了。残作業は「候補 CSV の採否」のみ（共有台帳編集は禁止事項のため親セッション/承認制）。

---

## §4 R-m（低→**解明**）: 未知 3 件 0x700A9A / 0x700E6E / 0x700F06 — **関数ではなく IAT 初期値（hint/name RVA）**

- 3 件は `v162` §3-3(d) で「IAT スロット 0xA28090 / 0xA28168 / 0xA282B0 の値」として FF15 17 site に帰属済みだが、
  **実体は未確定のまま**だった。本タスクで PE import ディレクトリを直接照合（`v174_p1.py` / pefile）。

| IAT スロット | import 名 | DLL | entry# | ファイル内 FirstThunk 値 | v162 の解釈 |
|---|---|---|---|---|---|
| 0xA28090 | **CreateFileA** | KERNEL32.dll | #19 | 0x700A9A | 「.text の補助/thunk 帯」（誤） |
| 0xA28168 | **FileTimeToLocalFileTime** | KERNEL32.dll | #73 | 0x700E6E | 同上 |
| 0xA282B0 | **GetActiveWindow** | USER32.dll | #24 | 0x700F06 | 同上 |

- delay import ディレクトリ: **なし**（`no delay import directory`）。
- 逆アセンブルの旁証: 3 件はいずれも**命令の途中（mid-instruction）から始まる**（0x700A9A: `lea ebx,[ebx]` = 8D DB の第 2 オペコードから、
  0x700E6E: `into` = CE、0x700F06: `or edi,eax`、直前バイト 06/8B/C3）→ **コードとしての関数エントリではない**。
- **構造的解釈**: PE の FirstThunk/INT エントリ（PE32）は「hint/name 構造体への **RVA**」を格納する。ファイル展開時の値 0x700A9A 等は
  その RVA 表現であり、実行時にはローダが IAT を実アドレス（CreateFileA 等）に書き換える。→ 「未知 3 関数」ではなく **3 つのインポート API の IAT 初期表現**。
- **裁定: R-m 解明**。3 件は `KERNEL32!CreateFileA` / `KERNEL32!FileTimeToLocalFileTime` / `USER32!GetActiveWindow` の IAT 初期値であり、
  **関数実体は存在しない（台帳未登録のままが正当）**。v162 §7 の「先に名前付きスタブ化」示唆は**取り消し相当**——移植側では
  通常のインポート解決（IAT フック/スタブ）で扱えばよく、個別関数化は不要。v162 §3-3(d) の「.text の補助/thunk 帯」記述は
  hint/name RVA の誤認だったことを訂正する（read_ptr 修正後も「呼び出し先」として扱った点が残誤）。
- 補足: v162 の FF15 17 site 帰属（8/5/4 site）自体は IAT スロット特定として正しいため、**帰属計算の訂正は不要**（site 数と API の対応は成立）。
- **ついでに判明（v162 §3-3(c) の隣接 2 値）**: 同一手法で IAT スロット 0xA28088 の値 0x700A6E = **hint=516 `GlobalMemoryStatus`**（KERNEL32）、0xA2808C の値 0x700A84 = **hint=326 `GetCurrentThreadId`**（KERNEL32）も判明。**v162 §3-3(c)「[0xA28088]=0x700A6E、[0xA2808C]=0x700A84（.text の補助/thunk 帯）」の「.text の補助/thunk 帯」という同定も誤り**で、実体は hint/name 構造体（RVA 表現）だった。→ 「0x700A9A 帯」という帯域表現は**まとめて hint/name 文字列プール**（.rdata 実体・RVA 表記）であることが確定。

---

## §5 R-f（中）: 終端命令なし 1,376 件 × Ghidra `Function.getBody()` — Ghidra MCP 不可・代替証拠で暫定決着

### 5-1 Ghidra MCP 使用可否

§2 のとおり **OblivionRe プロジェクト不在で使用不可**（稼働インスタンスは d1tamers_analysis のみ）。`Function.getBody()` 突合は本タスクでは**未実施**。

### 5-2 代替証拠: v133 の定義を完全再現して台帳突合（`v174_p6.py`）

- v133 の実測元は **`v133_4_probe.py`** と特定し、その定義（span=台帳 VA→次台帳 VA/`raw_end_va`、cap 0x10000、末尾連続 CC 除去、**最初の内部 CC ラン**（j>0 かつ後続非 CC あり）を 1 スパン 1 カウント、`pre=body[:cc]` に `ret/retf/iret/hlt/jmp` が出現すれば after-term、無ければ no-term、skipdata=True）を忠実に移植。
- **v133 の数値を完全再現**: internal-cc **4,923** / after-term 3,547 / **no-term 1,376**（v133_md 記載と一致）。v133 の例も一致（`0x00402D00` cc@81 bodylen=111 / `0x004048B0` cc@76 bodylen=336）。
- **no-term 1,376 span の CC 位置の突合**（＝`Function.getBody()` の近似。func_list_new は Ghidra Function 開始 VA 一覧であり、cc 位置が登録されない＝Ghidra も独立関数として認識していないことの直接証拠）:

| 突合先 | 該当 |
|---|---:|
| `func_list_new.txt` 開始 VA | **0 / 1,376** |
| `v155_union.csv` | **0 / 1,376** |
| `v170_append` CSV | **0 / 1,376** |
| いずれにも不在（未請求） | **1,376 / 1,376** |

- 構造: cc offset span 内 min=1 / med=77 / max=5,248、**16B アライン 215/1,376**、先頭 CC ラン長 **1 B が 1,366（残 2 B が 10）**。
- **裁定**: Ghidra が独立関数として認識する位置は **0/1,376**。v133 の「未請求の隣接コード仮説」（= 独立エントリの実在）は**支持されず**、v133 自身の「ブロック整列パディングの可能性」仮説が支持される（16B アライン 215 件はその一例、残り 1,161 は非アラインだが CC ラン 1 B 支配・台帳本文内 med=77 B 地点）。R-f は「**代替証拠により実在未証→パディング寄りで暫定決着**」。完全決着は OblivionRe Ghidra プロジェクト接続後の `getBody()` 突合（新規残点 N-c）。
- リスト: `%TEMP%\opencode\v174_r_f_noterm_spans.csv`（1,376 行 / 列: span_va, cc_va, cc_off, bodylen, 台帳・統合表・候補 CSV 各登録フラグ, ledger_name）。
- 註: v174_p5.py の初回実行は `MD.detail` 未有効化で全 span 例外、2 回目は int3 ラン全体を数えて 82,817 に暴走したが、`v133_4_probe` の「span 単位・最初の内部 CC ラン」定義に修正して上記の一致を得た（定義差の記録）。

---

## §6 R-a / R-h / 残点 A..J の能動裁定

### 6-1 [R-a] C 群 wrapper の上流 — **解明（C++ 例外 UnwindMap アクション + `___CxxFrameHandler3`）**

`v174_p2.py`/`v174_p3.py`/`v174_p4.py` の実測:

- **18 個の wrapper のうち 17 個は `.rdata` のポインタテーブルに格納**（0x410E20 のみ例外 → これは `push 0x410e20` @`0x410E4D` のコード内参照 = MessageFilter 登録系）。
- テーブルスロットの構造が **MSVC C++ 例外フレームディスクリプタ（FuncInfo）** と完全一致:

  ```
  slot-4 : toState = -1
  slot   : <C-group wrapper>        <- UnwindMap.action
  slot+4 : 0x19930522               <- CXX_FRAME_MAGIC_VC8 (magicNumber)
  slot+8 : maxState = 1
  slot+12: pUnwindMap = (slot-4)    <- UnwindMap {toState=-1, action=wrapper}
  +20..+36: nTryBlocks=0 / pTryBlockMap=0 / nIPMapEntries=0 / pIPtoStateMap=0 / pESTypeList=0 / EHFlags=1
  ```
  **17/17 が magic 一致（FuncInfo レイアウト完全成立）**。例: wrapper 0x9B91F0 ← pUnwindMap 0xAE35C4 ← FuncInfo 0xAE35CC / wrapper 0x9D5D80 ← 0xAFDD8C ← 0xAFDD94。
- **上流コードも特定**: 各 wrapper 直後の trampoline が `call 0x9811e2`（= `cmp ecx,[0xb30aac]; je; repz ret / jmp 0x98c49c` = **`__security_check_cookie` 相当**）の後に `mov eax, <FuncInfo>; jmp 0x980EA4`。
  **`0x980EA4` は台帳登録済み = `FID_conflict:___CxxFrameHandler3`**（本体は `call 0x98BA0C` = 台帳 `___InternalCxxFrameHandler`、`func_list_new` L18145）。
- **裁定**: C 群 18 個の「E8/E9 0 件・imm32 テーブル参照のみの未ラベル wrapper」の正体 = **C++ 例外の unwind action（stack unwinding 時に実行されるデストラクタ呼び出し thunk）**。呼出し元は「登録コード」ではなく **try/catch を持つ各関数の FuncInfo データ構造**であり、実行時の到達経路は `___CxxFrameHandler3`（例外発生時のみ）。R-a は**解明**。
- v162 §3-1「wrapper 経由の上流（誰が呼ぶか）はテーブル走査側（登録コード）」という前提は**誤り**だった（登録コードは存在しない。コンパイラが FuncInfo を静的配置する）。v162 §7-(a) の移植示唆「wrapper → まずスタブ化」も**微修正要**（EH アクションは通常実行時フローに無く例外時にのみ実行 → 移植側では例外機構の実装方針に合流させる。新規残点 N-b）。

### 6-2 [R-h] Cat-3b 0x87AD10 / 0x884D50 の登録元 — **解明（vtable エントリ）**

- 0x87AD10: `.rdata` 0xA941AC..0xA9426C の **code-ptr ラン（49 エントリ）** の 1 つ。登録コード = **`0x80AD9B: mov dword ptr [esi], 0xa941ac ; call 0x984bcf`**（`0x984bcf` は台帳 `` `eh_vector_constructor_iterator' ``）+ `push 0x7016a0; push 0x7c88a0; push 0xa`。
- 0x884D50: 0xA9459C..0xA94664 の **code-ptr ラン（51 エントリ）** の 1 つ。登録コード = **`0x810B25: mov dword ptr [esi], 0xa9459c ; call 0x984bcf`**（同一パターン）。
- 両者本体は **SEH フレーム構築プロローグ**（`push -1; push <handler 0x9D5740/0x9D5D60>; mov eax, fs:[0]; push eax; sub esp,0x10; push ebx/ebp/esi…`）= 実在関数。
- **裁定**: 2 件は vtable に入っている = **実在の仮想関数**であり、decompiled.c 欠落（Cat-3b）はツール側の欠落。v162 R-h「登録元未特定」は**解明**（残るは「decompiled に为何欠落したか」のツール側原因＝v127 R2 系列のみ）。

### 6-3 v162 残点 A..J の能動裁定（解けるもののみ）

| ID | 判定 | 根拠 / 試行 |
|---|---|---|
| R-b（BA0002 634 成員 split） | **維持（未解明）** | 634 関数の全文本文再観測＋v133-H（thunk 連鎖正規化）設計が前提。本タスクのコスト配分では見送り。観測定義差（first-exit≤600B vs 全文抽象）であることは v162 記載のまま |
| R-c（比較不能 2,771 本） | **維持** | k≤3 縮小 or FWD2 正規化の設計判断が親セッション依存（低優先のまま） |
| R-d（L3 53 件が L1 領域外） | **維持** | `v127_cat3_regions.json` 再生成時の確認事項。本タスクでは領域再生成禁止 |
| R-e（L3 ⊂ 却下領域 11 件） | **維持** | cls=F:noncode の領域分類が粗いという v162 判定のまま。入口 accept は正 |
| R-g（/OPT:ICF） | **維持（永続）** | PE 単体で判定不能。OBJ/リンカログ不入手なら決着不可 |
| R-i（V148） | **維持（範囲外）** | 元のタスク範囲外裁定のまま |
| R-j（v155 S4） | **維持（分析専任で実施不可）** | V159 起票済みのまま。本タスクでは編集しない |

---

## §7 Rs1–Rs6（read_ptr 防御修正束）の裁定

| ID | 対象（実測確認） | 裁定 |
|---|---|---|
| **Rs1** | `v142_p5_final.py` L8–16 `rd_u32` = `RD_OFF+(va−RD_VA) if va>=RD_VA else TEXT_OFF+(va−TEXT_VA)`（.text/.rdata の 2 定数のみ） | **防御的修正は不要**。現行呼び出しは .data（0xB02000+）引数 0 件（v165 で機械チェック済）＝無害。既存スクリプトは過去証跡として非編集（Rs6 原則）。将来 util 化する場合は `probe_harness.Ctx.rd`（セクションテーブル三分岐）へ**統一**すること（本裁定は「修正しない」ではなく「util 化時に統一」の条件付き不要） |
| **Rs2** | `v138_11_verify.py` L86–133 等の `buf[va−0x400C00]` 直書き（.text 定数） | **同上・不要**。全件 .text 引数を v165 で実測済み。util 化時の統一要件は Rs1 と同じ |
| **Rs3** | `tmp/re/s65_hc_vtable.py`「byte at 0xA12040」行の `raw[rva − s.VirtualAddress]`（PointerToRawData 欠落＝別種の誤換算）を本タスクで該当行実測確認 | **修正不要**。単発デバッグ出力 1 行で数値成果物に波及なし（v165 裁定を追認）。「v90 未満スクリプト全数走査」は本タスクでも**未実施**（範囲外のまま維持） |
| **Rs4** | **正式計測完了**（`v174_p7.py` = `v162_p2fix_cascade.py` の read_ptr のみ差替 exec・edge-kind カウンタは v162 実装そのまま）: **buggy `{'direct':164,'memabs':6210}` / fixed `{'direct':164,'memabs':13}`** | R-k の「memabs 6,210 件」を **identity 再現して確定**（buggy 側は P3 unlabeled 40,724 / mix 40,502+222 / unknown band 009E27B0..009E4200 で v140 公表と一致）。fixed 側は v162 の 13 と一致。併せて `.text` 内 FF15/FF14-25 絶対間接 call サイト総数 **7,035**（targets in .text: buggy 7,000 / fixed 7,008）を計測。**代用指標（seen 差 1,208 等）は本測定により不要化**。証跡 `%TEMP%\opencode\v174_rs4_buggy.txt` / `v174_rs4_fixed.txt` / `v174_rs4_out.txt` |
| **Rs5** | v140d_out の該当行を実測: `66 not in R1+adj: 6 -> 0x4672D0 0x4F65B0 0x4F7750 0x4F7800 0x4F78B0 0x4F7EF0` / `66 in R1+adj: 60` | **記録整合は「doc の 5 が正」で確定**（v165 の fixed 再計算で gen61/非生成5）。v140d の 6 との差 1 件 = **0x4672D0 の FF E0 誤 terminator 経路の有無**（v140d 実装は同経路で生成しないため非生成に計上）。v140d_out はバグ版証跡としてそのまま保存し、参照時は本注記を使う |
| **Rs6** | v140b_verify / v140c_adjfilter / v140d_sites / v140e_cascade / v140f2_cascade（＋範囲外 v152_p2_cascade）の upstream 修正要否 | **「証跡保存＋訂正版へ誘導する注記」を正式採用**（v165 §5-4 推奨を踏襲・本タスクで確定）。理由: ①`v140_e8_66_finalization.md` §3-2 等の旧数値がバグ版スクリプト出力と対応しており、スクリプト改変で証明連鎖（identity 検証）が壊れる、②訂正版が既に存在（`v162_p2fix_cascade.py` / `v165_cascade.py` fixed）、③本タスクは編集不可。誘導方法: 参照時に「read_ptr バグあり（+0xC00/+0xE00/+0x1400）。訂正＝v162_p2fix / v165_cascade fixed」を明記。**upstream 修正は行わない** |

---

## §8 新規残点

| ID | 残点 | 優先度 | 内容 |
|---|---|---|---|
| **N-a** | XCU 台帳化候補 1,199 件の採否 | 中 | `v174_xcu_candidates.csv`（1,199 行）を提出済み。統合表 Cat-3/v86 層（6,181）とは別に**完全未登録**の層。V159/V170 の承認フローでの採否判断（分析専任のため本タスクでは登録しない） |
| **N-b** | v162_gap_residuals.md の訂正反映 | 低 | ①§3-3(d)「.text の補助/thunk 帯」→ 正: **hint/name 構造体（RVA 表現）**、②§7-(d)「未知 3 件 → 先に名前付きスタブ化」示唆の**取り消し**（インポート API である）、③§7-(a)「C 群 wrapper → まずスタブ化」→ **EH unwind action（例外時のみ実行）**へ微修正、④§3-1「上流はテーブル走査側（登録コード）」→ **登録コードは存在しない（FuncInfo 静的配置）**に訂正。既存 tmp/re 上書き禁止のため本ファイル §3/§4/§6-1 が訂正記録。親セッションが v162 へ追記するか判断 |
| **N-c** | R-f の Ghidra `Function.getBody()` 突合 | 低 | OblivionRe プロジェクトが稼働 Ghidra インスタンス（現 d1tamers_analysis）に接続された時点で `v174_r_f_noterm_spans.csv`（1,376 行）と突合し暫定決着（§5-2）を確定させる |
| **N-d** | R-m/R-a に伴う移植示唆の更新 | 低 | v162 §7-(d)/(a) の訂正（N-b ②③）に伴い、Android 移植側の未実装ログ分類（「IAT 未知 3 件」「wrapper スタブ化」層）は**層の削除・変更**が必要 |

---

## §9 自己検証・証拠一覧

- **`probe_harness.py selftest` → 4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
- **R-l**: 配列 **29,608 B / 7,402 エントリ / 一意 7,380（NULL 23）** = v162 と一致。突合 713（func_list）/ 6,181（union）/ 1（v170 CSV）/ 完全未登録 **1,199**（CSV 書出行数と一致）。サンプル 18 件で識別子文字列 10 件、4 型（GMST 登録 / Ni クラス登録 / singleton 生成 / atexit 破棄）に分類。消費者 0x981C6B の E8 caller は 0x9877E1 の 1 件・`_initterm` ループ（esi=0xA283D0 / edi=0xA2F77C / `call eax` @0x981CCB）を再確認。
- **R-m**: IAT スロット × import 名は pefile `DIRECTORY_ENTRY_IMPORT` の name と FirstThunk 範囲の**両方**で一致（entry#19 / #73 / USER32#24）。hint/name 構造体実体は `pe.get_data(RVA, 24)` で `53 00 'CreateFileA'` / `c4 00 'FileTimeToLocalFileTime'` / `eb 00 'GetActiveWindow'` を直接確認（＋0x700A6E=`GlobalMemoryStatus` / 0x700A84=`GetCurrentThreadId`）。delay import ディレクトリなし。逆アセンブルが mid-instruction で始まる点と整合。
- **R-a**: FuncInfo レイアウト **17/17 magic 一致**（`v174_p4.py`）。上流 `mov eax,<FuncInfo>; jmp 0x980EA4` は 4 サイト実測（0x9B915C / 0x9B9222 / 0x9D5D9C / 0x9D5DFC）。`0x980EA4` = 台帳 `FID_conflict:___CxxFrameHandler3`（func_list_new / v155_union 両方に登録行あり）、本体 `call 0x98BA0C` = 台帳 `___InternalCxxFrameHandler`。GS check `0x9811E2` は `cmp ecx,[0xb30aac] / je / repz ret / jmp 0x98c49c`。
- **R-h**: vtable code-ptr ラン 0xA941AC..0xA9426C（49）/ 0xA9459C..0xA94664（51）、登録コード `mov [esi],<vtable>; call 0x984bcf` = 台帳 `` `eh_vector_constructor_iterator' `` を 2 サイト（0x80AD9B / 0x810B25）実測。
- **R-f**: `v133_4_probe` 定義再現で **internal-cc 4,923 / no-term 1,376**（v133 と完全一致）、例 0x402D00 cc@81 / 0x4048B0 cc@76 も一致。no-term の cc 位置は台帳/統合表/候補 CSV とも **0/1,376 登録**。
- **Rs4**: buggy `{'direct':164,'memabs':6210}`（P3 unlabeled 40,724 / mix 40,502+222 / band 009E27B0..009E4200 = v140 公表一致）/ fixed `{'direct':164,'memabs':13}`（band 00700A9A..00700F06 = v162/v165 一致）。FF15 総サイト 7,035。
- **Rs5**: v140d_out の実行行 `66 not in R1+adj: 6 -> 0x4672D0 0x4F65B0 0x4F7750 0x4F7800 0x4F78B0 0x4F7EF0` / `66 in R1+adj: 60` を直接確認。
- 新規スクリプト（すべて `%TEMP%\opencode\`）: `v174_p1.py`（R-l+R-m）/ `v174_p2.py`（R-m hint/name + R-a/R-h 初回）/ `v174_p3.py`（.rdata 全域スキャン）/ `v174_p4.py`（FuncInfo 検証 + vtable 参照）/ `v174_p5.py`・`v174_p5dbg.py`（R-f 定義探索）/ `v174_p6.py`（R-f 再現+突合）/ `v174_p7.py`（Rs4 memabs）/ `v174_p8.py`（0x980EA4/Rs5）。
  出力: `v174_p1_out.txt`…`v174_p8` 相当、`v174_rs4_buggy.txt` / `v174_rs4_fixed.txt` / `v174_rs4_out.txt`、CSV `v174_xcu_entries.csv`（7,402 行）/ `v174_xcu_candidates.csv`（1,199 行）/ `v174_r_f_noterm_spans.csv`（1,376 行）。
- 禁止事項の遵守: `docs/`・`TODO_re49.md`・共有台帳（`func_list_new.txt` / `v155_union.csv` / `v155_label_ledger_union.csv` / `v133_dup_results.csv` / `v170_ledger_append_candidates.csv`）・既存 `tmp/re/*.md`（v162 / v165 / v140 系ほか）・`tmp/re/cache/*`・git・session-memory は**無編集**。本ファイル（v174）のみ新規作成。ESM 再スキャンなし・Ghidra 再スキャンなし（MCP は list/connect の確認のみ）。
