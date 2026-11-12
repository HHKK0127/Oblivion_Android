# V126: .bind 複号ブートストラップ → CRT startup 接続の解読と CRT 関数境界の確定

- 作成日: 2026-10-26 / 手法: capstone 5.0.7（skipdata=True / md.detail=True）による線形逆アス + 自作 CFG（BFS 到達性）+ chain-XOR / XTEA の手動複号
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B、v1.2.0416 Steam、pefile なし・手動パース）
- ImageBase 0x400000 / AddressOfEntryPoint RVA **0x7AF2ED → VA 0xBAF2ED**（.bind 内）
- セクション（実測）:
  | SEC | VA | raw | rsz | chars |
  |---|---|---|---|---|
  | .text | 0x401000 | 0x400 | 0x626E00 | 0x60000020 |
  | .rdata | 0xA28000 | 0x627200 | 0xD9A00 | 0x40000040 |
  | .data | 0xB02000 | 0x700C00 | 0x30C00 (vsz 0xA9C1C→0xBABC1C) | 0xC0000040 |
  | .tls | 0xBAC000 | 0x731800 | 0x200 | 0xC0000040 |
  | .rsrc | 0xBAD000 | 0x731A00 | 0x1800 | 0x40000040 |
  | .bind | 0xBAF000 | 0x733200 | 0x55400 (vsz 0x56000→0xC05000) | 0x60000040 |
  - DataDir: import rva=0x700304(VA 0xB00304) / IAT rva=0x628000(VA 0xA28000, size 0x3D0) / TLS rva=0x6B34F0(VA 0xAB34F0) / debug rva=0x62F7D0
- 前提資料: `tmp/re/v90_b350d8_triangle.md` §5-1・§5-2、`tmp/re/v93_gmst_gt_residuals.md`「未解決の残点」1・2、`docs/RE_EXE_REPORT.md` §70.10・§35.5
- 本ノートは 1 ファイル新規作成のみ。`docs/`・`TODO_re49.md`・`session-memory.json`・`plan.md`・既存 tmp/re は無変更、git / memory / sync は未実施。

---

## §0 結論サマリ（4 問への回答）

| 問 | 判定 | 一句话 |
|---|---|---|
| (1) 複号 → CRT startup の接続 | **[解明・確定]** | **レジスタ経由の tail-jump（`0xBB097D: jmp eax`）**。eax = 手動マッピングした **`steamdrm.dll` の export `start()` の戻り値**、その値は **暗号化ヘッダ `0xC03168+0x318` に格納された `0x0098787C`**。E8/E9/imm32/TLS/ポインタテーブルを使わないため全探索で 0 件だった。 |
| (2) WinMainCRTStartup の prologue と名称 | **[確定]** | **開始点 = `0x98769C`**、prologue は **`6A 60 / 68 D8 FA AF 00 / E8 … call 0x98C5A0`（`__SEH_prolog(0x60, 0xAFFAD8)`）** 形（=`55 8B EC` が無い理由）。名称 = **`WinMainCRTStartup`（`__tmainCRTStartup` 本体）**、エントリスタブは **`0x98787C`（`call __security_init_cookie(0x997BE3); jmp 0x98769C`）**。 |
| (3) 0x981FB4 / 0x981F78 / §70.10 初期化配列 | **[確定]** | `0x981F78` = **`_onexit`（登録本体・SEH prolog）**、`0x981FB4` = **`atexit` ラッパ（成功=0/失敗=-1）**。§70.10 の `0xA28860..0xA2D184`（4,681 スロット全数 .text）を走査するのは **`0x981C6B` のインラインループ `0x981CC5..0x981CD4`（範囲 `[0xA283D0,0xA2F77C)`）**。 |
| (4) 0x981C6B の引数 / callback | **一部確定・一部残点** | 第1引数 = **`ebx = 1`**（`0x987769: xor ebx,ebx; inc ebx` が生成）→ **`[0xAA3E84]=0x982897`（FP 精度初期化フック）へ転送**。`[0xBABC18]` は **全セクションで書手 0 件・初期値 0 → 起動時は未呼出し**、呼び出し形は `cb(0,2,0)`。正体は残点。 |

---

## §1 複号ブートストラップの全体構造（.bind / 0xBAF2ED..0xBB1202）

スタブは 1 つの巨大関数（線形デコード 1,790 命令、CFG 到達 1,782 / 不到達 8 = エラー経路の死コード）。先頭:

```
0x00BAF2ED: 53 51 52 56 57 55        push ebx; ecx; edx; esi; edi; ebp
0x00BAF2F3: 8B EC                    mov ebp, esp
0x00BAF2F5: 81 EC 00100000           sub esp, 0x1000
0x00BAF2FB: C7 85 78ffffff 0412BB00  mov dword [ebp-0x88], 0xBB1204   ; 暗号 config VA
0x00BAF30B: B9 D9000000              mov ecx, 0xD9                     ; 0xD9 dwords = 0x364 B
0x00BAF310: 8D BD 10FCFFFF           lea edi, [ebp-0x3F0]              ; 複写先
0x00BAF316: F3 A5                    rep movsd
0x00BAF324: C7 85 D4FBFFFF 679BC972  mov dword [ebp-0x42C], 0x72C99B67 ; 初鍵
0x00BAF354..0xBAF3AA                 chain-XOR 複号ループ（*p ^= key; key = ct; p+=4）
```

### 1-1. 複号 config（[ebp-0x3F0]、868 B）の中身（実測・完全復号済み）

| +off | 値 | 用途 |
|---|---|---|
| 0x04 | 0xA281A4 | IAT スロット = kernel32!**GetModuleHandleA** |
| 0x0C | 0xA2811C | IAT スロット = kernel32!**GetProcAddress** |
| 0x10 | 0xA28118 | 追加 IAT スロット（未使用・0 で skip 判定あり） |
| 0x14 | 0 | 任意コールバック枠（0 のため未呼出し） |
| 0x18 / 0x1C | 0xBAF000 / 0x2204 | **自己ハッシュ対象 = スタブ自身 [0xBAF000,0xBB1204)** |
| 0x20 | **0x815CFDB0** | 期待ハッシュ値 = かつヘッダマジック |
| 0x24 / 0x28 | 0xC03168 / 0x1000 | **暗号ヘッダ VA / サイズ** |
| 0x2C | 0x573A | （ヘッダ+0x40C と同値。意味未特定=残点） |
| 0x34..0x39 | `'0'` + `"22330"` | **status 初値 '0' + Steam AppID 22330（Oblivion）** |
| 0x3C / 0x40 / 0x44 | 0x398 / 4 / 0x3FC | ヘッダ内のフィールドオフセット（srcva / size / XTEA key） |
| 0x48.. | `kernel32.dll` `user32.dll` `shell32.dll` `php13d3.tmp` | モジュール名（+0x98 の `php13d3.tmp` は用途未特定=残点） |
| 0xD8..0x2B0 | LoadLibraryExA / MessageBoxA / GetModuleFileNameA / GetModuleHandleA / VirtualProtect / VirtualQuery / OutputDebugStringA / GetLastError / ExitProcess / VirtualAlloc / VirtualFree / GetProcAddress / IsBadReadPtr / GetProcessHeap / HeapAlloc / HeapFree / OpenEventA / OpenFileMappingA / MapViewOfFile / UnmapViewOfFile / CloseHandle / GetCurrentProcessId / Sleep / WaitForSingleObject / CreateEventA / ShellExecuteA / GetCommandLineA / GetFileAttributesA | **解決する API 名一覧** |
| 0x2C4 / 0x2E4 | `Local\SteamStart_SharedMemFile` / `Local\SteamStart_SharedMemLock` | Steam IPC（共有メモリ / イベント） |
| **0x304** | **`"start"`** | ★**後段で解決する payload export 名** |
| 0x30C / 0x31C / 0x32C / 0x354 | `steam.exe` / `Steam Error` / `Application load error X:XXXXXXXXXX` / `-applaunch ` | 失敗時ダイアログ・Steam 起動フォールバック |

### 1-2. 段階（CFG 到達性で証明した実行順）

```
[1] 0xBAF2ED prologue + config 複写(0xD9 dwords) + chain-XOR(初鍵 0x72C99B67)
[2] 0xBAF3AB/0xBAF452  バイトコピー: config+0x31C→[ebp-0x80]("Steam Error"),
                       config+0x32C→[ebp-0x428]("Application load error X:XXXXXXXXXX")
[3] 0xBAF46C..0xBAF4B1 自己完整性検査: 0xBaf266(buf,len,seed) を 2 段連鎖
       h = hash(0xBAF000, 0x2204, 0) → h = hash(config, 0x364, h) → cmp 0x815CFDB0
       不一致 → status='1' → エラー経路
[4] 0xBAF4FF..0xBAF571 GetModuleHandleA("kernel32.dll")（IAT[0xA281A4] 直呼）
       → [ebp-0x70]=kernel32 base; IAT[0xA2811C] → [ebp-0x58]=GetProcAddress
[5] 0xBAF59E..0xBAF72E 上表の API 名 ×28 を GetProcAddress でローカル変数へ
       （ExitProcess=[ebp-0x434], VirtualAlloc=[ebp-0x6c], VirtualFree=[ebp-0x68],
         VirtualProtect=[ebp-0x64], VirtualQuery=[ebp-0x60], GetProcessHeap=[ebp-0x40],
         HeapAlloc=[ebp-0x3c], HeapFree=[ebp-0x38], OpenEventA=[ebp-0x34],
         OpenFileMappingA=[ebp-0x30], GetCommandLineA=[ebp-0x10],
         GetFileAttributesA=[ebp-0x0c] …）
[6] 0xBAF800..0xBAF866  GetCommandLineA() の結果を走査し "-drm" を検索
       見つからず → status='0'（通常）／見つかる → status = match[5] → エラー経路
[7] 0xBAF882..0xBAF8E7  アンチデバッグ: fs:[0x18] の自己参照整合 → PEB(+0x30)→[PEB+2]
       (BeingDebugged) を検査 → 既定 'T'(0x54) パス
[8] 0xBAF8F1  *(uint32*)0xC03168 == 0x815CFDB0 ?（ヘッダマジック検査）
[9] 0xBAF905..0xBAF9B5 Steam IPC: OpenEventA(EVENT_ALL_ACCESS,0,"Local\SteamStart_SharedMemLock")
       + OpenFileMappingA(FILE_MAP_WRITE,0,"Local\SteamStart_SharedMemFile")
       + WaitForSingleObject(ev, 0x1388=5000ms) → 失敗時 status='5'/'G'/'L'
[10] 0xBABF04..0xBABFC1B ヘッダ処理: HeapAlloc(0x1000) → 0xC03168 から複写
       → chain-XOR 複号（初鍵 = hdr[0]=0x815CFDB0）→ [ebp-0x3cc] = 複号ヘッダ
[11] 0xBABFC27..0xBABFCC7 ヘッダ読み出し:
       hdr[0x398]=0xBB1568(payload VA) / hdr[4]=0x51C00(size) / key = hdr+0x3FC
       HeapAlloc(0x51C00) → [ebp-0x4dc]
[12] 0xBABFCC7..0xBAFE44 **XTEA 複号**（32 ラウンド、delta 0x9E3779B9 @0xBAFD26、
       加算定数 0x61C88647 @0xBAFDA4、prev = 0x55555555、前ブロック暗号文との XOR）
       → payload は [0xBB1568, 0xC03168) = ヘッダの直前に配置
[13] 0xBAFE49..0xBB0700 手動マッピング: MZ/PE 検査 →
       VirtualAlloc(希望 ImageBase 0x10000000, MEM_RESERVE) → MEM_COMMIT でヘッダ複写
       → NT ヘッダ +0x34 へ実 ImageBase 書き戻し → セクション複写 → リロケーション →
       IAT 解決 → セクション保護付与（VirtualProtect 0xBB06FD）
[14] 0xBB0705..0xBB075F **DllMain(mapped_base, DLL_PROCESS_ATTACH=1, 0) を
       (base + AddressOfEntryPoint=0x1C757) で間接呼出し** → 0 なら status='B'
[15] 0xBB0766..0xBB077A **`0xBaf005(mapped_base, config+0x304="start")`**
       = export 名による関数解決（MZ/PE/0x14C/0xE0 検証 + 大文字小文字吸収の
       エクスポート名走査）→ eax = payload!start の VA
[16] 0xBB0780..0xBB07D5 payload のヘッダ（DOS+NT+optional = e_lfanew+0xF8 バイト）を
       0 埋め（マッピング先のヘッダ消去）
[17] 0xBB085D..0xBB086F `push [ebp-0x3c8](=0x1000); push [ebp-0x3cc](=複号ヘッダ);
       call [ebp-0x3f4](=start)  →  eax = 戻り値 → [ebp-0x430]
