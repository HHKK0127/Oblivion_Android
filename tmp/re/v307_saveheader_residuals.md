# v307 — .ess ヘッダ経路 残点10件の静的解析棚卸し（V287 / ai-todo #68 帯）

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000、7,898,624 B）
- 日付: 2026-11-06 / 担当: 静的解析サブエージェント（v307 のみ。v304/v305/v306・台帳・STATE・docs・GT・ソースは不変更）
- 帯境界: 本件 0x45D000..0x466000（ヘッダ読/書・SaveGame 管理）＋ ZLIB ラッパ帯（FUN_450550/42C6F0/46B2C0/46B370/4501F0）。formatter 帯 0x45A000..0x45D000 と FUN_00453A90 本体には深入りしない（v306 担当）。
- セクション（PE ヘッダ実測・契約値一致）: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` VA 0xA28000→file 0x627200 (0xD9A00) / `.data` VA 0xB02000→file 0x700C00 (0x30C00)。変換は `F(va)=0x400+(va-0x401000)` 等の section 基準で統一。
- ツール: python 3.11 + capstone 5.0.7（`md.detail=True`）、PE インポート目録を自前パース（IAT 230 エントリ）。
- **方法注（先頭規則の実測調整）**: 契約の「直前≥4 バイト CC ラン」は実バイナリで 1 件破れを確認 — **FUN_00464AB0 の CC ランは 3**（v305 が "CCラン3" と記録済み）。本束では minrun=3 で統一し、CC ラン 2 の 0x45F2E0 は規則外のため**直接 disasm で SEH プロローグ（push -1 / push 0x9AE52C / fs:[0] / sub esp,0x628）を確認**して先頭と裁定。imm32/raw 検索はすべて命令境界検証（関数先頭からの線形 disasm、デスシンク時は 1 バイト再同期）で採否を判定。

---

## 1. 残点ごとの結果

### 残点1: FUN_0045DA30 の役割分担（読取 FUN_45DBC0 との関係） — **[確定]**

- **[確定] 0x45DA30 は関数先頭ではない**: 16 アラインだが直前 CC ラン=0、先頭バイト 0x00。直前の命令は `0x45DA2B: sub eax, dword [esi+0x710]`（6 バイト、0x45DA2B..0x45DA31）で、0x45DA30 はその disp32 内 = **mid-instruction**。E8 呼び手・imm32 参照とも 0 件（線形 disasm のゴーストは `add [ecx],al` 等）。
- **[確定] 真の包含関数 = FUN_0045D920**（0x45D920..0x45DBB0）:
  - 先頭: 16 アライン + 直前 CC ラン4 + `push -1 / push 0x9AE3FB / fs:[0] / sub esp,0x230`（SEH プロローグ）
  - 末尾: `0x45DBB0: ret 4` + CC パディング 0x45DBB3..0x45DBBF → 次 FUN_45DBC0
  - E8 呼び手 = **1 箇所 0x45F370**（FUN_45F2E0 内、`lea eax,[esp+0x430]; push eax; call 0x45d920`）
- **[確定] 役割 = セーブスロットの説明文生成 + .ess ファイル名一意化**（XFER ロギング帯ではない）:
  - 文字列: `"Save %i - %s - %s, Level %i, Playing Time %02i.%02i.%02i"` @0xA3A8A4 / `" #%d"` @0xA3A8E0 / `"\/:*<>?|""` @0xA3A898（call 0x985480 = strchr 系、不正文字を置換ループ 0x45DAE0..0x45DB04）/ `"%s%s%s.ess"` @0xA3A88C
  - `mov edi,[0xA280D0]` ×2 = **GetTickCount**（IAT 確認）→ `[esi+0x710]/[esi+0x714]` へ経過時間累積（`sub eax,[esi+0x710] / add [esi+0x714],eax`）
  - 乗算逆数 3 段（`0x95217CB1>>21` / `0x45E7B273>>14` / `0x10624DD3>>6`）= 秒→時分秒のインライン除算 → `push edi/ebp/esi` で call 0x5E1FD0 → "Playing Time" 引数化
  - `[0xB33A04]` の仮想呼び出し `f(path, 0, 0, -1)`（ret 前の `test eax / sete al / je 0x45d9f2`）= 重複ファイル回避ループ（" #%d" 連番）
- **[確定] 契約記載「0xB02F9C 系ログ文字列＋0x9832E6 呼出」は本関数に不在（否定）**。それらの実体 = **FUN_0045DE30**（16 アライン + CC ラン10 検証、呼び手 0x4627E1）: `push 0 / push 0xB03248|0xB03800|0xB03718 / push 0xB02F9C / push 0 / push obj / call 0x9832E6`（0xB02F9C = `.data` の RTTI TypeDescriptor `".?AVTESForm@@"` 起点 → **RTTI 問い合わせ相当**）、文字列 `"Enchantable item %08X with no enchantment exists in the created base objects list."` @0xA3A960、`call 0x46B250 / 0x446B80` → 作成ベースオブジェクト検証。
- **[確定] v305 の範囲表記を訂正**: FUN_45D5F0 の実範囲は **0x45D5F0..0x45D919**（`ret 0x28` @0x45D919、CC4 @0x45D91C..0x45D91F）。v305 の "0x45D5F0..0x45DBB6" は FUN_45D920 までを含む過大表記。ゲート 7 サイト（0x45D699..0x45D84A）はすべて 0x45D919 未満で、v305 の FUN_45D5F0 帰属と矛盾なし。
- **Graphify 一致行**: 「27.3 セーブスロットの表示/保存まわりで判明した文字列定数」(L2820) と**一致**（"%s%s%s.ess" / "Save %i..."）。`FUN_0045DA30` という関数名のノードは**不一致**（非関数であることが本件で判明）。

