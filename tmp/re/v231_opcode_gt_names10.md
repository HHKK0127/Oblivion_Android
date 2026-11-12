# v231_opcode_gt_names10.md — opcode/GT 殘点 第10束（V-227-1..7）

- 作成日: 2026-11-01 / 担当: TODO serial 224 (V224・中) spawning / 分析: サブエージェント
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ: `tmp/re/v227_opcode_gt_names9.md` §10 新規残点 **V-227-1..7**
- 契約: 分析専任（docs/GT/TODO・cache・ソース編集禁止・既存 tmp/re 上書き禁止）。成果物は本ファイルのみ（新規）
- 手法: capstone 5.0.7（v172_common.py 経由・セクション個別変換）＋ MSVC RTTI デコード（v227_lib.py）
- selftest: 着手時 `probe_harness.py selftest` → **4/4 PASS**

## 1. 【V-227-1】TESFurniture +0x58 bit30/31 の意味と level/feet 裁定 — **[解明]**

### 1.1 +0x58 = 'MNAM' サブレコード 4 バイト（ReadRecord/Write/Clear/Copy の四経路確定）

- **ReadRecord = `FUN_004AE700`**（TESFurniture vtable 0xA41D1C の vt[0x1C]＝chunk ハンドラ帯。COL 0xABA4C0 pTD 0xB08088 `.?AVTESFurniture@@` 再確認）。chunk 分岐で **'MNAM'(0x4D414E4D)** → `lea ecx,[esi+0x58]; push ecx; mov ecx,ebx; call 0x4510e0`（ dword 読出＝+0x58 へ）@0x4AE7D6。他の chunk: EDID→vt[0xD8]、MODL/MODB/MODT→+0x30（`0x46DB00`）、FULL→+0x24、SCRI→+0x4C(+0x48 リスト)。
- **書出 = `FUN_004AE540`（vt[0x24]）**: `push 4; lea eax,[esi+0x58]; push eax; push 0x4d414e4d; call 0x46ae40`（'MNAM' 4B を +0x58 から書く）。
- **クリア = `FUN_004AE530`（vt[0x14]）**: `mov dword [ecx+0x58], 0; jmp 0x4b3c90`。
- **複写 = `FUN_004AEC50`（vt[0x2D] / rdata 0xA41DD0）**: RTTI dynamic_cast（pTD 0xB08088=TESFurniture）→ `0x46ACB0`(基底 CopyFrom) → `[esi+0x58]=[edi+0x58]` → `call FUN_004AE830`(検証)。
- ⇒ **v227「FURN DATA flags」呼称は不正確**（FURN に 'DATA' chunk は存在しない）。正しくは **'MNAM'（FurnitureMarker ビットマスク＋sit/sleep 指定）**。

### 1.2 bit30/31 の語義 — CS 検証メッセージで確定（CEF とは無関係）

`FUN_004AE830`（コピー後検証・文字列実取得）:
- flags==0 かつマーカー配列あり → **"You have not selected any FurnitureMarkers for '%s'."**（0xA41E74）
- flags==0 かつマーカー無し → **"No FurnitureMarkers found in '%s'."**（0xA41EAC）
- **flags!=0 かつ `(flags & 0xC0000000)==0` → "Furniture '%s' is not marked for sitting or sleeping."**（0xA41E3C）
- `flags & 0xC0000000 != 0` → 無言で合格（early return @0x4AE899→0x4AE8B8）

⇒ **bit30(0x40000000) = 「sitting 用に指定」/ bit31(0x80000000) = 「sleeping 用に指定」**。ビット0..19 は FurnitureMarker スロットの選択マスク（`FUN_004AE5B0(index)` = `test [this+0x58], 1<<index`（index<0x20）、callers: `FUN_004D73F0`（ref の EDL +0x44 から extra type 0x17 を引き、マーカー index を走査）/ `FUN_004DBAE0` / `FUN_005821F0`）。

### 1.3 level/feet 解釈の裁定（V-221-1 の混同の切り分け）

- 述語 `FUN_004AE590`（bit30）/ `FUN_004AE5A0`（bit31）は **TESFurniture バンドの非仮想ヘルパ**（vtable slot 隣接: vt[0x14]=0x4AE530 / vt[0x24]=0x4AE540）。callers 全 4+1 件: `FUN_004DBAE0`（formType 0x20 ゲート済）/ `FUN_004DE980`（case 0x20 ゲート済）/ `FUN_006505D0`・`FUN_00678750`（**GetBaseForm() 経由・formType ゲート無し**）。
- **同一ヘルパが二つの flags dword を読む**: TESFurniture の +0x58（MNAM＝sit/sleep）と、effect 帯の +0x58（EffectSetting mgefFlags＝CEF kMgefFlag_MagnitudeIsLevel/Feet）。**bit 位置は同じだがオブジェクトが違う** ⇒ v221 の「FURN bit30/31 = kMgefFlag_*」は**誤り（オブジェクトの取り違え）**。TESFurniture 側の bit30/31 に level/feet 意味は**無い**（CEF 対応は EffectSetting+0x58 が正）。
- 副次効果: `FUN_006505D0`/`FUN_00678750`（ゲート無し callers）は候補 ref の GetBaseForm() に対して述語を呼ぶため、**基底が TESFurniture のときだけ sit/sleep 意味**になり、それ以外の基底クラスでは [+0x58] の中身次第（クラス依存・未裁定の残点とする）。
- `FUN_004DBAE0` の marker ループ: bit[i] 必須 → marker type byte(+0xE) を `FUN_004AE5D0`（type<10）/ `FUN_004AE5E0`（10<=type<=19）で分類し、引数 [ebp+0xc]（bit30 許容＝type10..18 系）/[ebp+0x10]（bit31 許容＝type0..9 系）で受入ゲート → **arg1 ↔ bit30 ↔ marker type 10..18、arg2 ↔ bit31 ↔ marker type 0..9** の対応を実測。

