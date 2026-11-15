# v395 — `0x6C4200` の所属と初期化元の裁定（#153 ⑥-1・V-382-3）

- 採番: **v395**（ai-todo #153 束の第6弾＝低10件の第1弾、probe prefix v395_1..v395_11）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v382 の残点 **V-382-3（`0x6C4200` の所属と初期化元 — NiControllerManager::Update 相当 [推定高]・`[this+8]` bit3・`[this+0x4C]`（sequence 配列）/`[this+0x54]`（count）の構築元【低】）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内 RTTI・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **所属** | **[確定(RTTI＋命令)]** | **NiControllerManager**。証拠の三重: ①`0x6C4200` の packed dword は**全ファイル走査で1件のみ**（file 0x678A58 = **VA 0xA79858 (.rdata)**）＝**vtable VA `0xA79804` の slot21（+0x54）** ②**vtable 直前の RTTI COL（dword 0xA79800 = VA 0xAC5F78）の TypeDescriptor = `.?AVNiControllerManager@@`** ③**ctor `FUN_006c5520` が `*param_1 = &PTR_FUN_00a79804`（本 vtable）を格納**。E8/E9/即時参照＝**0件** → **virtual dispatch**（`call dword [vptr+0x54]` の機序を .text 内6件で確認 [確定(命令)]）。**v382 の「NiControllerManager::Update 相当 [推定高]」を確定に昇格** |
| 機能（再掲） | **[確定(命令＋decompiled)]** | `[this+8]` bit3 ゲート下で有効 sequence 配列 **`[this+0x4c]`（count `[this+0x54]`）** を走査し各 sequence へ `(time, flag=1)` で `0x6CA950`（update・v393 jump table fn）を呼び、**`state==0` の sequence を swap-with-last で除去**（count--） |
| **初期化元（ctor）** | **[確定(命令＋decompiled L495062-495104)]** | **`FUN_006c5520`（`0x6C5520`・NiControllerManager ctor）**: base ctor `FUN_00715990` → **vtable `0xA79804` 格納** → secondary vtable `[+0x3c]=0xA7977C` → word `[+0x44]=0`/`[+0x46]=0`/`[+0x48]=0`/`[+0x4a]=10` → `[+0x40]=0`・**`[+0x4c]=0`・`[+0x50]=0`・`[+0x54]=0`** → **`[+0x5c]=0x25`（capacity 37）→ `new(0x94)`（=37×4）を `[+0x60]` に格納し memset 0** → `[+0x58]=0xA798A0`（二次 vtable・先の0xA7975C から上書き）→ `[+0x64]/[+0x68]/[+0x6c]/[+0x70..+0x7c]=0`。**dtor `FUN_006c5120` が `[+0x4c]` を `free`（0x6C51C8）**＝alloc/free 対応。第2 ctor `FUN_006c5610`（0x6C5610）も `[+0x4c]/[+0x54]` をゼロ初期化（同一クラスかは [未確定]） |
| 積み上げ（非ゼロ投入） | **[部分確定(否定側)]** | **0x6C band 内に m_pArray(+0x4c)/count(+0x54) の非ゼロ書込は存在しない**: store census（opcode 89/C7 disp=0x4C/0x54）＝**ゼロ初期化2件のみ**（0x6C5576/0x6C5668）／increment census（83 /0 imm=1・01 /r・FF /0）＝**band 内 append 0件**（存在したのは減算2系統: Update内 `count--`（0x6C4233）＋remove loop `add [edi+0x54], ebx`（0x6C4B6F・`FUN_006c4a10` 経由 L494512 も count−1））／`lea reg,[reg+0x4c]` band 内0件／decompiled L490000-500000 の書込 grep＝**0件** → **band 外（game 側等）の Register/Add 経由で投入されると確定（否定側）・具体的 Register fn は残点** |
| bit3 ゲートの setter | **[部分確定]** | `FUN_006c46ab80`（`0x46AB80`）＝**`cmp byte [esp+4],0; je→and dword [ecx+8],0xFFFFFFF7 / or dword [ecx+8],8; ret 4`＝SetBit3(bool) 相当 tiny fn**（E8 多数: `push 1; mov ecx,esi; call` 形で0x415BB2/0x44BD4A/0x4A3212...）＋**inline `or dword [reg+8],8` サイト群**（0x475969/0x49E1D6/0x4DA632/**0x4DA68C**（`[esi+0x40]` 読み＋`[edi+0x44]` state チェック＝manager+seq 隣接を確認）/0x4DE555（`Deactivate 0x6C9CB0` 呼び出しと隣接）/0x4E3061/0x4E31AD/0x4E3721...）。**+8 フィールドの正式名は残点** |

**V-382-3 への回答**: `0x6C4200` の所属は **NiControllerManager [確定(RTTI)]**、**vtable slot21（VA 0xA79804+0x54）の virtual Update**。初期化元は **ctor `FUN_006c5520` [確定]**（zero-init ＋ capacity 37 の buffer を `[+0x60]` に構築）だが、**`[+0x4c]`/`[+0x54]` への非ゼロ投入は0x6C band 内に存在せず（否定側確定）、band 外の Register/Add 経由 [部分確定]**。

---

## §2 証拠

### §2.1 vtable 検出（v395_2(b)/v395_3(a)/v395_4/5）

- `.text` 即時参照（E8/E9/packed dword/push/mov imm32）＝**すべて0件**（v395_1/2/3/4）。
- **全ファイル packed dword `0x006C4200` ＝1件**: file 0x678A58 → **VA 0xA79858 (.rdata)**。
- PE セクション表（**COFF ヘッダ20バイト・opt=pe+24 で修正後**）: `.text` VA=0x401000 raw=0x400 / `.rdata` VA=0xA28000 raw=0x627200。
- vtable extent: **VA 0xA79804..0xA79878（30 slots）**、`0x6C4200` = **slot21**。前後 slot 例: slot0=`0x6C55F0`（scalar dtor）/slot6=`0x6C4260`/slot10=`0x6C42B0`（IsEqual 系・decompiled FUN_006c42b0）/slot22=`0x68F970` 等。

### §2.2 RTTI（v395_5(b)）

```
vtable-4 = 0xAC5F78 → COL { sig=0, offset=0, cdOffset=0, pTD=0xB24100, pCHD=0xAC5F8C }
pTD → TypeDescriptor name = '.?AVNiControllerManager@@'
CHD numBaseClasses = 4（base 名の取得は配列レイアウトずれで未取得＝残点）
```

### §2.3 ctor（v395_5(c)/v395_6(a) unfiltered ＋ decompiled L495062-495104）

```
6C5520: SEH prologue → mov esi,ecx
6C5549: call 0x715990（base ctor・FUN_00715990）
6C5550: [esi] = 0xA79804            ★primary vtable
6C555A: [esi+0x3c] = 0xA7977C       ★secondary vtable
6C5561..6F: word [+0x44]=0 / [+0x4a]=0xA / [+0x46]=0 / [+0x48]=0
6C5573: [+0x40]=0
6C5576: [+0x4c]=0 ; 6C5579: [+0x50]=0 ; 6C557C: [+0x54]=0   ★★ゼロ初期化
6C5581: eax=0x25 ; 6C5586: [+0x5c]=0x25（37）
6C5589..A7: new(0x94) → 6C55B6: [+0x60]=ptr → memset(0, 0x94)
6C55C4: [+0x58] = 0xA798A0（二次 vtable・6C5598 の0xA7975C を上書き）
6C55C1..D7: [+0x68]=0 / [+0x6c]=0 / [+0x70..+0x7c]=0 → ret
dtor 0x6C5120: 6C51C8: mov eax,[esi+0x4c]; push eax; call 0x401F20（free）
```

### §2.4 積み上げ census の否定側（v395_4(d)/v395_7/8/9/10）

| 手法 | 結果 |
|---|---|
| `mov/c7 [reg+0x4c/0x54], ...` band 内 | **ゼロ初期化2件のみ**（0x6C5576/0x6C5668） |
| `add [reg+0x54], 1`（83 /0 imm=1）band | 0件（0x6D2327 は float 書き込みの別クラス） |
| `add [reg+0x54], r32`（01 /r）band | **1件 = 0x6C4B6F（remove の減算）** |
| `inc [reg+0x54]`（FF /0）band | 0件 |
| `lea reg,[reg+0x4c]` band | 0件 |
| decompiled L490000..500000 の `+0x4c/0x54`・`[0x13]/[0x15]` 書込 | **ctor ゼロ初期化・Update/`FUN_006c4a10` の count−1・sequence 系 float 書き込みのみ（append 0件）** |

→ **非ゼロ投入は band 外（否定側の確定）**。減算系の所在: Update内 `count--`（0x6C4233）／**`FUN_006c4a10`（0x6C4A10・L494512 `count = count + -1`）**＋remove loop 0x6C4B63-0x6C4B88（`0x6C5860` lookup からも `call 0x6C4A10`＝v392 で接続済み）。

### §2.5 virtual dispatch（v395_11）

- `call dword [reg+0x54]`（FF /2 disp=0x54）＝.text 内 **6件**（0x8AA07B/0x8B8C98/0x8CBB89/0x8D9216/0x91FAE9/0x91FB01）＝**slot21 呼び出し機序の実在** [確定(命令)]。

### §2.6 bit3 setter（v395_6(c)/v395_7(d)）

```
46AB80: cmp byte [esp+4],0; je 0x46ab8e
46AB87:   or dword [ecx+8], 8    ; ret 4
46AB8E: and dword [ecx+8], 0xFFFFFFF7 ; ret 4
（直後 0x46ABA0 系: bit11 の同型 pair ＋ vtable[0x40] 呼び出し）
```
- inline サイト: `or dword [reg+8],8` は .text 多数（cap12 で0x4E3721 まで・他多数）。**0x4DA68C は `[esi+0x40]` 読み＋`[edi+0x44]` state チェック**、**0x4DE555 は `call 0x6C9CB0`（Deactivate）と隣接**＝manager+seq 文脈を確認。

## §3 残点 **V-395-1..3**
1. **`[this+0x4c]`/`[this+0x54]` への非ゼロ投入（Register/Add fn）の所在**（0x6C band 外・game band の `lea +0x4c` census は cap 耗尽で未完）【中】
2. **`[this+8]` bit3 の正式名** ＋ `FUN_006c46ab80` 呼び出し元が NiControllerManager で確定する裏付け（caller 側 ecx の同定）【低】
3. **NiControllerManager の base 4クラス（CHD base[0..3]）の完全同定**（CHD の pBaseClassArray 読み取りずれで未取得）＋ secondary vtable `0xA7977C`/`0xA798A0` の各 slot 意味【低】

## §4 検証・自己批評
- probe: **v395_1..6, v395_8..11 全 EXIT=0**、**v395_7 は初回 tuple bug で EXIT=1 → 修正後 EXIT=0**。
- **決定打の作り**: (i) E8/E9/即時0件 → (ii) **全ファイル packed dword 走査で vtable slot を1件検出** → (iii) **RTTI COL→TD でクラス名取得** → (iv) **ctor の vtable 格納＋field 初期化を machine/decompiled 双方で確定** → (v) **store/increment/lea/decompiled 4種 census の否定側で「band 外の投入」を確定** → (vi) `call [vptr+0x54]` 機序確認。
- 妥当性チェック: ①RTTI TD⇔ctor vtable 格納＝**独立二重一致** — ②Update の読み field（+0x4c/+0x54/+8bit3）⇔ctor の zero-init field 一致 — ③dtor free⇔ctor alloc 対応 — ④E8/E9/imm 0件⇔vtable slot 整合 — ⑤v382 仮説（Update 相当[推定高]）と**矛盾なし・確定昇格**。
- docs との不整合: **なし**（v382 の V-382-3 を解消）。
- **教訓**: ①**PE セクション表は COFF ヘッダ20バイト（opt = pe+24）を厳守**（+24 ずれで nsec/VA 全滅→全結果が誤った VA になる）②**vtable 検出の定石: .text 即時0件 → 全ファイル packed dword 走査 → vtable-4 の COL→TD の RTTI 鎖**（RTTI はクラス名の唯一の決定打）③Ghidra call 表示の hidden this 省略（v394 教訓）は本帯でも再現（`FUN_006ca950(param_2,1)`）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
