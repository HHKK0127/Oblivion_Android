# v457 — `FUN_006C6910` identity・Controller vtable 特定・subobject 残の裁定（V-457-1..3 完遂・旧残点群消化第2弾）

- 採番: **v457**（旧残点群消化・probe prefix **v457_1..v457_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-457-1..3（v456 の残点）** — 1=`FUN_006C6910` identity→`Bip01[+0x1C]` 一語名／2=`[seq+0x40]` manager class と vtable `+0x5C` 用途／3=subobject 残3＋`FUN_004E82E0` purpose（V-456-3 併走）を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-457-1: `FUN_006C6910` identity** | **[確定相当]（body 全読）** | **`void __fastcall FUN_006C6910(NiControllerSequence *seq)` ＝ StoreTargets の target resolve/assign fn**: SEH prologue → **`puVar3 = FUN_00700010(&DAT_00b3cd7c)`（v456 確定の doubly-linked key lookup をグローバル map `0xB3CD7C` に対して実行）→ `InterlockedIncrement(refcount)`（smart ptr 代わり）** → `piVar1 = *(seq+0x40 → +0x7C)`（manager+0x7C ＝ v456 head machine と同物）→ **loop: `uVar6 = *(u16*)(seq[+0x18]+4+i)`（target id 配列）→ name ptr 計算（`0xffff` なら0）→ `iVar4 = virtual[piVar1]+0x4C(name, hash)`（v456 の bone/name getter 同型）→ `puVar3+0x11`（u16 count）で `puVar3[0x10]`（ptr array）を走査して一致 index を探し、`seq[+0x14]+i+8 = index×0x30 + puVar3[0xf]`（stride-0x30 entry base）を書く** → `InterlockedDecrement`＋`virtual[0]`（dtor）[確定(構造)] → **「name→entry lookup→`+0x14` 配列へ stride-0x30 エントリ ptr を store」する resolve/assign fn＝`NiControllerSequence::StoreTargets` の実処理 [確定相当]** |
| **V-457-1: `Bip01[+0x1C]` の一語名** | **[推定高]（維持・強化）** | `FUN_006C6910` は `virtual+0x4C(name)` の**返り値（`iVar4`）を index 比較に使うのみで `+0x1C` を直接消費しない** [確定(否定側・本 fn)] → **`Bip01[+0x1C]` は「`virtual+0x4C("Bip02")` が返す node の `+0x1C` field＝child/target ptr」で、それを渡す `FUN_006C6910` の引数は `param_1=seq` ではなく bone 側（v456 docs L498850 の `FUN_006c6910(*(iVar13+0x1c))` 呼び出し）** → **呼び出し元の docs 式が `+0x1C` を第1引数（fastcall ecx）に渡す構造 [確定(構造)]、`+0x1C` field 自体の型名は [未確定]＝残点** |
| **V-457-2: manager class（`[seq+0x40]`）** | **[確定]（vtable 特定）** | **ctor `mov [reg],imm32` × RTTI 法（v441 fn-head 法の応用）で vtable を2クラス特定**: **`.?AVNiControllerManager@@` → vtable `0xA79804`（ctor 3件: `FUN_006C5120/006C5520/006C5610` が store）[確定]**／**`.?AVNiControllerSequence@@` → vtable `0xA79A34`（ctor 3件: `FUN_006C6550/006C7FB0/006CA7C0`）[確定]** → **`[seq+0x40]`＝`NiControllerManager`（v456 の推定を vtable で確定）** |
| **V-457-2: vtable `+0x5C` の用途** | **[確定]（両クラスの slot 実読）＋[未確定]（呼び出し先 class）** | **`NiControllerManager [vt+0x5C] = 0x69D990` → `FUN_0069D990`（L467268）＝`xor al,al; ret` の2命令 stub（常に false 返し）[確定(機械)]** → **manager の slot+0x5C は no-op 系 [確定相当]**。**`NiControllerSequence [vt+0x5C] = 0x416E6F4E` ＝ ASCII `"None"` 定数（fn ptr でない）[確定(否定側)]** → **sequence vtable の +0x5C は data スロット＝L498845 の virtual call は sequence ではなく `[manager+0x30]` の別オブジェクトの vtable を指す [確定（構造）]** → **`[manager+0x30]` field の class（= slot `+0x5C` の実際の owner）は [未確定]＝残点**（L498845 式の `**(int**)(*(int*)(local_28+0x40)+0x30)` は manager field を経由する3段） |
| **V-457-3: subobject 残（`+0x68`）** | **[確定相当]** | **`FUN_00468590`（L82052）＝`void(void)`: `if (DAT_00b05dc0=='\0') { FUN_00569540(); return; } return;` ＝ 一次性 init guard（lazy-init）fn [確定相当]**。`DAT_00b05dc0` の docs mentions は2件（L82055 本体／L1024902 `FUN_004040e0(&DAT_00b05dc0)`＝初期化）→ **`[+0x68]` subobject の reset は「未初期化なら `FUN_00569540()` で初期化」の意味 [推定高]** |
| **V-457-3: subobject 残（`+0x54`）** | **[部分確定]** | **`FUN_0046FC10`（L88966）＝`void __thiscall(int, int*)`・`char*` locals 多用 [確定(構造)]** → **string/text 処理系 [推定高]**、正式 class は [未確定]（`+0x44` の `FUN_00469E90` も同様に未同定） |
| **V-457-3: `FUN_004E82E0` purpose** | **[未確定]（残点継続）** | v454/455 の constellation＋close-band（`[推定高]`: Ni/scene-graph 系）までで一語名は未達 |

