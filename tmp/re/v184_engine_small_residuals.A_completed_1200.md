# v184 — 束D 音響・入力・CRT・I/O 少数残件束（F2 + O4 + s81 残9 の当該分）
# =====================================================================================
# 対象: Oblivion.exe v1.2.0416 (Steam, ImageBase 0x400000)
# 日付: 2026-10-29 / TODO serial 172 (V172・低) / 解析専任（docs/TODO は未編集）
# 出典: TODO_re49.md L33(F2)・L102(O4) / v180_letter_rows_audit.md 束D / s81_stream_audio_crt_io.md 残点
# 方法: capstone 5.0.7 逆アセンブル + raw E8/E9 バイト走査 + Ghidra FID 名台帳 (tmp/re/cache) + 外部 IDL 照合
# 凡例: [確定]=命令列/データで直接確認 / [推定]=強い状況証拠 / [未解]=未解決
# 証跡: %TEMP%\opencode\v184_{f2,f2b,o4a..o4e,s81a..s81l,f,g,h,i,j,k,l,m,n,o}.txt（本ファイルと同ディレクトリ）

## 0. サマリ

| 節 | 実施 | 解明 | 内訳 |
|---|---|---|---|
| F2 音響残件 7 項目 | 7 | **7** | [確定] 6 + [確定-構造/推定-強] 1（type bit 0x20 の名称） |
| O4 入力残件 6 項目 | 6 | **5 + 部分 1** | [確定] 5、+0x1BD5 は「8方向レイアウト説」を訂正のみ（値 0x0B の意味は未解） |
| s81 残9 の当該分（O3 1 + O5 2 + O6 3） | 6 | **6** | [確定] 5 + [確定-構造] 1（hypot = 死コード同梱） |
| **合計** | **19** | **18 解明 + 1 部分** | 新規残点 10 件（§5） |

- 束C（v183: レンダラ/ストリーミング = s81 Task A 群）と重複なし。v183 §11-4 の訂正素材（FUN_00450550 = zlib）は §3.4 で**確認・採用**。
- 主要な既存記述の訂正は §4 に一覧化（s60_audio / s60_input / s60_crt / s60_io / s81 の 9 件）。

---

## 1. F2 — 音響残件 7 項目（§56.2 / §56.8）

### F2-1. AM+0xA6 の絶対/相対 — [解明 [確定]]
- **結論**: `AM+0xA6 = bAudioActive`。`FUN_6ACAB0`（発音予約）0x6ACB14 `cmp byte [edi+0xa6],0 / je 0x6acb59` で分岐:
  - **A6 != 0（音響アクティブ）→ 相対予約**: 0x6ACB44 `neg ecx` → payload start = **−arg2**（負。初回処理で絶対 now+|arg2| に解決）。
  - **A6 == 0 → 絶対予約**: 0x6ACB82-0x6ACB8C `ecx = [0xB33EA0](now) + arg2` → start = **now + arg2**。
- 書込側: `FUN_6AE860`(AudioManager::Update) 0x6AE8F2（毎フレーム、状態遷移時 0→1 で 0x6A9B40 開始 / 1→0 で 0x6A9C00 停止）。
- s56 §2.5 の旧記述（A6!=0 → now+arg2）は逆 — s60_audio §1.1 の訂正を**再検証して確定**。試行: 0x6ACAF0-0x6ACBA0 リニア逆断（v184_f2.txt）。

### F2-2. FUN_6AD030 序盤の bit2 — [解明 [確定]]
- **結論**: **0x6AD030-0x6ADE30 に `test ?,0x4`（bit2）は 0 件**。`[esi+0xDC]` へのアクセスは全 24 箇所、内訳は `test byte ?,2`（bit1=再生中）9 件 / `test ?,0x18`（bit3 pending・bit4 fade-in）5 件 / `or ?,1` 6 件 / mov3 件のみ（v184_f2.txt F2-2 一覧）。
- s60_audio §1.2「bit2 テストは存在しない」を**全数走査で確定**。
- 付随: bit2(0x4) の実使用箇所は `FUN_006AB420` 0x6AB4A1 のみで、意味は **Run 後の Pause 要求**（§3.1 O3 で確定）— s60_audio §6-1 の推定「一時停止/再開系」を裏付け。

### F2-3. 曲カーブ (2×[2F0]−1)×[2F8] — [解明 [確定]]（FPU 厳密追跡）
- **結論**: カーブの X は **[2F0] ではなく t = clamp((now − [AM+0x2E8]) / 1700, 0, 1)**（フェード窓パラメータ）。[2F0] は `fcompp` 比較（定数 0xA68610=0.05 閾値 / [2F4] 完了判定）にのみ使用。
- 厳密経路（v184_f2.txt F2-3a/b, v184_f2b.txt）:
  1. 0x6AD2A7-0x6AD307: `fild(now−[2E8])` → `fdiv [0xA771C0]=1700.0` → fldz/fld/fcom/fld1 で **[0,1] clamp** → 完了時 FPU スタック頂 = **t**、直下 = 0（スタック形 [t, 0] を保持）。
  2. pending（旧曲）分岐 0x6AD5BC-0x6AD5C8: `fld [2F8] / fxch / fadd st(0),st(0) / fmul st(1) / fsubp` = **[2F8]·(1 − 2t)**。定数 0xA3D65C = **0.5**（t ≥ 0.5 で 0 へスナップするための比較）を 0x6AD5A4 で比較 [確定]。
  3. fade-in（新曲）分岐 0x6AD629-0x6AD637: `fsub [0xA2FAA0]=0.5 → fadd st(0),st(0) → fmul [2F8]` = **(2t − 1)·[2F8]**。
- s56 §5-3 の「[2F0]」表記は命令列上の fld（比較用）が由来 — **X = t で確定**（s60_audio §1.3 の[推定-強]を解消）。

