# v309 — .ess ヘッダ I/O 続編（v307 残点11件の静的解析）/ ai-todo #70 次束B

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000、7,898,624 B）
- 日付: 2026-11-07 / 担当: 静的解析サブエージェント（v309 のみ）
- 帯: **0x45D000..0x466000**（ヘッダ読/書・SaveGame）＋ **0x747000..0x748000**（下位ストリーム層）＋ zlib ラッパ帯（FUN_450550/42C6F0/4501F0 周辺）。v308 帯（0x45A000-0x45CFFF+0x494000-0x496000）には非侵入。
- 判定タグ: **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**（5段階）
- セクション: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` VA 0xA28000→file 0x627200 (0xD9A00) / `.data` VA 0xB02000→file 0x700C00 (0x30C00)。変換は section 基準 `F(va)=foff+(va-sva)`（v307_common.py、`pe.get_offset_from_rva(VA-0x400000)` と結果一致 = v305/v307 で +0x400 ずれ統一済み [確定]）。
- ツール: python 3.11 + capstone 5.0.7（`md.detail=True`）、PE インポート目録自前パース（IAT 230 エントリ・DLL 14 種）。**REA バイナリプロバイダは不使用**（os error 3 フォールバック契約に従い、既存中間物 `v307_common.py` + `v307_*.out` + 本会話の probe 13 本のみで検証）。
- クリーンルーム: OpenTES4 / xOBSE / COEF 等の逆コンパイル由来コードは参照・流用なし。実 .ess データ・B13 実機ダンプなし → 証拠不足箇所は [未確定] で明示。

## 0. 調査対象と目的・使用資料

**目的**: v307（`tmp/re/v307_saveheader_residuals.md`）§3 の新規残点11件を、命令証拠ベースで裁定し、`docs/RE_EXE_REPORT.md` 正典への反映候補を人間確認用に整理する。推測実装の材料を作らないこと（#70 完了条件④）。

**先に読んだ資料（既知結論の再調査なし）**:
- `tmp/re/v307_saveheader_residuals.md`（全文・§1 [部分]/[未確定] 段 + §3 11件が調査対象）
- `tmp/re/OPENCODE_REVERSE_ENGINEERING_INSTRUCTION.md`（追加契約: 5段階タグ・re_util.py・正典優先 §55補遺→§50・REA フォールバック・完了条件5項目）
- `tmp/re/v301_micro_residuals_bundle.md`（既知: seek=FUN_0042FF80→FUN_00747E20 が [this+0x148] 更新、write=FUN_00430070→FUN_00430030→FUN_00747F40、FUN_45F2E0 mode0..3+.bak ローテーション等 = **検証対象であって再導出対象でない**）
- `tmp/re/re_util.py`（変更禁止・本件では v307_common.py の既存関数で同一結果を確認済み）

**中間物（本件で新規作成・%TEMP%\opencode\）**: `v309_common.py` は使わず `v307_common.py` を再利用。`v309_01..13.py` / `v309_01..13.out`（probe 13 本、全 EXIT=0。v309_11 のみ esp 追跡部で TypeError → v309_12 で再実装）。

---

## 1. 項目ごとの結果

### 項目1: FUN_004302B0（read 実装）内部経路 — **[確定]**

**範囲**: 0x4302B0..0x430347（16 アライン + 直前 CC ラン≥4 検証、SEH prologue `push -1 / push 0x9ABDEE / fs:[0]`、`ret 8` @0x430347 → CC ラン6 → 次アライン 0x430350）。

**完全経路（命令証拠）**:
```
0x4302B0: SEH prologue + GS cookie（[0xB30AAC]）
0x4302D8: test byte [0xB33A00],1 / jne 0x43030D      ; once-init ガード
0x4302E0: or [0xB33A00],1 / mov ecx,0xB33980 / call 0x401000
0x4302F8: push 0xA17B60 / call 0x981FB4 / add esp,4  ; グローバル初期化＋登録（atexit 系 [推定]）
0x43030D: cmp dword [esi+0x1C],0 / jne 0x430320      ; FILE* 未開なら
0x430313: mov eax,[esi] / mov edx,[eax+0x18] / push 0 / push 0 / mov ecx,esi / call edx
                                                     ; → vtable+0x18 = FUN_0042FE80 open(this,0,0)
0x430320: mov eax,[esp+0x1C](arg2=n) / mov ecx,[esp+0x18](arg1=dst)
          push eax / push ecx / mov ecx,esi / call 0x747E80   ; lower read(this,dst,n)
