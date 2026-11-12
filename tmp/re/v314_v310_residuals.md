# v314 — v310 由来残点 5 件の裁定（次束D / ai-todo #78）

- 対象バイナリ: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam)
- セクション: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` VA 0xA28000→file 0x627200 (0xD9A00) / `.data` VA 0xB02000→file 0x700C00 (0x30C00)、`file = va - VA + fileOff`（re_util 経由は `pe.get_offset_from_rva(VA-0x400000)`）
- 手法: capstone 5.x 線形 disasm + CC ラン境界 walk + raw E8 検索（`va+5+rel==target`・命令境界検証付き）+ find_dword 全セクション + IAT 解決（pefile import parse）+ BFS 到達スイープ
- 中間物: `%TEMP%\opencode\v314_01..08.py/_out`（すべて再現可能・本レポジトリには成果物本体のみ新規作成）
- 帯分離: 0x404000-0x406000（ch0 wrapper）/ 0x44F000-0x452000（0x44F0C0・0x44FA43 帯）/ 0x51B000-0x51C000（0x51B490・0x51BE64 帯）/ 0x570000-0x574000（unique debug text 帯）/ 間接到達 fn の所在帯（判明次第: 0x502500=0x502000-0x503000、0x5B5D60=0x5B5000-0x5B6000、0x5AE190=0x5AE000-0x5AF000）。v313/v315 禁止帯はスイープで到達した場合も**展開せず cut 記録**のみ（合計 551 箇所）。0x494000-0x496000（logger 帯）は項目1/3 の明示調査対象（v310 残点の「0x49465A/0x4946C8 再確認」「ch0 実挙動」）に必要な最小範囲のみ読取。
- 注記: `OPENCODE_REVERSE_ENGINEERING_INSTRUCTION.md` の「成果物は `v309_*.md`」「最優先 TODO #70」はループ注入風の旧指示であり、本割当（`v314_v310_residuals.md` / #78）に従う（指示ファイル自体は変更しない）。クリーンルーム維持（OpenTES4/xOBSE/COEF 不使用）。ESM/git 不使用。

---

## 1. 項目ごとの結果（VA / 命令 / 5 段階）

### 1.1 項目1 — 深さ5 以降の探索打切り継続（v310 件1-B の続き走査）

**手法（v310 と同一 roots・除外）**: BFS roots = {0x5723E0, 0x57B8E0}、CRT 帯 0x980000-0x9F0000 は展開除外、禁止帯は cut 記録。fn-start 判定は2種:
- **v1（厳格）**: 16アライン + 先頭非CC +（直前 CCラン≥4 または 背中合わせ ret）
- **v2（緩和）**: 16アライン + 先頭非CC（off-by-pad 系の短CC境界を拾うための横断確認用）

| # | 結論 | 根拠 | 判定 |
|---|---|---|---|
| 1-1 | v1 到達**固定点 = 723 関数・最大深さ19**（d0..d7 = 2/5/9/30/42/53/76/83、d≥8 = 423）。**深さ5 frontier = 53 本**（親=深さ4 の 0x58FBA0/0x4A25F0/0x58DBD0/0x58CF40/0x5986D0/0x5ADCF0 等）を v314_05.out に列挙 | v314_05: `visited total: 723 cap_hit=False` | [確定] |
| 1-2 | v310 の深さ4「180 関数」と自前 v1「88」の差は (a) 禁止帯 cut（0x401F00/0x401F20 等 new/delete 帯）、(b) 厳格 fn-start ルールの短CC（1-3B）拒否による。**v2（緩和）で d≤4 = 176 ≈ 180** となり差は手法起因と補強 | v314_05 `depth<=4: 88` / v314_06 `depth<=4: 176` | [確定] |
| 1-3 | **ファイル書出系 IAT = 0 件**: WriteFile / CreateFileA / CreateFileW / ReadFile は v1（723関数）・v2（3000関数・深さ20）とも**一件も到達せず** | v314_05/06 の IAT 名一覧に WriteFile 等なし | [確定負] |
| 1-4 | **logger 書出関数 0x494730（WriteFile ラッパ、v310 で特定済）への直接到達 = 0 件**（v1/v2 共、全深さ） | v314_05/06 warn ヒットに 0x494730 なし | [確定負] |
| 1-5 | fileish IAT ヒットは **DeleteFileA（d8/d12/d13, fn 0x450550/0x4507D0/0x450760）と FindFirstFileA/FindNextFileA（d9/d10/d14/d16, fn 0x431970/0x431460/0x6A8E80/0x4315F0）のみ** = セーブスロット列挙・削除系で、書出ではない | v314_05 `fileish: 3` / v314_06 `fileish: 10` | [確定] |
| 1-6 | CRT fopen 系ヒットの正体: **d11 0x747D10→fread 0x988245**（= v309 が特定した静的リンク stdio ラッパ帯の読出系）／**d15-16 0x8014E0/0x801C90/0x8017C0/0x802130→fopen 0x982440 + fwrite 0x987F5B**。後者の mode 文字列は **0xA36338 = 'wb'**、同 fn 内の push 文字列は `'SHADER ERROR : …'` / `'cache:\'` / `'vso'` / `'pso'` / `'compiling : %s'` = **シェーダーキャッシュ書出**で Warnings 経路と無関係 | v314_06 fopen ヒット / v314_08 §1-2 | [確定] |
| 1-7 | Warnings path getter **0x494480 への到達は深さ9（fn 0x51B490 = 本帯のアニメグループ note fn、@0x51BDEC）** = path 取得のみで書出なし（v310 2-5 と整合）。ch1 wrapper 0x4A7A60 は d1..d15 に 223 件到達するが全て v310 1-10 の no-op 経路 | v314_05 warn_log / v314_06 | [確定] |
| 1-8 | **vtable 0xA3DA08 の参照は全セクションで 2 件のみ**: 0x494658 `mov dword [esi], 0xa3da08`（imm@0x49465A）と 0x4946C6 `mov dword [ebx], 0xa3da08`（imm@0x4946C6）＝**ctor での vptr 代入のみ、実行時差し替えなし**（v310 残点1 の再独立確認） | v314_04 §3-12 `refs(2): 0049465A 004946C8` | [確定] |
| 1-9 | シングルトン [0xB34D90] の**書手も 2 件のみ**: 0x49468B `mov dword [0xb34d90], esi`（ctor が構築 obj を公開）と 0x49471B `mov dword [0xb34d90], 0`（条件付きクリア）。＝ch0 鎖のオブジェクトが ctor で固定されることを補強 | v314_07 §H / v314_08 §3 | [確定] |
| 1-10 | スイープ内の間接 call（`call reg`/非 IAT `call [mem]`）は v1 で **595 件**、これは追跡していない（留保） | v314_05 `indirect: 595` | [未確定] |