### F2-4. type bit 0x20 の最終意味 — [解明 [確定-構造] / 意味は [推定-強]]
- **結論**: bit0x20 = **2D UI/アクティベーション系の使い捨てサウンド**。エンジン内の単独 `test ?,0x20` は **5 箇所存在**（s60_audio §1.4「存在しない」は誤り → §4 訂正）。全挙動:
  | 箇所 | 関数 | 意味 |
  |---|---|---|
  | 0x6A9BBB | FUN_006a9b40 | IsPlaying 中で bit0x20 **無し** のときのみ 0x200（再供給）を付与 → UI 音は再供給しない |
  | 0x6AB5DD | FUN_006ab540 | 水中([AM+0xA5]) でも bit0x20 なら**水中ピッチ変調を除外** |
  | 0x6AB750 | FUN_006ab6f0 | 同上（fUnderwaterFrequencyDelta 適用の除外） |
  | 0x6AE7A1 | FUN_006ae720 | bit0x20 なら減衰加算系をスキップ |
  | 0x6AF4D9 | FUN_006ae860(Update) | bit0x20 なら 0x100（started mark）を**立てない** |
  - 併用: 0x6AE947 `test al,0x21`（bit0 OR bit0x20）= sound map → アクションキュー([AM+0x320]) 再投入条件。
- 生成元: 0x21 = 0x540448/0x540640/0x5D0BA6、0x121 = 0x4B7D34/0x5985E3/0x599F70（ContainerMenu "DRSBodyOpen" 等 = s60 §1.4 どおり）。
- s81 B2（使い捨て UI サウンド）と一致 → 二源照合で意味は [推定-強]、構造は [確定]。

### F2-5. [0xA785E8] = 1e-5 の適用箇所 — [解明 [確定]]
- **結論**: dword 参照は **0x6B6FA8 の 1 箇所のみ**（find_dword 全走査 = 1 hit）。`FUN_6B6F20`（音量+減衰適用）内 `fld qword [0xA785E8]`(=1e-5) → `max(scale, 1e-5)` → log10 → ×2000 → _ftol2 → [−10000,0] mB クランプ。下限 1e-5 = log10 −5 = **−100dB 床**。
- s56 §5-5 の「0x6B7340 内」表記は誤り → **0x6B6F20**（§4 訂正）。値検証: f64(0xA785E8)=1e-05 [v184_f2.txt]。

### F2-6. 0x65A9C6 / 0x65AC82 — [解明 [確定]]
- **結論**: 両サイトの E8 ターゲット = **0x6AE0A0**（バイト列 rel 直読で確認: 0x65A9C6→0x6AE0A0, 0x65AC82→0x6AE0A0）。収容関数は 0x65A970（ret 0xC）/ 0x65AC50（ret 0x10）= 3D エミッタ wrapper で、s60_audio §1.6 の帰属を確定。

### F2-7. FUN_6AC210 呼出元一覧 — [解明 [確定]]
- **結論**: raw E8 走査で **6 件**: `0x442694 / 0x443367 / 0x44630D / 0x446811 / 0x4599D1 / 0x66ED1C`（s60_audio §1.7 と完全一致）。
- 本体: FUN_6AC210 は [AM+0x304] リスト走査型（先頭 `push -1 / push 0x9b23e8` EH prologue、`mov edx,[esi+0x304]`）→ 正式名 `AudioManager::StopAllSounds(float fadeSeconds)` [推定]。文脈はロード/移動/ポーズ/セーブ/コンソールの全停止移行点（0x442xxx-0x446xxx メインループ・セーブ系、0x4599D1 セーブ系、0x66ED1C ロード系）[推定]。

## 2. O4 — 入力残件 6 項目（s60_input.md §60.7）

### O4-1. コンソール grave の実呼出経路 — [解明 [確定]]
- **結論**: 0x403520 経由（s60 の「直接 push 未検出」は誤り → 0x583731 に `push 0x1E` 発見）。完全経路:
  1. **入口**: `FUN_005821f0`（メニュー系更新）0x58372F-0x583735: `push 1 (mode) / push 0x1E (action 30) / mov ecx,ebx(inputMgr) / call 0x403520` [v184_o4b.txt]。
  2. **判定**: 0x403520 の action≥29 分岐（`sub eax,0x1d / sub eax,1`）→ **0x403594**: `test [edi+0x1A1D],0x80`（前フレーム DIK 0x29）jne→0、`test [edi+0x191D],0x80`（現フレーム）→ 常に **press-edge** を返す（**mode 引数は特殊 ID 29/30/31 で無視**。+0x191D = +0x18F4+0x29, +0x1A1D = +0x19F4+0x29）[v184_o4a.txt]。
  3. **反応**（0x58373A 以降）: `call 0x579060`（現メニュー ID 取得）→ `cmp eax,0x414` なら無視 → `push 1; call 0x585C10` → `mov ecx,ebx; call 0x403160`（キーボードデバイス操作）→ `mov ecx,edi; call 0x5859C0` → true なら `push 3; call 0x57D640`（**入力モード 3 = コンソール入力へ push**、`mov [ecx+0xE0],3`）、false なら `push 0 / push 3 / call 0x57CFE0`（モード 3 pop）[v184_o4b.txt]。
- 付随: bAllowConsole ゲートの実行時読取箇所は未特定 → §5-(3)。

