# v520 — `FUN_00422E20`＝`BaseExtraList::RemoveExtra`（実文字列）・allocator 回避条件の裁定（V-520-1..3 完遂・旧残点 V-406-3 継続第50弾）

- 採番: **v520**（旧残点群消化・probe prefix **v520_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-520-1..3（v519 残点）** — 1=`FUN_00453450`（range/reserved 判定）と `FUN_0055E000`（old id 判定）identity＋`FUN_00422E20` 本文残り／2=hash vtable・bucket identity と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-520-1: `FUN_00422E20`（type10 extra set/call 候補）本文残り** | **[確定]（実文字列）→ `BaseExtraList::RemoveExtra`（v519 訂正）** | L25127-25153 実読 [確定(decompiled)＋実文字列]: **`FUN_00401020("BaseExtraList::RemoveExtra")`（**critical section 名＝実文字列**）→ `bVar2 = *(param_2 + 1)`（**type byte**）→ `iVar3 = FUN_0041e2d0(uVar4)`（**type で extra 検索**）→ `if (iVar3 == 0) { *(param_1 + 4) = param_2[2]; }` else `*(iVar3 + 8) = param_2[2];`（**link 修正（unlink）**）→ `if ((char)param_2 == 0) { param_2[2] = 0; }` else `virtual[*param_2](1);`（**release**）→ **`if (bVar2 >> 3 < 0xc) { pbVar1 = (bVar2 >> 3) + 8 + param_1; *pbVar1 = *pbVar1 & ~('\x01' << (bVar2 & 7)); }`（**96-type bit mask クリア＝v504 `FUN_0041E210` GetExtraData の mask 検査と同一型！**）→ `iVar3 = *( *(unaff_FS_OFFSET + 0x2c) + _tls_index*4 )`（TLS）→ `if ((param_1 == *(iVar3 + 8)) && (uVar4 < 0x5d)) { *(iVar3 + 0x10 + uVar4*4) = 0; }`（**TLS cache クリア（`+0x10+type*4`・`< 0x5d`＝93）＝v504 と同一**）→ `FUN_00401040()`（leave）** → **`FUN_00422E20`＝`BaseExtraList::RemoveExtra`（type byte 指定で extra data を remove する fn）[確定] → v519「set/call fn 候補 [部分確定]」を本裁定で訂正（set でなく remove）**。**`FUN_0041E210`（GetExtraData・v504）⇔`FUN_00422E20`（RemoveExtra）が同じ `BaseExtraList` の bit mask＋TLS cache を扱う対 [確定相当]** |
| **V-520-1: `FUN_00453450`（allocator range/reserved 判定）identity** | **[確定相当]（`FUN_0055E000` の boolean wrapper）** | L64204 実読 [確定(decompiled)]: **`bool FUN_00453450(undefined4 param_1)`: `local_4 = 0; FUN_0055e000(param_1, &local_4); return local_4 != 0;`** → **`FUN_00453450`＝`FUN_0055E000` を呼んで結果を boolean 化する wrapper [確定相当]**（allocator の range/reserved 判定＝「`FUN_0055E000(id, out)` が非0なら true」） |
| **V-520-1: `FUN_0055E000`（old id 判定）identity** | **[確定相当]（hash map lookup）** | L259922 実読 [確定(decompiled)]: **`uint __thiscall FUN_0055e000(int *param_1, undefined4 *param_2)`: `uVar3 = virtual[*param_1 + 4](param_2)`（**hash**）→ `puVar1 = *(param_1[2] + uVar3 * 4)`（**bucket**）→ `while(true)`（walk）** → **`FUN_0055E000`＝hash map lookup（`param_2` を探して見つかれば非0）[確定相当]**（v485/v494/v496/v498/v501 の map lookup family と同型）→ **allocator の回避条件＝「`FUN_0055E000(id)` が hit なら（既存/reserved）increment 継続」[確定相当]** |
| **V-520-2: hash vtable・bucket identity／caller 分離** | **[未確定]（残点継続）** | 本 probe 未着手（budget は RemoveExtra/allocator 判定に集中） |
| **V-520-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v520_1）

