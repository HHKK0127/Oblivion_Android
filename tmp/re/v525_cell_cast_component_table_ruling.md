# v525 — `PTR_PTR_00b03038`＝`TESObjectCELL`（TD 実読）・`FUN_00446CB0`＝list add-node（v524 訂正）・`FUN_00466Exx`＝TESForm component view table family の裁定（V-525-1..3 完遂・旧残点 V-406-3 継続第55弾）

- 採番: **v525**（旧残点群消化・probe prefix **v525_1..7 全7本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-525-1..3（v524 残点）** — 1=`PTR_PTR_00b03038` RTTI type（`DeleteForm` cast 先）＋`FUN_00446CB0` identity＋`FUN_00466E60`/`FUN_00466E80`/`FUN_00466DD0` family identity／2=`param_1+0x18` owner object と caller 分離／3=引継ぎ残り。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE データ・decompiled.c・EXE 内文字列のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-525-1: `PTR_PTR_00b03038` RTTI（`DeleteForm` の cast 先）** | **[確定]（TD 実読）→ `.?AVTESObjectCELL@@`** | **TypeDescriptor オブジェクトの実読で確定 [確定(実RTTI)]: `0xB03038`（TD オブジェクト）の dword 列＝`80 3e aa 00｜00 00 00 00｜2e 3f 41 56 54 45 53 4f 62 6a 65 63 74 43 45 4c 4c 40 40`＝vftable `0x00AA3E80`＋spare `0`＋inline name `.?AVTESObjectCELL@@` → `DeleteForm` の cast 先 type＝`TESObjectCELL` [確定]**。同法で **`0xB02F9C`＝`.?AVTESForm@@`（cast 元）／`0xB03534`＝`.?AVEffectSetting@@`（v523 を再確認）**。**TD オブジェクト layout（`+0`＝`type_info` vtable `0x00AA3E80`、`+4`＝0、`+8`＝inline mangled name）を確定 [確定] → v525_1 で `+0` を TD と誤読した箇所を本裁定で訂正**。 |
| **V-525-1: `FUN_00446CB0` identity** | **[確定相当]→ 単方向リスト add-node（v524「delete/free 候補」を訂正）** | **L54233-54254 実読 [確定(decompiled)]: `void __thiscall FUN_00446cb0(int *param_1, int param_2)`: `if (param_2 != 0) { if (*param_1 != 0) { piVar1 = FUN_00401f00(8)`（**8B node alloc**）`→ *piVar1 = *param_1; piVar1[1] = param_1[1]; *param_1 = param_2; param_1[1] = (int)piVar1; return; } ... *param_1 = param_2; }` → **head 挿入（node `{value, next}` を 8B alloc して先頭に連結）[確定相当]** → **v524 の「delete/free 候補 [推定]」は [確定負] 相当で訂正（free ではなく add）**。caller 傍証 [確定]: `FUN_004040e0` が重複 key 検査 `FUN_004a7a60("Setting key '%s' already used in list.…")` の後に `FUN_00446cb0(param_2)`＝**settings list への追加**、`FUN_00416650` も要素追加で呼ぶ。 |
| **V-525-1: `FUN_00466E80` = TESForm component view table 構築** | **[確定]（26 slot の cast 先 TD を全実読）** | L80808-80922 実読 [確定(decompiled)＋実RTTI]: `_memset(param_1, 0, 0x68)`（**26 dword＝104B**）→ `do { switch(iVar2) { case 0:` `*param_1 = param_2;`（TESForm 生）`case 1..25:` `FUN_009832e6(param_2, 0, &PTR_PTR_00b02f9c, &PTR_PTR_00b0XXXX, 0)` → `param_1[i] = 結果`。**TD 実読で 25 種の cast 先を列挙 [確定]**: `TESAIForm`／`TESAnimation`／`TESActorBaseData`／`TESAttackDamageForm`／`TESAttributes`／`TESBipedModelForm`／`TESContainer`／`TESDescription`／`TESEnchantableForm`／`TESFullName`／`TESHealthForm`／`TESTexture`／`TESLeveledList`／`TESModel`／`TESProduceForm`／`TESQualityForm`／`TESRaceForm`／`TESScriptableForm`／`TESSpellList`／`TESUsesForm`／`TESValueForm`／`TESWeightForm`／`TESSoundFile`／`TESReactionForm`／`TESModelList` → **`FUN_00466E80`＝1つの `TESForm` から 26 種の component interface view を一括取得して表に格納する fn [確定]**（slot 0 のみ生 form）。 |
| **V-525-1: `FUN_00466E60` / `FUN_00466DD0` / `FUN_00466E10` / `FUN_0046AD70`** | **[確定] / [確定相当]** | **`FUN_00466E60`＝`void* __fastcall(void* p) { _memset(p,0,0x68); return p; }`＝component 表ゼロ初期化 [確定]**。**`FUN_00466DD0`＝`void __thiscall(int *a, int b)`: `b -= (int)a; iVar2 = 0x1a; do { if ((*a != 0) && (iVar1 = *(int*)(b + (int)a), iVar1 != 0)) { (**(code**)(*(int*)*a + 8))(iVar1); } a++; } while(--iVar2);`＝2表の同 slot 対応で `vtable[+8]` を呼ぶ 26 回ループ [確定相当]**（`+8`＝表側 interface の処理 method、semantic [推定]）。**`FUN_00466E10`＝component 表の等値比較 [確定]`: `for i in 0..0x19 { if (a[i] == 0) b = (b[i] == 0) else b = (vtable[+0xC](b[i]) == 0); if (!b) return 1; } return 0;`（**0＝一致／1＝不一致**）。**`FUN_0046AD70`＝「2 form の component 表比較」predicate [確定]**: `if (param_2 == 0) return 1;` → 2表 zero→fill(param_1)／fill(param_2) → `uVar1 = FUN_00466e10(local_dc)` → 2表破棄 → return。 |
| **V-525-1: `FUN_0046ACB0`（v524 部分確定の引き上げ）** | **[部分確定]→[確定相当]（2表操作 handler）** | **L84420-84439 実読 [確定(decompiled)]: SEH → `FUN_00466e60(uVar1)`（表A zero）→ `FUN_00466e60()`（表B zero）→ `FUN_00466e80(param_1)`（表A fill）→ `FUN_00466e80(param_2)`（表B fill）→ `FUN_00466dd0(local_dc)`（2表 slot 対応処理）→ `FUN_0060d0a0()`×2 → SEH 復元**。`local_dc[208]`＝**0x68×2＝208B＝component 表2枚**（v524 の「208B バッファ」の正体が判明）→ **`FUN_0046ACB0`＝2 form の component 表を構築して slot 対応処理を行う handler [確定相当]**（`FUN_0046AD70` と同型で、比較返却の有無だけが違う [確定]）。**v524 の「serialize/copy 候補 [推定]」は component 表操作へ引き上げ**。 |
| **V-525-1: `FUN_0046B090` / `FUN_0046B170`（family 残り）** | **[確定相当]（form type gate 付き component 処理）** | 両 fn とも同一形 [確定(decompiled)]: SEH＋`DAT_00b30aac ^ &stack` → `switch(*(undefined1*)(param_1 + 4))`（**`TESForm+4`＝form type byte** [確定相当]）で `case 0xd / 0x31 / 0x32 / 0x33 / 0x34 / 0x36 / 0x3d`＝skip、`case 0x30`＝専用処理（`FUN_005a6a20` / `FUN_0060d0a0`）、`default`＝表 zero→`FUN_00466e80(param_1)`→`FUN_00466d80()`（`FUN_0046B090`）／`FUN_00466da0()`（`FUN_0046B170`）→破棄 → **`FUN_0046B090`/`FUN_0046B170`＝form type（`+4`）で分岐する component 表処理ハンドラ（`+8` 相当の実体は `FUN_00466D80`/`FUN_00466DA0`）[確定相当]**。 |
| **V-525-2: `param_1+0x18` owner object identity** | **[部分確定]** | 根拠: `FUN_0045C7A0` が `param_2[3]`（`+0xC`＝refID 相当・v524）を渡し、`param_2[2] >> 0xe`（`+8`＝flags bit14）を gate に使い、`+4` が form type byte（本裁定 family）であることから **`param_2`＝`TESForm*`** を [確定相当] へ引き上げ。`param_1`（`+0x18` に thread 依存 flag、`+0x30` に form list）は **TESDataHandler 系 singleton [推定]**、20 の E8 caller（`FUN_00465860` 経由 4 件＝save/load 系、`FUN_0046xxx` 系 3 件等）を列挙したが identity 確定は残点。 |
| **V-525-3: 引継ぎ残り** | **[未確定]** | predicate slot35 英名・新 FourCC 意味（残点継続） |

## §2 証拠

### §2.1 TypeDescriptor オブジェクト実読（v525_3 / v525_4）
```
0xB02F9C: 80 3e aa 00 | 00 00 00 00 | ".?AVTESForm@@"
0xB03038: 80 3e aa 00 | 00 00 00 00 | ".?AVTESObjectCELL@@"   <-- DeleteForm cast destination
0xB03534: 80 3e aa 00 | 00 00 00 00 | ".?AVEffectSetting@@"    <-- v523 re-confirmed
layout: +0 vfptr (= type_info vtable 0x00AA3E80), +4 spare (=0), +8 inline mangled name
(0xAA3E80 itself is the type_info vtable: 0x981E65 / 0x982897 / 0x98D90E + "Access violation - no RTTI data!")
=> rtti_name() on +0 is WRONG; read the descriptor object at the .data address (correction applied here)
```

### §2.2 `FUN_00466E80` = 26-slot component view table（v525_5）
```
case 0 : *param_1 = param_2                       // raw TESForm
case 1 : TESAIForm          case 2 : TESAnimation        case 3 : TESActorBaseData
case 4 : TESAttackDamageForm case 5: TESAttributes       case 6 : TESBipedModelForm
case 7 : TESContainer       case 8 : TESDescription      case 9 : TESEnchantableForm
case 10: TESFullName        TESHealthForm   TESTexture   TESLeveledList  TESModel
         TESProduceForm     TESQualityForm  TESRaceForm  TESScriptableForm
         TESSpellList       TESUsesForm     TESValueForm TESWeightForm
         TESSoundFile       TESReactionForm TESModelList