### O4-2. ゲームパッド物理ボタン対応 — [解明 [確定]]（物理名の並びは [推定]）
- **結論**: XInput 不使用（DirectInput8 のみ）。実体は **生 DIJOYSTATE (sizeof=0x50)**:
  - **poll**: `FUN_004046A0` — パッド毎に record = `inputMgr+0x30+n*0xA0`、`vt+0x1C`(Poll) 失敗時 memset(0x50)、成功時 `vt+0x24`(GetDeviceState)(0x50, record)。前フレーム複製は `rep movsd 0x14 dwords` で record+0x00 → +0x50 [v184_o4d.txt]。
  - **ボタン (keycode 0..0x1F)**: `DIJOYSTATE.rgbButtons[keycode]`（cur = record+0x30 = inputMgr+n*0xA0+0x60、prev = record+0x80 = +0xB0、bit0x80 = 押下）。判定 = `FUN_00402FC0`（stride 0xA0、mode 4 種 jump table 0x403054）[v184_o4c.txt]。
  - **方向 (keycode 0x20..0x27)**: `DIJOYSTATE.rgdwPOV[0]`（cur = inputMgr+n*0xA0+0x50）。`FUN_00403070` が **POV値 / 4500（=45°×100）→ 0..7 セクタ + 0x20**（0xFFFF=中央 → −1）。除算は magic `0x7482296B`+`sar 11` = ÷4500 [確定]。**方向名 U,UR,R,DR,D,DL,L,UL = 0x20+0..7**（0xB02B98 表と同順）。
  - 既定の物理対応: btn0..btn10 = HID ボタン 0..10（DirectInput 列挙順。XInput 互換パッドなら A/B/X/Y/LB/RB/Back/Start/L3/R3 系 [推定（HID 慣習）]）、**btn32..39 (0x20..0x27) = POV 45° セクタ = D-pad 斜め含む 8 方向**（Quick1..8 既定 = s60_input 表と整合）。
- デバイス判定: `FUN_00403490(device,keycode,mode)` — device 0→kb `0x4032D0` / 1→mouse `0x4031E0` / ≥2→gamepad（`device-2` = pad index、`[this+0x18F0]` 台数チェック）[v184_o4c.txt]。

### O4-3. mode 0/1/2/3 の厳密意味 — [解明 [確定]]
- **結論**: `FUN_004032D0` の jump table 0x403368（4 経路）で確定。現 = `this+0x18F4+keycode`、前 = `this+0x19F4+keycode`、bit0x80 = 押下:
  | mode | 経路 | 条件 | 意味 |
  |---|---|---|---|
  | 0 | 0x4032E7 | `[cur]&0x80` | **held（現押下）** |
  | 1 | 0x403300 | `![prev]&0x80 && [cur]&0x80` | **press edge（押した瞬間）** |
  | 2 | 0x403323 | `[cur]&0x80` 無し && `[prev]&0x80` 有り | **release edge（離した瞬間）** |
  | 3 | 0x403346 | `([prev]^[cur]) & 0x80` の符号（jns→0） | **changed（双方向の変化）** |
- 同型実装が他デバイスにも存在: mouse `FUN_004031E0` / gamepad `FUN_00402FC0`（jump table 0x403054）— mode はデバイス共通 [確定]。
- s60_input の既存[推定]（0/1/2 = held/edge/changed 系）を確定・mode3 を「両方向 edge」に確定。特殊 ID 29/30/31 は mode を無視（O4-1）。

### O4-4. +0x1BD5 の 8 方向レイアウト — [一部解明（説を訂正）・値 0x0B の意味は未解]]
- **結論 1（構造）**: inputMgr オブジェクトの総サイズは **0x1BD8**（`FUN_00404A00` 内 `push 0x1BD8 / call 0x401F00` = new(0x1BD8) → [esi+0x20] へ格納 → 0x404540(LoadBindings) 呼出）→ バインディング表（0x1B7E + 3×0x1D = 0x1BD5 終端）の**直後 3 バイトでオブジェクト終端**。
- **結論 2（読み取り 0 件）**: disp32 全走査（.text の D5 1B 00 00）で **+0x1BD5 参照は 1 箇所のみ = `FUN_00403960` 0x403B02 の書込み**（`mov byte [ecx+0x1bd5],0x0B`、gamepad 初期化時のみ）。隣接 0x1BD0/0x1BD4/0x1BD8 の参照も ResetBindings 書き込みと別構造体（0x7CDAxx = スタックローカル、0x404A77 = new サイズ即値）[v184_o4d.txt]。
- → **「+0x1BD5.. に 8 方向バインディング配列がある」という s60_input の想定は不成立**。8 方向は独立配列を持たず、**keycode 0x20..0x27 + POV 変換（O4-2）で動的に解決**される。
- **未解**: 書込値 `0x0B` の意味（読取 0 件の write-only フィールド）→ §5-(1)。
- 付随: 方向名テーブル `0xB02B98`（U,UR,R,DR,D,DL,L,UL）は **dword 参照 0 件 = 死データ**。文字列本体は別用途で使用（'U'(0xA2FA90) は `FUN_009f3400` のキーボードキー名 sKBU 対として間接使用）[v184_o4d/f.txt]。

### O4-5. 0x9F6000〜 の先頭 init の用途 — [解明 [確定-構造]]
- **結論**: CRT の**静的初期化（C++ コンストラクタ）スタブ群**。ファンクション実体は `FUN_009f5940`（0x9F6000 は其中間。0x9F6000 へは E8/E9/絶対参照すべて 0 件）:
  - **FUN_009f5940**（0x9F5FF3 で [0xB399C4]、0x9F6024 で [0xB399C8] へ書込み）: `new(8)` + `call 0x419A70`（2 文字列ペアのオブジェクト構築）で **{token 'sXboxNone'(0xA621BC), 表示文 'assign a button in the Controls menu'(0xA62194)}** を構築 → ControlsMenu の「未割当」プレースホルダ候補 [推定]。
  - **FUN_009f6040〜0x9F66xx + 0x9F66D0..0x9F69D0**（細分化スタブ）: **0xB399D0[29]** に 1 要素ずつアクション名オブジェクトを構築（{'Forward'(0xA2FA6C), 'sUActnForward'(0xA62854)} 形式、以降 Back/Sldleft...）。
  - **消費者**（0xB399D0 の dword 参照 5 件）: 0x40C652（8 要素走査）、0x575D50/0x575D70（**名前→アクション ID 逆引き** 29 件走査 + strcmp）、0x59BC44（**ControlsMenu 行描画**で [obj] = 表示名をセット）、0x9F609B（書込み）。
  - **登録**: .rdata に**関数ポインタの連続表 0xA2A540..0xA2A6AC+**（0x9F2AE0..0x9F69D0 系 init スタブ、26+ 件）。E8/E9/絶対参照は**全 0 件**（0xA2A6E8 の"参照"は 0x89D325 の `call rel32` と偶然一致する偽陽）→ リンカが **.CRT$XCU 区画を境界で走査**する CRT 初期化方式 [推定-強]。`__initterm`(0x981BC0) の実呼出は 0x981D75/0x981D85（別バンド 0xA2F7B0..0xA2F7C8 = 終了系）のみ [v184_i/j.txt]。