[18] 0xBB08D1  `cmp [ebp-0x430], 0xFF`
       未満 → 戻り値は「エラー文字コード」→ status へ代入 → 0xBB097F（エラー経路:
       "Application load error" 整形 → MessageBoxA → `push 0; call [ebp-0x434]`=ExitProcess(0)）
       以上 → [19]
[19] 0xBB096B..0xBB097D  ★接続の本体★
       mov eax, dword [ebp-0x430]
       add esp, 0x1000
       pop ebp / pop edi / pop esi / pop edx / pop ecx / pop ebx   ← 入口 6 push の逆
       jmp eax                                                     ← **OEP へ tail-jump**
```

- スタブ内で **間接ジャンプは `0xBB097D: jmp eax` の 1 箇所のみ**（線形デコード全 1,790 命令を機械分類して確認）。
- 入口の 6 レジスタ push を逆順 pop してから飛ぶため、**ローダが入口で用意したスタック（復帰先 = BaseThreadInitThunk）とレジスタ状態を保ったまま CRT startup に到達**する = スタブが存在しなかったかのような状態になる。
- **コード・IAT・スタックへのパッチ書き換えは無い**: `VirtualProtect` の呼出しは全て payload のセクション VA（0x10000000 台）に対するもので、主画像 (.text 0x401000..) を書く箇所は 0 件。TLS コールバック配列 `0xA2F7AC` は**先頭 0**（ローダは先頭 NULL で打ち切るため非到達。3,4 項目に 0x9A102F / 0x98BBA7 が続くが隣接する静的 dtor 配列 `[0xA2F7B0,0xA2F7BC)` の一部）。

---

## §2 複号 → CRT startup 接続の決定的証拠

### 2-1. OEP の所在 = **暗号ヘッダ `0xC03168 + 0x318`**

復号したヘッダ（chain-XOR、初鍵 = 自身の先頭 dword）の実測:

```
hdr[0x000] = 0x815CFDB0   (magic / chain-XOR 初鍵)
hdr[0x004] = 0x00051C00   (payload size = 334,848 B)
hdr[0x318] = 0x0098787C   ★★ OEP = WinMainCRTStartup entry stub ★★
hdr[0x3FC] = 6d2739a2 a55a5716 31ccff41 43333ab2  (XTEA key16)
hdr[0x40C] = 0x0000573A   (config+0x2C と同値)
hdr[0xBE4] = 0x00000026   (start() が [0x10046D28] へ保持するフィールド)
```

**payload (`steamdrm.dll`) の export `start` @RVA 0x404F の実処理（逆アス）**:

```
0x10004058  call [0x10030024]                    ; IsDebuggerPresent → 0 でなければ 'T'(0x54)
0x10004079..0x100040F8  "ntdll.dll" / "NtSetInformationThread" を即値で構築
0x100040FB  call esi                             ; GetModuleHandleA("ntdll.dll")
0x10004102  call [0x10030010]                    ; GetProcAddress(..., "NtSetInformationThread")
0x1000410A  push 0x11                            ; ThreadHideFromDebugger
          push 0 / push 0 / call [0x10030018]    ; GetCurrentThread
0x10004115  call edi                             ; NtSetInformationThread(...)  ← アンチデバッグ
0x10004117  call 0x10004de4 ; test al → else '8'(0x38)
0x10004130  mov eax, [ebp+8]                     ; arg1 = 複号ヘッダ
0x10004133  cmp dword [eax], 0x815CFDB0 ; je ok → else 'Q'(0x51)
0x10004142  mov ecx,[eax+0xBE4] → [0x10046D28]
0x1000414E  mov edx,[eax+0x40C] → [0x10046D24]
0x1000415A  mov edx,[eax+0x318] → [0x10046D2C]   ★ OEP を自前グローバルへ退避
   …（ファイル操作・ハッシュ検証 → 失敗 'D'(0x44)、タイマ検証 GetTickCount 差 ≦ 0x2710
      ms → 超過時 'S'(0x53)、その他 '3'(0x33) '6'(0x36) 'P'(0x50) '5'(0x35)）…
0x100044C2  call 0x10004b49
0x100044C7  mov eax, dword [0x10046D2C]          ; ★ eax = 0x98787C
0x100044D0  ret 8                                ; （stdcall 2 引数 = start(hdr, hdrsize)）
```

→ **`start()` の戻り値 = 0x98787C**。スタブが `cmp [ebp-0x430],0xFF` で「エラー文字かアドレスか」を判別し、アドレス側なら `jmp eax`。

### 2-2. なぜ従来の全探索で 0 件になったか（証拠一覧）

| 探索 | 結果 | 備考 |
|---|---|---|
| 全ファイル raw で imm32 = `0x98787C` | **0 件** | ファイル上は暗号文 `99 2c 7a 9a`（file off **0x787680** = 0xC03168+0x318 の暗号バイト） |
| 全ファイル raw で imm32 = `0x98769C` / `0x987881` / `0x987886` / `0x40DF80` / `0xBAF2ED` | **各 0 件** | |
| .text 等の E8/E9 rel32 → 0x98787C / 0x98769C | **0 件**（0x98769C への E9 は 0x987881 の内部 1 件のみ = v93 再現） | |
| **.bind 内の E8/E9 → .text** | 3 件（0xBC5902→0x7B4697, 0xBDBE3F→0x4AFBBA, 0xBE2FB5→0x913274） | **3 件とも XTEA 暗号文領域 [0xBB1568,0xC03168) 内の偶然一致**。スタブ実行部 [0xBAF2ED,0xBB1204) には **0 件** |
| .bind 内の imm32 で .text 範囲 | 632 件 | 全数がオペコード断片の偶然一致（例 0xBAF017 = `cmp eax,0x5A4D` の後続バイト）。**命令境界で operand として成立するものは 0** |
| TLS コールバック（配列 0xA2F7AC） | 先頭 0 → **未実行** | |
| .rdata/.data 等の絶対ポインタテーブル | 0x98787C を指すもの **0 件** | |
| payload 内の abs32 = 0x98787C / 0x98769C / 0x40DF80 | **各 0 件** | payload にも絶対 VA は無く、値は暗号ヘッダにのみ存在 |
| ポッチ書き換え（VirtualProtect/MemMove が主画像 .text を書く） | **0 件** | VirtualProtect の引数は全て payload の 0x10000000 台 |

**結論: 接続手段 = 「暗号化されたデータ（ヘッダ+0x318）を解決した DLL の関数の戻り値」として受け取り、レジスタ tail-jump で飛ぶ**（スタック直書き・E8/E9・imm32・ポッチのいずれでもない）。

### 2-3. 到達しなかった場合の経路（補完）

`start()` が文字コード（<0xFF）を返した場合、またはスタブ自前の検査が失敗した場合は `0xBB097F` へ:

```
0xBB097F  movsx eax, byte [ebp-0x3bc] ; cmp eax,'5' → 分岐（status による分岐）
… 0xBB0B01  "steam.exe" / "-applaunch " を使い ShellExecuteA で Steam 起動を試みる
… 0xBB1164  call 0xBaf201 で "Application load error X:XXXXXXXXXX" を整形
… 0xBB1178  MessageBoxA(...)
… 0xBB11F6  push 0 ; call [ebp-0x434]  = ExitProcess(0)
… 0xBB11FE  pop edi/esi/ebx ; leave ; ret   ← 実行されない（ExitProcess で終了）
```

エラー文字: `'1'`(CRC 不一致) `'B'`(DllMain=0) `'T'`(PEB BeingDebugged) `'5'`(IPC 失敗) `'G'`(WaitForSingleObject 失敗) `'L'`、payload 側 `'T' 'Q' 'D' 'S' '8' '3' '6' 'P' '5'`。

---

## §3 WinMainCRTStartup の prologue 確定と名称

### 3-1. 問題だった 3 つの観測の説明

| 観測（v90/v93） | 解決 |
|---|---|
| `0x98769C` は prologue 無しで `[ebp-4]` を書くのに、入辺は `0x987881` の E9 のみ | **`0x98769C` が関数の開始点**。prologue は `push ebp;mov ebp,esp` ではなく **SEH prolog 形**: `push 0x60`（ローカル 0x60 B）`push 0xAFFAD8`（SEH スコープテーブル）`call 0x98C5A0`。`0x98C5A0 = __SEH_prolog` の本体を逆アスで確認: `push 0x988AB0 / push fs:[0] / mov eax,[esp+0x10] / mov [esp+0x10],ebp / lea ebp,[esp+0x10] / sub esp,eax / push ebx,esi,edi / cookie xor / fs:[0]=&frame / ret` → **呼び出し側は ebp を持ったままに戻る**ため `0x9876A8` からの `[ebp-4]` 書きが成立する。 |
| `55 8B EC` は `0x98717A` / `0x987886` にしかない | どちらも **WinMainCRTStartup と別の CRT 関数**（§3-3）。 |
| `0x987699` の 3 バイト `xor eax,eax; ret` の混入 | **直前関数 `0x98765B` の失敗時リターン**（§3-2）。 |

### 3-2. 領域の切り分け（本タスクで確定した境界）

```
0x0098765B  [leaf 関数]  cmp word [0x400000],'MZ' → mov eax,[0x40003C](e_lfanew)
                          → cmp [eax+0x400000],'PE' → cmp [eax+0x400018],0x10B (PE32)
                          → cmp [eax+0x400074],0xE (NumberOfRvaAndSizes>14)
                          → cmp [eax+0x4000E8],0 → setne
                          → 0x987696 mov eax,ecx / 0x987698 ret
