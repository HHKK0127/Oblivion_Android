# V140 — §70.16 (5)-4: 新規 66 件の確定絞り込みと A 節 64 件の切り分け

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / SHA-256 先頭 `a8f313845c1545e9…`。`OblivionStandalone.exe` と**同一バイナリ**であることをハッシュ照合で確認。ImageBase 0x400000 / `.text` VA 0x401000–0xA27C39 / raw 0x400 / size 6,450,233）
- 作成日: 2026-10-27。担当: opencode サブエクト（分析専任）。
- 着手時 probe_harness selftest **4/4 PASS**。
- 証拠台帳（読取専用）: `tmp/re/v96_e8_candidates.txt`（565 行 = 台帳外 E8 ターゲット全数）/ `tmp/re/v92_e8_214_triage.txt`（214 行）/ `tmp/re/v95_recall_gap.txt`。Ghidra ground truth 21,597 開始点（`.text` 内 21,591）。
- **本節の数値はすべて独立再実装（新規スクリプト `%TEMP%\opencode\v140f_final.py` / `v140f2_cascade.py` / `v140f3_64.py`）で再現**したものを記載する。前回中断セッションの中間出力（`v140_step1..6` / `v140b..v140e`）とは別実装で一致した項目が绝大多数（差分箇所は §3-4 に明記）。

> **read_ptr バグ訂正 (2026-10-28, v165 により)**: `v140f2_cascade.py` の `read_ptr()` にセクション定数欠落バグ（`.rdata` +0xE00 等。`raw[va − ImageBase]`）があり、§3-2/§3-3/§4 R3/§5 の cascade 系数値は**バグ由来**と判明。v165 fixed 再計算値へ旧値を「訂正前」として併記した。**訂正済みの正**: P2 候補 **111,305** / P3 候補 **58,989** / P3 未ラベル **40,778** / call-origin **150**（known **147** + unknown **3**）/ 未知帯 **0x700A9A–0x700F06**（3 件・IAT スロット 0xA28090/0xA28168/0xA282B0）/ bucket cat3 entry **0** / 抑制率 **-79.9%**。**判定は不変（v165 で不変を実測済み）**: A41/B15/C8・66 件 60/6・GT ゼロ損失（66/214/64 全段）・P3 本命の結論・P0/P1/P4/P5・coverage 公表値 78.10%（78.13% は v140c バグ版の出力値で公表値ではない）。出典: `tmp/re/v165_v140_recompute.md` §2・§5。

---

## 1. 新規 66 件の確定絞り込み

### 1-1 抽出定義の再現（台帳整合）[確定]

「新規 66 件」を v96 台帳から次の条件で抽出し、**ちょうど 66 件**（class mix: GAP 54 / MID-INSN 12）が得られることを独立確認した。

> `accept=ok` ∧ 実呼出しサイト（good ≥ 1）∧ fall-through ではない ∧ class ∈ {GAP, MID-INSN} ∧ A 節 214 件に非属

- A 節 214 件の再現: pass（実サイト ∧ accept=ok）= **150** / fail = **64**（§2）。fail 理由は 64 件すべて **site-NG のみ**（target 側 accept は全て ok）。
- v96 台帳の行数 565 = 台帳外 E8 ターゲット総数と一致。

### 1-2 判定基準と結果 [確定]

| 判定 | 条件 | 件数 |
|---|---|---|
| **確定 (CONFIRM)** | ターゲット直前が well-formed terminator（`c3` / `c2 iw` / `eb` / `e9` / `ff 25` / `0f 0b` / `iret` / `hlt` の 1–6 B 隣接）∧ accept=ok ∧ 実サイト | **60** |
| **保留 (HOLD-entryshape)** | 直前がパディングラン（CC/00）内で terminator 非隣接。ただし accept=ok ∧ 実サイト ∧ 呼出し側証拠あり | **6** |
| **除外** | — | **0** |

宿主（E8 サイトを包含する Ghidra 台帳関数）との位置関係（v96 台帳の ext 列と独立再計算で一致）:

- **宿主外から call: 51 件** = 直前 terminator 隣接 **45 件**（＝確定）+ パディングラン内 **6 件**（＝保留 6 件と完全一致）
- **宿主内からのみ call: 15 件** — Ghidra が本体へ飲み込んだアウトライン/内部ヘルパー。関数化しない Ghidra の判断は妥当（実関数だが台帳との二重登録は避けるべき）。

### 1-3 個別判定表（66 件全数）

証跡列: cls = v96 台帳 class（GAP = 宿主の first-exit 以降の境界 / MID-INSN = 宿主線形 decode では境界にならない = int3 停止領域、§1-5 参照）。sites(good) = E8 サイト総数(うち宿主線形境界検証通過数)。ext/int = 宿主外/内のサイト数。

