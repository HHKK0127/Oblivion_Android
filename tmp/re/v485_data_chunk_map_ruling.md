# v485 — `DATA` チャンク6 float 確定・setting map＝bucket hash map・chunk reader の裁定（V-485-1..3 完遂・旧残点 V-406-3/V-406-4 継続第15弾）

- 採番: **v485**（旧残点群消化・probe prefix **v485_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-485-1..3（v484 残点）** — 1=旧残点 V-406-3 継続（6 float の正式語＝FourCC 突合と chunk reader identity）／2=`DAT_00B37D00` 完全英名（`FUN_00412D30` identity）／3=V-484-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-485-1: 6 float の出所（`local_2c..local_18`）** | **[確定]** | L171393-171404 実読 [確定(decompiled)]: **`case 0x41544144`（LE 逆バイト＝`"DATA"`・`0x41544144`＝bytes `44 41 54 41`）: `if (local_9 == '\0') { virtual[*param_1 + 0x8c](0);`（**`+0x8C` predicate を mode-0 で呼ぶ＝v405/v463 slot 系 [確定]**）** → **`FUN_00450C20(&local_2c, 0x18)`（**`0x18`＝24 bytes＝6 float を block read [確定]**）→ **`param_1[0xb]=local_2c; [0xc]=local_28; [0xd]=local_24; [8]=local_20; [9]=local_1c; [10]=local_18;`（**24B を2つの3float 群へ一括格納 [確定]**）` }`** → **`"DATA"` チャンクの 6 float（24B）が `+0x2C..+0x34` と `+0x20..+0x28` の両群に入る [確定]**。**FourCC 名は LE 逆バイト解釈（dword `0x41544144` ⇒ メモリ `"DATA"`）で読む**（v484 `0x454d414e`⇔`"NAME"` と同一方式）。**6 float の semantic（何の DATA か＝transform/値 [推定]）は [未確定]＝残点** |
| **V-485-1: `FUN_004D9D00` の性格（追加）** | **[確定相当]（reference resolve 読み込み）** | 実文字列 **`"Missing object (%08X) for reference (%08X)"`（L171384・`FUN_004A7A60` へ）[確定(実文字列)]** → **`param_1[7] = iVar2`（object 参照解決結果）が0なら missing 警告** → **`FUN_004D9D00`＝「object/reference を解決しながら FourCC チャンクを読む読み込み/解決 fn [確定相当]`**（v484 の読み込み fn 判定に reference resolve を追補）。他 FourCC（LE 逆バイト）: `0x44455358`＝`"XSED"`／`0x434f4c58/59`＝`"XLOC"/"YLOC"`／`0x424c4758`＝`"XGLB"`／`0x43524d58`＝`"XMRC"` 他 [確定(実定数)]（各 case の意味は [未確定]） |
| **V-485-2: `DAT_00B37D00` 完全英名（container 構造）** | **[確定相当]（hash map 構造）＋[推定]（class 名）** | `FUN_00412D30`（init の register 先・L11094）実読 [確定(decompiled)]: **`void __thiscall FUN_00412d30(int *param_1, undefined4 param_2)`: `iVar2 = virtual[*param_1 + 4](param_2)`（**hash 計算**）→ `puVar3 = *(param_1[2] + iVar2*4)`（**bucket 配列 `param_1[2]`**）→ loop: null なら `virtual+0x14()`（**新 node alloc**）＋`virtual+0xC(node, param_2, param_2)`（**key/value 設定**）→ `*puVar3 = bucket`（先頭リンク）→ `param_1[3]++`（**count `param_1[3]`**）／`cVar1 = virtual+0x8(param_2, puVar3[1])`（**キー比較**）` [確定(構造)]` → **`FUN_00412D30`＝「bucket 配列＋chained node の hash map insert [確定相当]` → **`DAT_00B37D00`（this）＝**bucket hash map singleton（setting key 用）[推定相当→強化]**（v460 `NiTMapBase`/`NiTStringTemplateMap` 系（`[vt+0x4C]`＝`div` hash・v460 確定）と型的に整合 → **正式 class 名は `NiTMapBase` 系 [推定]**、**確定英名は [未確定]＝残点**（RTTI 判定に進むには singleton の vtable が runtime 構築のため image 内静的 vtable 法が効かない＝残点の理由を明記） |
| **V-485-3: chunk readers** | **[確定相当]＋[確定]** | **`FUN_00450C20(self, void *param_2, uint param_3)`（L61753）＝DWORD/HLOCAL locals を持つ block read（`param_3` bytes 読み込み）[確定相当]**／**`FUN_004510E0(param_1)`（L61980）＝`FUN_00450c20(param_1, 4); return;`（**4-byte read wrapper [確定]**） |
| **V-485-3: V-484-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り、sink `DAT_00b34d90` 正体 |

## §2 証拠（v485_1）

