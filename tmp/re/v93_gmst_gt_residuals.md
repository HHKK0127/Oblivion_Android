# V93: GMST / GT 残点束3点（2026-10-25）

ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B、pefile なし・手動パース）。
セクション定数（実測）: .text = 0x400C00（VA 0x401000 / raw 0x400 / rsz 0x626E00）、.rdata = 0x400E00（VA 0xA28000 / raw 0x627200 / rsz 0xD9A00）、.data = 0x401400（VA 0xB02000 / raw 0x700C00 / rsz 0x30C00）。
台帳: `%TEMP%\opencode\func_list_new.txt`（Ghidra FunctionManager 21,597 件）。

---

## §a. `sSpecialization` 登録有無 → GMST 377 vs 378 の数値

### a-1. 【v81 の前提に誤り】v81 が走査した 2 つの VA は別物

| 項目 | v81 記載 | 実測（file offset × 定数 0x400E00） | v81 VA の実バイト列 |
|---|---|---|---|
| 「byte 0x73 長文字列の一部」 | 0xA5D12D | **0xA5D55D**（file 0x65C75D、含む C 文字列 = `sClassSpecialization` @0xA5D558、直前 byte = 0x73='s'） | `b'- there are limits to what the b'` |
| 「独立 null 終端」 | 0xA5DD64 | **0xA5E194**（file 0x65D394、`sSpecialization\0` 独立） | `b'est Items from your Inventory.\x00'` |

- v81 の 2 VA は file offset 0x65C32D / 0x65CF64 に相当し、どちらも無関係な英文。**v81 の .rdata 定数は 0x4009D0（正しくは 0x400E00、差 0x430）**。
- したがって v81 の「**この VA を指す絶対ポインタは全セクション 0 件**」は誤 VA に対する走査結果であり、前提自体が成立しない。

### a-2. 直接参照は実在する（逆アセンブルで確定）

`.text` 内の abs32 は **0xA5E194 のみ 1 件 = VA 0x9F0D56**（file 全体でも 1 件）。0xA5D55D（部分文字列）は 0 件。

```
0x009F0D50: 68 a8e1a500   push 0xa5e1a8   ; "You will gain +10 in all specialization skills. ..."
0x009F0D55: 68 94e1a500   push 0xa5e194   ; "sSpecialization"   <-- 直接 push
0x009F0D5A: b9 4086b300   mov ecx, 0xb38640
0x009F0D5F: e8 0c8da2ff   call 0x419A70   ; FUN_00419a70 = Setting 初期化 + FUN_00412d30(name,this) = map.insert
0x009F0D64: 68 a013a200   push 0xa213a0
0x009F0D69: e8 4612f9ff   call 0x981FB4   ; atexit 登録
0x009F0D6E: 59            pop ecx
0x009F0D6F: c3            ret
```

- 0xA213A0 = `b9 4086b300 (mov ecx, 0xb38640) / e9 ... (jmp FUN_004055e0)` = **デストラクタ・シンクロン**（FUN_004055e0 = v81 §3.1 の erase(name)+`'S'` 判定 free）。つまり登録ブロックは **atexit で SLOT 単位の後始末も登録している**。
- FUN_00419a70 本体（decompiled L16471-16502）: `param_1[1]=name; *param_1=help; 重複検査 FUN_0055e000 → FUN_00412d30(param_1[1], param_1)`。
- この 0x20 バイト・ブロックは **.rdata の関数ポインタ表スロット 0xA2A1BC = 0x9F0D50** に格納され、§c の起動時ループから呼ばれる（→ 実行時に到達する）。

### a-3. ESM 382 名前 × EXE の全数再計数（本タスクの決定打）

ESM 382 件（`gmst_group()` errors=0、unique=382）に対し:

| 基準 | 件数 | 未一致 |
|---|---|---|
| (1) EXE 内に `name + NUL` が在る（文字列一致） | **378** | 4 件 = `fRegionGenTexPlacedNotMatch` / `fRegionGenTexPlacedMatch` / `fRegionGenTexGenNotMatch` / `fRegionGenTexGenMatch` |
| (2) (1) の文字列 VA に **.text 内 abs32 参照**がある（直接 push / mov / lea 系） | **378** | **0 件** |

- (1) は s50_gmst.md の「名前一致 378/382・未登録 4 件 = fRegionGenTex*」を**完全再現**。
- (2) は (1) と**同一集合**（差 = 0）。`sSpecialization` は候補 2 VA のうち独立側 0xA5E194 にのみ参照があり 0x9F0D56 がヒット。
- 注: 「直前バイトが NUL」で独立文字列を判定すると 355 に落ちる（.rdata の詰め込みパディング `...\x00@\x1cF` 等のゴミバイトのため）。正しくは `name+NUL` の出現 + 含む文字列同一性で判定する。

**[判定: 登録あり → 378 が正。377 は v81 の VA 誤り（定数 0x430 差）に起因する過小計数であり、リポジトリ基準の再計数では両基準とも 378・差 0。＝ 378 [確定]]**

---

## §b. `FUN_0040c350` 唯一の呼出し元（無名 gap 関数）の caller と到達時期

### b-1. gap 関数の実体 = **WinMain（0x40DF80）**

- 台帳: `0x40d4d0` → 次が `0x40f640`。**[0x40DF74, 0x40F640) に開始点 0 件**（v81 どおり）。
- 実際の境界（線形逆アセンブル 1,536 命令が 0x40F63F まで連続デコード）:
  - `0x40DF73: c3 ret` → `0x40DF74..0x40DF7F: int3×12` → **開始 = 0x40DF80**
  - **終端 = 0x40F637 `c2 10 00  ret 0x10`**（stdcall 4 引数 = 16 B クリーンアップ）→ `0x40F638..0x40F63F int3`
  - サイズ = **5,816 B**。gap 内の `ret` は 1 箇所のみ（byte スキャン c3=1、線形 decode の ret-like = 0x40F637 のみ）。
- WinMain である証拠:
  - 先頭: `sub esp,0x6a8` + `__security_cookie`、`mov ebx,[esp+0x6b0]` = **引数2 (hPrevInstance)**
  - `sprintf(buf, ".\\%s", [0xb02c84])` → `call [0xA280E4]` = **KERNEL32!GetPrivateProfileIntA("General","bUseMyGamesDirectory",1,buf)**
  - `0x40F130` = **Win32 メッセージポンプ本体**（`call edi/ebx/ebp` + `call [0xa282b0]` + `Sleep(0x32)` 系）
  - 末尾 `ret 0x10`（`int __stdcall WinMain(HINSTANCE,HINSTANCE,LPSTR,int)` の引数解放）

### b-2. トップからの呼び出し（唯一）

- **E8 → 0x40DF80 は 1 件のみ = 0x98780E**。E9 → 0 件、abs32（全セクション）→ 0 件。
- 0x98780E の呼び出し形:
  ```
  0x987805: 51        push ecx        ; nShowCmd = 10 (SW_SHOWDEFAULT) / STARTUPINFO.wShowWindow
  0x987806: 50        push eax        ; lpCmdLine (call 0x997725)
  0x987807: 6a00      push 0          ; hPrevInstance
  0x987809: 6800004000 push 0x400000  ; hInstance = ImageBase
  0x98780E: e86d67a8ff call 0x40DF80  ; WinMain(...)
  ```