0x00987699      33 C0 C3  xor eax,eax ; ret     ← 同関数の early-fail 戻り（v90 の「混入」の正体）
0x0098769C  [★ WinMainCRTStartup 開始]
            6A 60              push 0x60
            68 D8 FA AF 00     push 0xAFFAD8
            E8 F8 4E 00 00     call 0x98C5A0        ← __SEH_prolog = 「prologue」
            83 65 FC 00        and [ebp-4],0
            8D 45 90 / 50 / FF 15 D081A200   GetStartupInfoA(&[ebp-0x70])  IAT 0xA281D0
            BF 94000000 / 57 / 6A00 / 8B 1D CC81A200 / FF D3 / 50 / FF 15 9481A200
                                       HeapAlloc(GetProcessHeap(),0,0x94=sizeof(STARTUPINFOA))
            FF 15 C881A200      GetVersionExA        IAT 0xA281C8
            … [0xBA9D94..0xBA9DA4] へ _osplatform/_osver/_winver 系を保存
            FF 15 C481A200 / A3 04BCBA00   GetCommandLineA → [0xBABC04] (_acmdln)
            E8 … call 0x997AAE / 0x9979F5 (jge なければ _amsg_exit(8)) /
                   call 0x997782 (同 9)     ← cinit 系（v93 の記述どおり）
            53 / E8 85A4FFFF  push ebx(=1) ; call 0x981C6B   ← §4
            E8 2EFF0000       call 0x997725   （argv0 除去 → lpCmdLine）
            51 / 50 / 6A00 / 68 00004000
            E8 6D67A8FF       call 0x40DF80   ← WinMain(0x400000, 0, lpCmdLine, nShow)
            89 45 E4          mov [ebp-0x1c],eax
            … call 0x981DCB / 0x981DED（atexit/静的 dtor 実行系）…
            B8 FF000000       mov eax,0xFF     ← 失敗時コード
0x00987876  E8 6A4D0000       call 0x98C5E5    ← __SEH_epilog
0x0098787B  C3                ret
0x0098787C  E8 62030100       call 0x997BE3    ← **__security_init_cookie**
0x00987881  E9 16FEFFFF       jmp 0x98769C     ← 本体への入辺はこの E9 のみ
0x00987886  [別の CRT 関数の開始] 55 8B EC 83 EC 20 53 33 DB 39 5D 14 …
```

- **`0x98787C` = リンカが出力する実行可能エントリスタブ**（MSVC の典型配置: `__security_init_cookie` 呼出し + 本体への jmp を本体の直後に置く）。**Steam DRM 導入前の AddressOfEntryPoint は 0x98787C だったと推定 [仮説]（証拠: DRM が復帰させる OEP が exactly これ）**。
- **名称 = `WinMainCRTStartup`（`__tmainCRTStartup` 本体）[確定]**: `GetStartupInfoA` + `HeapAlloc(0x94)` + `GetVersionExA` + `GetCommandLineA` + `lpCmdLine` 構築 + **`WinMain(4 引数)` 呼出し**（0x40DF80 は `ret 0x10` = stdcall 4 引数）+ `nShowCmd`（STARTUPINFO.wShowWindow か 10=SW_SHOWDEFAULT）+ 戻値を exit へ渡す構成。`mainCRTStartup`（Win32 `main`）ではなく **WinMain 版**であることは 4 引数呼び出しで確定。

### 3-3. `0x98717A` と `0x987886` の正体

| VA | prologue | 構造 | 判定 |
|---|---|---|---|
| 0x98717A | `55 8B EC 83 EC 14 53 33 DB` | 3 引数（`[ebp+8]/[+0xC]/[+0x10]`）。null 時に `errno=0x16`(EINVAL) を設定（`mov [eax],0x16` + `call 0x984D3A`）、`FILE` 相当の `+0x70` フラグを `and …,0xFFFFFFFD`、末尾 `5F 5E 5B C9 C3` @0x9872B2 | **stdio 系の別 CRT 関数**（3 引数・FILE フラグ操作）[仮説] |
| 0x987886 | `55 8B EC 83 EC 20 53 33 DB 39 5D 14` | 4〜7 引数、**第1引数 `[ebp+8]` を関数ポインタとして `call dword [ebp+8]`** する高階関数。出力構造体 `[ebp-0x20]`、残数 `[ebp-0x1c]`（初期 0x7FFFFFFF）、`[ebp-0x14]=0x42` | **書式化/書き出し系の別 CRT 関数**（コールバック引数を持つ）[仮説] |

いずれも **CRT エントリではない**（エントリは 0x98787C の jmp 先 = 0x98769C のみ）。

---

## §4 0x981C6B: 第1引数（ebx）と [0xAA3E84] / [0xBABC18]

### 4-1. 全文（実逆アス）

```
0x00981C6B  83 3D 843EAA0000    cmp dword [0xAA3E84],0
0x00981C72  74 1A               je  0x981C8E
0x00981C74  68 843EAA00         push 0xAA3E84
0x00981C79  E8 24BC0000         call 0x98D8A2          ; ポインタ検証（下記 4-3）
0x00981C7E  85 C0 / 59 / 74 0B  test eax; pop ecx; je 0x981C8E
0x00981C83  FF 74 24 04         push dword [esp+4]     ; ← 第1引数をそのまま転送
0x00981C87  FF 15 843EAA00      call [0xAA3E84]        ; ★ callback
0x00981C8D  59                  pop ecx
0x00981C8E  E8 72BB0000         call 0x98D805          ; [0xB312A0..0xB312C8) 10 スロットを
                                                        ;   0x98BE7F() で各々変換して書戻し
0x00981C93  68 A4F7A200 / 68 80F7A200 / E8 36FFFFFF
                                    push 0xA2F7A4; push 0xA2F780; call 0x981BD8
                                                        ; = __initterm(6 スロット)
0x00981CA2  85 C0 / 59 / 59 / 75 54   test eax → 0 でなければ即 return eax（失敗）
0x00981CA8  56 / 57 / 68 E1D79800 / E8 00030000
                                    push 0x98D7E1; call 0x981FB4   ; atexit(終了時 walker)
0x00981CB4  BE D083A200         mov esi, 0xA283D0      ; 走査開始
0x00981CBB  BF 7CF7A200         mov edi, 0xA2F77C      ; 走査終端
0x00981CC5  8B 06               mov eax,[esi]          ; ★ インラインループ本体
0x00981CC7  85 C0 / 74 02       test eax; je next
0x00981CCB  FF D0               call eax               ; 初期化関数を呼ぶ
0x00981CCD  83 C6 04 / 3B F7 / 72 F1   add esi,4; cmp esi,edi; jb loop
0x00981CD4  83 3D 18BCBA0000    cmp dword [0xBABC18],0
0x00981CDB  5F / 5E / 74 1C     pop edi; pop esi; je 0x981CFA（= xor eax,eax; ret）
0x00981CDF  68 18BCBA00 / E8 B9BB0000  push 0xBABC18; call 0x98D8A2  ; 同検証
0x00981CE9  85 C0 / 59 / 74 0C  test; pop ecx; je 0x981CFA
0x00981CEE  6A 00 / 6A 02 / 6A 00 / FF 15 18BCBA00
                                    push 0; push 2; push 0; call [0xBABC18]  ; cb(0,2,0)