### 1.4 残点（本項目内）

- marker type byte の 0..9 / 10..19 の正式な区切り名（CS の FurnitureMarker 種別）は EXE 内文字列未取得（未解明・低）。
- `FUN_006505D0`/`FUN_00678750` の非家具基底に対する [+0x58] 読みの実運用意義（候補が家具限定かどうか）は item3 と連続（→ §3）。

## 2. 【V-227-2】FUN_004DE980 所属メニュー — **[大幅進展]**

### 2.1 呼び出し系統の確定（0E8 全件）

`FUN_004DE980` の E8 呼び出し元 5 件の所属を実測:

| caller | 所属 | 証拠 |
|---|---|---|
| **`FUN_005A4980`** | **フォーム表示ラベル生成**（本体。`FUN_004DE980` を category 0..11 として消費） | 文字列実取得: `'%s %s'`(0xA32564) / `"%s's %s"`(0xA6BE8C) / `'%s (%d)'`(0xA6BE84) を tile へ書出（`FUN_0058CED0`/`FUN_0058CEB0`、tile ID 0xFDE / 0xFA1、`[esi+0x28]`/`[esi+0x48]` 読取） |
| **`FUN_005A5540`** | **HUDInfoMenu vtable slot 0x2C** | vtable 0xAC22A8（COL）→ pTD `.?AVHUDInfoMenu@@` |
| **`FUN_00671620`** | **PlayerCharacter vtable slot 0x228** | vtable 0xAC3D80（COL）→ pTD `.?AVPlayerCharacter@@` |
| `FUN_005ADCF0` | **ロードメニュー** | `'Data\Menus\loading_menu.xml'` 参照（呼び出し元 `FUN_00587550` に `"ind cell (%i, %i) in world '%s' (%08X)."`） |
| `FUN_005806D0` / `FUN_00583E60` | タイル UI 帯（`FUN_005A4980` の callers） | 下記 2.3 |

### 2.2 グローバル 0xB3B33C = ラベル生成コントローラ・シングルトン

- imm32 参照は **7 件のみ**・全て 0x5A4660..0x5A4A4D バンド内: `0x5A4672`(FUN_005A4660) / `0x5A47B1`(FUN_005A47B0) / `0x5A47E1`(FUN_005A47E0) / `0x5A481B`(FUN_005A4810) / `0x5A4842`(FUN_005A4840) / `0x5A49AB`・`0x5A4A4D`(FUN_005A4980)。
- `FUN_005A4980` 冒頭: `cmp dword [0xB3B33C], 0; je 0x5a54f7`（＝未初期化なら即 return）。中盤で `+0x24` を 1/4/8 と比較、`+0x54` を form として使用。**⇒ ラベル生成の有効化/モード制御シングルトン**。
- `FUN_005A4980` 内で category 3/5 のときに `FUN_004DBAE0`（§1.3 の FurnitureMarker ループ）を呼ぶ＝**カテゴリ 3/5 が家具系**（v221 §2 継承を再確認）。

### 2.3 タイル UI 帯（0xFA1 / 0xFDE）

- `FUN_00583E60`: `[0xB33398]+0x20`（UI オブジェクト）取得 → `[esi+8]==3 && [esi+9]==0` のとき `[esi+8]=5`、`and word [eax+0x18], 0xFFFE`、`push 0xFA1`（tile ID）＋float `0xA379B4` ＋文字列 `0xA69194` で `FUN_0058CEB0` 呼び出し → **状態機械（3→5）で tile 0xFA1 を更新**。
- `FUN_005806D0`: スタック 0xC0 の大関数。`push 0x3ED(=1005); call 0x589B70`（失敗時 `call 0x5A4840`＝0xB3B33C 参照関数）、グローバル `0xB3BB0C` を読む。
- `FUN_005A8BC0`: `[0xB3B350]` ヌルチェック → tile `0xFB2` を float 1.0 と定数 `0xA379B4` で `FUN_0058CEB0` により2回更新。

### 2.4 死コードの再確認

`FUN_005A8C90`: **E8 呼び出し 0 件 + imm32 参照 0 件**（再走査）＝到達不能。

### 2.5 残点

- enum 0..11 の**各値の語義**（0/1/2=?, 3/5=家具系, …）は未特定。
- `FUN_004DE980` の `[base+0x64]` 述語（formType 0x15 ケース）の実体は未解明（v227 継承）。
- 「どの HUD メニューか」は HUDInfoMenu（vt 0x2C）／loading_menu / PlayerCharacter（vt 0x228）の 3 系統と特定済みだが、HUDInfoMenu が表示する具体的 UI（クロスヘア情報等）は未確定。

## 3. 【V-227-3】`[x+0x58]` オブジェクトクラスと vt[0x370] 型引数 — **[解明（主要部）]**

### 3.1 `[x+0x58]` = **LowProcess\***（再確認）

- Actor ctor `FUN_005E1880` が `new(0x90)` + ctor `0x6438F0` の結果を `[esi+0x58]` へ格納。RTTI `.?AVLowProcess@@`、**vtable 0xA720A4**（extent 0x56C = 347 エントリ）。

### 3.2 vt[0x370] の実装 — LowProcess=スタブ / HighProcess=実装