0x430331: add dword [esi+0x148], eax                ; **現在位置 += 読取バイト数**
0x430347: ret 8
```

- **[確定] ReadFile 経路は不在（否定）**: FUN_4302B0 内の E8 は 0x401000 / 0x981FB4 / 0x747E80 の3件のみ、間接 IAT call 0 件。IAT `ReadFile`(0xA2809C) の全5サイトは 0x448E20/0x7823C0 帯（v307_05 済）で本経路に不在。
- **[確定] バッファ分岐は下位 FUN_00747E80 側**（項目2）。
- **[確定] out/flag 引数は実装に到達しない**: FUN_4302B0 は `ret 8` = 2引数（dst, n）。thunk FUN_00430050 は (dst,n) のみ転送（v307 確定）→ 呼び手の 5引数形 (stream,dst,n,out,flag) のうち **out/flag は thunk で捨てられ、FUN_4302B0 の栈上に現れない**。戻り値 eax = 読取バイト数。
- **[確定] [this+0x148] 更新 = 加算式**: `add [esi+0x148],eax`（seek 実装 FUN_0042FF80 の `mov [esi+0x148],edi`（絶対設定）と対で、+0x148 = 現在位置バイト）。
- **[部分確定] once-init の中身**: `call 0x401000`（ecx=0xB33980）+ `push 0xA17B60; call 0x981FB4` = グローバル初期化＋ハンドラ登録とみられるが、両関数の正体は未特定。

### 項目2: 下位層 0x747Exx の所属 — **[確定]（静的 CRT stdio + 自前バッファ層）**

**4 関数の同定（v307 未同定の read 相当を含む）**:

| VA | 役割 | 判定根拠（命令証拠） |
|---|---|---|
| **FUN_00747E80** | **read 相当**（v307 残点2の「read 相当」を解消） | `ret 8`。`edi=[esi+0x10]-[esi+0x14]`（バッファ残量）→ 残量分 `call 0x9812C0`(memcpy, dst←[esi+0x18]+[esi+0x14])、超過分は `call 0x988245` = **fread(dst,1,n,[esi+0x1C])**（大は直接 dst、小は `mov [esi+0x10],eax` でバッファ充填後 memcpy）。`[esi+0x24]==0` で `xor eax,eax; ret 8`（エラー時は 0 返し） |
| FUN_00747E20 | seek | `ret 8`。`cmp [esi+0x24],0` ガード、origin==1 なら `[esi+0x14]+offset`（相対）、絶対は `call 0x747D30`(flush) → **`push [esi+0x1C]; push origin; push offset; call 0x984801` = fseek(fp,offset,origin)**、`sete cl` で結果を [esi+0x24] へ |
| FUN_00747F40 | write | `ret 8`。`edi=[esi+0x0C]-[esi+0x14]`（書込残量）→ 残量分 memcpy、超過分は **`push [esi+0x1C]; push n; push 1; push buf; call 0x987F5B` = fwrite(buf,1,n,FILE*)** |
| FUN_00747D30 | flush | `cmp [esi+0x20],0`（=ctor flag 読/書）→ 0 なら read モードで `[esi+0x10]=[esi+0x14]=0` リセット、非0 なら dirty フラッシュ |

**open（vtable+0x18 = FUN_0042FE80）**: `[esi+0x1C]!=0` で即 return 1。モード選択 = **`'rb'`@0xA36348 / `'rt'`@0xA3634C / `'r+b'`@0xA36340 / `'r+t'`@0xA36344**（ctor flag `[esi+0x20]` と引数 byte で4種）→ `call 0x982440` = **fopen(path,mode)**。失敗かつ write モードなら `'wb'`@0xA36338 / `'wt'`@0xA3633C で fopen → `call 0x982179`(fclose) → 元 mode で再 fopen。成功時 `[esi+0x1C]=FILE*`、`[esi+0x18]=new([esi+0xC])`（バッファ、0x401F00=operator new）、read モードは `call 0x747D10` で初期充填。

**所属裁定 — [確定] 静的リンク CRT stdio（fopen/fseek/fread/fwrite）＋自前バッファ層**。根拠:
1. **mode 文字列6種**が CRT 形式（rb/rt/r+b/r+t/wb/wt、0xA36338..0xA3634C に連続配置、vtable 0xA3636C の直前）。
2. **errno 設定パターン**: fseek/fwrite 内 `call 0x98540B; mov dword [eax],0x16`（0x16=22=EINVAL）→ CRT の `_set_errno` 相当。
3. **引数形**: fread(buf,1,n,FILE*) / fwrite(buf,1,n,FILE*) / fseek(fp,offset,origin)、cdecl（caller `add esp,0x10`）。
4. **CRT DLL import 不在**: IAT DLL = KERNEL32/USER32/GDI32/ADVAPI32/SHELL32/COMCTL32/ole32/VERSION/WINMM/WSOCK32/DSOUND/DINPUT8/d3dx9_27/binkw32 の14種のみ（msvcrt/ucrt/msvcr 無し）→ **静的リンク**。
5. **[確定負] zlib gz* ではない**: gzread/gzseek の引数形（gzFile は1引数）・モード文字列（"rb" は通るが gz は内部で fd 管理）と不一致。**[確定負] Win32 直叩きではない**: ReadFile/WriteFile/SetFilePointer IAT サイトが本帯に不在（v307_05 済）。

**帯 0x747000..0x748000 の構成**: 16 アライン+CC ラン≥4 の head = 20 個（0x747020/72B0/7380/7830/78C0/78E0/78F0/7930/79F0/7BE0/7C20/7C40/7C70/7D10/7D30/7D80/7DE0/7E20/7E80/7F40）。可読 strings 0 件（偽陽性のみ）→ **自前ストリーム層（バッファ管理＋CRT 呼び出し）** [確定]。補助 thunk: 0x747FF0 / 0x748010（cdecl 形 read/write ラップ）、0x748030（read + 0x748260 後処理）。

**呼び手**: 0x747E20 = 0x42C434（zlib ラッパ帯）/0x42FFF0（FUN_0042FF80 seek 実装内）/0x430410。0x747F40 = 0x43003F（FUN_00430030 write 実装内）/0x74801E/0x7480AC。

### 項目3: FUN_0045DE30 の完全解析 — **[確定]**

**範囲**: **0x45DE30..0x45E0CC**（16 アライン + 直前 CC ラン10 検証、`ret 4` @0x45E0CC → int3 @0x45E0CF → 次関数 0x45E0D0）。v309_03 の初回 dump は境界未検証で0x45F2E0 帯まで到達したが、v309_05 の ret/CC 解析で上記に確定。

**契約の「FUN_45DA30 説明の実体」= 本関数 [確定]**。構造:

```
this = ebx = ecx / arg1 = [esp+0x38] = stream
[ebx+0x28] = 連結リスト（node: [0]=key, [4]=next）
1周目（検証カウント）:
  eax = [edi] → call 0x46B250 (global [0xB0613C] map lookup via 0x55E000, 戻り=found? value:0)
  → __RTDynamicCast ×3（call 0x9832E6, 1819 callers）:
      TESForm@0xB02F9C → TESBoundObject@0xB03248 / SpellItem@0xB03800 / TESEnchantableForm@0xB03718
      （push 0 / push dstTD / push srcTD / push 0 / push obj の5引数形 = __RTDynamicCast [確定]）
  TESEnchantableForm 成功時: [eax+4] = enchantment ptr → [eax+4]+0xC = formID
      → call 0x446B80 = `cmp [esp+4],0xFF000000 / sbb eax,eax / add eax,1 / ret 4`
        = **formID ≥ 0xFF000000 なら 1**（生成/一時 formID 判定）[確定(命令)]
      → 該当で [esp+0x10]++（カウント）
  TESBoundObject / SpellItem なら [esp+0x10]++
2周目（カウント>0 のとき）:
  同様の cast → TESENchantableForm で [eax+4]==0（enchantment 無し）なら
    push formID / push 0xA3A960 / call 0x4A7A60
    = **"Enchantable item %08X with no enchantment exists in the created base objects list."**
    （0x4A7A60 = 0x4941A0 薄ラッパ → [0xB34D90] 警告ロガー → 'Warnings.txt' sink = v301 件2 で確定 [確定]）
  0x446B80 判定通過の enchantment は call 0x45BAB0(this, stream, [0xB33C14], [0xB33C18]) へ渡される
書き出し（dry-run flag [ebx+0x18]>>9&1 で分岐）:
  カウント4B（dry: [ebx+0x90]+=4 / real: write from 物理[esp+0x10]）
  [0xB33C14] から [0xB33C18] バイト（dry: +=esi / real: write）→ call 0x45BAB0 経由