=> 25 typed views + slot 0 raw form = 26 dwords = 0x68
```

### §2.3 `FUN_00446CB0` = list add-node（v525_2）
```
L54238: if (param_2 != 0) {
L54239:   if (*param_1 != 0) {
L54240:     piVar1 = FUN_00401f00(8);        // 8-byte node
L54242:     *piVar1 = *param_1;              // node.value = old head
L54244:     piVar1[1] = param_1[1];          // node.next
L54245:     *param_1 = param_2;              // head = new value
L54246:     param_1[1] = (int)piVar1;        // next = node
L54252:   *param_1 = param_2;
caller FUN_004040e0: duplicate-key check -> "Setting key '%s' already used in list." -> FUN_00446cb0(param_2)
=> add-node, NOT delete/free  (v524 [推定] corrected to [確定負])
```

### §2.4 DeleteForm branch re-read（v525_6）
```
L73065: if ((bit14 == 0) && (cast to TESObjectCELL == 0)) {
L73066:   FUN_0046c300(0, 1);                  // id := 0
L73067:   FUN_0046b2c0();                      // reset
L73068:   virtual[*param_2 + 0xd8](&DAT_00a2f7ec);
L73069:   puVar1 = param_1 + 0x30;             // list walk
L73072:   FUN_00446cb0(param_2);               // NOT in list -> ADD to list
L73079: } else {
L73081:   FUN_0046c300(FUN_00448d40(1), 1);    // fresh id -> replace
=> non-cell, non-bit14 form: id cleared + registered in list;  cell (or bit14): re-identified
```

## §3 判定の内訳

- **V-525-1 完遂**: ①cast 先 TD＝`TESObjectCELL` [確定] ②`FUN_00446CB0`＝list add-node [確定相当]（v524 訂正）③`FUN_00466E80`＝26-slot component view 表 [確定] ④`FUN_00466E60`/`FUN_00466DD0`/`FUN_00466E10`/`FUN_0046AD70`/`FUN_0046B090`/`FUN_0046B170` identity [確定〜確定相当] ⑤`FUN_0046ACB0`＝2表操作 [確定相当]。
- **V-525-2 部分確定**: `param_2`＝`TESForm*` [確定相当]、`param_1`＝TESDataHandler 系 singleton [推定]・caller 20 件列挙（identity 残点）。
- **V-525-3 残点継続**: predicate slot35 英名・新 FourCC 意味。

## §4 残点（V-526-1..3）

1. `FUN_00466D80`/`FUN_00466DA0`（`FUN_0046B090`/`FUN_0046B170` の `+8` 相当実体）identity＋`FUN_0046C2E0`/`FUN_0046C2F0` thunk の接続【中】
2. `param_1`（`DeleteForm` の this）identity（TESDataHandler 候補）＋caller 20 件の経路分離（save/load 系 4 件の `FUN_00465860` が手掛かり）【低】
3. 引継ぎ残り（predicate slot35 英名・新 FourCC 意味）【低】

## §5 後続への申し送り

- **TypeDescriptor 読み方の確定**: `.data` 上の TD オブジェクトは `+8` に inline mangled name。`+0` は `type_info` vtable（`0x00AA3E80`）。**v521 の `rtti_name()`（`+0` を辿る）は vtable 用**であり、`PTR_PTR_xxx`（TD オブジェクト）には使えない — 以後の probe は2方式を明示的に分ける。
- **`TESForm` の field 整理**: `+0` vtable、`+4` form type byte、`+8` flags（bit14＝v512 family gate）、`+0xC` refID（`FUN_00452df0(param_2[3],1)`）。component 表（0x68）は TESForm の 26 interface view キャッシュ。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
