# v458 — `Bip01[+0x1C]`＝sequence ptr の確定・manager ctor・`+0x44` list fn の裁定（V-458-1..3 完遂・旧残点群消化第3弾）

- 採番: **v458**（旧残点群消化・probe prefix **v458_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-458-1..3（v457 の残点）** — 1=`[manager+0x30]` field class／2=`Bip01[+0x1C]` field 型名と subobject 残2（`+0x44/+0x54`）／3=旧残点移行判断を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-458-2: `Bip01[+0x1C]` field の型名** | **[確定相当]（docs 式＋body 突合）** | **docs L498848-51 の完全構造**: `iVar13 = virtual[local_24]+0x4C("Bip02")` → **`iVar13 = virtual[piVar2]+0x4C(&DAT_00a30968)`（第2 lookup）→ `if (*(iVar13+0x1c) != 0) FUN_006c6910(*(int*)(iVar13+0x1c))` [確定(decompiled)]**。**`FUN_006C6910` の body（v457）が param_1 の `+0x40`（manager）／`+0x18`（target id 配列）／`+0x14`（store 先）をアクセス＝param_1 は `NiControllerSequence` [確定(構造)]** → **`virtual+0x4C(name)` lookup 結果（`iVar13`）の `+0x1C` ＝ `NiControllerSequence` ptr [確定相当]** → **`Bip01[+0x1C]`＝avatar/bone 系 node の「sequence 埋め込み/参照 field」**（呼び出し元 `FUN_006C9590`=StoreTargets の文脈と整合）。`iVar13` 自体の class（`+0x4C` getter の返り値型）は [未確定]（残点・`"Bip02"` lookup の owning class） |
| **V-458-1: `[manager+0x30]` field の書手** | **[確定]（否定側）＋[未確定]（残点）** | **`FUN_006C5120`（NiControllerManager ctor・`*param_1=&PTR_FUN_00a79804` が v457 vtable と完全一致 [確定]）の machine `[reg+0x30]` store＝0件 [確定(否定側)]**、docs body（L494830-494867）の `param_1[0xC]`（=+0x30）代入も表示範囲に無し → **ctor は `[manager+0x30]` を書かない＝別 fn（setter/add 系）が書く [確定（否定側）]、書手 fn は [未確定]＝残点**。副次: ctor field 列（`param_1[0x1e]`=+0x78＝0／`[0x1f]`=+0x7C＝smart ptr（InterlockedDecrement dtor 伴う）／`[0x1c]`=+0x70＝vector（dtor）／`[0x13]`=+0x4C／`[0x10]`=+0x40）を docs で取得 [確定(decompiled)] |
| **V-458-2: subobject `+0x44`＝`FUN_00469E90`** | **[確定相当]（構造）＋[未確定]（class）** | L83675: **`void __thiscall FUN_00469e90(int self, undefined4 key)`: `if ((*(u8*)(self+4) & 1) == 0)`（flag bit0 gate）→ `piVar1 = (int*)(self+8)`（`+0x20`）linked list walk → `FUN_004695d0(key)` でマッチ判定 → node 更新/挿入（`piVar3` は prev）** [確定(構造)] → **「flag bit0 未立なら `+0x20` list に key を insert/update」する list 管理 reset fn [確定相当]**（`+0x44` の正式 class は [未確定]） |
| **V-458-2: subobject `+0x54`＝`FUN_0046FC10`** | **[未確定]（残点継続）** | v457 の `char*` locals 観察（string/text 系 [推定高]）まで。本 probe 未着手 |
| **V-458-3: 旧残点への移行** | **[確定]（方針）** | **V-459 から旧残点（V-416-2／V-405-4..5／V-406-2..6 等）へ軸を移す [確定（作業方針）]**（V-458 で `Bip01[+0x1C]`＝sequence ptr を確定相当まで到達したため、Controller 系の残点（`[manager+0x30]` 書手／`iVar13` class／`+0x54`）は低優先で V-460+ に併走） |

## §2 証拠（v458_1）

### §2.1 manager ctor（v458_1a）
```
L494830: void __fastcall FUN_006c5120(undefined4 *param_1) {
  SEH prologue (LAB_009c732f / DAT_00b30aac)
  *param_1 = &PTR_FUN_00a79804;        // == v457 NiControllerManager vtable [確定]
  FUN_00739670(cookie);
  param_1[0x1e] = 0;                   // +0x78
  puVar1 = param_1[0x1f]; if (puVar1) InterlockedDecrement -> dtor  // +0x7C smart ptr
  pvVar2 = param_1[0x1c]; if (pvVar2) vector dtor (FUN_007016a0)    // +0x70
  FUN_006c5050(); FUN_00401f20(param_1[0x13]); pvVar2 = param_1[0x10]; ... }
machine [reg+0x30] stores in 0x6C5120..0x6C5230: 0   [確定(否定側)]
```