0x00981CFA  33 C0               xor eax,eax
0x00981CFC  C3                  ret
```

- 関数範囲 `[0x981C6B, 0x981CFD)`（台帳 21,597 件に開始点なし = 無名 gap、v93 §c 再確認）。先行は `0x981C6A: ret` で落下不可 → 単独関数。
- 戻り値 = 0（成功）または `__initterm` 失敗時のコード → 呼び出し元 `0x9877E7: test eax,eax; je; push eax; call 0x981B4F`（_amsg_exit）。

### 4-2. 第1引数（ebx）= **1** [確定]

`WinMainCRTStartup` 内で ebx は 1 に固定される:

```
0x987769  33 DB        xor ebx, ebx
0x98776B  43           inc ebx                      ; ebx = 1
0x98776C  53 / E8 …    push ebx ; call 0x98D55E(1)   ← 以降 ebx は 1 のまま
0x987795  89 5D FC     mov [ebp-4], ebx              ; SEH スコープ状態 = 1
0x9877E0  53           push ebx                      ; ★ 0x981C6B の第1引数 = 1
0x9877E1  E8 85A4FFFF  call 0x981C6B
0x9877F7  84 5D BC     test byte [ebp-0x44], bl      ; ebx をマスクとしても使用
```

第1引数は **`[0xAA3E84]` コールバックへそのまま渡る**（4-1 の `push [esp+4]; call [0xAA3E84]`）。表側（0x981C6B 本体）ではループ走査・atexit 登録等の制御には使われない。

### 4-3. `[0xAA3E84]` = **0x982897**（.rdata 実初期値）[確定・名称は仮説]

- 値: `.rdata` ファイル上の実データ = `0x00982897`（書き手は無く定数）。
- **ガード `0x98D8A2`** = `push 0x400000; call 0x98D830`（ImageBase の MZ/PE/PE32 検証）→ `call 0x98D860(base, ptr-base)`（区間テーブルの二分探索）→ `[entry+0x24] >> 31` の否定を返す = **「そのポインタが実行可能イメージ内か」を検証してから呼ぶ**（CFG 的ガード）。
- 本体 `0x982897`:
  ```
  0x982897  call 0x982837   ; 0xB312A0..0xB312C8 に 10 個の CRT ハンドラを格納
                             ;  (0x99074E,0x98FE2F,0x98FDED,0x98FE21,0x98FD97,
                             ;   0x99074E,0x9906C8,0x98FDAD,0x98FD17,0x98FCA6)
  0x98289C  call 0x9907D4   ; GetModuleHandleA(0xAA5054) → GetProcAddress(...,0xAA5038)
                             ;  → 戻り値(0) を呼ぶ（動的解決フック）
  0x9828A1  cmp [esp+4],0
  0x9828A6  mov [0xBA9DE0], eax
  0x9828AB  je skip / 0x9828AD call 0x99076F
                             ; 0x99076F = push 0x30000; push 0x10000; push 0;
                             ;            call 0x99E24F(_controlfp) → 失敗時 _invalid_parameter
  0x9828B2  fnclex
  0x9828B4  ret
  ```
- 判定: **`_controlfp(0x10000,0x30000)` = 倍精度(53bit)化 + `fnclex` + 内部ハンドラ表初期化** = **浮動小数点初期化フック（`_FPinit` / `_setdefaultprecision` 相当）[仮説]**。第1引数=1 のときのみ `_controlfp` を実施する点が引数の意味（=「FP 初期化を行うか」）と整合。
- 注: `0x9907D4` の 2 定数 `0xAA5054` / `0xAA5038` は ASCII 文字列ではなくデータ（動的解決のモジュール名/関数名が別の形式で与えられている）= 未解 [残点]。

### 4-4. `[0xBABC18]` = **書手 0 件・起動時は未登録** [確定（正体は残点）]

| 走査 | 結果 |
|---|---|
| 全セクションで imm32 = `0xBABC18` | **3 件すべて `0x981C6B` 内**（0x981CD6 cmp / 0x981CE0 push / 0x981CF6 call） |
| 隣接スロットの書手 | 0xBABC00(6) 0xBABC04(3) 0xBABC08(2) 0xBABC0C(4) 0xBABC10(4) 0xBABC14(5) / **0xBABC18(3=読手のみ)** 0xBABC1C(0) |
| セクション属性 | VA 0xBABC18 = .data の vsz 終端 0xBABC1C の直前 → **raw 範囲外 = BSS、初期値 0** |

- 近傍は CRT グローバル群: `0xBABC00` = `_maxstdio`（既定 0x200 / 最小 0x14、`0x98BAF6` が拡張）、`0xBABC04` = `_acmdln`（`0x9877B0` が GetCommandLineA 結果を格納）、`0xBABC0C`/`0xBABC10` = **onexit テーブルの境界**（`0x981E9C` が使用）、`0xBABC14` = mbcp 初期化済フラグ（`0x98FC88` が 1 を書く）。
- 呼び出し形 = **`cb(0, 2, 0)`**（登録者が居れば）。imm32 書手が 0 件なので、**起動時点では 0 = ガードでスキップされる**。
- 判定: 「CRT グローバル末尾の未設定コールバック枠。書手は間接（レジスタ+disp）でも見当たらず = 未特定」→ **V143 残点**。

---

## §5 §70.10「0xA28860..0xA2D184 初期化配列を走査する起動コード」

### 5-1. 走査コード = **`0x981C6B` のインラインループ `0x981CC5..0x981CD4`** [確定]

- 配列実体: **`[0xA28860, 0xA2D184)` = 4,681 スロット（24,682 B）**、実測で **NULL 0 件 / 全数が .text（0x401000..0xA27C39）** = 「初期化関数ポインタ」列。§70 の「4,682 スロット」は終端込みの数え方と一致。
- その外側の実行時テーブル = `[0xA283D0, 0xA2F77C)` = **7,403 スロット（NULL 24 / .text 7,379）**、**`0xA28860..0xA2D184` はこれに厳密に内包**。
- 走査コードの境界は `mov esi,0xA283D0`（imm32 @**0x981CB5**）/ `mov edi,0xA2F77C`（imm32 @**0x981CBC**）のみ。

### 5-2. 「別の走査器は無い」ことの証拠

| 走査 | 結果 |
|---|---|
| 全セクション imm32 = **0xA28860** | **0 件** |
| 全セクション imm32 = **0xA2D184** | **0 件** |
| 全セクション imm32 = 0xA283D0 / 0xA2F77C | **各 1 件（0x981CB5 / 0x981CBC）** |
| 他の走査器（v93 §c-2 再現） | `0x981BD8` = `[0xA2F780,0xA2F7A4)` の 6 スロットのみ（0x981C9D から）、`0x981BC0` = `[0xA2F7B0,0xA2F7BC)` と `[0xA2F7C0,0xA2F7C8)` のみ |
| ポインタテーブルとして 0xA28860..0xA2D184 を指す絶対ポインタ | .text 0 件（v93 の §c 再現どおり） |

→ **§70.10 の疑問は「0x981C6B（特に 0x981CC5..0x981CD4）」で決着**。到達時期 = `WinMainCRTStartup` が `push ebx(1); call 0x981C6B` する**起動時に 1 回**（`0x9877E1`、abs32/E9 からの入辺は 0 件、`0x981C6A: ret` 直後のため単独関数）。

---

## §6 0x981FB4 / 0x981F78 の役割

```
0x00981F78  6A 0C / 68 20F9AF00 / E8 1CA60000     push 0xC; push 0xAFF920; call __SEH_prolog
            E8 25FCFFFF                            call 0x981BAE        （ロック取得）
            83 65 FC 00                            and [ebp-4],0
            FF 75 08 / E8 07FFFFFF                 push [ebp+8]; call 0x981E9C
                                                   （onexit テーブル [0xBABC0C,0xBABC10) への挿入）
            89 45 E4                               mov [ebp-0x1c], eax
            E8 09000000                            call 0x981FAE → 0x981BB7（ロック解放）
            8B 45 E4 / E8 38A60000                 eax = 結果 → __SEH_epilog(0x98C5E5)
            C3                                     ret
0x00981FB4  FF 74 24 04                            push dword [esp+4]    （引数 fn を再 push）
            E8 BBFFFFFF                            call 0x981F78         （_onexit(fn)）
            F7 D8 / 1B C0 / F7 D8                  neg; sbb; neg         → (result!=0 ? 1 : 0)
            59                                     pop ecx
            48                                     dec eax               → 成功時 0 / 失敗時 -1
            C3                                     ret