### §2.1 `"DATA"` チャンク（v485_1b）
```
L171393: if (iVar2 == 0x41544144) {            // LE bytes = "DATA"
L171394:   if (local_9 == '\0') {
L171395:     (**(code **)(*param_1 + 0x8c))(0);            // +0x8C predicate mode 0
L171396:     FUN_00450c20(&local_2c, 0x18);                // 24 bytes = 6 floats
L171397:     param_1[0xb] = local_2c; param_1[0xc] = local_28; param_1[0xd] = local_24;
L171400:     param_1[8]  = local_20; param_1[9]  = local_1c; param_1[10] = local_18;
L171404:   } goto LAB_004da021;
L171384 (ref case): FUN_004a7a60("Missing object (%08X) for reference (%08X)", local_10, param_1[3]);
other tags (LE bytes): 0x44455358 XSED / 0x434f4c58 XLOC / 0x434f4c59 YLOC /
                        0x424c4758 XGLB / 0x43524d58 XMRC / 0x434f4c58.. (see probe)
```

### §2.2 `FUN_00412D30`（v485_1a）
```
L11094: void __thiscall FUN_00412d30(int *param_1, undefined4 param_2) {
L11101:   iVar2 = virtual[*param_1 + 4](param_2);              // hash
L11102:   puVar3 = *(param_1[2] + iVar2 * 4);                  // bucket array
L11103:   while (true) {
L11104:     if (puVar3 == 0) {
L11105:       puVar3 = virtual[*param_1 + 0x14]();              // alloc node
L11106:       virtual[*param_1 + 0xc](puVar3, param_2, param_2); // set key/value
L11107:       *puVar3 = *(param_1[2] + iVar2*4);                // prepend
L11108:       *(param_1[2] + iVar2*4) = puVar3;
L11109:       param_1[3] = param_1[3] + 1;                      // count++
L11110:       return; }
L11112:     cVar1 = virtual[*param_1 + 8](param_2, puVar3[1]);   // key compare
  -> chained bucket hash map insert  [確定相当]
```

### §2.3 chunk readers（v485_1c）
```
L61753: void __thiscall FUN_00450c20(int param_1, void *param_2, uint param_3)
        (DWORD/HLOCAL locals -> block read of param_3 bytes)
L61980: void FUN_004510e0(undefined4 param_1) { FUN_00450c20(param_1, 4); return; }
        (4-byte read wrapper  [確定])
```

## §3 残点 **V-486-1..3**
1. 【中】**旧残点 V-406-3 継続** — **6 float（`"DATA"`）の semantic**（`param_1` の class が `TESObjectREFR` 系なら transform/値 [推定]・**`DATA` ケース前後の case（`local_9` flag 条件）と `+0x8C(0)` 呼び出しの意味**）＋新 FourCC（`XLOC/YLOC/XGLB/XMRC/XSED`）のインベントリ突合。
2. 【低】**`DAT_00B37D00` 確定英名** — `NiTMapBase` 系 [推定] の裏取り（singleton の vtable が runtime 構築のため **init fn 内の vtable store 探索**（`call 0x419A70` 前後の `mov [reg], imm`）が次の手）。
3. 【低】**V-485-3 引継ぎ** — `FUN_004CBA80` の `param_3`、一語名別法、predicate slot35 英名、sink 正体。

## §4 検証・自己批評
- probe: **v485_1 の1本 EXIT=0**（V-485-1 6 float 出所＝`DATA` 24B block read を確定／V-485-2 `FUN_00412D30`＝bucket hash map insert で container 構造を確定相当・英名は残点／V-485-3 readers 確定・引継ぎ残点）。
- **決定打の作り**: (i) **`FUN_004D9D00` の value-case 深部（L171382-171412）を読む** → `case 0x41544144`（`DATA`）の `FUN_00450C20(&local_2c, 0x18)`＝**`0x18`＝24B＝6 float** が決定打 → 両群セットと同時確定 → (ii) **`FUN_00412D30` 本文で bucket＋chain＋count の構造**を hash map と即決 → (iii) chunk readers は decompiled 1行（`FUN_00450C20(…,4)`）で wrapper 判明。
- 妥当性チェック: ①`0x18`＝24B⇔6 float（local_2c..local_18 の6宣言と一致）— ②`param_1[0xb..0xd]/[8..10]`⇔`+0x2C..+0x34/+0x20..+0x28`（v482/v483 と整合）— ③`virtual+0x8C(0)`⇔v405/v463 predicate slot（`NiInterpolator` 系で `+0x8C`＝slot35・**REF 系 vtable の `+0x8C` は別 vtable だが offset 一致**）— ④`param_1[2]` bucket⇔`param_1[3]` count（map 構造の定番）— ⑤`Missing object … for reference`⇔reference resolve（v484 参照解決と整合）— **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **size 引数（`0x18`）が「要素数×要素幅」を語る** — block read の size 24B⇔6 float と即決（**block read の size はデータ構造の型情報**）。
  2. **FourCC は LE 逆バイト（dword⇒メモリ文字列）で読む**（`0x41544144`⇒`"DATA"`・`0x454d414e`⇒`"NAME"`）— **v484 の解釈方式を反復**。
  3. **`case` 冒頭の `local_9` flag／`virtual+0x8C(0)` 呼び出しが「読む前の状態遷移」** — flag 付き case design（`local_9`＝error flag）を拾う。
  4. **map singleton の英名は「container 構造（bucket/chain）」で型を、RTTI は runtime 構築のため「init fn 内の vtable store」へ**（**static vtable 法が効かない singleton の対処法を明記**）。
  5. **wrapper 1行（`f(x, 4)`）が「4-byte read」と即分かる** — reader/wrapper の size 定数を読む。
