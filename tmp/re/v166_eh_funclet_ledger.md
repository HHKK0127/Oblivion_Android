# V166: C++例外 funclet 帯 TryBlockMap 一括走査 — 台帳化（665 → 10,742 エントリ）

- 作成: 2026-10-27 / タスク: TODO serial 156（V156 残点 R2 = v156 §5 の「189KB EH funclet 665 件の一括台帳化」）の実施。
- 前提: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時の 2 回）。
- 検証対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B）。capstone 5.0.7（skipdata）。VA=off+0x400C00(.text)/+0x400E00(.rdata)/+0x401400(.data)。
- 編集範囲: **本新規 1 ファイル + `%TEMP%\opencode` 新規のみ**。docs / TODO / session-memory / plan / `func_list_new.txt` / 既存 `tmp/re/*` は無改変。**Ghidra 不使用**（capstone + decompiled.c 部分検索不使用〔本タスクでは不要〕+ .rdata 生ダンパで完結）。git 不使用。ESM 再スキャンなし。
- 入力（読取専用）: `tmp/re/v156_tail_band_identity.md`、`%TEMP%\opencode\v156_tail_entries.json`（665 エントリ）、`v127_cat3_regions.json`（16,660 region）、`func_list_new.txt`（21,597）、`tmp/re/v155_label_ledger_union.csv`（31,905 行）。

---

## (0) 手法と口径

| 項目 | 内容 |
|---|---|
| 走査対象 | 帯域 `[0x9A9D50, 0x9D8020)`（189,136 B / v127 Cat-3 開始点 **665**）。 |
| FuncInfo 検出 | .rdata aligned dword 全走査で **`magic==0x19930522` を 2,663 件**検出し、全件レイアウト整合検証（0 例外）。 |
| ロール展開 | 各 FuncInfo から unwindMap（maxState 分）/ TryBlockInfo 配列（nTryBlocks 分）→ CatchHandler 配列を辿る。 |
| 参照センサス | ①.rdata/.data aligned dword → 帯域値 **7,957 件** ②.text 全域の imm32（numpy sliding-window）→ 帯域値 6,932 / FI 値 2,678 ③`6A FF 68 <imm32>`（SEH 登録プッシュ）4,387 サイト。 |
| 終端判定 | 各ブロックを「次の参照済ブロック開始 or 2 連続 int3」まで線形デコードし、最後の jmp/call/ret を記録。 |
| 宿主特定 | thunk プッシュ site / FI imm32 site の **enclosing = `func_list_new.txt` の二分探索**（台帳粒度）。catch は handler 自身の enclosing で独立検証。 |
| 突合 | v155 統合 CSV 31,905 行（VA 列）に全行照合。I1–I10 は明示アドレス照合。 |
| 再現 | `python "%TEMP%\opencode\v166_probe1.py"` … `v166_probe6.py` → `python "%TEMP%\opencode\v166_scan.py"`（本体・CSV 出力）。 |

---

## (1) EH 構造の確定 — 帯域の完整模型

### 1-1 FuncInfo レイアウト（本実測・VC8 系 x86）

```
FuncInfo (+0):  magic = 0x19930522
                maxState            (+0x04, int)
                unwindMap*          (+0x08)  -> UnwindMapEntry[maxState] {toState(int32), action*}
                nTryBlocks          (+0x0C, int)
                TryBlockInfo*       (+0x10)  -> TryBlockInfo[n] {tryLow, tryHigh, catchHigh, nCatches, CatchHandler*}
                (+0x1C)             dispatcher 検査用（本バイナリ全 FI で 0）
                flags byte (+0x20)  dispatcher の `test byte [eax+0x20],1` 対象
CatchHandler (16B): {adjectives, pTypeDescriptor*, objadj(int32), handler*}
                pType=0 → catch(...)（115 件）、型付き → TypeDescriptor（29 件）
```