- 0x98780E の含入関数 = **0x98769C .. 0x987886（490 B）**（台帳は `0x987390` → `0x987886` の 1 関数で、**0x98769C は台帳外 = 無名 gap**。dec ヘッダも `// _qsort @ 0x987390` のみで 0x98769C は無い）。
- この関数が **WinMainCRTStartup** である根拠:
  - `GetStartupInfoA` (IAT 0xA281D0) / `HeapAlloc(GetProcessHeap(),0,0x94)`（0x94 = sizeof(STARTUPINFOA)）/ `GetVersionExA` (0xA281C8) / `GetCommandLineA` (0xA281C4)
  - 失敗系 `_amsg_exit(8)` / `_amsg_exit(9)`（`call 0x9979f5` / `call 0x997782` → `push 8|9; call 0x981B4F`）
  - **`push ebx; call 0x981C6B`（§c の初期化テーブル走査）→ `call 0x997725`（lpCmdLine）→ WinMain 呼び出し → `mov [ebp-0x1c],eax` → `call 0x98C5E5`(__SEH_epilog) → `ret`（exit(WinMain 戻り値)）**

### b-3. `call 0x40C350` の所在 = 終了経路

分岐全走査（線形 decode 由来、branch 135 本）で `[0x40F440, 0x40F640)` へ入る**外部分岐は 4 本のみ**:

| 元 | 分岐 | 先 |
|---|---|---|
| 0x40F27D | `jne 0x40F488` | **終了経路の入口（唯一）** |
| 0x40E2C6 | `jmp 0x40F61D` | 早期 return（teardown 素通り） |
| 0x40E37B | `je 0x40F61D` | 早期 return（teardown 素通り） |
| 0x40E3AC | `jmp 0x40F61D` | 早期 return（teardown 素通り） |

- 入口直前（ループ退出判定）: `0x40F275 mov eax,[0xB33398] / 0x40F27A cmp byte [eax],0 / 0x40F27D jne 0x40F488`、分岐不成立側は `cmp byte [eax+1],0 / je 0x40F130`（メッセージループへ戻る）。
- 経路: `0x40F488 → 0x40F49A … → 0x40F574 → 0x40F580 call 0x40C350 → 0x40F585 … → 0x40F618 call 0x747be0 → 0x40F61D epilog → 0x40F637 ret 0x10`。
- → `FUN_0040c350`（動的 Setting* 9 配列 350 スロットの一括 erase）は **WinMain が正常終了するときだけ実行される**。

**[判定: 到達時期 = プロセス終了時（メッセージループ退出 → WinMain の shutdown 経路）に 1 回。起動時 1 回でも ESM リロード毎でもない。caller は WinMainCRTStartup@0x98769C。＝ [確定]]**

旁証: 登録ブロック（§a-2）は `call 0x981FB4` で destructor thunk `0xA213xx` を atexit 登録しているため、SLOT 単位の `FUN_004055e0` も終了時に走る。実行中の ESM リロードでこれらが消化されることはない（v81 §3.5 の「377 系は保持される」と整合）。

---

## §c. CRT 初期化テーブルの走査側コード特定

### c-1. テーブルの実体（v81 の 299 エントリを再確認しつつ外周を拡張）

- v81 の run `[0xA2ED04, 0xA2F1B0)` = **1,196 B = 299 × 4 B**、全 299 件が `.text` 0xA10000..0xA12000 を指す（min 0xA10000 / max 0xA12000）。再現 ✓
- ただしこれは**外側の巨大配列の一部**:
  - **実行時テーブル = `[0xA283D0, 0xA2F77C)` = 7,403 スロット（29,612 B）**
  - 内訳（cls 集計）: `NULL 24 / .text 他 4,459 / バルク登録モジュール 0x9D9D00..0x9FD000 2,621 / シンクロン 0xA10000..0xA12020 299`
  - **「299」は 0xA10000 系シンクロンを指すスロットの全数と一致**し、それらが連続して `[0xA2ED04, 0xA2F1B0)` に並ぶ（＝299 という数値自体は正しい）

### c-2. 走査コード = **0x981C6B のインラインループ 0x981CC5..0x981CD4**