| # | アドレス | 判定 | cls | shape（直前） | insns | sites(good) | ext/int | host（Ghidra 名） | プロローグ要約 / 根拠 |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 0x00440400 | 確定 | GAP | adjacent(2B) | 6 | 1(1) | 1/0 | FUN_004402f0 | `mov eax,ecx ; mov [0xb33c28],0 ; mov ecx,[eax+0x34]` |
| 2 | 0x004672D0 | **保留** | MID-INSN | padCC(8)-noterm | 6 | 1(1) | 1/0 | FUN_004672b0 | `mov ax,[esp+4] ; mov edx,[ecx] ; mov [ecx+0xc],ax`。直前 `FF E0`(jmp eax) の 1 B 先 = terminator 隣接だが rule は FF E0 を 2 B terminator として認識せず pad 始点と誤認（§3-4）。R1+adj でも生成される |
| 3 | 0x004F5060 | 確定 | GAP | adjacent(1B) | 31 | 1(1) | 1/0 | FUN_004f5010 | `fldz ; push esi ; mov esi,[esp+8]`（fldz 帯） |
| 4 | 0x004F5230 | 確定 | MID-INSN | adjacent(1B) | 31 | 1(1) | 1/0 | FUN_004f5010 | 同上プロローグ。host 線形 decode は int3 停止 |
| 5 | 0x004F56D0 | 確定 | GAP | adjacent(1B) | 32 | 1(1) | 1/0 | FUN_004f5650 | fldz 帯 |
| 6 | 0x004F5DF0 | 確定 | MID-INSN | adjacent(1B) | 22 | 1(1) | 1/0 | FUN_004f5730 | fldz 帯（max-22 gap の member） |
| 7 | 0x004F65B0 | **保留** | MID-INSN | pad00(1)-noterm | 49 | 1(1) | 1/0 | FUN_004f6400 | `fldz ; push ebx ; push esi`。直前 1 B の 00 ラン + その先は ASCII "leO\0" 系の文字列表らしきバイト列（データ混在領域） |
| 8 | 0x004F7370 | 確定 | GAP | adjacent(1B) | 37 | 1(1) | 1/0 | FUN_004f7330 | fldz 帯。host first-exit と同位置（first_dist=0） |
| 9 | 0x004F7650 | 確定 | MID-INSN | adjacent(1B) | 37 | 1(1) | 1/0 | FUN_004f74b0 | fldz 帯 |
| 10 | 0x004F76B0 | 確定 | MID-INSN | adjacent(1B) | 32 | 1(1) | 1/0 | FUN_004f74b0 | fldz 帯 |
| 11 | 0x004F7750 | **保留** | MID-INSN | padCC(1)-noterm | 32 | 1(1) | 1/0 | FUN_004f74b0 | fldz 帯。直前 1 B の CC + その前は 0x4F77xx 帯ポインタ表 |
| 12 | 0x004F7800 | **保留** | MID-INSN | padCC(8)-noterm | 32 | 1(1) | 1/0 | FUN_004f74b0 | fldz 帯。直前 8 CC + ポインタ表 |
| 13 | 0x004F78B0 | **保留** | MID-INSN | padCC(8)-noterm | 26 | 1(1) | 1/0 | FUN_004f74b0 | `fldz ; push esi ; mov esi,[esp+0x14]`。直前 8 CC + ポインタ表 |
| 14 | 0x004F7C00 | 確定 | MID-INSN | adjacent(1B) | 35 | 1(1) | 1/0 | FUN_004f7af0 | `fldz ; push esi ; mov esi,[esp+0x14]` |
| 15 | 0x004F7EF0 | **保留** | MID-INSN | padCC(12)-noterm | 34 | 1(1) | 1/0 | FUN_004f7af0 | `fld qword [0xa3d360] ; push esi ; …`（fldz ではない唯一の fld 帯）。直前 12 CC |
| 16 | 0x004F8420 | 確定 | GAP | adjacent(1B) | 29 | 1(1) | 1/0 | FUN_004f83d0 | `push esi ; mov esi,[esp+8] ; test esi,esi`（fldz 帯 range 内だが fldz 無し） |
| 17 | 0x005793B0 | 確定 | GAP | adjacent(1B) | 25 | 1(1) | 1/0 | FUN_00579370 | `push 1 ; push 0 ; call 0x582160`。site 0x500E00 は ledger seed 到達 |
| 18 | 0x005797E0 | 確定 | GAP | adjacent(1B) | 40 | 1(1) | 1/0 | FUN_00579770 | 同型 wrapper 群 |
| 19 | 0x0065D620 | 確定 | GAP | adjacent(1B) | 15 | 2(2) | 2/0 | FUN_0065d610 | `push ebx ; mov bl,[esp+8] ; test bl,bl`。2 サイト |
| 20 | 0x007B81A0 | 確定 | GAP | adjacent(1B) | 35 | 1(1) | 1/0 | FUN_007b8150 | `push esi ; push edi ; mov edi,0xb42ec0`。caller 0x50AA31 = **台帳登録済 FUN_0050aa20**（既知の FunctionManager 未登録例の近傍） |
| 21 | 0x00981C6B | 確定 | GAP | adjacent(1B) | 50 | 1(1) | 1/0 | __get_winmajor | CRT。caller = _qsort 帯（0x9877E1） |
| 22 | 0x00981DED | 確定 | GAP | adjacent(1B) | 6 | 1(1) | 1/0 | __exit | `push 1 ; push 0 ; push 0` |
| 23 | 0x00981DFC | 確定 | GAP | adjacent(1B) | 6 | 1(1) | 1/0 | __exit | `push 1 ; push 1 ; push 0` |
| 24 | 0x00981E0B | 確定 | GAP | adjacent(1B) | 25 | 1(1) | 1/0 | __exit | `push esi ; call 0x98bee2 ; mov esi,eax` |
| 25 | 0x0098347E | 確定 | GAP | adjacent(5B) | 9 | 1(1) | 0/1 | _atol | `call 0x98c0f5 ; push [esp+0xc] ; add eax,0x20`（宿主内 helper） |
| 26 | 0x0098349C | 確定 | GAP | adjacent(1B) | 6 | 4(2) | 2/0 | _atol | `push 0 ; push [esp+0xc] ×2`（**strtol 系の実関数**。外部 4 サイト = FUN_00410c40 / FUN_0042f4c0） |
| 27 | 0x00983DAA | 確定 | GAP | adjacent(5B) | 9 | 1(1) | 0/1 | FUN_00983d6e | 宿主内 helper |
| 28 | 0x00983DC3 | 確定 | GAP | adjacent(3B) | 8 | 1(1) | 0/1 | FUN_00983d6e | 宿主内 helper |
| 29 | 0x00983DDB | 確定 | GAP | adjacent(3B) | 8 | 1(1) | 1/0 | FUN_00983d6e | 外部 caller 0x9833AF |
| 30 | 0x0098474B | 確定 | GAP | adjacent(1B) | 17 | 2(2) | 2/0 | __findnext64i32 | CRT find 系。caller FUN_0042f4c0（oblivion 本体コード） |
| 31 | 0x00984C34 | 確定 | GAP | adjacent(1B) | 3 | 1(1) | 1/0 | FUN_00984c1c | **3 命令セッター** `mov eax,[esp+4] ; mov [0xba9de8],eax ; ret`（caller __exit） |
| 32 | 0x009850E7 | 確定 | GAP | adjacent(1B) | 7 | 2(1) | 1/0 | __isxdigit_l | caller FUN_0099576e（0x996712/0x9967EA。1 サイトは site 検証失敗だがもう一方が通過） |
| 33 | 0x00985D92 | 確定 | GAP | adjacent(5B) | 19 | 2(2) | 0/2 | FUN_00985bcd | `fld st(0) ; frndint ; fcomp st(1)`（宿主内 2 呼出し = 数学 helper） |
| 34 | 0x00987637 | 確定 | GAP | adjacent(1B) | 10 | 3(3) | 0/3 | _qsort | `cmp [0xba9e00],1 ; jne … ; call 0x98d758`（宿主内 3 呼出し） |
| 35 | 0x0098765B | 確定 | GAP | adjacent(1B) | 14 | 1(1) | 0/1 | _qsort | **PE MZ 検証** `cmp word [0x400000],0x5A4D ; jne … ; mov eax,[0x40003c] ; cmp [eax+0x400000],0x4550`（V126 の自前イメージ PE 検証 leaf 同定を踏襲・確定） |
| 36 | 0x00988837 | 確定 | GAP | adjacent(1B) | 4 | 1(1) | 1/0 | FUN_0098882d | `push 1 ; call 0x98870a ; pop ecx` |
| 37 | 0x0098AEEF | 確定 | GAP | adjacent(1B) | 10 | 1(1) | 1/0 | ___TypeMatch | RTTI/例外型照合。caller ___FrameUnwindToState |
| 38 | 0x0098B014 | 確定 | GAP | adjacent(1B) | 20 | 1(1) | 1/0 | FUN_0098aff9 | **0xE06D7363 検査** `cmp dword [eax],0xE06D7363 ; jne … ; cmp [eax+0x10],3`（MSVC C++ 例外フィルタ候補の既存同定を踏襲・確定） |
| 39 | 0x0098BF81 | 確定 | GAP | adjacent(1B) | 16 | 2(2) | 2/0 | ___set_flsgetvalue | caller __getptd ×2 |
| 40 | 0x0098C22E | 確定 | GAP | adjacent(1B) | 10 | 1(1) | 1/0 | __getptd | `push edi ; push 0xaa4960 ; call [0xa281a4]` |
| 41 | 0x0098C435 | 確定 | GAP | adjacent(5B) | 5 | 1(1) | 1/0 | _inconsistency | CRT 内部矛盾通知。caller __exit |
| 42 | 0x0098C85D | 確定 | GAP | adjacent(1B) | 24 | 1(1) | 1/0 | __VEC_memzero | caller __getptd |
| 43 | 0x0098CA04 | 確定 | GAP | adjacent(1B) | 8 | 1(1) | 1/0 | __lock | **確保ラッパ** `push 0x140 ; push 0 ; push [0xbaa2ac] ; call [0xa28194]`(HeapAlloc 呼出し IAT)（既存同定を踏襲・確定） |
| 44 | 0x0098D55E | 確定 | GAP | adjacent(1B) | 12 | 1(1) | 1/0 | ___heap_select | caller _qsort 帯 |
| 45 | 0x0098D791 | 確定 | GAP | adjacent(1B) | 3 | 1(1) | 1/0 | __FF_MSGBANNER | **3 命令セッター** `mov eax,[esp+4] ; mov [0xbaa5c4],eax ; ret` |
| 46 | 0x0098D7BD | 確定 | GAP | adjacent(1B) | 17 | 1(1) | 1/0 | __callnewh | caller _qsort 帯 |
| 47 | 0x0098D805 | 確定 | GAP | adjacent(1B) | 14 | 1(1) | 1/0 | __callnewh | caller __get_winmajor |
| 48 | 0x0098DA7E | 確定 | GAP | adjacent(1B) | 6 | 1(1) | 1/0 | __XcptFilter | `mov eax,[esp+4] ; mov [0xbaa5cc],eax ; mov [0xbaa5d0],eax`（2 連セッター） |
| 49 | 0x0098DC88 | 確定 | GAP | adjacent(1B) | 3 | 1(1) | 1/0 | FUN_0098dc4c | **3 命令セッター** `… mov [0xbaa5e0],eax ; ret` |
| 50 | 0x0098DDEC | 確定 | GAP | adjacent(2B) | 3 | 1(1) | 1/0 | ___getlocaleinfo | **3 命令セッター** `… mov [0xbaa5ec],eax ; ret` |
| 51 | 0x0098DDF6 | 確定 | GAP | adjacent(1B) | 3 | 1(1) | 1/0 | ___getlocaleinfo | **3 命令セッター** `… mov [0xbaa5f0],eax ; ret` |
| 52 | 0x0098EEB9 | 確定 | GAP | adjacent(1B) | 22 | 1(1) | 1/0 | __filbuf | **SEH 付き** `push 0x54 ; push 0xafff08 ; call 0x98c5a0 ; xor edi,edi ; mov [ebp-4],edi`（既存同定を踏襲・確定） |
| 53 | 0x00991501 | 確定 | MID-INSN | adjacent(1B) | 8 | 1(1) | 1/0 | FUN_00990d03 | host 線形 decode は int3 停止（§70.16 (4b) の int3-block 例）。caller FUN_00993f39 |
| 54 | 0x009917CE | 確定 | GAP | adjacent(1B) | 60 | 2(2) | 0/2 | FUN_0099171c | `push eax ; push ebx ; push ecx`（宿主内 2 呼出し） |
| 55 | 0x009919D4 | 確定 | GAP | adjacent(1B) | 12 | 1(1) | 0/1 | FUN_0099171c | 宿主内 helper |
| 56 | 0x00993B2C | 確定 | GAP | adjacent(1B) | 18 | 2(2) | 0/2 | FUN_0099358e | `fabs ; fld st(0) ; fld st(0)`（宿主内 FPU helper） |
| 57 | 0x00994BDE | 確定 | GAP | adjacent(1B) | 24 | 1(1) | 0/1 | FUN_00993f39 | 宿主内 FPU helper |
| 58 | 0x00994C21 | 確定 | GAP | adjacent(1B) | 18 | 2(2) | 0/2 | FUN_00993f39 | `fld st(0) ; frndint ; fcomp st(1)` |
| 59 | 0x00995CA4 | 確定 | GAP | adjacent(5B) | 16 | 6(6) | 0/6 | FUN_0099576e | `mov eax,[esi] ; cmp [esp+4],eax ; jne …`（宿主内 6 呼出し） |
| 60 | 0x00995CF8 | 確定 | GAP | adjacent(1B) | 7 | 19(16) | 0/19 | FUN_0099576e | `dec [edx+4] ; js … ; mov ecx,[edx]`（宿主内 19 呼出し・最多） |
| 61 | 0x00995D0E | 確定 | GAP | adjacent(1B) | 16 | 2(2) | 0/2 | FUN_0099576e | 宿主内 helper |
| 62 | 0x00997725 | 確定 | GAP | adjacent(1B) | 32 | 1(1) | 1/0 | __mbsbtype_l | CRT |
| 63 | 0x00997782 | 確定 | GAP | adjacent(1B) | 13 | 1(1) | 1/0 | __mbsbtype_l | CRT |
| 64 | 0x00997AAE | 確定 | GAP | adjacent(1B) | 20 | 1(1) | 1/0 | __setargv | CRT |
| 65 | 0x0099C8B1 | 確定 | GAP | adjacent(1B) | 44 | 1(1) | 1/0 | _ValidateRead | `push 0x10 ; push 0xb00088 ; call 0x98c5a0`（SEH プロローグ） |
| 66 | 0x0099C947 | 確定 | GAP | adjacent(1B) | 4 | 1(1) | 0/1 | _ValidateRead | `push 1 ; call 0x98c8fb ; pop ecx`（宿主内） |

