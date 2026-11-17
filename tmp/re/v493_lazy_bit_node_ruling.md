# v493 — gated body＝lazy bit 設定・`StolenGoods` marker・thread-affinity 判定の裁定（V-493-1..3 完遂・旧残点 V-406-3/V-406-4 継続第23弾）

- 採番: **v493**（旧残点群消化・probe prefix **v493_1 の1本 EXIT=0**（初回構文エラー→修正再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-493-1..3（v492 残点）** — 1=`FUN_00452C20`（gated 実行体の本体）identity／2=`0xA38094`（第2 marker 名）と `0x4D0E74` site・`FUN_0045A500`／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-493-1: `FUN_00452C20`（gated body 本体）identity** | **[確定相当]** | L63630 実読 [確定(decompiled)]: **`uint * FUN_00452c20(uint *param_1, uint param_2)`: `uVar1 = param_1[3]`（**`+0xC`＝key ptr**）→ **`cVar2 = FUN_0055e000(uVar1, &param_1)`（key lookup・v478 と同 fn）** → `if (cVar2==0) { puVar3=FUN_00401f00(8)`（**8B node alloc**）` if(!puVar3){FUN_00452570(uVar1,0); param_1=0;} else {*puVar3=0; puVar3[1]=0; FUN_00452570(uVar1, puVar3); param_1=puVar3;} }` → **`if (param_1[1]==0) *param_1 = *param_1 \| param_2;`（**`param_2`（mask）を `*node |=` OR 設定**）` return param_1;` [確定(構造)]** → **`FUN_00452C20`＝「key（`param_1+0xC`）で lazy get-or-create（node 8B＋`FUN_00452570` 登録）し、`param_2`（bit mask）を node 先頭へ OR 設定する bit 設定 fn [確定相当]`** → **gated 実行（`FUN_0045B670`→本 fn）の実体は「対象 bit の設定 [確定相当]」＝bit5 semantic の経路が完成（何の bit5 かは node の用途 [推定]＝残点）**。**`+0xC`＝key ptr が再確認 [確定]**（v485/v481 の `+0xc` 比較系と整合） |
| **V-493-2: `DAT_00B33AAC` の名前（第2 marker）** | **[確定相当]** | **`0xA38094` の実文字列＝`"StolenGoods"` [確定(実文字列)]**（docs mentions 1＝L58077・`FUN_0044AC20` 内）／`0xA38154`＝`"NorthMarker"`（v469/v470 と一致・docs L57927）→ **`DAT_00B33AAC`＝`StolenGoods` marker singleton [確定相当]**（v470 NorthMarker に続く第2 marker・**盗品 StolenGoods オブジェクト [推定]**）。`DAT_00B33AAC` docs mentions 8件（L58065..L58078＝`FUN_0044AC20` band） |
| **V-493-2: `FUN_0045A500`（gate 内 mode 判定）** | **[確定相当]** | L71176 実読 [確定(decompiled)]: **`uint __fastcall FUN_0045a500(int param_1)`: `DVar1 = *(DWORD*)(DAT_00b33398 + 0x10)`（**登録 thread id**）→ `DVar2 = GetCurrentThreadId()` → **`if (DVar2 == DVar1) return *(param_1+0x18)` 下位（byte 部分）／`return *(uint*)(param_1+0x18) >> 0x12 & 0x1`（**bit18**）`** [確定(構造)] → **`FUN_0045A500`＝「current thread が登録 thread（`DAT_00B33398+0x10`）と一致するかで `param_1+0x18` の byte か bit18 を返す thread-affinity 判定 fn [確定相当]`（**main/critical thread 判定 [推定]**）→ gated 実行の mode gate が thread 判定であることを確定 |
| **V-493-2: `0x4D0E74` site（4push 形）** | **[未確定]（残点継続）** | 本 probe 未着手（budget は body/marker/mode に集中） |
| **V-493-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v493_1）

### §2.1 `FUN_00452C20` body（v493_1a）
```
L63630: uint * FUN_00452c20(uint *param_1, uint param_2) {
L63637:   uVar1 = param_1[3];                          // +0xC key
L63638:   cVar2 = FUN_0055e000(uVar1, &param_1);       // lookup
L63639:   if (cVar2 == 0) {
L63640:     puVar3 = (uint *)FUN_00401f00(8);          // alloc 8B node
L63646:     *puVar3 = 0; puVar3[1] = 0;
L63648:     FUN_00452570(uVar1, puVar3);               // register
L63649:     param_1 = puVar3; }
L63652:   if (param_1[1] == 0) { *param_1 = *param_1 | param_2; }   // |= mask
L63655:   return param_1; }
```

### §2.2 marker names（v493_1b）
```
0xA38094 = 'StolenGoods'      (DAT_00B33AAC name)   docs: 1 (L58077)
0xA38154 = 'NorthMarker'      (DAT_00B35EB8 name)   docs: 1 (L57927)
```

### §2.3 `FUN_0045A500`（v493_1c）
```
L71176: uint __fastcall FUN_0045a500(int param_1) {
L71182:   DVar1 = *(DWORD *)(DAT_00b33398 + 0x10);    // registered thread id
L71183:   DVar2 = GetCurrentThreadId();
L71184:   if (DVar2 == DVar1) return *(param_1 + 0x18) (low byte);
L71187:   return *(uint *)(param_1 + 0x18) >> 0x12 & 0x1;   // bit18
  -> thread-affinity check on self+0x18 bits  [確定相当]
```

## §3 残点 **V-494-1..3**
1. 【中】**node（`FUN_00452C20` の lazy node）の用途と bit5（`+0x8` `0x20`）の最終 semantic**（`FUN_00452570` の identity・node がどの flag 群か）＋ `FUN_0045A500` の `param_1+0x18` bit18/byte が何の状態か。
2. 【低】**`0x4D0E74` site（4push 形）の引数確認**（`FUN_004D0C20` 内の `FUN_004CBA80` 呼び出し）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v493_1 の1本 EXIT=0**（初回構文エラー→修正）。V-493-1 `FUN_00452C20`＝lazy get-or-create＋`|=` bit 設定を確定相当（gated 実行の実体判明）／V-493-2 `StolenGoods` marker 名と `FUN_0045A500` thread-affinity 判定を確定相当／V-493-2 `0x4D0E74` は残点／V-493-3 引継ぎ。
- **決定打の作り**: (i) **`FUN_00452C20` decompiled が「lookup→alloc→register→`|=`」の4段**で bit 設定 fn と即決（**gated 完成の最終段**）→ (ii) **`0xA38094` 実文字列 `StolenGoods`** が第2 marker identity を一撃（v470/v487 key 文言法の再現）→ (iii) **`FUN_0045A500` の `GetCurrentThreadId` 比較**が thread-affinity 判定を即決。
- 妥当性チェック: ①`param_1[3]`＝`+0xC`⇔`FUN_0055E000(uVar1, …)` lookup⇔v481/v485 の `+0xc`＝key ptr — ②`FUN_00401F00(8)`⇔8B node（`*node`＋`node[1]` 2field）— ③`StolenGoods`⇔`FUN_004B2B80` list link（v470 marker pattern）— ④`GetCurrentThreadId`⇔`DAT_00B33398+0x10`（登録 thread）⇔gate の thread gate — ⑤`>>0x12 &1`⇔bit18（`0x40000`）— **五方向矛盾なし**。
- docs との不整合: **なし**（3fn とも annotation 実在）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **gated 実行の「本体」が `|=` bit 設定なら、notify chain の意味が一気に確定**（bit5→notify→bit 設定＝**flag 管理系**と特定）。
  2. **`key → lookup → alloc → register → |=` の5段は lazy flag node の定番**（v469/v481 の map insert と同型＋bit 設定の追加）。
  3. **実文字列（`StolenGoods`/`NorthMarker`）の取得は「singleton identity」の決定打**（v470→v487→v493 で3度目の成功）。
  4. **`GetCurrentThreadId`＋global 比較は thread-affinity 判定の目印**（gate の mode が thread 由来と即分かる）。
  5. **shift 値（`>>0x12`＝bit18）から bit 位置を特定**（v473 の `0xFEFFFFFF`、v489 の `0x20` に続く bit 解釈の反復）。