- **LowProcess vtable 0xA720A4 の vt[0x370] = `FUN_005E1850`** = **`ret 0x10` スタブ**（4 引数・no-op）→ サブクラスがオーバーライド。
- **HighProcess vtable = `0xA71814`**（RTTI: COL `0xAC39B4` → pTD `0xB05924` `.?AVHighProcess@@`）の **vt[0x370] = `FUN_00654D00`**（実装）。
  - 近接スロット: vt[0x36C]=0x64B090 / vt[0x374]=0x654EA0 / vt[0x378]=0x629630 / vt[0x384]=0x64B0B0。
  - LowProcess 側は vt[0x36C]/[0x374]/[0x380]=0x6F7070（空）/ vt[0x384]=0x6FE080。
- `.?AVMidProcess@@` の RTTI 文字列は **EXE 内に存在しない**（`HighProcess`/`LowProcess` のみ検出）。

### 3.3 `FUN_00654D00`（HighProcess vt[0x370]）の型引数分岐 — 逐語実測

```
arg1 = [esp+0xC] = ebp（対象） / arg2 = [esp+0x10] = type
  ebp==0 → 0x654DB6 へ
  edi = ebp->vt[0x154]()（非0必須）
  ebp == [0xB333C4]（Player） → 0x654DB6 へ
  al = [this+0x11D];  bl_flag = (al==9 || al==4)      ; [esp+0x14] へ
  type = [esp+0x18];  bl = (type==9 || type==4)
  eax = this->vt[+8]()
  （eax==0 && bl_flag==0 && bl==0） → 0x88CE30(edi, 0, 1, 0)
  （eax!=0 || (bl && bl_flag)）→ 0x654DB6
  （eax==0 && bl && bl_flag && ebp->vt[0x380]()==0）→ 0x88CE30(edi, 1, 1, 0)
  eax = [this+0x17C];  if eax!=0:
      type ∈ [3,5] → [eax+0x98]→[+0x7C] に vt[+0x4C](グローバル 0xB06560/0xB06568) でノード探索 → `or word [x+0x18], 1`
      それ以外     → `and word [x+0x18], 0xFFFE`
```

⇒ **vt[0x370] = 「type コード付きのプロセス通知/適用」**で、
- **bit0 立下がち条件: type ∈ {3,4,5}**（`[this+0x17C]` → `[+0x98]` → `[+0x7C]` ノードの `word+0x18`）
- **type ∈ {4,9} は別ゲート（`[this+0x11D]` と対になって `0x88CE30` を発火）**
- type 1/6/0x7F（`FUN_00678750` の旧観測値）は上記いずれにも該当せず → bit0 クリア側。

### 3.4 残点

- `FUN_00678750` の this クラス（callers `FUN_00444C70`/`0x4455E0`/`0x445A10`/`0x66F420` の系統）は未特定。
- `0x88CE30` の正体（4 引数: (form, ?, ?, ?)）と `word+0x18` bit0 の意味は未解明。
- `[this+0x11D]` / `[this+0x17C]` のフィールド名は未特定。

## 4. 【V-227-4】EffectSetting MGEF DATA フィールド意味論 — **[大幅進展]**

### 4.1 MGEF DATA = 0x40 バイト（生読出生書き）の再確認

- **ReadRecord = `FUN_00416100`**: `call 0x4511C0; cmp eax, 0x0C; je`（レコードタイプ 0x0C=MGEF 照合）→ chunk 分岐。`'DATA'`(0x41544144) 分岐 @0x41617B。
- **WriteRecord = `FUN_00415800`**: `push 0x40; lea eax,[esi+0x58]; push eax; push 'DATA'; call 0x46AE40`。

### 4.2 **NEW**: WriteRecord が **7 フィールドを TESForm* → formID に変換**

`FUN_00415800` は DATA 書出しの**前**に以下を実施し、書出しの**後**で元のポインタへ戻す:

```
+0x80 ← [+0x80]+0xC / 0        (元値は [esp+4] へ退避)
+0x84 ← [+0x84]+0xC / 0        (元値は [esp+8] へ退避)
+0x88 ← [+0x88]+0xC / 0        (元値は [esp+0xC] へ退避)
+0x8C ← [+0x8C]+0xC / 0        (元値は [esp+0x10] へ退避)
+0x78 ← [ebp]+0xC / 0          (ebp = 元の +0x78 値、@0x4158B5)
+0x7C ← [ebx]+0xC / 0          (ebx = 元の +0x7C 値、@0x4158C6)
+0x70 ← [edi]+0xC / 0          (edi = 元の +0x70 値、@0x4158D7)
... 0x46AE40 で 'DATA' 0x40B 書出し ...
+0x88←eax / +0x70←edi / +0x80←ecx / +0x78←ebp / +0x7C←ebx / +0x84←edx / +0x8C←ecx で復元
```

`[ptr+0xC]` は **TESForm の formID オフセット**（+0=vtable, +4=formType, +8=formFlags, +0xC=formID）と整合。

⇒ **ESM DATA のうち次の 7 フィールドは formID、実行時は TESForm\***:
`DATA+0x18(+0x70) / +0x20(+0x78) / +0x24(+0x7C) / +0x28(+0x80) / +0x2C(+0x84) / +0x30(+0x88) / +0x34(+0x8C)`

### 4.3 **NEW**: ReadRecord の DATA 分岐（0x41617B）逐語