ret 4（arg1 = stream を pop）
```

- **呼び手 0x4627E1**（head 0x462080）: `push edi(stream) / mov ecx,esi / call 0x45DE30`、直前に `[esi+0x18]>>9&1` dry-run 分岐付きの4B write（save 側）。
- **[確定] 0x46B250 = global map lookup**（`mov ecx,0xB0613C / call 0x55E000 / neg al / sbb eax,eax / and eax,[esp]` = found なら value、非0 なら 0）。
- **[確定] 0x446B80 = formID ≥ 0xFF000000 判定**（上記命令列）。
- **[確定] 0x9832E6 = __RTDynamicCast**（5引数形・1819 callers・TypeDescriptor 実体は NUL 前の `.?AV...` 形）。
- **v307 記載の補正**: 0xB03248/0xB03800/0xB03718 は v307 で「data」とのみ記載 → **TESBoundObject / SpellItem / TESEnchantableForm の TypeDescriptor** と同定（+8 に `.?AV...`）。
- **[確定] 契約「FUN_45DA30 の説明の実体」は本関数**（v307 残点1 で 0x45DA30=非関数・実体 FUN_45D920 と判明済。ただし **0x9832E6/0xB02F9C 帯の実体は FUN_45DE30** = v307 残点1 の訂正⑤を踏襲）。

### 項目4: FUN_0045E6A0（0xD000 第2経路）の用途 — **[確定]（Xbox 保存列挙＋0xD000 無条件）**

**範囲**: **0x45E6A0..0x45E933**（16 アライン + CC ラン11 検証、`ret` @0x45E933 → CC ラン12 → 次 head 0x45E940）。v309_04 の初回 dump は境界未検証で 0x45F08A まで到達したが、v309_05 で上記に確定。

**中身（命令証拠）**:
```
0x45E6B3: call 0x983BE0（_chkstk [推定]、大スタック）
0x45E6DF: push 0xA3A9F4 ('XBoxSaves\*.*') / call [0xA2812C] = FindFirstFileA
0x45E701: call [0xA2812C] FindFirstFileA（ループ先頭）
0x45E736: push 0xA3A9E4 ('XBoxSaves\%s') / call 0x98208B (sprintf) / call 0x401F00 (new)
0x45E773: call 0x430970（ストリーム ctor、XBoxSaves 経路）
0x45E7A8: push 0xA3A88C ('%s%s%s.ess') / call 0x98208B / call 0x401F00
0x45E7E5: call 0x430970（ストリーム ctor、通常 .ess 経路）
0x45E80C: add ebp,0xFFFF3000        ; 残サイズ -= 0xD000
0x45E812: push [0xA853D0](=0) / push 0xD000 / call vtable+0xC   ; seek(0xD000, SEEK_SET)
0x45E832: mov eax,0xAA000           ; チャンクサイズ候補A（696,320 B）
0x45E839: mov eax,0xA9000           ; チャンクサイズ候補B（692,224 B）
（巨大スタックバッファ 0xAA484 = 697,476 B を経由して上記チャンクを読み込み）
0x45E8DE: call [0xA280AC] DeleteFileA
0x45E8F5: call [0xA28124] FindNextFileA
0x45E904: call [0xA28120] FindClose
```

- **呼び手**: 0x5AEB8B（head 0x5AEA60）、0x5D3AAA（head 0x5D39A0）。**両者とも直前に `mov ecx,[0xB33B00]`（SaveGame シングルトン）** [確定]。呼び手の直後は `mov byte [esp+0x17],al`（戻り bool 退避）→ `call 0x45D450`（セーブ一覧再構築、項目11）→ `[0xB33B00]+0x6C` リスト反復。0xFA5/0xFAB/0xFA1 の push + call 0x588BD0/0x58CEB0 は設定/フォーム参照系（本件の主目的外）。
- **[確定] 'CON ' コンテナ経路との関係 = 独立した第2経路**: FUN_45E6A0 は **'CON ' 検査を一切しない**（0xA3A954 xref は 0x45DC1B の1箇所のみ = v307 確定）。`XBoxSaves\*.*` 列挙されたファイルに対して **無条件に seek(0xD000)** → Xbox 360 CON コンテナのデータ開始オフセット前提 [確定]。対して FUN_45DBC0 はファイル内容の 'CON ' 4B一致で seek(0xD000)（内容検出型）。**2 経路は独立**（A: 内容 'CON ' 検出 → 0xD000 = ヘッダ読取、B: XBoxSaves 列挙 → 0xD000 無条件 = コンテナ前提）。
- **[部分確定] 2つの ctor のどちらに seek(0xD000) が紐づくか**: 0x45E77A jmp / 0x45E7F0 jmp の分岐構造が細部未追跡（新規残点7）。
- **[確定] 0xAA000/0xA9000 = チャンク読みサイズ定数**（`mov eax,imm` のみで、以降の使用箇所は jmp先）。0xD000 以降の残サイズから 0xD000 を減算してチャンク分割 [確定]。

### 項目5: 帯外 version 型候補ゲートのオブジェクト同定 — **[確定]（全サイト [0xB33B00]+0x7c）**

v307 残点5 の [部分]「比較対象オブジェクトが未同定」を全解除:

| 関数 | 範囲 | gate サイト（命令証拠） | 判定 |
|---|---|---|---|
| FUN_45FC60 | 0x45FC60..0x45FD93（ret 4 @0x45FD93、CC10 → 0x45FDA0） | `0x45FC61: mov eax,[0xB33B00] / mov al,[eax+0x7c] / cmp al,0x25`、`0x45FCA3: mov ecx,[0xB33B00] / 0x45FCA9: cmp [ecx+0x7c],0x27`、`0x45FD19/0x45FD50: mov eax/ecx,[0xB33B00] / cmp ...,0x5E` | **[確定]** |
| FUN_462B20 | 0x462B20..0x4636F1（ret 4 @0x4636F1、CC12 → 0x463700） | `0x4634E5: mov ecx,[0xB33B00] / 0x4634EB: cmp [ecx+0x7c],0x21`、`0x46358A/0x46359A: 同 0x21`、`0x4635A0: 0x53`、`0x46363F/0x463644: 0x79` | **[確定]** |
| FUN_465860 | 0x465860..（SEH prologue、sub esp,0x310） | `0x465DE4: mov ecx,[0xB33B00] / 0x465DEE: cmp [ecx+0x7c],0x5B` | **[確定]** |
| FUN_45D5F0（帯内・v307 既出の再確認） | 0x45D5F0..0x45D919 | `0x45D693: mov edx,[0xB33B00] / 0x45D699: cmp [edx+0x7c],0x38`、`0x45D6DA: mov eax,[0xB33B00] / 0x45D6DF: cmp ...,0x34` | **[確定]** |

- **[確定] オブジェクト = [0xB33B00] = SaveGame シングルトン、+0x7c = ファイル version バイト**（0x45DCC1 でファイルから格納、writer FUN_464AB0 @0x464AF2 で `mov byte [esi+0x7c],0x7D`(125) に設定）。imm8 = **version 閾値**（0x21=33 / 0x25=37 / 0x27=39 / 0x34=52 / 0x38=56 / 0x53=83 / 0x5B=91 / 0x5E=94 / 0x79=121）。
- **[確定] 各ゲートの機能**: FUN_45FC60 = 'Temp Effects List'@0xA3AB5C の保存/読取（LoadTempEffectsList バッファ整合チェック @0x45FD76）；FUN_462B20 = 'Quick Keys'@0xA3AF1C / 'HUD Reticle'@0xA3AF10 / 'Interface'@0xA3AF04 の保存/読取；FUN_465860 = ヘッダ読取経路（FUN_45DBC0 呼び手 0x465999）。
- **[確定] imm ヒストグラムの解釈補正（v307 残点5）**: 帯内 raw `80 mod01 /7 disp7C` = 299 件のうち version 型は FUN_45D5F0/45DBC0/45FC60/462B20/465860 の 17 サイト、他は ASCII 文字比較等の汎用使用。

### 項目6: 書出 size 式 `0x24+len+len+thumb` とヘッダ 38B の 2 バイト差 — **[確定]（2B 差は偽問題）**

**FUN_464AB0 の完全書き出し列（全 site 命令証拠・v309_07/10）**:

| file offset | size | 内容 | 書き出し site |
|---|---|---|---|
| 0..11 | 12 | `"TES4SAVEGAME"`@0xA38FA0 | 0x464B04 |
| 12 | 1 | `this+0x70`（=0） | 0x464B37 |
| 13 | 1 | `this+0x71`（=0x7D） | 0x464B6A |
| 14..29 | 16 | `this+0x94` SYSTEMTIME（GetSystemTime@0x464B95） | 0x464BB4 |
| 30..33 | 4 | `this+0xA4` version(125) | 0x464BE9 |
| **34..37** | **4** | **size = `lea ecx,[eax+edx+0x24]`** | 0x464DEC |
| 38..41 | 4 | `[esi+0x88]` counter（`cmp [esi+0x88],0 / je call 0x464320` で初期化、write 後 `add [esi+0x88],ebp` で +1） | 0x464E74 |
| 42 | 1 | name1 長さ = `strlen+1`（NUL 含み） | 0x464ED1 |
| 43.. | len1 | name1 本体（src=[esp+0x3C]、[0xB333C4] 由来） | 0x464F00 |
| +0 | 2 | u16 = `call 0x5E1FD0`（ecx=[0xB333C4] thiscall、ax→movzx）の戻り | 0x464F34 |
| +2 | 1 | name2 長さ = `strlen+1`（[esp+0x74]） | 0x464F65 |
| +3 | len2 | name2 本体（src=[esp+0x1C]、0x4DE8D0 経由の取得） | 0x464F97 |
| +len2 | 4 | **日数 float**（物理 [esp+0x44]、項目8） | 0x464FCF |
| +4 | 4 | **遊戯時間 ms**（物理 [esp+0x48]、[ebp+0x714]） | 0x465000 |
| +4 | 16 | **SYSTEMTIME**（物理 [esp+0x54] = GetLocalTime 結果） | 0x465044 |
| +16 | 4 | **thumb size**（物理 [esp+0x18]） | 0x465075 |
| ... | thumb | thumbnail データ（call 0x45BAB0 経由） | 0x46509D.. |

**size の内訳（命令証拠で完全一致）**:
```
size = 0x24 + len1 + len2 + thumb
     = [counter 4] + [name1 lenバイト 1] + len1 + [u16 2] + [name2 lenバイト 1] + len2
       + [日数4] + [時間4] + [SYSTEMTIME 16] + [thumb size 4] + thumb
     = 4+1+2+1+4+4+16+4 + len1 + len2 + thumb
     = 36 + len1 + len2 + thumb = 0x24 + len1 + len2 + thumb   ✓ 完全一致
