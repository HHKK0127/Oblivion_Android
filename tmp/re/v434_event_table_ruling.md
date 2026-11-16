# v434 — id 0x00..0x0F 帯の所有者（script event table）と animation name 群の裁定（V-434-1..3 完遂・派生残点第26弾）

- 採番: **v434**（派生残点群・probe prefix **v434_1..v434_29 の29本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v433 の残点 **V-434-1【低】（id `0x00..0x0F` 帯の正体）／V-434-2【低】（rec370 以降の idle animation 名テーブル構造と consumer）／V-434-3【低】（`FUN_004FD7C0` の unannotated callers 4件の identity）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-434-1: id `0x00..0x0F` 帯の正体** | **[確定]（表実読）** | **`0x00..0x0F` の所有者は script event テーブル `.data 0xB0AF48`**（stride `0x28`・**31件・id `0x00..0x1E`**）。実読: **`GameMode(0)/MenuMode(1)/OnActivate(2)/OnAdd(3)/OnEquip(4)/OnUnequip(5)/OnDrop(6)/OnYield_unused(7)/OnHit(8)/OnHitWith(9)/OnDeath(0xA)/OnMurder(0xB)/OnKnockout(0xC)/OnAlarm(0xD)/OnAlarmVictim(0xE)/OnPackageStart(0xF)`** → `OnPackageDone(0x10, alias='OnPackageEnd')/ScriptEffectStart(0x11)/ScriptEffectFinish(0x12)/ScriptEffectUpdate(0x13)/OnPackageChange(0x14)/OnLoad(0x15)/OnMagicEffectHit(0x16)/OnSell(0x17)/OnTrigger(0x18)/OnStartCombat(0x19)/OnTriggerActor(0x1A)/OnTriggerMob(0x1B)/OnActorEquip(0x1C)/OnActorUnequip(0x1D)/rec30(0x1E)` — **TES4 script event 定義 [確定(実文字列)]**。console table（`0xB0B420`）と**物理隣接**（`0xB0AF48 + 31×0x28 = 0xB0B420` ✓） |
| **keyword table と 0x00..0x0F の関係** | **[確定(否定側)]** | keyword table（`0xB0A2C8`）は **`0x10..0x1F` のみ**（rec16 name ptr=0 で終端）で、**直前は AV name table（stride4）が連続する＝`0x00..0x0F` を持つ table は keyword table 直前に存在しない**。**dispatch switch（`0x4FF7B9`: `id-0x10 <= 0xF` → keyword、default で console `0x100..0x182` → function `0x1000..0x1170` → それ以外は `Unknown function code %d.` エラー）にとって `0x00..0x0F` は [確定(否定側)] 無効 id** → **keyword/console/function の id 空間と event の id 空間は別系統 [確定]**（重複 `0x10..0x1E` があるため同一空間でない） |
| **event table の consumer** | **[確定]** | **`FUN_004ff4c0`（script parser）内 `0x4FF904: lea eax,[ebx + 0xB0AF48]`**（base＋byte offset 形・`[eax+0x10]` の byte 判定と `push 0xA4B524`（エラー文言）→ `FUN_004fce30`）＝ **event record を offset で直接取得 [確定(命令)]**。＋ `FUN_004fce30`＝エラー報告 fn（`"Script command \"%s\" not found."` 等を整形・L196408-） |
| **V-434-2: rec370 以降の構造** | **[確定]（構造実読）＋[推定高]（用途）** | function table sentinel（`0xB10268`）の後ろには **別構造が3系統** 連続: **(a) flat string-ptr 配列 `0xB10290..0xB102DC`（20件・stride4）**: `'Left Foot'/'Right Foot'/'Left Back Foot'/'Right Back Foot'/'Idle'/'Aware'/'Attack'/'Hit'/'Death'/'Weapon'/(空)/'Sneak'/'Swim'/'Fly'/(空)/'HandToHand'/'OneHand'/'TwoHand'/'Staff'/'Bow'` ／ **(b) stride-`0x24` レコード table `0xB102E0` 起点**: `rec0='Idle'`／`rec1='DynamicIdle'`／`rec2='SpecialIdle'`（`0xB10328`＝rec2 直接アクセスで stride 検証）／`rec3='Forward'`／`rec4='Backward'`／`rec5='Left'` …（field0=name、他は numeric: `+0x10=0/+0x14=1/+0x18=0xFFFFFFFF` 等）／**(c) parallel string 配列 `0xB108EC`**: `'LowerBody'/'LeftArm'/'LeftHand'/'RightArm'/'SpecialIdle'/'Start'×8/'End'×2/'Attach'/'Detach'/'Hit'×2/'Attack'/'Attach'` — **(a)(b)＝animation group/state 名、(c)＝NiControllerManager sequence group 名＋text key（event）名 [推定高]**（Gamebryo 標準名と一致） |
| **rec370+ の consumer** | **[確定]** | **`FUN_00407AE0`（L5705・巨大 fn）** が **`lea edx,[eax+eax*8]; mov eax,[edx*4+0xB102E0]`（＝`0xB102E0 + idx×0x24`＝stride-0x24 レコード name 取得）**→ `call 0x51A9E0/0x51A9D0`（name→index 変換 helper）→ **`mov ecx,[eax*4+0xB102C8]`／`mov edx,[eax*4+0xB102B8]`／`mov ecx,[eax*4+0xB108EC]`** → **format 文言 `'%s -> %s/%s/%s, Count: %d'`（`0xA30670` 実読）で4 name＋count を出力** ＝ **animation group の dump/report [推定高]**。base refs: `0xB102B8`＝6件／`0xB102C8`＝11件／`0xB102E0`＝22件／`0xB108EC`＝3件（`0x40A2A4`/`0x4FEB7C`/`0x509FDC`） |
| **V-434-3: `FUN_004FD7C0` の callers** | **[確定]** | E8=8件: **`0x4FDBC7`（自己再帰）／`FUN_004FDE30`（×1）／`FUN_004fef80`（×4: `0x4FF015/0x4FF12E/0x4FF26A/0x4FF3C7`）／`FUN_004ff4c0`（×2: `0x4FF5AE/0x4FF6BD`）**。**unannotated 4件は全て `FUN_004fef80`（L197796）に所属 [確定]**（annotation 範囲走査で特定）— `FUN_004fef80`＝**`__alloca_probe` 使用・512B ローカル buffer 多数の script source 処理 pass [推定高]**。`FUN_004ff440`（L197991）＝**parser state の dtor/reset**（`[+0x20]`/`[+0xc]` free、`[+0x10]`/`[+0x12]` u16 ゼロ）[確定] |
| **`FUN_004FDE30` の cross-link（副次）** | **[確定]** | **`+0x1C` フィールド＝`0x4FDE30` が event/console/function table の全 rec で共通 [確定]**（`FUN_004FDE30` 自身が `FUN_004FD7C0` を呼ぶ）→ **`+0x1C`＝共通の name/parse callback [推定高]** |