```
eax=[ebx+0x60](旧 AV) を [ebp-0xC] へ、edi=[ebx+0x58](旧 flags) を保存
al = byte[+0x58+3] & 1              ; 旧 flags の bit24 (0x01000000) を [ebp-5] へ
memset(+0x58, 0, 0x40)              ; 0x981630
+0x74 ← 1.0 (fld1)                  ; 既定値。ただし直後の生読出で上書きされる
file->Read(+0x58, 0x40)             ; 0x450C20（生読み）
フラグ正規化ループ:
  テーブル 0xB03418 = {u32 mask, u8 used}×N（N=[0xB034C0]）
  テーブル 0xB034C4 = u32 mask×M（M=[0xB034DC]）
  各 mask に対し「新フラグに mask が立っていれば旧へ OR / 無ければ旧から AND NOT」
[+0x58] ← 正規化済み flags
旧 flags の bit24 が立っていた場合 → flags |= 0x01000000 し、旧 AV を [+0x60] へ復元
[+0x58] &= 0xFFDFFFFF               ; bit21 常時クリア
```

⇒ **+0x74（DATA+0x1C, float）の「既定 1.0」は読出で必ず上書きされる**（ctor 既定値は実行時にのみ意味を持つ）。bit24 は「既存 AV(+0x60) を保護」フラグ、bit21 は読出後に必ず落とされる。

### 4.4 フィールド表（訂正版）

| オフセット | DATA 偏移 | 型 | 語義（証拠） |
|---|---|---|---|
| +0x58 | +0x00 | u32 | mgefFlags（bit24=AV保護 / bit21=読出後クリア / テーブル正規化） |
| +0x5C | +0x04 | float | cost 系（登録全 0.0・ctor 0.0） |
| +0x60 | +0x08 | u32 | ActorValue（WABR=55/SHLD=43/BRDN=11/WKFI=61 等、全登録一致） |
| +0x64 | +0x0C | u32 | school 0..5（ctor 既定 6 = 学校なし） |
| +0x68 | +0x10 | u32 | resistAV（-1=none / FIDG=61 / DGxx=64 等） |
| +0x6C | +0x14 | u16 | 関連効果件数（`FUN_00415D40` 格納、+0x9C の配列長としても使用） |
| **+0x70** | **+0x18** | **TESForm\*** | **NEW: formID 変換される（4.2）** |
| **+0x74** | **+0x1C** | **float** | ctor 既定 1.0（倍率系）。読出でファイル値優先（4.3） |
| **+0x78** | **+0x20** | **TESForm\*** | **NEW: formID 変換される（4.2）。ActiveEffect instance+0x34 への複写はポインタ複写と再解釈** |
| **+0x7C** | **+0x24** | **TESForm\*** | **NEW: formID 変換される（4.2）。v221「VFX/new 0x4C」説は要訂正** |
| **+0x80** | **+0x28** | **TESForm\*** | **NEW: formID 変換される（4.2）。2 callers の「非ゼロ必須ゲート」は「関連フォーム有り」判定** |
| +0x84 / +0x88 / +0x8C | +0x2C / +0x30 / +0x34 | TESForm\* | NEW: 同上（formID 変換） |
| +0x90 / +0x94 | +0x38 / +0x3C | float | 既定はグローバル 0xB33704 / 0xB3370C |
| +0x98 | — | code | mgefCode（既定 -1、登録時 fourcc） |

### 4.5 「非ゼロ必須ゲート 2 callers」の実体

- `FUN_005ED5A0` @0x5ED684 と `FUN_005F3E00` @0x5F40C7: いずれも `X = FUN_00419B30(...)` の結果に対し `cmp dword [X+0x80], 0; je 0x…719/0x…176`（不成立で即棄却）→ 成立時 `mov ecx,[X+0x80]; mov eax,[ecx+0xC]` で **formID を取り出し** `owner->vt[0x330](0, formID)` へ渡す。
- ⇒ **+0x80 = 「関連フォーム（ESM では formID）」で、無ければ効果適用を棄却する**（4.2 と整合）。

### 4.6 残点

- 7 formID フィールドの**各々の意味**（何の関連フォームか）は未特定。
- formID → TESForm\* の**解決パスの所在**（ReadRecord は生読みのみ）は未特定。
- v221/v227 の「+0x7C = VFX/シェーダ・new 0x4C/vt0x420」記述は 4.2 と矛盾 → 要訂正（残点）。

## 5. 【V-227-5】ENIT/SPIT 殘フィールドと type enum 1/3 — **[解明（大部分）]**

### 5.1 ENIT アクセサ三点セットの実体（v227「同一 impl」の内訳を確定）

| 役割 | EnchantmentItem | AlchemyItem / IngredientItem | 実装 |
|---|---|---|---|
| ChunkType（スロット +0x24） | `FUN_00419730` | `FUN_00419730` | `mov eax, 0x54494E45; ret` = **'ENIT'** |
| Ptr（スロット +0x28） | **`FUN_00419000`** = `lea eax,[ecx+0x1C]` | **`FUN_00419740`** = `lea eax,[ecx+0x54]` | data 先頭 |
| Size（スロット +0x2C） | **`FUN_008C4830`** = `mov eax, 0x10`（**16B**） | **`FUN_00419750`** = `mov eax, 8`（**8B**） | サイズ |