```
0x00981CB4: bed083a200   mov esi, 0xa283d0     ; begin（imm32 @0x981CB5）
0x00981CB9: 8bc6         mov eax, esi
0x00981CBB: bf7cf7a200   mov edi, 0xa2f77c     ; end  （imm32 @0x981CBB）
0x00981CC0: 3bc7         cmp eax, edi
0x00981CC2: 59           pop ecx
0x00981CC3: 730f         jae 0x981cd4
0x00981CC5: 8b06         mov eax, dword ptr [esi]
0x00981CC7: 85c0         test eax, eax
0x00981CC9: 7402         je  0x981ccd
0x00981CCB: ffd0         call eax             ; 各初期化関数
0x00981CCD: 83c604       add esi, 4
0x00981CD0: 3bf7         cmp esi, edi
0x00981CD2: 72f1         jb  0x981cc5
0x00981CD4: 833d18bcba0000 cmp dword ptr [0xbabc18], 0
```

- 関数範囲 = **[0x981C6B, 0x981CFD) = 146 B**（`0x981C6A: c3 ret` の直後開始、末尾 `0x981CFA xor eax,eax / 0x981CFC c3 ret`）。**台帳 21,597 件に開始点なし（無名 gap 関数）**。
- **v81 が abs32 を 0 件と判定した理由**: 参照は**外周境界 2 点だけ**にある。run 端点 (0xA2ED04 / 0xA2F1B0)・中央・末尾直後・`va−0x401000` 相対形をどれだけ試しても、そのどれもコードから参照されていない（配列の一部は「外側の begin/end でまとめて歩く」だけ）。
  - 全セクションで run `[0xA2ED04,0xA2F1B0)` を指す abs32 = **.bind の 1 件（0xBD3765 → 0xA2F04F、非整列のデータ）のみ、.text は 0 件**（v81 の「0 件」を再現）
  - .text 全体で `[0xA2D000, 0xA2F780)` を指す 4 バイト列 = 24 件。台帳開始点からの線形復元で各ヒット位置を含む命令を特定し operand 一致を検証した結果、**成立は 0x981CBC（`mov edi, 0xa2f77c` @0x981CBB）の 1 件のみ、残 23 件は全数 drift（opcode バイト＋rel32 下位バイト等の偶然一致）**と確定（例: 0x4202EA = `e8 f1 a2 00` = `call 0x42a5e0` の先頭 4 バイト、0xA11F0C = `push 0xa27a10` の末尾 + `call` 先頭）
  - `call $+5` ストンク、delta スキャン（`v+next_ip` / `next_ip−v`）も候補はすべて偽陽性（`push 0x4000` 等）で棄却
- 他の走査器は 299 ランに届かない:
  - `0x981BD8`（begin/end を引数に取る `__initterm` 相当、`xor eax,eax` + `call ecx` ループ）は **0x981C9D から [0xA2F780, 0xA2F7A4) の 6 スロットのみ**
  - `0x981BC0`（開始 = EAX、終端 = スタック）は 0x981D75 / 0x981D85 から **[0xA2F7B0,0xA2F7BC) と [0xA2F7C0,0xA2F7C8) のみ**

### c-3. 到達時期

- **E8 → 0x981C6B は 1 件のみ = 0x9877E1**（WinMainCRTStartup 内、WinMain 呼び出し 0x98780E の 45 B 手前 = 0x2D）。abs32 / E9 → 0 件。big array 自体にも 0x981C6B は含まれない（自己再帰なし）。
- `0x981C6B` の先行は `0x981C6A: ret`（落下不可）→ 単独関数。

**[判定: 299 エントリを回すのは「0x981C6B（無名 gap 関数、台帳外）の 0x981CC5..0x981CD4、走査範囲 [0xA283D0, 0xA2F77C)」。WinMainCRTStartup(0x98769C) が起動時に 1 回だけ呼ぶ。＝ [確定]]**

---

## 補足（今回判明した副次事項）