**項目1 主判定**: 深さ5 以降（固定点 723 関数・深さ19、緩和版3000 関数・深さ20）まで継続して走査した結果、**Warnings.txt への書出経路（WriteFile/CreateFileA/0x494730）には一切到達しない** → v310 の深さ4 打切りによる「書出なし」は深さ5 以降でも維持される。**[確定]（負証拠 2 行を含む・間接 call 留保付き）**

### 1.2 項目2 — 間接到達 3 fn（0x44FA43/0x51BE64 帯含む）の役割と参照元

v310 残点2「vtable/関数ポインタの参照元特定は未実施」→ **3 件とも参照元を特定、v310 残点2 解消**。

| fn（範囲） | 参照元（全セクション find_dword = 各件のみ） | 役割（命令根拠） | 判定 |
|---|---|---|---|
| **0x502500**（0x502500..0x502647、`ret` 後 CC で終端、size 0x147） | **.data 0xB0BF78 の1件**。エントリ内容 = `{impl=0x502500, 0x4FDE30, 0, 0, 0xA509D4, 0xA509D0, 0x149, 0xA50988}` = SCRIPT_OPCODE_TABLE idx103/op0x0148 **LoadGame 行の impl=0x502500 / help=0x4FDE30 と一致** | **コンソール/スクリプト `load` コマンド impl**: `call 0x4fae80`（引数デコード）→ ファイル名組立（`push 0x2e; push ecx; call 0x982ec0` + IAT 0xA28164/0xA28160）→ `mov ecx,[0xb33b00]; push ecx; lea edx,[esp+0x1c]; push edx; push 0; call 0x465860`（save manager LoadGame）→ `call 0x579b60`（結果表示）。svm_495 の callee 列 `0x4fae80, 0x982ec0, 0x465860, 0x579b60, 0x9811e2` と完全一致 | 役割 **[確定]**／参照元 **[確定]**／**ディスパッチ命令（テーブル基址の .text imm32）は不在＝グローバル経由と推定 [未確定]** |
| **0x5B5D60**（..0x5B5DE4、`ret` @0x5B5DE4） | **.text 0x5B64FF の `push 0x5b5d60`（imm@0x5B6500）のみ** → 直後 `call 0x579c10`（push 列 = 0, eax, ecx, 1, **0x5B5D60**, edx → cb = arg2） | **msgbox CONTINUE コールバック**: `push 0x414; call 0x589b70`（メニュー0x414 取得）→ `mov ecx,[0xb33b00]; mov eax,[ecx+0x6c]…push 0; push 0; push eax; call 0x465860`（最新スロット名で LoadGame）→ 成功時 `call 0x459400`（apply）→ `jmp 0x5b5960`（close+display）、スロット未設定なら 0x579C10 でダイアログ | **[確定]**（s82 §B.2 と命令一致） |
| **0x5AE190**（..0x5AE216、`ret` @0x5AE216） | **.text 0x5AF057 の `push 0x5ae190`（imm@0x5AF058）のみ** → 直後 `call 0x579c10`（push 列 = 0, ecx, edx, 1, **0x5AE190**, eax → cb = arg2） | **LoadMenu(0x40E) 行選択ハンドラ**: メニュー 0x40E 取得 → `[esi+0x4c]==0` なら 0x579C10 ダイアログ（文字列 0xA2F7EC = 空）、非0なら `push 0; push 0; push eax; call 0x465860` → 成功時 `call 0x459400; call 0x57afb0` → メニュー再取得・`[esi+0x64]=0` | **[確定]**（s58「行選択 0x5AE1D6→0x465860」と一致、0x5AE1D6 は本 fn 内 BB＝v310 1-16 と整合） |