- **付随（未解）**: [0xB399C4]/[0xB399C8] 本体は**読取 0 件（write-only）** → §5-(2)。
- 補足: 同バンドの `FUN_009f3400` は 0xB395xx 系（キーボードキー名 238 件: sKBU→'U' 等）を構築する別テーブル初期化 [確定]。

### O4-6. 0x403960 の方向 (U/UR..) 既定値 — [解明 [確定]]
- **結論**: **方向専用の既定値は存在しない**。`FUN_00403960`(ResetBindings) の書込み全数 [v184_o4b.txt]:
  1. 対象 device 行を 0xFF へクリア（device 3 = 3 行全）: `cmp edx,2/jle` 分岐で 0x1B7E / 0x1B9B / 0x1BB8 行。
  2. keyboard(device 0/3): 29 件 DIK 直書き（0x1B7E=0x11(W)…、0x1B82(Use) はスキップ、0x1B98=0x3F(F5), 0x1B99=0x43(F9), 0x1B9A=0x2C(Z)）— s60_input 表と完全一致。
  3. mouse(device 1/3): 0x1B9F=0(LMB/Use), 0x1BA1=1(RMB/Block), 0x1BA9=2(MMB/TogglePOV)。
  4. gamepad(device 2/3 かつ `[this]&1` 有効時): btn0..btn10（action 9/13/8/5/15/14/7/28/6/4/16）+ **0x1BCA..0x1BD1 = 0x20..0x27**（Quick1..8 = POV セクタ既定）+ **0x1BD5=0x0B**（O4-4）。
- → U/UR.. の「方向既定」は上記 POV キーコード既定（+ キーボードでは Forward/Slide 系アクションから導出）にのみ現れる。値 0x0B の单 byte のみ [未解]（O4-4/§5-(1)）。

---

## 3. s81 残9 の当該分（O3 残1 + O5 残2 + O6 残3。レンダラ/ストリーム系は束C= v183 で処理済のため除外）

### 3-1. O3 残1: 0x6AB420 の vt+0x08（0x6AB48E）GetState 系戻り値 — [解明 [確定] + s60_audio §6-5 を訂正]
- **結論**: vt+0x08 の 2 箇所は **IUnknown::Release（戻り値未使用）**であり GetState ではない。真にテストされる戻り値は **`IMediaControl::Run()` の HRESULT**。
- 完全デコード（スタック整合から引数個数を逆算: saved-edi ペア 0x6AB491(push)/0x6AB4C6(pop) が全 stdcall 呼出しの純移動を 0 に固定 → #2..#5 は this のみ = 0 引数メソッド、#1 のみ this+double=12B pop と強制）[v184_s81a.txt + 手計算]:
  1. QI#1: `IID 0xA78FA4 = 56A868B1 = IMediaControl` → ローカル A（esp+4）。
  2. QI#2: `IID 0xA78F84 = 56A868B2 = IMediaPosition` → ローカル B（esp+8）。
  3. #1 @0x6AB474-0x6AB486: **B = IMediaPosition::put_CurrentPosition(0.0)**（vt+0x20、IDispatch 後 2 番目 = double 引数）= 先頭へシーク（QI#2 成功時のみ無条件）。
  4. #2 @0x6AB48E: **B.Release()**（vt+0x08）。push edi は callee-saved 保存（引数ではない）。
  5. #3 @0x6AB49F: **A = IMediaControl::Run()**（vt+0x1C = IDispatch 後 1 番目、0 引数）→ HRESULT → 0x6AB4A8 `mov edi,eax`。
  6. #4 @0x6AB4B6（`test [0xDC],4` で条件分岐）: **A.Pause()**（vt+0x20 = 2 番目、0 引数）= **bit2 = Run 直後の Pause 要求フラグ**。
  7. #5 @0x6AB4BE: **A.Release()**。
  8. 0x6AB4C4 `test edi,edi / jl` → **Run 成功（HRESULT≥0）時のみ `or [AM+0xDC],2`（bit1 = グラフ起動中）** [確定]。
- IID は Wine `include/control.idl`（IMediaControl 56a868b1 = Run/Pause/Stop/GetState 順、IMediaPosition 56a868b2 = Duration/CurrentPosition 順）で照合 [確定]。
- **訂正**: s60_audio §6-5「vt+0x08 = GetState 系戻り値」→ 誤（Release）。s81 B1「bit2 時の 0x6AB4B6 = put_CurrentPosition(0)」→ 誤（対象は IMediaControl、メソッドは Pause、引数無し）→ §4 訂正。