### 残点2: 'CON ' 4バイト一致分岐の意図 — **[確定]（機構）／[推定]（用途）**

- **[確定] 参照は 1 サイトのみ**: `0xA3A954 ("CON ")` の .text imm32 参照 = **0x45DC1B**（FUN_45DBC0 内）。
  ```
  0x45DC14: push 4 / lea eax,[esp+0x18] / push 0xA3A954 / push eax / call 0x984166
  0x45DC28: test eax,eax / jne 0x45dc64        ; 不一致 → TES4SAVEGAME 検査へ
  0x45DC2C: mov eax, dword [0xA853D0]          ; = 0x0（v305 の "=0x78?" は誤り）
  0x45DC33: mov edx,[edx+0xC] / push eax / push 0xD000 / mov ecx,esi / call edx
                                                     ; seek(offset=0xD000, origin=0=SEEK_SET)
  0x45DC40..0x45DC61: vtable+4 で 0xC バイト再読（buf）
  0x45DC4D: mov dword [edi+0x8c], 0xD000       ; this+0x8C ← 0xD000（現在オフセット記録）
  0x45DC64..: memcmp 相当で "TES4SAVEGAME" 検査（一致で継続 / 不一致で ret 0）
  ```
  不一致時は現在位置（offset 12）から直に TES4SAVEGAME 検査。read 失敗時（0x45DC83）は `seek(0, origin=[0xA853D0]=0)` = 先頭 rewind。
- **[確定] 0xA853D0/0xA853D4/0xA853D8 = seek origin 定数 0/1/2（SEEK_SET/CUR/END）** — 決定的証拠は **seek 実装 FUN_0042FF80**（= vtable 0xA3636C の +0xC スロット、残点6）:
  ```
  0x42FF85: cmp ebx, dword [0xA853D0]   ; ebx = origin引数 → 0 なら SET 分岐
  0x42FFAB: cmp ebx, dword [0xA853D4]   ; 1 = CUR（[esi+0x148]=現在位置へ相対変換）
  0x42FFC1: cmp ebx, dword [0xA853D8]   ; 2 = END（vtable+0x1C=サイズ取得 → size-offset）
  ```
  rdata 実値: [0xA853D0]=0, [0xA853D4]=1, [0xA853D8]=2（連続 0,1,2 列）。0xA853D0 の参照 26 サイトはすべて seek origin 引数。
- **[確定] 第 2 の 0xD000 site = 0x45E812**、包含関数 **FUN_0045E6A0**（16 アライン+CC ラン11 検証、呼び手 0x5AEB8B/0x5D3AAA = ロード系）: `push [0xA853D0] / push 0xD000 / call seek` ＋ `add ebp,0xFFFF3000`（残サイズから 0xD000 減算）＋ 0xA9000/0xAA000 チャンク読み（stack 0xAA484 の巨大バッファ）→ **「データ開始 = 0xD000」を前提とする同一コンテナ経路**。同関数内に ctor FUN_430970 呼び（0x45E773/0x45E7E5）。
- **[推定] 分岐の意図**: 書出側 FUN_464AB0 は無条件で offset 0 に TES4SAVEGAME を書くため、**現行 PC 版（Steam/GOG 両版）の自前セーブではこの分岐は不通過**。「CON 」は外部コンテナ形式（Xbox360/GFWL 系の署名コンテナとして知られる）のマジックであり、**非 PC/変換元セーブの受け入れ経路**と推定。GOG/Steam 判定説（v305 前提）は、両者が同一マジックを offset 0 に書く以上、**本 EXE の静的証拠では支持されない**。動的証拠なし → [推定] 明示。
- **Graphify 一致行**: docs/RE_EXE_REPORT.md 既出の TES4SAVEGAME 節と整合。'CON '・0xD000 は **docs 未出（本件が初出）**。

### 残点3: COMP/DECOMP チャンクマジック組立 site — **[負]**