1. **台帳外 gap 関数 3 件を同定**（いずれも 21,597 件に開始点なし）:
   - `0x40DF80` = **WinMain**（5,816 B、唯一の E8 caller = 0x98780E）
   - `0x98769C` = **WinMainCRTStartup**（490 B、外部 caller なし = 間接転移）
   - `0x981C6B` = **CRT 初期化テーブル走査関数**（146 B、唯一の E8 caller = 0x9877E1）
2. **PE AddressOfEntryPoint = 0x7AF2ED → VA 0xBAF2ED = `.bind`+0x2ED**。これは `0xBB1204` から 0xD9 dword をチェーン XOR（初鍵 0x72C99B67: `dec = src[i] ^ src[i-1]`）でデコードし、kernel32!GetModuleHandleA / GetProcAddress の IAT アドレス（0xA281A4 / 0xA2811C）と `"kernel32.dll"` 文字列を保持する**難読化スタブ**。デコード結果にも 0x98769C 等の絶対アドレスは無く、CRT 起動関数への転移は計算経由（E8/E9/imm32 では検出不能）。
3. `0x98C5A0` = `__SEH_prolog` / `0x98C5E5` = `__SEH_epilog`（`push size; push scope; call` 形）。`0x981FB4` = atexit ラッパ（`call 0x981F78` の真偽を `!x-1` に変換、戻り値 0=成功）。
4. `0x981CFD`（3 引数、SEH prolog で ebp 生成）は WinMain 復帰後に呼ばれる `0x981DCB/0x981DDC/0x981DED/0x981DFC` の 4 ラッパ経由で **onexit テーブル逆走 + 静的 dtor 配列 [0xA2F7B0,0xA2F7BC) / [0xA2F7C0,0xA2F7C8) の実行**を担う（＝atexit 登録された GMST SLOT destructor の実行者）。

---

## 未解決の残点

1. **`0x98769C`（WinMainCRTStartup）への制御移行経路**: `.bind` スタブの最終ジャンプ先が絶対バイト列で特定できない（計算経由と推定）。**[未特定]**
2. **`0x981C6B` の引数と周辺 callback**: 第1引数（ebx）の意味、`[0xAA3E84]` callback（検証後に呼ぶ）、`[0xBABC18](0,2,0)` の正体。**[未特定]**
3. **WinMain の早期 return 経路 3 本**（0x40E2C6 / 0x40E37B / 0x40E3AC → 0x40F61D、teardown を素通り）の成立条件。**[未特定]**
4. **v81 が .rdata 定数を 0x4009D0 とした原因**（ハーネス由来か手動計算か）は未特定。本タスクでは結果（誤 VA）のみを確定。**[残]**
5. §b の「1 回」は静的解析上の経路証明。実機での到達（プロセス終了以外に到達しないこと）の動的確認は未実施。**[残]**

---

## 証跡（すべて `%TEMP%\opencode\`、`tmp/re/` は成果物 1 ファイルのみ）

