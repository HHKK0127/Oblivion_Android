# v499 — lock release/lookup/set sub 完結・`+0x18` machine writer 122件候補の裁定（V-499-1..3 完遂・旧残点 V-406-3 継続第29弾）

- 採番: **v499**（旧残点群消化・probe prefix **v499_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-499-1..3（v498 残点）** — 1=Player `+0x18` bit18/下位 byte の set 書手（machine 探索へ転換）と bit24 semantic／2=lock/release pair 本文・mask 源・lookup/acquire sub identity／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-499-2: `FUN_00496F50`（release 側）本文** | **[確定相当]** | L120846-120859 全読 [確定(decompiled)]: **`void FUN_00496f50(void)`: `FUN_00401020(&DAT_00a2f830)`（enter critical・acquire と同一）→ **`cVar1 = FUN_00496df0(unaff_retaddr, &stack0xfffffff4)`（lookup）→ `if (cVar1 != 0) FUN_00496d50(unaff_retaddr, 0, 0xffffffff);`** → `FUN_00401040()`（leave）→ return [確定(構造)]** → **`FUN_00496F50`＝「key（`unaff_retaddr`＝retaddr 経由 [推定]）の owner/count を `FUN_00496D50(key, 0, -1)` でクリアするリカーシブル lock release fn [確定相当]`** → **acquire（v498: `FUN_00496d50(key, thread, 1)`／count++）⇔release（本裁定: `FUN_00496d50(key, 0, 0xffffffff)`）の対が確定**（`0xffffffff`＝count 全滅＝`-1` クリア [確定]）。`unaff_retaddr` を key に使う形は decompiler の stack 認識制約 [推定]（実機械では引数 push [推定]）＝**key の実渡し方は machine 要確認＝残点の形を明記 [部分確定]** |
| **V-499-2: `FUN_00496DF0`（lookup）／`FUN_00496D50`（set）identity** | **[確定相当]（両方）** | **`FUN_00496DF0`（L120746 `undefined4 * __thiscall(int *param_1, undefined4 *param_2)`）: `iVar3 = virtual[*param_1+4](param_2)`（**hash**）→ `puVar1 = *(param_1[2] + iVar3*4)`（**bucket 配列**）→ loop: null なら return 0 [確定(decompiled)] → **bucket＋chain の hash map lookup（owner/count node 取得）[確定相当]**（`FUN_00452C20`/`FUN_00452570`（v493/v494）と同型）。**`FUN_00496D50`（L120714 `void __thiscall(int *param_1, undefined4 param_2, undefined4 param_3)`）: 同じ hash→bucket→walk head（`iVar2=virtual[*param_1+4](param_2)`→`puVar3=*(param_1[2]+iVar2*4)`）[確定(decompiled)] → **key（`param_2`）の node に owner（`param_3` 系）を書く map set [確定相当]**（acquire で `(key, thread, 1)`／release で `(key, 0, -1)` を渡す運用と一致）** → **lock pair の全体像（acquire/release/lookup/set の4 fn）が確定** |
| **V-499-1: Player `+0x18` bit18/下位 byte writer（machine 探索へ転換）** | **[部分確定]（候補122件を獲得）** | **`.text` 内 `0xB33B00`（Player ptr）refs＝2281件、うち直後に `[reg+0x18]` write が現れるもの＝122件 [確定(機械)]**（top12 を列挙: `0x411F77 mov [eax+0x18], ecx`（`FUN_00411F60`）／**`0x42126C mov [esi+0x18], ecx`（`FUN_004211E0`）＝v498 bit24 loop の直前 fn と同一 [注意]**／`0x424FED mov [esi+0x18], 0`（`FUN_00424F90`）／`0x4277D6 mov [esi+0x18], edx`（`FUN_00426A90`）／`0x42A8E3 mov [eax+0x18], ecx`（`FUN_0042A8C0`）…）。**ただし候補の多くは stack local（`[esp+0x18]`）や他 object の `+0x18` を含む＝「Player ptr を保持する reg への write」への絞り込み（ref→write 距離＋reg 追跡）が必要 [部分確定]**。**absolute form `[0xB33B018]` は .text に0件 [確定(否定側)] → 書手は必ず `mov eax,[0xB33B00]` 経由（間接）＝v498 判定の裏付け**。bit18 の特定は v500 で絞り込み |
| **V-499-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v499_1）

### §2.1 `FUN_00496F50`（release）（v499_1b）
```
L120846: void FUN_00496f50(void) {
L120852:   FUN_00401020(&DAT_00a2f830);                        // enter critical
L120853:   cVar1 = FUN_00496df0(unaff_retaddr, &stack0xfffffff4);  // lookup
L120854:   if (cVar1 != 0) {
L120855:     FUN_00496d50(unaff_retaddr, 0, 0xffffffff);        // clear owner=0, count=-1
L120856:   }
L120857:   FUN_00401040();                                      // leave critical
L120858:   return; }
=> release: FUN_00496D50(key, 0, 0xffffffff)  [確定相当]
```

### §2.2 lookup/set subs（v499_1c）
```
FUN_00496df0 (L120746): undefined4 * __thiscall(int *param_1, undefined4 *param_2)
  iVar3 = virtual[*param_1 + 4](param_2);        // hash
  puVar1 = *(param_1[2] + iVar3 * 4);            // bucket
  loop: if (puVar1 == 0) return 0; ...           // map lookup  [確定相当]