| 探索 | 結果 |
|---|---|
| 全ファイル raw `DECOMP`/`NOCS`/`SCON`/`ZNPS`/`CHNK`/`HUNK`/`Compress\0`/`Decompress`/`ZLIB`/`LZ4 `/`DEFL` | **0 件** [負] |
| 全ファイル raw `COMP` | 3 件すべて部分文字列偽陽性: `PROCEDURE_ACCOMPANY` @0xA74308 / `COMPLETE` @0xA74FC0 / `INCOMPLETE` @0xA74FCE [負] |
| .text u32 imm（`'COMP'`=0x504D4F43 / `'DECO'` / `'NOCS'`=0x53434F4E / `'SCON'`） | **0 件** [負] |
| .text `mov byte ptr [..], imm8`（C6 /0、magic 文字 C/O/M/P/D/E/S/N の 120 件を列挙）→ C→O→M→P 順 64 バイト内クラスタ | **0 クラスタ** [負] |
| .text `mov word ptr [..], imm16`（66 C7、0x4F43/0x504D/0x4D43 等） | **0 件** [負] |
| `.data`/`.rdata` の 4B 定数 | 上記 3 偽陽性のみ（実在 4CC 定数なし）[負] |
| FUN_46B2C0 内の比較（0x46B2C0..0x46B590） | `cmp ecx,0x14` / `test [edi+8],0x40000` / `cmp edx,[0xB05E20]` / `cmp byte [ecx+4],0x30/0x35/0x39`（ASCII '0','5','9'）— **マジック比較なし** [部分] |

- **裁定**: 契約の 3 択（バイト逐次書 / `.data` 4B 定数 / ラッパ内部比較）は**すべて否定**。このリテール EXE には COMP/DECOMP/NOCS チャンクマジックが**一切存在しない**（組立 site を含め）[負]。.ess の圧縮は zlib ストリームのみで、v305 の「ペイロード開始位置=負証拠」と完全整合。
- **Graphify 一致行**: docs 既出マジックは TES4SAVEGAME のみで COMP 系は**不一致（不在を本件で確定）**。

### 残点4: +0x94 の 16B / +0xA4 の 4B / 0x45DD83 の 4B のオフセット対応 — **[確定]**

**書出 FUN_464AB0 のオフセット列**（`[esi+0x18]>>9&1` = サイズ計算ドライランで write の代わりに `[esi+0x90]+=n`）:

| 書出 site | 内容 | file offset |
|---|---|---|
| 0x464B04 | magic `"TES4SAVEGAME"` 12B | 0..11 |
| 0x464B37 | `this+0x70` 1B（=0） | 12 |
| 0x464B6A | `this+0x71` 1B（=0x7D） | 13 |
| 0x464BB4 | `this+0x94` **16B** | **14..29** |
| 0x464BE9 | `this+0xA4` **4B** | **30..33** |
| 0x464DEC | local 総サイズ 4B | **34..37** |

**読取 FUN_45DBC0 の対応**: read12(0..11) → read1→`+0x70`(12) → read1→`+0x71`(13)、`0x45DCC1: mov cl,[ebp] / mov [edi+0x7c],cl`（version 転送）→ file version ≥0x52 かつ arg≠0 なら read16→`+0x94`(14..29) + read4→`+0xA4`(30..33)、それ以外は `seek(0x14, origin=1)`（0x45DD21、20B スキップ）→ **`0x45DD69: read 4B` → ローカル [E+0x18]（0x45DD83 の call eax）**。file version <0x52 かつ arg≠0 なら `0x45DD48: or eax,-1` で `+0x94..+0xA4` を -1×5dword 埋め。

- **[確定] +0x94 16B = file offset 14..29 = SYSTEMTIME**: 書出時 `0x464B7F: cmp dword [esi+0xA4],0 / jne 0x464ba1 / lea ecx,[esi+0x94] / push ecx / call [0xA28108]`。**IAT 0xA28108 = KERNEL32!GetSystemTime**（PE インポート目録で同定）→ **GetSystemTime(&this+0x94)** = 16B SYSTEMTIME = **セーブ作成日時**。契約ヒントの「SYSTEMTIME相当8B」は実体 **16B（SYSTEMTIME 構造体そのまま）**。
- **[確定] +0xA4 4B = file offset 30..33 = u32 version(125)**: 同ブロック `0x464B9B: movzx edx,byte [ebp]`（ebp = `lea ebp,[esi+0x71]` → 0x7D）→ `0x464B9F: mov [ebx],edx`（ebx = `lea ebx,[esi+0xA4]`）→ **[esi+0xA4] ← 125**。読取側は素の 4B コピー。
- **[推定] +0xA4 は初期化フラグ兼用**: `[esi+0xA4]==0` のときのみ GetSystemTime と version 設定の両方を実行（≠0 なら両方スキップ）。
- **[確定] 0x45DD83 の 4B = file offset 34..37**（file version ≥0x52 時。<0x52 時は offset 14）: `push 1 / lea ecx,[esp+0x14](out) / push ecx / push 4 / lea edx,[esp+0x20](dst) / push edx / push esi / call [esi+4]` → **ローカルへ読み、FUN_45DBC0 内で値は戻されない**（副作用はストリーム前進のみ）→ **[部分] 位置確定・内容未使用**。
- **[推定] 書出側の対応 4B（0x464DEC）= 総サイズフィールド**: dst = `lea ecx,[esp+0x3c]`（esp 計算で E+0x30 = `0x464DD8: mov [esp+0x30],ecx` のスロット）= **`0x24 + len(name1) + len(name2) + thumbSize`**（`0x464DB9: movzx edx,[esp+0x74] / add edx,[esp+0x18] / movzx eax,[esp+0x17] / lea ecx,[eax+edx+0x24]`）。ヘッダ through size フィールドは 38B (=0x26) で式の 0x24 と 2B 差 → 解釈は残点化。
- **[確定] ゲート意味の訂正**: `cmp [0xB33B00]+0x7c, 0x52`（0x45DCD0/0x45DD38）の **this == [0xB33B00]（SaveGame シングルトン）を呼び手で直接確認** — FUN_4610F0←`0x5AE346: mov ecx,[0xB33B00]`、FUN_463EC0←`0x60D542`、FUN_465860←`0x5025EA`、FUN_465130←`0x5024B1/0x5D3237/0x5D342D`。よって `[global+0x7c]` は **0x45DCC1 でファイルから格納済みの version バイト** → **ゲート = ファイル version ≥0x52**（<0x52 で拡張ヘッダ無し→-1 埋め、の意味論と整合。v305 の「現ゲームバージョン」表記は**訂正**）。
- **[確定] [0xB33C14]/[0xB33C18] は無関係（否定）**: それらは **FUN_0045DE30** の 0x45DFE9/0x45DFEF で `call 0x45BAB0` に push される別経路。0x45DD83 の 4B から [0xB33C14/18] への転送は FUN_45DBC0 内に存在しない。
- **Graphify 一致行**: 「セーブスロットオブジェクト (0x160バイト; name[260]@+0x3c; TESFile派生+0x20000バッファ)」と**一致**（`new(0x160)` @0x45D543、ctor の `[esi+0x3C]=path`（0x104=260B）、`[esi+0xC]=0x20000` を実測）。+0x94/+0xA4 の日時・version 対応は **docs 未出（本件が初出）**。

