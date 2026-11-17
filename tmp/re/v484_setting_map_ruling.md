# v484 — `DAT_00B37D00`＝setting map singleton・log core・FourCC 読み込み fn の裁定（V-484-1..3 完遂・旧残点 V-406-3/V-406-4 継続第14弾）

- 採番: **v484**（旧残点群消化・probe prefix **v484_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-484-1..3（v483 残点）** — 1=旧残点 V-406-3 継続（6 float の正式語＝`FUN_004D9D00` context と `FUN_004941a0` identity）／2=旧残点 V-406-4 semantic 完遂（`DAT_00B37D00` 正体）／3=V-483-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-484-2: `DAT_00B37D00` 正体** | **[推定相当]（実文字列でほぼ確定）** | **`FUN_00419A70`（`mov ecx,0xb37d00` init 実体・L16472）実読 [確定(decompiled)]: `undefined4 * __thiscall FUN_00419a70(undefined4 *param_1, int param_2, undefined4 param_3)`: `param_1[1]=param_2; *param_1=param_3;` → **`if (param_2 != 0) { cVar1 = FUN_0055e000(param_2, &param_2); if (cVar1) { FUN_004a7a60("Setting key '%s' already used in map.\nSetting keys must be unique.\n", param_1[1], uVar2); return param_1; } FUN_00412d30(param_1[1], param_1); }` [確定(実文字列＋構造)]** → **`FUN_00419A70`＝「key 登録 map insert（`FUN_0055E000` で重複検出→重複なら `FUN_004A7A60` で警告ログ→`FUN_00412D30` で登録）[確定相当]`** → **`DAT_00B37D00`（this）＝「**Setting key の登録 map（key→value）singleton**」[推定相当]**（**実文字列 `Setting keys must be unique.` が決定打**）→ `*(class+0xc) == DAT_00B37D00` 比較（v479/v481）は「対象がこの setting map singleton を持つか」の判定 [推定]。**完全な英名（GameSettings/NiTStringTemplateMap 等の正式 class 名）は [未確定]＝残点**（ただし v460 の `NiTStringTemplateMap` 候補と型的に整合 [推定]） |
| **V-484-1: `FUN_004941a0`（log core）identity** | **[確定相当]** | L118678 実読 [確定(decompiled)]: **`void FUN_004941a0(undefined4 param_1, char *param_2, va_list param_3)`（**level/format/args 3引数**）: `char local_32cc[12996]`（**約13KB バッファ**）→ **`if ((DAT_00b34d90 == 0) \|\| (iVar2 = _vsprintf(local_32cc, param_2, param_3), iVar2 < 0)) goto …`（**`_vsprintf`＝format 展開、`DAT_00b34d90`＝出力 sink 判定）[確定(構造)]** → **`FUN_004941a0`＝「`_vsprintf` で format 展開し sink（`DAT_00b34d90`）へ出力する logging core [確定相当]`** → **`FUN_004A7A60`＝その level-1 wrapper（v483 推定を裏付け）→ NaN 修正ログはこの経路 [確定相当]**。sink（`DAT_00b34d90`）の正体は [未確定]＝残点 |
| **V-484-1: `FUN_004D9D00`（6 float コピー元）context** | **[確定相当]（読み込み fn）＋[未確定]（6 float の語）** | L171336 実読 [確定(decompiled)]: **`void __thiscall FUN_004d9d00(int *param_1, undefined4 param_2)`: `FUN_00451210(param_1)` → **FourCC チャンクループ: `iVar2 = FUN_004510b0()`（tag 読み）→ 比較ツリー（`0x454d414e`＝**`"NAME"`（LE 表記 EMAN）**／`0x4c4f5358/59`＝`"LOSX"/"LOSY"`／`0x444f4c58/59`＝`"DOLX"/"DOLY"`／`0x44475258`＝`"DGRX"`／`0x44494445`＝`"DIDE"`／`0x45535558`＝`"ESUX"` 等 [確定(実定数)]** → **`case "NAME"(EMAN): FUN_00450C20(&stack,0x200); virtual[*param_1+0xd8](&stack)`（**`+0xD8`＝v478 `"NorthMarker"` と同 slot＝SetName 系 [推定相当]**）／`case` 他: `FUN_004510E0`（値読み）→ `FUN_009832E6`（`__RTDynamicCast`）等** → **`FUN_004D9D00`＝「FourCC チャンクループでオブジェクト/データを読み込み、`virtual+0xD8`（SetName 系）へ反映する読み込み/反映 fn [確定相当]`**（`local_2c..local_18`（6 float）はこのループで読んだ値で、v483 で見た `param_1[8..10]/[0xb..0xd]` への束セットが本 fn 内）→ **6 float の正式語は FourCC 名（`DOL/DGR/LOS` 系＝配列/配列値チャンク [推定]）から「2×3D vector（例: ボーン/変形値）」だが確定は [未確定]＝残点** |
| **V-484-3: V-483-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v484_1）

### §2.1 `FUN_00419A70`（v484_1c）
```
L16472: undefined4 * __thiscall FUN_00419a70(undefined4 *param_1, int param_2, undefined4 param_3) {
L16486:   param_1[1] = param_2; *param_1 = param_3;
L16489:   if (param_2 != 0) {
L16490:     cVar1 = FUN_0055e000(param_2, &param_2);
L16491:     if (cVar1 != 0) {
L16492:       FUN_004a7a60("Setting key '%s' already used in map.\nSetting keys must be unique.\n",
L16493:                    param_1[1], uVar2);  return param_1; }
L16497:     FUN_00412d30(param_1[1], param_1);   // register
  -> key-insert with duplicate detection + warning log  [確定相当]
this = 0xB37D00 (machine: mov ecx,0xb37d00 / call 0x419a70)
```

### §2.2 `FUN_004941A0`（v484_1b）
```
L118678: void FUN_004941a0(undefined4 param_1, char *param_2, va_list param_3) {
L118685:   char local_32cc [12996];                 // ~13KB buffer
L118690:   if ((DAT_00b34d90 == 0) ||
             (iVar2 = _vsprintf(local_32cc, param_2, param_3), iVar2 < 0)) goto ...;
  -> vsprintf + sink (DAT_00b34d90)  [logging core, 確定相当]
```

### §2.3 `FUN_004D9D00` FourCC loop（v484_1a）
```
L171336: void __thiscall FUN_004d9d00(int *param_1, undefined4 param_2) {
  FUN_00451210(param_1);
  if (((uint)param_1[2] >> 5 & 1) == 0) {
    do { iVar2 = FUN_004510b0();        // chunk tag read
      if (iVar2 < 0x4c4f5359) { ... 0x4c4f5358 LOSX / 0x45535558 ESUX ...
        0x444f4c58 DOLX / 0x44475258 DGRX / 0x44494445 DIDE ...
      } else { ... 0x454d414e NAME ...
        case NAME: FUN_00450c20(&stack,0x200); virtual[*param_1 + 0xd8](&stack);
      } ...
  // later (v483): param_1[0xb..0xd] and [8..10] <= local_2c..local_1c (6 floats)
```

## §3 残点 **V-485-1..3**
1. 【中】**旧残点 V-406-3 継続** — **6 float の正式語**（FourCC 名（`DOL/DGR/LOS` 系）のインベントリ突合・`FUN_00450C20`/`FUN_004510E0`（chunk reader）の identity と値の意味）＋ `FUN_004D9D00` の残り case（`local_2c` 設定元）の特定。
2. 【中】**`DAT_00B37D00` 完全英名** — setting map singleton の正式 class（v460 `NiTStringTemplateMap` [推定] との突合＝`FUN_00412D30`（register）の identity）。
3. 【低】**V-484-3 引継ぎ** — `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）、sink `DAT_00b34d90` 正体。

## §4 検証・自己批評
- probe: **v484_1 の1本 EXIT=0**（V-484-1 log core 相当確定＋FourCC 読み込み fn 相当確定・6 float 語は残点／V-484-2 `DAT_00B37D00` を実文字列で setting map 推定相当／V-484-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00419A70`（init 実体）の docs 実読で実文字列 `"Setting keys must be unique."` を獲得** → map insert と即決 → **setting map singleton と推定相当**（**init body に潜む実文字列が identity を一撃**）→ (ii) `FUN_004941A0` の **`_vsprintf`＋13KB buffer＋sink 判定**で logging core → (iii) `FUN_004D9D00` の **FourCC 比較ツリー（`NAME`/`LOSX`/`DOLX`…）**で読み込み fn。
- 妥当性チェック: ①`FUN_004A7A60` wrapper⇔`FUN_004941A0` core（v483 推定⇔本文一致）— ②`mov ecx,0xb37d00; call 0x419a70`⇔L16472 init body（this＝singleton）— ③`FUN_0055E000`（重複検出）⇔v478 で「donor 比較で使用」の同 fn — ④FourCC `NAME`＋`virtual+0xD8`⇔v478 `"NorthMarker"` set name — ⑤6 float 束（v483）⇔本 fn 内 `local_2c..local_18` 宣言 — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **init body（singleton の構築 fn）に潜む実文字列が identity を一撃** — `"Setting keys must be unique."` で setting map singleton と即決（**v456 の error 文言法の singleton 版**）。
  2. **`_vsprintf`＋巨大ローカル buffer＋sink ポインタ判定が logging core の定番**（wrapper（`f(1,fmt,...)`）⇔core（`_vsprintf`）のペアで確定）。
  3. **FourCC 定数（`0x454d414e`＝`"NAME"` 等）を big/little 両視点で読む** — LE 表記（`EMAN`）を字列として解さず**整数をバイト分解して FourCC 名を得る**（`NAME`/`LOSX`/`DOLX`）。
  4. **vtable slot（`+0xD8`）の usage が複数裁定で一致**（v469/v478 `"NorthMarker"` ⇔ 本 fn の `NAME` ケース）＝**slot の機能（SetName 系）が cross-verified**。
  5. **init fn（`call 0x419a70`）の実体を読むことで「this singleton の中身（map）」が分かる** — init 経路の遡及（v483 の経路固定）が効く。