- **FuncInfo = 2,663 個**（try 付き = nTryBlocks>0 は **90** 個）、dispatcher `0x98BA0C`（= `mov ebx,0x19930522` の比較実装）は magic `& 0x1FFFFFFF >= 0x19930522` を検査。
- **unwind action = 7,915 個（全数が帯域内、7,915/7,915）**、**catch handler = 144 個（全数が .text 中間、帯域内 0）**。unwind の重複被参照 4 件。
- .rdata 帯域参照 7,957 の内訳: unwind action **7,921**（distinct **7,915**、同一値への多重参照 6）/ **初期化配列（.CRT$XCU 系）26** / **帯域形状不明 10**（`toState` 形状だが FI 未該当 2 + `other` 8。実体は .data の 16bit テーブル・浮動小数点配列への偶然一致 `0x9C0000` `0x9B0900` `0x9BE959` 等、コード先頭ではない）。distinct 合計 7,949、多重参照 8 組 = probe3 の「multiple ref sites 8」と一致。

### 1-2 **第 4 の参照経路 = SEH 登録プッシュ（v156 §1-5 に不在だった経路）**

関数プロローグの実バイト列（全 4,387 サイト、例 0x4010C0）:

```
push -1                 ; 6A FF
push <band thunk>       ; 68 xx xx xx xx   ← 帯域 addr を直接参照！
mov eax, dword ptr fs:[0] ...
```

- これを v156 が調べなかった **`.text` imm32 参照**（E8/E9 ではない）。distinct 対象 = **2,657 thunk**（**全数が帯域内**）。
- **thunk の正体 = cookie チェック + continuation**: `mov edx,[esp+8]; lea eax,[edx-N]; mov ecx,[edx-M]; xor ecx,eax; call __security_check_cookie(0x9811E2); mov eax, <FuncInfo>; jmp 0x980EA4`。**2,657/2,657 が有効 FI を埋込み、2,657/2,657 が `jmp 0x980EA4` で終端**。
- つまり帯域 thunk は **OS から直接呼ばれる登録済みフレームハンドラ**であり、`E8/E9=0` でもこの `push imm32` 経路で実行時に到達する。v156 §1-5 の参照種別表にこの経路を追加すべき（→ 残点 R6）。

### 1-3 実行時モデル（本実測から復元）

```
例外発生 → OS が登録 thunk（帯域）を呼ぶ（引数: ExcRecord, EstFrame, Ctx, DC）
        → thunk: cookie 検証 → mov eax,FI → jmp ___CxxFrameHandler3(0x980EA4)
        → 内部 dispatcher 0x98BA0C:
            (search) nTryBlocks>0 → TryBlockInfo → CatchHandler → catch handler 呼出
            (unwind) maxState と unwindMap をループ（cleanup funclet を call、funclet は ret で復帰）
        catch handler: 本体実行後 `mov eax,<next>; ret` でランタイムに復帰（_JumpToContinuation 経路は 1 サイトのみ）
```

- **U ブロック（cleanup funclet）の終端**: `ret` 2,308 / dtor ヘルパへの tail-jmp 群 — `0x75FA70`(refcount +0x5c 減算→delete) **1,524** / `0x7016A0`(IAT Interlocked 系 delete) **866** / `0x403BC0`(バッファ free) **644** / `0x402DA0`(文字列 dtor) **527** / `0x79AB00`(文字列 dtor) **176** / `0x4027D0`(refcount +0x60) **106** / `0x401010`(`push ecx; call [IAT 0xA28068]; ret` = import thunk 経由クリーンアップ) 3 …（dtor tail-jmp 先は ret で dispatcher に復帰する）。
- **E9 → 0x980EA4 は .text 全域で 2,663 サイト = FuncInfo 数と完全一致**（うち thunk 内 2,658、thunk 外 5 = 下記 1-5）。

### 1-4 665 開始点の構成（v127 開始点 = region先頭）

| 種別 | 件数 | 備考 |
|---|---:|---|
| **unwind**（unwindMap action = region 先頭） | **611** | cleanup funclet 本体が region 先頭 |
| **seh_thunk**（登録 thunk が region 先頭） | **28** | cookie+continuation が region 先頭 |
| **xcu_stub**（初期化配列参照） | **25** | v156 の callstore 21 + `mov` 系 3 + `pushcall` 1（→ 残点 R7 口径差） |
| **unref** | **1** | `0x9AACE8`（下記） |
| 合計 | **665** | |