### 残点5: 全 .text の `cmp byte ptr [reg+0x7c], imm8` 整列検証 — **[確定]**

- **[確定] 厳密 raw パターン（opcode 0x80 /7、mod=01、disp8=0x7C、SIB 形含む）= 299 件 → 命令境界検証（各候補の最近先頭=minrun3 からの線形 disasm）で 299/299 一致、偽陽性 0**。
- **[確定] 既知帯内 8 サイトと完全整合**: 0x45D699/0x45D6DF/0x45D791/0x45D7B6/0x45D7DB/0x45D84A（FUN_45D5F0）＋ 0x45DCD0/0x45DD38（FUN_45DBC0）すべて検出。（`cmp al,0x38` @0x45D67E は本パターン非該当で v305 の 9 サイト目と一致。）
- **[確定] 帯内（0x45D000..0x466000）17 サイト表**:

| 包含関数 | site VA（imm8） |
|---|---|
| FUN_45D5F0 | 0x45D699(0x38) / 0x45D6DF(0x34) / 0x45D791(0x1D) / 0x45D7B6(0x38) / 0x45D7DB(0x38) / 0x45D84A(0x2E) |
| FUN_45DBC0 | 0x45DCD0(0x52) / 0x45DD38(0x52) |
| FUN_45FC60 | 0x45FCA9(0x27) / 0x45FD1E(0x5E) / 0x45FD56(0x5E) |
| FUN_462B20 | 0x4634EB(0x21) / 0x46358F(0x21) / 0x4635A0(0x53) / 0x463644(0x79) |
| FUN_465860 | 0x465D7A(0x5B) / 0x465DEE(0x5B) |

- **[確定] imm ヒストグラム**: 0x00..0x7E に分散、最多は 0x71(24)/0x2A(12)/0x40(12)/0x20(10)。imm の大半は ASCII 文字比較（'a','*','@',' ' 等）→ **+0x7c は多数クラスの汎用構造体オフセット**であり、version 型ゲートは FUN_45D5F0/45DBC0 の 8 サイトに限られる。`[esp+0x7c],0x00` 形 7 件（0x490FA7 等）はスタックローカルで対象外。
- **[部分] 帯外の version 型候補**（0x45FC60 の 0x5E×2、FUN_462B20 の 0x79、FUN_465860 の 0x5B×2 等）は比較対象オブジェクトが未同定（残点化）。
- **[負] v305 の raw 579 件は再現不能**: 本条件の変異パターンは `80 mod01 /7 disp7C`=299（+SIB6）/ `80 ?? 7C` リラックス=327 / opcode 非依存 modrm(reg=7,mod=01)+7C=451 / `38 mod01 disp7C`(cmp r/m8,r8)=23 / disp32 形=0。いずれも 579 に一致せず、v305 の生パターン仕様が未記載のため突合不能。
- **Graphify 一致行**: docs 既出ゲート（0x1C/0x2E/0x34/0x37/0x38/0x52）と整合（0x1C/0x37 は帯内ゼロで v305 どおり）。

### 残点6: ストリーム vtable +4/+8/+0xC の read/write/seek 割当 — **[確定]**