## §2 証拠（v434_1..v434_29）

### §2.1 event table 実読（v434_27/v434_28）
```
0xB0AF48 stride 0x28, 31 records, id = index:
rec0  'GameMode'          id=0x00  +0x10=0x0      +0x18=0x5022D0
rec1  'MenuMode'          id=0x01  +0x10=0x10000  +0x18=0x502270
rec2  'OnActivate'        id=0x02  +0x10=0x10001  +0x18=0x503F90
rec3  'OnAdd'             id=0x03
rec4  'OnEquip'           id=0x04
rec5  'OnUnequip'         id=0x05
rec6  'OnDrop'            id=0x06
rec7  'OnYield_unused'    id=0x07
rec8  'OnHit'             id=0x08
rec9  'OnHitWith'         id=0x09
rec10 'OnDeath'           id=0x0A
rec11 'OnMurder'          id=0x0B
rec12 'OnKnockout'        id=0x0C  +0x10=0x1
rec13 'OnAlarm'           id=0x0D  +0x10=0x20001
rec14 'OnAlarmVictim'     id=0x0E  +0x10=0x20001
rec15 'OnPackageStart'    id=0x0F  ← 0x00..0x0F 帯の最終
rec16 'OnPackageDone'     id=0x10  +4='OnPackageEnd'（alias）
rec17 'ScriptEffectStart' id=0x11
rec18 'ScriptEffectFinish'id=0x12
rec19 'ScriptEffectUpdate'id=0x13
rec20 'OnPackageChange'   id=0x14
rec21 'OnLoad'            id=0x15
rec22 'OnMagicEffectHit'  id=0x16
rec23 'OnSell'            id=0x17
rec24 'OnTrigger'         id=0x18
rec25 'OnStartCombat'     id=0x19
rec26 'OnTriggerActor'    id=0x1A
rec27 'OnTriggerMob'      id=0x1B
rec28 'OnActorEquip'      id=0x1C
rec29 'OnActorUnequip'    id=0x1D
rec30 (id=0x1E)
rec31 @0xB0B420 'Show' id=0x100  ← console table 開始（id break）
layout: {+0=name, +4=alias, +8=id, +0xC=help, +0x10=flags, +0x14=ptr,
         +0x18=handler fn, +0x1C=0x4FDE30(共通), +0x20=0, +0x24=0}
```
- `0xB0AF48 + 31×0x28 = 0xB0B420` ✓（event→console の物理隣接）。