## §2 証拠（v457_1..v457_3）

### §2.1 `FUN_006C6910` body（v457_1a + v457_2b）
```
L496358: void __fastcall FUN_006c6910(int param_1) {
  SEH prologue (LAB_009c7568 / DAT_00b30aac cookie)
  puVar3 = FUN_00700010(&DAT_00b3cd7c);          // doubly-linked key lookup (v456)
  if (puVar3) InterlockedIncrement(puVar3 + 1);
  if (puVar3) {
    piVar1 = *(int**)(*(int*)(param_1 + 0x40) + 0x7c);   // manager+0x7C
    for (i over param_1+0xc count, stride 0x10) {
      uVar6 = *(u16*)(param_1[0x18] + 4 + i);            // target id
      iVar4 = (uVar6==0xffff) ? 0 : uVar6 + *(int*)(*(int*)(param_1[0x18]+i) + 8);
      iVar4 = virtual[piVar1]+0x4C(iVar4, hash);          // name getter
      if (iVar4) {
        for (j < *(u16*)(puVar3+0x11))                    // count
          if (puVar3[0x10][j] == iVar4) {                 // ptr array match
            if (j != DAT_00a7a160)
              *(seq+0x14+i+8) = j*0x30 + puVar3[0xf];     // stride-0x30 entry
            break; } } }
    local_14++; i += 0x10; }
  InterlockedDecrement -> dtor if 0; }
=> name->entry resolve + store into seq[+0x14]  [確定(構造)]
```

### §2.2 vtable 特定（v457_3）
```
ctor `mov [reg], imm32` scan x RTTI (record classes):
  .?AVNiControllerManager@@    vt=0xA79804  ctors: FUN_006c5120 / 006c5520 / 006c5610
    [vt+0x5C] = 0x69D990 -> FUN_0069d990 (L467268): xor al, al / ret   [stub]
    slots +0x0=0x6C55F0 +0x4=0x6C5230 +0x8..0x14=0x6F7070 +0x18=0x6C5CC0 ...
  .?AVNiControllerSequence@@  vt=0xA79A34  ctors: FUN_006c6550 / 006c7fb0 / 006ca7c0
    [vt+0x5C] = 0x416E6F4E = ASCII "None"  (data, not fn)   [否定側]
    slots +0x0=0x6CB090 +0x4=0x6C6600 +0x8..0x14=0x6F7070 +0x18=0x6C87A0 ...
  template classes also matched:
    NiTArray<NiPointer<NiControllerSequence>> vt=0xA7977C  [vt+0x5C]=0x4F0F60
      (add ecx,0xc / jmp 0x44d810 = thunk)
    NiTMapBase<...> vt=0xA7975C / NiTPointerMap vt=0xA797A4 / NiTStringTemplateMap vt=0xA797E4
```