**オブジェクト生成（ctor FUN_00430970、16 アライン+CC4 検証、`ret 0x10`、4 引数 (path, flag, bufsize, 0)）**:
```
0x4309B7: mov dword [esi],      0xA3636C   ; クラス構造体（vtable）ptr
0x4309BD: mov dword [esi+4],    0x430050   ; read スロット（thunk）
0x4309C4: mov dword [esi+8],    0x430070   ; write スロット（thunk）
0x4309CB: mov dword [esi+0xC],  edx        ; = ctor 引数 0x20000（128KB バッファ）
0x4309AE: mov dword [esi+0x20], ecx        ; = ctor 引数 flag（読/書モード）
0x430A21: lea ebx,[esi+0x3C]                ; path コピー（≤0x104=260B）
0x430A2E: push 0 / push ebx / call 0x982D71 / cmp eax,-1 / setne dl / mov [esi+0x24],dl
                                             ; +0x24 = ファイル存在フラグ（GetFileAttributes 相当 [推定]）
```

| スロット | 呼び出し命令（呼び手） | 実体 | 判定 |
|---|---|---|---|
| **+4 = read** | `mov edx,[esi+4]; ... call edx`＋`add esp,0x14`（FUN_45DBC0 0x45DBFC/0x45DC04、5 引数 cdecl 形 (stream,dst,n,out,flag)） | thunk **FUN_00430050**（cdecl `ret`、`mov ecx,[esp+4](this); mov eax,[ecx]; mov eax,[eax+0x38]; push a3; push a2; call eax`）→ vtable+0x38 = **[0xA363A4] = FUN_004302B0**（SEH prologue、once-init `[0xB33A00]`） | [確定]（thunk/転送先）／[部分]（FUN_4302B0 内部経路は未解析） |
| **+8 = write** | `mov edx,[edi+8]; ... call edx`＋`add esp,0x14`（FUN_464AB0 0x464B01/0x464B1B 等） | thunk **FUN_00430070**（同型、vtable+0x3C）→ **[0xA363A8] = FUN_00430030**（`call 0x747F40 / add [esi+0x148],eax / ret 8` = 下位 write + 現在位置加算） | [確定] |
| **+0xC = seek** | `mov edx,[esi]; mov edx,[edx+0xC]; call edx`（FUN_45DBC0 0x45DC31-0x45DC3E、`push origin; push offset`、`ret 8`） | **[0xA3636C+0xC] = [0xA36378] = FUN_0042FF80** — origin を [0xA853D0]/[0xA853D4]/[0xA853D8] で 0/1/2 3岐、絶対↔相対変換、`call 0x747E20`（下位 seek）、`mov [esi+0x148],edi`（現在位置更新） | [確定] |

- 補足: オブジェクトの +4/+8 は ctor が固定値を直接セットする**インスタンススロット**で、+0 の vtable（0xA3636C）とは別系統（vtable+4/+8 = 0x7481C0/0x4307F0 と不一致）。下位層 **FUN_00747E20(seek)/FUN_00747F40(write)** は 0x747Exx 帯（zlib/CRT 隣接）で未同定 [部分]。
- thunk の転送は (dst, n) の 2 引数のみ（out/flag は thunk 経由では転送されず）[確定:命令] — read 実装での out 扱いは FUN_4302B0 未解析のため [部分]。
- **Graphify 一致行**: 「セーブスロットオブジェクト」節と一致（+0x3C name / +0x20000）。+4/+8/+0xC の read/write/seek 割当は **docs 未出（本件が初出）**。

### 残点7: FUN_004501F0 の圧縮チャンクループ境界 — **[部分]**

- **[確定] 契約前提の否定**: **FUN_004501F0 は FUN_450550 の呼び手ではない**。FUN_450550 の 3 呼び手 site の包含関数は **FUN_00450C20**（0x450D61/0x450EC3、エラー文字列 `"First/Second ReadFile() in GetChunkData failed with error:\n%s"` @0xA38E78/0xA38DE0）と **FUN_00450F10**（0x450F88）（いずれも先頭検証済）。FUN_4501F0 本体に call 0x450550 は存在せず、呼び手は 0x4D353E/0x4D35B3 の 2 サイトのみ。
- **[確定] FUN_004501F0 の実体 = seek(END) によるサイズ取得ヘルパ**: `mov edx,[0xA853D8](=2=SEEK_END) / mov eax,[ecx] / mov eax,[eax+0xC] / push edx / push 0 / call eax` = `seek(0, SEEK_END)` ＋ `[esi+0x10]` オブジェクトの +0x30/+0x148 参照。**ループなし**。
- **[確定] 圧縮ゲート `[esi+0x244] & 0x40000` の実在 site**（帯 0x450000..0x452000）: 0x450577（FUN_450550 本体、head 直後 `test dword [esi+0x244],0x40000 / je 0x450709`）、0x450F35（FUN_450F10、`jne 0x450f7a` で圧縮経路へ）、0x45118B、レジスタ形 `test [esi+0x244],ebx` = 0x450C74/0x450CD2/0x450E1A（GetChunkData）、0x451906 は imm=0x1000。
- **[負] 0x10000（64KB）ループ境界は帯内に不在**: raw imm32 0x10000 は 0x45161E の 1 箇所のみで本ループ非関連。
- **[部分] チャンク反復の本体は呼び手側**: FUN_00450F10 は単発処理（`push 6` で vtable+4 read 6B → type=`[esi+0x250]`、size word=`[esi+0x254]` → `0x450FB7: cmp ecx,0x58585858`（'XXXX'）一致時 `call 0x450C20`(GetChunkData) + `call 0x44FEA0` → `mov al,1; ret`、後退ジャンプのループ本体なし）。チャンク反復は NextForm 帯（0x4513xx、`SetFilePointer() in NextForm failed` xref 0x4513D1 = FUN_451310 領域）側と推定。
- **Graphify 一致行**: 「inflate ラッパ FUN_450550」既存ノード（v305 由来）と整合。FUN_4501F0 の役割は **docs 未出（本件が初出）**。

