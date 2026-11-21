# v625 — 範囲の排他端訂正・depth 書き込み先の確定・21 pane と名前・`<template>` 全抽出

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u82_1 / u83_1 / u84_1**（EXIT=0）／実物 XML。
判定: `[確定(命令)]` / `[確定(実物XML)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 0. 範囲の訂正（査読受理）— 以後 `[start, exclusive_end)` で書く

| 配列 | 範囲（正） | stride | 件数 | 内容 |
|---|---|---|---|---|
| int | **`+0x8F0 .. +0x930`** | 4 | 16 | hair color 値（`iHairColor00..15`） |
| char | **`+0x930 .. +0x9B0`** | 8 | 16 | hair color 名（`sHairColor0..15`） |

- v624 の `0x9AC` は誤り（最終要素は `0x930 + 15*8 = 0x9A8`、**排他端 `0x9B0`**）。
- ◪ の `0x8F4` 起点は **[確定(命令)（negative）] 否定**（int 配列は `0x8F0` 起点）。

---

## 1. depth の書き込み先 = **ebx（XML から読み込んだ menu root tile）** [確定(命令)]

`0x5CA010` 冒頭の追跡:
```
0x5CA04F call 0x582160(0,1)           ; InterfaceManager singleton → esi
0x5CA062 mov ecx, [esi+0x68]          ; = menuRoot
0x5CA065 push 0xA6973C                ; ★ "Data\Menus\CharGen\race_sex_menu.xml"
0x5CA06A call 0x590420                ; ★ XML ローダ → eax
0x5CA06F mov ebx, eax                 ; ★ ebx = 読み込んだ root tile
0x5CA077 call 0x5898F0                ; GetContainingMenu(ebx) → esi（= menu オブジェクト）
0x5CA133..0x5CA149                    ; esi+0x28/0x2c/0x30/0x3c が全て非 0 なら続行
0x5CA15D push 0xfa5 ; mov ecx,ebx ; call 0x588BD0   ; GetFloat(ebx, stackingtype)
0x5CA18F fld [esp+0x14]               ; = 0x588890() の戻り（0x5CA059 で fstp）
0x5CA197 push 0xfab ; mov ecx,ebx ; call 0x58CEB0   ; ★ SetFloat(ebx, depth, 値)
```
- **`ecx = ebx` ＝ menuRoot（XML を読み込んだ root tile）**。`esi`（menuRoot）ではない。
- `0x590420(menuRoot, path)` = **XML ローダ**（`0x58DBD0` 解析 → `0x590330` 構築 → `0x5898F0` GetContainingMenu）。
  パスは `"Data\Menus\CharGen\race_sex_menu.xml"`（`0xA6973C`）。callers 49。
- → **depth は root tile に 1 回だけ書かれる**（tile ごとのコピーではない）。値は `0x588890()`。
  **「stackingtype を見て root の depth を決める」z オーダーの後処理**で確定。chargen 固有の意味は無い。

---

## 2. V-624-1 決着: 21 pane と名前 [確定(命令)]

builder は各 pane の直前に **名前グローバル**を読み、`0x5C4340` で pane を作る。`callers_of(0x5C4340)=21` の順:

| # | 生成 site | 名前 global | pane 名 |
|---|---|---|---|
| 1 | `0x5CA448` | `0xB38F70` | **Main** |
| 2 | `0x5CA5A4` | `0xB38F78` | **Race** |
| 3 | `0x5CA62D` | `0xB38F88` | **Hair** |
| 4 | `0x5CA726` | `0xB38F80` | **Face** |
| 5 | `0x5CA85B` | `0xB38FE0` | **Shape** |
| 6 | `0x5CAAE4` | `0xB38FE8` | **Tone** |
| 7 | `0x5CACA1` | `0xB39000` | **GeneralFace** |
| 8 | `0x5CAD84` | `0xB39008` | **Brow** |
| 9 | `0x5CAE1B` | `0xB39010` | **Cheeks** |
| 10 | `0x5CAEFE` | `0xB39018` | **Chin** |
| 11 | `0x5CB02D` | `0xB38F90` | **Eyes** |
| 12 | `0x5CB0EA` | `0xB39020` | **Forehead** |
| 13 | `0x5CB181` | `0xB39028` | **Jaw** |
| 14 | `0x5CB23E` | `0xB39030` | **Mouth** |
| 15 | `0x5CB3DF` | `0xB39038` | **Nose** |
| 16 | `0x5CB618` | `0xB39040` | **GeneralSkin** |
| 17 | `0x5CB6FB` | `0xB39050` | **Beard** |
| 18 | `0x5CB819` | `0xB39320` | **EyesTex** |
| 19 | `0x5CB937` | `0xB39058` | **Eyebrows** |
| 20 | `0x5CBA7E` | `0xB39060` | **MouthLips** |
| 21 | `0x5CBB21` | `0xB39328` | **NoseTex** |

- 各 pane の直前に `call 0x5C93F0`（`0x5C4800`/`0x5C4480` も）で **pane の項目を構築**。親は全 pane 共通で `[esi+0x28]`（window）。
- **pane の名前は XML のラベル名（Main/Race/Hair/Face/…）と一致**。＝ V-624-1 の「pane → 名前」はこれで確定。
- **21 pane ≠ 21 ページ**。`0x5CDEF0` が読む 12 サイトは **Hair(7)/Race(2)/Main(3)** の要素のみ（v622）。
  → 残り 9 pane（Face/Shape/Tone/GeneralFace/Brow/Cheeks/Chin/Forehead/Jaw/Mouth/Nose/GeneralSkin/Beard/EyesTex/Eyebrows/MouthLips/NoseTex のうち未参照分）は
  **サブ pane / ページでない pane**。**ページ扱いは 12 が指す pane のみ**（V-625-2）。

---

## 3. `<template>` 全抽出 [確定(実物XML)]

`race_sex_menu.xml` の `<template>` は **5 個**:

| template | 内包要素 | `<id>` |
|---|---|---|
| `race_template_pane` | `race_pane` (rect) | **100** |
| `race_template_button` | `race_template_button_layout` (rect) / `_label` / `_text` / `_marker` / `_marker_2` | **101** |
| `race_template_toggle` | `race_template_toggle` (rect) / `_label` / `_left` / `_value` / `_right` / `_marker` / `_marker_2` | **102**（left/right は `&generic;`） |
| `race_template_slider` | `race_template_slider` (rect) / `_label` / `_bar` / `_marker` / `_marker_2` | **103**（bar は `&generic;`） |
| `race_template_text` | `race_template_text` (text) | **104** |

- **各ページのラベル/スライダ/ボタンは template 側**にある（`<id>` だけでは構造が取れない）。抽出は `<template>` 単位で行う（査読 §3 の指示どおり）。

---

## 4. user0 は tile クラス依存 [確定(命令)]

| tile クラス | user0 の意味 | 根拠 |
|---|---|---|
| menuRoot | **bool（2.0）** | v618/v619（`menuRoot.user0 == 2.0`） |
| pane / button | **id** | `0x5C4340`（`SetFloat(tile,0xFAE,id)`） |

- **V-623-3 で 155 の書き手を絞るときは、tile クラスが pane/button のものだけ**に限定する。
  クラス判定は vtable slot 3（`GetTypeID`）で `0x389`（TileMenu）を見る（査読 §5）。

---

## 5. id == -1 の自動採番 [部分確定]

- `0x5C4340` は `ret 0x14`（5 引数）。呼出側は `push edi; push edi` と `0x419B10` で BSStringT を積み、
  `push [esi+0x28]` を追加してから呼ぶ。**明示的な id 即値は見えない** → **全 pane が id==-1 で自動採番**の可能性が高い。
- その場合 **pane id は実行時依存**（counter `[0xB3B5D4]`）になり、XML の `<id>` とは一致しない。
  → **どの pane が -1 かを V-625-1 で明示**する（`0x5C4340` の arg 完全同定）。

---

## 6. 残点

- **V-625-1【中】**: `0x5C4340` の 5 引数の完全同定と、id が -1 か即値かの確定。
- **V-625-2【中】**: `0x5CDEF0` の 12 サイトが指す pane id の特定（21 のうちどれがページか）。
- **V-625-3【中】**: `menu+0x94`（pane id→tile 表）の読み手（`0x5C64E0`/`0x5C655C`/`0x5C6C59`/`0x5C6D57`）＝ pane 選択の実装。
- **V-625-4【低】**: `0x5C93F0`/`0x5C4800`/`0x5C4480`（pane 項目ビルダ）の同定。
- （継続）V-623-3（pane/button クラスに限定した 155 の user0 書き手）。

---

## 7. 検証

- probe **u82_1**（`0x5CA197` の `ecx`＝`ebx`／21 呼出サイトの名前グローバル／`<template>` 5 個）。
- probe **u83_1**（builder 冒頭で `ebx = 0x590420(menuRoot, path)`、`0x5C4340` の `ret 0x14`）。
- probe **u84_1**（`0x590420` = XML ローダ、`0xA6973C = "Data\Menus\CharGen\race_sex_menu.xml"`、callers 49）。