- 補足1: **直接 call/jmp が0件なのは参照が push-imm（コールバック登録）と .data テーブルだから**であり、raw E8/E9 スキャンには現れない（v310 の「間接待ち」を機構的に説明）。[確定]
- 補足2（帯注記）: 0x502500 は 0x4F0000-0x503000（コンパイラ帯＝v315「formatter 帯」該当候補）に重複しうるが、本割当の「間接到達 fn の所在帯（判明次第）」条項により**単関数 read-only** で調査した。
- 補足3: s82 の登録地址表記 0x5B64E4 は同文脈の jump-table case 先頭（`mov byte [esi+0x4d],1` @0x5B64E4）で、実際の push は 0x5B64FF。同一登録シーケンス内の別アンカー＝不矛盾。

### 1.3 項目3 — ch0 wrapper `0x404EC0` の実挙動

| # | 結論 | 根拠（命令） | 判定 |
|---|---|---|---|
| 3-1 | 本体 = **0x404EC0..0x404ED4**（CCラン12・16アライン、caller 44 件）。実体: `mov ecx,[esp+4]; lea eax,[esp+8]; push eax; push ecx; push 0; call 0x4941a0; add esp,0xc; ret` = cdecl `fatal_log(fmt, ...)` を **ch=0** で 0x4941A0 に委譲（v310 の「push 0; call 0x4941A0」は省略表記で、va リスト組立てを含む実体に**補強**）。参照 0x404EE0 は同一形の ch=3 wrapper（`push 3`）で再確認 | v314_01 §3-1/3-2 | [確定] |
| 3-2 | 経路: 0x4941A0 で **`cmp dword [0xb34d90],0; je 0x49434b`（logger NULL → 0x49434B epilogue へ early-return、出力も終了もせず eax=0）**。非 NULL なら vswprintf（`call 0x982820`）整形 → CRLF 正規化 → `cmp eax,9; ja 0x494337; jmp dword ptr [eax*4+0x494364]`（@0x494228-0x494231、eax=ch）。**jump table 0x494364 ch0 → 0x494238** | v314_01 §3-4/3-9 | [確定] |
| 3-3 | 0x494238: `mov ecx,[0xb34d90]; mov eax,[ecx]; mov eax,[eax]; lea edx,[esp+4]; push edx; call eax` → **vtable[0]**。vtable 0xA3DA08 実測 [0..9] = {0x494390, 0x4943A0, 0x68F970, 0x494890, 0x68F970, 0x494950, 0x4943A0, 0x4943E0, 0x4943B0, 0x4949E0} → **[0] = 0x494390** | v314_01 §3-5/3-6 | [確定] |
| 3-4 | 0x494390 = **`push 0; call dword ptr [0xa280d4]`**、IAT 0xA280D4 解決 = **KERNEL32.dll!ExitProcess** | v314_01 §3-7/3-8 | [確定] |
| 3-5 | **実挙動 = 整形済みメッセージをどこにも出力せず（ch0 ハンドラは ExitProcess のみ）、プロセスを exit code 0 で即終了**。例外経路2種は終了しない: (a) [0xB34D90]==NULL → 0x49434B へ return、(b) vswprintf 失敗（`jl 0x49434b` @0x4941E6）→ return | 3-2..3-4 の連鎖 | [確定] |
| 3-6 | [0xB34D90] obj の **vptr = 0xA3DA08 は ctor で代入**（0x494658/0x4946C6）され obj は 0x49468B で公開 → vtable[0] 鎖が構築時に固定（項目1-8/1-9 と同一証拠） | v314_04/07/08 | [確定] |
| 3-7 | 本帯 caller 文脈: FUN@0x44F3D0 の ch0 呼出4箇所 = 0x44F5CF `'Unable to find masterfile: %s'`(0xA38854) / 0x44F5F3 `'Unable to find masterfile: <unknown>'`(0xA3882C) / 0x44F646・0x44F6C1 `'Too many selected files to compile!'`(0xA38808) — いずれも DataHandler 致命系 → **ch0 = 即終了**。起動フェンス 0x4FCCEF/0x4FCD0D（`0xA4A0E0`/`0xA4A098`）とも整合 | v314_02 callee 列 / v314_08 §5 / TODO_re49 既存記載 | [確定] |