- 登録先 vtable（rdata 実測）: **EnchantmentItem 副 vtable `0xA33C3C`**（+0x24/+0x28/+0x2C）、AlchemyItem `0xA32134`、IngredientItem `0xA33F14`。
- EnchantmentItem の埋め込み subobject は **+0x18**（WriteRecord `FUN_0041B1B0` の `lea esi,[edi+0x18]`）⇒ **ENIT data = obj+0x18+0x1C = obj+0x34、size 16B**。
- AlchemyItem 埋め込み subobject は **+0x24**（chunk dispatcher `FUN_00419760` の `lea ecx,[esi-0x24]`）⇒ **ENIT data = obj+0x24+0x54 = obj+0x78、size 8B**。
- ⇒ v227 の「16B {type, dword, dword, byte}」／「ALCH 8B @+0x78」は**レイアウトとして正しい**。

### 5.2 ENIT/SPIT の読出・書出経路

- **ReadRecord = `FUN_0041B2D0`**（EnchantmentItem 主 vtable `0xA33C84` vt[0x1C]）: chunk 分岐 `EDID`(0x44494445) / `EFID`(0x44494645) / `FULL`(0x4C4C5546) は専用処理、それ以外は `esi = obj+0x18` の `vt[0x3C]`（=`FUN_0060CF60` = `ret 8`）へ委譲 → その後 **`vt[0x24]()`（tag getter）と現在 chunk ID を比較し、一致なら `memset(vt[0x28](), 0, vt[0x2C]())` + `file->Read(vt[0x28](), vt[0x2C]())`**（0x981630 = memset / 0x450C20 = Read）。
  ⇒ **ENIT 16B が obj+0x34 へ読まれる経路を特定**（v227 の「埋め込み ExtraData」仮説を実証）。
- **WriteRecord = `FUN_0041B1B0`**: `lea esi,[edi+0x18]` → `call 0x46C730` → `vt[0x38]()` → `vt[0x2C]()`(size) → `vt[0x28]()`(ptr) → `vt[0x24]()`(tag) → `call 0x46AE40(tag, ptr, size)`。
- ctor 既定値（実測）:
  - EnchantmentItem `FUN_00419570`: `[esi]=0xA33C84 / [esi+0x18]=0xA33C3C / [esi+0x24]=0xA33C28`、`byte [esi+4]=0x0F`、**`+0x34=2 / +0x38=-1 / +0x3C=-1 / byte +0x40=0`**
  - SpellItem `FUN_0041D420`: `byte [esi+4]=0x10`、**`+0x34=0 / +0x38=-1 / +0x3C=0 / +0x40=0`**
  - AlchemyItem `FUN_00412BE0`: `byte [esi+4]=0x28`、**`+0x78=-1 / +0x7C=0`**

### 5.3 **`FUN_004190C0` = `EnchantmentItem::CanApplyTo(target)`**（type enum の正体）

- 呼び出し元 `FUN_0046A880`（`[this+4]` を EnchantmentItem へ dynamic_cast、失敗時 `"Unable to find enchanting (%08X) on owner object …"`）→ 成立時 `FUN_004190C0(ench, obj)`、不成立時 **`"Enchanting (%08X) '%s' has invalid cast type or magic effect for object (%08X) '%s'."`（0xA3B98B）**。
- 引数 obj を `TESForm→TESObject` dynamic_cast → `byte [obj+4]`（formType）で switch（0x14..0x22）。

**formType ↔ クラス対応を RTTI で確定**（ctor 内の `mov byte [x+4], imm` と primary vtable store が同一関数内にあることを実測）:

| formType | クラス | ctor |
|---|---|---|
| 0x14 (20) | **TESObjectARMO**（vtable 0xA441BC） | `FUN_004B5140` |
| 0x15 (21) | **TESObjectBOOK**（vtable 0xA44454） | `FUN_004B59F0` |
| 0x16 (22) | **TESObjectCLOT**（vtable 0xA44644） | `FUN_004B6150` |
| 0x21 (33) | **TESObjectWEAP**（vtable 0xA45354） | `FUN_004BB610` |
| 0x22 (34) | **TESAmmo**（vtable 0xA4068C） | `FUN_004A9430` |

**`[ench+0x34]`（ENIT dword0）= "cast type"**（エラーメッセージの呼称が決定打）。要求値:

| target | 追加条件 | 要求 cast type |
|---|---|---|
| ARMO (0x14) | `FUN_00415130(effリスト, 0x20000)` が**偽** | **3** |
| BOOK (0x15) | `byte [book+0x88] & 1` が立つこと | **0** |
| CLOT (0x16) | TESBipedModelForm への cast 成功。`FUN_00469050`（述語）真 → 追加条件なし / 偽 → 上記0x20000 リスト述語が偽 | **3** |
| WEAP (0x21) | TESObjectWEAP への cast 成功。`byte [weap+0x90]==4` | **1** |
| WEAP (0x21) | 同上・`[weap+0x90]!=4` | **2** |
| AMMO (0x22) | — | **2** |
| それ以外（0x17..0x20 等） | — | 不成立（return 0） |

- **`FUN_00415130(list@+0x24, mask)`**: 効果リストを走査し、`[(node+0x1C)]+0x58`（＝EffectSetting mgefFlags）に対し **`(mgefFlags & mask) != 0 && (mgefFlags & 0x00400000(bit22)) == 0`** を満たす効果が存在すれば真。
- **`FUN_00469050`**（callers 2 件: `FUN_004190C0` / `FUN_00419350`）: `movzx eax, word [ecx+4]` → formType の bit6/bit7 と byte[+5] の bit0 をテストして真偽を返す。

⇒ **cast type 0..3 の正体 = 付与先カテゴリ**:
- **0 = BOOK 系**（`[+0x88]&1` の条件付き）
- **1 = WEAP うち `[+0x90]==4` のサブタイプ**
- **2 = WEAP（その他）/ AMMO**
- **3 = ARMO / CLOT（biped 服装・防具）**

