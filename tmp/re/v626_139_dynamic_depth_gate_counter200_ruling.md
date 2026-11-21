# v626 — 139 の位置づけ訂正・depth 演算の確定・21 サイトの引数・counter 初期値 200

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u85_1 / u86_1 / u87_1**（EXIT=0）／既存逆コンパイル `FUN_005ca010`。
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. 139 の位置づけ訂正（査読受理）

- **XML スケルトン** = `0x590420`（**ReadXML**・汎用・callers **49**）。パスはハードコード。
- **chargen の動的コンテンツ** = `0x5CA010` 内の **139 generator**（**Pane 21** / Slider 90 / Button 22 / Toggle 5 / Text 1）。
- → v619 で「139 は汎用の XML 生成側」と否定したのは不正確。**正しくは 139 は動的側、汎用なのは ReadXML**。
  **Pane 21 が §3 の 21 pane と一致**し、Slider 90 が顔/体のスライダであることとも整合する。
- **移植時の対応**: `0xA6973C` = `"Data\Menus\CharGen\race_sex_menu.xml"`（ハードコード）。
  BSA 内パスは `menus\chargen\race_sex_menu.xml`（小文字・相対）。**両者の対応表が必要**（査読 §6）。

---

## 2. depth の演算 = **stackingtype はゲート（条件）** [確定(命令)]

`0x5CA15D..0x5CA19E`（`ebx` = `0x590420` が返した root tile）:
```
0x5CA15D push 0xfa5 ; mov ecx,ebx ; call 0x588BD0   ; GetFloat(root, 0xFA5 = stackingtype)
0x5CA169 fcomp dword [0xA69770]                      ; ★ 6006.0
0x5CA171 test ah,0x44 ; jnp 0x5CA18F                 ; == 6006.0 なら depth を書く
0x5CA176 push 0xfa5 ; mov ecx,ebx ; call 0x588BD0   ; GetFloat(root, stackingtype) 再読
0x5CA182 fcomp qword [0xA69778]                      ; ★ 102.0
0x5CA18A test ah,0x44 ; jp 0x5CA1A3                  ; != 102.0 なら skip
0x5CA18F fld dword [esp+0x14]                        ; = 0x588890()（0x5CA05E で fstp）
0x5CA197 push 0xfab ; mov ecx,ebx ; call 0x58CEB0   ; ★ SetFloat(root, 0xFAB = depth, GetDepth())
```
- 定数: **`0xA69770` = 6006.0（float）**、**`0xA69778` = 102.0（double）**。
  **102.0 = `0x66` = `&no_click_past;`**（XML の `<stackingtype> &no_click_past; </stackingtype>` と一致）。
- → **`stackingtype` は depth を書くかどうかの条件**（6006.0 か 102.0 のときだけ書く）。
  **加減算は無い（オフセットではない）**が、**「無関係」でもない**。査読 §2 の指摘どおり。
- `0x588890` は **thunk（`jmp 0x584980`）**。depth の値は `0x584980` の戻り。
- → まとめ: **「stackingtype が 102(no_click_past) か 6006 のとき、root の depth に UI レイヤ基準値を 1 回設定」**。

---

## 3. 21 pane の id（V-625-1）[確定(命令)] / [部分確定]

各 pane 生成の直前に `push edi; push edi; push edx(名前グローバル); push eax([esi+0x28]=親)`。
`0x5C4340` が読む引数は **arg1 = parent（`[esp+0x34]`）** と **arg4 = id（`[esp+0x40]`）**（`ret 0x14` = 5 引数）。

- **id は `-1` のとき counter で採番**（`0x5C4385 cmp eax,-1`）。
- **counter `0xB3B5D4` の初期値は 200（`0xC8`）**: `0x5C4F3C mov dword [0xB3B5D4], 0xC8`。
  → **`id==-1` の pane は 201, 202, …** と呼出順に決まる（**ランタイム不要で静的確定**）。
- **id は明示値との混在**（逆コンパイル `ppuStack_40`）:
  観測値 = **-1 / 0 / 2 / 3 / 5 / 6 / 7 / 9 / 0xe / 0xf / 0x12 / 0x13 / 0x16 / 0x18 / 0x19 / 0x1a / 0x1b / 0x1c / 0x1e / 0x1f / 0x28 / 0x29 / 0x36**。
  （`-1` は 2 箇所で観測。値は「グループ idx」的性格で、**21 pane を一意にする連番ではない**）
- **pane 配列は `menu+0x40 + i*4`**（`puVar3[0x10..]` に格納）。`menu+0x94 + id*4` = id→tile 表。
- → **V-625-1 は「id は -1 と明示値の混在、-1 は counter 200 起点」で部分確定**。完全な (pane, id) 表は **V-626-1**。

---

## 4. V-625-2 への含意

- `0x5CDEF0` の 12 サイトは **Hair(7) / Race(2) / Main(3)** の要素のみを読む（v622）。
- 21 pane のうち **Hair / Race / Main の 3 pane だけが「ページ」**。他 18 pane は page ではない（サブ pane / 非ページ）。
- → 12 サイトの比較相手は **Hair/Race/Main pane の user0**。id は §3 の対応表から引く（**V-626-1**）。

---

## 5. 残点

- **V-626-1【中】**: 21 pane の **(index, 名前, id)** の完全表（`ppuStack_40` の全値と、-1 → 採番の対応）。
- **V-626-2【中】**: `menu+0x94`（id→tile 表）の読み手 ＝ pane 選択の実装（V-625-3）。
- **V-626-3【中】**: `0x5C93F0`/`0x5C4800`/`0x5C4480`（pane 項目ビルダ）と `<template>` の対応（V-625-4）。
- **V-626-4【低】**: `0x584980`（GetDepth 実体）の同定。
- （継続）V-623-3（pane/button クラスに限定した 155 の user0 書き手）。

---

## 6. 検証

- probe **u85_1**（depth 演算／21 サイトの push 列＝即値 0x17,2,7,0x12,0x1a,0x1e,0x28,0x36,6,5,0x13,0xf,0x18／counter 200／`0x588890`=thunk）。
- probe **u86_1**（`0x5C4340` 全 body＝arg1=parent, arg4=id／`0x419B10`=BSStringT ctor `ret 4`／定数 6006.0・102.0）。
- probe **u87_1**（逆コンパイル `FUN_005ca010` の `FUN_005c4340()` 戻り → `puVar3[0x10..]`＝`menu+0x40`／id 列）。
