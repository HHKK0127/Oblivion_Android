# v519 — `FUN_00448D40`＝form id allocator（`+0x8C0` increment）・gated mask 源＝`FUN_0045B670` arg3 の裁定（V-519-1..3 完遂・旧残点 V-406-3 継続第49弾）

- 採番: **v519**（旧残点群消化・probe prefix **v519_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-519-1..3（v518 残点）** — 1=`FUN_00448D40` body 残り（`uVar2` 出所）＋`FUN_00422E20` identity＋type10 `+0xc` field 語／2=gated mask 源と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-519-1: `FUN_00448D40` body 残り（`uVar2` 出所・current id source）** | **[確定相当]（form id allocator＝`+0x8C0` increment＋衝突チェック）** | L55969-55993 全読 [確定(decompiled)]: **`do { uVar1 = *(param_1 + 0x8c0)`（**old id 読み**）→ `uVar2 = *(param_1 + 0x8c0)`（再読）→ `if ((uVar2 & 0xffffff) == 0xffffff) { iVar5 = (uVar2 & 0xff000000) + 0x800; }` else `iVar5 = uVar2 + 1;`（**下位24bit が満杯なら FF segment を上げて `0x800`（2048）へリセット／そうでなければ id+1**）→ `*(param_1 + 0x8c0) = iVar5;`（**`+0x8C0` へ書く＝v512/v513 `FUN_00446B40` の同 field！**）→ `iVar6 = virtual[*PTR_PTR_00b0613c + 4](iVar5)`（**hash**）→ `for (puVar3 = *(DAT_00b06144 + iVar6*4); puVar3; puVar3 = *puVar3) { cVar4 = virtual[+8](iVar5, puVar3[1]); if (cVar4) goto LAB; }`（**bucket 走査で衝突なら再 increment**）→ `cVar4 = FUN_00453450(*(param_1+0x8c0))`（**range/reserved 判定 [推定]**）`} while (cVar4)` → `cVar4 = FUN_0055e000(uVar1, &iStack_4); if (!cVar4 && !FUN_00453450(uVar1)) return uVar1;`（**old id が使えれば返す**）` } while(true)` [確定(構造)]** → **`FUN_00448D40`＝「`param_1+0x8C0`（current form id）を increment（`0xFFFFFF` 時 FF segment rollover＋`0x800` リセット）し、hash bucket 衝突＋range 判定を回避して **old id を返す form id allocator** [確定相当]」** → **`+0x8C0` field が「id counter」であることを確定**（v512 `FUN_00446B40`（set/rollback）⇔本裁定（increment/allocate）の対） |
| **V-519-2: gated 側 mask 源（`edx`）** | **[確定相当]（mask＝`FUN_0045B670` の arg3）** | `FUN_0045B670` docs head 実読 [確定(decompiled)]: **`void __thiscall FUN_0045b670(int param_1, int param_2, undefined4 param_3)`（3引数）→ `iVar1 = *( *(unaff_FS_OFFSET + 0x2c) + _tls_index*4 )`（**TLS block**）→ gate: `(*(iVar1+0x185) != 0) && (*(iVar1+0x184) == 0) && (*(DAT_00b33a98+0xcd4) == 0) && ((*(param_2+8) >> 0xe & 1) == 0)`（**4条件 gate: TLS flags＋global `+0xcd4` flag＋bit14**）→ `cVar2 = FUN_0045a500()`（**Player `+0x18` flag reader・v497**）→ `if (!cVar2) FUN_00452c20(param_2, param_3);` else `if ((*(param_1+0x18) >> 3 & 1) && *(param_1+4)) FUN_00452c20(param_2, param_3);`** → **`FUN_00452C20`（v493 lazy node `|=`）の第2引数（mask）に渡るのは **`param_3`＝`FUN_0045B670` の第3引数** [確定相当]** → **「gated 側 mask 源（`edx` 設定元）」residual＝解消（mask は計算されず caller から arg3 で供給される）[確定相当]**。**副次: `param_1+0x18` bit3 判定（別 object の flag）と `param_2+8` bit14 gate（node holder）が別 object（`param_1`≠`param_2`）[確定相当]** |
| **V-519-1: `FUN_00422E20`（type10 extra set/call）identity** | **[部分確定]** | L25117 実読 [確定(decompiled)]: **`void __thiscall FUN_00422e20(int param_1, undefined4 *param_2)`（`param_2`＝ptr）→ `if (param_2 != 0) { ... }`（**null check**）＋WARNING unreachable block** → `L26539` の `FUN_00422e20(iVar2, 1)`（`1` を ptr 扱いで渡す）と整合 → **type10 extra に対する set/call fn 候補 [部分確定]**（本文残り cap 外） |
| **V-519-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v519_1）