### 残点8: FUN_00984166 引数順 — **[確定]**

- **[確定] 引数順 = (s1, s2, n)、cdecl（caller が `add esp,0xC`）**: reader の直前 push 群 = `push 4 / push 0xA3A954 / push eax(buf)` → **f(buf, "CON ", 4)**。第 3 サイト例（0x42DE0C）も `push 5 / push 0xA2FFB8 / push ebp` → f(ebp, str, 5)。** memcmp(dst, src, n) 形式と一致**。
- **[確定] 本体は memcmp ではなく strncmp 相当**: 本体 disasm（0x984166..0x984222）:
  ```
  0x98416F: mov ebx,[ebp+0x10]   ; n
  0x98418A: mov ecx,[ebp+0xC]    ; s2
  0x98418D: mov eax,[ebp+8]      ; s1
  0x984190..0x9841CC: 4バイト展開ループ — s1 バイトの `test dl,dl; je` で **NUL 早期終了**
  0x984200..0x984212: 逐バイトループ（同上、counter<n）
  一致: 0x984214 xor eax,eax → ret 0
  不一致: 0x98421A movzx eax,[eax] / movzx ecx,[ecx] / 0x984220 sub eax,ecx → s1-s2 差分
  ```
- **[負] CRT 名**: `.rdata`/`.data` に `memcmp`/`_memcmp`/`strcmp`/`strncmp` の NUL 終端文字列は**一切なし**（静的 CRT もシンボル名を保持しない）→ ライブラリ名は未確認のまま。
- **Graphify 一致行**: docs 未出（本件が初出）。

### 残点9: セーブファイル名生成（FUN_464910 / FUN_465130 / [0xA2F920]） — **[確定]**

- **[確定] FUN_00464910 はファイル名生成ではない（契約前提を否定）**: 0x464910（16 アライン+CC ラン10 検証、`ret 4`）は `mov eax,[0xB33398] / mov esi,[eax+0x10] / call [0xA2808C]`（**GetCurrentThreadId**、IAT 同定）によるスレッド照合 → `[ebx+0x18]` フラグ → `call 0x4637C0 / 0x4C97F0 / 0x55E000`（リンク走査）/ `0x463D70` → **プラグイン/変更ファイルリスト検査系**。呼び手は **0x4CDA22 の 1 サイトのみ**で SaveGame chain 外。「マジック書き直前・同一呼び手 chain」の前提も不成立（FUN_464AB0 の呼び手は FUN_465130 の 0x465228 のみ）。
- **[確定] ファイル名/パス生成の実体は FUN_0045D920 と FUN_0045F2E0**:
  - **FUN_45D920**: `"%s%s%s.ess"`（[0xB05564]=ディレクトリ + [0xB3F280]=ベース名 + ローカルの " #%d" 連番）→ `[0xB33A04]` 仮想呼び出し (path,0,0,-1) で重複回避 → 不正文字サニタイズ（残点1）。
  - **FUN_0045F2E0**（0x45F2E0、SEH プロローグ直接確認・CC ラン2で規則外 → 直接 disasm で先頭裁定、呼び手 8）: mode スイッチ（`cmp ebp,3 / ja / jmp [ebp*4+0x45F790]`、mode 0..3 = 0x45F5EF/0x45F6FC/0x45F756/0x45F763）、`"%s%s%s.ess"` sprintf @0x45F3A7、**.bak ローテーション**（`mov ebp,[0xA3AAE0]`=".bak"、`mov bl,[0xA3AAE4]`、回数 [0xB05BC4]≤10、`call [0xA280AC]`（DeleteFile 系 [推定]）、`call 0x98544F`（rename 系 [推定]））、mode0/1 = `new(0x154)` + ctor 0x430970、mode2/3 = 既存オブジェクト再利用（`call [vtable+0x18](obj,0,0)`）。
