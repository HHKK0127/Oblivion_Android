# v627 — 21 pane の index→名前と中身（V-626-1）＋ pane id

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: probe **u88_1**（EXIT=0）＋既存逆コンパイル `FUN_005ca010`（line 333140〜）。
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. pane 配列 = `menu+0x40 + i*4`（index → 名前）[確定(命令)]

逆コンパイルの `puVar3[0x10 + i] = FUN_005c4340(...)` の戻り、および各生成直前の名前グローバル:

| i | menu+ | 名前 | i | menu+ | 名前 |
|---|---|---|---|---|---|
| 0 | 0x40 | **Main** | 11 | 0x6C | **Forehead** |
| 1 | 0x44 | **Race** | 12 | 0x70 | **Jaw** |
| 2 | 0x48 | **Hair** | 13 | 0x74 | **Mouth** |
| 3 | 0x4C | **Face** | 14 | 0x78 | **Nose** |
| 4 | 0x50 | **Shape** | 15 | 0x7C | **GeneralSkin** |
| 5 | 0x54 | **Tone** | 16 | 0x80 | **Beard** |
| 6 | 0x58 | **GeneralFace** | 17 | 0x84 | **EyesTex** |
| 7 | 0x5C | **Brow** | 18 | 0x88 | **Eyebrows** |
| 8 | 0x60 | **Cheeks** | 19 | 0x8C | **MouthLips** |
| 9 | 0x64 | **Chin** | 20 | 0x90 | **NoseTex** |
| 10 | 0x68 | **Eyes** | | | |

- **V-626-1 の index→名前 は確定**。`menu+0x40..+0x90`（21 スロット）。

## 2. 各 pane の中身（item index → 名前）[確定(命令)]

逆コンパイルの `uStack_4 = <index>; FUN_00419b10(<名前>); FUN_005c4480(<pane>)`:

| pane | item index | 名前 |
|---|---|---|
| **Main**(0x40) | 0 / 1 / 2 | **Race / Face / Hair**（＝タブ） |
| **Face**(0x4C) | 3 / 4 / 5 / 6 | **Shape / Tone / Random / Reset** |
| **Shape**(0x50) | 7 / 8 / 9 / 10 / 0xb / 0xc / 0xd / 0xe / 0xf | **GeneralFace / Brow / Cheeks / Chin / Eyes / Forehead / Jaw / Mouth / Nose** |
| **Tone**(0x54) | 0x10 / 0x11 / 0x12 / 0x13 / 0x14 / 0x15 | **GeneralSkin / Beard / EyesTex / Eyebrows / MouthLips / NoseTex** |

- → **Main pane = タブバー**（Race/Face/Hair の 3 タブ）。Face pane = Shape/Tone/Random/Reset。
  Shape/Tone pane = 顔詳細の各スライダ。
- → **12 読みサイト（Hair/Race/Main）との整合**: `Hair`(Red/Green/Blue/Length/HairColor/Style)・`Race`(Gender)・
  `Main`(Age/Complexion/Eyes) の要素は、**pane の item ではなく別のラベル要素**。`0x5CDEF0` はこれらを読む（V-622）。

## 3. pane id（`ppuStack_40`）[部分確定]

- `FUN_005c4340` の id は `ppuStack_40`。観測値: **-1**（Race・Face）/ **0x13** / **0** / **2** / **3** / **7** …。
- **`-1` は counter `0xB3B5D4`（初期値 200）から採番**（v626）。→ **Race=201 / Face=202**（作成順）。
- 明示値（0,2,3,7,0x13,…）は **グループ idx 的性格**で、21 pane を一意にする連番ではない。
- → **(index, 名前) は確定、(id) は -1 の 2 件が確定・他は明示値**。完全な id 表は **V-627-1**。

## 4. 残点

- **V-627-1【低】**: 21 pane の id を全件確定（`ppuStack_40` の全出現を地址順に）。
- **V-626-2【中】**: `menu+0x94`（id→tile 表）の読み手 ＝ pane 選択。
- **V-626-3【中】**: `0x5C93F0`/`0x5C4480`/`0x5C4800`（item builder）と `<template>` の対応。
- （停止条件）V-626-2/3 が閉じたら chargen lane を一旦停止し、残りは人間ゲート待ちとして記録。

---

## 5. 検証

- probe **u88_1**（21 pane の名前グローバル＋直前窓）。
- 逆コンパイル `FUN_005ca010` line 333140〜（`puVar3[0x10..]`＝pane 配列、`uStack_4`＝item index、`ppuStack_40`＝id）。