### §2.2 dispatch switch の id 空間（v434_3/v434_5/v434_15/v434_16/v434_17）
```
FUN_004ff4c0 @0x4FF7B9:
  mov eax,[esi+0x410]; lea ecx,[eax-0x10]; cmp ecx,0xf; ja 0x4fff43
  movzx edx, byte [ecx + 0x500114]        ; byte table [0,1,2,2,2,3,4,5,4,6,7,7,6,6,6,2]
  jmp dword [edx*4 + 0x5000F4]            ; 8-case jump table
default @0x4FFF43:
  lea ecx,[eax-0x100]; cmp ecx,0x82; ja ... ; console 0x100..0x182 → 0xB0B420
  lea ecx,[eax-0x1000]; cmp ecx,0x170; ja ...; function 0x1000..0x1170 → 0xB0C8C0
  push eax; push 0xA4B338; ...            ; 'Unknown function code %d.'
```
- ⇒ **`0x00..0x0F` は keyword/console/function のどの band にも属さずエラー**＝**event id 空間は別系統 [確定]**。
- 文言実読: `0xA4B338='Unknown function code %d.'`／`0xA4B21C='Script command "%s" is a console-only command.'`／`0xA4B1FC='Script command "%s" not found.'`（v434_17）。

### §2.3 rec370+ の3構造（v434_1/v434_3/v434_8/v434_12/v434_18/v434_20）
```
(a) flat string array 0xB10290..0xB102DC (20, stride4):
    'Left Foot','Right Foot','Left Back Foot','Right Back Foot',
    'Idle','Aware','Attack','Hit','Death','Weapon',
    ''(0xA2F7EC),'Sneak','Swim','Fly','',
    'HandToHand','OneHand','TwoHand','Staff','Bow'
(b) stride-0x24 records @0xB102E0:
    rec0 'Idle'         {+0C=1,+14=1,+18=0xFFFFFFFF}
    rec1 'DynamicIdle'
    rec2 'SpecialIdle'  {+08=4,+14=1,+18=0xFFFFFFFF}   ← 0xB10328 直接 base
    rec3 'Forward' / rec4 'Backward' / rec5 'Left' ...
(c) parallel string array 0xB108EC:
    'LowerBody','LeftArm','LeftHand','RightArm','SpecialIdle',
    'Start'×8,'End'×2,'Attach','Detach','Hit'×2,'Attack','Attach'
```

### §2.4 consumer（v434_11/v434_18/v434_19/v434_20）
```
FUN_00407AE0 (L5705):
  40A26C: call 0x51aa00                      ; name→idx helper
  40A271: lea edx,[eax+eax*8]                ; ×9
  40A274: mov eax,[edx*4 + 0xB102E0]         ; → 0xB102E0 + idx*0x24 = rec.name
  40A280: call 0x51a9e0 / 40A291: call 0x51a9d0
  40A285: mov ecx,[eax*4 + 0xB102C8]
  40A296: mov edx,[eax*4 + 0xB102B8]
  40A2A1: mov ecx,[eax*4 + 0xB108EC]
  40A2B4: push 0xA30670 ; call 0x98208b      ; '%s -> %s/%s/%s, Count: %d'
```
- base refs（v434_18）: `0xB1026C`=1／`0xB10270`(sentinel id)=1／`0xB102B8`=6／`0xB102C8`=11／`0xB102E0`=22／`0xB102E4/8/C/F0`（stride-0x24 field base）／`0xB10328`(rec2)=2／`0xB108EC`=3。
- `FUN_004fcce0`（L196321）＝**起動時 validation**: `if (DAT_00b10270 != 0x1171) print("The script function list doesn't have all the names filled in.\r\n")` ＋ `if (DAT_00b0c8a0 != 0x183) print("The script console-command list doesn't have all the names filled in.\r\n")` ＝ **v433 の sentinel を実行時検証 [確定]**。