### 1-4 サブグループの個別判定（タスク指定 6 群）[確定]

**(a) `fldz ; push esi ; mov esi,[esp+8]` 帯（0x004F5060–0x004F8420）** — 帯内に 66 件中 **14 件**。うちバイト列 `D9 EE`(fldz) 始まりは **12 件**（報告 §70.16 (4b) の「同一プロローグ 12 件」はこれと一致。厳密にはプロローグ変種あり: `[esp+8]` 型 9 件 / `[esp+0x14]` 型 2 件（0x4F78B0, 0x4F7C00）/ `fldz;push ebx;push esi` 1 件（0x4F65B0）。残り 2 件 = 0x4F7EF0（`fld qword [0xa3d360]` 始まり・保留）/ 0x4F8420（`push esi` 始まり・確定）。判定: 9 確定 + 5 保留（保留 5 = 0x4F65B0/0x4F7750/0x4F7800/0x4F78B0/0x4F7EF0）。同一プロローグ反復 + 外部 caller（FUN_0050xxxx 系 getter 呼出し群）+ 16 B 揃え + accept=ok で**実関数族として確定**（Ghidra は 0x4F5730 / 0x4F74B0 等へ 5–9 個ずつ飲み込み）。

**(b) 0x0098/0x0099 帯 3 命令セッター** — `0x00984C34` / `0x0098D791` / `0x0098DC88` / `0x0098DDEC` / `0x0098DDF6` の **5 件すべて 66 件に含まれ全件確定**。全て `mov eax,[esp+4] ; mov [0xBA9DExx/0xBAA5xx],eax ; ret` の同一型（グローバル変数セッター）。加えて `0x0098DA7E`（__XcptFilter 内、2 連セッター 6 命令）も確定。