- **[確定] FUN_00465130 = SaveGame 本体**: 16 アライン+CC ラン7 検証、呼び手 6（0x466AF6/0x466BD0/0x466D21/0x5024BA/0x5D3246/0x5D3438、うち 3 つが `mov ecx,[0xB33B00]`）。`repe cmpsb` で `"autosave"` @0xA3AAD4 比較（0x465182）→ `call 0x45F2E0`（stream open、0x4651DE）→ `call 0x464AB0`（header write、0x465228、直前に `mov [ebp+0x70],0 / mov [ebp+0x71],0x7D / mov [ebp+0x7c],0x7D`）。
- **[確定] [0xA2F920] = double 24.0**（36 箇所 xref、うち 0x464CAD が writer 内 `fdiv qword [0xA2F920]`）。文脈: `mov ecx,0xB332E0 / call 0x402BD0`（float getter、`[this+0xC]+0x24` or 既定 [0xA2F918]）→ **÷24** → `+ 0x402C00`（int 化 getter、負値は [0xA2FC78]=4294967296.0=2^32 で符号補正）→ 日数浮動小数 → 直後に `call [0xA28174]`（**GetLocalTime**）。**秒→日時換算ではなく 24（時間/日）除算**。
- **[推定] 24.0 の用途 = 時→日換算**（オブジェクト 0xB332E0 の 2 つの float getter を「日 + 時/24」で合成するカレンダー系計算）。xref 36 箇所は 0x402A59/0x45A473/0x464CAD/0x4F79FB… と広範で、汎用定数。
- **Graphify 一致行**: 「FUN_00465130 (SaveGame本体)」「27.3 セーブスロットの表示/保存まわりの文字列定数」と**一致**。FUN_464910 の用途は docs 記載と**不一致（本件で訂正: プラグイン/スレッド検査系）**。

### 残点10: その他の残余 — **[確定]**

残点1〜9 の調査过程中で判明した未着手点を §3（新規残点）に一覧化した（件数 11）。本項目は「残余の棚卸し」自体が完了 [確定]。

---

## 2. 判定サマリ

**主判定（残点 10 件ベース）**

| 判定 | 件数 | 該当残点 |
|---|---|---|
| [確定] | **8** | 1, 2（機構。用途は [推定] 付記）, 4, 5, 6, 8, 9, 10 |
| [部分] | **1** | 7（ループ本体は呼び手側のため未特定） |
| [推定] | **0** | —（推定は各項目内のサブ判定に付記） |
| [負] | **1** | 3（マジック不在） |

**サブ判定（根拠単位・全 52 件）**

| 判定 | 件数 | 主なもの |
|---|---|---|
| [確定] | **34** | 0x45DA30=非関数/FUN_45D920 同定/'CON ' 1 サイト/origin=0,1,2/+0x94=GetSystemTime 16B/+0xA4=125/0x45DD83=offset34/ゲート=ファイル version/299 件全境界検証/+4=read +8=write +0xC=seek/strncmp 引数順/FUN_464910 否定/[0xA2F920]=24.0 ほか |
| [部分] | **6** | FUN_46B2C0 マジックなし/0x45DD83 値の用途/帯外ゲート候補/FUN_4302B0・下位 747Exx 未解析/チャンク反復は呼び手 |
| [推定] | **4** | 'CON ' 用途（非 PC コンテナ）/+0xA4 初期化フラグ兼用/size フィールド/24.0=時→日 |
| [負] | **8** | COMP 系 raw・u32・mov byte・mov word・data 定数（5）/0x10000 ループ不在/CRT 名/v305 raw579 再現不能 |

---

## 3. 新規残点（次束用・低優先）

1. **FUN_004302B0（read 実装）の内部経路** — `[this+0x148]` 更新、バッファ/ReadFile の分岐、out/flag 引数の扱い（thunk は (dst,n) のみ転送）。
2. **下位層 FUN_00747E20 (seek) / FUN_00747F40 (write) / read 相当の同定** — 0x747Exx 帯（zlib/CRT 隣接）の所属（静的 CRT ? zlib gz* ?）。
3. **FUN_0045DE30 の完全解析** — 0xB02F9C RTTI + 0x9832E6 バンド（契約の「FUN_45DA30」説明の実体）。呼び手 0x4627E1、`call 0x46B250/0x446B80`、"Enchantable item %08X…" 経路。
4. **FUN_0045E6A0（0xD000 第 2 経路）の用途特定** — 呼び手 0x5AEB8B/0x5D3AAA、`add ebp,-0xD000` + 0xA9000/0xAA000 チャンク読み。'CON ' コンテナ経路との関係。
5. **帯外 version 型候補ゲートのオブジェクト同定** — FUN_45FC60(0x27/0x5E×2)、FUN_462B20(0x21/0x53/0x79)、FUN_465860(0x5B×2) ほか。
6. **書出 size 式 `0x24+len+len+thumb` とヘッダ 38B の 2 バイト差**の解釈（size フィールドの定義）。
7. **v305 の raw 579 パターン仕様の復元**（本検証は 299/327/451/23 で 579 に非一致）。
8. **FUN_464AB0 内 GetLocalTime 直前の日数浮動小数（[esp+0x48]）の使途**（0x464CDA〜0x464DC7 のスロット再利用を精査）。
9. **`new(0x154)`（FUN_45F2E0 mode0/1）と `new(0x160)`（FUN_45D390 スロット生成）のクラス差 0xC の意味**。
10. **FUN_45F2E0 の mode 0..3 全経路**（ジャンプテーブル 0x45F790）の各戻り値・ストリーム種別分類。
11. **FUN_45D390（スロット生成、呼び手 0x4E11A2 のみ）の呼び出し文脈**（セーブ一覧生成か新規作成か）。

---

## 4. selftest 再現メモ