- **`0x9AACE8`**: 帯域内外のどこからも参照なし（`.text` の「参照」は `E8` rel32 バイトの偶然一致）。直前ブロック（region 0x9AACD0）の cookie チェック**中間**（`xor ecx,eax; call 0x9811E2; mov eax,0xad7bfc; jmp 0x980ea4` の途中）を指す **偽開始点**。union にも不在（唯一の absent）。

### 1-5 thunk 非登録 FI 6 件（例外群）

`push -1; push` 形式の登録が無い FI = **`0xAFF75C` `0xAFF788` `0xAFF7B4` `0xAFF7E8` `0xAFF81C` `0xAFFCF8`**（maxState 1–2、連続クラスタ、うち 1 件のみ try 付）。continuation（`mov eax,FI; jmp 0x980EA4`）は帯域末尾 `0x9D7C16–0x9D7CF0` に 5 箇所（E9 thunk 外）+ 未捕捉 1。**宿主不明の U ブロック 7 件（7,915 中）はすべてこの 6 FI 由来**。`0xAFFCF8` の catch handler は台帳名付き CRT 関数 **`CallUnexpected` の内部（0x98B17C = +0x31）**。

---

## (2) 台帳化結果 — `v166_eh_entries.csv`（10,742 行）

### 2-1 列仕様

| 列 | 内容 |
|---|---|
| `VA` | 8 桁 hex |
| `kind` | `unwind` / `seh_thunk` / `catch` / `xcu_stub` / `unref` |
| `scope` | unwind = `s{fromState}->{toState}`（try 範囲に掛かる場合は `;tryN[L..H]`）/ catch = `tryN[L..H];catchHigh=H` / thunk = `frame` / 他 = `-` |
| `host_guess` | 台帳 enclosing（`VA:NAME`、複数は `first(+N more)`。空 = 宿主不定） |
| `notes` | `region_start` / `fi=` / `states=` / `term=` / `push_sites=` / `union:…` / `host_via=…` / catch は `adj`・型・`enclosing`・`agree` |

### 2-2 件数内訳

| kind | 行数 | うち region_start（= 665） | 宿主付与 |
|---|---:|---:|---:|
| unwind（cleanup funclet） | **7,915** | 611 | 7,908（**99.9%**） |
| seh_thunk（SEH 登録ハンドラ） | **2,657** | 28 | 2,657（**100%**） |
| catch（TryBlockMap 由来・帯域外 .text 中間） | **144** | 0（帯域外） | 144（100%、うち FI 突合一致 **143**） |
| xcu_stub（初期化スタブ） | **25** | 25 | —（CRT 初期化系のため宿主なし） |
| unref（偽開始点） | **1** | 1 | — |
| **合計** | **10,742** | **665** | **10,709（99.7%）** |

- **宿主突合率（検証付き）**: U ブロック = FI→thunk→push-site 経由で 7,908/7,915；catch = handler の enclosing と FI 宿主の一致 **143/144（99.3%）**。不一致 1 = `CallUnexpected` 内部 handler（1-5、FI 自体が thunk 非登録のため fi_hosts 空）。
- **catch 内訳**: `catch(...)` **115**（adjectives=64）+ 型付き **29**（adjectives=8、型は **全件 `.?AVexception@std@@`**）。try 付 FI 90 から 144 handler を全数展開。
- `scope` に try 注記が付く行 = **179**（catch 144 + try 範囲内の state 遷移 U 行 35）。
- **宿主共有（shared registration）**: 1 thunk を複数関数が push する例 = **FI 333 個**が複数宿主（最大 **253 関数**が同一 FI/thunk `0x9CA7EB`（FI `0xAF2E8C`, maxState=1, actions=1）を共有）。同一フレームレイアウトの関数群が同一登録を共有した結果（→ 残点 R3）。

---

## (3) `0x980EA4` 466 件と残 199 件の裁定

### 3-1 v156 測定の完全再現（口径同一）

`v156_probe2.py` の分類器（`first = mov …ebp` ∧ `last = jmp`、窓 = `[start, min(region_end, start+200))`）を**逐語再実装**:

| 測定 | v156 記載 | 本タスク |
|---|---:|---:|
| catchstub クラス | 469 | **467** |
| うち `jmp → 0x980EA4` | **466** | **466（完全一致）** |
| うち `jmp → 0x4027D0` | — | 1 |