```

- **`0x981F78` = `_onexit(fn)`（登録本体・SEH prolog 付き）**、**`0x981FB4` = `atexit(fn)`（戻り値 0=成功 / -1=失敗 のラッパ）** [確定]。
- 呼び出し例: `0x981CAA: push 0x98D7E1; call 0x981FB4`（`0x98D7E1` = `[0xAD6E0C,0xAD6E0C)` を走る空レンジの終了時 walker）、GMST SLOT destructor 登録 `0x9F0D69: push 0xA213A0; call 0x981FB4`（v93 §a-2）。
- **訂正（docs 対応は本タスクでは行わない）**: `docs/RE_EXE_REPORT.md` §35.5 の「`FUN_00981FB4` = 初期化完了通知／登録関数（第1引数 = 動的初期化テーブル領域のエントリ VA）」は **誤り**。第1引数は**終了時に呼ぶ関数（destructor thunk）**であり、初期化テーブル走査とは無関係（走査は 0x981C6B）。→ V143 で docs 訂正タスクとして起票。

---

## §7 payload = `steamdrm.dll`（複号結果の正体）

| 項目 | 実測 |
|---|---|
| 配置 | `.bind` 内 **[0xBB1568, 0xC03168)**（334,848 B = 0x51C00、直後にヘッダ 0xC03168） |
| 複号 | XTEA（32R、delta 0x9E3779B9、加算 0x61C88647、prev=0x55555555、前ブロック暗号文と XOR） |
| 形式 | MZ/PE、machine 0x14C、**ImageBase 0x10000000**、subsystem 2(GUI)、4 セクション（.text rsz 0x2E800 / .rdata 0x12400 / .data / .reloc） |
| **エクスポート DLL 名** | **`steamdrm.dll`**（1 エントリ） |
| **export** | **`"start"` @RVA 0x404F**（ordinal 1、`ret 8` = stdcall 2 引数） |
| 自前 entry | RVA 0x1C757 = `_DllMainCRTStartup` 形（`cmp [esp+8],1 / call __security_init_cookie 相当 / push [esp+4] ... ret 0xC`） |
| import | KERNEL32（CRT 大部分 + GetProcAddress/LoadLibraryA/IsDebuggerPresent/GetStartupInfoA/...）、ADVAPI32（RegOpenKeyA/RegQueryValueExA/RegCloseKey） |
| 文字列 | `STEAMAPPS_INTERFACE_VERSION003` / `SteamUtils004` / `R6031` `R6034` 等 |

→ **本 EXE は Valve Steam DRM（SteamDRM）でラップされている**。`.bind` = DRM スタブ、`steamdrm.dll` = DRM 本体（対話 DLL として手動マッピングされ、`start(hdr, 0x1000)` が OEP を返す）。

---

## §8 decompiled（`docs/Oblivion_decompiled.c`）の有無（本タスクで実測）

| 検索キー | 件数 |
|---|---|
| `FUN_00981bd8` | **2**（L983432 ヘッダ / L983434 定義） |
| `0098769c` / `0098787c` / `00987886` / `0098717a` / `00987660` | **0** |
| `00981c6b` / `00981f78` / `00981fb4` / `0098d8a2` / `0098c5a0` / `00997be3` | **0** |

→ 0x98xxxx 系の本タスク対象は **Ghidra が関数化していない（ラベルなし）領域**であり、decompiled には**存在しない**。`0x981BD8` のみ例外的に存在。

---

## §9 試行経路と「各経路で 0 件」の証拠（V90 残点としての記録）

| # | 試した経路 | 結果と証拠 |
|---|---|---|
| 1 | E8/E9 rel32 → 0x98787C（全セクション） | **0 件**（raw 全体スキャン） |
| 2 | E8/E9 → 0x98769C / 0x987881 | E8 **0 件**、E9 は 0x987881 の 1 件（スタブ内部）のみ = v93 再現 |
| 3 | imm32 = 0x98787C / 0x98769C / 0x987886 / 0x987881 / 0x40DF80 / 0xBAF2ED / 0x981C6B（全ファイル raw） | **各 0 件** |
| 4 | .bind 内の E8/E9 → .text | 3 件、**すべて XTEA 暗号文領域内**（0xBC5902 / 0xBDBE3F / 0xBE2FB5 ∈ [0xBB1568,0xC03168)）→ 実行経路上は 0 件 |
| 5 | .bind 内の imm32 が .text 範囲 | 632 件、全数オペコード断片の偶然一致（命令境界で operand 成立 = 0） |
| 6 | TLS コールバック（0xA2F7AC） | 先頭 0 → ローダが打ち切り → 未実行 |
| 7 | ポインタテーブル（0x98787C を指す絶対ポインタ） | 0 件 |
| 8 | ポッチ書き換え（スタブが主画像を VirtualProtect/MemMove で書く） | 0 件（VirtualProtect 引数は全て payload 0x10000000 台） |
| 9 | stack 直書き（push imm / ret による OEP 選択） | 0 件（スタブ内に `push 0x98…` 系・`ret` によるジャンプなし。末尾 `ret` はエラー経路のみ） |
| 10 | payload 内の abs32 = 0x98787C 等 | **0 件** |
| 11 | **暗号ヘッダの復号** | **hdr[0x318] = 0x0098787C を検出 → 経路確定**（file 0x787680 の暗号バイト = `99 2c 7a 9a`） |
| 12 | CFG 到達性（スタブ 1,790 命令） | 到達 1,782 / 不到達 8。`0xBB097D jmp eax` は到達（depth 369）、エラー経路 `0xBB097F` も到達 |

---

## §10 残点（V143+ へ起票する粒度）

1. **V143: `[0xBABC18]` コールバックの書手特定** — 全セクション imm32 で書手 0 件（読手は 0x981C6B の 3 件のみ）、BSS 初期値 0。レジスタ+disp 形の書手も含め探索する（候補: `0x981D6B: push 0xA2F7BC` 近傍の onexit 初期化、`0x99772B` 系）。呼び出し形 `cb(0,2,0)`。
2. **V144: `[0xAA3E84] = 0x982897` の名称確定と `0x9907D4` の 2 定数（0xAA5054 / 0xAA5038）** — 動的解決先（GetModuleHandleA/GetProcAddress の引数）の正体。`_FPinit` 相当という仮説の裏付け（0x99076F = `_controlfp` は確定、0x982897 全体の名称は仮説）。
3. **V145: `0x981C6B` の名称確定（CRT 内での呼称）** — 「cinit / __scrt_initialize / ゲーム登録テーブル走査」のいずれに相当するか、0x98D805（0xB312A0 系 10 スロットを 0x98BE7F で変換）と 0x98D7E1（空レンジ walker）の意味論込み。
4. **V146: docs 訂正 `RE_EXE_REPORT.md` §35.5** — `0x981FB4` を「初期化完了通知／登録関数（第1引数=動的初期化テーブル領域のエントリ VA）」と記述しているが、実体は **atexit ラッパ（第1引数=終了時関数）**。§70.10 の「0x981FB4・0x981F78 の役割」も同件で決着済みなので §70.10 一覧からも除去可。
5. **V147: .bind スタブのエラー文字一覧とフォールバック経路の意味論** — `'1' 'B' 'T' '5' 'G' 'L' 'M'`（スタブ）/ `'T' 'Q' 'D' 'S' '8' '3' '6' 'P' '5'`（payload）。`-drm` コマンドラインガード（GetCommandLineA 走査、match+5 を status に代入）の意図、`php13d3.tmp`（config+0x98、GetFileAttributesA/ShellExecuteA 経由で使用と推定）の用途。
6. **V148: `0x98717A` / `0x987886` / `0x98765B` の名称確定** — いずれも CRT のラベルなし関数（0x98765B = 自前イメージ PE 検証 leaf、0x98717A = 3 引数 stdio 系、0x987886 = コールバック引数を持つ書式化系）。
7. **V149: Steam DRM 導入前の AddressOfEntryPoint = 0x98787C の外部裏付け** — DRM が復帰する OEP は 0x98787C（本タスクで確定）。導入前の PE ヘッダが入手できれば完結する（現行ファイルの AddressOfEntryPoint は 0x7AF2ED に上書き済み）。
8. **V150: payload `start()` の残りの検証系** — `0x10004de4`（アンチデバッグ後チェック）、`0x10003a6f`（ハッシュ検証、失敗時 'D'）、`0x10004b49`（成功直前のクリーンアップ）、`0x10046D24/28` に退避する hdr+0x40C(0x573A)/hdr+0xBE4(0x26) の意味。
9. **（継続）V90 残点 3「ラベルなし関数の全数抽出」へ本タスク分を追加** — 新規検出: `0x98765B`（PE 検証 leaf）、`0x981C6B`（CRT 初期化+テーブル走査）、`0x981F78`/`0x981FB4`（_onexit/atexit）、`0x98D8A2`（ポインタ検証ガード）、`0x98D805`（ハンドラ表変換）、`0x982897`（FP 初期化）、`0xBaf005`（export 解決）、`0xBaf266`（ハッシュ）。

---

## §11 証跡（すべて `%TEMP%\opencode\`、`tmp/re/` の成果物は本 1 ファイル）

| スクリプト / 出力 | 内容 |
|---|---|
| `v126_step1.py` | PE ヘッダ・セクション・DataDir 実測 |
| `v126_step2.py` / `v126_dis.py` | 0xBAF2ED からの線形逆アス |
| `v126_step3.py` | **config(0x364) chain-XOR 復号 + hexdump + 文字列** |
| `v126_step4.py` | .bind 内 E8/E9 → .text（3 件=暗号文）+ imm32 走査（632 件） |
| `v126_step5.py` | [ebp-0x430]/[-0x434] 等の disp スキャン |
| `v126_step6.py` | API 名解決ブロックのローカル対応付け |
| `v126_step7.py` | **ヘッダ 0xC03168 の chain-XOR 復号（size/srcva/xkey）** |
| `v126_step8.py` | **payload の XTEA 復号 → MZ/PE 検出 → `v126_payload.bin` 生成** |
| `v126_step9.py` | payload PE ヘッダ解析（未完） |
| `v126_step10.py` | 全ファイル imm32 走査（0x98787C 等 0 件）+ payload PE 完全解析（import/entry） |
| `v126_step11.py` | 指定 VA 逆アス汎用（0xBaf005 export 解決 / 0xBB05C0.. / 0xBAFDF0..） |
| `v126_step12.py` | payload の export 解析 + 主画像 VA 参照走査（0 件） |
| `v126_step13.py` | disp32 生バイト走査（[ebp-0x3c8]/[-0xec]）+ API 解決ブロック |
| `v126_step14.py` | **スタブ CFG（BFS 到達 1,782/1,790・キー箇所パス抽出）** |
| `v126_step15.py` | CRT 帯域逆アス（crt/prol/init/atexit）+ imm32 参照走査 |
| `v126_step16.py` | **payload export 特定（`"start"` / dllname `steamdrm.dll`）** |
| `v126_step17.py` | payload の主画像 VA 参照・[0x10046D2C] 書手探索 |
| `v126_step18.py` | 0xBABC00 系書手・登録テーブル統計・0x987886/0x98765B 周辺 |
| `v126_full.txt` | 0xBAF2ED..0xBB1203 の全 1,790 命令 |
| `v126_cf_out.txt` | スタブの分岐・間接参照フィルタ出力 |
| `v126_start_fn.txt` / `v126_start_full.txt` | payload `start()` の逆アス（0x1000404F..） |
| `v126_payload.bin` | XTEA 復号済み payload（334,848 B） |

主要再現コマンド（本ノートの主張の裏付け）:

```
python "%TEMP%\opencode\v126_step10.py"   # 0x98787C 等の imm32 全ファイル 0 件 / payload PE 解析
python "%TEMP%\opencode\v126_step14.py"   # CFG: 到達 1,782/1,790、0xBB097D 到達
python "%TEMP%\opencode\v126_step15.py" init   # 0x981C6B 全文（0xAA3E84 / 0xBABC18 / ループ）
python "%TEMP%\opencode\v126_step15.py" imm32 A28860,A2D184,AA3E84,BABC18,A283D0,A2F77C
python "%TEMP%\opencode\v126_step16.py"   # export "start" / steamdrm.dll
```

検証日: 2026-10-26。バイナリ・`docs/`・既存 tmp/re・`TODO_re49.md`・`session-memory.json`・`plan.md` は無変更。新規作成は本ノート 1 ファイル（スクリプトは `%TEMP%\opencode\v126_*.py`）。

## §2 証拠（第1報: .bind 複号系の完全解読 — 2026-10-26）

### 2-1. ペイロード構造（実測）

| VA | 内容 | 出典 |
|---|---|---|
| `0xBAF000` | `.bind` 先頭。ヘルパ: `0xBAF000`(nop frame) / `0xBAF005`(手製 GetProcAddress・export 遍歴) / `0xBAF201`(数値→10桁文字列) / `0xBAF266`(CRC32系 poly `0x488781ED`, `ret 0xC`) | `v126_c_tail.py` |
| `0xBAF2DB` | 本体 prologue `push ebp; mov ebp,esp; sub esp,0xD58; push ebx/esi/edi; jmp 0xBAF2ED` | 同上 |
| `0xBAF2ED` | PE AddressOfEntryPoint（OptionalHeader+16 = RVA `0x7AF2ED` → VA `0xBAF2ED`） | `v126_a_entry.py` |
| `0xBAF2FB..0xBAF3AA` | 第1複号: `mov [ebp-0x88],0xBB1204` → `ecx=0xD9; rep movsd`(0x364B 複写) → 連鎖 XOR `dec[0]=c[0]^0x72C99B67`, `dec[i]=c[i]^c[i-1]` | 同上 |
| `0xBB1204` | 第1複号の暗号語源（0x364B） | 同上 |
| `0xBB1568` | 第2複号の暗号語源（0x51C00 B、第1複号領域の直後） | `v126_k_payload.py` |
| `0xC03168` | フッタ（設定ブロック）: 先頭 dword = マジック `0x815CFDB0`（`0xBAFAE3: cmp [eax],[ebp-0x3d0]` で検証） | `v126_j_config.py` |

### 2-2. 復元した設定構造体（第1複号後 = スタック `ebp-0x3f0` に 0x364B）

```
+0x00 0x18A799E0
+0x04 0x00A281A4  +0x0C 0x00A2811C  +0x10 0x00A28118  <- .rdata IAT slots
+0x18 0x00BAF000 (.bind base)   +0x1C 0x2204 (config ciphertext offset)
+0x20 0x815CFDB0 (footer magic = 第2複号の初期鍵)
+0x24 0x00C03168 (footer VA)    +0x28 0x1000 (footer size)  +0x2C 0x573A (=22330 Steam AppID)
+0x34 "022330" (先頭 '0' が成功ステータスコード [ebp-0x3bc] を兼ねる)
+0x3C 0x398 (XXTEA 文字列 ptr フィールド)  +0x40 0x4 (長さフィールド)  +0x44 0x3FC (鍵フィールド)
+0x48 "kernel32.dll" … +0xD8 以降 API 名 24 種
+0x2C4 "Local\SteamStart_SharedMemFile" … +0x304 "start" … +0x354 "-applaunch "
```

### 2-3. 本体フロー（逆アセンブル実測 = `%TEMP%\opencode\v126_stub_full.txt` 1,791 命令）

1. **API 解決**: `0xBAF58A` の `GetProcAddress` を手製 export 遍歴 `0xBAF005` で取得 → `[ebp-0x58]`。以降 24 種を解決（`ExitProcess=[ebp-0x434]` / `VirtualAlloc=[ebp-0x6c]` / `VirtualProtect=[ebp-0x64]` / `GetProcessHeap=[ebp-0x40]` / `HeapAlloc=[ebp-0x3c]` / `HeapFree=[ebp-0x38]` / `LoadLibraryExA=[ebp-0x5c]` / `GetModuleFileNameA=[ebp-0x50]` 等）。
2. **フッタ複写 + 第2複号** `0xBAFB04..0xBACFC1A`: `HeapAlloc(0,0x1000)` → `0xC03168` から 0x1000B 複写 → 連鎖 XOR（初期鍵 `0x815CFDB0`、`+4` 起点）でデコード。
3. **カスタム XXTEA 復号** `0xBAFC1B..0xBAFE48`（MX = `z -= (y+((y<<4)^(y>>5))) ^ (sum+key[(sum>>11)&3])`、rounds=0x20、delta `0x9E3779B9`、格納時に元暗号語と XOR（初期ガード `0x55555555`））。
   - 長さ `*(base+4)=0x51C00` / 暗号語 `*(base+0x398)=0xBB1568` / 鍵 `base+0x3FC={0xA239276D,0x16575AA5,0x41FFCC31,0xB23A3343}`。
   - 復号結果 = **PE32 `steamdrm.dll`**（PDB `d:\s3_main\bin\util\drm\steamdrm.pdb`、ImageBase `0x10000000`、entry RVA `0x1C757`、4 セクション、imports ADVAPI32/KERNEL32、export は `start` 1 本のみ、334,848 B）。保存先 `%TEMP%\opencode\v126_payload.bin`。
4. **マップ** `0xBAFEA3..0xBB0705`: `VirtualAlloc(preferred, SizeOfImage, 0x2000, 4)` → 失敗時 `VirtualAlloc(0,…)` → `VirtualAlloc(base,…,0x1000,4)` → セクション複写 `0xBACFFA1` → `[NT+0x34]=実配置基址` → imp/再配置ブロック `0xBB002D..0xBB04FD`（`GetProcAddress` 呼出し `0xBB049D`,`0xBB04CD`）→ セクション毎 `VirtualProtect` `0xBB0587..0xBB0700`。
5. **DllMain 呼出し** `0xBB0734`: `push 0 / push 1 / push mapbase / call [ebp-0x5c4]`（`[ebp-0x5c4]=mapbase+AddressOfEntryPoint`）= **DllMain(mapbase, DLL_PROCESS_ATTACH, NULL)**。
6. **エクスポート解決** `0xBB0766`: `call 0xBAF005(mapbase, "start")` → `[ebp-0x3f4]`。
7. **最終呼出し** `0xBB085D`: `push 0x1000; push config; call [ebp-0x3f4]` = `start(config,0x1000)` → `[ebp-0x430]`。
8. **動的転移** `0xBB08D1..0xBB097D`:
   ```
   cmp dword [ebp-0x430], 0xFF
   jb  -> エラーコード文字として 0xBB097F (ExitProcess 経路)
   jae -> バッファ zeroize -> 0xBB096B: mov eax,[ebp-0x430]
          add esp,0x1000; pop ebp/edi/esi/edx/ecx/ebx
   0xBB097D: jmp eax   <- スタブ内の間接転移はこの 1 本のみ（実測 125 jmp / 90 call 中）
   ```
9. `steamdrm.dll!start` の戻り値 = **フッタ `+0x318` に格納された元エントリ `0x98787C`**
   （`0x1000415A mov edx,[eax+0x318] → [0x10046D2C]`、成功末尾 `0x100044C7 mov eax,[0x10046D2C]; ret 8`。失敗時は `0x54('T')`/`0x33`/`0x35`/`0x36`/`0x50`/`0x51`/`0x53` 等のコードを返し `<0xFF` 判定でエラー扱い）。

### 2-4. v90 §5-1 / v93 未解決1への回答（経路確定）

```
PE entry 0xBAF2ED
 -> 連鎖XOR第1複号 (0xD9 dwords @0xBB1204, 鍵 0x72C99B67) -> 設定構造体
 -> API 24 種の手製 GetProcAddress 解決 (0xBAF005)
 -> 連鎖XOR第2複号 (0x1000B @0xC03168, 鍵 0x815CFDB0)
 -> カスタム XXTEA 復号 (@0xBB1568, 0x51C00 B) -> steamdrm.dll (PE32 @0x10000000)
 -> VirtualAlloc マップ + セクション複写 + imp/再配置 + VirtualProtect
 -> DllMain(mapbase,1,0) @0xBB073E
 -> GetProcAddress(mapbase,"start") @0xBB0773
 -> call start(config,0x1000) @0xBB0869 -> 戻り値 = 0x98787C
 -> 0xBB096B mov eax,[ebp-0x430] / epilogue
 -> 0xBB097D jmp eax  -->  0x98787C (元の CRT エントリ)