実行環境: `chcp 65001` → `python $env:TEMP\opencode\v307_<n>.py *> $env:TEMP\opencode\v307_<n>.out`（%TEMP%=C:\Users\hiroki.kogarumai\AppData\Local\Temp）。共通ライブラリ `v307_common.py`（F(va) 変換・関数先頭規則・capstone・callers_of・imm32 xref）に全スクリプトが依存。

| スクリプト | 内容 | 再現結果（今回） |
|---|---|---|
| v307_01 | 0x45DA30 頭部規則検証・nearest head・呼び手/文字列 | 非関数（CC0・byte 00・mid-instr）、head=0x45D920、E8/imm refs=0 |
| v307_02 | FUN_45D5F0/45D920/45DBC0 head・FUN_45D920 全体 dump・0x9832E6/0xB02F9C 帯 | CC4=0x45D91C..1F、"Save %i…" 等 4 文字列、呼び手 0x45F370、0x9832E6 帯=FUN_45DE30 |
| v307_03 | FUN_45D5F0 末尾・FUN_45DE30 dump・'CON ' xref・0xA853D0/4 値・FUN_45DBC0 詳細・push 0xD000 | ret 0x28@0x45D919、refs('CON ')=1(0x45DC1B)、[A853D0]=0/[A853D4]=1、D000 sites=0x45DC37/0x45E812 |
| v307_04 | reader 尾部・writer part1/2・第2 D000 site・I/O エラー文字列 | write 列 12/1/1/16/4/4B 確認、GetSystemTime ブロック確認、"SetFilePointer…" xref=0x4513D1/0x451519 |
| v307_05 | PE インポート目録パース・ReadFile/WriteFile/SetFilePointer ヘッド候補 | IAT 230、0xA28108=GetSystemTime/0xA28174=GetLocalTime/0xA280D0=GetTickCount |
| v307_06 | SetFilePointer 文言 xref・呼び手 ecx・impl の data 出現 | 0x4513D1/0x451519、`mov ecx,[0xB33B00]` 呼び手複数、0x494730→rdata 0xA3DA30 |
| v307_07 | vtable 3 箇所展開・FUN_45F2E0 近傍・呼び手チェーン | 0xA3636C 系の同定に到達（→v307_09/11 で確定） |
| v307_08 | CC ラン再検証（minrun 問題）・FUN_45F2E0 直接 dump・FUN_464910/465130・定数 | **FUN_464AB0 CC=3 判明**（規則調整の根拠）、[A2F920]=24.0、[A2FC78]=2^32 |
| v307_09 | IAT 0xA2808C・C7 スキャン・FUN_45F2E0 part2 | GetCurrentThreadId、.bak ローテーション確認 |
| v307_10 | reader 先頭 read 完全列・FUN_45F2E0 tail・ジャンプテーブル・SetOffset 帯 | 5 引数 read 形確認、`new(0x154)`+ctor 0x430970、mode 0..3 表 |
| v307_11 | FUN_430970 ctor dump・C7 スキャン | `[esi]=0xA3636C / +4=0x430050 / +8=0x430070 / +0xC=0x20000 / +0x3C=path / +0x24=exists` |
| v307_12 | FUN_450550 呼び手 head・FUN_984166 本体・push 群 | head=0x450C20/0x450F10、strncmp 相当本体、34 callers |
| v307_13 | item3 raw/u32/mov byte/mov word/data 定数 | 全 [負]（COMP=3 偽陽性のみ） |
| v307_14 | item5 全 .text 厳密 pattern → 境界検証 | raw 299 → **verified 299 / rejected 0**、既知 8 サイト全一致、histogram 出力 |
| v307_15 | vtable 0xA3636C dump・thunk/seek/4501F0/0x244 sites/984166 尾/402BD0・327・FUN_46B2C0 cmp | seek=FUN_42FF80（origin 3岐）、4501F0=seek END helper、0x40000 ゲート site 列挙 |
| v307_16 | [0xA363A4/0xA363A8] 実メソッド・seek tail・0x40000 imm・FUN_465130 呼び手・pattern 変異・slot 生成 | read=FUN_4302B0、write=FUN_430030→0x747F40、seek 下位=0x747E20、variants=293/296/6/23/327/0 |
| v307_17 | 0xD000 第2 site head・[0xA2F920] xref・v7 パターン・残 head 検証 | head=0x45E6A0（呼び手 0x5AEB8B/0x5D3AAA）、xref 36、v7=451、0x45DE30/0x45D390 head 確認 |

- 再実行は上記コマンド列で同一出力（決定的・非決定要素なし）。中間物: `%TEMP%\opencode\v307_common.py` および `v307_01..17.py` / `*.out`。
- v305 由来の既知事項のうち本件で**訂正したもの**: ①「[0xA853D0]=0x78?」→ 実値 0（origin 定数）②「FUN_45D5F0=0x45D5F0..0x45DBB6」→ 0x45D919 ③「ゲート=現ゲームバージョン」→ ファイル version ④「FUN_4501F0=FUN_450550 呼び手」→ 該当なし ⑤「FUN_45DA30 に 0x9832E6/0xB02F9C」→ FUN_45DE30 ⑥「FUN_464910=ファイル名生成」→ プラグイン/スレッド検査系 ⑦「raw 579」→ 再現不能（299 で全検証）。