- **466 は「region 窓の最終 jmp が thunk の continuation」だった開始点の数**であり、エントリ自身（cleanup 本体）の終端ではない。エントリ自身の終端は `ret` 396 / dtor tail-jmp 70（`0x7016A0` 27、`0x402DA0` 14 他）。
- **残 199 件 = 665 − 466** の内訳（catchstub 467 のうち 1 件は `jmp→0x4027D0` なので、非 catchstub 198 + catchstub 内 1 = 199）:

| 分類（first-insn 口径） | 件数 | kind 内訳と実体 |
|---|---:|---|
| `other:mov` | 100 | unwind **97**（cleanup 本体 `mov…` 開始）+ xcu **3** |
| `other:push` | 31 | unwind **30**（`__ehvec_dtor` 系配列破棄: `push dtor; push count; push size`）+ xcu **1** |
| **`cookie-first`（`mov …,[esp+8]`）** | **28** | **seh_thunk 28 = 665 構成表と完全一致**（SEH 登録 thunk が region 先頭） |
| `other:call` | 21 | xcu **21**（初期化 callstore） |
| `other:.byte` | 16 | unwind **16**（デコード開始不良 = **mid-instruction 偽開始点**。U 参照は有り → 残点 R5） |
| `other:xor` | 1 | unref **1**（`0x9AACE8` 偽エントリ） |
| `other:cmp` | 1 | unwind **1**（cleanup 本体） |
| **非 catchstub 小計** | **198** | （+ catchstub 内 `jmp→0x4027D0` **1** = **199**） |

- 構成チェック: 非 catchstub 198 = unwind **144**（97+30+16+1）+ seh_thunk **28** + xcu **25**（3+1+21）+ unref **1**。665 = catchstub **467** + 198。

### 3-2 領域最終終端（665 全数・クリーン判定）

| 最終終端 | 件数 |
|---|---:|
| `jmp → 0x980EA4`（末尾 thunk の continuation） | **640** |
| xcu_stub（初期化スタブで終端） | **25** |

（466 との差 = 窓サイズ/first-insn 制約の有無。**構造的不変式: 665 = 640 + 25**。）

### 3-3 `0x980EA4` の正体 — v156 記述の訂正

- 台帳名は **`FID_conflict:___CxxFrameHandler3` @0x980EA4**（前後: `_UnwindNestedFrames` @0x980E52（終端 0x980EA3 まで）、`_CallSETranslator` @0x980F0A）。
- 本体は独立プロローグ（`push ebp; mov ebp,esp; …`）で、引数 4 個 + **eax** を受け内部 dispatcher **`0x98BA0C`** へ 8 引数で転送するラッパ。eax = **FuncInfo**（dispatcher が `[eax]` を magic として検査）。
- → v156 §1-2.3 の「`_UnwindNestedFrames` の中間（+0x52）＝内部エントリ」は**要訂正**（`_UnwindNestedFrames` の body は 0x980EA3 で終わる別関数）。機能的説明「unwind 継続の再ディスパッチ入口」自体は正しい（→ 残点 R6 docs 訂正・承認制）。

### 3-4 466/199 の位置づけ（結論）

- **466 = 「thunk 登録 + cleanup 本体」の二段構造を持つ region のうち、窓末尾が thunk continuation に達した群**。
- **残 199 = 非 catchstub 198（seh_thunk 先頭 28 + 初期化スタブ 25 + cleanup 本体 144 + 偽開始点/偽エントリ 17）+ catchstub 内 `jmp→0x4027D0` 1**（3-1 表の合算、665 = 467 + 198 と整合）。
- 帯域の**機能構造**: cleanup 7,915 + 登録 thunk 2,657 + （帯域外）catch 144 = **実 EH エントリ 10,716**、+ 初期化混入 25 + 偽 1 = **10,742 行**。

---

## (4) 6 層統合表（v155）との重複裁定