**項目3 主判定: [確定]** — ch0 wrapper 呼出時、logger 初期化済みなら「メッセージ無出力・ExitProcess(0) 即終了」、未初期化/整形失敗なら「無出力・return」。

### 1.4 項目4 — `0x44F0C0` の戻り値契約

| # | 結論 | 根拠 | 判定 |
|---|---|---|---|
| 4-1 | 関数範囲 = **0x44F0C0..0x44F3A7**（先頭 CCラン8・16アライン、単一エピローグ `ret 8` @0x44F3A5）。後続: 0x44F3A8..0x44F3C0 = switch ジャンプテーブル（本体 0x44F2A9 `jmp dword ptr [eax*4+0x44f3a8]` が指す）、CC 0x44F3C1..0x44F3CF、**0x44F3D0 = v310 の FUN@0x44F3D0（境界一致）** | v314_02 全文 disasm + v314_01 CC 検査 | [確定] |
| 4-2 | **訂正（自己 probe の誤差を含む）**: v314_02 の初期 walk が「end=0x44FA44」と出したのは、ret+CC 観測が jump-table データ（0x44F3A8..）を通過し隣接 fn 0x44F3D0 まで到達したため。**本関数の実終端は 0x44F3A7**。0x44F41A 以降の call 列は 0x44F3D0（v310 帯）のもの | disasm 中の `loopne` 等の偽命令（0x44F3A8 起点）と 0x44F3C1 CC ラン | [確定]（裁定: 旧値不削除・訂正記録） |
| 4-3 | 戻り値スロット = **[esp+0x18]**（4 push 相対、絶対 R-0x114）≡ エピローグの **[esp+8]**（0x44F394 `mov eax, dword ptr [esp+8]`）。**mov 書き込みは2箇所のみ**: 0x44F101 `mov dword [esp+0x18], 1`（初期化）／0x44F225 `mov dword [esp+0x18], ebp`（ebp=0、`call 0x44dcf0; test eax,eax; jne 0x44f22e` の**失敗時のみ**） | v314_07 §A（0x44F0C0..0x44F3A8 全命令走査でこの3行のみ） | [確定] |
| 4-4 | **契約: eax = 1（成功・既定）／0（失敗）**。0 になる条件は「0x44F21C の `call 0x44dcf0`（ecx=this=DataHandler、arg1=esi=レコード/フォーム、arg2=本 fn の第2引数）が0を返したとき」のみで、1→0 の単方向・他状態なし。**unknown/失敗判定であり再発抑制ではない**（呼出元 0x44F759 はフォーム毎に独立呼出しで毎回 0x44F101 が再初期化され、ローカルフラグに横断状態が存在しない） | 4-3 + v314_02 §4-6（caller ループ 0x44F735..0x44F77E: `call 0x44f0c0; test eax,eax; jne 0x44f775` → 0 のとき 0xA37B78 'unrecognized form' 発報） | [確定] |
| 4-5 | 0x44DCF0 = **レコード処理関数**（SEH prologue、arg1 → `call 0x4511c0` 型取得＝0x44F0C0 と同一 API、`cmp byte [ebp+0x114],0` で arg2 フラグ分岐、`call 0x46abd0` チェック）。callers = 0x44F21C（本 fn）/ 0x45BFD7 / 0x461479 / 0x4D14EE。**0 を返す具体条件は本体未完全追跡** | v314_07 §B | [部分確定] |
| 4-6 | 呼出元2件: **0x44F759**（FUN@0x44F3D0 のフォーム列ループ、0 → getter 0x494480 + 0x4A7A60 で 0xA37B78 発報）と **0x45ECE4**（v313 禁止帯のため VA のみ記録、文脈読取なし） | v314_01 §4-2 / v314_02 §4-6 | [確定] |

**項目4 主判定: [確定]** — 「成功=1／0x44DCF0 失敗=0 の unknown 判定」で、再発抑制説は否定（ローカル単方向フラグ）。0x44DCF0 の0条件の深層は [部分確定]（新残点 r3 へ分離）。