### §2.1 `FUN_00448D40` allocator body（v519_1a）
```
L55969: do {
L55970:   uVar1 = *(param_1 + 0x8c0);              // OLD id
L55973:   uVar2 = *(param_1 + 0x8c0);              // re-read
L55974:   if ((uVar2 & 0xffffff) == 0xffffff) {     // 24-bit full?
L55975:     iVar5 = (uVar2 & 0xff000000) + 0x800;  // bump FF segment, reset 2048
L55978:   } else { iVar5 = uVar2 + 1; }            // else id+1
L55980:   *(param_1 + 0x8c0) = iVar5;              // STORE to +0x8C0 (same field as FUN_00446B40!)
L55981:   iVar6 = virtual[*PTR_PTR_00b0613c + 4](iVar5);      // hash
L55982:   for (puVar3 = *(DAT_00b06144 + iVar6*4); puVar3; puVar3 = *puVar3) {
L55984:     cVar4 = virtual[+8](iVar5, puVar3[1]);  // key compare
L55985:     if (cVar4) goto LAB;                     // collision -> retry
L55987:   cVar4 = FUN_00453450(*(param_1 + 0x8c0));  // range/reserved check [推定]
L55988: } while (cVar4);
L55989: cVar4 = FUN_0055e000(uVar1, &iStack_4);
L55990: if (!cVar4 && !FUN_00453450(uVar1)) return uVar1;   // return OLD id
L55993: } while (true);
=> form id allocator: increment +0x8C0 (FF rollover @0xFFFFFF->+0x800),
   hash collision + range avoidance, return previous id  [確定相当]
   +0x8C0 = id counter field (pair with FUN_00446B40 set/rollback)  [確定相当]
```

### §2.2 gated mask source（v519_1c）
```
L72253: void __thiscall FUN_0045b670(int param_1, int param_2, undefined4 param_3)  // 3 args
L72260: iVar1 = *(*(unaff_FS_OFFSET + 0x2c) + _tls_index * 4);        // TLS block
L72261: if ((*(iVar1+0x185) != 0) && (*(iVar1+0x184) == 0) &&
            (*(DAT_00b33a98 + 0xcd4) == 0) && ((*(param_2 + 8) >> 0xe & 1) == 0)) {
L72263:   cVar2 = FUN_0045a500();                    // Player +0x18 flag reader (v497)
L72265:   if (!cVar2) FUN_00452c20(param_2, param_3);   // MASK = param_3 (arg3)
L72267:   else if ((*(param_1 + 0x18) >> 3 & 1) && *(param_1 + 4))
L72268:     FUN_00452c20(param_2, param_3);
=> mask source = caller-supplied arg3 (param_3), not computed internally
   -> gated mask residual CLOSED  [確定相当]
   param_1 (+0x18 flags) and param_2 (+0x8 bit14) are different objects  [確定相当]
```

### §2.3 `FUN_00422E20`（v519_1b）
```
L25117: void __thiscall FUN_00422e20(int param_1, undefined4 *param_2) {
L25126:   if (param_2 != (undefined4 *)0x0) { ... }
  + WARNING: Removing unreachable block (ram, 0x00422e8d)
=> type-10 extra set/call candidate; called as FUN_00422e20(iVar2, 1)  [部分確定]
```

## §3 残点 **V-520-1..3**
1. 【中】**`FUN_00453450`（range/reserved 判定）と `FUN_0055E000`（old id 判定）identity**（allocator の回避条件完遂）＋ **`FUN_00422E20` 本文残り**（type10 extra set の中身）。
2. 【低】**`PTR_PTR_00b0613c`（hash fn vtable）と `DAT_00b06144`（bucket 配列）の identity**（allocator の hash map 完全解）＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v519_1 の1本 EXIT=0**（V-519-1 `FUN_00448D40`＝form id allocator を本文全読で確定相当・`+0x8C0`＝id counter を確定／V-519-2 gated mask 源＝arg3 を確定相当で residual 解消／V-519-1 `FUN_00422E20` は部分確定／V-519-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00448D40` body L55969-55993 を全読** → `+0x8C0` increment＋`0xFFFFFF` rollover＋hash 衝突＋`return uVar1`（old id）で **allocator と即決**、**`+0x8C0` が v512/v513 `FUN_00446B40` の同 field と一致** → id counter の get/set 対が完成 → (ii) **`FUN_0045B670` head 22行** → `FUN_00452c20(param_2, param_3)` の第2引数＝`param_3`（arg3）で **mask 源 residual を解消** → (iii) **`FUN_00422E20` head** で null check まで部分確定。
- 妥当性チェック: ①`(uVar2 & 0xff000000) + 0x800`⇔v518 current id 計算（同一式）— ②`*(param_1+0x8c0) = iVar5`⇔v512 `FUN_00446B40` の `*(param_1+0x8c0) = param_2`（同一 field write）— ③`FUN_00452c20(param_2, param_3)`⇔v493 lazy node `|=`（mask＝param_3）— ④`FUN_0045a500()`⇔v497 Player `+0x18` flag reader（同 fn）— ⑤`param_1+0x18` bit3⇔v507 node bit census（bit3＝43 hits）— **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_00448D40`/`FUN_0045B670` とも docs と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **id counter field の「allocate（increment）⇔set/rollback」の対が揃うと field が確定**（**`FUN_00448D40`（get/increment）⇔`FUN_00446B40`（set/rollback）が `+0x8C0` で対**＝v512→v513→v519 の連鎖完遂）。
  2. **mask 源は「mask を渡す fn の引数」で決まる**（**`FUN_0045B670` arg3 が mask＝計算されず caller 供給**＝v493/v495/v498 に続く mask 追跡の完成）。
  3. **allocator の定番（increment→hash→collision retry→range check→return old）**が本文で判明（**`0xFFFFFF` rollover＋`0x800` リセットが FF segment 型 id の目印**）。
  4. **gate が4条件（TLS×2＋global＋bit14）＋別 gate（Player flag＋`param_1+0x18` bit3）の複層**（**gated notify の条件構造が判明**＝v512 family の semantic 完遂）。
  5. **`unaff_FS_OFFSET`（SEH/TLS）が fn head の目印**（v518 に続く・**`_tls_index` が TLS 使用の決定打**）。
