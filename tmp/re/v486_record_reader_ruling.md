# v486 — record 読み込み fn（EDID/NAME/DATA）・owning 誤り訂正・log 証拠なしの裁定（V-486-1..3 完遂・旧残点 V-406-3/V-406-4 継続第16弾）

- 採番: **v486**（旧残点群消化・probe prefix **v486_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-486-1..3（v485 残点）** — 1=旧残点 V-406-3 継続（`DATA` semantic＝case 前後と `+0x8C(0)` 呼び出しの意味、FourCC インベントリ）／2=`DAT_00B37D00` 確定英名（init fn 内 vtable store 探索）／3=V-485-3 引継ぎ（`FUN_005222D0` 一語名の text evidence）を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-486-1: `FUN_004D9D00` の対象フォーマット（`DATA` semantic の前提）** | **[確定相当]（record 読み込み）** | dispatch 深部実読 [確定(decompiled)]: **FourCC を LE メモリ文字列で読むと — `case 0x44494445`＝**`"EDID"`**（メモリ `45 44 49 44`）: `FUN_00450C20(&stack,0x200)`（512B 読み）→ **`virtual[*param_1+0xd8](&stack)`（SetEditorID/SetName 系）**／**`case 0x454d414e`＝`"NAME"`**: `FUN_004510E0(&local_10)`（**4B＝reference form id 読み**）→ `FUN_0046BB20` → `FUN_0055E000` → **`FUN_009832E6(local_14, 0, &PTR_PTR_00b02f9c(＝TESForm), &PTR_PTR_00b03248, 0)`（`__RTDynamicCast`: TESForm→`0xB03248` class）→ `param_1[7]=iVar2`（解決 form）**、null なら **`"Missing object (%08X) for reference (%08X)"` [確定(実文字列)]**／**`case 0x41544144`＝`"DATA"`**: `virtual+0x8C(0)` → `FUN_00450C20(&local_2c, 0x18)`（24B＝6 float）→両3float 群** → **`FUN_004D9D00`＝「`EDID`/`NAME`/`DATA` 等の subrecord を読む record/データ読み込み fn（`param_1`＝TESForm 系 form・`param_1[7]`＝reference 解決結果）[確定相当]**（Bethesda record 系 FourCC と一致）→ **6 float ＝ `"DATA"` subrecord の 24B 値 [確定相当]**、**何の record の DATA か（値の語）は [未確定]＝残点**（`local_9`＝missing error flag、`+0x8C(0)` は読込前 mode 設定 [推定]）。**新 FourCC（LE メモリ文字列）: `XLOC`（`0x434f4c58`）／`YLOC`（`0x434f4c59`）…各 case 意味は [未確定]** |
| **V-486-2: `DAT_00B37D00` init の実行位置（owning 訂正）** | **[確定]（訂正）＋[未確定]（英名）** | **machine 実読 [確定(機械)]: `FUN_009E62B0`（0x9E62B0..0x9E6304）は `push 0xb11e34` → **`mov ecx,0xb07bf0`（**`FUN_004040E0` の this＝`0xB07BF0`**）→ `call 0x4040e0`（once-init guard）→ `push 0xa1d4b0; call 0x981fb4`（atexit 登録）→ `ret`（**`DAT_00B37D00` は登場しない**）** → **v478 ruling の「`FUN_009E62B0` 内 `0x9EDEAA: mov ecx,0xb37d00…`」表記は **annotation bisect の owning 誤り（`0x9EDEAA` は `FUN_009E62B0` の range（0x9E62B0-0x9E6304）外）→ 本裁定で訂正 [確定]`**。**`DAT_00B37D00` init（`mov ecx,0xb37d00; call 0x419a70`＋`push 0xA20120; call 0x981FB4`）の実行位置は `0x9EDEAA` 帯で、docs annotation が無い（v437 annotation gap）→ owning fn 未特定 [確定（否定側）]**。`FUN_009E62B0` は **隣接 singleton init 群（`0x9E6310: mov ecx,0xb366e0`／`0x9E6340: mov ecx,0xb366e8`…）が int3 で分断された連続 init 帯 [確定(機械)]** → **この帯の docs annotation が1つ（`FUN_009E62B0`）しかない＝annotation 稀疏帯 [確定]**。**`DAT_00B37D00` の確定英名は [未確定]＝残点**（vtable store 探索も本帯では `mov [reg], imm` が見当たらず＝static vtable でなく runtime 構築の v460/v478 判定と整合） |
| **V-486-3: `FUN_005222D0` 一語名（text evidence）** | **[確定]（否定側）** | **body（L216747..L216927）に `FUN_004a7a60` 呼び出し・format 文字列ともに 0件 [確定(否定側)]** → **一語名は error/log 文言から付与不能（v463 に続く否定側の再確認）＝残点**（機能名（class 属性→derived AV 初期適用）は v462 で確定相当） |
| **V-486-3: predicate slot35 英名** | **[未確定]（残点継続）** | `CanResolveDependencies` [推定] の裏取りは本 probe 未着手 |

## §2 証拠（v486_1）

### §2.1 `FUN_004D9D00` dispatch（v486_1a）
```
sig: void __thiscall FUN_004d9d00(int *param_1, undefined4 param_2)
L171360: if (((uint)param_1[2] >> 5 & 1) == 0) {        // flag gate
L171362:   do { iVar2 = FUN_004510b0(); ...             // subrecord tag
L171371: case 0x44494445 ("EDID"): FUN_00450c20(&stack,0x200); virtual[*param_1+0xd8](&stack);
L171376: case 0x454d414e ("NAME"): FUN_004510e0(&local_10) -> FUN_0046bb20 ->
          FUN_0055e000 -> FUN_009832e6(id,0,&TESForm,&0xB03248,0) -> param_1[7]=form
          null: FUN_004a7a60("Missing object (%08X) for reference (%08X)", ...); local_9=1;
L171393: case 0x41544144 ("DATA"): virtual[*param_1+0x8c](0); FUN_00450c20(&local_2c,0x18);
          param_1[0xb..0xd] / [8..10] <= local_2c..local_18  (24B -> 6 floats)
```

### §2.2 `FUN_009E62B0` machine (v486_1b)
```
9E62B0..9E6303: push 0xb11e34 / mov ecx,0xb07bf0 / call 0x4040e0 (once-init guard)
                 / push 0xa1d4b0 / call 0x981fb4 (atexit) / ret
                 -> DAT_00B37D00 NOT in this fn  (v478 claim corrected)
9E6310: fld [0xa2fe7c] / push 0xa57310 / mov ecx,0xb366e0 / call 0x41bae0  (next singleton)
9E6340: fld [0xa379cc] / push 0xa57328 / mov ecx,0xb366e8 / call 0x41bae0
docs: only one annotation FUN_009E62B0 (L1031387) covers 0x9E62B0..0x9E6304 -> sparse band
0x9EDEAA (mov ecx,0xb37d00 / call 0x419a70 / push 0xa20120 / call 0x981fb4):
  owning unknown (docs annotation gap)  [訂正: v478 "FUN_009e62b0 内" は bisect 誤り]
```

### §2.3 `FUN_005222D0` log evidence (v486_1c)
```
FUN_004a7a60 / format-string hits in body: 0   [否定側]
```

## §3 残点 **V-487-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`"DATA"` 24B（6 float）が何の record の値か**（`param_1`（TESForm 系）の record 種別＝`FUN_004D9D00` の呼び出し元（record read fn）特定）＋ `+0x8C(0)`（読込前 mode 設定）の意味。
2. 【低】**`DAT_00B37D00` 確定英名** — `0x9EDEAA` 帯の owning fn（annotation gap の機械境界（int3）特定）→ init fn 内の vtable store／dtor（`LAB_00A1D4B0`）逆引き。
3. 【低】**V-486-3 引継ぎ** — predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）、`FUN_004CBA80` `param_3`、sink `DAT_00b34d90` 正体、新 FourCC（`XLOC/YLOC` 等）意味。

## §4 検証・自己批評
- probe: **v486_1 の1本 EXIT=0**（V-486-1 record 読み込み fn を確定相当（EDID/NAME/DATA と一致）／V-486-2 **v478 owning 誤りを訂正**し init 位置を確定・英名は残点／V-486-3 log evidence 否定側）。
- **決定打の作り**: (i) **FourCC を LE メモリ文字列で読むと `EDID`/`NAME`/`DATA` が揃い record フォーマットと即一致** → `virtual+0xD8`＝SetName（EDID）、`NAME`＝reference id（TESForm cast）、`DATA`＝24B → **fn の対象が record と確定相当** → (ii) **`FUN_009E62B0` の machine 全体（int3 まで）を読む**ことで `DAT_00B37D00` が**この fn に無い**ことを発見 → v478 表記の訂正 → (iii) log evidence 0件の否定側。
- 妥当性チェック: ①`EDID`＋`0x200`（512B＝EditorID 文字列バッファ）⇔`virtual+0xD8`（SetName・v478 "NorthMarker" と同 slot）— ②`NAME`＋`0xB02F9C`（TESForm）⇔reference id cast（v474 で `PTR_PTR_00b02f9c` と同物）— ③`DATA`＋`0x18`⇔6 float（v485 確定）— ④`0x9EDEAA` が `FUN_009E62B0`（`ret` at `0x9E6303`）外⇔owning 訂正 — ⑤log 0件⇔v463 否定側と整合 — **五方向矛盾なし**。
- docs との不整合: **①v478 ruling「`DAT_00B37D00` init は `FUN_009E62B0` 内 `0x9EDEAA`」→ 本裁定で owning 誤りを訂正（annotation bisect が range 外を own）[確定]／②v463 ruling「`FUN_005222D0` 一語名＝docs 名前無し」→ log evidence 0件で再確認（訂正なし・否定側強化）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **FourCC は LE メモリ文字列で読むとフォーマットが分かる**（`EDID`/`NAME`/`DATA` が record 系と即一致＝**v484/v485 の解釈を確定させる**）。
  2. **owning（bisect annotation）は fn 境界（`ret`/int3）で必ず裏取る** — `0x9EDEAA` が `FUN_009E62B0`（`ret` at `0x9E6303`）の range 外なのに own された（**v478 の誤りを機械境界で検出**＝bisect owning の限界）。
  3. **1 annotation が巨大帯を覆う（sparse band）と「fn が int3 で分断された連続 init 群」かもしれない**（`0x9E6310`/`0x9E6340` の別 singleton init）→ **annotation gap は fn 境界探索へ**。
  4. **`virtual+0xD8`（SetName）と `EDID` の一致で slot の意味（EditorID setter）が cross-verified**（v478 `"NorthMarker"`⇔本裁定 `EDID`）。
  5. **log/format string 0件の否定側も積み上げる**（v463→v486 で一語名 text evidence は「無い」と3回確認）。