### 3-2. O5 残2-(a): hypot — [解明 [確定-構造] = 実質未使用（死コード同梱のみ）] + s81 C2 を訂正
- **結論**: **hypot は入口・呼出とも存在しない**。
  - 文字列: 精確 cstr "hypot" = **0 件**。"_hypot" (0xAA67D0) は matherr 名前表スロット 0xB31C14（id 0x18, args 2）への 1 参照のみでコード参照 0 件。
  - dispatcher 型入口（`mov edx, <name-struct> ; jmp __cintrindisp2/__ctrandisp2` パターン）は **fmod/atan2/exp の 3 種のみ**（§3.3）— hypot 用 struct（"\\x06_hypot" 等）は .text に無し。
  - E8/E9 到達路: hypot の SSE 本体は `FUN_00993f39` バンド内に **matherr id 0x18 のエラー経路 3 箇所（0x994920/0x99496A/0x9949BC）として物理存在**するが、外部から当該帯へ入る経路は `E9 0x985B6A→0x993F39`（呼出 0 件の 0x985B30 系）と `E9 0x985BA6→0x993F20`（pow 0x985B70 のみ）のみ → **hypot 本体は到達不能 = リンカが単位で同梱した死コード** [v184_j/k/n.txt]。
- **s81 C2 訂正**: 「0x9860F0 = asin と分析」は**誤り**。0x9860F0 ∈ `FUN_00986058` = **cos**（0x98606D `fcos` + fprem1 周期縮小 + 0x986087/0x986089 で name "cos"@0xB30D50 / 0x9860DE、edx=0x12=id）。併せて **s60_crt 残件「0x9860F0 の正体（hypot 候補）」も cos として解消**。
- 移植示唆: hypot は未使用 → 移植側で hypot 実装は不要 [確定-構造]。

### 3-3. O5 残2-(b): CRT exp/tan/atan/atan2/asin/acos の入口 VA — [解明 [確定] 全 6 件]
- 名前検出法: (1) 入口が `lea ecx,[0xB30Dxx]` で name を __math_exit/__startOneArgErrorHandling(0x991AB7) へ渡す 1 引数系、(2) 薄入口が `mov edx, <長さプレフィックス name-struct> ; jmp __cintrindisp1/2(0x9908FE/0x9908C0) / __ctrandisp1/2(0x990AD1/0x99093B)` で渡す系、の 2 パターンを全走査。
  | 関数 | 入口 VA | 構造・証拠 | ゲーム呼出 |
  |---|---|---|---|
  | **exp** | **0x9866BC** | SSE 可時 `E9→0x996BC0`（SSE 本体）/ 不可時 name-struct `0xB31D0A={03,"exp",1,0x14}` → __ctrandisp1(0x9866FC)/__cintrindisp1(0x986706) [v184_o.txt] | 13 件（0x53868D..0x7DF752） |
  | **tan** | **0x983AF0**（x87 本体 0x983B48） | SSE: `E9→0x9925A0` / x87: __fload_withFB(0x991B88) → 0x983B48（`fptan`+fprem1 縮小+name "tan"@0xB30BA0, edx=0x20） | 12 件（入口 0x983AF0） |
  | **atan** | **0x9870A0**（x87 本体 0x9870F8） | SSE: `E9→0x9973A0` / x87: 0x9870F8（`fld1; fpatan` = atan2(1,x) + name "atan"@0xB30DA0, edx=0x0F） | 9 件 |
  | **atan2** | **0x98598A**（+0x985980 = __ctrandisp2 版） | 薄入口: `mov edx, 0xB31C70={05,"atan2",2,0x10} ; jmp __cintrindisp2(0x9908C0)`（0x985980 側は __ctrandisp2(0x99093B)）。SSE 実装は dispatcher 内（位置は未特定 → §5-(9)） | **24 件**（0x54B0E4..0x7A6721） |
  | **asin** | **0x985870**（x87 本体 0x9858C8） | SSE: `E9→0x993570→0x99358E`（単一 double 入力・√(1−x²) 多項式 SSE2）/ x87: fsqrt((1+x)(1−x)) + `fpatan` = atan2(x,√(1−x²)) + name "asin"@0xB30D30, edx=0x0E [v184_g/misc2.txt] | 11 件 |
  | **acos** | **0x986130**（x87 本体 0x986188） | SSE: `E9→0x995750` / x87: 同式+`fxch` → atan2(√(1−x²),x) + name "acos"@0xB30D60, edx=0x0D | 18 件 |
- **共通 prologue パターン**（0x985870/0x986000/0x986130/0x9870A0/0x983AF0 等）: `[0xbaabdc]==0? → x87` / `stmxcsr&0x1f80==0x1f80 かつ fnstcw==0x27F → SSE (E9)` / 他 → x87 経路（`fst qword[esp] → __fload_withFB → x87 本体`）— MSVC CRT の標準的な 2 経路ディスパッチ [v184_misc2.txt]。
- 付随解明（s60_crt 残件の回収）: **sin = 0x986300**（name "sin"@0xB30D70 refs 0x986389/0x9863DE、E8 69 件 = ゲーム常用）、**cos = 0x986000**（SSE: E9→0x9955A0 / x87 本体 0x986058、E8 98 件）、**fmod = 0x982BFA**（name-struct 0xB30AF0 {04,"fmod"}、E8 63 件）、sqrt = 0x982C30（id 0x05 @FUN_00982c4d）、pow = 0x985B70（E8 23 件、id 0x1D @FUN_00985bcd、SSE 時 E9→0x993F20）。
- **訂正素材**: s60_crt「0x993400 = tan 候補」→ 誤（`__aulldiv`）。「exp core = 0x994A80 帯」→ exp の入口/SSE は 0x9866BC/0x996BC0、0x994A80 帯（FLDL2E@0x994A9A を含む FUN_00993f39）は pow 系 blob（hypot/ldexp/modf の死コードを含む）。「sin/cos dispatch 0x9862C0/0x986680」→ 0x9862C0 ∈ _sscanf 帯・0x986680 ∈ _toupper 帯で不整合（sin=0x986300, cos=0x986058 が name+呼出証拠）→ §5-(5) に親裁定待ちとして計上。