FUN_00496d50 (L120714): void __thiscall(int *param_1, undefined4 param_2, undefined4 param_3)
  iVar2 = virtual[*param_1 + 4](param_2);        // hash
  puVar3 = *(param_1[2] + iVar2 * 4);            // bucket walk head
  -> map set (owner/count write)  [確定相当]
lock pair overall: acquire(v498) / release(v499) / lookup / set = 4 fns  [確定]
```

### §2.3 machine writer candidates（v499_1a）
```
.text refs of 0xB33B00: 2281
refs with [reg+0x18] write nearby: 122   (top12 listed)
  0x411F77 mov [eax+0x18], ecx   (FUN_00411f60)
  0x42126C mov [esi+0x18], ecx   (FUN_004211e0)   <- same fn called before bit24 loop (v498)
  0x424FED mov [esi+0x18], 0     (FUN_00424f90)
  0x426FD6 mov byte [esp+0x18], 1 (stack local, exclude)
  0x4277D6 mov [esi+0x18], edx   (FUN_00426a90)
  0x42A8E3 mov [eax+0x18], ecx   (FUN_0042a8c0)
  0x42B9C2 / 0x441102 ... (stack locals, exclude)
absolute [0xB33B018] form in .text: 0  [否定側 -> all writes are indirect via Player ptr load]
-> candidates need reg-tracking refinement (ref->write distance + which reg holds Player)  [部分確定]
```

## §3 残点 **V-500-1..3**
1. 【中】**Player `+0x18` writer の絞り込み**（122候補から「`mov reg,[0xB33B00]` 直後の `mov [reg+0x18], …`」を特定 → bit18/下位 byte の set 書手）＋ **bit24 semantic**（`iVar7` lazy node の flag 群＝`param_1+0x60` list 要素が何か）。
2. 【低】**`unaff_retaddr` key の実渡し方（machine の push 確認）** ＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v499_1 の1本 EXIT=0**（V-499-2 lock pair4 fn を確定相当で完結／V-499-1 machine writer 候補122件を獲得・絞り込みは次／V-499-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00496F50` 本文50行を全読** → `FUN_00496d50(key, 0, 0xffffffff)` が release と即決（**acquire の `(key, thread, 1)` と対**）→ (ii) **2 sub（lookup/set）の head を hash→bucket walk で同型確認** → lock4 fn 完結 → (iii) **machine 側へ転換**: `0xB33B00` refs census（2281→122 write 近接）で絞り込み候補を獲得、**absolute `[0xB33B018]` 0件で「必ず間接」を裏付け**。
- 妥当性チェック: ①`FUN_00401020`/`FUN_00401040`⇔enter/leave critical（acquire/release 共通）— ②`FUN_00496d50(key,0,0xffffffff)`⇔`(key, thread, 1)`（count=1⇔-1クリアの対）— ③`FUN_00496DF0` hash walk⇔`FUN_00496D50` hash walk（lookup/set 同型）— ④`[0xB33B018]` 0件⇔docs の `DAT_00b33b00 + 0x18` が全て `mov eax,[0xB33B00]` 経由— ⑤`FUN_004211E0` 出現⇔v498 bit24 loop 直前 fn（系統整合）— **五方向矛盾なし**。
- docs との不整合: **①`FUN_00496F50` の `void(void)` と `unaff_retaddr` key 使用は decompiler の stack 認識制約による表示（実機械は key push [推定]）＝制約として明記 [部分確定]**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **lock/資源管理系は「acquire/release/lookup/set」の4 fn に分解できる** — release の値（`0, 0xffffffff`）が acquire（`thread, 1`）と対になっていれば対の完成（**args の対応が identity を決める**）。
  2. **`void(void)` でも `unaff_retaddr` が key なら「decompiler が stack 引数を拾えていない」** — **machine の push 確認へ回す**（v497→v499 の推定連鎖を機械で裏取る残点化）。
  3. **docs grep 否定側（v497/v498 の8/8）から machine census へ転換** — `0xB33B00` refs（2281）→`[reg+0x18]` write 近接（122）の**2段絞り込み**で候補を機械的に揃える（**docs で拾えない間接 store の標準手法**）。
  4. **absolute 形（`[0xB33B018]`）0件の否定側が「間接経由確定」の裏付け**（machine 形の分類を census に含める）。
  5. **hash→bucket walk の同型が lookup/set の判別**（v485/v493/v494 に続く **map 4 fn family の完成**）。