```

- **[確定] size = 「size フィールドの直後（offset38）からファイル末尾までのバイト数」= ファイル総長 − 38**。総ファイル = 38(header through size) + size が成立。
- **[確定] 38B との 2B 差の解釈 = 偽問題**: 38 は「size フィールドまで含む累計」、0x24=36 は「size 以降の残長の定数部分」。**別物の直接比較**であり、内訳（38 = 12+1+1+16+4+4、36 = 4+1+2+1+4+4+16+4）はいずれも命令証拠と一致。v307 残点6 の「2B 差」はこれで解消。
- **[確定] reader 側**: FUN_45DBC0 は size(4B) をローカル [esp+0x14] に読む（0x45DD69..0x45DD83）だけで **name 以降は読まずに `ret 8`**（0x45DE23）。呼び手 0x46112C/0x4640B4/0x464395/0x465999 が size を受け、**0x465999 経路では `push ebx(size)` を含む12引数で FUN_45D5F0 へ転送**（0x465A17..0x465A3F）→ **name1/name2/thumb 読取の実体 = FUN_45D5F0** [確定]。
- **[部分確定] reader の name 読取オフセット**: FUN_45D5F0 内の実際の読取順は本件の帯内だが未全追跡（新規残点4）。ただし writer の書き出し列と size 定義から **offset38 起点**で整合 [推定]。
- **[未確定] size の意味の動的裏付け**: 実 .ess データなし → 「総長−38」の実ファイル照合は未実施（不足証拠）。

### 項目7: v305 raw 579 パターン仕様の復元 — **[確定]**

**`%TEMP%\opencode\v305_23.py`（v305 の中間物・読み取り専用）part(1) を逐語再現**（v309_12）:
```python
THS = {0x1C,0x1D,0x2E,0x34,0x37,0x38,0x51,0x52,0x7D}
for o in range(0, TRS-3):
    b0 = tblob[o]
    if b0 == 0x80:
        modrm = tblob[o+1]
        if (modrm>>6)==0 and ((modrm>>3)&7)==7 and modrm&7 not in (4,5):
            if tblob[o+2]==0x7C and tblob[o+3] in THS: hits.append(...)   # branchA
    elif b0 == 0x3C and tblob[o+1] in THS: hits.append(...)               # branchB