### 3-4. O6 残1: BSA の zlib inflate 呼出 VA とファイル名表圧縮ビット — [解明 [確定] + v183 §11-4 採用]
- **inflate 本体**: **`FUN_007425A0`** — zlib エラー文字列の直接参照で確定: "invalid window size"(0xA826C8)@0x742722、"incorrect header check"(0xA826B0)@0x74276C、"invalid distance too far back"(0xA82568)@0x7435F7（いずれも `mov [edx+0x18], <msg>` = z_stream.msg 設定 + `jmp 0x74379d` エラー尾）。バージョン文字列 "1.2.1" @0xA827A8（エラー帯隣接）。
- **inflate_fast**: **`FUN_00745010`**（"invalid distance too far back" @0x74542E を参照、inflate 本体から 0x743212 の E8 のみで呼出）。
- **呼出サイトは 2 件のみ**（raw E8 全走査）:
  1. **0x42C798** = BSA/BSFile 読込系の圧縮展開経路（FUN_0042c6a0 次点の未ラベル帯。BSA ローダ帯 0x42E000-0x430200 に直接呼出は無く、共有 read 経路側から呼出）→ **BSA compressed 本体展開 = 0x42C798** [確定]。
  2. **0x45068B** = **`FUN_00450550`（TESFile 圧縮レコード解凍）内** → **v183 §11-4 の訂正素材を確認・採用**（callers(0x450550) = 0x450D61/0x450EC3/0x450F88 = TESFile 系 3 件）[v184_f.txt/h.txt]。
- **ファイル名表圧縮のビット対応 → 該当ビット無し（無圧縮）** [確定]: `FUN_0042EE80` はマジック `cmp [edi],0x415342`("BSA") + `cmp [esi+0x158],0x67`（**version ≤ 103=v103 保証**）→ v104 系 filename-compression ビット(0x100)は存在しない前提で実装:
  - flags `[[esi+0x160] bit0]` = ディレクトリ名同梱 → ブロック read（0x42F035-0x42F083: new([+0x16c]) → [+0x198]、offsets new(4×count) → [+0x19c]）。
  - `bit1` = ファイル名同梱（0x42F089 `shr eax,1; test al,1`）→ new([+0x170]) → [+0x1a0] + offsets [+0x1a4]、エントリ読込ループ 0x42F104-0x42F167: **1 バイト長 + 生文字列を直接 read（0x42c8e0）**、展開/復号ループは無し → **名前は無圧縮** [v184_h.txt]。
- 付随: `FUN_0042BD70` は「名前読み込み要否」述語（[0xB04460]/[0xB04430] グローバル + flags>>7 &1 + [0xB04438] モードを見て 0/1 を返す）。

### 3-5. O6 残2: CRT ラッパ 5 件の厳密同定 — [解明 [確定] 全 5 件（Ghidra FID 名 + 本体挙動で確定）]
  | VA | 名称 | 証拠 |
  |---|---|---|
  | **0x982D71** | **`__access`** | FID_conflict:__access。`push mode / push path → call 0x982CEA(__waccess_s) ; neg/sbb` で 0/-1 返す（呼出 14、FUN_6AD030 0x6AD491 の `cmp eax,-1` パスと一致） |
  | **0x983754** | **`__stat64i32`** | FID 名そのまま。filetime 取得はこの関数内（0x983803 で FindFirstFile IAT 0xA2812C 呼出、errno 0x16 セット経路併設） |
  | **0x982440** | **`_fopen`** | `push 0x40 / push mode / push path → call __fsopen(0x98237C)` の 3 行ラッパ（呼出 12） |
  | **0x9821F5** | **`_fgets`** | FID 名 + 本体は [ebp+8]/[+0xC]/[+0x10] を使う行読込（__SEH_prolog、errno 0x16 パス） |
  | **0x9840E0** | **`_strstr`** | FID 名 + 2 文字高速strstr 本体（`mov dl,[ecx]; mov dh,[ecx+1]` ループ、呼出 65） |
- s60_io §8-2 の [推定-強] を名称確定に更新（§4 訂正の一部）。

### 3-6. O6 残3: mod BSA 探索の実行順 — [解明 [確定-構造] = プラグイン単位・起動期の一括処理内で逐次]
- **文字列**: 0xA36134 = **"\\*.bsa"**（dword+word で 6 バイト書込に使用）、0xA3613C = **".esp"**、0xA36130/0xA36132 = ", "（区切り）。
- **上位入口（最重要）**: **`0x42F610`** — prologue `sub esp,0x448` + `memset(0xB338E8, 0, 0x48)` を持つ関数先頭で、**起動期に 1 回だけ** `FUN_0040d4d0`（メイン初期化/ループ）の 0x40E75E から呼出（`call 0x42F610`、E8/E9 とも 1 件のみ）[v184_misc.txt]。s60_io の「master archive ロード 0x42F610」と同一。
- **処理順序**（FUN_0042f4c0 帯の逆アセンブル、Ghidra は 0x42F4C0/0x42F610 を 1 関数にマージしている = §5-(10)）:
  1. リスト走査（改行/空白/カンマ区切り。区切りトークナイザ = 0x98349C → 0x98347E、separator 文 "," ）で **1 エントリ（プラグイン名 ".esp"）ずつ**処理。
  2. エントリ内 `strstr(token, ".esp")`（0x42F7E2）でヒットした 4 バイトを **"\\*.bsa\\0"（dword@0xA36134 + word@0xA36138 = 6 バイト）で置換** → パターン "X\\*.bsa"。
  3. **`__findfirst64i32`(0x9844EC) / `__findnext64i32`(0x98461C) ループ**（0x42F86C-0x42F918）で全一致ファイル名を ", " 区切りで蓄積 → ハンドル解放 0x98474B（1 引数 cdecl、_findclose 相当 [推定]）。
  4. 蓄積リストを ',' 区切りで再トークナイズし（0x42F933-0x42F95E）、**各 ".bsa" トークンごとに `call 0x42F4C0`（0x42F95F、アーカイブ個別ロード）**へ渡す。
  5. 個別ロード側 `FUN_0042F4C0`(0x42F4C0): FileFinder マネージャ [0xB33A04] vtable でパス解決 → `new(0x280)` → **`FUN_0042EE80`（BSA ローダ）**。