### 1.5 項目5 — 帯内表走査 5 本の役割名（0x570000-0x574000）

v310 の5範囲を実測解体。**範囲内に 2 fn が含まれるものがある**（境界 = ret + **短CC（1-2B）**。v310 の CCラン≥4 境界探索が短CCを見逃したため範囲が隣接 fn と融合していた — off-by-pad の逆方向ケース）。

| v310 範囲 | 実 fn（境界証拠） | 役割（命令根拠） | 判定 |
|---|---|---|---|
| 0x571640..0x571716 | **0x571640..0x57165C**（`ret 4` @0x57165C + CC 1B @0x57165F） | **scalar deleting dtor**: `test byte [esp+4],1; mov [esi],0xa68784`（vtable 代入）→ flag&1 で `call 0x401f20`(delete this) → this を返す。直接 caller 0 件 = vtable 経由 | [確定] |
| 同上 | **0x571660..0x571716**（`ret 0x10` @0x571709/0x571713） | **表キー検索→文字列コピー（get-and-copy）**: 引数 (dst, f32, f32, u32)、`lea esi,[ecx+8]` を起点に 200 件×0x1C 線形走査（[+0]f32/[+4]f32 を fucompp、[+8]u32 を `cmp [esi],edi`）。命中時 `[ecx+idx*0x1C+0x10]` の文字列を **len([+0x14])==0xFFFF なら strlen で算出し `strncpy(dst, str, 0x7fe)`** → al=1、未命中は dst を空文字列にして al=0。caller は 0x5860A1 の1件（this = `call 0x571f90; mov ecx,eax` のシングルトン、key = fild [0xB3A700]/f32/1、約6KB スタックバッファ） | [確定] |
| 0x571720..0x571782 | **単一 fn**（`ret 0xc` @0x571769/0x57177F、CCラン10） | **表キー検索→文字列ポインタ返却（get-ptr）**: 同一キー走査、命中時 `lea eax,[edx*8]; sub eax,edx; mov eax,[ecx+eax*4+0x10]` = **char\* を eax で返す**／未命中 `xor eax,eax`。コピーなし。callers 2 件（0x63ABD4/0x657E51、いずれも `fild dword [0xb1399c]` を鍵に供給） | [確定] |
| 0x571800..0x5718FB | **0x571800..0x57181D**（`ret 4` @0x57181B + CC 2B @0x57181E-1F） | **scalar deleting dtor**: `call 0x571790`（dtor 本体、CCラン14で独立 fn）→ flag&1 で delete → this を返す | [確定] |
| 同上 | **0x571820..0x5718FA**（`ret` @0x5718FA、CC 1B） | **表全件クリア（shutdown clear）**: 200 件走査（esi=base+0xc, stride 0x1C）。object([+0xC]) 非0 エントリのみ: `push 1; push 0; call 0x582160` ゲート → vtable[+0x88] 参照 → **`call ebp`（ebp=[0xa2807c]=InterlockedDecrement）→ 結果0 なら vtable[0] に Release(1)**（別 ptr でも同様）→ str `call 0x401f20`(free) → f0/f1/key/object/str/len/cap ゼロ、[+0x18] = 定数 [0xA30634]。首尾の `push 3`/`push 2; call 0x977c50` は **常真スタブ 0x977C50 = `mov al,1; ret`（retail 無効化デバッグフック、v172 docs と一致）**。callers 5 件（0x407BD8/0x407D2B = WinMain 帯ティアダウン、0x572044、0x5857FD、0x585A49） | [確定] |
| 0x571DF0..0x571F66 | **0x571DF0..0x571E7D**（SEH prologue、`ret` @0x571E7D + CC 2B @0x571E7E-7F） | **要素デストラクタ**: str ptr(+0x10) free → len/cap ゼロ → [+0xC] object 非0なら `call [0xa2807c]`(InterlockedDecrement(&+4)) → 結果0 で vtable[0] Release(1)。**200×0x1C 配列の dtor として 0x984BCF(CRT) 経由で登録**（後続 fn の `push 0x571df0; push 0x571d40; push 0xc8; push 0x1c; push edi; call 0x984bcf` = ctor 0x571D40 / dtor 0x571DF0 ペア）。**v310 両論併記**: 「entry 破棄」は維持、「InterlockedDecrement ループ」は**単発 Release（0x571E50..0x571E6A、後方に戻る分岐なし）**であり、範囲に隣接 fn が融合していたと裁定（ret+短CC 境界 0x571E7D/0x571E7E が根拠） | [確定]（v310『ループ』表記の訂正を含む裁定） |
| 同上 | **0x571E80..0x571F65**（`ret` @0x571F65 + CC 1B @0x571F66） | **配列/テーブル初期化**: 0x984BCF で 200×0x1C 配列構築、`mov dword [edi+0x15e0], 0xa687a4`（BST ヘッダ vtable）、+0x15E4/+0x15E8/+0x15EC ゼロ、200 件の f0/f1/object/str クリアループ（`sub [esp+0x14],1; jne 0x571ef6`） | [確定] |
| 0x572170..0x5723DA | **単一 fn**（`ret` @0x5723D9、CC11） | **期限管理 tick（expiry sweep）**: ゲート `call 0x578f60` → `push 1; call 0x571f90` のシングルトン → 200 件ループで **`[+0x18] > 0` かつ `([+0x18] − [0xB33E9C]) <= 0` のクロス1フレームのみ**処理（`fldz; fcom [esi+0xc]; test ah,5; jp skip` と `fsub [0xb33e9c]…fcompp; test ah,0x41; jp skip` の極性を PF 偶奇で実測解読）→ object の vtable[+0x88] 参照 → InterlockedDecrement/Release ×2 系 → エントリ全フィールドクリア（[+0x18] は定数 [0xA30634]）。その後 **singleton+0x15E4 の連結リスト列挙**: 値の `[+4] < 2` は即時 unlink（+0x15E0 の木探索）+ 破棄、`>= 2` は値の [+0x18] タイマを同様にクロス判定し期限到達で破棄。破棄尾 = `call 0x7aa860`（コンテナ削除, callers 29）→ `call 0x571df0`（要素 dtor）→ `call 0x401f20`（delete）。caller = 0x583975 の1件（フレーム更新系） | [確定]（機構） |
| 同上（減算ステップ） | [0xB33E9C] | **初期 f32 = 195.06265258789062（正）**。.text 内の書手は **CRT 静的初期化域 0x9DC2BC の `fstp` の1件のみ**（`mov [imm],reg` 系は0件）＝実行時更新は未確認。単位/意味は特定不能 | [未確定]（新残点 r2） |