| 集合 | 行数 | L1（台帳） | L3（V86） | L4（Cat-3） | union 不在 | I_no |
|---|---:|---:|---:|---:|---:|---:|
| **665 開始点** | 665 | 0 | **664** | **663** | **1**（`0x9AACE8`） | 0 |
| unwind 全行 | 7,915 | 0 | 611 | 612 | 7,302 | 0 |
| seh_thunk 全行 | 2,657 | 0 | 28 | 30 | 2,627 | 0 |
| catch | 144 | 0 | 0 | 0 | 144 | 0 |
| xcu_stub | 25 | 0 | 25 | 25 | 0 | 0 |
| **CSV 全体** | **10,742** | **0** | — | — | **10,074**（存在 668） | **0** |

- **裁定 1**: 665 開始点は **L3/L4 に既出**（= v127 Cat-3 由来、v155 と矛盾しない）。唯一の union 不在 `0x9AACE8` = 1-4 の偽開始点。**L3 のみ 1 件 `0x9A9D50`** は v155 §5 の interior-4（`0x73863C/0x73EF3B/0x990D10/0x9A9D50`）と**完全一致**（Cat-3 未採用＝口径差の既知件）。
- **裁定 2**: CSV の新規供給分は **interior 行 9,933**（U interior 7,304 + thunk interior 2,629）+ catch 144。統合表は「関数開始点」粒度のため interior は構造的に不在（absent が正常）。
- **裁定 3**: **I1–I10（V136 §3 別紙）と交差 0**、**台帳（L1）交差 0 = 既登録 0 件**。よって本 CSV は全行が**追記候補（新規）**。
- **裁定 4**: 本 CSV は v155 の 6 層に対して**第 7 層（EH funclet 層）候補**。ただし登録粒度（665 のみか 10,742 全か）は親裁定待ち（→ 残点 R2）。

---

## (5) 成果物

| 種別 | パス / サイズ |
|---|---|
| **本成果物（新規・UTF-8）** | `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v166_eh_funclet_ledger.md` |
| **台帳 CSV（新規）** | `%TEMP%\opencode\v166_eh_entries.csv` — **1,306,165 B / 10,742 行 + ヘッダ** |
| 本体スクリプト | `%TEMP%\opencode\v166_scan.py`（FI 走査→CSV 出力・再現: `python v166_scan.py`） |
| 探索スクリプト（新規） | `v166_probe1.py`（参照/magic 初走査）〜 `v166_probe6.py`（終端ヘルパ・thunk 外 E9・catch 不一致・CSV 抜粋） |
| 中間出力 | `%TEMP%\opencode\v166_scan_out.txt`（13,861 B・全集計）/ `v166_probe1_out.txt`（8,313）/ `probe2`（7,093）/ `probe3`（5,862）/ `probe4`（4,778）/ `probe5`（11,453）/ `probe6`（184,963、E9 全数列のため大） |
| 読取のみの参照 | `tmp/re/v156_tail_band_identity.md`・`v155_label_ledger_union.md`・`v155_label_ledger_union.csv`・`func_list_new.txt`・`%TEMP%\opencode\v156_tail_entries.json`・`v127_cat3_regions.json`・`probe_harness.py` |
| 未実施（禁止事項） | docs / TODO / session-memory / plan / `func_list_new.txt` / 既存 tmp/re の編集、git、Ghidra、ESM 再スキャン **すべて未実施** |

---

## (6) 新規残点（V167+ 起票粒度）