**(c) 0x0098765B（PE MZ 検証）** — 66 件に含まれ確定。`cmp word [0x400000],0x5A4D → [0x40003c] = e_lfanew → cmp [eax+0x400000],0x4550('PE') → cmp word [eax+0x400018],0x10B (PE32)`。V126（`v126_bind_crt_bootstrap.md`）の自前イメージ PE 検証 leaf 同定を**踏襲**。host=_qsort 内（宿主内 1 呼出し、caller 0x987761）。

**(d) 0x0098B014（0xE06D7363 検査）** — 66 件に含まれ確定。`mov eax,[eax] ; cmp dword [eax],0xE06D7363 ; jne … ; cmp dword [eax+0x10],3` = MSVC C++ 例外ディスパッチの例外コード/型チェック。CallCatchBlock（0x98B227）からの 1 呼出し。既存「C++ 例外フィルタ候補」同定を**踏襲**。

**(e) 0x0098CA04（確保ラッパ）** — 66 件に含まれ確定。`push 0x140(320B) ; push 0 ; push [0xBAA2AC](hHeap) ; call dword [0xA28194](IAT HeapAlloc) ; test eax,eax ; mov [0xBAABC8],eax`。__lock 直後の first-exit 隣接、caller ___heap_select(0x98D595)。

**(f) 0x0098EEB9（SEH）** — 66 件に含まれ確定。`push 0x54 ; push 0xAFFF08 ; call 0x98C5A0 ; xor edi,edi ; mov [ebp-4],edi ; lea eax,[ebp-0x64]` = SEH スコープ登録付き CRT 内部ルーチン。__filbuf の first-exit 隣接、caller _qsort 帯（0x987798）。

### 1-5 MID-INSN 12 件と int3-block [確定]

66 件中 class=MID-INSN は **12 件**: `0x4672D0, 0x4F5230, 0x4F5DF0, 0x4F65B0, 0x4F7650, 0x4F76B0, 0x4F7750, 0x4F7800, 0x4F78B0, 0x4F7C00, 0x4F7EF0, 0x991501`。これらは宿主関数の開始点からの線形 decode が宿主本体内の **int3 で停止**するため「宿主の命令境界」にならない（例: host 0x4F5010 / 0x990D03）。ただしターゲット直前 byte は well-formed terminator（6 件は pad ラン経由＝保留）で、ターゲット自身の前方 decode は accept=ok。§70.16 (4b) の結論「int3-block はデータの証拠にならず、直前 terminator で実在エントリと再分類」を**独立再検証で支持**（MID-INSN 12 件すべて該当）。

### 1-6 Cat-3 台帳突合 [確定]