- **EnchantmentItem 既定 cast type = 2**（＝デフォルトは「武器/矢」向け）、**SpellItem 既定 = 0**。`FUN_004190C0` は EnchantmentItem のみを this とするため、SpellItem の 0 は本関数では検査されない。

### 5.4 残点

- `byte [weap+0x90]` の語義（==4 のサブタイプが何か）。書手候補は `FUN_004BB150`(=0) / `FUN_00473EB0`・`FUN_00476D10`(=0xFF) / `FUN_0089BF50`・`FUN_008CD4E0`(=0/1) だが、値 4 の書手は未発見（＝ファイル DATA 由来の可能性が高い）。
- `byte [book+0x88]` の bit0 の語義。
- ENIT dword1(+0x38, 既定 -1) / dword2(+0x3C, ENCH=-1・SPIT=0) / byte3(+0x40, 既定 0) の意味は**未解明のまま**（比較 `FUN_00419010` は 4 フィールド全比較、複写 `FUN_00419060` は全複写のみ実測）。
- ALCH/INGR ENIT 8B（+0x78 u32 既定 -1 / +0x7C u8 既定 0）の意味は**未解明のまま**（`FUN_00412A00` が dword+byte のみ比較）。

## 6. 【V-227-6】bit23 ホルダクラスの RTTI/ctor — **[解明]**

- **vtable `0xA6EF94` → COL `0xAC3218` → pTD `0xB1497C` = `.?AVAlarmPackage@@`**（実デコード）。
- **ctor = `FUN_006068D0`**: vtable 書き + 基底 ctor `0x568E10` + リスト `+0x3C` 構築。E8 呼び出し元は **`FUN_0060F250` のみ（@0x60F98A）**。
- **`FUN_0060F250`**: `new` + ctor → `FUN_005661E0(this, 0xF)`（type 設定。`byte [esi+0x20]` を読取、type 3..7 は破棄処理）→ `[ebx+0x1C]` の **bit23 / bit20 / bit21** を `[ebp+0x1C]` へ or/and で複写 → 最後に `or dword [ebp+0x1C], 6`（＝**bit20/bit21 を強制立上げ**）。
- ⇒ **bit23(0x00800000) は AlarmPackage の +0x1C フラグ dword 上のビット**（bit20/21 と同居）。`FUN_0060F250` は既存パッケージの +0x1C から bit23 を引き継ぎつつ bit20/21 を立てる。

## 7. 【V-227-7】school グローバル 6 オブジェクトの正体 — **[解明]**

### 7.1 正体 = TESSound\* の「魔法失敗音」6 学校分

- グローバル `0xB33560`..`0xB33574` = **`TESSound*`**（TD `0xB03500` = `.?AVTESSound@@`）。
- 初期化 = **`FUN_0041BB80`**（imm32 参照は 0x41BBA0..0x41C000 バンドに限定）: 各 formID ごとに
  `_RTDynamicCast(FUN_004471A0(formID,…))` → 無ければ `new(0x44)` + ctor `0x517F40` → `FUN_0046C300(obj, formID, 1)` → `vt[0xD8](EDID 文字列)` → `vt[0x90](0)` → グローバル格納 → `[0xB33A98]+0x6C` へ `FUN_00446CB0` でリスト追加。

### 7.2 formID ↔ グローバル ↔ EDID の全対応（push imm32 実測）

| formID | グローバル | EDID 文字列（va） | 学校 |
|---|---|---|---|
| 0x12C | `0xB33560` | `MagicFailureSoundAlteration` (0xA34530) | 0 = Alteration |
| 0x12D | `0xB33564` | `MagicFailureSoundConjuration` (0xA34510) | 1 = Conjuration |
| 0x12E | `0xB33568` | `MagicFailureSoundDestruction` (0xA344F0) | 2 = Destruction |
| 0x12F | `0xB3356C` | `MagicFailureSoundIllusion` (0xA344D4) | 3 = Illusion |
| 0x130 | `0xB33570` | `MagicFailureSoundMysticism` (0xA344B8) | 4 = Mysticism |
| 0x131 | `0xB33574` | `MagicFailureSoundRestoration` (0xA34498) | 5 = Restoration |

- **resolver `FUN_0041B820`**: school 0..5 → `0xB33560`..`0xB33574`（jmp table 0x41B858、>5 → NULL）。参照実測: `0x41B831`(0xB33560) / `0x41B83D`(0xB33568) / `0x41B84F`(0xB33574)。
- ⇒ **学校順（0..5）= Alteration/Conjuration/Destruction/Illusion/Mysticism/Restoration** で v227 §6 の school 割当と完全一致。EDID は 0xA34498..0xA34530 に連続配置（Restoration→Alteration の順）。
- グローバルの参照は resolver と初期化関数のみ（他に参照なし）＝**読み手は `FUN_0041B820` 経由のみ**。

## 8. GT ドラフト（反映は要承認）