```
E8/E9/imm32/TLS/ポインタテーブルで 0 件だった理由: **転移先はフッタ `0xC03168+0x318` に暗号化され、実行時に初めて `[ebp-0x430]` へ展開される**ため静的走査では検出不能。

### 2-5. CRT 側（エントリ境界と名称 — v90 §5-2 / 0x981C6B / §70.10 の解決）

#### (a) 元エントリ 0x98787C と WinMainCRTStartup の境界 [確定]

```
0x98787C: e8 62 03 01 00   call 0x997BE3        ; ___security_init_cookie (台帳 exact)
0x987881: e9 16 fe ff ff   jmp  0x98769C        ; 唯一の入辺 (E8->0x98769C=0 件)
0x987886: 55 8B EC 83 EC 20 ...                ; = __vsnprintf_helper (台帳 exact・無関係)
...
0x98769C: 6A 60            push 0x60            ; WinMainCRTStartup 本体開始
0x98769E: 68 D8 FA AF 00   push 0x0AFFAD8       ; SEH scope table
0x9876A3: E8 F8 4E 00 00   call 0x98C5A0        ; __SEH_prolog4 (台帳 exact) -> ebp フレーム生成
...
0x98780E: E8 6D 67 A8 FF   call 0x40DF80        ; WinMain(0x400000, 0, cmdline, nShow)
...
0x987876: E8 6A 4D 00 00   call 0x98C5E5        ; __SEH_epilog4
0x98787B: C3               ret
0x98787C: (次の関数 = entry サンク)
```
- **WinMainCRTStartup = [0x98769C, 0x98787C) = 480 B**。prologue はインライン `55 8B EC` ではなく
  **`push 0x60; push scope; call __SEH_prolog4`**（= v90 の「prologue 所在不明」の正体）。
  epilogue は `call __SEH_epilog4; ret`（0x987876/0x98787B）。失敗経路は 0x987871 `mov eax,0xFF; ret`（= return 255）。
- **0x98717A = `__mbsnbcat_l` / 0x987886 = `__vsnprintf_helper`（いずれも台帳 exact）** → どちらも
  CRT 起動とは無関係な 4 引数の汎用関数で、v90 の「55 8B EC はこの2箇所にしかない」は**両者とも WinMainCRTStartup の prologue ではない**ことを意味する。
- **0x987699 `xor eax,eax; ret` は独立関数ではなく直前関数の共有 false 終端ブロック**:
  直前関数は **0x98765B**（`ret` 0x98765A の直後開始）で `cmp word [0x400000],'MZ' / cmp [e_lfanew+0x400000],'PE' /
  cmp [e_lfanew+0x400018],0x10B / cmp NumberOfRvaAndSizes(=e_lfanew+0x74),0xE / cmp CLR dir RVA(e_lfanew+0xE8),0`
  と並べ、全不一致を 0x987699 へ逃がして 0 を返す（= **PE 画像タイプ判定ヘルパ**、WinMainCRTStartup が 0x987761 で呼ぶ）。
  さらに 0x987637（= gap）は `__FF_MSGBANNER/__NMSG_WRITE` + `___crtExitProcess(0xFF)` の **fatal error ラッパ**
  （0x9876DA=0x12 / 0x987777=0x1C / 0x987788=0x10 の3種を呼ぶ）。
- **WinMainCRTStartup 内の対応付け（IAT 実測）**: `[0xA281D0] GetStartupInfoA` / `[0xA281CC] GetProcessHeap` +
  `[0xA28194] HeapAlloc`(0x94=148B = OSVERSIONINFOA) / `[0xA281C8] GetVersionExA` / `[0xA28198] HeapFree` /
  `[0xA281C4] GetCommandLineA` → グローバル `0xBA9D94=osplatform / 0xBA9D9C=_winver / 0xBA9DA0=_winmajor /
  0xBA9DA4=_winminor / 0xBA9D98=_osver` / `0xBABC04=_acmdln` に格納。
  その後 `0x997AAE` → `__setargv(0x9979F5)` → gap `0x997782` → gap `0x997725` → **`push ebx; call 0x981C6B`（0x9877E1）** → WinMain(0x98780E)。
- 台帳/デコンパイル: 0x98769C / 0x98787C / 0x981C6B / 0x40DF80 は**いずれも台帳 21,597 件に開始点なし・
  `FUN_` ラベルも decompiled.c に 0 件**（`dec_search '@ 0x98769c'` 等すべて空）。

#### (b) 0x981C6B（CRT 初期化ワーカ）の全解読と第1引数 [確定]

```c
int walk_startup(void* arg1 /* = ebx = 1 */) {          // 0x981C6B .. 0x981CFC (146B)
    if (*(void**)0xAA3E84 != 0)                          // .rdata 定数 = 常時 0x982897
        if (__IsNonwritableInCurrentImage(0xAA3E84))     // 0x98D8A2 (台帳 exact)
            ((int(*)(int))0x982897)(arg1);               // -> 引数1を唯一ここへ転送
    0x98D805();                                          // [0xB312A0..+0x28) の10 ptr を 0x98BE7F 経由で再符号化/復号
    if (FUN_00981bd8(0xA2F780, 0xA2F7A4)) return <nonzero>;  // _initterm_e 相当(6 slot) 失敗なら中断
    _atexit(0x98D7E1);                                   // 0x981CAA -> 終了時: 0xAD6E0C..0xAD6E0C(=空, no-op) 走査
    for (p=0xA283D0; p<0xA2F77C; ++p) if (*p) (*p)();    // 7,403 スロット走査 (0x981CC5..0x981CD4)
    if (*(void**)0xBABC18) (*(void(**)(int,int,int))0xBABC18)(0, 2, 0);  // bss=0 & 書込点0件 -> 実質不発
    return 0;
}
```
- **第1引数 = ebx = 1**: WinMainCRTStartup 内 `0x987769: xor ebx,ebx / inc ebx`（=1）で設定され、
  0x9877E0 の `push ebx` まで**再代入なし**（0x9876C5 の `[0xA281CC]` 読み込みはこれで上書き済み）。
  値 1 はワーカ内で**callback1(1) にだけ**渡り、意味は「DLL_PROCESS_ATTACH 相当の起動理由」（1 のときのみ
  callback1 内の追加初期化 `FUN_0099076F` を呼ぶ）。
- **`[0xAA3E84]` = 0x982897（.rdata 定数・毎回呼ばれる）**: gap 関数で
  `call FUN_00982837 / call FUN_009907D4(=GetModuleHandleA("KERNEL32")+GetProcAddress("IsProcessorFeaturePresent")(0)) → [0xBA9DE0] 保存 / arg!=0 なら FUN_0099076F / fnclex; ret`。
  = プロセッサ機能フラグ保存 + FPU 関連の追加初期化ラッパ。`__IsNonwritableInCurrentImage` ガードは
  MSVC の標準イディオム（.rdata の関数 ptr を画像内非書き込みと確認してから呼ぶ）。
- **`[0xBABC18]` = bss（ファイル無バック、既定0）**。全セクションで abs32 参照はワーカ内 3 箇所（すべて読み出し）のみ →
  **書込点 0 件 = 起動時に必ず 0 で、callback2(0,2,0) は本バイナリでは実行されない**（防御的フック）。
  近くの `0xBABC04=_acmdln / 0xBABC0C,0xBABC10=onexit テーブル ptr（0x981E9C/0x981F24/0x981F38/0x981F66 が参照）`。
- 表 `[0xA283D0, 0xA2F77C)` = **7,403 スロット**（NULL 24 / sync0xA10000 299 / 他）。呼び出し元は 0x9877E1 の 1 件のみ。

#### (c) §70.10「0xA28860..0xA2D184 初期化配列を走査する起動コード」[確定]

- **0xA28860 / 0xA2D184 を直接参照するコードは .text に 0 件**（abs32・imm32 全走査）。
  存在するのは外周境界のみ: `0x981CB5: mov esi,0xA283D0` / `0x981CBB: mov edi,0xA2F77C`。
- よって **走査者は 0x981C6B のインラインループ 0x981CC5..0x981CD4（範囲 [0xA283D0,0xA2F77C)）のみ**で、
  §70.10 の [0xA28860,0xA2D184) はその**内側の部分区間**（4,681 スロット = (0xA2D184-0xA28860)/4）。
- 内容（実測）: **4,681 件すべて .text ポインタ**（NULL 0）、値域 `0x9DC370..0xA08690`、**単調増加・全件 16B 整列**、
  unique=4,681。= 外周の 7,403 スロット（NULL/sync/バルク混在）の中で、**ブルク登録モジュール領域を指す連続ラン**として出現。
  事実上の `.CRT$XCU` 系の初期化関数列（v93 の「バルク登録モジュール 0x9D9D00..0x9FD000」と同域）。

#### (d) §70.10「0x981FB4・0x981F78 の役割」[確定]（台帳 exact）

- **`0x981F78 __onexit`**: `__SEH_prolog4(0xC, 0xAFF920)` → `FUN_00981BAE`（ロック）→ `push func; call __onexit_nolock(0x981E9C)` →
  `FUN_00981FAE`(= `FUN_00981BB7` アンロック) → 登録結果（非0=成功）を返す。
- **`0x981FB4 _atexit`**: `r = __onexit(fn); return (neg/sbb/neg) (r!=0) ? 0 : -1`（`dec eax` までで **0=成功 / -1=失敗**）。
  = v93 の記述を裏付け。ワーカ内では `0x981CAA: push 0x98D7E1; call _atexit` の 1 箇所のみ使用。

### 2-6. 動作経路の最終形（本タスクの中心主張）

```
[PE ローダ] AddressOfEntryPoint = 0x7AF2ED -> VA 0xBAF2ED (.bind)
[0xBAF2ED] 連鎖XOR第1複号: 0xD9 dword @0xBB1204, 鍵0x72C99B67 -> 設定構造体(0x364B)
[API解決]   手製GetProcAddress 0xBAF005 + 24 API 名(.bind 設定内)
[0xBAFB04] フッタ0xC03168(0x1000B) HeapAlloc 複写 -> 連鎖XOR第2複号(鍵0x815CFDB0)
[0xBAFC1B] カスタムXXTEA復号(長0x51C00 / 暗号語0xBB1568 / 鍵base+0x3FC) -> steamdrm.dll(PE32)
[0xBAFEA3] VirtualAlloc マップ + セクション複写 + imp/再配置 + VirtualProtect
[0xBB073E] DllMain(mapbase, DLL_PROCESS_ATTACH, 0)          ; entry RVA 0x1C757
[0xBB0773] GetProcAddress(mapbase, "start")  <- 0xBAF005 手製版
[0xBB0869] start(config, 0x1000) -> steamdrm.dll!start が
             - NtSetInformationThread(ThreadHideFromDebugger) 逆デバッグ
             - フッタ magic 0x815CFDB0 検証 / ファイル完全性検証
             - 成功時 [0x10046D2C] = footer[+0x318] = **0x98787C** を返す (0x100044C7)
