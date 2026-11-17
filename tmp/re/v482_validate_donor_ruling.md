# v482 — `FUN_004DF820` validate fn・`+0x28` 3float 群・`cell+0x104` donor＝`+0x7C` の裁定（V-482-1..3 完遂・旧残点 V-406-3/V-406-4 継続第12弾）

- 採番: **v482**（旧残点群消化・probe prefix **v482_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-482-1..3（v481 残点）** — 1=旧残点 V-406-3 継続（`FUN_004DF820` 本文 identity と `+0x28` 正式 field 名）／2=旧残点 V-406-4 semantic 完遂（`DAT_00B37D00` 正体と donor 入手元）／3=V-481-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-482-1: `FUN_004DF820`（REFR `+0x6C` impl・walk が各 elem に呼ぶ fn）本文** | **[確定相当]（機能）＋[推定]（一語名）** | L175826 実読 [確定(decompiled)]: **`void __fastcall FUN_004df820(int *param_1)`: `if (((uint)param_1[2] >> 3 & 1) == 0)`（**flag bit3 gate＝v474 walk と同一条件**）→ `DAT_00b06b18=1; DAT_00b34d88=0;` → **`if ((param_1[7]!=0) && ((*(uint*)(param_1[7]+8) >> 5 & 1)!=0)) virtual[*param_1 + 0x8c]()`（`param_1[7]`＝`+0x1C` field・**`+0x8C` virtual call＝v405/v463 predicate slot 系**）** → **float 3つ組検証①: `param_1[0xb]`（`+0x2C`）／`[0xc]`（`+0x30`）／`[0xd]`（`+0x34`）を `FUN_00983ffd`（==0）＋`__isnan` で検査 → 異常時 `param_1[0xb]=DAT_00b3f9a8; [0xc]=DAT_00b3f9ac; [0xd]=DAT_00b3f9b0`（3デフォルトで補正）** ／**float 3つ組検証②: `param_1[8]`（`+0x20`）／`[9]`（`+0x24`）／`[10]`（`+0x28`）同様の検査→同じ3デフォルト補正 [確定(decompiled)]** → **`FUN_004DF820`＝「`TESObjectREFR` の `+0x6C` method: flag bit3 未立のとき float 3つ組×2 を NaN/0 検証し異常ならデフォルト補正、`param_1[7]` flag 条件付きで `+0x8C` predicate を呼ぶ validate/repair fn [確定相当]`**（一語名は [推定]＝`Validate/Repair/Refresh` 系） |
| **V-482-1: `elem[+0x28]` の正式位置づけ（3float 群）** | **[確定]（構造）＋[推定高]（語）** | **`+0x28`（＝`param_1[10]`）は `param_1[8]/[9]/[10]`（byte `+0x20/+0x24/+0x28`）の3 float 群の3番目 [確定(decompiled)]**、**デフォルト `DAT_00b3f9b0` で補正 [確定]**。**別の3つ組 `+0x2C/+0x30/+0x34`（`[0xb..0xd]`）が同じ3デフォルト（`DAT_00b3f9a8/9ac/9b0`）で補正＝2つの3 float 群が対で存在 [確定]**。`FUN_004DD070`（L174089 `__thiscall(int, undefined4*, float)`: **`param_3 = *(float*)(param_1+0x28) + param_3;` → `0 <= param_3` と `_DAT_00A3D5B0` 上限で clamp [確定(decompiled)]**）→ **`+0x28`＝累積可能な float（上限 clamp 付き）[推定高]**（`+0x20..+0x28` と `+0x2C..+0x34` の3float 群が **位置/スケール系 [推定]**、正式な field 名（何の3軸か）は [未確定]＝残点） |
| **V-482-2: donor（`TESObjectCELL+0x104`）の入手元** | **[確定相当]** | **CELL band（0x4A0000..0x4C8000・678 fn）の `+0x104) =` 代入を census [確定(decompiled)]: `L137167 (FUN_004AD630@0x4AD630): *(param_2 + 0x104) = *(param_1 + 0x7c);` ＝「`param_1` の `+0x7C` を `param_2` の `+0x104` へコピー [確定(構造)]** → **`cell+0x104`（donor field・v477）の donor は `+0x7C` field の object [推定高→確定相当]**（v448 ctor で `+0x78/+0x7C` smart ptr（v457/v458 manager で見た型）と整合）。他 census 件は `FUN_004C7A30`（巨大 annotation＝v437 類型の owning 誤り）由来で `param_1[0x41]` は type 依存（v478 と整合）→ **否定側として整理 [確定]**。**`DAT_00B37D00` 正体は残点**（経路は `cell+0x7C` → `+0x104` → `+0xc` と確定・残は singleton 自身の正体） |
| **V-482-3: V-481-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v482_1）

### §2.1 `FUN_004DF820` body head（v482_1a）
```
L175826: void __fastcall FUN_004df820(int *param_1) {
L175840:   uVar8 = DAT_00b34d88; uVar2 = DAT_00b06b18;
L175842:   if (((uint)param_1[2] >> 3 & 1) == 0) {            // flag bit3 (== walk gate)
L175843:     DAT_00b06b18 = 1; DAT_00b34d88 = 0;
L175845:     if ((param_1[7] != 0) && ((*(uint*)(param_1[7]+8) >> 5 & 1) != 0))
L175846:       (**(code **)(*param_1 + 0x8c))();              // +0x8C predicate call
L175848:     // group A: param_1[0xb] (+0x2C) / [0xc] (+0x30) / [0xd] (+0x34)
L175848-57: FUN_00983ffd(...==0) x3 + __isnan(...) x3
             -> param_1[0xb]=DAT_00b3f9a8; [0xc]=DAT_00b3f9ac; [0xd]=DAT_00b3f9b0
L175859-68: // group B: param_1[8] (+0x20) / [9] (+0x24) / [10] (+0x28)
             FUN_00983ffd / __isnan x3
             -> param_1[8]=DAT_00b3f9a8; [9]=DAT_00b3f9ac; [10]=DAT_00b3f9b0
```

### §2.2 `+0x28` clamp usage（v482_1b）
```
L174089: undefined4 * __thiscall FUN_004dd070(int param_1, undefined4 *param_2, float param_3) {
L174096:   param_3 = *(float *)(param_1 + 0x28) + param_3;
L174097:   if (0.0 <= param_3) {
L174098:     if ((float)_DAT_00a3d5b0 < param_3 ...) clamp via FUN_00982bfa(); }
L174577 (FUN_004ddcb0): iVar2 = *(int *)(param_1 + 0x28);   // int view too
L172829 (FUN_004da4f0): FUN_006faee0((float)*(ushort*)(param_3+3)/(float)_DAT_00a2fc70
                                    + (float)param_1[10]);
```

### §2.3 donor `+0x7C -> +0x104`（v482_1c）
```
band 0x4A0000..0x4C8000 (678 fns): `+ 0x104) =` / `[0x41] =` census
L137167 (FUN_004AD630@0x4AD630): *(param_2 + 0x104) = *(param_1 + 0x7c);
  -> +0x7C object copied into +0x104  [確定(構造)]
other hits owned by FUN_004C7A30 (huge annotation, v437-type owning error):
  param_1[0x41] copies/zeros = type-dependent (v478 consistent)
```

## §3 残点 **V-483-1..3**
1. 【中】**旧残点 V-406-3 継続** — **3 float 群（`+0x20..+0x28` と `+0x2C..+0x34`）の正式な field 語**（何の3軸か：位置/スケール/色 [推定] の切り分け）＋ **`FUN_004A7A60`（NaN 補正ログ/処理）の identity**。
2. 【低】**旧残点 V-406-4 semantic 完遂** — **`DAT_00B37D00` 正体**（`cell+0x7C` → `+0x104` → `+0xc` 経路確定・**残は singleton 自身**）→ `+0x64` 中身の語。
3. 【低】**V-482-3 引継ぎ** — `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v482_1 の1本 EXIT=0**（V-482-1 `FUN_004DF820` 本文＝validate/repair 相当を確定相当・`+0x28` を3float 群の一部として構造確定／V-482-2 donor を `+0x7C` で確定相当・正体は残点／V-482-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_004DF820` の decompiled head 45行を読む** → **flag bit3 gate が walk（v474）と同一**＝「walk が呼ぶ fn」と構造一致 → float 3つ組×2 の検証/補正が body 主部 → validate/repair 相当と確定 → (ii) **`+0x28` が `param_1[10]`＝3float 群 B の一部**と判明し、**デフォルト3点セット（`DAT_00b3f9a8/9ac/9b0`）が group A/B 共通**で対であることを確認 → (iii) **band census の `*(param_2+0x104)=*(param_1+0x7c)` の1行**で donor 入手元を確定。
- 妥当性チェック: ①flag bit3 gate⇔v474 walk `piVar8[2]>>3 & 1`（同一 fn 内の対）— ②`param_1[7]`＝`+0x1C`⇔v480 REFR `+0x1C`（linked ptr）— ③`+0x8C` call⇔v405/v463 predicate slot（`+0x8C`＝slot35・vtable slot と vtable offset の対応は別物だが `NiInterpolator` 系で確認済み）— ④`DAT_00b3f9a8/9ac/9b0`⇔3デフォルトの連番（group A/B 共通）— ⑤`*(param_1+0x7c)`⇔`cell+0x7C` donor（v448 `+0x78/+0x7C` smart ptr 系と整合）— **五方向矛盾なし**。
- docs との不整合: **①CELL band census の多件が `FUN_004C7A30`（巨大 annotation）に owned されている＝v437 annotation 欠落型の owning 誤りを再確認**（`param_1[0x41]` 件群は type 依存＝v478 と整合）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **walk gate と callee gate の一致（flag bit3）が「walk→fn」接続の構造証拠**（v474 で見た `>>3 & 1` が `FUN_004DF820` 内にも同形）。
  2. **NaN/0 検証＋`DAT_` 連番デフォルト補正が「float 3float 群（3軸）」の目印** — **デフォルト3点セットが共通なら群が対**（`+0x20..+0x28` と `+0x2C..+0x34`）。
  3. **`param_1[N]` index 表記の `[10]`/`[0xb]` を byte offset に翻訳（`×4`）**して field 位置を対応させる（`[10]`＝`+0x28`）。
  4. **band census の1行コピー代入（`*(dst+X)=*(src+Y)`）が field 経路（donor）を即確定**（`+0x7C→+0x104`）。
  5. **巨大 annotation（`FUN_004C7A30`）が ownership を偽る**（678 fn band で多数件が同一 annotation）→ **owning は fn 境界（int3/annotation 順）で再確認する習慣**（v437 教訓の再現）。