- 66 件すべて（**66/66**）が v127 Cat-3 リージョン（`lo`–`hi` 吞み込み領域）内。うち **40 件は Cat-3 エントリ開始点そのもの**、26 件は他のエントリの領域内（内訳: E:orphan 10 / A:extref 9 / C:v86-overlap 3 / F:noncode 2 / D:imm32-only 2）。
- **台帳矛盾なし**: 66 件はいずれも func_list 21,597 に開始点として存在しない（台帳外）。「0x98/0x99 帯・0x4F 帯は台帳が 449/226/218 件充実しているのに未ラベル」= 真のギャップという §70.16 (4b) の評価を維持。
- v127 の Cat-3a 採用数 10,261 と `v127_cat3_regions.json` の 16,660（cls: C:v86-overlap 10,047 / F:noncode 4,860 / E:orphan 987 / B:ledger-nohdr 337 / D:imm32-only 319 / A:extref 110）は**階層の違い**（JSON = 採用前スキャン、10,261 = 5 条件フィルタ後の採用）。v140 は JSON を保守的超集合として領域判定に使用。エントリ開始点ベースの判定（40/214/64 件）は両者で不変。

---

## 2. A 節 214 件のうち「実サイト + accept=ok」を通らない 64 件の切り分け

### 2-1 失敗モード [確定]

- pass 150 / fail 64。**64 件の失敗理由は 100%「site-NG のみ」**（target 側 accept=ok、宿主線形境界上の E8 サイト = good 0）。
- v96 のサイト検証は「E8 サイトが**宿主台帳関数の開始点からの線形 decode** で命令境界上にあること」を要求する。64 件の 186 サイトすべてがこれを不通（linear site check: **186/186 blocked**）。停止理由は int3（int3@0x9A9D4A: 55 件 / int3@0x990D0A: 46 件 / int3@0x40DF74: 24 件 等）、サイト-停止距離は中央 1,397 B / 最大 188,074 B。

### 2-2 CFG 到達性による実在証拠（独立再実装）[確定]

ledger 21,591 開始点を種に CFG walk（fall-through / jmp / cond-jmp / call を追跡、decode 1,635,166 命令・カバレッジ 78.10%）:

| 種 | 186 サイトの状態 |
|---|---|
| ledger のみ | **start 17 / unreached 169 / mid-instruction(inside) 0** |
| +66 件 +214 件を追加 | **start 47 / unreached 139 / inside 0** |

- **「カバレージ済みコードの中間命令」= データ由来の E8 は 0/186**。仮に E8 バイト列がデータ（imm32 / jump table 等）なら inside か unreached だが、inside はゼロ。
- unreached 139 サイトすべてが**前方 decode accept=ok**（= サイトから始まる配列は必ず ret/jmp に到達するコード列）。文脈もコード様（例 `CC CC CC CC 8B 45 F0 50 E8…` = パディング→`mov eax,[ebp-0x10]; push eax; call`。直前 byte が `50`(push eax) のサイトが 58/139）。
- 186/186 サイトが Cat-3 領域内、64/64 ターゲットも領域内。`docs/Oblivion_decompiled.c` 本文に 64 件のアドレス言及は **0 件**（本検索。v140b の旧検索は 1 件。いずれにせよヘッダ/ラベルは皆無）。

### 2-3 切り分け結果 [確定]

| グループ | 件数 | 定義 | 解釈 |
|---|---|---|---|
| **A: サイト検証が厳しすぎる** | **41** | ≥1 サイトが CFG walk で命令開始点として実在証明（L1） | 呼出し元関数自体が未ラベル（吞み込み）であり、宿主線形 decode 前提のサイト検証が原理的に不通。**E8 は実サイト** |
| **B: 後方 decode で実在寄り** | **15** | L1 なし・≥1 サイトで「その手前 8–15 B から直進 decode がちょうどサイトに着地」（L2、文脈は push 引数等のコード様） | 呼出し元が 0x9D/0x9B/0x8BC 帯の未ラベル領域に集中。実サイトと判断するが形式証明は未達 |
| **C: 保留（反証なし）** | **8** | L1/L2 なし（後方 window < 8 B）・ただし前方 accept=ok | `0x410D10 0x533D60 0x533D80 0x889570 0x88D520 0x89E2E0 0x8A01D0 0x8A2D00`。文脈は同様にコード様（`CC×4 8B 45 xx 50` 型）でデータ由来の兆候なし。呼出し元復元待ち（§4 R1） |

**結論: 64 件は「データ由来の E8」ではなく「サイト検証（宿主線形 decode 前提）の厳しさ」に起因**。実サイト率は A+B = 56/64 で形式証明済み、残り 8 件も反証ゼロ。A 節 214 件はいずれも実関数（v92 triage で 214/214 が次開始点前に終端）である previous 結論と矛盾しない。

補足: A 節 64 件のターゲット群は Cat-3 エントリ開始点 64/64。v140_step4 の旧切り分け（A=41/B=1/C=22）は構造証拠のみの暫定版であり、本節の A=41/B=15/C=8（後方 decode 証拠を反映）が確定版。

### 2-4 64 件個別表