### §2.3 subobject 残（v457_1b tail / v457_2c）
```
FUN_00468590 (L82052): void(void) { if (DAT_00b05dc0==0) { FUN_00569540(); return; } return; }
  DAT_00b05dc0 docs: L82055 (guard) / L1024902 (FUN_004040e0(&DAT_00b05dc0) = init)
FUN_0046FC10 (L88966): void __thiscall(int, int*) { char* locals ... }  -> string/text [推定高]
```

## §3 残点 **V-458-1..3**
1. 【低】**`[manager+0x30]` field の class**（L498845 の3段 virtual call の中間 ptr＝slot `+0x5C` の実際の owner class）→ `NiControllerManager` の `+0x30` field を ctor/store から特定。
2. 【低】**`Bip01[+0x1C]` field の型名**（`virtual+0x4C("Bip02")` 返り値の `+0x1C` が指す構造）＋ **subobject 残2（`+0x44`＝`FUN_00469E90`／`+0x54`＝`FUN_0046FC10`）の正式 class**。
3. 【低】**旧残点（V-416-2／V-405-4..5／V-406-2..6 等）への移行** — V-458 で上記2点を片付けた後、次 v459 から旧群へ。

## §4 検証・自己批評
- probe: **v457_1..v457_3 の3本全 EXIT=0**（V-457-1 完遂（`FUN_006C6910`＝resolve/assign）／V-457-2 完遂（vtable 2クラス特定＋`+0x5C` 両読）／V-457-3 は `+0x68` 完遂・`+0x54` は部分）。
- **決定打の作り**: (i) **`FUN_006C6910` は decompiled body 全読（60行 cap→tail）で `FUN_00700010` lookup→stride-0x30 store まで一気に確定** → (ii) **vtable は「COL dword 参照探索（失敗）→ ctor `mov [reg],imm32`×RTTI 法（成功）」と手法を切り替え**（v441 fn-head 法の再利用で2クラス同時特定）→ (iii) **`+0x5C` は両クラスの slot を実読し、sequence 側が ASCII `"None"` 定数と判明＝「virtual call の対象は sequence ではない」の否定側を確定** → (iv) subobject は decompiled 実読（init guard／string）。
- 妥当性チェック: ①`FUN_006C6910` の `seq+0x40→+0x7C`⇔v456 StoreTargets head の `mov edi,[eax+0x7C]` — ②`virtual+0x4C(name)`⇔v456 の `Bip02` lookup 同型 — ③stride `0x30`⇔`j*0x30+base`（entry 構造）— ④manager `+0x5C`=`xor al,al; ret`⇔slot が no-op＝L498845 の call は中間 ptr 経由と整合（manager 直接ではない）— ⑤`"None"`(0x416E6F4E)⇔sequence vtable data slot — **五方向矛盾なし**。
- docs との不整合: **なし**（L498845 の3段式が「manager field 経由」であることを sequence vtable の `+0x5C` が fn でないことで裏付け）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **vtable 探索は「COL dword 参照」が失敗したら「ctor `mov [reg],imm32`×RTTI」へ切り替える**（本裁定で即成功・v441 法の移植性を確認）。
  2. **`[vt+off]` を読んだら「fn か data か」を必ず判定** — ASCII 定数（`"None"`）が fn ptr に見えた瞬間に「virtual call の対象が別オブジェクト」に気づける（**slot 値の byte 列が printable ASCII なら data**）。
  3. **resolve 系 fn は decompiled の「lookup→配列走査→store（stride 計算）」パターンで即 identity** — stride（`×0x30`）と base（`[0xf]`）が entry 構造の直接証拠になる。
  4. **一次性 init guard（`if (flag==0) init()`）は subobject reset fn の一類型** — `DAT_` flag＋`FUN_004040e0` 初期化の2件 grep で lazy-init と分かる。
  5. **probe は body を cap で切らず tail まで読む** — `FUN_006C6910` は L496358..L496424 の70行全体で resolve fn と確定（初回22行 cap では local 宣言止まり）。