- **(T)** GT「formType 欄」行 追加: **formType 0x14=TESObjectARMO（ctor `FUN_004B5140`・vtable 0xA441BC） / 0x15=TESObjectBOOK（`FUN_004B59F0`・0xA44454） / 0x16=TESObjectCLOT（`FUN_004B6150`・0xA44644） / 0x21=TESObjectWEAP（`FUN_004BB610`・0xA45354） / 0x22=TESAmmo（`FUN_004A9430`・0xA4068C）**（各 ctor 内の `mov byte [x+4], imm` と primary vtable store の同一関数内実測）。v221/v227 の formType 表を拡張。
- **(U)** GT「`FUN_004190C0`」行: **`EnchantmentItem::CanApplyTo(target)`**。target formType（0x14 ARMO / 0x15 BOOK / 0x16 CLOT / 0x21 WEAP / 0x22 AMMO）ごとに **ENIT dword0(+0x34)=cast type の要求値 {ARMO→3, BOOK(+0x88&1)→0, CLOT→3, WEAP(+0x90==4)→1, WEAP(その他)→2, AMMO→2}** を検証。不成立時エラー 0xA3B98B `"Enchanting … has invalid cast type or magic effect for object …"`。効果側述語 `FUN_00415130(list@+0x24, mask)` = 「mgefFlags に (mask & f)!=0 && !(f & 0x00400000) を満たす効果の存在」（ARMO/CLOT は mask=0x20000 の**不在**を要求）。biped 述語 = `FUN_00469050`（`word[obj+4]` bit6/bit7 と byte[+5] bit0）。
- **(V)** GT「ENIT/SPIT」行 訂正: **ENIT アクセサ三点セット = ChunkType `FUN_00419730`(='ENIT') / Ptr Ench=`FUN_00419000`(`lea [ecx+0x1C]`)・ALCH/INGR=`FUN_00419740`(`lea [ecx+0x54]`) / Size Ench=`FUN_008C4830`(0x10=16B)・ALCH/INGR=`FUN_00419750`(8B)**。埋め込み subobject は Ench=+0x18 / ALCH=+0x24 ⇒ **ENIT 16B @ Ench+0x34、8B @ ALCH+0x78**。読出は ReadRecord `FUN_0041B2D0` の tag 比較→memset→`file->Read`、書出は `FUN_0041B1B0` の size/ptr/tag 取得→`0x46AE40`。v227 (P) の「16B {type, dword, dword, byte}」は維持し、**type=cast type（(U)）** で確定。
- **(W)** GT「EffectSetting フィールド」行 追加: **MGEF DATA 0x40B @+0x58..+0x97 のうち 7 フィールド（+0x70/+0x78/+0x7C/+0x80/+0x84/+0x88/+0x8C = DATA+0x18/0x20/0x24/0x28/0x2C/0x30/0x34）は TESForm\*（ESM では formID）**。書出 `FUN_00415800` が全7件を `[p]+0xC` へ変換→書出→復元。読出 `FUN_00416100` は「旧 flags bit24 と旧 AV(+0x60) を保存 → memset → +0x74=1.0 既定 → 生読出 → テーブル 0xB03418(count 0xB034C0)/0xB034C4(count 0xB034DC) によるフラグ正規化 → bit24/AV 復元 → **常時 `flags &= 0xFFDFFFFF`（bit21 クリア）**」。v221「+0x7C=VFX/new 0x4C」は**訂正候補**。
- **(X)** GT「`FUN_00678750` vt[0x370]」行: **vt[0x370] の実装は LowProcess=`FUN_005E1850`(`ret 0x10` スタブ) / HighProcess(vtable 0xA71814)=`FUN_00654D00`（実装）**。`FUN_00654D00` の type 引数: **type∈{3,4,5} → `[this+0x17C]→[+0x98]→[+0x7C]` ノードの `word+0x18` bit0 を立上げ**、それ以外は bit0 クリア。**type∈{4,9} は `[this+0x11D]`∈{4,9} と対で `0x88CE30(form, …)` を発火**。`.?AVMidProcess@@` は EXE 内に不在。
- **(Y)** GT「bit23 伝播」行 補強: ホルダクラス = **`.?AVAlarmPackage@@`（vtable 0xA6EF94 / COL 0xAC3218 / pTD 0xB1497C）**、ctor `FUN_006068D0`（base ctor 0x568E10・リスト +0x3C）、生成は `FUN_0060F250` のみ。bit23 は AlarmPackage **+0x1C** 上のビット（bit20/21 と同居、末尾で `or +0x1C, 6`）。
- **(Z)** GT「school グローバル」行 訂正: **`0xB33560..0xB33574` = TESSound\* 6 本（EDID `MagicFailureSound{Alteration,Conjuration,Destruction,Illusion,Mysticism,Restoration}`、formID 0x12C..0x131）**。初期化 `FUN_0041BB80`（new(0x44)+ctor 0x517F40→`FUN_0046C300`→vt[0xD8] EDID→vt[0x90]→`[0xB33A98]+0x6C` 追加）、読手は resolver `FUN_0041B820`（school 0..5→0xB33560..74、>5→NULL）。**学校順 0..5 = Alteration/Conjuration/Destruction/Illusion/Mysticism/Restoration**（v227 §6 の school 割当と一致）。
- **(AA)** GT「`FUN_005A4980`」行: **フォーム表示ラベル生成**（`'%s %s'`/`"%s's %s"`/`'%s (%d)'` を tile 0xFDE/0xFA1 へ、グローバル `0xB3B33C` = ラベル生成シングルトン、category 3/5 で `FUN_004DBAE0`）。`FUN_004DE980` の callers 所属 = HUDInfoMenu(vt 0x2C) / PlayerCharacter(vt 0x228) / loading_menu / タイル UI（`FUN_005806D0`・`FUN_00583E60`）。`FUN_005A8C90` は E8/imm32 とも 0 件＝死コード。

## 9. 新規残点（次タスク候補）

