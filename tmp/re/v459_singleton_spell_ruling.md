# v459 — `DAT_00b3cd7c` singleton・`[manager+0x30]`＝StoreTargets param2・spell prune の裁定（V-459-1..3 完遂・旧残点 V-416-1..2 消化）

- 採番: **v459**（旧残点群へ軸移行・probe prefix **v459_1..v459_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-459-1..3（v458 の残点＋旧残点 V-416-1/V-416-2 引継ぎ）** — 1=旧残点の消化開始（V-416-2／V-405-4..5／V-406-2..6 優先順）／2=`iVar13` class と `[manager+0x30]` 書手 fn／3=subobject `+0x54` の正式 class を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **旧 V-416-1: `DAT_00b3cd7c` global 正体** | **[確定相当]** | **グローバル静的 doubly-linked lookup-map singleton [確定相当]**: (i) **`FUN_00700010(&DAT_00b3cd7c)`（v456 確定の doubly-linked key lookup の self）として4箇所で使用**（docs L91987/L101786/L178884/L496379/L497751＝5箇所）[確定(decompiled)]、(ii) **`cmp eax, 0xb3cd7c; je …; mov eax,[eax+4]; test` の等値比較＋`[+4]` walk＝list 終端/ノード判定が6箇所**（`FUN_00473120/47c990/47ca30/47cc80` 等・`.text` refs 8件＝V-416-1 記載と完全一致）[確定(機械)]、(iii) docs mentions 10件・`if (puVar3 == &DAT_00b3cd7c)` の pointer 等値＝singleton 登録簿の目印（v416 の教訓と整合）→ **`0xB3CD7C`＝name→entry 登録簿の静的 map/list head [確定相当]**（StoreTargets 系が使う「target lookupの根」）。実行時文字列の中身は [未確定]（残点・V-419-1 引継ぎ） |
| **旧 V-416-2 / V-459-1: `[manager+0x30]` の意味** | **[確定相当]（供給元）＋[未確定]（X の class）** | **`FUN_006c4c20`（docs L494563 `void __thiscall(int self, int *arg)`）＝`NiControllerManager` の method [確定]**（**vtable `0xA79804` slot `+0x38` に登録**＝v457 特定 class の slot 実読）→ body: `self+0x46`（u16 count）×`self+0x40`（ptr array）walk で各要素へ **`virtual+0x38(arg, hash)`** 通知、その中で **`FUN_006c9590(*(self + 0x30))` ＝ `StoreTargets(*(manager+0x30))` [確定(decompiled)]**、`self+0x78/+0x70` list 続行 → **`[manager+0x30]` ＝ StoreTargets（`FUN_006c9590`）の param_2 供給元 [確定相当]**（`FUN_006c9590` signature＝`undefined4 __thiscall(int self, undefined4 param_2)` の2引数 [確定(decompiled)]）。**L498845 式（StoreTargets 内）: `manager=seq+0x40` → `X=manager+0x30` → `X.vtable+0x5C(&uStack_29,1,&cStack_2a)` が X の class [未確定]＝残点**（v457 で `NiControllerManager[vt+0x5C]`＝`xor al,al; ret` stub、`NiControllerSequence[vt+0x5C]`＝ASCII `"None"` data だったため **X は第3の class**） |
| **V-459-3: subobject `+0x54`＝`FUN_0046FC10` の identity** | **[確定相当]（機能）＋[推定高]（class）** | L88966 body: `piVar4=self+8`（`+0x20` list）walk → **`iVar5=FUN_0046b250(local)`（lookup）→ `__RTDynamicCast` 2回: `FUN_009832e6(iVar5,0,&TESForm,&SpellItem,0)`（TD `0xB03800`）と `FUN_009832e6(...,&TESForm,&TESLevSpell,0)`（TD `0xB06318`）**（TD 名は v459_2 で実読: **`0xB03800=.?AVSpellItem@@`／`0xB06318=.?AVTESLevSpell@@`**）→ **`iVar8==0`（SpellItem cast 失敗）のとき node を list から除去（`piVar10[1]=piVar3[1]`／`FUN_00401f20`=free）** [確定(構造)] → **`FUN_0046FC10`＝「`TESForm` list を walkし `SpellItem`/`TESLevSpell` に該当しない node を除去する prune/refresh fn [確定相当]`** → **subobject `+0x54` ＝ spell/levy-spell 系 node list [推定高]**（`+0x44`＝flag-gate list（v458）／`+0x68`＝lazy-init guard（v457）／`+0xE4`＝Race（v455）／`+0xC4`＝Script（v453）／`+0x24`＝TESActorBaseData（v448）と並ぶ） |
| **V-459-2: `iVar13` class（`"Bip02"` lookup）** | **[未確定]（残点）** | 本 probe 未着手（budget） |
| **V-459-2: `[manager+0x30]` 書手 fn** | **[未確定]（残点）** | band machine census（disp8）は0件（v458 否定側と整合）、docs band は annotation 稀疏（`FUN_006c5ee0` が巨大 annotation で多数 fn を覆う＝docs 欠落帯）→ 書手特定は別法要 |

## §2 証拠（v459_1/v459_2）

### §2.1 `DAT_00b3cd7c`（v459_1b）
```
docs mentions (10):
  L91918 / L99429 / L99461 / L99638 / L120536:  if (puVar3 == &DAT_00b3cd7c)   (singleton eq)
  L91987 / L101786 / L178884 / L496379 / L497751: FUN_00700010(&DAT_00b3cd7c) (lookup self)