| ターゲット | G | n_sites | サイト証拠（walk3 状態 / bw(k,cnt)） | host |
|---|---|---|---|---|
| 0x004040A0 | A | 1 | 0x40C18A:start / bw10,5 | FUN_00404010 |
| 0x00404940 | A | 2 | 0x40E227:start, 0x40E381:unreached | FUN_004048b0 |
| 0x004052F0 | A | 1 | 0x40E865:start | FUN_00405090 |
| 0x0040C1B0 | A | 1 | 0x40E88E:start | FUN_00407ae0 |
| 0x0040FE00 | A | 1 | 0x410E8D:start | FUN_0040fdd0 |
| 0x00410310 | A | 1 | 0x40EADB:start | FUN_004102c0 |
| 0x00410840 | A | 1 | 0x410DE6:start | FUN_00410810 |
| 0x00410B80 | A | 1 | 0x40F4A5:start | FUN_00410b00 |
| 0x00410D10 | C | 1 | 0x410E25:unreached / bw5,2 | FUN_00410c40 |
| 0x00410E40 | A | 1 | 0x40EAEA:start | FUN_00410c40 |
| 0x0041DF70 | B | 26 | 全 unreached / bw14,8（`01 5B C2 04 00 53 8B CF` 型文脈） | FUN_0041df60 |
| 0x0042B2D0 | B | 1 | 0x428F41:unreached / bw15,5 | FUN_0042b2a0 |
| 0x0044CF80 | A | 1 | 0x445E8A:start | FUN_0044cdf0 |
| 0x004523F0 | A | 1 | 0x66337A:start | FUN_004523a0 |
| 0x0047D640 | A | 1 | 0x47E195:start | FUN_0047d5b0 |
| 0x0047E190 | A | 4 | 4/4 start | FUN_0047e140 |
| 0x00497270 | B | 1 | 0x4295AA:unreached / bw15,6 | FUN_00497240 |
| 0x00497B50 | A | 1 | 0x40F5CA:start | FUN_00497b20 |
| 0x004A2BA0 | A | 2 | 2/2 start | FUN_004a2a90 |
| 0x004CB730 | A | 2 | 2/2 start | FUN_004cb6c0 |
| 0x004F5ED0 | A | 1 | 0x4F69D8:start | FUN_004f5ea0 |
| 0x004F5F70 | A | 1 | 0x4F6A28:start | FUN_004f5ea0 |
| 0x0051C490 | B | 1 | 0x66642E:unreached / bw15,4 | FUN_0051c3b0 |
| 0x00529BB0 | B | 1 | 0x6662FD:unreached / bw15,5 | FUN_00529b70 |
| 0x00533D60 | C | 1 | 0x9B91F7:unreached / bw7,2 | FUN_00533d30 |
| 0x00533D80 | C | 1 | 0x9B9144:unreached / bw4,2 | FUN_00533d30 |
| 0x0053AC60 | A | 1 | 0x40E759:start | FUN_0053a720 |
| 0x0053AD60 | A | 1 | 0x40F5A3:start | FUN_0053a720 |
| 0x0053FB30 | A | 1 | 0x40F4E9:start | FUN_0053f4c0 |
| 0x005535D0 | A | 1 | 0x40F541:start | FUN_00553550 |
| 0x00572DC0 | A | 1 | 0x40E844:start | FUN_00572d90 |
| 0x0057BDE0 | A | 2 | 0x4F57D9/0x4F57F9:start（max-22 gap 内から呼出し） | FUN_0057bdb0 |
| 0x005B5C90 | A | 1 | 0x40F0C3:start | FUN_005b5b70 |
| 0x005E8870 | A | 1 | 0x4F5A29:start | FUN_005e83a0 |
| 0x006078C0 | B | 14 | 全 unreached / bw12–15 | FUN_00607880 |
| 0x0065DAC0 | A | 1 | 0x4F560F:start | FUN_0065da50 |
| 0x00662EE0 | A | 1 | 0x40EB6D:start | FUN_00662da0 |
| 0x006632A0 | A | 1 | 0x40F050:start | FUN_00663070 |
| 0x00682430 | A | 1 | 0x40F4AF:start | FUN_00681df0 |
| 0x00684710 | A | 1 | 0x40F4B9:start | FUN_00684530 |
| 0x006F96B0 | A | 1 | 0x40F4FE:start | FUN_006f95e0 |
| 0x006F98F0 | A | 1 | 0x40E636:start | FUN_006f98d0 |
| 0x00701A80 | A | 1 | 0x40F495:start | FUN_00701a30 |
| 0x007362C0 | B | 1 | 0x736769:unreached / bw15,8 | FUN_00735cd0 |
| 0x007362E0 | B | 1 | 0x7367C7:unreached / bw15,4 | FUN_00735cd0 |
| 0x00747BA0 | A | 1 | 0x40E62A:start | FUN_00747930 |
| 0x00747BE0 | A | 1 | 0x40F618:start | FUN_00747930 |
| 0x00748490 | A | 1 | 0x748A00:start | FUN_00748410 |
| 0x007489F0 | A | 1 | 0x747BA0:start（直前 gap 内候補を起点とする site） | FUN_007489b0 |
| 0x007B4870 | A | 1 | 0x40E8ED:start | FUN_007b4830 |
| 0x007B8530 | A | 1 | 0x497B56:start（既知の真 FunctionManager 未登録例） | FUN_007b84e0 |
| 0x00889570 | C | 2 | 全 unreached / bw4,2 | FUN_00889470 |
| 0x008896D0 | B | 2 | unreached / bw12–15 | FUN_008896a0 |
| 0x0088D520 | C | 3 | 全 unreached / bw4,2 | FUN_0088d4e0 |
| 0x008903B0 | B | 7 | unreached / bw15 | FUN_00890390 |
| 0x0089E2E0 | C | 4 | 全 unreached / bw4,2 | FUN_0089e210 |
| 0x008A01D0 | C | 5 | 全 unreached / bw4,2 | FUN_008a0180 |
| 0x008A2D00 | C | 1 | unreached / bw4,2 | FUN_008a2c70 |
| 0x008A9900 | B | 1 | 0x5131B5:unreached / bw15,7（`D9 44 24 0C 51 D9 1C 24` 型 FPU 文脈） | FUN_008a98d0 |
| 0x008B66A0 | B | 2 | unreached / bw14,4 | FUN_008b6650 |
| 0x008C9290 | B | 13 | unreached / bw4–15 | FUN_008c9070 |
| 0x0090C460 | B | 1 | 0x8BC4A5:unreached / bw14,5 | FUN_0090c290 |
| 0x0090D210 | B | 3 | unreached / bw15 | FUN_0090d200 |
| 0x00990D10 | A | 46 | ≥1 start（うち多数 unreached / bw15） | FUN_00990d03 |

---

## 3. 同一 gap に複数候補が並ぶ構造と adjacent 拡張の偽陽性抑制

### 3-1 gap 構造の実測と解釈 [確定]