### §2.5 callers（v434_9/v434_10）
- `FUN_004FD7C0` E8=8: `0x4FDBC7`(self)／`FUN_004FDE30`／**`FUN_004fef80`×4（`0x4FF015/0x4FF12E/0x4FF26A/0x4FF3C7`）**／`FUN_004ff4c0`×2。
- `FUN_004fef80`（L197796）: `/* WARNING: Function: __alloca_probe replaced with injection: alloca_probe */` ＋ `char local_18f4[512]`/`char local_16e0[512]`＝**512B バッファ多数の script pass [推定高]**。
- `FUN_004ff440`（L197993）: `FUN_00401e40(*(param_1+0x20)); FUN_00401f20(*(param_1+0xc)); *(param_1+0xc)=0; *(u16*)(param_1+0x12)=0; *(u16*)(param_1+0x10)=0;` ＝ **parser state reset/dtor [確定]**。
- `FUN_004fcbd0`（L?・v434_7）＝**whitespace skipper**（`cmp al,9` tab／`cmp al,0x20` space／`cmp al,0x2c` comma を skip）— tokenizer 本体は別 [確定]。

## §3 残点 **V-435-1..3**
1. 【低】**event table の `+0x10` flags 語義**（`0x0`/`0x1`/`0x10000`/`0x10001`/`0x20001` の bit意味＝event の分類（global/object/quest 等）か）＋`+0x14` ptr と `+0x18` handler fn の関係。
2. 【低】**`(a)` flat string array（foot 名＋state 名）の consumer**（`0xB102B8`/`0xB102C8` base の index 系統＝`(b)` stride-0x24 レコードとどう対応するか）と **`0xB108EC` の2段構造**（5 sequence group 名＋16 text key 名が1配列に混在する理由）。
3. 【低】**`FUN_00407AE0` の正式 identity**（`'%s -> %s/%s/%s, Count: %d'` を出す dump fn — どの subsystem の report か）と `0x51A9D0/0x51A9E0/0x51AA00`（name→index helper 3本）の identity。

## §4 検証・自己批評
- probe: **v434_1..v434_29 の29本全 EXIT=0**（V-434-1/V-434-2/V-434-3 完遂）。
- **決定打の作り**: (i) **`cmp reg,0xf` の2件を拾い、両方とも `id-0x10 <= 0xF` 形と判明** → keyword band のみを dispatch し `0x00..0x0F` は default→エラーと**否定側で確定** → (ii) **「id フィールドに imm<0x10 が store されない」全 .text census**（偽陽性6件は mid-instruction と decode で棄却）で**賦流元が table 由来と推論** → (iii) **「stride-0x28 で id run 0,1,2,3 が続く table」を .data/.rdata 全走査**して **event table `0xB0AF48` を一撃発見** → (iv) 全31件実読で id 0x00..0x0F の所属を確定 → (v) consumer（`lea eax,[ebx+0xB0AF48]`）で access 形を確定。
- **rec370+**: (i) **stride 候補（0x10/0x14/0x18/0x20/0x28）を横断比較**して flat（stride4）と stride-0x24 の2系統に分離 → (ii) **`0xB10328`＝`0xB102E0+2×0x24`（rec2 直接 base）で stride 検証** → (iii) **format 文字列実読（`'%s -> %s/%s/%s, Count: %d'`）と4本の base 取得を linear decode で接続**。
- 妥当性チェック: ①event table 終端 `0xB0AF48+31×0x28=0xB0B420`⇔console table 開始 — ②keyword `0x10..0x1F`⇔switch `id-0x10<=0xF`⇔default の console/function range⇔`Unknown function code` — ③stride-0x24 rec2 base `0xB10328`⇔`0xB102E0+0x48` — ④`FUN_004fcce0` の sentinel 検証（`0x1171`/`0x183`）⇔v433 の sentinel 実読 — ⑤`FUN_004fef80` の annotation range⇔4 callers の所属 — **五方向矛盾なし**。
- docs との不整合: **なし**（v433 の残点3件を一括確定）。**副次訂正**: v434_1/v434_2 で stride-0x10 ペア仮説を採ったが、v434_8 の raw dump で **flat stride4 と判明（訂正）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **「どの id band が未所有か」は「switch の range check 連鎖を上から読む」ことで否定側から確定できる**（keyword→console→function→error の順に外れる band は存在しない）。
  2. **「id に imm store が無い」ことは「table 由来」の間接証明になる** — ただし **mid-instruction 偽陽性（`C7 modrm disp32` パターン）を decode で必ず潰す**（本件6件すべて偽陽性）。
  3. **新テーブル探索の決定打＝「stride と id run の組合せ census」**（stride-0x28×`id==index` で .data/.rdata 全走査）— 1本の probe で所有者を一撃特定できる。
  4. **隣接 table の境界は「base + N×stride == 次 table の base」で相互検証**（v433 の sentinel 検証 `FUN_004fcce0` と本件 event→console 隣接が同じ手法）。
  5. **stride 判定は候補横断比較＋直接 base アクセスの2系統で**（flat/0x24 の2系統併存を片方だけ見ると誤構造を書く＝本件 v434_1→v434_8 の訂正事例）。