```

- **[確定] 再現結果**: **branchA(mod00 系) = 0 件 / branchB(raw `0x3C <th>`) = 579 件 / total = 579** → v305 記載の 579 と一致。
- **[確定] v305 の 579 の正体 = opcode 0x3C（`cmp al, imm8`）の raw byte スキャン**。`+0x7c` disp は**一切関与しない**（branchA が0件のため）。THS = version 閾値集合。**命令境界検証なし**。
- **[確定] 全579 件の boundary verify（v309_12）**: **真の `cmp al,imm8` 命令起点 = 33 件 / 命令境界外（偽陽性）= 546 件**（偽陽性率 94.3%）。真33件の hist: `0x2E`=11, `0x34`=10, `0x38`=5, `0x37`=3, `0x1C`=2, `0x7D`=1, `0x1D`=1（**0x51/0x52 は 0 件**）。
- **[確定] v307 の 299 との不一致原因**: v307 = `80 /7 mod01 disp8==0x7C`（imm 任意、境界検証 299/299）＝**別パターン**。v305 の 579 は opcode 依存の raw スキャンで、両者は比較対象が異なる。v305 の生パターン仕様は `v305_23.py` に明記あり（本件で復元完了）。
- **[確定負] v305_21.py (E) 節の mod==0 判定も同様の誤り**: `(modrm>>6)==0` を "reg+disp8" とコメントするが実際は disp なし形（o+2=imm8）→ こちらも0件系。

### 項目8: FUN_464AB0 の日数浮動小数（[esp+0x48]）の使途 — **[確定]**

**esp-delta 追跡（v309_12、push/pop/add/sub + call 直後3命令の `add esp` 有無で callee-pop/caller-clean 判定）**:
- 0x464C53 `push edx` → 0x464C58 `call 0x4DE8D0`（直後 add esp なし → callee pop = thiscall `ret 4` [確定(整合)]）→ delta 復帰。
- 0x464CD5 `lea ecx,[esp+0x54]; push ecx` → 0x464CDA `fstp dword [esp+0x48]` → **物理 = delta(−4) + 0x48 = 0x44**。
- 0x464CDE `call [0xA28174]`(GetLocalTime、push1 callee pop) → delta 復帰 0。以降 0x464DB9 まで push/pop 相殺（0x464D08/0x464D40/0x464D1D の `add esp,4/4/8` で確認）。

**[esp+0x44] / [esp+0x48] の全参照（FUN_464AB0 内・v309_13）**:

| site | 命令 | delta | 物理 | 方向 |
|---|---|---|---|---|
| 0x464CC1 | `mov [esp+0x44],eax` | 0 | **0x44** | write（int getter 結果） |
| 0x464CC5 | `fild dword [esp+0x44]` | 0 | **0x44** | read（FPU 読み込み） |
| 0x464CDA | `fstp dword [esp+0x48]` | −4 | **0x44** | write（日数 float が int getter スロットを上書き） |
| 0x464DC7 | `mov [esp+0x48],ecx` | 0 | **0x48** | write（ecx = [ebp+0x714] = 累積遊戯時間 ms） |
| 0x464DFD | `mov [esp+0x48],ebp` | −28 | 0x2C | write（flag 退避、別スロップ） |

**書き出し側の read（物理特定）**:
- 0x464FCF: `push ebp / lea edx,[esp+0x7C] / push edx / push 4 / lea eax,[esp+0x50]` → 3 push 後 = **物理 0x44** → **日数 float を 4B 書き出し**。
- 0x465000: `... lea ecx,[esp+0x54]` → 3 push 後 = **物理 0x48** → **遊戯時間 ms を 4B 書き出し**。
- 0x465044: `... lea edx,[esp+0x60]` → 3 push 後 = **物理 0x54** = GetLocalTime の &SYSTEMTIME → **16B 書き出し**。
- 0x465075: `... lea eax,[esp+0x24]` → 3 push 後 = **物理 0x18** = thumb size → **4B 書き出し**。

- **[確定] 裁定**: 「[esp+0x48] スロット再利用（0x464CDA〜0x464DC7）」は **ディスプレイスメント表記の偶然**。fstp は push ecx 直後の [esp+0x48] = **物理 0x44**、mov は delta 0 の [esp+0x48] = **物理 0x48** → **別物理・別フィールド**。両値はそれぞれ0x464FCF / 0x465000 で 4B ずつファイルへ書き出される（ヘッダの日数 float・遊戯時間 ms フィールド = 項目6の表）。
- **[確定] 日数 float の生成**: `mov ecx,0xB332E0 / call 0x402BD0`（float getter）→ `fdiv qword [0xA2F920]`(÷24) → `fstp [esp+0x34]` → `call 0x402C00`（int getter）→ `fild` + 負値時 `fadd [0xA2FC78]`(2^32 補正) → `fadd [esp+0x34]` → fstp。**V307 残点9 の [推定]「24.0=時→日」を本件で再確認**（÷24 は時間/日換算 [確定]、用途は日数 float のファイル書き出し [確定]）。

### 項目9: new(0x154) と new(0x160) のクラス差 0xC — **[確定]**

| 項目 | new(0x154) | new(0x160) |
|---|---|---|
| 呼び元 | **FUN_45F2E0** mode0 @0x45F6BE / mode1 @0x45F6FC | **FUN_0045D450** @0x45D540（`push 0x160`） |
| ctor | 0x430970（path, flag, 0x20000, 0） | 0x430970（path, 0, 0x20000, 0） |
| vtable | **0xA3636C**（ctor がセット、base stream） | **0xA39094**（ctor 後 `mov [esi],0xA39094` @0x45D577 で差し替え） |
| 追加初期化 | なし | `mov byte [esi+0x154],0` @0x45D57D |
| 用途 | I/O ストリーム（読/書） | **セーブスロット**（list 0x416650 登録、dtor 0x459450） |

- **[確定] vtable 0xA39094 の +0x00..+0x3C は 0xA3636C と完全一致**（read/write/seek/open 等の全スロット同一）→ 派生クラスで**継承オフセット 0**（vtable 先頭共有）。+0x40 以降は RTTI データ（`0x6C756F43` = "Colu…" 連続 ASCII =隣接 RTTI ラベル）。
- **[確定] クラス差 0xC = +0x154..+0x15F の追加 12B**: 0x160=352、0x154=340、差 12。base（0x154）は +0x154 に到達せず（0x00..0x153 のみ使用）。派生クラスは +0x154 に byte フィールドを1つ初期化（他 11B は未初期化のまま [未確定]）。
- **[未確定] +0x154 byte の意味**: 本帯内で以降の書き込み/読取サイトを未特定（新規残点1）。
- **[確定] ctor arg2 flag = +0x20（読/書モード）**: mode0 は flag=1（'r+b' 系＝書）、mode1/ FUN_45D450 は flag=0（'rb' 系＝読）。open 内のモード選択（項目2）と一致。

### 項目10: FUN_45F2E0 の mode 0..3 全経路 — **[確定]**

**プロローグ引数**: this=ecx、arg1=[esp+0x64C]（existing obj or 0）、arg2=[esp+0x650]（path or 0）、**arg3=[esp+0x654]=mode**（`cmp ebp,3 / ja 0x45F763 / jmp [ebp*4+0x45F790]` @0x45F34D）。

**ジャンプテーブル 0x45F790（4 dwords）**:

| mode | 先頭 | 経路（命令証拠） | 戻り値 | ストリーム種別 |
|---|---|---|---|---|
| **0** | 0x45F5EF | ebx==0 → name 生成（arg2==0 なら `call 0x45D920`）→ `sprintf '%s%s%s.ess'` → `CreateDirectoryA` → **.bak ローテーション**（[0xB05BC4]≤10、`DeleteFileA`+`call 0x98544F`(rename)）→ **`push 0x154` new + ctor(path, flag=1, 0x20000, 0)**。ebx!=0 → パス正規化（'.ess' 除去 @0x45F637、'Save ' 前置き判定 @0x45F66B、'autosave' 判定 @0x45F684）→ 該当なしで `call 0x45F2E0(0, &basename, 0)`（再帰）／該当ありで `call 0x453480` 経由の **mode3** | 新規 ptr or 0 | **書モード新規**（flag=1） |
| **1** | 0x45F6FC | `push 0x154` new + ctor(path, **flag=0**, 0x20000, 0) → `mov edx,[esi]; mov eax,[edx+0x18]; push 0; push 0; call eax`（open） | 新規 ptr or 0（`mov eax,esi`） | **読モード新規**（flag=0） |
| **2** | 0x45F756 | `mov edx,[ebx]; mov eax,[edx+0x18]; push 0; push 0; mov ecx,ebx; call eax`（既存 open） | `mov eax,ebx` | **既存再 open** |
| **3** | 0x45F763 | `mov eax,ebx`（open なし） | 既存 ptr | **既存再利用** |

（`ja 0x45F763` = mode>3 は mode3 と同経路 [確定]）

**呼び手の mode 分布（v309_12、push 順の最初の push = arg3 = mode）**:

| 呼び手 | 所属 | push 群 | mode |
|---|---|---|---|
| 0x453495 | FUN_453480（0x453480..0x453471、`ret 4`） | `push 3 / push eax / push edi` | **3** |
| 0x45F6A2 | FUN_45F2E0 内再帰 | `push eax / push ecx / push eax`（eax=0 経路） | **0** |
| 0x45F6B4 | FUN_45F2E0 内（'Save ' 判定該当） | `push 0 ×3` | **0** |
| 0x46110A | FUN_4610F0 | `push 2 / push 0 / push edi` | **2** |
| 0x46409E | FUN_463EC0 | `push 2 / push 0 / push ebp` | **2** |
| 0x46437F | FUN_464320 | `push 2 / push 0 / push ebp` | **2** |
| 0x4651DE | FUN_465130（SaveGame 書出） | `push edi / push ebx / push edx`（edx=[esp+0x6C]=FUN_465130 arg1） | **arg3 = edi** [部分確定] |
| 0x465923 | FUN_465860（ヘッダ読取） | `push 1 / push edi / push esi` | **1** |

- **[確定] FUN_453480(this, arg1, arg2) は内部で mode=3**（0x453491 `push 3`）→ FUN_45F2E0 の mode0 既存経路から呼ばれる（0x45F65D `push 0 / push ebx / mov ecx,esi / call 0x453480`）。
- **[部分確定] FUN_465130 の arg3**: 0x4651DE の `push edi` が mode。FUN_465130 の prologue（sub esp,0x48 + SEH +4push + cookie）から [esp+0x6C]=arg1、[esp+0x70]=arg2（'autosave' 比較用）、edi はローカル由来か引数か未特定（新規残点3）。

### 項目11: FUN_45D390（呼び手 0x4E11A2）の呼び出し文脈 — **[確定]（v307 前提を訂正）**

- **[確定] FUN_0045D390 = 0x45D390..0x45D44A（`ret 4` @0x45D44A → int3×3 → 次 0x45D450）** — **スロット生成ではない**。中身:
  ```
  GetCurrentThreadId（[0xA2808C]）と [0xB33398]+0x10 の照合 → [edi+0x18] bit0 ゲート
  arg1 = [esp+0x10]（= caller の ebp）
  __RTDynamicCast(arg1, 0, TESObjectREFR@0xB03054, Actor@0xB03070, 0)  ; 0x9832E6
  test dword [eax],0x2000000 / je out                     ; flag ビット判定
  call 0x4E0840(this=arg1) → movzx ebp,ax / test bp,bp / je out
  push 0xA3A854 ('TESSaveLoadGame::SaveQueuedAttachedAnimation') / mov ecx,0xB33B80 / call 0x401020（ログ [推定]）
  call 0x452310(SaveGame, arg1) → call 0x4E08D0 → lea ecx,[esi+0x44] / call 0x4210E0
  vfunc+0x40(SaveGame, 0x1000000) / call 0x401040
  ```
  → **役割 = 引数オブジェクト（TESObjectREFR/Actor）の attached animation キュー保存チェック** [確定]。`ret 4` = 1引数（+this）。
- **[確定] セーブ一覧生成の実体 = FUN_0045D450**（0x45D450..0x45D5E2、SEH prologue `sub esp,0x350`、`ret` @0x45D5E2）:
  ```
  GetCurrentThreadId ゲート → [edi+0x6C] リストが非0なら call 0x459400（破棄）
  new(8) → [edi+0x6C] = 新リストノード
  lstrcpyA([0xA28164]) / [0xA28160](PathCombine 系 [推定]) / push 0xA3A884 ('*.ess')
  FindFirstFileA ループ → sprintf '%s%s%s'（0xA2F88C）→ new(0x160) + ctor + vtable 0xA39094
  → call 0x416650(list, slot, dtor=0x459450) で登録
  ```
- **[確定] FUN_45D450 の callers**: 0x464333 / 0x5AE9D8 / 0x5AEBB6 / 0x5B6342 / 0x5D3934 / 0x5D3AC9（6 箇所。0x5AEBB6/0x5D3AC9 は FUN_45E6A0 の呼び手と同じ関数内 = ロード経路）。
- **[確定] FUN_45D220（0x45D220..、`ret 4`）**: GetCurrentThreadId ゲート → `push ebp / push 0xB03070(Actor) / push 0xB03054(TESObjectREFR) / push 0 / push esi / call 0x9832E6`（RTTI cast）→ …。callers = 0x4E1188 / 0x4E1196 の2箇所のみ。**FUN_45D390 と同型の RTTI cast を行う別関数** [部分確定]（残りの本文未追跡）。
- **[確定] caller 0x4E11A2 の文脈**（head 0x4E0F80、imm32 rdata 文字列参照 0 件）:
  ```
  0x4E1159: mov ecx,ebp / call 0x4D7470
  0x4E1160: mov ecx,[ebp+8] / shr ecx,5 / test cl,1 / jne 0x4E11A7   ; フラグ bit5 でスキップ
  0x4E116F: (esi!=0 時) vfunc+0x198(edi) → test al / je 0x4E11A7
  0x4E1181: mov ecx,[0xB33B00] / push ebp / call 0x45D220             ; 条件付き: D220 のみ
  0x4E118F: mov ecx,[0xB33B00] / push ebp / call 0x45D220             ; 基本経路:
  0x4E119B: mov ecx,[0xB33B00] / push ebp / call 0x45D390  ← 0x4E11A2 ;   D220 → D390 の連続
  0x4E11A7: lea ecx,[ebp+0x44] / call 0x41E650
  ```
  → **this = [0xB33B00]（SaveGame シングルトン）、arg = ebp（フォーム/REFR オブジェクト）**。0x4E0F80 は attached animation 保存系の処理関数（文字列参照なし）[部分確定]。
- **v307 残点11 への回答**: 「セーブ一覧生成か新規作成か」→ **どちらでもない**。FUN_45D390 は SaveQueuedAttachedAnimation チェックであり、**セーブ一覧生成は FUN_45D450**（0x4E11A2 の直前 0x4E1196 とは別呼び手群を持つ）[確定]。v307 の「FUN_45D390（スロット生成）」表記は**本件で訂正**（旧結論は削除せず本記録で裁定）。

---

## 2. 判定サマリ

**主判定（11 項目ベース）**

| 判定 | 件数 | 該当項目 |
|---|---|---|
| **[確定]** | **11** | 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11（全項目） |
| [部分確定] | 0 | —（各項目内のサブ判定に付記） |
| [推定] | 0 | —（同上） |
| [未確定] | 0 | —（同上） |
| [確定負] | 0 | —（同上。否定的根拠は各項目内に付記） |

**サブ判定（根拠単位・主要なもの）**

| 判定 | 主な内容 |
|---|---|
| **[確定]** | ReadFile 不在/out・flag 不到達/`add [this+0x148]`/下位4関数の同定/read=0x747E80/静的 CRT stdio（mode 文字列・errno・引数形・DLL 不在）/zlib gz* 否定/FUN_45DE30=0x45DE30..0x45E0CC/__RTDynamicCast×3/0x446B80=formID≥0xFF000000/FUN_45E6A0=0x45E6A0..0x45E933/'CON ' 検査なし＝独立2経路/全ゲート=[0xB33B00]+0x7c/size=offset38..末尾残長・total=38+size・2B差=偽問題/v305 579=opcode3C raw・真33件/物理0x44 vs 0x48 別スロット・両方書き出し/0x154 vs 0x160=vtable 0xA3636C vs 0xA39094・差0xC=+0x154..+0x15F/JT mode0..3 全経路・呼び手 mode 分布/FUN_45D390=SaveQueuedAttachedAnimation・一覧生成=FUN_45D450 |
| **[部分確定]** | once-init 中身（0x401000/0x981FB4 未特定）/FUN_45E6A0 の seek 紐づく ctor/reader の name 読取順（FUN_45D5F0 内）/FUN_465130 arg3=edi/FUN_45D220 残り本文/0x4E0F80 の全体像/FUN_45DBC0 の flag 引数（ebx）の意味 |
| **[推定]** | 0x981FB4=atexit 系/0xA28160=PathCombine 系/0x401020=ログ/ctor arg2=読/書モード（+0x20） |
| **[未確定]** | +0x154 byte の意味/[esi+0x88] counter の初期化 FUN_464320 役割/size の動的裏付け（実 .ess なし）/0x5E1FD0・0x4DE8D0 の正体/FUN_45D5F0 内の name 読取順/FUN_45E6A0 の 2 ctor 分岐構造 |
| **[確定負]** | ReadFile 経路（項目1）/zlib gz*（項目2）/Win32 直叩き（項目2）/v305 の +0x7c 関与（項目7、branchA=0 件） |

---

## 3. 新規残点（次束用・低優先）

1. **0x160 クラス追加フィールド +0x154 byte の意味** — FUN_45D450 @0x45D57D で0初期化のみ。以降の書き込み/読取サイトを帯内外で grep 要。
2. **`[esi+0x88]` counter の初期化 FUN_00464320 の役割** — writer @0x464E4B `cmp [esi+0x88],0 / je call 0x464320`。FUN_464320 は reader caller（0x464395）でもある = 既存ヘッダ読取と counter 保存の関係。
3. **FUN_465130 の arg3（mode）実値** — 0x4651DE の edi の出所（prologue のローカルか引数か）。
4. **FUN_45D5F0 内の name/thumb 読取順** — size 引数（0x465A3B `push ebx`）の使われ方、offset38 起点の裏付け。
5. **0x5E1FD0（u16 戻り）と 0x4DE8D0（thiscall ret 4、name2 取得）の正体** — 2B フィールド・name2 の意味論。
6. **FUN_45E6A0 の seek(0xD000) の紐づく ctor** — 0x45E77A/0x45E7F0 jmp 分岐の追跡（XBoxSaves 経路か .ess 経路か）。
7. **0x4E0F80 の全体像** — 文字列参照0件、vfunc+0x198/+0x188 の対象。
8. **FUN_45D220 の残り本文** — RTTI cast 後の処理（callers 0x4E1188/0x4E1196 のみ）。
9. **FUN_4302B0 once-init の 0x401000/0x981FB4 正体** — グローバル初期化の帰属。

---

## 4. selftest 再現メモ

実行環境: `chcp 65001` → `python $env:TEMP\opencode\v309_<n>.py *> $env:TEMP\opencode\v309_<n>.out`（%TEMP%=C:\Users\hiroki.kogarumai\AppData\Local\Temp）。全スクリプトは `v307_common.py`（F(va)/func_head/find_head/callers_of/resolve_imm32_refs/strings_in/str_at）に依存。

| スクリプト | 内容 | 主な結果（EXIT=0） |
|---|---|---|
| v309_01 | FUN_4302B0 full dump + 下位 747E20/747F40 + callers | once-init/lazy open/call 0x747E80/add [esi+0x148]、fseek/fread 呼び出し形、0x747E80=0x747E80 発見 |
| v309_02 | vtable 0xA3636C 全展開 + open FUN_42FE80 + CRT 候補 + 帯走査 | vtable +0x18=0x42FE80、mode 文字列6種、fread/fwrite/fseek 同定、heads 20 個 |
| v309_03 | FUN_45DE30 帯 dump + RTTI 名 + caller 0x4627E1 + 0x46B250/0x446B80 | TD 名4種、__RTDynamicCast 1819 callers、range 未確定（→v309_05） |
| v309_04 | FUN_45E6A0 帯 dump + callers 0x5AEB8B/0x5D3AAA | 'XBoxSaves\*.*'、seek 0xD000、0xAA000/0xA9000、[0xB33B00] 呼び手（range 未確定→v309_05） |
| v309_05 | IAT DLL 目録 + open linear + **ret/CC 境界解析（5 関数）** | DLL 14 種（CRT 不在）、FUN_45DE30=0x45E0CC 終端、FUN_45E6A0=0x45E933、FUN_45FC60=0x45FD93、FUN_462B20=0x4636F1 |
| v309_06 | gate サイト9箇所の文脈 + 各 prologue | **全 gate = [0xB33B00]+0x7c**、FUN_45FC60 prologue の al ゲート確認 |
| v309_07 | FUN_464AB0 writer full + FUN_45DBC0 reader tail | 書き出し列 12/1/1/16/4/4B、size 式、name len= strlen+1、物理 slot の lea 群 |
| v309_08 | v305_23 part1 逐語再現 | **branchA=0 / branchB=579 / total=579**、mod01 raw=293/24（THS） |
| v309_09 | FUN_45F2E0 full + JT + alloc + FUN_45D390 alloc + caller 0x4E11A2 | JT 4 エントリ、new(0x154)×2 / new(0x160)、vtable 差し替え+0x154 byte、0x4E11A2 文脈 |
| v309_10 | FUN_45DE30 full + FUN_45DBC0 full + callers + writer tail + gate base + vtable 0xA39094 | **範囲確定（0x45E0CC）**、reader 完全版、writer tail（size 後の全 site）、gate base=0xB33B00、vtable 一致 |
| v309_11 | esp 追跡 + item7 verify + item10 callers + item11 strings | **TypeError 途中失敗**（callee ret-N None）→ v309_12 で再実装（probe のみ再調査しない方針に従い他結果は統合済み） |
| v309_12 | item7 全579 verify + esp 追跡再実装 + reader name + item10 push + item11 strings/heads | **真33/偽546**、esp 相殺ルール、FUN_45D5F0 への size 転送、mode 分布、FUN_45D220/45D390 head |
| v309_13 | RTTI TD 名 + FUN_45D450 callers + writer slot refs + FUN_465130 callers | TESObjectREFR/Actor、FUN_45D450 callers 6 箇所、slot refs 5 件、FUN_465130 mode 呼び手 |

- 再実行は上記コマンド列で同一出力（決定的）。v309_11 のみ一部失敗だが、失敗範囲は v309_12 で再実装済み → **全 probe の結果は再調査せず本成果物へ統合**。
- **v307 由来の既知事項のうち本件で訂正/昇格したもの**（旧結論は削除せず裁定理由を記録）:
  ① v307 残点4「0x45DD83 4B = 位置確定・内容未使用 [部分]」→ **内容 = size、呼び手が name 読取に使用 [確定]** に昇格。
  ② v307 残点5「帯外ゲート候補オブジェクト未同定 [部分]」→ **全サイト [0xB33B00]+0x7c [確定]** に昇格。
  ③ v307 残点6「thunk out/flag は (dst,n) のみ転送 [確定:命令]、read 実装での out 扱い未解析 [部分]」→ **out/flag は FUN_4302B0 に到達しない [確定]**。
  ④ v307 残点7「下位層 0x747Exx 未同定 [部分]」→ **read=0x747E80・静的 CRT stdio [確定]** に昇格。
  ⑤ v307 残点11「FUN_45D390 = スロット生成」→ **訂正: スロット生成は FUN_0045D450、FUN_45D390 は SaveQueuedAttachedAnimation チェック**。
  ⑥ v307 残点9「[推定] 24.0=時→日」→ **用途（日数 float のファイル書き出し 0x464FCF）[確定]** に昇格。
  ⑦ v305「raw 579」→ **opcode 0x3C raw・境界検証なし・真33 件 [確定]**（v307 の「再現不能」から「仕様復元完了」へ）。

---

## 5. docs/RE_EXE_REPORT.md 等への反映要否

**本指示のみでは docs/移植コード/CI/TODO/STATE は変更しない**（追加契約 §成果物）。人間確認後の反映候補（正典 `docs/RE_EXE_REPORT.md` を正とし、矛盾時は §55 補遺→§50 の順で優照合。矛盾点は旧結論を削除せず裁定理由を記録 = 本ファイル §4 訂正一覧）:

1. ストリーム下位層 = 静的 CRT stdio（fopen/fseek/fread/fwrite + 自前バッファ、mode 文字列 0xA36338..34C）— **docs 未出（本件が初出）**
2. read 相当 = FUN_00747E80（+0x148 加算、3 分岐）— docs 未出
3. FUN_45DE30 = created base objects 検証 + "Enchantable item…" ログ + RTTI 3 種（docs の「FUN_45DA30」記述は v307 で非関数と裁定済、本件で 0x9832E6 帯の実体を追記）
4. FUN_45E6A0 = XBoxSaves 列挙 + 0xD000 無条件（'CON ' 検査なし、独立2経路）— docs 未出
5. version gate 全サイト = [0xB33B00]+0x7c、閾値表 — docs はゲート値のみ出（オブジェクトは v307 由来）
6. size = offset38..末尾の残長（total = 38+size、2B 差=偽問題）+ writer 完全列 — docs 未出
7. new(0x154)/(0x160) = stream / save-slot（FUN_45D450、vtable 0xA39094、+0x154..+0x15F）— docs は「セーブスロット 0x160B」のみ出
8. FUN_45D390 = SaveQueuedAttachedAnimation チェック（v307 訂正の追記）— docs 未出
9. v305 raw579 の仕様復元（opcode 3C raw、真33 件）— docs 未出

**変更すべきでないもの**: GT・ソース（app/**）・CI・TODO.md・STATE.md・他 lane（v308/v310/v311）の成果物。

---

## 6. #70 完了条件の充足（5項目）

| 完了条件 | 充足 | 根拠 |
|---|---|---|
| ① FUN_004302B0 内部 read 実装に命令証拠ベースの結論 | ✅ | §1 項目1（full dump + 3 分岐 + ReadFile 否定 + out/flag 不到達） |
| ② 0x747E20 / 0x747F40 下位層の所属または未確定理由の記録 | ✅ | §1 項目2（[確定] 静的 CRT stdio、根拠5項目） |
| ③ 11 残点すべてが確定度付きで分類 | ✅ | §1 項目1〜11、§2 サマリ（主判定 [確定] 11/11） |
| ④ 推測実装なし | ✅ | 全 [推定]/[未確定] を明示（§2 サブ判定、§3 新規残点9件） |
| ⑤ v309_*.md に再現可能な根拠 | ✅ | 本ファイル全 VA/命令列 + §4 selftest 表（probe 13 本） |

**帯境界遵守**: 0x45D000..0x466000 ＋ 0x747000..0x748000 ＋ zlib ラッパ帯のみ。v308 帯（0x45A000-0x45CFFF/0x494000-0x496000）非侵入。ESM 読取・git 操作なし。成果物は本ファイルのみ新規作成（v304〜v307 成果物・台帳・STATE・TODO・GT・ソース・.github/**・docs/ は未編集）。

## 7. 訂正（v313 統合時・2026-11-07）

後続束 `v313_save_ess_boundary_followup.md`（51,732B/453行・probe14本・selftest MD5 前後一致）により以下を**訂正**する（旧記述は削除せず本節で上書き裁定）:

1. **`0x459450` は「dtor」ではなく FILETIME 比較関数** — `FindFirstFileA`→`ftLastWriteTime`→3-way 比較で **`+0x154` valid flag** の判定元。本ファイル L250/L308 の「dtor 0x459450」表記を**訂正**。[確定]
2. **`FUN_45D5F0` は 10 引数（`ret 0x28`）** — 従来の引数件数 12 の記述は 10 へ統一（引数列 F1 counter/F2 name1/F3 u16/F4 name2/F5 float/F6 playtime/F7 SYSTEMTIME（fallback=FileTimeToSystemTime）/F8- thumb・詳細と該当箇所一覧は v313 §6）。[確定]
3. **`0x4E0F80` は「文字列参照 0 件」ではなく `'BSX'`・`'REF'` を検出** — 本ファイル L312/L322 の記述を**訂正**（範囲 0x4E0F80..0x4E1571・callers 7・Interlocked 使用）。[確定]
4. **境界 3 件の過走を再検証で訂正** — 本ファイル §1 に「初回 dump は境界未検証」と明記した箇所（FUN_45DE30/FUN_45E6A0 帯ほか）を v313 が CC ラン規則で再確定（v313 §6 に整合表）。[確定]
5. **新同定**: `+0x88` counter 初期化 fn = **`FUN_0045B300`**（SaveGame ctor・singleton store 0x441BB9）— off-by-pad **第 5 例 = `0x45B300`** を教訓として追加（既知 4 例: 0x466D4D/0x5AE18D/0x44F3C1/0x51B484）。[確定]

**整合（v313 主判定）**: 9 項中 [確定]8/[部分確定]1。**実 `.ess` サンプルは不在 [確定]**（8 系統検索ゼロ）→ 指示書の動的照合 5 項目は未実施（本ファイルのバイナリ構造記述は静的範囲内と再確認）。本件に矛盾する v309 結論は他にない。