[0xBB096B] mov eax,[ebp-0x430]; add esp,0x1000; pop 6reg
[0xBB097D] jmp eax  ==========================>  0x98787C
[0x98787C] call ___security_init_cookie -> jmp 0x98769C
[0x98769C] WinMainCRTStartup (__SEH_prolog4) -> ... -> call 0x40DF80 (WinMain)
```
E8/E9/imm32/TLS/ポインタテーブルで 0 件とされた理由を定量で裏付け:
- `.bind` の E8/E9→.text は 3 件のみで**全件が暗号ペイロード領域 [0xBB1568,0xC03168) 内のドリフト**（実コード外）。
- ファイル全体の生バイトに `abs32(0x98787C)` は **0 件**（複号後フッタ+0x318 に 1 件）。
- `.bind` スタブ内の間接転移は **`jmp eax` @0xBB097D の 1 件のみ**（125 jmp / 90 call 中）。
- E9→0x98769C は 0x987881 の 1 件、E8→0x98769C は 0 件、E8→0x981C6B は 0x9877E1 の 1 件。

## §3 結論（3行）

1. **経路確定**: PE entry 0xBAF2ED の連鎖XOR(0x72C99B67) → 設定構造体 → フッタ連鎖XOR(0x815CFDB0) → カスタムXXTEA → **steamdrm.dll を 0x10000000 に手動マップ** → `DllMain` → `start(config,0x1000)` の戻り値 **0x98787C** を `0xBB097D: jmp eax` へ → **CRT startup へ到達**（静的走査で 0 件だったのは転移先が複号後にしか存在しないため）。
2. **CRT 境界/名称**: 元エントリ **0x98787C = `call ___security_init_cookie; jmp 0x98769C` サンク**、本体 **WinMainCRTStartup = [0x98769C,0x98787C)（prologue は `__SEH_prolog4`・epilogue `__SEH_epilog4`、0x98780E で WinMain 呼び出し）**、`0x98717A=__mbsnbcat_l`・`0x987886=__vsnprintf_helper`（無関係）、`0x987699 xor eax,eax;ret` は直前関数0x98765B（PE 画像判定）の共有 false 終端。
3. **残点解消**: `0x981C6B` の第1引数は **ebx=1（起動理由、0x987769 で設定）** で `[0xAA3E84]=0x982897`（FPU/プロセッサ機能初期化ラッパ）へ転送されるだけ、`[0xBABC18]` は bss=0 かつ書込点0件で実質不発；§70.10 の `0xA28860..0xA2D184`（4,681 スロット）は **0x981C6B の走査範囲 [0xA283D0,0xA2F77C) の内側部分区間**で独立走査者は存在せず、`0x981FB4=_atexit(0/-1) / 0x981F78=__onexit` は登録系として確定。

## §4 残留不明点

1. `steamdrm.dll!start` の検証系内部（`0x100046C8` の戻りオブジェクト、`0x10003A6F` の照合、ファイル read/compare 経路）は
   **関数単位の意味論まで未解読**（経路確定には不要なため深掘りせず）。失敗時エラーコード 0x33/0x35/0x36/0x44/0x50/0x51/0x53/0x54 の対応表は未作成。
2. マップ済 steamdrm.dll の**再配置/インポート解決ブロック**（0xBB002D..0xBB04FD）は呼び出し構造のみ把握。
   .reloc(IAT=0x100476xx スロット) の個別解決表は未展開。
3. WinMainCRTStartup 内の gap 関数 `0x98D55E(1)` / `0x98C22E` / `0x98D7BD` / `0x98EEB9` / `0x997AAE` / `0x997782` / `0x997725` /
   `0x98D805` / `0x98D7E1` / `0x987637` / `0x98765B` / `0x982897` は**台帳・デコンパイルとも名称なし**（役割は呼び出し文脈から推定、確定名は未取得）。
4. `[0xAA3E84]` 直後の .rdata 構造（`0x981E65`/`0x982897`/`0x98D90E`×2 と "mscoree.dll"/"CorExitProcess" 領域）の
   構造体としては未同定（本ワーカは 0xAA3E84 単体を読む）。
5. 表 [0xA283D0,0xA2F77C) の 7,403 スロット全数の分類（v93 の 4 分類）に対し、本タスクは
   [0xA28860,0xA2D184) の 4,681 件のみ監査（全 .text・単調・16B 整列）。残区間の個別監査は未実施。
6. `0x98765B`（PE 画像判定）/ `0x987637`（fatal ラッパ）の**確定名**は未取得（Ghidra gap）。

## §5 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・最終時の両方）。
- 内部整合: `v126_w_verify.py` で **22/22 PASS**
  （C1 .bind E8/E9→.text=3 かつ全件が暗号ペイロード領域内 / C2 間接 jmp=1=0xBB097D /
  C3 生バイト abs32(0x98787C)=0・複号フッタ+0x318=0x98787C・+0x398=0xBB1568・+0=magic /
  C4 設定+0x24=0xC03168・+0x28=0x1000・+0x34='0'・+0x3C/+0x40/+0x44=0x398/4/0x3FC /
  C5 E8→0x98769C=0・E9=1(0x987881)・E8→0x981C6B=1(0x9877E1)・E8→0x98787C=0・E8→WinMain=1 /
  C6 ラン包含 7,403/4,681 スロット / C7 abs32(0xBABC18)=3 かつ全件ワーカ内 /
  C8 ペイロード 334,848B・steamdrm.dll・export 先頭=55 8B EC）。
- 追加実測数: 台帳照合 30 件（exact 16 / gap 14）、IAT 解決 10 スロット名、
  セクション abs32 走査 6 アドレス×6 セクション、.text 線形逆アセンブルは該当範囲のみ。
- 変更ファイルは本成果物 1 件のみ。中間スクリプトは `%TEMP%\opencode\v126_*.py`（a〜w の 12 本）、
  復号ペイロードは `%TEMP%\opencode\v126_payload.bin`。`docs/`・既存 `tmp/re`・`TODO_re49.md`・台帳・git・Supermemory は無編集。

## §6 Android 移植への示唆

移植上は本ブートストラップは**再現不要**（Steam DRM/Windows ローダ依存のため）。ただし移植時に移植すべきは
CRT 起動の**意味論**である: 元エントリ 0x98787C は「`___security_init_cookie`（GS クッキー）→ WinMainCRTStartup」
という MSVC 標準の起動順で、WinMainCRTStartup 自体が (1) GetStartupInfoA/GetVersionExA で
`osplatform/_winver/_winmajor/_winminor/_osver/_acmdln` を確立し、(2) `__setargv`/env 系、
(3) **0x981C6B による初期化表 [0xA283D0,0xA2F77C) の一括実行（=C++ グローバル ctor 列、内側 4,681 件が本命）**、
(4) `WinMain(0x400000,0,cmdline,show)`、(5) 終了時に `doexit` → onexit テーブル → 静的 dtor 配列
[0xA2F7B0,0xA2F7BC)/[0xA2F7C0,0xA2F7C8) という順を取る。移植版ではこの (3) の表相当（グローバル初期化の
登録順・一括実行）と (5) の逆順破棄を明示的に再現すればよく、`_atexit/__onexit` は 0=成功/-1=失敗の
契約だけ踏襲すればよい。また起動時に一度だけ走る 7,403 スロット走査はオーバーヘッドが大きいため、
移植側では初期化表をコンパイル時に静的に並べて順に呼ぶ形（同順・同内容）へ置換するのが安全。

---

## §12 二重実行の突合（本ノートの構成注記 + 独立再現の一致）

**構成注記**: 本ファイルは同一タスク V126 の**並列 2 実行**が同一パスを書いた結果、
**1〜504 行 = 実行 A**（§0〜§11、CFG 到達性・エラー経路表・raw 暗号バイト等を含む）、
**505 行目以降 = 実行 B（本実行、§2〜§6 + §12）**の追記、となっています。
見出し番号が重複するのはこのため（内容は矛盾せず、§-番号は実行 A のものが本体）。

### 12-1. 独立に一致した主要主張（2 実行の相互検証）

| 主張 | 実行 A | 実行 B（本実行） |
|---|---|---|
| 転移手段 | `0xBB097D: jmp eax`（スタブ唯一の間接転移） | 同（125 jmp/90 call 中 indirect = 1、`v126_w_verify.py` C2） |
| OEP 保管先 | 暗号ヘッダ `0xC03168+0x318` = `0x98787C` | 同（連鎖XOR 複号後 `heap+0x318` = 0x98787C、C3b） |
| 生バイトに `abs32(0x98787C)` | 0 件（file off 0x787680 に暗号バイト） | 0 件（全セクション raw count = 0、C3a） |
| payload 正体 | `steamdrm.dll` / export `start` @RVA 0x404F | 同（334,848 B、`v126_payload.bin`、C8） |
| `start()` 戻り値 | `0x100044C7 mov eax,[0x10046D2C]` = OEP | 同（`ret 8` = stdcall 2 引数） |
| `.bind` E8/E9→.text = 3 件 | 3 件とも XTEA 暗号文領域内 | 同（全件 [0xBB1568,0xC03168) 内、C1/C1b） |
| WinMainCRTStartup | 開始 `0x98769C`、prologue = `__SEH_prolog(0x60,0xAFFAD8)` | 同（`0x98C5A0=__SEH_prolog4` / `0x98C5E5=__SEH_epilog4` 台帳 exact） |
| エントリスタブ | `0x98787C = call __security_init_cookie(0x997BE3); jmp 0x98769C` | 同（`0x997BE3=___security_init_cookie` 台帳 exact、E9→0x98769C は 1 件のみ） |
| `0x98717A` / `0x987886` | 無関係な汎用関数 | **`__mbsnbcat_l` / `__vsnprintf_helper`（台帳 exact で同定）** |
| `0x987699 xor eax,eax; ret` | 前関数の共有 false 終端 | 直前関数 = `0x98765B`（MZ/PE/PE32/NumberOfRvaAndSizes/CLR 判定）の全不一致出口、`ret` 0x98765A 直後開始 |
| 0x981C6B 第1引数 | `ebx = 1`（0x987769） | 同（0x9877E0 `push ebx` まで再代入なし） |
| `[0xAA3E84]` | `0x982897`（FP 精度初期化フック） | 同 + 呼び出し先詳細: `FUN_009907D4 = GetModuleHandleA("KERNEL32")/GetProcAddress("IsProcessorFeaturePresent")(0)` → `[0xBA9DE0]`、arg≠0 で `FUN_0099076F`、`fnclex` |
| `[0xBABC18]` | 書手 0 件・初期値 0 → 起動時未呼出し、`cb(0,2,0)` | 同（abs32 参照 3 件すべてワーカ内読み出し、C7）＋ bss（rd は `no file backing`） |
| §70.10 走査者 | `0x981C6B` のインラインループ（範囲 `[0xA283D0,0xA2F77C)`） | 同（`0xA28860`/`0xA2D184` の直接参照は .text 0 件、外周のみ 0x981CB5/0x981CBC、C5/C6） |
| `0x981FB4` / `0x981F78` | `atexit` / `_onexit` | 同 + 内部: `__onexit = SEH_prolog → FUN_00981BAE(lock) → __onexit_nolock(0x981E9C) → FUN_00981BB7(unlock)` |

### 12-2. 本実行（B）が追加で得た固有情報

- **設定構造体の完全フィールド表**（`v126_bind_crt_bootstrap.md` §2-2 / +0x00..+0x354）と
  **24 API のスタブ内スロット割当表**（ExitProcess=0x434 / VirtualAlloc=0x6c / VirtualProtect=0x64 /
  GetProcessHeap=0x40 / HeapAlloc=0x3c / HeapFree=0x38 / LoadLibraryExA=0x5c / GetModuleFileNameA=0x50 等）。
- **第2複号のアルゴリズム完全形**: 連鎖XOR（初鍵 0x815CFDB0、+4 起点）→ カスタム XXTEA
  （MX=`(y+((y<<4)^(y>>5))) ^ (sum+key[(sum>>11)&3])`、rounds 0x20、delta 0x9E3779B9、**格納時に元暗号語と XOR**、
  初期ガード 0x55555555）、鍵 `{0xA239276D,0x16575AA5,0x41FFCC31,0xB23A3343}`、暗号語 0xBB1568 / 長 0x51C00。
- **WinMainCRTStartup 内の IAT 照合**: `GetStartupInfoA / GetProcessHeap / HeapAlloc(0x94=OSVERSIONINFOA) /
  GetVersionExA / HeapFree / GetCommandLineA` → グローバル `0xBA9D94 osplatform / 0xBA9D9C _winver /
  0xBA9DA0 _winmajor / 0xBA9DA4 _winminor / 0xBA9D98 _osver / 0xBABC04 _acmdln`。
- **gap 関数の役割同定**: `0x987637` = fatal ラッパ（`__FF_MSGBANNER`+`__NMSG_WRITE`+`___crtExitProcess(0xFF)`）、
  `0x98765B` = PE 画像タイプ判定、`0x98D7E1`（atexit 登録先）= `[0xAD6E0C,0xAD6E0C)` 空レンジ走査 = 実質 no-op、
  `0x98D805` = `[0xB312A0,+0x28)` 10 ptr を `0x98BE7F` 経由で再生成。
- **§70.10 部分区間の内容監査**: 4,681 スロット = 全件 .text・NULL 0・値域 `0x9DC370..0xA08690`・単調増加・全 16B 整列・unique 4,681。
- **内部整合 22/22 PASS**（`%TEMP%\opencode\v126_w_verify.py`：C1..C8、§5 参照）。

### 12-3. 本実行の証跡

`%TEMP%\opencode\`: `v126_a_entry.py`(PE/entry) / `v126_b_decode.py`(第1複号) / `v126_c_tail.py`(ヘルパ+末尾) /
`v126_d_scan.py`(.bind E8/E9・abs32) / `v126_e_epilogue.py`(0xBB097D) / `v126_f_slot.py` / `v126_g_success.py` /
`v126_h_slot3f4.py` / `v126_i_sweep.py`(スタブ全 1,791 命令 + `v126_stub_full.txt`) / `v126_j_config.py`(設定ダンプ) /
`v126_k_payload.py`(XXTEA 複号 → `v126_payload.bin`) / `v126_l_payload2.py` / `v126_m_start.py`(export start) /
`v126_n_ret.py`(start 末尾=OEP) / `v126_o_crt.py`(CRT 境界) / `v126_p_names.py`(台帳+IAT) / `v126_q_walkers.py` /
`v126_r_crtinit.py` / `v126_s_names2.py` / `v126_t_final.py` / `v126_u_probe.py` / `v126_v_last.py` /
`v126_w_verify.py`(22/22) / `v126_append{1,2,3}.py`。
selftest: **4/4 PASS**（着手時・最終時）。編集したリポジトリファイルは本成果物 1 件のみ。
