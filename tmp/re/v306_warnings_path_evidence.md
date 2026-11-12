# v306 — REF 変更状態フォーマッタ呼び出し元帯の「警告生成経路」静的棚卸し（命令証拠付き）

- 対象バイナリ: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam)
- セクション: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` VA 0xA28000→file 0x627200 (0xD9A00) / `.data` VA 0xB02000→file 0x700C00 (0x30C00)
- 手法: capstone 5.x 線形 disasm + int3(CC) 境界探索 + 整列不依存の imm32 全バイト scan + IAT スロット解決
- 中間物: `%TEMP%\opencode\v306_01..41.py/_out`
- 調査対象: フォーマッタ `FUN_00453A90` (0x453A90..0x457A8C, 16,384B, E8 呼び出し元 2 点のみ)

---

## 1. 件A — 両 caller の正確な関数範囲・構造・verbose ゲード

### 1.1 関数境界（CC=int3 パディング境界で確定）

| 項目 | caller1 | caller2 |
|---|---|---|
| call site | 0x45B029 | 0x45CD9F |
| **関数先頭 VA** | **0x45ADC0** | **0x45CC60** |
| 関数末尾 VA (直前の ret 直後) | 0x45B300 | 0x45CDF2 |
| サイズ | 0x540 = 1,344 B | 0x192 = 402 B |
| 先頭前の CC run | 0x45ADB5..0x45ADBF (11B) | 0x45CC5F (1B) |
| プロローグ証拠 | `45ADC0: sub esp,0x554` → cookie (`mov [esp+0x550],eax`) → push ebx/ebp/edi | `45CC60: sub esp,0x204` → cookie (`mov [esp+0x200],eax`) → push ebx/ebp/esi/edi |
| エピローグ証拠 | `45B2E6: mov ecx,[esp+0x55c]; pop edi/ebp/ebx; xor ecx,esp; call 0x9811E2; add esp,0x554; ret 4` | `45CDD9: mov ecx,[esp+0x210]; pop edi/esi/ebp/ebx; xor ecx,esp; call 0x9811E2; add esp,0x204; ret` |
| 呼び出し規約 | thiscall (ecx=this) + 1 stack 引数（path）、`ret 4` | thiscall (ecx=[0xB33B00]) + 4 stack 引数、`ret`（stdcall 相当・4 引数は callee cleanup。formatter 呼出し後 add esp 不要＝formatter も stdcall） |
| 呼び出し元 | 0x4657E1 (∈ FUN_00465130) / 0x4669AC (∈ FUN_00465860) — 2 点 | 0x40A9A5 の 1 点のみ |

**推定値の検証結果（手元推定 0x45AE65 / 0x45CC5F との照合）**

- caller1 推定 0x45AE65 → **誤り（訂正: 0x45ADC0）**。0x45AE65 は命令中間（`45AE63: push 0xa3a4cc` の即時部 `cc a4 a3 00` に該当し、"CC" は即時値であって padding ではない）。0x45AE64 の byte `0xCC` も `push 0xA3A4CC` のオペランドであり、CC 探索の偽陽性の典型例。実際の先頭は 11B CC run の直後 0x45ADC0（Graphify ノード `FUN_0045adc0` docs/Oblivion_decompiled.c L71794 **と一致**）。
- caller2 推定 0x45CC5F → **誤り（訂正: 0x45CC60）**。0x45CC5F の byte `CC` は実際に padding（直前 `45CC5C: c2 04 00 ret 4` の直後、1B padding）だが、それは**前関数の末尾 padding**であり関数先頭はその次の 0x45CC60。Graphify ノード `FUN_0045cc60`（L73272, community=FUN_006ecc80）**と一致**。0x45CD9F がこの関数内にあることも確認済（0x45CC60 ≤ 0x45CD9F < 0x45CDF2）。
- **v305 独立計測との照合**: v305 は「0x45B029∈FUN_0045ADC0」で**一致**、「0x45CD9F∈FUN_0045C9C0」で**不一致**。本計測では 0x45C9C0 は 8B CC run（0x45C9B8..0x45C9BF）の直後の別関数で、その終端は `45CC5C: ret 4`（+1B CC）＝0x45CC5F まで。つまり **FUN_0045C9C0 = 0x45C9C0..0x45CC5F** と FUN_0045CC60 = 0x45CC60..0x45CDF2 に分かれており、call site 0x45CD9F は後者に属する。v305 は 8B の長い CC run に引きずられ 0x45CC5F の 1B CC 境界を見逃した可能性が高い（証拠: 0x45CC5C の `ret 4` が FUN_0045C9C0 のエピローグ、0x45CC60 の sub esp,0x204 が新規 prologue、Graphify ノード FUN_0045cc60 の存在）。
- 注: caller1 の終端 0x45B300 は 0x45B2FD `ret 4` の直後だが、その直後から `45B300: push -1; push 0x9ae32e; mov eax,fs:[0] …`（SEH prologue、SLG 単例初期化＝`mov [0xb33b00],esi` @0x45B3C8 を含む別関数）が始まる。**CC padding が 0B の背中合わせ境界**であり、初回の線形 sweep（CC 停止条件）が 0x45B669 まで先の関数へ侵入した原因。

### 1.2 caller1 = FUN_0045ADC0（what-changed ファイル直接出力関数）

構造要約（命令証拠）:

1. **path 構築**: `arg0`（= セーブ系オブジェクト +0x3C の char[0x104] パスメンバ）を `lstrcpyA` (@0x45ADED, IAT [0xA28164]) でバッファへコピー → `lstrcatA` (@0x45AE00, IAT [0xA28160]) で拡張子 **".txt"（0xA3A4D4）** を付加。つまり出力先は **`arg0 + ".txt"`**。
2. **既存ファイル掃除**: `[0xB33A04]`（グローバル）非 null なら vtable+4 の関数に `(path, 0, 0, -1)` で呼叫 → 戻り値 !=0 なら `DeleteFileA` (@0x45AE31)。
3. **open**: `CreateFileA(path, GENERIC_READ|GENERIC_WRITE(0xC0000000), share=0, NULL, OPEN_EXISTING(4), FILE_ATTRIBUTE_NORMAL(0x80), NULL)` @0x45AE4E。失敗(h==-1)→0x45B2E6 のエピローグへ直行。
4. **ヘッダ書込**: `sprintf` (0x98208B) に format **0xA3A494 `  %-8s %-5s %-8s %02s %-40s  %s\r\n\r\n`** + 文字列 "Form ID"(0xA3A4B8)/"Size"(0xA363B8)/"Flags"(0xA3A4C0)/"V"(0xA3A4C8)/"Name"(0xA363D4)/"Changes"(0xA3A4CC) → `WriteFile` (IAT [0xA28098]) @0x45AEC9。
5. **レコード走査ループ** (0x45AEF0..0x45B15E): `[esi]`列の非 0 要素を走査 → フォーマッタ呼叫 **0x45B029** → フォーマッタ出力バッファ `[esp+0x36C]` を、format **0xA3A44C `  %08X %5i %08X %2i %-40s- %s\r\n`** の末尾 `%s` 引数として sprintf (@0x45B10D, 0x98208B) に合成 → `WriteFile` @0x45B13D。**what-changed 行はここで生成・出力される**。
6. **フッタ**: "Extra Stats:\r\n\r\n"(0xA3A3D4)、合計行 format 0xA3A3E8 ` \r\nTotal %s Num: %i …` を sprintf→WriteFile（@0x45B1A2/0x45B1CD/0x45B21D/0x45B26F/0x45B2D9）。
7. **close**: `CloseHandle` (IAT [0xA28094]) @0x45B2E0 → エピローグ。

フォーマッタへ渡す 5 引数（stdcall、push 逆順）:

```
45B016: push 0              ; arg4 = verbose gate = 0
45B018: push eax            ; arg3 = movzx byte [ebx+4]
45B019: push ecx            ; arg2 = dword [ebx+5]  (レコードの changeFlags)
45B01A: mov  ecx,[0xb33b00] ; this = SLG 単例
45B020: push ebp            ; arg1 = obj (0x46B250 の戻り値)
45B021: lea  edx,[esp+0x37c]; arg0 = 出力バッファ (= 呼出後 [esp+0x36C])
45B029: call 0x453a90
```

- **フォーマッタ結果 buf の行き先**: sprintf("…%-40s- %s") の `%s` → 1 レコード行 → WriteFile。直接的な到達先はファイルであり、ロガー FUN_004941A0 とは**接続しない**（後述 4.）。
- **verbose ゲードの決定源**: フォーマッタの arg4（エントリ [esp+0x14]、prologue `sub esp,0x154` + 4 push 後の **[esp+0x178]**）。caller1 はリテラル `push 0`（@0x45B016）。**実行時フラグではなく呼び出し点の定数**。ゲードはフォーマッタ内で 64 箇所すべて `cmp byte [esp+0x178],0` の読取のみで書込なし（0x453ADC, 0x453B76, 0x453B96, … 0x45796F）。

### 1.3 caller2 = FUN_0045CC60（SAVEGAME INFO コンソール通知関数）

構造要約:

1. `SAVEGAME INFO` (0xA3A740) を format として 0x57B8E0 へ（@0x45CCB7）— 数値 2 つと組み合わせたログ。
2. `call 0x55e000`（this=[eax]、args=edx, &local）@0x45CD15 → 戻り値を `test eax,eax; je 0x45CD6C`（**null チェック**）。
3. **BB-A（0x45CD6C..0x45CD8A）**: null → format **0xA3A6E8 `Current reference has no changes.`** を push → jmp 0x45CD89→0x45CDC1 で 0x57B8E0 呼出。
4. **BB-B（0x45CD22..0x45CD56）**: `[eax]` を取り出し `ecx=[0xB33B00]`、`push eax; push edi; call 0x4535A0`（CheckFlags）→ `test eax,eax; jne 0x45CD8B`。**eax==0** のとき: `fild [esp+0x10]` 等の float 整形 → `push 0xa3a70c`（**References changes were nullified by CheckFlags().**）→ `call 0x57B8E0`（@0x45CD52）→ jmp 0x45CDD9。つまり**この BB で「CheckFlags 戻り値 == 0」を検査している**。
5. **BB-C（0x45CD8B..0x45CDA4, フォーマッタ路）**: CheckFlags 結果 !=0 のとき:

```
45CD8B: movzx ecx, byte [edi+4] ; arg3
45CD8F: push 1                  ; arg4 = verbose gate = 1
45CD91: push ecx                ; arg3
45CD92: mov  ecx,[0xb33b00]     ; this = SLG 単例
45CD98: push eax                ; arg2 = CheckFlags 戻り値 (changeFlags)
45CD99: push edi                ; arg1 = obj (REF)
45CD9A: lea  edx,[esp+0x2c]     ; arg0 = 出力バッファ
45CD9E: push edx
45CD9F: call 0x453a90
```

6. フォーマッタ結果 buf → `push eax(=lea [esp+0x2c])` → **0x57B8E0**（コンソール/メッセージ系ロガー、@0x45CDC1）。エピローグ 0x45CDD9。

- **verbose ゲード**: caller2 はリテラル `push 1`（@0x45CD8F）。同 arg4=[esp+0x178]。よび caller1=0 / caller2=1 の**呼び出し点定数**で、ゲード=1 のときフォーマッタは例として 0x453B84 `push 0xa39f34`（`Process Level: None\r\n`）等の追加行を出す分岐を通る（0x453B76 `cmp [esp+0x178],0; je 0x453c33`）。
- フォーマッタは内部でも CheckFlags を再呼出: `453AC7: mov eax,[esp+0x170] (arg2); push eax; push ebp; call 0x4535a0; mov [esp+0x170],eax` — arg2 を検証結果で上書きして以降の整形に反映。

---

## 2. 件B — Warnings.txt 出力経路の接続（チェーンまたは負証拠）

### 2.1 'Warnings.txt' @0xA3D9BC への .text 内 imm32 参照（全件）

- **参照 site = 1 件のみ: 0x4944BC**。命令は `mov ecx, dword [0xa3d9bc]`（push ではなく 13B の**内容コピー**）。所在関数先頭 = **0x494480**（CC run 0x494474..0x49447F=12B の直後、prologue `xor eax,eax; jmp 0x494490` → コピーループ）。
- この関数は: ベースディレクトリ文字列 **0xB3F280** を 0xB34C80 へコピー → 末尾に `Warnings.txt`(0xA3D9BC, 13B incl NUL) を連結 → **`0xB34C80`（"＜base＞\Warnings.txt" 完成パス）を返す path getter**（`mov eax,0xb34c80; ret` @0x4944DD）。
- 同型の兄弟 getter（同じ 0xB34C80 を返す）: 0x4944F0(+16B=ProgramFlow.txt), 0x494560(+21B=ProgrammerOutput.txt), 0x4945E0(+15B=StackTrace…)。
- `.rdata` 内の他 'Warnings.txt' 出現（0xA38A03 `Check the "Warnings.txt" file…`, 0xA3AB3F/0xA3ABB0/0xA3AFD5 `See Warnings.txt for more info`）はいずれも**長文メッセージの一部**で、これらの長文の .text 参照は 2.3 に列挙（ファイル名リテラルとしては 0xA3D9BC のみが参照される）。

**getter の呼び出し元 6 site と用途（命令証拠）**

| ref site | 所在関数先頭 | 用途 |
|---|---|---|
| 0x494661 | 0x494650 | vtable=0xA3DA08 付与・`DeleteFileA` ×3（Warnings/ProgramFlow/ProgrammerOutput）・`[0xB34D90]=esi` — **スタート時の 3 ファイル削除コンストラクタ**（CRT 初期化 0x9DC337 から呼叫） |
| 0x4946CC | 0x4946B0 | `CreateFileA(path, GENERIC_READ, share=1, OPEN_EXISTING, …)` → GetFileSize==0x46 なら `DeleteFileA`（呼び出し元 0x40C4FA） |
| 0x494705 | 0x4946B0 | 同関数内の2回目: チェック通過後に `DeleteFileA` |
| 0x40E18F | 0x40DF74 | `DeleteFileA` ×3（Warnings/ProgramFlow/ProgrammerOutput を削除） |
| 0x44F762 | 0x44F3C1 | `push path; push 0xa37b78; call 0x4a7a60` — **path をメッセージとして log** |
| 0x51BDEC | 0x51B484 | `push path; push 0xa52a98; call 0x4a7a60` — 同上 |

**負証拠①: 6 site のいずれでも path を「書出ヘルパ」に渡していない（削除・読取チェック・path ログのみ）。`call 0x494730` の直接参照は 0 件。**

### 2.2 ロガー FUN_004941A0 へのチェーン

 FUN_004941A0 は巨大フレーム（`mov eax,0x32cc; call 0x983be0` = 0x32CCB のスタック確保）で、vswprintf 相当 `0x982820` により (arg0=ch(0..9), arg1=fmt, arg2=va) を整形 → **`[0xB34D90]`（＝2.1 のコンストラクタが作ったオブジェクト）の vtable** へ dispatch（jump table 0x494364, ch0..9）。

**接続チェーン（命令証拠つき）**

- **caller2 経由（到達可）**: `FUN_0045CC60 → 0x57B8E0`（@0x45CCB7/0x45CCEA/0x45CD52/0x45CDC1）`→ 0x571F90`（シングルトン 0xB3A6A4 生成）`+ 0x5723E0`（this=シングルトン、200 エントリ×0x1C のテーブルを走査）`→ 0x4A7A60`（**0x572624**: `cmp esi,0xc8; jne …; push 0xa68810; call 0x4a7a60` — テーブル満杯時のみ）`→ FUN_004941A0`（0x4A7A6C: `push eax(&va); push ecx(fmt); push 1(ch); call 0x4941a0; add esp,0xc`）。
- ただし **0x4A7A60 は ch=1 を固定**（`push 1`）し、ch1 の dispatch（0x494337）は `vtable[+4] = 0x4943A0 = xor eax,eax; ret 4` の**no-op** → **ロガー到達はするが出力先に届かない（断絶）**。
- **caller1 経由**: caller1 の call 列挙（0x45ADC0..0x45B2FF）に 0x4941A0 / 0x4A7A60 / 0x57B8E0 は**不在**（直接 CreateFileA/WriteFile/CloseHandle）。→ **負証拠: caller1 は FUN_004941A0 へ接続しない**。
- v301 既知の「ログシンク FUN_004941A0 の呼び出し元」は他に 0x404ECC/0x404EEC/0x40FECC/0x4A7A6C/0x533D3F/0x5E14CC の 6 site — いずれも caller1/caller2 の直列経路上には無い。

**書出ヘルパの実体（参考・本筋の裏付け）**

- `FUN_00494730`（vtable[10]）= 本物の**ファイル追記ヘルパ**: `EnterCriticalSection(0xB34E00)` → `CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING=4, NORMAL, NULL)` @0x4947DE → `SetFilePointer(h,0,0,FILE_END=2)` (IAT [0xA280A4]) → `WriteFile` @0x494818 → 末尾が CRLF で無ければ `WriteFile(0xA3D9B0="\r\n",2)` → `CloseHandle`。**ただし到達するのは ProgramFlow(0x494890→getter0x4944F0)/StackTrace(0x494950→getter0x4945E0) 経路のみで、Warnings.txt path を受け取る呼び出しは 0 件（負証拠）**。
- **負証拠②**: vtable 0xA3DA08（0x49465A/0x4946C8 の 2 参照のみ・実行時差し替えなし）の ch 到達先は、ch0=ExitProcess(0)、ch1/ch7=0x4943A0 no-op、ch2/ch4=0x68F970（`ret 4`）、ch3=0x494890(ProgramFlow 追記)、ch5=0x494950(StackTrace 追記)、ch6/ch8=0x4943E0/0x4943B0（0x4949E0=Confirm 系）、ch9=0x494950。**どの channel でも Warnings.txt は書かれない**。
- したがって v301 の「Warnings.txt は FUN_004941A0 系から出る」という前提は本バイナリでは**命令証拠上たてられない**。Warnings.txt path の実利用は **DeleteFile のみ**（起動時削除・サイズ 0x46 時削除）。

### 2.3 既知文言の基本ブロックと前後の検査（'See Warnings.txt…' 系メッセージ全件）

| 文言（先頭 VA） | 参照 site | 関数先頭 | 直前での検査 | 出力先 |
|---|---|---|---|---|
| `References changes were nullified by CheckFlags().` 0xA3A70C | 0x45CD4E | 0x45CC60 (caller2) | `call 0x4535a0; test eax,eax; jne 0x45cd8b`（CheckFlags 戻り==0 のときのみこの BB） | 0x57B8E0（コンソール系） |
| `Current reference has no changes.` 0xA3A6E8 | 0x45CD85 | 0x45CC60 (caller2) | `mov eax,[esp+0x18]; test eax,eax; je 0x45cd6c`（0x55E000 結果の null チェック） | 0x57B8E0 |
| `Forms were leaked during ClearData. Check Warnings file…` 0xA37C10 | 0x449C27 | 0x4492E0 | （ClearData 後の leak 検出） | 0x4A7A60 系メッセージ |
| `SaveTempEffectsList()…See Warnings.txt…` 0xA3AB00 | 0x45FC03 | 0x45FB50 | バッファ充足チェック | メッセージ系 |
| `LoadTempEffectsList()…See Warnings.txt…` 0xA3AB70 | 0x45FD77 | 0x45FC60 | バッファ空チェック | メッセージ系 |
| `LoadGame()…See Warnings.txt…` 0xA3AFA0 | 0x463A4E / 0x46671A | 0x463700 / **0x465860**（caller1 の caller 側関数） | 0x466700: `sub eax,ecx; sub eax,esi; je / cmp eax,-2; je` → 不一致時のみ | `mov ecx,[0xb34d90]; call [vtable+0x18]`(=0x4943A0 no-op) |

注: 0x46671A の出力先 `vtable[+0x18] = 0x4943A0`（no-op）——「Warnings.txt を参照する長文メッセージ」も現行 vtable では**書き出されない**。

---

## 3. 件C — CheckFlags() の正体

**結論: CheckFlags = FUN_004535A0（0x4535A0..0x453906、size 0x346 = 838 B）** — [特定]

命令証拠:

1. 参照元関数（caller2, 参照 site 0x45CD4E の BB）内での call target は 0x4535A0 ただ 1 本（`45CD2C: call 0x4535a0; test eax,eax; jne 0x45cd8b`）。その戻り ==0 のときにのみ '…nullified by CheckFlags().' BB (0x45CD35..0x45CD56) に入る → **戻り値 0 = "CheckFlags により changeFlags が nullify された"** を意味する。
2. フォーマッタ自身も同一関数を呼ぶ: `453ACE: push eax(arg2=flags); push ebp(obj); call 0x4535a0`（`mov [esp+0x170],eax` で arg2 スロットへ戻り値を書戻し）。
3. 関数本体（0x4535A0..0x453905, thiscall: ecx=[0xB33B00] 相当, `ret 8` = 2 stack 引数）は changeFlags の**ビット検証・補正**:
   - `4535EE: test ebx,0x10000000; je …; call 0x4cced0; test eax,eax; jne …; and ebx,0x0fffffff`（ビット清除）
   - `453607: test ebx,0x01000000; …; call 0x4af170; …; and ebx,0xfeffffff`
   - `453620: call 0x4c97f0; test al,al; jne 0x4538fb`（早期経路）
   - `453637: call 0x4c9a80 / 453640: call 0x4c9aa0` → `add esi,0x80; cmp esi,0xff; ja → or ebx,0x02000000` / `or ebx,0x04000000`（派生ビット付与）
   - `453681: test esi,esi; je 0x4538fb`（対象欠如時は補正省略でそのまま return）
   - 終端エピローグ `4538fb: pop edi; pop esi; mov eax,ebx; pop ebx; add esp,0x18; ret 8`（関数末尾 CC run 0x453906.., 10B）
   - 追加の検証系呼び出し: 0x41e6f0, 0x453530（前置）ほか。
4. 呼び出し元は 5 site: 0x453AD0（フォーマッタ内）, 0x45CD2C（caller2）, 0x45F1E3, 0x463B72, 0x465494 — すべて "flags 検査 → 0 なら警告/スキップ" の形。
5. 名前文字列 'CheckFlags' は単独では無く、文言 0xA3A70C 内の部分文字列（0xA3A731）のみ（独立したシンボル名は存在しない = 負証拠）。

補足: 戻り値の型は eax=changeFlags（ビット付き整数）。0 以外＝有効 change、0＝全 bit 無効化。caller2 はこの値をそのままフォーマッタ arg2 に渡し、フォーマッタ内でも再検証する（二重 CheckFlags）。

---

## 4. 判定サマリ

| # | 項目 | 判定 |
|---|---|---|
| A-1 | caller1 範囲 = 0x45ADC0..0x45B2FF（先頭 0x45ADC0）| **[確定]**（CC 境界＋プロローグ/エピローグ証拠） |
| A-2 | caller2 範囲 = 0x45CC60..0x45CDF1（先頭 0x45CC60）| **[確定]** |
| A-3 | 手元推定 0x45AE65 / 0x45CC5F はいずれも誤り | **[確定]**（訂正値を提示） |
| A-4 | Graphify ノード一致（FUN_0045adc0 / FUN_0045cc60）| **[確定] 一致** |
| A-5 | v305 归属との照合: 0x45B029∈FUN_0045ADC0 **一致** / 0x45CD9F∈FUN_0045C9C0 **不一致**（本計測は FUN_0045CC60）| **[確定]**（境界証拠つき、後述残点で仲裁要） |
| A-6 | verbose ゲード = フォーマッタ arg4 [esp+0x178]、caller1=0 / caller2=1 の呼び出し点定数、読取のみ 64 箇所 | **[確定]** |
| A-7 | フォーマッタ結果 buf の行き先: caller1=WriteFile 行 / caller2=0x57B8E0 コンソール | **[確定]** |
| B-1 | 'Warnings.txt'@0xA3D9BC の .text imm32 参照 = **1 site (0x4944BC)**、関数先頭 0x494480（path getter）| **[確定]** |
| B-2 | getter 6 caller = 削除・読取・path ログのみ（書出 0 件）| **[確定]（負証拠）** |
| B-3 | caller2→…→0x4941A0 チェーンは存在するが ch1→vtable no-op で断絶、caller1 はロガー非接続 | **[確定]（負証拠）** |
| B-4 | vtable ch 到達先に Warnings.txt 書出なし（書出ヘルパ0x494730 は ProgramFlow/StackTrace 専用）| **[確定]（負証拠）** |
| B-5 | caller1 の出力ファイル名 = arg0(object+0x3C path)+".txt" が実行時に "Warnings.txt" と一致するか | **[未確定]**（2.1 の path と未連結） |
| B-6 | 既知文言 3 系（caller2 2 文言 + See Warnings.txt 系 4 文言）の BB と前後検査 | **[確定]** |
| C-1 | CheckFlags = FUN_004535A0（0x4535A0..0x453906, 838B）| **[確定]** |
| C-2 | 戻り値 0 = nullify、戻り値 = 補正済み changeFlags | **[確定]** |
| C-3 | 独立した 'CheckFlags' 名前文字列の不在 | **[確定]（負証拠）** |

判定件数: **[確定] 13 / [部分] 0 / [未確定] 1**（B-5 のみ）。

---

## 5. 新規残点（次束用・低優先）

1. **[未確定] B-5**: caller1 の arg0（caller1 caller 側 FUN_00465130 @0x4657DD `lea edx,[esi+0x3c]` / FUN_00465860 @0x4669A8 `lea eax,[esi+0x3c]` の `esi` オブジェクト）の +0x3C に実行時に何が入るか。セーブ系オブジェクト型なら `<base>\Saves\<name>.ess`（0x45D52C の sprintf "%s%s%s" 帯）、別のオブジェクト型なら `<base>\Warnings` の可能性。`[esi+0x3c]` への書き込み site を横断検索（disp8/disp32 の lea/mov 全パターン、特に 0x465130..0x466B80 の外側）で確定可能。
2. **v305 归属の仲裁**: 0x45CD9F の帰属（FUN_0045CC60 vs FUN_0045C9C0）を Ghidra の関数境界表示と突合。1B CC（0x45CC5F）と即時値 CC（0x45AE64 等）の偽陽性パターンを v305/v307 に周知。
3. **fun_start ヘルパの off-by-pad**: CC run 起点を返す簡易ヘルパは run 先頭を返すため `run_start + run_len = 実先頭`。本棚卸しの表は補正済みだが、他レーン再利用時は注意（例: 0x465859→0x465860, 0x494474→0x494480）。
4. **FUN_00494730 の header 迴り**: `[0xB34D8C]` オブジェクト +0xE0/+8 からのヘッダ sprintf (0xA3DA34) 内容と、`[0xB34D90]` が null の場合の挙動（現状 path getter 経路と接続なし）。
5. **caller2 の 0x5723E0 テーブルの正体**: 0x1C×200 エントリの溜め込み先が最終的にどこへ flush されるか（本調査では 0x572624 のみ確認、他 flush 経路未特定）。
6. **0xA37B78 / 0xA52A98 のメッセージ文言**（Warnings path を引数に取る log 文言）の全文取得と、それらを処理するコンソール/ダイアログ経路の突合。

---

## 6. selftest 再現メモ

中間物はすべて `%TEMP%\opencode\`（= `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\`）に保存。再実行:

```powershell
chcp 65001
# 関数境界・CC run
python $env:TEMP\opencode\v306_41.py *> $env:TEMP\opencode\v306_41.out
# imm32 参照スキャン
python $env:TEMP\opencode\v306_4_refs.py *> $env:TEMP\opencode\v306_4_refs.out
# caller 関数 disasm 一式
python $env:TEMP\opencode\v306_6.py *> $env:TEMP\opencode\v306_6.out
# Warnings 文言の全出現→参照
python $env:TEMP\opencode\v306_40.py *> $env:TEMP\opencode\v306_40.out
python $env:TEMP\opencode\v306_39.py *> $env:TEMP\opencode\v306_39.out
# 書出ヘルパ・vtable
python $env:TEMP\opencode\v306_30.py *> $env:TEMP\opencode\v306_30.out
python $env:TEMP\opencode\v306_31.py *> $env:TEMP\opencode\v306_31.out
python $env:TEMP\opencode\v306_32.py *> $env:TEMP\opencode\v306_32.out
```

主要 selftest 結果（本棚卸しの根拠値）:

- `calls to 0x453A90: 2 -> ['0x45b029', '0x45cd9f']`（フォーマッタ呼び出し元は 2 点のみ・契約一致）
- `calls to 0x45ADC0: ['0x4657e1','0x4669ac']` / `calls to 0x45CC60: ['0x40a9a5']`
- `refs to 0xA3D9BC (.text): ['0x4944bc']`（1 件）
- `site 0x45B029 -> func start 0x45ADC0 (pad=11)` / `site 0x45CD9F -> func start 0x45C9C0 (pad=8)` ※後者は"直前の CC run 起点"であり、run 先頭+pad と 1B CC(0x45CC5F)/プロローグ証拠で 0x45CC60 に補正済み（v306_41 出力中の `bytes 0x45CC50..` hex が `…c20400cc81ec04020000…`＝`ret 4; CC; sub esp,0x204` を直接示す）
- `vtable 0xA3DA08[0..10] = 494390,4943a0,68f970,494890,68f970,494950,4943a0,4943e0,4943b0,4949e0,494730`
- `direct calls to 0x494730: []`（書出ヘルパ直接呼び出し 0 件）
- formatter `[esp+0x178]` uses: 64 箇所すべて `cmp byte ptr [esp+0x178], 0`（書込 0 件）

## 訂正（v310 統合時・2026-11-07）

後続束 `v310_warnings_flush_residuals.md` により以下を**訂正・確定**する（旧記述は削除せず本節で上書き裁定）:

1. **関数先頭 2 件の off-by-pad 訂正**: 本 artifact 記載の `0x44F3C1` / `0x51B484` は **CC run 起点の誤検出**であり、正しい関数先頭は **`0x44F3D0`**（0xA37B78 参照 0x44F769 を含む関数・範囲 0x44F3D0..0x44FA43）と **`0x51B490`**（0xA52A98 参照 0x51BDF3 を含む関数・範囲 0x51B490..0x51BE64）。[確定]
2. **§4 残点「0x5723E0 flush 経路」の裁定**: `FUN_005723E0`（0x5723E0..0x572840）は **"unique debug text" 200×0x1C テーブルの lookup/upsert であり flush ではない**。満杯時のみ 0x4A7A60 経由で `0xA68810 "Too many unique debug text items.
"` を ch=1 に出すのみ。ch1=0x4943A0 no-op ＝ Warnings 書出なしに対する**反証経路は存在しない [確定負]**（深さ 4/180 関数スイープ＋帯 0x570000-0x574000 raw スキャンでファイル I/O 0 件）。[確定]
3. **§4 残点「0xA37B78/0xA52A98 全文」の確定**:
   - `0xA37B78` = `"DataHandler: unrecognized form\r\nLook in the %s file for more info.\r\n"`（68+NUL・参照 1 件 0x44F769）
   - `0xA52A98` = `"Animation group note problem. See %s file."`（42+NUL・参照 1 件 0x51BDF3）
   - 両者とも getter 0x494480 の実行時パスを %s へ注入 → 0x4A7A60 → ch1 → no-op で**出力されない**。「See Warnings.txt」固定文言帯と同族別系統。[確定]