- A 節 214 件は **132 個の gap**（[prev 開始点, next 開始点)）に分布。**≥2 件の gap = 33**、≥3 件 = 11、**最大 22 件 = gap [0x004F5730, 0x004F5EA0)（span 1,904 B）**。multi-gap の member 合計 = 115。新規 66 件は 46 gap（≥2 は 14、最大 5 件）。
- multi-gap の帯別: **0x4F0xxx 帯 17 / その他 16**。上位: [0x4F5730,0x4F5EA0)=22, [0x4F5010,0x4F54A0)=9, [0x4F45D0,0x4F48F0)=9, [0x4F8590,0x4F8840)=6, [0x4F67E0,0x4F6AB0)=5, [0x556EC0,0x556FE0)=4, [0x5575C0,0x557740)=3, [0x4F8B90,0x4F8E70)=3, [0x4F5EA0,0x4F6060)=3, [0x4F4E20,0x4F5010)=3, [0x8897A0,0x889A70)=2。
- **解釈**: これは「1 つの台帳関数が同型小関数のクラスタごと飲み込んだ」構造。0x4F 帯 multi-gap 17 個の member は 16 B 揃え + padCC + 同一系列のプロローグ（`fldz;push esi;…` getter 群、caller は FUN_0050xxxx 系の集中呼出し元）で、Ghidra がクラスタ先頭（例 0x4F5730）のみを関数化し 22 個を first-exit 後の gap へ落とした。0x556/0x557/0x889 帯は同型（padCC + 16B 揃え）。v92 triage の shape mix は 214/214 が padCC、align16 が 214/214 — 「CC-pad 規則が gap 内 pad-run 終端を生成する」構造と「MSVC の 16 B 関数配置」が multi-gap を作る。**1 gap = 1 関数ではない**ことの実証であり、Ghidra 関数境界と実関数境界の乖離が最大 22 個/gap に及ぶ。

### 3-2 adjacent 拡張（偽陽性 203,213 件）の抑制 [確定]

v95b の `+adjacent` は再現率 89.8%→97.3% の一方で未ラベル候補が 13,798→**203,213** に膨張。これを段階フィルタで絞る cascade を独立再実装（origin 付き CFG walk。seed=台帳 21,591、origin: seed=1 / call=2 / flow(fall-through・jmp・cond-jmp)=3）:

| 段階 | 候補位置 | 台帳再現率 | 未ラベル残 | ground truth 生存（66 / 214 / 64 / Cat-3 16,660） |
|---|---|---|---|---|
| P0 R1+adjacent（境界位置のみ） | 234,974 | 97.4% (21,034) | 213,940 | 61 / 214 / 64 / 14,867 |
| P1 +accept（≥3 命令で ret/jmp ≤600 B） | 156,933 | 84.3% (18,211) | 138,722 | 61 / 214 / 64 / 10,846 |
| P2 +F1（カバレージ済みの中間命令位置の拒否） | **111,305**（訂正前 111,276） | 84.3% | **93,094**（訂正前 93,065） | 61 / 214 / 64 / 10,845 |
| P3 +F2（flow 到達ブロック先頭の拒否。origin=flow のみ拒否、seed/call は保護） | **58,989**（訂正前 58,935） | **84.3%** | **40,778**（訂正前 40,724） | **61 / 214 / 64 / 10,828**（訂正前 Cat-3 10,812） |
| P4 +E8 証拠 | 12,789 | 57.9% | 289 | 61 / 214 / 64 / 256 |
| P5 +16 B 揃え | 12,243 | 55.6% | 235 | **15** / 214 / 64 / 230 |

読み方と推奨:

- **F1+F2（P3）が本命の抑制**。203,213 → **40,778（-79.9%）**〔v165 訂正。訂正前 203,213 → 40,724（-80.0%）〕を ground truth（66/214/64）ゼロ損失・台帳再現率ゼロ損失で達成。F2 は「fall-through/jmp エッジで到達した位置」のみを拒否する origin ベース実装に限れば台帳開始点を誤殺しない（再現率 84.3% は P1 の accept フィルタのコストであり F1/F2 のコストは 0）。
- **E8 証拠（P4）は hard filter にしない**（再現率 84.3%→57.9% と大きく落ちる）。優先度タグとして用いる。P4 未ラベル 289 件のうち 256 件が Cat-3 エントリ = E8 台帳と Cat-3 台帳の相互補完を確認。
- **16 B 揃え（P5）は hard filter にしない**: 66 件のうち 46 件（CRT 帯の unaligned 実エントリ全滅）を殺す。台帳側でも 17,673/18,211 = 97.0% のみ 16 B 揃え。
- **6 件の保留（pad-noterm）は R1+adjacent でも生成されない**（生成 61/66。非生成 5 = 0x4F65B0/0x4F7750/0x4F7800/0x4F78B0/0x4F7EF0。※0x4672D0 は `FF E0`(jmp eax) の第 2 バイト E0 を 2 B terminator と誤認するバイトパターン経由で P0 に生成される = 意図しない偶然的正解）。→ **境界規則系は无论如何に E8 直接列挙（v96）を代替できない**ことを 66 件が示す。
- P3 残余 **40,778** 件の origin は unreached **40,628** / **call 150**〔v165 訂正。訂正前 40,724 件 / unreached 40,502 / call 222〕。call-origin 150 件は「台帳コードから call エッジで到達した未ラベル位置」= 最有望な新規 lead 群。内訳: 既知（66/214 系）**147** / Cat-3 エントリ **0** / **未知 3 件**（**0x700A9A–0x700F06** 帯・IAT スロット 0xA28090/0xA28168/0xA282B0）〔v165 訂正。訂正前 Cat-3 エントリ 53 / 未知 22 件（0x9E27B0–0x9E4200 帯、E8 サイト 0 = 間接 call 由来と推定）〕。0x9E27xx 帯の真の正体は `.CRT$XCU` 登録配列（v162 R-l）で、旧「未知 22 件」は read_ptr バグ由来の誤帰属。

### 3-3 台帳・報告との整合 [確定]