- → **「起動時に一括走査」ではなく「起動期の 1 回のロード処理の中で、プラグイン 1 件ごとに X\\*.bsa を列挙して 1 アーカイブずつロード」** [確定-構造]。実行タイミングは Plugins.txt 走査/マスターアーカイブ読込と同じ起動フェーズ [推定]（呼出元 0x40E75E が起動初期化帯）。

---

## 4. 本成果による既存記述の訂正一覧（親セッションの docs 反映判断用）

| # | 対象記述 | 旧 | 新（本調査） | 根拠 |
|---|---|---|---|---|
| 1 | s60_audio §6-5 / O3 残件 | `0x6AB420 vt+0x08(0x6AB48E)` = GetState 系戻り値 | **vt+0x08 = Release（B=IMediaPosition, A=IMediaControl、戻り値未使用）**。真のテストは 0x6AB49F の **IMediaControl::Run() HRESULT**（成功時 [0xDC] bit1 設定） | §3.1（IID 照合 + スタック整合逆算） |
| 2 | s81 B1 | bit2 時 0x6AB4B6 = IMediaPosition::put_CurrentPosition(0) | **bit2 時 0x6AB4B6 = IMediaControl::Pause()（0 引数）**。put_CurrentPosition(0.0) は 0x6AB474-0x6AB486（QI 成功時・無条件） | §3.1 |
| 3 | s60_audio §6-1 | AM+0xDC bit2 = 「一時停止/再開系」[推定] | **Run 直後の Pause 要求フラグ**（設定箇所の実使用は 0x6AB4A1 の 1 箇所のみ）[確定-構造] | §3.1 + F2-2 |
| 4 | s60_audio §1.4 | 「音響エンジン内に type&0x20 の単独テストは存在しない」 | **5 箇所存在**（0x6A9BBB/0x6AB5DD/0x6AB750/0x6AE7A1/0x6AF4D9） | F2-4 |
| 5 | s56_audio §5-5 | `[0xA785E8]=1e-5` は 0x6B7340 内 | **FUN_6B6F20（0x6B6FA6）の 1 箇所のみ**（他セクションの参照は 0 件） | F2-5 |
| 6 | s81 C2 | 「0x9860F0 = asin（atan2(√(1−x²),x)）」 | **0x9860F0 = cos 本体**（fcos+fprem1+name "cos"）。ラベル入れ替え: asin=0x9858C8 本体/入口0x985870、acos=0x986188/入口0x986130、0x995768 帯=acos SSE | §3.2/§3.3 |
| 7 | s60_crt §残件 | 0x986300 = 「sin 変種か cos か未確定の別入口」/ 0x9860F0 = hypot 候補 / 0x993400 = tan 候補 / exp core=0x994A80 / sin-cos dispatch 0x9862C0・0x986680 | **0x986300=sin 入口（69 呼出）/ 0x9860F0=cos 本体 / 0x993400=__aulldiv / exp=入口0x9866BC・SSE0x996BC0（0x994A80 帯は pow 系 blob）/ 0x9862C0 は _sscanf 帯・0x986680 は _toupper 帯で不整合 → sin=0x986300, cos=0x986000（要親裁定 → §5-(5)）** | §3.3 |
| 8 | s60_input §60.7-1 | 「ID 30 (grave) の直接 push 呼出が見つからない」 | **0x583731 `push 0x1E` を発見**（FUN_005821f0 → 0x403520(action=30, mode=1)） | O4-1 |
| 9 | s60_input §60.2 想定 | 「+0x1BD5.. = ゲームパッド 8 方向バインディング等」 | **不成立**。+0x1BD5 は write-only 1 バイト（既定 0x0B）。8 方向は keycode 0x20..0x27 + POV/4500 変換で動的解決。0xB02B98 方向名テーブルも dword 参照 0 件の死データ | O4-4 |
| 10 | s60_io §8-2 | CRT ラッパ 5 件 [推定-強] | **FID 名で確定**: __access / __stat64i32 / _fopen / _fgets / _strstr | §3-5 |
| 11 | s60_io §8-1（補強） | inflate 呼出サイト [未確定] | **inflate=0x7425A0 / inflate_fast=0x745010 / 呼出は 0x42C798(BSA 系) と 0x45068B(TESFile)**。ファイル名表圧縮ビットは**存在しない（v103 は長さ+生文字列で無圧縮）**。v183 §11-4 の FUN_00450550 候補は**確認・採用** | §3-4 |
| 12 | s60_input §60.7-3（mode） | [推定] 0/1/2/3 = held/edge/changed | **0=held / 1=press-edge / 2=release-edge / 3=changed（双方向）で確定**、特殊 ID 29/30/31 は mode 無視 | O4-3 |

---

## 5. 新規残点（本調査で発覚・親セッションで起票判断）