**項目5 主判定: [確定]（5/5 範囲に役割名）** — read×2（get-and-copy / get-ptr）、clear×1（全件解放）、dtor/init×2（要素 dtor + 配列初期化）、tick×1（期限管理）で、全て同一の 200×0x1C 表（+0x18 = タイマ、+0x10 = 文字列、+0xC = object）を扱う。

---

## 2. 判定サマリ（5 段階）

| # | 項目 | 判定 |
|---|---|---|
| 1-1 | スイープ再現（roots 0x5723E0/0x57B8E0・CRT 除外・禁止帯 cut） | [確定] |
| 1-2 | v1 固定点 723 関数・深さ19・深さ5 frontier 53 本 | [確定] |
| 1-3 | 180 vs 88 vs 176 の差＝ルール+帯 cut 起因 | [確定] |
| 1-4 | WriteFile/CreateFileA/ReadFile IAT 0 件（v1/v2） | [確定負] |
| 1-5 | 0x494730 への到達 0 件（v1/v2） | [確定負] |
| 1-6 | fileish = 削除/列挙のみ（セーブ管理系） | [確定] |
| 1-7 | fopen 系 = stdio 読出（0x747D10）+ シェーダーキャッシュ 'wb'（0x8014E0 系） | [確定] |
| 1-8 | getter 0x494480 深さ9到達＝path 取得のみ | [確定] |
| 1-9 | ch1 0x4A7A60 深さ1-15 223 件＝全 no-op 経路 | [確定] |
| 1-10 | vtable 0xA3DA08 参照 全セクション 2 件＝差し替えなし | [確定] |
| 1-11 | [0xB34D90] 書手 2 件（公開/クリア） | [確定] |
| 1-12 | 間接 call 595 件の未追跡（留保） | [未確定] |
| 2-1 | 0x502500 役割 = `load` コマンド impl | [確定] |
| 2-2 | 0x502500 参照元 = .data 0xB0BF78 エントリ | [確定] |
| 2-3 | 0x502500 ディスパッチ基址の .text 命令 | [未確定] |
| 2-4 | 0x5B5D60 役割 = msgbox CONTINUE コールバック | [確定] |
| 2-5 | 0x5B5D60 参照元 = push 0x5B64FF → 0x579C10 cb | [確定] |
| 2-6 | 0x5AE190 役割 = LoadMenu 行選択ハンドラ | [確定] |
| 2-7 | 0x5AE190 参照元 = push 0x5AF057 → 0x579C10 cb | [確定] |
| 2-8 | 直接 call/jmp 0 件の理由 = push-imm / .data 経由 | [確定] |
| 3-1 | 0x404EC0 本体（va 組立て + ch=0、範囲・caller44） | [確定] |
| 3-2 | 経路 0x4941A0 → jump table → 0x494238 → vt[0] → 0x494390 | [確定] |
| 3-3 | IAT 0xA280D4 = KERNEL32!ExitProcess | [確定] |
| 3-4 | 実挙動 = 無出力・ExitProcess(0) 即終了（NULL/整形失敗時は return） | [確定] |
| 3-5 | [0xB34D90] obj の vptr=0xA3DA08 構築時固定 | [確定] |
| 3-6 | 本帯 caller = DataHandler 致命4件と整合 | [確定] |
| 4-1 | 0x44F0C0 範囲 0x44F0C0..0x44F3A7（+テーブル/CC/次 fn 境界） | [確定] |
| 4-2 | 戻り値スロット [esp+0x18]≡[esp+8]、mov 書手 2 件 | [確定] |
| 4-3 | 契約 = 成功1/失敗0（unknown 判定・非抑制） | [確定] |
| 4-4 | 0x44DCF0 = レコード処理、0 条件の深層 | [部分確定] |
| 4-5 | 呼出元 2 件（0x44F759 発報 / 0x45ECE4 VA のみ） | [確定] |
| 5-1 | 範囲1 = dtor(0x571640..5C) + get-and-copy(0x571660..716) | [確定] |
| 5-2 | 範囲2 = get-ptr(0x571720..781) | [確定] |
| 5-3 | 範囲3 = deleting-dtor(0x571800..81D) + 全件クリア(0x571820..8FA) | [確定] |
| 5-4 | 範囲4 = 要素 dtor(0x571DF0..E7D) + 配列初期化(0x571E80..F65) | [確定] |
| 5-5 | 範囲5 = 期限 tick(0x572170..23D9) | [確定] |
| 5-6 | クロス1フレーム判定の極性解読（PF 偶奇） | [確定] |
| 5-7 | 0x977C50 = 常真スタブ（retail 無効化） | [確定] |
| 5-8 | [0xB33E9C]=195.06 の実行時意味・単位 | [未確定] |
| 5-9 | v310『InterlockedDecrement ループ』→単発 Release＋範囲2fn 融合の裁定 | [確定] |