| スクリプト | 内容 |
|---|---|
| `v93_a_scan.py` | `sSpecialization` の file offset / VA / abs32 全セクション走査、v81 VA の実体照合 |
| `v93_a_step2.py` | 0x9F0D56 周辺逆アセンブル、'Specialization' 4 箇所の同定、dec 検索 |
| `v93_a_step3.py` | E8→0x419A70 全 1,312 サイト・クラスタ化、台帳 bisect、文字列表 |
| `v93_a_step4.py` | 台帳 0x9E0000..0x9F8000 列挙、.rdata 参照、E8 入り口走査 |
| `v93_step5.py` | CRT run 実体（299×4B）、.text abs32 スキャン、0x9F0C00 系ブロック逆アセンブル |
| `v93_step6.py` | push imm32 → バルクモジュール、0x9F0D50 の abs32（.rdata 0xA2A1BC 発見）、run/window delta スキャン |
| `v93_step7.py` | .rdata/.data のバルクポインタ全走査（2,629 件）、0xA2A1BC 周辺ダンプ、delta スキャン |
| `v93_step8.py` | run 外周の u32 クラス連続化、全セクション run abs32、`call $+5` ストンク走査 |
| `v93_step9.py` | .rdata コードポインタ run 869 本の抽出、配列 A/B の .text 参照 |
| `v93_step10.py` | `FF 10` + `add reg,4` パターン、[0xA2D000,0xA2F780) imm32、0xA10000/0xA10EB0 逆アセンブル |
| `v93_step11.py` | 0xA2F77C / 0xA2E800 / 0xA2D1E8 等の参照箇所逆アセンブル（0x981CB4 発見） |
| `v93_step12.py` | **walker 関数同定 + 配列 7,403 スロットの class 分布** |
| `v93_step13.py` | (b) E8→0x40C350 = 0x40F580、gap 先頭逆アセンブル |
| `v93_step14.py` | gap 内 ret 数 / E8 入り口 / 全セクション abs32 |
| `v93_step15.py` | caller site 0x98780E の台帳・dec 帰属と逆アセンブル |
| `v93_step16.py` | **PE import 解析（IAT 同定）**、E8→0x987390/0x981C6B/0x40DF80/0x981BD8/0x981BC0 |
| `v93_step17.py` | 0x987390 線形デコード（ret 9 本）、WinMain 末尾、gap 全体の branch 135 本抽出 |
| `v93_step18.py` | **gap の CFG: [0x40F440,0x40F640) へ入る外部分岐 4 本 + 線形 0x40F240..0x40F640** |
| `v93_step19.py` | 0x987620..0x9877C0 線形（WinMainCRTStartup 同定）、WinMain tail、abs32 refs |
| `v93_step20.py` | 0x981CFD..0x981E20 線形、配列メンバシップ、IAT 解決、dec `b33398` 検索 |
| `v93_step21.py` | PE entry 読取、0x981FB4 / 0xA21360（destructor thunk）逆アセンブル |
| `v93_step22.py` | PE ヘッダ生ダンプ、0x98C5A0/0x981F78 逆アセンブル |
| `v93_step23.py` | ESM 382 × EXE 初回計数（独立 NUL 判定 = 355、判定基準の誤りを検出） |
| `v93_step24.py` | **ESM 382 全数: 文字列一致 378 / .text abs32 参照 378 / 差 0** |
| `v93_step25.py` | 0x40F130（メッセージポンプ）確認、サイズ集計 |
| `v93_final_verify.py` | **主張の再現チェック（§a 378/378、§b 呼出しグラフ、§c 配列分布・abs32・24 件 operand 検証）一括** |

`v93_final_verify.py` 実行結果（要点）:

```
[a] records=382 errs=0 / (1) name+NUL present 378 (missing=fRegionGenTex* 4) / (2) .text abs32 ref 378 (no-ref [])
    sSpecialization cands ['0xa5d55d','0xa5e194'] / abs32(0xA5E194) [('.text','0x9f0d56')]
[b] E8->0x40C350 ['0x40f580'] / E8->0x40DF80 ['0x98780e'] / E9,abs32(0x40DF80) []
    ledger 0x40DF80/0x98769C/0x981C6B = False/False/False
    E8->0x98769C [] / E9 ['0x987881'](内部) / E8->0x981C6B ['0x9877e1'] / abs32(0x981C6B) []
[c] array slots=7403 {NULL:24, text:4459, bulk:2621, sync0xA10:299}
    v81 run 299 entries all in 0xA10xxx: True
    abs32(0xA283D0)=[('.text','0x981cb5')] abs32(0xA2F77C)=[('.text','0x981cbc')]
    abs32 into run all sections = [('.bind','0xbd3765','0xa2f04f')] のみ
    .text 4B in [0xA2D000,0xA2F780)=24, operand-valid=1
```

検証日: 2026-10-25。ハーネス: `probe_harness.py selftest` 4/4 PASS。本ファイルが V93 の正本（`docs/`・`TODO_re49.md`・`session-memory.json`・`plan.md` は未変更）。