1. **inputMgr +0x1BD5 の書込値 `0x0B` の意味** — ResetBindings が gamepad 初期化時にのみ書くが、.text 内に読み取り参照 0 件（disp32 全走査 D5 1B 00 00 = 1 hit）。オブジェクト末尾 3 バイト内の write-only フィールド。
2. **[0xB399C4] / [0xB399C8] グローバルの読取箇所** — FUN_009f5940 が {'sXboxNone','assign a button in the Controls menu'} オブジェクトを書くが、dword 参照は書込み 1 件のみ（配列ベースの間接読取も未検出）。「未割当表示」用途は [推定]。
3. **bAllowConsole の実行時ゲート読取箇所** — Setting 値スロット 0xB135C8 のコード参照は (a) 0x57E6AA の float 一括コピー、(b) getter thunk 0x9BEF10（`mov ecx,0xB135C8→jmp 0x403BC0`、E8 呼出 0 件、ptr は 0xAE8560 表のみ・表の消費者未追跡）、(c) Setting 登録 0x9FB5A1 の 3 種のみで、コンソール経路（O4-1）からの直接ゲートは未検出。※0xB135C8 の既定値が 0.25f 相当（0x3E800000）と解釈できない点も併せて要調査。
4. **CRT 静的初期化表 0xA2A540..0xA2A6AC+ の走査主体** — ポインタ表自体は E8/E9/絶対参照すべて 0 件（`.CRT$XCU` 区画走査で実行 [推定-強]）。実行主体（start/end 界）の突合は未了（`__initterm` の既知呼出は別バンド 0xA2F7B0..0xA2F7C8）。
5. **sin/cos の「dispatch」値の不整合** — s60_crt/s81 C1 の 0x9862C0/0x986680/0x996998/0x996BD8 と本調査の name+呼出証拠（sin=0x986300(69)・cos=0x986000(98)→SSE0x9955A0）が不一致。Ghidra FID では 0x98629E=_sscanf / 0x98664D=_toupper。親セッションでの裁定・docs 訂正が必要。
6. **FUN_00985A00（E8 19 件・float ラッパ 0x4842F0 経由含む）と FUN_00987C00（3 件）の正体** — 桁抽出型 SSE（指数範囲 [1023,1074] チェック、|x|<1 → ±0、NaN → ___libm_error_support(0x3ED)）で floor/trunc 系 [推定] だが未確定。s60_crt「0x4842F0 = truncf」との関係も要突合（正の wrapper は floorf 的構造）。
7. **FUN_00985870 の SSE 先 0x993570 の実装内容の完全検証** — 単一 double 入力・√(1−x²) 多項式から asin SSE と確定寄りだが、0x993B1B 帯の fpatan/fldpi 象限コード（Ghidra は同一関数にマージ）の帰属を精査する余地。
8. **atan2 の SSE 実装位置** — 入口 0x98598A/0x985980 は name-struct を __cintrindisp2/__ctrandisp2 へ渡すのみで、dispatcher 内の atan2 本体 VA が未特定（asin/acos/atan/tan/exp は入口から E9 直結で特定済み）。
9. **到達不能な CRT 死コード群** — hypot(SSE, id0x18 @0x994920 帯) / ldexp(id0x19) / modf(id0x1C) の本体が FUN_00993f39 blob 内に存在するが入口・到達路なし。加えて 0x985B30（E9→exp 系 0x993F39、呼出 0 件）。移植時は再実装不要の判断材料（hypot は §3.2 で未使用確定）。
10. **Ghidra 関数境界のマージ 2 件** — (a) FUN_0042f4c0 は実体 2 関数（0x42F4C0 アーカイブ個別ロード + 0x42F610 起動期リスト処理）をマージ（call 0x42F610 が唯一の外部種、台帳の 0x42F4C0 呼出は再帰に見えるのは境界ずれ）、(b) CRT math 帯の薄入口複数（0x985980/0x985870 内部等）が未ラベル。名前台帳（named_funcs.txt）利用時の注意点。

## 6. 証跡ファイル一覧（%TEMP%\opencode\ = C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\）

| ファイル | 内容 |
|---|---|
| v184_f2.txt / v184_f2b.txt | F2-1〜7（A6 分岐・6AD030bit 全数・FPU カーブ厳密・bit0x20テスト 5 箇所・0xA785E8 参照・E8 2 サイト・6AC210 呼出 6 件） |
| v184_o4a.txt〜v184_o4e.txt | 0x403520/0x4032D0/0x403960 全逆断、0x403520 呼出 95 件の push 解析、0x403490/0x403380/0x402FC0/0x403070、poll 0x4046A0、+0x1BD5 disp スキャン、0xB02B98 参照、0x9F6000 文字列・初期化表、bAllowConsole refs |
| v184_s81a.txt | FUN_006AB420 フル（GetState=Release 訂正）+ CRT 数学名 cstr/xref 走査 + CRT 帯 E8 一覧 |
| v184_f.txt / v184_h.txt | zlib 文字列→inflate 本体特定、FUN_00450550 検証、CRT ラッパ 5 件 FID 名、mod BSA 文字列/呼出、bAllowConsole thunk、初期化ポインタ表走査 |
| v184_g.txt | CRT 名台帳 0x980000-0x99B000、asin/acos/atan/tan 本体 head、fpatan/fldl2e スキャン、E8 一覧 |
| v184_i.txt / v184_j.txt / v184_k.txt / v184_l.txt / v184_m.txt / v184_n.txt / v184_o.txt | exp 候補・BSA flags・0x42C798・mod BSA 呼出連鎖・initterm・E9 走査・matherr id スキャン・name-struct 入口（fmod/atan2/exp） |
| v184_misc.txt / v184_misc2.txt | 0x42F610 入口検証、tan/asin/acos/atan/cos 各 wrapper のリニア逆断 |
| （外部照合） | Wine `include/control.idl`（IID_IMediaControl/IMediaPosition の UUID とメソッド順）— Context7/webfetch で取得 |

- スクリプトは全て `%TEMP%\opencode\v184_*.py`（re_util 共用: raw E8 走査 + capstone linear）。ESM 再スキャンは未実施。cache/{named_funcs,call_targets} を読み取りのみ使用。

（完 — 実施 19 / 解明 18 + 部分 1。TODO/docs 未編集）