判定件数: **[確定] 32 / [部分確定] 1 / [推定] 0 / [未確定] 3 / [確定負] 2**（合計 38 行）。**5 項目の主判定はすべて [確定]**。

**解消した v310 残点（5/5）**: 残点1（深さ5打切り→723関数で解消、vtable 再確認含む・間接 call 留保）／残点2（3fn 参照元特定・ディスパッチ基址のみ新残点へ）／残点3（ch0 実挙動）／残点4（0x44F0C0 契約・0x44DCF0 深層のみ新残点へ）／残点5（5本の役割名・定数意味のみ新残点へ）。

---

## 3. 新規残点（次束用・低優先）

1. **[未確定] 0x502500 のディスパッチ基址**: .data 0xB0BF78 を読む .text 命令が imm32 で不在（0xB0BF78/近傍基址の全セクション走査で .text ヒット0）。グローバル経由（`mov eax,[global]; call [eax+idx*N+off]`）と推定。opcode VM の fetch 経路特定は次束で。
2. **[未確定] [0xB33E9C]（初期 195.06）の実行時意味・単位**: 書手は CRT 静的初期化 0x9DC2BC の fstp のみ。0x5723E0 が [+0x18] に書くタイマ値との対応（何秒/何フレーム分の TTL か）が未特定。0x572170 の「クロス1フレーム」実効値の解釈に直結。
3. **[未確定] 0x44DCF0 が 0 を返す具体条件**: 型取得（0x4511C0）後の分岐本体（0x44DD3D 以降）を未追跡。契約レベル（0=失敗）は確定済みで、移植に必要な「失敗の定義」だけが残る。
4. **[未確定] スイープ内間接 call 595 件（v1）の先**: `call reg`/非 IAT を追跡すれば書出否定がさらに強化される（現状は直接呼出経路のみで負証拠）。
5. **[未確定] fn-end walker の短CC（1-3B）境界欠損**: 0x450550 の end=0x450978 が 0x4507D0 を内包（DeleteFileA@0x4508FB が2 fn に重複帰属）など、ret+短CC 境界で隣接 fn に融合。境界判定の改良（ret 直後の CC ラン 1-3 でも停止）は次束で。項目5 の範囲融合（v310 側）と同根源。

---

## 4. selftest 再現メモ