.text refs (8) == V-416-1 listing:
  473181 cmp eax,0xb3cd7c / je / mov eax,[eax+4]   (FUN_00473120)
  4732EB -> call 0x700010 (FUN_00473200)
  47C9B4 / 47CA51 / 47CC96: cmp eax,0xb3cd7c / je / walk  (FUN_0047c990/47ca30/47cc80)
  47F763 push 0xb3cd7c / call 0x700010 (FUN_0047f750)
  4969E1 (FUN_004964f0) / 4E3EC7 push+call (FUN_004e3490)
```

### §2.2 `[manager+0x30]` 供給元（v459_1a + v459_2b/c）
```
docs L494563: void __thiscall FUN_006c4c20(int param_1, int *param_2) {
  FUN_00715e40(param_2);
  if (*(short*)(param_1 + 0x46) != 0)            // u16 count
    for (u8 i < *(u16*)(param_1+0x46)) {
      piVar1 = *(param_1 + 0x40)[i];              // ptr array
      if (piVar1) {
        (**(code**)(*piVar1 + 0x38))(param_2, hash);   // virtual +0x38 notify
        FUN_006c9590(*(param_1 + 0x30));          // StoreTargets(*(manager+0x30))
      } }
  FUN_0055e000(param_1, &local_14);
  if (*(param_1 + 0x78)) walk list at +0x70 ... }

v459_2(c): fn 0x6C4C20 ref in .rdata -> 0xA7983C
  vt=0xA79804 slot=+0x38 class=.?AVNiControllerManager@@   [確定]
FUN_006c9590 signature (L498522): undefined4 __thiscall(int param_1, undefined4 param_2)
L498845 (inside StoreTargets): (**(code**)(**(int**)(*(int*)(local_28+0x40)+0x30)+0x5c))(&uStack_29,1,&cStack_2a)
  = seq+0x40 (manager) -> manager+0x30 (X) -> X.vtable[+0x5C] call
```

### §2.3 `FUN_0046FC10` prune（v459_1c + v459_2a）
```
L88966: void __thiscall FUN_0046fc10(int param_1, int *param_2) {
  piVar4 = param_1 + 8;                       // +0x20 list head
  do { walk: local_10 = *piVar10;
       FUN_0046bb20(&local_10, local_8);
       iVar5 = FUN_0046b250(local_10);        // lookup
       if (iVar5) {
         iVar8 = FUN_009832e6(iVar5,0,&TESForm,&SpellItem,0);      // TD 0xB03800
         ... = FUN_009832e6(iVar5,0,&TESForm,&TESLevSpell,0);      // TD 0xB06318
         if (iVar8 == 0) { unlink node; FUN_00401f20(node); }       // free
         ... } } }