| # | 提案 V-id タイトル | 優先 | 内容 |
|---|---|---|---|
| R1 | **V167: 189KB 帯の台帳追記粒度裁定（region 開始点 665 vs 実エントリ 10,716 vs 全 10,742）** | 高 | Ghidra への一括登録方針。interior 9,933 行は「Ghidra 未ラベルの実エントリ」だが関数化すると親 region と二重になる可能性。catch 144 は既存関数の interior（独立関数化の可否）。 |
| R2 | **V168: 共有 FI/thunk 333 件（最大 253 関数共有）の実態裁定** | 中 | 同一登録を共有する関数群（例: FI `0xAF2E8C` = 253 関数）の同一性条件（フレームレイアウト一致）の検証。台帳化時の host_guess 複数表記の扱い。 |
| R3 | **V169: thunk 非登録 FI 6 件（`0xAFF75C`–`0xAFFCF8`）の登録経路特定** | 中 | `push -1; push` が無い。外部 FI imm32 参照 15 サイト（`mov reg,FI` 型）との対応、または未登録のままか。`CallUnexpected` 内部 handler を含む CRT 内部群。 |
| R4 | **V170: `0x980EA4` = `___CxxFrameHandler3` への docs 訂正（v156 §1-2.3「_UnwindNestedFrames 内部エントリ」）** | 低 | 本タスクで台帳名 + 実測（独立プロローグ、内部 `0x98BA0C` へ 8 引数転送）を確定。docs 編集は承認制。 |
| R5 | **V171: 帯域偽開始点 17 件（`.byte` 16 + `0x9AACE8`）の再定義** | 低 | mid-instruction 開始。開始点=「先頭命令デコード可 ∧ 参照あり」等への再ルール化と v127 JSON への反映要否。 |
| R6 | **V172: v156 §1-5 参照種別表への「.text push-imm32 登録経路（4,387 サイト/2,657 thunk）」追加** | 中 | 「E8/E9=0＝到達経路不在」の結論は維持（Ghidra は push-imm32 を関数化しない）が、参照センサスに第 4 経路を追記。 |
| R7 | **V173: xcu_stub 口径差（v156 callstore 20+pushcall 1 vs 本タスク 25 = call 21 + mov 3 + push 1）** | 低 | `v156_tail_entries.json` の pat 分類と本タスク kind 分類の差分原因（first-insn 口径）の確定。 |

---

## (7) 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時の 2 回）。
- 内部整合（一致した項目のみ）:
  1. FuncInfo 検出 **2,663**、レイアウト例外 **0**（maxState/unwindMap/nTryBlocks/TryBlockInfo 全件整合）。
  2. unwind action **7,915 = 帯域内 7,915/7,915**（帯域外 0）。
  3. .rdata 帯域参照 7,957 = U 7,921 + XCU 26 + 偶然 10（7,957 と一致）。
  4. `6A FF 68` 4,387 サイト → distinct **2,657**、帯域内 **2,657/2,657**、FI 埋込 **2,657/2,657**、終端 `jmp 0x980EA4` **2,657/2,657**。
  5. **E9 → 0x980EA4 = 2,663 = FuncInfo 数**（thunk 内 2,658 + thunk 外 5 + 端数 0、`0x9D7C57/7A/A8/D5/CF0` を明示列挙）。
  6. 665 開始点構成 611+28+25+1 = **665**。領域最終終端 640+25 = **665**。
  7. v156 分類器逐語再実装: catchstub **467**（v156 469、差 2 = decode 窓端の int3 有無）/ **`jmp→0x980EA4` 466 完全一致** / `0x4027D0` 1。
  8. v155 統合 CSV **31,905 行**を全行照合: 665 = L3 664 / L4 663 / absent 1、**L1 0 / I_no 0**、catch 144 全 absent、CSV 全体 present 668 + absent 10,074 = **10,742**。
  9. `0x9A9D50` = L3 のみ（v155 §5 interior-4 の 4 件と一致）。I1–I10 10 アドレスと CSV 行集合の交差 **0**。
  10. 宿主: U **7,908/7,915**、thunk **2,657/2,657**、catch enclosing↔FI 一致 **143/144**、CSV 宿主付与 **10,709/10,742**。
  11. catch 内訳 115（pty=0）+ 29（typed = `.?AVexception@std@@`）= **144**、try 付 FI **90**。
  12. 台帳交差 **0**（`func_list_new.txt` 21,597 行と全 10,742 行が互いに素）。
- 禁止事項の遵守: docs / TODO / session-memory / plan / func_list / 既存 tmp/re の編集、git、Ghidra、ESM 再スキャン **すべて未実施**。新規ファイルは本 MD + `%TEMP%\opencode` 配下（`v166_*.py` / `v166_*.txt` / `v166_eh_entries.csv`）のみ。

---

生成物: `tmp/re/v166_eh_funclet_ledger.md`（新規） + `%TEMP%\opencode/v166_eh_entries.csv`（新規・10,742 行） / 生成スクリプト: `%TEMP%\opencode/v166_scan.py`, `v166_probe1.py`–`v166_probe6.py` / 前提: `probe_harness.py selftest` 4/4 PASS