### §2.2 Bip01[+0x1C] = sequence（v458_1b）
```
L498848: iVar13 = (**(code**)(*local_24 + 0x4c))("Bip02");
L498849: iVar13 = (**(code**)(*piVar2 + 0x4c))(&DAT_00a30968);
L498850: if (*(int*)(iVar13 + 0x1c) != 0) {
L498851:   FUN_006c6910(*(int*)(iVar13 + 0x1c)); }
FUN_006C6910 param_1 accesses (v457): +0x40 (manager) / +0x18 (target ids)
  / +0x14 (store) / +0xc (count)  => param_1 = NiControllerSequence
=> *(iVar13 + 0x1C) is a NiControllerSequence*   [確定相当]
machine [reg+0x1c] in FUN_006c6910: only stack aliases (lea [esp+0x1c])
```

### §2.3 FUN_00469E90（v458_1c）
```
L83675: void __thiscall FUN_00469e90(int param_1, undefined4 param_2) {
  if ((*(byte*)(param_1 + 4) & 1) == 0) {
    piVar1 = (int*)(param_1 + 8);          // +0x20 list head
    while (piVar1) {
      iVar2 = *piVar1;
      if ((iVar2 == 0) || (FUN_004695d0(param_2), *(int*)(iVar2 + 4) != 0))
        piVar1 = piVar1[1];                // walk next
      else if (piVar3 == 0) { ... insert ... }
      ... } } }
```

## §3 残点 **V-459-1..3**
1. 【低】**旧残点の消化開始** — **V-416-2**／**V-405-4..5**／**V-406-2..6** を優先順に着手（V-458-3 の方針どおり）。
2. 【低】**`iVar13` class**（`virtual+0x4C("Bip02")` の owning class＝`"Bip02"` lookup の vtable 特定）と **`[manager+0x30]` 書手 fn**（setter/add 系の E8 caller census）。
3. 【低】**subobject `+0x54`（`FUN_0046FC10`）の正式 class**（v457 string/text [推定高] の確定）。

## §4 検証・自己批評
- probe: **v458_1 の1本 EXIT=0**（V-458-2 `Bip01[+0x1C]`=sequence ptr を**確定相当**まで／V-458-1 否定側確定・書手は残点／V-458-3 方針確定）。
- **決定打の作り**: (i) **docs 式（`FUN_006c6910(*(iVar13+0x1c))`）と callee body の field アクセス（v457 の `+0x40/+0x18/+0x14`）を突合**して引数型を同定 — **「callee が触る field の束」が引数の class を語る** → (ii) manager ctor は `*param_1=&PTR_FUN_00a79804`（v457 vtable と一致）で identity 再確認＋`[reg+0x30]` 否定側 → (iii) `+0x44` は decompiled の flag gate＋list walk で構造確定。
- 妥当性チェック: ①ctor vtable `0xA79804`⇔v457 特定値（完全一致）— ②`FUN_006C6910` の field 群⇔seq 構造（v457 body）⇔docs L498851 引数 — ③`param_1[0x1f]`=+0x7C⇔v456 head の `mov edi,[eax+0x7C]`（manager+0x7C＝seq からアクセスされる manager field）— ④`FUN_00469E90` の `+0x20` list⇔`piVar1[1]` walk（`+4`＝next）— **四方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **引数の class は「callee が触る field の束」で同定できる** — `FUN_006C6910` の `+0x40/+0x18/+0x14` が seq 構造と一致したことで、docs の引数式 `*(iVar13+0x1c)` が sequence ptr と一気に確定（**body と call site の双方向突合**）。
  2. **vtable は ctor の `*param_1=&PTR_FUN_xxx` docs 表示でも確認できる**（machine imm32 と並行して docs の `&PTR_FUN_` が vtable 値と一致するかを照合）。
  3. **「ctor が書かない」は否定側証拠として価値が高い** — `[manager+0x30]` は setter/add 系残点と確定し調査経路を1つ消す。
  4. **flag gate（`byte[+4]&1`）＋list walk は reset fn の定型2型目**（v457 の lazy-init guard に続く類型化）。