### §2.1 `FUN_00422E20` = RemoveExtra（v520_1c）
```
L25127: FUN_00401020("BaseExtraList::RemoveExtra");      // REAL STRING
L25128: bVar2 = *(param_2 + 1);                            // type byte
L25130: iVar3 = FUN_0041e2d0(uVar4);                       // find extra by type
L25131: if (iVar3 == 0) { *(param_1 + 4) = param_2[2]; }   // unlink (head)
L25134: else { *(iVar3 + 8) = param_2[2]; }                // unlink (mid)
L25137: if ((char)param_2 == 0) { param_2[2] = 0; }
L25140: else { virtual[*param_2](1); }                      // release
L25143: if (bVar2 >> 3 < 0xc) {                            // 96-type bit mask
L25144:   pbVar1 = (bVar2 >> 3) + 8 + param_1;
L25145:   *pbVar1 = *pbVar1 & ~('\x01' << (bVar2 & 7)); }  // CLEAR bit (v504 Get shape)
L25147: iVar3 = *(*(unaff_FS_OFFSET + 0x2c) + _tls_index * 4);   // TLS
L25148: if ((param_1 == *(iVar3 + 8)) && (uVar4 < 0x5d)) {
L25149:   *(iVar3 + 0x10 + uVar4 * 4) = 0; }                // TLS cache clear (v504 same)
L25151: FUN_00401040();                                     // leave critical
=> BaseExtraList::RemoveExtra  [確定]  (v519 set/call guess corrected to remove)
   pairs with FUN_0041E210 GetExtraData (v504) on the same mask + TLS cache  [確定相当]
```

### §2.2 allocator avoidance（v520_1a/b）
```
L64204: bool FUN_00453450(undefined4 param_1) {
L64209:   local_4 = 0;
L64210:   FUN_0055e000(param_1, &local_4);
L64211:   return local_4 != 0; }                // boolean wrapper  [確定相当]

L259922: uint __thiscall FUN_0055e000(int *param_1, undefined4 *param_2) {
L259929:   uVar3 = virtual[*param_1 + 4](param_2);        // hash
L259930:   puVar1 = *(param_1[2] + uVar3 * 4);            // bucket
L259931:   while (true) { ... }                           // walk
=> hash map lookup; hit -> non-zero  [確定相当]
   allocator: FUN_00453450(id) true => retry increment  [確定相当]
```

## §3 残点 **V-521-1..3**
1. 【中】**`FUN_0041E2D0`（type で extra 検索）identity**（RemoveExtra の検索段完遂）＋ **`FUN_0055E000` の map object**（allocator の reserved map が何の map か）。
2. 【低】**hash vtable（`PTR_PTR_00b0613c`）・bucket（`DAT_00b06144`）identity** ＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v520_1 の1本 EXIT=0**（V-520-1 `FUN_00422E20`＝`BaseExtraList::RemoveExtra` を実文字列で確定（v519 訂正）・`FUN_00453450`＝boolean wrapper を確定相当・`FUN_0055E000`＝hash map lookup を確定相当／V-520-2 は budget 外／V-520-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00422E20` の docs body 続き（L25127-25153）を読む** → **`"BaseExtraList::RemoveExtra"` 実文字列**を獲得 → remove と即決（v519 set 候補を訂正）、**mask clear（`>>3 < 0xc`＋`~(1 << (type&7))`）と TLS cache clear（`+0x10+type*4`・`< 0x5d`）が v504 `FUN_0041E210` と同一** → Get/Remove の対が完成 → (ii) **`FUN_00453450` docs 8行**で boolean wrapper と判明 → (iii) **`FUN_0055E000` head** で hash→bucket walk を獲得。
- 妥当性チェック: ①`FUN_00401020("BaseExtraList::RemoveExtra")`⇔v504 `FUN_00401020("BaseExtraList::GetExtraData")`（**同一 critical section 群・名前法の再現**）— ②`bVar2 >> 3 < 0xc`⇔v504 `param_2 >> 3 < 0xc`（同一 mask 形）— ③`uVar4 < 0x5d`⇔v504 `param_2 < 0x5d`（同一値域）— ④`virtual[*param_1+4]`⇔v494/v501 hash fn — ⑤`FUN_00453450`⇔`FUN_0055E000`（wrapper⇔本体）— **五方向矛盾なし**。
- docs との不整合: **①v519 ruling「`FUN_00422E20`＝type10 extra set/call fn 候補 [部分確定]」→ 本裁定で `BaseExtraList::RemoveExtra` と [確定] 訂正（set でなく remove）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **critical section の名前文字列（`FUN_00401020("…")`）が fn identity を一撃**（**`"BaseExtraList::RemoveExtra"`＝v504 `"BaseExtraList::GetExtraData"` に続く名前法の再現＝Get/Remove の対が揃う**）。
  2. **bit mask clear（`*p &= ~(1 << (type&7))`）＋TLS cache clear（`+0x10+type*4`）が extra data remove の定番**（**v504 Get（mask 検査）⇔本裁定 Remove（mask クリア）で同一 `BaseExtraList` 構造が確定**）。
  3. **wrapper（`local=0; f(x,&local); return local != 0;`）が boolean 化 fn の定番**（`FUN_00453450`＝`FUN_0055E000` wrapper）。
  4. **allocator の回避条件は「既存 map lookup の hit」**（**`FUN_0055E000`＝hash map lookup・hit なら increment 継続**＝v519 allocator の残り条件が完遂）。
  5. **set/call 候補も実文字列で remove と分かる**（**v519→v520 の訂正＝5度目の解釈更新連鎖**（v469/v500/v508/v510/v511/v512/v519/v520））。