- 本 cascade の P0–P5 数値は前回中断分の中間出力（v140e）と一致〔**v165 訂正（2026-10-28）**: P2 の差 111,276↔111,305 の正体は **read_ptr バグ由来**（v140f2_cascade の `raw[va − ImageBase]` 誤換算）。v140f 初版の誤りは **F2 のみ**（P3=84,499）。origin-F2 + read_ptr 修正の真値 = **P2 111,305 / P3 58,989**（v165 実測・v162 p2fix と完全一致）。旧帰属「P2 のみ v140f 初版で F2 に jmp 対象を含めた誤実装により 111,305/84,499 となったが、origin ベースへ修正して 111,276/58,935 を再現」は**訂正前**として記録〕。jmp 対象への拒否は tail-call jmp 先の実関数を誤殺するため不採用が正しい。
- §70.16 (2)(4b) の記述（214/214 E8 検出、fall-through 0 件、66 = 51 外部 + 15 内部、45/6 split、int3-block 12 件、Cat-3 帯の台帳充実）はすべて本節の独立再検証と一致。**報告書改訂は不要**（docs 編集は本タスクの禁止事項のまま）。

---

## 4. 新規残点（V143+ 粒度）

| ID | 残点 | 優先度 | 内容 |
|---|---|---|---|
| R1 | **V143: C 群 8 件の呼出し元復元** | 中 | `0x410D10 0x533D60 0x533D80 0x889570 0x88D520 0x89E2E0 0x8A01D0 0x8A2D00` の site（0x9B91xx/0x9D5Dxx–0x9D74xx/0x410E25 帯）を含む swallowed caller 関数の入口推定。`CC×4 8B 45 xx 50 E8` 文脈が支配的で、caller 入口は site の数 B 手前–数百 B 手前にあると推定。caller 復元により A 節 64 件全数が L1 相当に |
| R2 | **V144: 保留 6 件の最終裁定** | 中 | 0x4672D0 / 0x4F65B0 / 0x4F7750 / 0x4F7800 / 0x4F78B0 / 0x4F7EF0。パディングラン内配置（直前 32 B に 0x4F 帯ポインタ表・ASCII 列混在）。Ghidra への手動関数化（診断目的）か実行時トレースで確定 |
| R3 | **V145: P3 残余の call-origin 150 件の消化**〔v165 訂正。訂正前 call-origin 222 件〕 | 高 | うち未知 **3 件（0x700A9A 帯・IAT スロット 0xA28090/0xA28168/0xA282B0）**〔v165 訂正。訂正前 未知 22 件（0x9E27B0 帯、E8 サイト 0 = 間接 call 由来）〕。0x9E27xx 帯は `.CRT$XCU` 登録配列（v162 R-l）＝バグ由来の誤帰属で対象消滅。imm32 ポインタ表（IAT/vtable）参照との突合で caller を特定するパイプライン |
| R4 | **V146: terminator バイトパターンの FF E0-E7 欠落修正** | 低 | v96/v140 系の `term_end()` は 2 B 系で `EB/E0-E7/CF/F4` を「第 1 バイト」として検査するため `FF E0`(jmp reg) の第 2 バイトを terminator 先頭と誤認（0x4672D0 の生成がこれに乗っかる）。`FF E0-E7` / `FF D0-D7`(call reg だが terminator 前提の外) の正規検出を追加。影響は限定的（今回の判定結果は不変） |
| R5 | **V147: Cat-3 台帳の三層整理** | 低 | `v127_cat3_regions.json` 16,660 / accepted 11,463 / published Cat-3a 10,261 の差分（accepted−published = 約 1,202 = 距離≥16B・偽境界・領域統合フィルタ由来）を文書化し、以降の突合で数字の食い違いをなくす |
| R6 | **V148: accept フィルタの再現率コスト回収** | 中 | P1 で 2,823 台帳開始点が失われる（tiny thunk / 短関数）。mininsns=3・600 B 以内の閾値を「first-exit が int3 で即停止する関数」等に向けた代替判定（例: 逆方向 padCC+terminator との複合）で回収検討 |

---

## 5. 検証証跡（Git 管理外・本タスク新規）

| ファイル | 内容 |
|---|---|
| `%TEMP%\opencode\v140f_final.py` → `v140f_out.txt` | 66 件抽出・個別判定・named 確認・64 件 linear site check・CFG walk（17/47 start・inside 0）・Cat-3 突合・gap 構造・cascade 初版 |
| `%TEMP%\opencode\v140f2_cascade.py` → `v140f2_out.txt` | origin ベース F2 で P0–P5 を v140e と完全一致に再現。**read_ptr バグあり・§3-2 の値はバグ由来。訂正は v165_cascade.py（fixed）**〔v165 訂正 2026-10-28〕 |
| `%TEMP%\opencode\v140f3_64.py` → `v140f3_out.txt` | 64 件 per-target 表（walk 状態 + 修正後 backward-window + A/B/C 分類 A=41/B=15/C=8） |
| `%TEMP%\opencode\v140f4_six.py` / `v140f5_why.py` | 保留 6 件の P0 生成経路究明（0x4672D0 の FF E0 誤パターン経由を特定） |
| `%TEMP%\opencode\v140f6_classes.py` | 66 件の v96 class（GAP 54 / MID-INSN 12）内訳抽出 |
| （参照）前回中断分の中間出力 | `%TEMP%\opencode\v140_step1..6.txt` / `v140b_out.txt` / `v140c_out.txt` / `v140d_out.txt` / `v140e_out.txt`（本タスクの独立再検証とは別実装で一致） |

（本節の新規確定）: 新規 66 件は **確定 60 / 保留 6 / 除外 0**（宿主外 51 = 終端隣接 45 + pad 内 6、宿主内 15）。A 節 64 件は**データ由来 E8 が 0 件**、サイト検証の厳しさ起因で **A(実在証明) 41 / B(後方証拠) 15 / C(保留) 8** に切り分け。multi-gap 構造は「台帳関数が同型小関数クラスタを最大 22 個飲み込む」構造と同定。adjacent 拡張の推奨抑制構成は **P3（accept + F1 + F2、偽陽性 203,213→40,778〔v165 訂正。訂正前 40,724〕、ground truth ゼロ損失）**、E8 証拠は優先度タグ、16 B 揃えは hard filter 不採用。