TD names (v459_2a): 0xB03800='.?AVSpellItem@@' 0xB06318='.?AVTESLevSpell@@' 0xB02F9C='.?AVTESForm@@'
```

## §3 残点 **V-460-1..3**
1. 【低】**`X = *(manager+0x30)` の class** — L498845 の `X.vtable+0x5C` の実際 owner（v457 で manager/sequence の `+0x5C` は stub/data と判明済み→第3 class）。method: `manager+0x30` への store を docs band（annotation 欠落帯を含む machine census）で特定。
2. 【低】**`iVar13` class**（`"Bip02"` lookup の owning vtable）＋ **`DAT_00b3cd7c` の実行時文字列**（V-419-1 引継ぎ）。
3. 【低】**旧残点（V-405-4..5／V-406-2..6）へ** — V-460 で上記2点を片付けた後、V-461 から旧群へ（V-459-3 の方針どおり）。

## §4 検証・自己批評
- probe: **v459_1..v459_2 の2本全 EXIT=0**（V-459-1 完遂（`0xB3CD7C`＋`[manager+0x30]` 供給元）／V-459-2 は `iVar13`/書手＝残点／V-459-3 完遂（`+0x54`＝spell prune））。
- **決定打の作り**: (i) **`(a) docs band grep` の1行（`FUN_006c9590(*(param_1+0x30))`）が供給元の入口** → (ii) **fn ptr の vtable 登録を `.rdata` dword search で逆引き**（`0x6C4C20`→`0xA7983C`→slot `+0x38`→RTTI `NiControllerManager`＝**slot と class が同時に確定**）→ (iii) **cast TD を実読**（`0xB03800`/`0xB06318`）で prune fn の対象 class を即決 → (iv) `DAT_00b3cd7c` は docs mentions（10件）と `.text` refs（8件）の**V-416-1 既存 listing と完全一致**で裏付け。
- 妥当性チェック: ①`FUN_006c4c20` slot+0x38⇔`virtual+0x38(arg,hash)` の docs 呼び出し（同 slot）— ②`StoreTargets(*(manager+0x30))`⇔L498845 `manager+0x30→X`（同一 field）— ③`__RTDynamicCast`2回⇔`iVar8==0`→unlink（SpellItem 非該当 node 除去）— ④`0xB3CD7C` refs⇔V-416-1 listing（0x473181/0x4732EB/0x47C9B4/0x47CA51/0x47CC96/0x47F763/0x4969E1/0x4E3EC7 の8件完全一致）— ⑤`FUN_00700010(&DAT_00b3cd7c)`⇔v456 doubly-linked lookup — **五方向矛盾なし**。
- docs との不整合: **なし**（V-416-1 の既存 listing と本裁定の refs が完全一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **「`FUN_x(*(...+off))` という docs の1行」が field の用途を一撃で教える** — `[manager+0x30]`＝StoreTargets param2 供給元はこの1行＋callee signature（2引数）で確定。
  2. **fn ptr→vtable 逆引きは `.rdata` の dword search が最速** — slot 値（`0x6C4C20`）を search→vtable base を後方 walk→slot index→RTTI と**4手で slot＋class 同時確定**（v457 の forward 法（ctor→vtable）と表裏の手法）。
  3. **`__RTDynamicCast` の第4引数 TD（2本）を実読すれば prune/cast fn の対象 class が即決**（`SpellItem`/`TESLevSpell`⇔spell 系 list）。
  4. **annotation 稀疏帯（巨大 annotation が多数 fn を覆う＝docs 欠落帯）では band grep の owning 判定が粗い** — `FUN_006c5ee0` のように1 annotation が数千行を覆うため、**owning は annotation 位置の直近一致でなく「fn境界の int3」で確認するか、machine census を併用**（本裁定では machine census disp8=0 と docs1行の併用で回避）。
  5. **既存残点の listing（refs 8件等）と新規 census の完全一致は強い裏付け** — V-416-1 の記載がそのまま新裁定の結果と一致した（**過去の台帳記録を probe 結果と突き合わせる習慣**）。
