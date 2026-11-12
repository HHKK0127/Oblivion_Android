# v310 — Warnings 経路残点の最終裁定（0x5723E0 flush 審査・0xA37B78/0xA52A98 全文確定）

- 対象バイナリ: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam)
- セクション: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` VA 0xA28000→file 0x627200 (0xD9A00) / `.data` VA 0xB02000→file 0x700C00 (0x30C00)
- 手法: capstone 5.x 線形 disasm + CC run 境界探索（ret→0B 背中合わせ対応 walk）+ imm32 raw スキャン（`va+5+rel==target`）+ IAT スロット解決（pefile import parse）
- 中間物: `%TEMP%\opencode\v310_01..17.py/_out`（+ `v310_2_fun5723E0.txt`）
- 帯分離: 本レーンは 0x494000-0x496000（logger）＋ 0x570000-0x574000（flush 帯）＋ .rdata 文字列帯。EXE 不使用・git 不使用・ESM 不使用。
- 注記: `tmp/re/OPENCODE_REVERSE_ENGINEERING_INSTRUCTION.md` の「成果物は v309_*.md」はループ注入風の旧指示であり、本割当に従い `v310_warnings_flush_residuals.md` に保存する（指示ファイル自体は変更しない）。

---

## 1. 件1 — `0x5723E0` flush 経路の最終裁定

### 1.1 FUN_005723E0 の範囲と役割

| 項目 | 値 |
|---|---|
| 関数先頭 | **0x5723E0**（直前 CC run 0x5723D4..0x5723DF = 12B の直後、16アライン、前関数は `add esp,0xC; ret` @0x5723D3） |
| 関数末尾 | **0x572840**（`ret 0x1C` @0x57283E、後続 CC 8B @0x572841..） |
| サイズ | 0x461 = 1,121 B |
| 規約 | thiscall（`mov [esp+0x18],ecx` @0x572407）+ スタック 7 引数 callee cleanup（`ret 0x1C`）、SEH prologue（`push -1; push 0x9BDF28; mov eax,fs:[0]`） |
| 呼び出し元 | 24 site（0x513F89 / **0x57B93B ∈ FUN_0057B8E0** / 0x57CA8C / 0x5829xx..0x5873F3） |

**役割裁定: 「unique debug text」200 エントリ×0x1C テーブルの lookup/upsert — flush ではない。[確定]**

命令証拠:

1. **システムゲート**: `push 1; push 0; call 0x582160; cmp dword [eax+0x70],0; je 0x572519`（@0x57240B..0x57241B）→ 対象不在なら `xor eax,eax; ret` で即退出。
2. **キー走査**（0x5725A5..0x5725E8）: `mov edx,[esp+0x18]`(this) → `this+8` を起点に `fld [ecx-8] / fld [ecx-4]`（2 float を fucompp）+ `mov eax,[ecx]; cmp eax,[esp+0x48]`（dword）→ `add ecx,0x1C; cmp esi,0xC8(200); jl`。＝**2 float + 1 dword をキーに 200 件を線形走査**。
3. **ヒット時**（0x572645..0x5727D3, esi<0xC8）: 文字列一致比較（`call 0x98262D` = strcmp 系）、オブジェクト refcount（`call [0xA2807C]`=InterlockedDecrement / `call [0xA28078]`=InterlockedIncrement、vtable 経由 `call edx` @0x5726C8）、記録の上書き（`fstp [esi] / [esi+4] / [esi+8] / [esi+0x18]`、`[esi+0xC]`=object、`[esi+0x10]`=string（`lea ecx,[esi+0x10]; call 0x4028D0` @0x5727D3））。
4. **ミス時**: 空スロット探索（`cmp dword [eax],0` + stride 0x1C、0x572600..0x572611）→ 空きあり = 0x572712 で新規構築（`call 0x571900` で文字列生成）。
5. **満杯時の唯一の外部吐き出し**（0x572613..0x572640）: `cmp esi,0xC8` で空きなし → `push 0xA68810; call 0x4A7A60` → `or eax,-1` で ret。
   - **0xA68810 全文 = `Too many unique debug text items. \n`**（35 文字 + NUL、.rdata 0xA68810..0xA68832、NUL @0xA68833）。[確定]
6. **ファイル I/O 0 件**: 関数内 direct call は {0x582160, 0x977C50, 0x4028D0, 0x576A30, 0x574A80, 0x401F20, 0x98262D, 0x571900, 0x6FF420, 0x4A7A60} のみ。IAT indirect は InterlockedIncrement/Decrement のみ（WriteFile/CreateFileA 等 0 件）。[確定]

補助:
- `FUN_0057B8E0` = 0x57B8E0..0x57B943（先頭は前関数 `ret` @0x57B8DF の **0B 背中合わせ**＋`push 1; push 0; call 0x582160` prologue、外部 call 304 site）。callee は **0x582160 / 0x571F90 / 0x5723E0 の 3 本のみ**、I/O 0 件。中身はゲート 2 段（`[eax+0x70]`, `[eax+0x1C]`）→ `call 0x571F90`（シングルトン生成、戻りを ecx に）→ `call 0x5723E0`。[確定]
- `FUN_004A7A60` = 0x4A7A60..0x4A7A74: `mov ecx,[esp+4]`(fmt); `lea eax,[esp+8]`(va); `push eax; push ecx; push 1; call 0x4941A0; add esp,0xC` → **ch=1 固定**（v306 記載を本体で再確認）。[確定]

### 1.2 反証経路の最終審査 → **[確定負]（反証経路なし）**

「ch1=0x4943A0 no-op ＝ Warnings 書出なし」を覆す経路が無いか、次の層で審査した。

| 層 | 範囲 | 結果 |
|---|---|---|
| A. 0x5723E0 直接 callee 9 本 | 1 レベル | ファイル系 IAT 0 件・interesting（0x494730/0x494480/0x4941A0/0x4A7A60 等）0 件 |
| B. 到達可能スイープ | **深さ4・180 関数**（CRT 帯 0x98xxxx-0x9Exxxx 除外） | hit は depth0 `0x5723E0→0x4A7A60`、depth0 `0x57B8E0→0x571F90/0x5723E0`、depth3 `0x581CC0→0x533D30` のみ。**WriteFile/CreateFileA 等 0 件** |
| C. ロガー出口の ch | 0x4A7A60 経路（ch=1 固定・本体確認）＋ 0x533D30 経路 | 0x533D30 は ch=引数経由だが、本ツリー内の唯一の呼び出し **0x582094 は `push 0xA6915C; push 1` ＝ ch=1**。他の 0x533D30 caller（0x533E39/0x53421C/0x534A1E/0x534BFE/0x534CA6）は本ツリー外 |
| D. FUN_004941A0 本体 | 0x4941A0..（jump table 0x494364 前まで） | direct call = 0x983BE0/0x982820/0x9811E2 のみ、IAT = ExitProcess のみ。`cmp [0xB34D90],0; je 0x49434B` = **null なら整形も dispatch もせず即 return**（fallback 出力なし）。vswprintf 失敗時も同 je で無出力 |
| E. ch 到達先（vtable 0xA3DA08） | ch0=0x494390（`push 0; call [ExitProcess]`）、ch1/ch7=**0x4943A0 `xor eax,eax; ret 4`（本体確認）**、ch3=ProgramFlow、ch5/9=StackTrace、ch8=直接 epilogue、ch>9 はデフォルト 0x494337→vtable[+4]=0x4943A0 | **どの ch も Warnings.txt 書出経路を持たない**（v306 負証拠②と一致、vtable 参照は 0x49465A/0x4946C8 の 2 点のみで実行時差し替えなし） |
| F. flush 帯 0x570000-0x574000 全体 raw スキャン | `FF 15 <IAT>` + `E8`（0x494730/0x494480/0x4A7A60/0x4941A0/0x45ADC0） | ファイル系 IAT = **0 件**（Interlocked のみ）。ch1 wrapper `call 0x4A7A60` = 12 点（0x570038/0x5700E7/0x5703CC/0x570411/0x570461/0x570FD7/0x571084/0x5710A2/0x571334/0x571379/0x5713C9/**0x572624**）・getter/logger 直接 0 件 |
| G. 同帯の表走査関数 5 本 | 0x571640..0x571716 / 0x571720..0x571782 / 0x571800..0x5718FB / 0x571DF0..0x571F66 / 0x572170..0x5723DA（`cmp 0xC8`+stride 0x1C で同定） | 全て I/O 0 件・logger 出口 0 件（InterlockedDecrement/free/0x571F90 等の in-memory 操作のみ）＝**テーブルに flush-to-file 経路は存在しない** |

**裁定: 0x5723E0 経由で Warnings.txt が書かれる反証経路は存在しない → [確定負]。**
0x5723E0 の役割は「テーブル upsert（別経路）」であり、v306 残点5「最終 flush」の概念自体が不適切だった（唯一の外部吐き出しは満杯エラー ch1）。帯 0x570000-0x574000 にもファイル I/O は存在しない（[確定負]）。

### 1.3 FUN_00465860 の 5 caller site 引数との整合 → **[確定] 整合**

- `FUN_00465860` = **0x465860..0x466AC5**（SEH prologue `push -1; push 0x9AE797; … sub esp,0x310` + cookie、epilogue `ret 0xC` @0x466AC3 = 3 スタック引数 callee cleanup、後続 CC 10B）。
- `callers(0x465860)` = **同じ 5 site のみ**、全 site で raw E8 検証 `va+5+rel==0x465860` 通過。

| site | 含有関数（範囲） | push 列（call 直前） | 引数（arg1,arg2,arg3） | v308 追補との照合 |
|---|---|---|---|---|
| 0x466B31 | fn@**0x466B00** (0x466B00..0x466B68) | `push 0` / `push 0xA3B144` / `push 0`（`mov ecx,esi`） | **(0, 'quicksave', 0)** | ✅ 0/'quicksave'（0xA3B144 全文 = `quicksave` を確認） |
| 0x466D5F | fn@**0x466D50** (0x466D50..0x466D70) | `push 0` / `push eax` / `push 0` | **(0, [ecx+0x1C0], 0)** | ✅ [ecx+0x1C0]（0x466D50: `mov eax,[ecx+0x1c0]`） |
| 0x5025F7 | fn@**0x502500** (0x502500..0x50262E) | `push ecx`(flag) / `push edx`(&local) / `push 0`（`mov ecx,[0xB33B00]`） | **(0, &buf, flag)** | ✅ 0 |
| 0x5AE1E6 | **FUN_005AE190** (0x5AE190..0x5AE216) | `push 0` / `push 0` / `push eax`（`mov ecx,[0xB33B00]`） | **(0, 0, [esi+0x4C])** | ✅ [esi+0x4C]（0x5AE1B3 `mov eax,[esi+0x4c]` → 0x5AE1B8 `jne 0x5AE1D6` → 0x5AE1E5 `push eax`、間に eax 書込なし） |
| 0x5B5DA8 | fn@**0x5B5D60** (0x5B5D60..0x5B5DE4) | `push 0` / `push 0` / `push eax`（`mov ecx,[0xB33B00]`） | **(0, 0, [[0xB33B00]+0x6C][0])** | ✅（0x5B5D96 `mov eax,[ecx+0x6c]` → 0x5B5D9D `mov eax,[eax]`） |

- **fn 先頭 0x466D50 / 0x5AE190 の CC ラン 0B 背中合わせを再確認**:
  - `0x466D4D = c2 04 00`（ret 4、直前関数の epilogue）→ 0x466D50 = `8b 81 c0 01 00 00`（`mov eax,[ecx+0x1c0]`）。直前域（0x466B00..0x466D50）に 0x466D50 への branch なし、.text 全体 raw スキャンでは **0x663286 `jmp 0x466D50`（tail call）のみ**。[確定]
  - `0x5AE18D = c2 08 00`（ret 8）→ 0x5AE190 = `53 56`（push ebx; push esi prologue）。.text 全体で 0x5AE190 への直接 call/jmp 0 件（間接到達と推定）。[確定]
  - **注意（同型トラップ）**: `0x5AE1D6` は ret @0x5AE1D5 の直後だが**関数先頭ではない**——0x5AE1B8 `jne 0x5AE1D6` が指す関数内 BB（ret は [esi+0x4C]==0 経路の epilogue）。「ret 直後＝新関数」と機械的に判定しないこと。[確定]
- **整合性**: `FUN_00465860` の call 全列挙（0x465860..0x466AC5）に 0x5723E0 / 0x57B8E0 / 0x533D30 / 0x4941A0 / 0x494480 / 0x494730 は**不在**。存在するのは `0x45ADC0`（what-changed `.txt` writer）と `0x4A7A60`（ch1 no-op）のみ。→ **5 site の引数・経路は 0x5723E0 裁定（ch1 no-op ＝ Warnings 書出なし）と矛盾しない＝整合 [確定]**。

---

## 2. 件2 — `0xA37B78` / `0xA52A98` の全文確定

### 2.1 全文（NUL/CRLF 境界まで）

| VA | 全文 | 境界証拠 |
|---|---|---|
| **0xA37B78** | `DataHandler: unrecognized form\r\nLook in the %s file for more info.\r\n`（68 文字 + NUL = 69B） | 前 string `…unknown type '%s'.` の NUL @0xA37B72 + パディング 0xA37B73..0xA37B77 → 0xA37B78 が厳密な先頭。終端 NUL @0xA37BBC（次は 0xA37BC0 `DataHandler: internal error`）[確定] |
| **0xA52A98** | `Animation group note problem. See %s file.`（42 文字 + NUL = 43B） | 前 token `FSTHorseSoft` の NUL @0xA52A94 + パディング 0xA52A95..0xA52A97 → 0xA52A98 が厳密な先頭。終端 NUL @0xA52AC2（次は 0xA52AC4 `%s: End frame is less than or equal to Start frame in "%s".`）[確定] |

- 全セクション（.text/.rdata/.data）imm32 参照は**各 1 件のみ**: `0xA37B78 → 0x44F769`、`0xA52A98 → 0x51BDF3`（.rdata 内のポインタテーブル参照なし）。[確定]

### 2.2 参照 site と用途

**① 0xA37B78 — 参照 0x44F769（`68 78 7B A3 00` = push 0xA37B78 @0x44F768）**

- 含有関数 = **FUN@0x44F3D0..0x44FA43**（先頭: CC run 0x44F3C1..0x44F3CF = 15B 直後 0x44F3D0、16アライン。外 call 0x4067C7 / 0x4645B0 で入口を検証。終端: `mov eax,1; pop ebx; add esp,0xC; ret 8` @0x44FA38..0x44FA43、後続 CC 12B）。
- 命令列:
  ```
  44F759: call 0x44f0c0            ; form 検査
  44F75E: test eax,eax
  44F760: jne 0x44f775             ; 戻り!=0 ならスキップ
  44F762: call 0x494480            ; Warnings path getter（<base>\Warnings.txt）
  44F767: push eax                 ; %s 引数 = 実行時パス
  44F768: push 0xa37b78            ; fmt
  44F76D: call 0x4a7a60            ; ch=1 wrapper
  44F772: add esp, 8
  ```
- 文脈: 0x44F735..0x44F77E のループ（`[esi+0x8D0]` 件のフォーム列 `[esi+edi*4+0x8D4]` を走査、`cmp edi,[esi+0x8d0]; jb`）。用途 = **DataHandler の unknown form をフォーム単位で報告し、警告ファイルの実行時パスを埋め込むメッセージ**。[確定]

**② 0xA52A98 — 参照 0x51BDF3（`68 98 2A A5 00` = push 0xA52A98 @0x51BDF2）**

- 含有関数 = **FUN@0x51B490..0x51BE64**（先頭: CC run 0x51B484..0x51B48F = 12B 直後 0x51B490、SEH prologue `push -1; push 0x9B76DC; mov eax,fs:[0]; sub esp,0x1A0` + cookie。外 call 0x436DCD。終端: `xor eax,eax; ret` @0x51BE62..0x51BE64、後続 CC 11B。線形 disasm が 0x51BDEC を連続カバー＝境界内 [確定]）。
- 命令列:
  ```
  51BDC9: push 0xa52ac4 ; push 0x4a7a60      ; 直前の関連メッセージ（End frame…）
  51BDD6: mov byte [esp+0x17], 1             ; フラグ立上
  51BDEA: je 0x51bdff                        ; [esp+0x17]==0 なら本メッセージ省略
  51BDEC: call 0x494480                      ; Warnings path getter
  51BDF1: push eax
  51BDF2: push 0xa52a98
  51BDF7: call 0x4a7a60                      ; ch=1 wrapper
  ```
- 用途 = **アニメグループ note 問題を報告し、%s に実行時 Warnings パスを埋め込むメッセージ**（前のアニメ系メッセージ発報後にエラーフラグが立っていた場合のみ発報）。[確定]

**両メッセージの出力先**: いずれも `0x4A7A60`（ch=1 固定）→ `FUN_004941A0` → jump table 0x494364 → 0x494337 → vtable 0xA3DA08[+4] = **0x4943A0 `xor eax,eax; ret 4` no-op** → **書出・表示ともにされない**。[確定負]（本文は warnings を参照するが、経路上どこにも出力されない）

### 2.3 'See Warnings.txt…' 系メッセージ帯との対応

| 区分 | 文言 VA | ファイル名の持ち方 | 出力先 |
|---|---|---|---|
| **固定文言帯**（v306 §2.3） | 0xA37C10 `Forms were leaked…Check Warnings file…` / 0xA38A03 / 0xA3AB00 / 0xA3AB3F / 0xA3AB70 / 0xA3ABB0 / 0xA3AFA0 / 0xA3AFD5 | リテラル内に `Warnings.txt`/`Warnings file` を**固定持ち** | 0x4A7A60 系 or vtable+0x18=0x4943A0 no-op（全て swallow、v306 §2.3） |
| **本件 2 文言** | 0xA37B78 / 0xA52A98 | ファイル名を持たず、**getter 0x494480 の実行時パスを %s で受け取る** | 0x4A7A60 → ch1 → 0x4943A0 no-op |

- 対応裁定: **同族（Warnings 参照メッセージ）だが別系統**——固定文言 vs 実行時パス注入。getter の 6 caller のうち「path をメッセージとして log」する 2 点（v306 §2.1 表の 0x44F762/0x51BDEC = 本件の `call 0x494480` site）が、まさに本 2 文言の発報 site である（v306 の "ref site" は getter call site、本調査の 0x44F769/0x51BDF3 は同一命令群の push 即時部位置）。[確定]
- つまり v306 §2.1 の「path ログ用途 2 site」＝件2 の 2 文言に**対応**し、いずれも ch1 no-op で終端する。ファイル名リテラルの .text 参照は 0xA3D9BC のみ（getter）という v306 の裁定とも矛盾しない。[確定]

---

## 3. 判定サマリ（5 段階）

| # | 項目 | 判定 |
|---|---|---|
| 1-1 | FUN_005723E0 = 0x5723E0..0x572840（CC run 12B 直後、`ret 0x1C`） | **[確定]** |
| 1-2 | 役割 = 200×0x1C テーブル lookup/upsert（**flush ではない**） | **[確定]** |
| 1-3 | 満杯時のみ 0x4A7A60（0xA68810 `Too many unique debug text items. \n`）、他吐き出し 0 件 | **[確定]** |
| 1-4 | FUN_004A7A60 は ch=1 固定（本体 `push 1`） | **[確定]** |
| 1-5 | FUN_0057B8E0 = 0x57B8E0..0x57B943、callee 3 本・I/O 0 件 | **[確定]** |
| 1-6 | 0x5723E0 ツリー（深さ4/180 関数）にファイル I/O・getter・FUN_00494730 到達 0 件 | **[確定]（負証拠）** |
| 1-7 | ロガー出口の ch：0x4A7A60=ch1 固定、0x533D30@0x582094=ch1 | **[確定]** |
| 1-8 | FUN_004941A0 は null [0xB34D90] 時無出力、IAT は ExitProcess のみ | **[確定]** |
| 1-9 | vtable 全 ch に Warnings.txt 書出先なし（ch1=0x4943A0 本体 no-op 再確認） | **[確定]（負証拠）** |
| 1-10 | **反証経路なし＝「ch1 no-op ＝ Warnings 書出なし」維持** | **[確定負]** |
| 1-11 | flush 帯 0x570000-0x574000 全体にファイル I/O 0 件（raw FF15/E8） | **[確定負]** |
| 1-12 | 同帯の表走査関数 5 本すべて I/O・logger 出口 0 件 | **[確定]（負証拠）** |
| 1-13 | FUN_00465860 = 0x465860..0x466AC5（`ret 0xC`） | **[確定]** |
| 1-14 | 5 caller site の E8・引数（0/'quicksave'・[ecx+0x1C0]・0・[esi+0x4C]・[[0xB33B00]+0x6C][0]） | **[確定] ×5** |
| 1-15 | fn 先頭 0x466D50/0x5AE190 は CC ラン 0B 背中合わせ | **[確定]** |
| 1-16 | 0x5AE1D6 は関数内 BB（`jne` target）であり関数先頭ではない | **[確定]** |
| 1-17 | FUN_00465860 は 0x5723E0/0x57B8E0 に非接続・0x4A7A60 のみ → 整合 | **[確定]** |
| 2-1 | 0xA37B78 全文（68 文字+CRLF×2、NUL 境界確定） | **[確定]** |
| 2-2 | 0xA52A98 全文（42 文字、NUL 境界確定） | **[確定]** |
| 2-3 | .text 参照 = 各 1 件（0x44F769 / 0x51BDF3）、.rdata ポインタ参照なし | **[確定]** |
| 2-4 | 含有関数 = FUN@0x44F3D0..0x44FA43 / FUN@0x51B490..0x51BE64 | **[確定]** |
| 2-5 | 用途 = unknown form 報告 / アニム note 報告、%s=getter 実行時パス | **[確定]** |
| 2-6 | 出力先 = 0x4A7A60→ch1→0x4943A0 no-op（両文言とも出力されない） | **[確定負]** |
| 2-7 | 'See Warnings.txt' 帯との対応 = 同族別系統（固定文言 vs 実行時パス注入）、終端は全て swallow | **[確定]** |
| 2-8 | v306 の関数先頭 0x44F3C1 / 0x51B484 は **CC run 起点（off-by-pad）**、実先頭は 0x44F3D0 / 0x51B490 | **[確定]（訂正）** |

判定件数: **[確定] 23 / [部分確定] 0 / [推定] 0 / [未確定] 0 / [確定負] 4**（合計 27 行、訂正 2 件を含む）。

---

## 4. 新規残点（次束用・低優先）

1. **[未確定] 深さ5 以降の到達探索**: 件1-B のスイープは深さ4・180 関数で打ち切り。ただし vtable 全 ch に Warnings 書出先が無いため（1-9）、実務上の影響は軽微。全 ch の vtable 差し替えが無いこと（0x49465A/0x4946C8 の 2 参照のみ）を再独立確認する価値あり。
2. **[推定] fn@0x502500 / fn@0x5B5D60 / FUN_005AE190 の外部到達は間接呼び出し**: .text 全体 raw スキャンで直接 call/jmp が 0 件（0x466B00 は 0x672F56/0x673955 の直接 call あり、0x466D50 は 0x663286 の tail jmp あり）。vtable/関数ポインタの参照元特定は未実施。
3. **[未確定] ch0 wrapper `FUN_00404EC0`（0x404EC0..0x404ED4, `push 0; call 0x4941A0`）の実挙動**: FUN@0x44F3D0 は 0x44F5F3 `push 0xA3882C; call 0x404EC0` で呼ぶ。ch0 の到達先 0x494238→vtable[0]=0x494390=`push 0; call [ExitProcess]` は「致命系ログ＝即終了」に見えるが、ch0 呼出時の実際の終了挙動は未照合（本帯の判定外）。参照: 0x404EE0 は `push 3`（ch3=ProgramFlow 追記）wrapper。
4. **[未確定] FUN@0x44F3D0 内 `call 0x44F0C0` の戻り値契約**: 戻り==0 のときのみ 0xA37B78 を発報（`jne 0x44f775` スキップ）。0x44F0C0 の戻り値意味（unknown 判定か再発抑制か）は未特定。
5. **[推定] flush 帯の表走査関数群の役割名**: 0x571640/0x571720/0x571800/0x571DF0/0x572170（pattern: stride 0x1C + cmp 0xC8、0x571DF0 は InterlockedDecrement ループ＝entry 破棄と推定）は I/O なしを [確定負] 済みだが、各関数の厳密な役割（clear/get/reset）は未同定。
6. **解消した残点**: v306 残点5（0x5723E0 最終 flush → upsert と裁定・帯内 I/O 0 件）、v306 残点6 / v308 §3（0xA37B78/0xA52A98 全文 → 本件 2-1/2-2 で確定）、v306 §2.1 の関数先頭 off-by-pad 2 件（0x44F3C1→0x44F3D0、0x51B484→0x51B490）。

---

## 5. selftest 再現メモ

中間物はすべて `%TEMP%\opencode\`（= `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\`）。再実行:

```powershell
chcp 65001
python $env:TEMP\opencode\v310_1.py  *> $env:TEMP\opencode\v310_1.out   # 0x5723E0 境界・callers
python $env:TEMP\opencode\v310_2.py  *> $env:TEMP\opencode\v310_2.out   # FUN_005723E0 全文 disasm
python $env:TEMP\opencode\v310_3.py  *> $env:TEMP\opencode\v310_3.out   # 0xA37B78/0xA52A98 全文+参照
python $env:TEMP\opencode\v310_4.py  *> $env:TEMP\opencode\v310_4.out   # ref site 文脈・0x4A7A60 本体
python $env:TEMP\opencode\v310_5.py  *> $env:TEMP\opencode\v310_5.out   # callee 1 レベル監査+IAT 名
python $env:TEMP\opencode\v310_6.py  *> $env:TEMP\opencode\v310_6.out   # 深さ2 監査+0x57B8E0 本体
python $env:TEMP\opencode\v310_7.py  *> $env:TEMP\opencode\v310_7.out   # 0x533D30 ch・fn先頭 CC run
python $env:TEMP\opencode\v310_8.py  *> $env:TEMP\opencode\v310_8.out   # walk 型 fun_start+5 caller site
python $env:TEMP\opencode\v310_9.py  *> $env:TEMP\opencode\v310_9.out   # int3 対応 walk+'quicksave'
python $env:TEMP\opencode\v310_10.py *> $env:TEMP\opencode\v310_10.out  # プロローグ証拠・0x5AE1 歸属
python $env:TEMP\opencode\v310_11.py *> $env:TEMP\opencode\v310_11.out  # 関数終端・全セクション xref
python $env:TEMP\opencode\v310_12.py *> $env:TEMP\opencode\v310_12.out  # 深さ4 スイープ+0x404EC0
python $env:TEMP\opencode\v310_13.py *> $env:TEMP\opencode\v310_13.out  # 0x4941A0 監査+全 .text branch scan
python $env:TEMP\opencode\v310_14.py *> $env:TEMP\opencode\v310_14.out  # 0x4941A0 null/jump table
python $env:TEMP\opencode\v310_15.py *> $env:TEMP\opencode\v310_15.out  # IAT 名+帯内 0xC8/0x1C 走査
python $env:TEMP\opencode\v310_16.py *> $env:TEMP\opencode\v310_16.out  # 表走査関数 5 本の監査
python $env:TEMP\opencode\v310_17.py *> $env:TEMP\opencode\v310_17.out  # 帯 raw FF15/E8 スキャン
```

主要 selftest 値（本裁定の根拠）:

- `FUN_005723E0: start 0x005723E0 -> end 0x00572841 postCC=8` / `callers(0x5723E0): 24 sites`（0x57B93B を含む）
- `callers(0x465860): 5 call sites: 00466B31 00466D5F 005025F7 005AE1E6 005B5DA8`（E8 rel 全通過）
- `FUN_004A7A60` 本体: `push va; push fmt; push 1; call 0x4941a0`（ch=1 固定）
- `refs to 0xA37B78: ['0x44f769']` / `refs to 0xA52A98: ['0x51bdf3']`（全セクション）
- `0xA37B78 = 'DataHandler: unrecognized form\r\nLook in the %s file for more info.\r\n'`（len 68）
- `0xA52A98 = 'Animation group note problem. See %s file.'`（len 42）
- `0xA68810 = 'Too many unique debug text items. \n'`
- `0xA3B144 = 'quicksave'`
- 深さ4 スイープ: `visited functions: 180`、hit = 3（0x5723E0→0x4A7A60 / 0x57B8E0→chain / 0x581CC0→0x533D30）、writeish 0
- flush 帯 raw: ファイル系 IAT 0 件、`call 0x4A7A60` 12 点（0x572624 含む）
- logger 帯 raw: WriteFile は 0x494818/0x49484D（FUN_00494730 内）、getter call は 0x494661/0x4946CC/0x494705 のみ
- `FUN_004941A0: cmp [0xb34d90],esi; je 0x49434b`（null→無出力）/ jump table 0x494364 = {0x494238,0x494337,0x494261,0x49428B,0x4942B5,0x4942DF,0x4942EC,0x494300,0x49434B,0x49430D}
- `0x4943A0: 33c0 c20400`（xor eax,eax; ret 4）
- global branch scan: 0x44F3D0←call 0x4067C7/0x4645B0、0x51B490←call 0x436DCD、0x466D50←jmp 0x663286、0x466B00←call 0x672F56/0x673955

## 追補（次束D＝v314 統合時・2026-11-07）

`v314_v310_residuals.md`（29,417B/195行・probe 8 本）により本 artifact の新規残点 5 件は**全件解消**され、以下を追記する:

1. **深さ 5 以降の打切り → 打切り点まで走査して確定**: roots 再現の到達固定点 723 関数・深さ 19（緩和版 3000/深さ 20）まで到達し、`WriteFile`/`CreateFileA`/`0x494730` 到達 **0 件** → 「Warnings 書出なし」は深さ 5 以降へ拡張して **[確定]**（間接 call 595 件は留保＝新残点④）。
2. **間接到達 3 fn の役割を確定**: `0x502500`＝`.data 0xB0BF78` の opcode テーブル（LoadGame 行と一致）／`0x5B5D60`・`0x5AE190`＝`push imm → call 0x579C10` の cb 登録。役割＝load コマンド impl ／ msgbox CONTINUE ／ LoadMenu 行選択 [確定]。
3. **ch0 wrapper `0x404EC0` の実挙動**: `ch0 → vtable[0] = 0x494390 → IAT ExitProcess` ＝整形メッセージ無出力・`ExitProcess(0)` 即終了（logger NULL/整形失敗時は return）[確定]。
4. **`0x44F0C0` 戻り値契約**: 範囲 0x44F0C0..0x44F3A7・戻り値は `[esp+0x18]` の単方向フラグ（初期 1／`0x44DCF0` 失敗時のみ 0）＝**unknown 判定で再発抑制ではない** [確定]（0 条件の発生経路のみ [部分確定]）。
5. **表走査 5 本に役割名付与**: read×2（get-and-copy/get-ptr）・全件 clear×1・dtor/init×2・期限 tick×1。**本 artifact の「InterlockedDecrement ループ」記述は裁定**＝単発 `Release`＋範囲 2 fn 融合（短 CC 境界）で、旧記述と両論併記のうえ v314 の裁定を正とする [確定]。

（v310 残点 1〜5 は 5/5 解消。新規残点 5 件＝①0x502500 ディスパッチ基址 ②[B33E9C]=195.06 の意味 ③0x44DCF0 の 0 条件 ④間接 call 595 ⑤fn-end walker 短 CC 境界改良 → #83。）