中間物はすべて `%TEMP%\opencode\`（= `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\`）。再実行:

```powershell
chcp 65001
python $env:TEMP\opencode\v314_01.py *> $env:TEMP\opencode\v314_01.out   # ch0 wrapper 境界・jump table・vtable・IAT・0x44F0C0 callers
python $env:TEMP\opencode\v314_02.py *> $env:TEMP\opencode\v314_02.out   # 0x44F0C0 全文・eax 書き込み・caller 文脈
python $env:TEMP\opencode\v314_03.py *> $env:TEMP\opencode\v314_03.out   # 表走査5本 callers/callees/key（初回）
python $env:TEMP\opencode\v314_04.py *> $env:TEMP\opencode\v314_04.out   # dispatch gap・vtable refs・3fn heads+refs
python $env:TEMP\opencode\v314_05.py *> $env:TEMP\opencode\v314_05.out   # 深さスイープ v1（厳格 fn-start・固定点）
python $env:TEMP\opencode\v314_06.py *> $env:TEMP\opencode\v314_06.out   # 5本全文+push site+imm32 スキャン+スイープ v2（緩和）
python $env:TEMP\opencode\v314_07.py *> $env:TEMP\opencode\v314_07.out   # [esp+0x18] 書手・0x44DCF0・0xB33E9C・0x977C50・境界・singleton 書手
python $env:TEMP\opencode\v314_08.py *> $env:TEMP\opencode\v314_08.out   # fopen mode='wb'・shader 文字列・[0xB34D90] 書手・0xB33E9C fstp
```

主要 selftest 値（本裁定の根拠）:

- `CC run before 0x404EC0 = 12 / align16=True`、`callers(0x404EC0): 44 sites`
- jump table 0x494364 = `{0x494238, 0x494337, 0x494261, 0x49428B, 0x4942B5, 0x4942DF, 0x4942EC, 0x494300, 0x49434B, 0x49430D}`
- vtable 0xA3DA08[0..9] = `{0x494390, 0x4943A0, 0x68F970, 0x494890, 0x68F970, 0x494950, 0x4943A0, 0x4943E0, 0x4943B0, 0x4949E0}`
- `0x494390: 6a00 ff15d480a200` → IAT 0xA280D4 = `('KERNEL32.dll', 'ExitProcess')`
- `refs to 0xA3DA08 (all sections): ['0x49465A', '0x4946C8']`（＝2 件のみ）
- `[0xB34D90] stores: 0x49468B (mov [imm],esi) / 0x49471B (mov [imm],0)`
- `callers(0x44F0C0): 2 sites: 0044F759 0045ECE4`、first `ret 8` @0x44F3A5、範囲 0x44F0C0..0x44F3A7
- `[esp+0x18] writes: 0x44F101 (mov …,1) / 0x44F225 (mov …,ebp)`、read `0x44F394 (mov eax,[esp+8])`
- `0xA37BC0 = 'DataHandler: internal error'`、`0xA38854 = 'Unable to find masterfile: %s'`、`0xA38808 = 'Too many selected files to compile!'`
- スイープ v1: `visited 723 / d≤4=88 / d5=53 / d6=76 / d7=83 / fileish=3 / WriteFile=なし / 0x494730 hit=0 / band cuts=551 / suspects=1719`
- スイープ v2: `visited 3000(cap) / d≤4=176 / fileish=10 / fopen hit=9 / 0x494730 hit=0 / WriteFile=なし`
- `refs to 0x502500: ['00B0BF78']`（.data、{0x502500, 0x4FDE30, …}）／`refs to 0x5B5D60: ['005B6500']`（push @0x5B64FF）／`refs to 0x5AE190: ['005AF058']`（push @0x5AF057）
- `0x977C50: b001 c3`（mov al,1; ret 常真）、`0xA36338 = 'wb'`
- `[0xB33E9C] f32 = 195.06265258789062、書手 = fstp @0x9DC2BC のみ`
- 表レイアウト（5 fn 共通・0x1C stride×200）: `{+0 f32, +4 f32, +8 u32 key, +0xC object, +0x10 char*, +0x14 u16 len(0xFFFF=未計算), +0x16 u16, +0x18 f32 timer}`

**既存資料との不一致（明記）**:
- `OPENCODE_REVERSE_ENGINEERING_INSTRUCTION.md` の成果物表記（`v309_*.md`）・最優先タスク（TODO #70）は本割当と不一致 → 本割当優先（本ファイルに保存、指示ファイルは不変更）。
- `svm_495_miss_addrs.md` の 0x502500「160B」列は実測 size **0x147（327B）** と不一致（列意味が size なら訂正要。impl アドレス・callee 列は一致）。
- v310 §4-5 の表走査5本「範囲」は内部に2 fn を含む（短CC境界の融合）→ 本ファイル §1.5 で境界を再定義（旧範囲は削除せず併記）。
- v310 残点3 の `0x404EC0 = push 0; call 0x4941A0` は省略表記 → 実体は va 組立て込み（矛盾なし・補強）。