| ID | 内容 | 優先 |
|---|---|---|
| V-231-1 | MGEF DATA の 7 formID フィールド（+0x70/+0x78/+0x7C/+0x80/+0x84/+0x88/+0x8C）の各々の意味と、ReadRecord 生読み後の **formID→TESForm\* 解決パスの所在** | 中 |
| V-231-2 | ENIT cast type 0..3（(U)）の **ESM 由来かランタイム設定か**、および付与時に type を決めるコード（`[weap+0x90]==4` / `[book+0x88]&1` の語義を含む） | 中 |
| V-231-3 | ENIT dword1(+0x38, 既定 -1) / dword2(+0x3C, ENCH=-1・SPIT=0) / byte3(+0x40) と ALCH/INGR ENIT {u32=-1, u8=0} の意味（v227 殘のまま） | 低 |
| V-231-4 | `FUN_004DE980` の enum 0..11 の各値の語義と `[base+0x64]` 述語（formType 0x15 ケース）の実体 | 中 |
| V-231-5 | `FUN_00678750` の this クラス（callers `FUN_00444C70`/`0x4455E0`/`0x445A10`/`0x66F420` の系統）と `[x+0x58]` 以外の slot 8/0x17C/0x380/0x384 | 中 |
| V-231-6 | `FUN_00654D00` の arg2 type コードの全意味（3/4/5 立上げと 4/9 発火の二重条件）、`0x88CE30` の正体、`word+0x18` bit0 / `[this+0x11D]` / `[this+0x17C]` の語義 | 中 |
| V-231-7 | MGEF flags bit24（旧 AV 保護）と bit21（読出後常時クリア）、テーブル 0xB03418/0xB034C4 の正体（フラグ正規化の目的） | 低 |
| V-231-8 | v221/v227「+0x7C = VFX/シェーダ・new 0x4C/vt0x420」記述の再裁定（(W) の formID 変換と矛盾） | 低 |

## 10. 自己検証・証跡

- **selftest**: 着手時・完成直前とも `probe_harness.py selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- **7 件の判定**: V-227-1 [解明] / V-227-2 [大幅進展] / V-227-3 [解明（主要部）] / V-227-4 [大幅進展] / V-227-5 [解明（大部分）] / V-227-6 [解明] / V-227-7 [解明]。
- **証跡（すべて `%TEMP%\opencode/`・本タスク生成・`v231_*_out.txt` 付き）**:
  - `v231_20.py` ENIT/SPIT tag 呼び出し元の int3 パディング越え走査 / `v231_21.py` `push 'ENIT'/'SPIT'` 走査（0 件＝tag は `mov eax,imm` 形） / `v231_22.py` ENIT/SPIT fourcc imm32 参照（0x419731 / 0x41D251）
  - `v231_23.py` `FUN_004190C0` 全文＋callers / `v231_24.py` ジャンプテーブル（0x4191D4/0x4191EC）＋`FUN_0046A880` 全文
  - `v231_25.py` エラーメッセージ4種＋ENIT アクセサ callers＋`FUN_00415130`/`FUN_00469050` / `v231_26.py` vtable slotから ENIT アクセサの owner class 特定 / `v231_27.py` 副 vtable 0xA33C3C・0xA32134 全スロット＋ENCH ctor
  - `v231_28.py` ENIT ptr/size アクセサ実体＋`FUN_00415130` 全文＋ReadRecord `FUN_0041B2D0` 全文 / `v231_29.py` `0x981630`(memset)＋WriteRecord `FUN_0041B1B0`＋type-form 工場 `FUN_00447DF0`
  - `v231_30.py` chunk dispatcher `FUN_00419760`＋slot0x3C / `v231_31.py` TD 名称検証（6 件）＋biped 述語 callers＋type-form 工場冒頭 / `v231_32.py` `mov byte [x+4],imm` 全走査＋'cast type' 文字列
  - `v231_33.py` formType 0x14/0x15/0x16/0x21/0x22 の ctor RTTI / `v231_34.py` EDID 文字列帯＋item4 readers＋`0xB3B33C` 参照＋`FUN_00419350`
  - `v231_35.py` `FUN_006904B0`/`FUN_005ED5A0`/`FUN_005F3E00` の +0x80 ゲート文脈 / `v231_36.py` `FUN_00419B30`＋item7 グローバル参照＋MidProcess RTTI 走査
  - `v231_37.py` `mov byte [x+0x90],imm` 全走査＋HighProcess RTTI→vtable / `v231_38.py` HighProcess/LowProcess vt[0x370]＋school sound ブロック / `v231_39.py` `FUN_00654D00` 全文
  - `v231_40.py` `FUN_0041BB80` の EDID/formID push 全列 / `v231_41.py` `mov [reg+0x78/0x7C/0x80],reg` マジック帯走査 / `v231_42.py` `FUN_005806D0`/`FUN_00583E60`/`FUN_005A4980` 冒頭
  - `v231_43.py` `FUN_00416870`＋WriteRecord ポインタ変換帯 / `v231_44.py` WriteRecord 冒頭（7 フィールド変換）＋ctor / `v231_45.py` ReadRecord `FUN_00416100` 全文
- **訂正まとめ**: (1) v227「+0x7C = VFX/new 0x4C」→ formID 変換されることと矛盾（V-231-8）。(2) v227「ENIT type enum 1/3 未解明」→ 付与先カテゴリとして確定（(U)）。(3) v227「school グローバルの正体 未解明」→ TESSound\* の MagicFailureSound 6 学校分として確定（(Z)）。(4) v227「bit23 ホルダクラス名未取得」→ AlarmPackage として確定（(Y)）。(5) v221「vt[0x370] 型引数の意味」→ HighProcess 実装 `FUN_00654D00` の分岐として部分確定（(X)）。
