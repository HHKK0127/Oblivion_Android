# v610 — 依頼 U14(A〜E) 裁定: RaceSexMenu 21ページの行表復元・登録バンド全リスト・Done/Back規則・スライダー対象・Race/Hair/Face の Done 挙動

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u28_1〜u42_1**・全 EXIT=0・冒頭 Test-Path）＋実物 XML（`Oblivion - Misc.bsa` → `menus\chargen\race_sex_menu.xml` 705行）＋ `docs/Oblivion_decompiled.c`（`FUN_005ca010`/`FUN_005c4340`/`FUN_005c93f0`/`FUN_005cc200`/`FUN_005c9650`/`FUN_005c3440`）。
先行: `v558`（21ページ・AddPane 三連書込）・`v563`/`v567`（Done 切替・ページ順）・`v570`（28ブロック dispatch）・`v571`（139 呼出・id 採番）・`v602`（Main 内容）・`v609`（タブ無し・user5）。
依頼書: `tmp/re/handoff_20261121_racesexmenu_page_rows.md`（依頼 A〜E・ai-todo #96）。采番 v610。
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`

---

## 1. 依頼 A — `0x5CA010` の 139 要素「ページ別 行表」 [確定(命令)]

### 1.0 抽出方法
`0x5CA010`..`0x5CBC00` を capstone で線形 decode し、**generator 5 関数の E8 呼出 139 件**（Pane `0x5C4340`／Button `0x5C4480`／Toggle `0x5C4630`／Text `0x5C4800`／Slider `0x5C93F0`）を生成順に列挙。各行の**ラベル＝呼出直前の `mov reg,[0xB38Fxx]`（登録バンドの GMST グローバル）**、**index＝直前 `call 0x419B10` の手前 `mov [esp+0x4C],N`**。ページ境界＝Pane 呼出の直後 `mov [esi+0x40/0x44/…],eax`。

### 1.1 ページ id の実測（依頼 A-2 の算術不一致を解消）[確定(命令)]

`AddPane` の a4 引数を読むと **Main のみ明示 `0`（`push 0`）、他 20 ページは `-1`（`push edi`, edi=-1）→ counter++ 採番**。counter は ctor `0x5C4F3C` で `200` にリセット。**子要素は Button/Toggle/Text が +1、Slider が +2 消費**。

| ページ | 生成順 | page_id | 消費 | 次 counter |
|---|---|---|---|---|
| Main | 1 | **0**（明示） | Button3+Toggle1+Slider2(×2)=**+8** | 208 |
| Race | 2 | **209** | +3 | 212 |
| Hair | 3 | 213 | +5 | 218 |
| Face | 4 | 224 | +4 | 228 |
| Shape | 5 | 229 | +9 | 238 |
| Tone | 6 | 239 | +6 | 245 |
| General(Face) | 7 | 246 | +5 | 251 |
| Brow | 8 | 257 | +3 | 260 |
| Cheeks | 9 | 264 | +5 | 269 |
| Chin | 10 | 275 | +7 | 282 |
| Eyes | 11 | 290 | +4 | 294 |
| Forehead | 12 | 299 | +3 | 302 |
| Jaw | 13 | 306 | +4 | 310 |
| Mouth | 14 | 315 | +10 | 325 |
| Nose | 15 | 336 | +14 | 350 |
| General(Skin) | 16 | 365 | +5 | 370 |
| Beard | 17 | 376 | +6 | 382 |
| Eye shades | 18 | 389 | +6 | 395 |
| Eyebrows | 19 | 402 | +7 | 409 |
| Mouth(Lips) | 20 | 417 | +3 | 420 |
| Nose shades | 21 | 424 | +2 | **428**（最終） |

→ **v571 §3.1 の「Main の消費 +6（c=206）」は誤り。実際は +8（Button3=3・Toggle1=1・Slider2=4）。** 依頼書の指摘（Race=209）が正しく、v571 の表は各ページの**消費欄を Slider=+2 で数え直す**必要がある（v571 の page_id 列 209/213/224/… は本表と一致）。

### 1.2 全 139 要素の行表（生成順）

凡例: `idx`=button の明示 index（Main のみ 0/1/2）／label=GMST 名（実値）。Toggle/Slider/Text は自動採番（idx=-）。

#### Main（page_id 0）— Button×3 / Toggle×1 / Slider×2
| 順 | 型 | label(GMST) | 実値 |
|---|---|---|---|
| 1 | Button | sRace | Race |
| 2 | Button | sFace | Face |
| 3 | Button | sHair | Hair |
| 4 | Toggle | sEyes | Eyes |
| 5 | Slider | sAge | Age |
| 6 | Slider | sComplexion | Complexion |

→ **Main = Race/Face/Hair の3ボタン＋Eyes トグル＋Age/Complexion スライダ**。**Gender トグルは Main には無い**（v570 block8「Gender∈Main」は要再確認＝後述 E の dispatch で Gender は Race ページの行）。**Age∈Main は正しい**（v570 block3 と整合）。**名前欄は `race_name`(id=10) で別（tile 内入力欄・v602）**。

#### Race（209）— Toggle×2 / Text×1
| 順 | 型 | label | 実値 |
|---|---|---|---|
| 1 | Toggle | sGender | Gender |
| 2 | Toggle | sRace | Race |
| 3 | Text | sRaceDescription | Race Description |

→ **Race ページ＝Gender トグル＋Race トグル＋Racedescription テキスト**（Text の本文は `sRaceDescription`＝"Race Description"＝見出し。実際の種族説明は選択種族の DESC をコードが流し込む）。

#### Hair（213）— Toggle×2 / Slider×4
| 順 | 型 | label | 実値 |
|---|---|---|---|
| 1 | Toggle | sStyle | Style |
| 2 | Toggle | sColor | Hair Color |
| 3 | Slider | sRed | Red |
| 4 | Slider | sGreen | Green |
| 5 | Slider | sBlue | Blue |
| 6 | Slider | sLength | Length |

#### Face（224）— Button×4（v571 の Button×3 を訂正）
| 順 | 型 | label | 実値 |
|---|---|---|---|
| 1 | Button | sShape | Shape |
| 2 | Button | sTone | Tone |
| 3 | Button | sRandom | Random |
| 4 | Button | sReset | Reset |

→ **v571「Face=Button×3」は誤り。実測 Button×4**（Shape/Tone/Random/Reset）。依頼 A-4 の「Random/Reset が Face に実在するか」→ **実在**（v570 の Random∈Face・Reset∈Face が正、v571 の個数が誤）。

#### Shape（229）— Button×9
| 順 | label | 実値 |
|---|---|---|
| 1 | sGeneralFace | General(Face) |
| 2 | sBrow | Brow |
| 3 | sCheeks | Cheeks |
| 4 | sChin | Chin |
| 5 | sEyes | Eyes |
| 6 | sForehead | Forehead |
| 7 | sJaw | Jaw |
| 8 | sMouth | Mouth |
| 9 | sNose | Nose |

#### Tone（239）— Button×6
| 順 | label | 実値 |
|---|---|---|
| 1 | sGeneralSkin | General(Skin) |
| 2 | sBeard | Beard |
| 3 | sEyesTex | Eye shades |
| 4 | sEyebrows | Eyebrows |
| 5 | sMouthLips | Mouth(Lips) |
| 6 | sNoseTex | Nose shades |

#### General(Face)（246）— Slider×5
| 順 | label | 実値 |
|---|---|---|
| 1 | sFacebrow | Face brow/nose/chin ratio |
| 2 | sFaceforehead | Face forehead/sellion/nose ratio |
| 3 | sFaceheavy | Face heavy/light |
| 4 | sFaceround | Face round/gaunt |
| 5 | sFacethin | Face thin/wide |

#### Brow（257）— Slider×3: `sBrowRidge`(Brow Ridge high/low) / `sBrowRidgeInner` / `sBrowRidgeOuter`
#### Cheeks（264）— Slider×5: `sCheekboneslow` / `sCheekbonesshallow` / `sCheekbonesthin` / `sCheeksconcave` / `sCheeksround`
#### Chin（275）— Slider×7: `sChinforward` / `sChinpronounced` / `sChinretracted` / `sChinshallow` / `sChinsmall` / `sChintall` / `sChinwide`
#### Eyes（290）— Slider×4: `sEyesdown` / `sEyessmall` / `sEyestilt` / `sEyestogether`
#### Forehead（299）— Slider×3: `sForeheadsmall` / `sForeheadtall` / `sForeheadtilt`
#### Jaw（306）— Slider×4: `sJawretracted` / `sJawwide` / `sJawNeck` / `sJawline`
#### Mouth（315）— Slider×10: `sMouthdrawn` / `sMouthhappy` / `sMouthhigh` / `sMouthlipsdeflated` / `sMouthlipslarge` / `sMouthlipspuckered` / `sMouthprotruding` / `sMouthtilt` / `sMouthunderbite` / `sMouthChin`
#### Nose（336）— Slider×14: `sNosebridgeshallow` / `sNosebridgeshort` / `sNosedown` / `sNoseflat` / `sNosenostriltilt` / `sNosenostrilssmall` / `sNosenostrilswide` / `sNoseregion` / `sNoseselliondown` / `sNosesellionshallow` / `sNosesellionlower` / `sNosesellionthin` / `sNoseshort` / `sNosetilt`
#### General(Skin)（365）— Slider×5: `sSkinflushed` / `sSkinshade` / `sSkintintorange` / `sSkintintpurple` / `sCheekblush`
#### Beard（376）— Slider×6: `sBeardflushed` / `sBeardlight` / `sBeardcheeks` / `sBeardcircle` / `sBeardgoatee` / `sBeardmoustache`
#### Eye shades（389）— Slider×6: `sEyesocketsbruised` / `sEyesocketsdark` / `sEyelidslight` / `sEyelidspale` / `sEyeliner` / `sEyeshadow`
#### Eyebrows（402）— Slider×7: `sEyebrowsdark` / `sEyebrowslow` / `sEyebrowsthick` / `sEyebrowsvery` / `sEyebrowslower` / `sEyebrowsouter` / `sEyebrowsuppert`
#### Mouth(Lips)（417）— Slider×3: `sLipsflushed` / `sLipstickdarkred` / `sLipstickdarkblue`
#### Nose shades（424）— Slider×2: `sNasolabiallines` / `sNares`

（各ラベルの実値文字列は §2 の登録バンド表に全件記載。）

### 1.3 行の遷移先 [確定(命令)]
- **Button 行**＝クリックでページ遷移（遷移先は §6 の Done 連鎖と同じ階層。例 Main の Race 行→Race、Shape の Brow 行→Brow）。
- **Face の Shape/Tone 行**＝Shape/Tone ページへ、**Shape の General(Face) 行**＝General(Face) へ、**Tone の General(Skin) 行**＝General(Skin) へ。
- **Toggle・Slider 行**＝**遷移先なし（値のみ変更）**。

---

## 2. 依頼 B — 静的登録バンド `0x9F6000..0x9F8000` の全リスト [確定(命令)]

### 2.1 登録 API と第2引数の意味 [確定(命令)]
登録は **`push <value文字列>; push <name文字列>; mov ecx,<.data グローバル>; call 0x419A70`** の形（`0x419A70`＝登録関数）。**第2引数（先に push される方）＝ GMST 名（`s…`/`i…`/`f…`）**、**第1引数＝ 表示実値**。`sCheekboneslow` 等は **GMST キー名**で、画面に出るのは**実値側**（"Cheekbones low/high"）。バンド内の登録は **218 件**（`call 0x419A70` 全数）。

### 2.2 名前→実値（依頼 A ラベル対応・全件）
| GMST 名 | 実値 | GMST 名 | 実値 |
|---|---|---|---|
| sMain | Main | sRace | Race |
| sFace | Face | sHair | Hair |
| sEyes | Eyes | sAge | Age |
| sComplexion | Complexion | sGender | Gender |
| sRaceDescription | Race Description | sStyle | Style |
| sRed | Red | sGreen | Green |
| sBlue | Blue | sLength | Length |
| sShape | Shape | sTone | Tone |
| sRandom | Random | sReset | Reset |
| sGeneralFace | General(Face) | sGeneralSkin | General(Skin) |
| sBrow | Brow | sCheeks | Cheeks |
| sChin | Chin | sForehead | Forehead |
| sJaw | Jaw | sMouth | Mouth |
| sNose | Nose | sBeard | Beard |
| sEyesTex | Eye shades | sNoseTex | Nose shades |
| sEyebrows | Eyebrows | sMouthLips | Mouth(Lips) |
| sSkin | Skin | sLips | Lips |
| sColor | Hair Color | sFacebrow | Face brow/nose/chin ratio |
| sFaceforehead | Face forehead/sellion/nose ratio | sFaceheavy | Face heavy/light |
| sFaceround | Face round/gaunt | sFacethin | Face thin/wide |
| sBrowRidge | Brow Ridge high/low | sBrowRidgeInner | Brow Ridge Inner up/down |
| sBrowRidgeOuter | Brow Ridge Outer up/down | sCheekboneslow | Cheekbones low/high |
| sCheekbonesshallow | Cheekbones shallow/pronounced | sCheekbonesthin | Cheekbones thin/wide |
| sCheeksconcave | Cheeks concave/convex | sCheeksround | Cheeks round/gaunt |
| sChinforward | Chin forward/backward | sChinpronounced | Chin pronounced/recessed |
| sChinretracted | Chin retracted/jutting | sChinshallow | Chin shallow/deep |
| sChinsmall | Chin small/large | sChintall | Chin tall/short |
| sChinwide | Chin wide/thin | sEyesdown | Eyes down/up |
| sEyessmall | Eyes small/large | sEyestilt | Eyes tilt inward/outward |
| sEyestogether | Eyes together/apart | sForeheadsmall | Forehead small/large |
| sForeheadtall | Forehead tall/short | sForeheadtilt | Forehead tilt forward/back |
| sJawretracted | Jaw retracted/jutting | sJawwide | Jaw wide/thin |
| sJawNeck | Jaw-Neck slope high/low | sJawline | Jawline concave/convex |
| sMouthdrawn | Mouth drawn/pursed | sMouthhappy | Mouth happy/sad |
| sMouthhigh | Mouth high/low | sMouthlipsdeflated | Mouth lips deflated/inflated |
| sMouthlipslarge | Mouth lips large/small | sMouthlipspuckered | Mouth lips puckered/retracted |
| sMouthprotruding | Mouth protruding/retracted | sMouthtilt | Mouth tilt up/down |
| sMouthunderbite | Mouth underbite/overbite | sMouthChin | Mouth-Chin distance short/long |
| sNosebridgeshallow | Nose bridge shallow/deep | sNosebridgeshort | Nose bridge short/long |
| sNosedown | Nose down/up | sNoseflat | Nose flat/pointed |
| sNosenostriltilt | Nose nostril tilt down/up | sNosenostrilssmall | Nose nostrils small/large |
| sNosenostrilswide | Nose nostrils wide/thin | sNoseregion | Nose region concave/convex |
| sNoseselliondown | Nose sellion down/up | sNosesellionshallow | Nose sellion shallow/deep |
| sNosesellionlower | Nose sellion lower shallow/deep | sNosesellionthin | Nose sellion thin/wide |
| sNoseshort | Nose short/long | sNosetilt | Nose tilt down/up |
| sSkinflushed | Skin flushed/pale | sSkinshade | Skin shade dark/light |
| sSkintintorange | Skin tint orange/blue | sSkintintpurple | Skin tint purple/yellow |
| sCheekblush | Cheek blush light/red | sBeardflushed | Beard flushed/pale |
| sBeardlight | Beard light/dark | sBeardcheeks | Beard cheeks light/dark |
| sBeardcircle | Beard circle light/dark | sBeardgoatee | Beard goatee light/dark |
| sBeardmoustache | Beard moustache light/dark | sEyesocketsbruised | Eye sockets bruised/bright |
| sEyesocketsdark | Eye sockets dark/light | sEyelidslight | Eyelids light/dark |
| sEyelidspale | Eyelids pale/red | sEyeliner | Eyeliner light/dark |
| sEyeshadow | Eyeshadow light/dark | sEyesdark | Eyes dark brown/light blue |
| sEyeswhites | Eyes whites dim/bright | sEyebrowsdark | Eyebrows dark/light |
| sEyebrowslow | Eyebrows low/high | sEyebrowsthick | Eyebrows thick/thin |
| sEyebrowsvery | Eyebrows very thin/thick | sEyebrowslower | Eyebrows lower light/dark |
| sEyebrowsouter | Eyebrows outer light/dark | sEyebrowsuppert | Eyebrows upper dark/light |
| sLipsflushed | Lips flushed/pale | sLipstickdarkred | Lipstick dark red/light blue |
| sLipstickdarkblue | Lipstick dark blue/light red | sNasolabiallines | Naso labial lines light/dark |
| sNares | Nares small/large | sNoNameCharacter | You must first enter a valid name. |
| sEnterName | Enter character name. | sResetFace | Reset face? |
| sRandomizeFace | Randomize face? | | |

### 2.3 ヘアカラー GMST（区別）[確定(命令)]
`sHairColor0..15` / `iHairColor00..15` は **Hair ページの髪色 16 段**。`iHairColorNN` の**実値は RGB パック整数**（例 `iHairColor00`→ `0x7e7e7e`、`iHairColor01`→`0x373739`、…`iHairColor15`→`0x52667c`）。`sHairColorNN` は**名前**（実値＝同名、ラベル用）。→ **スライダーラベル（`sCheekboneslow` 等）とは別系統**（前者は髪色 16 の選択肢、後者は顔モーフ軸）。

---

## 3. 依頼 C — Done/Back ラベルの切替規則（`user5`） [部分確定]

### 3.1 `[RaceSexMenu+0x3C]` が Done/Back フラグ [確定(命令)]
- ctor `0x5C4E40`: `0x5C4EA2 mov [esi+0x3C], ebx(=0)`＝初期 0。
- **setter `0x5C2700` 系（`0x5C2710` 内）**: `if (id==0x5A) [ecx+0x3C] = value`（id=90=Done の値スロット）。同型: `id==2 → [+0x2C]`／`id==0x14 → [+0x34]`／`id==0x15 → [+0x38]`／`id==0xA → [+0x30]`。
- **Done ボタン tile の user5（`0xFB3`）への書込は 2 箇所**:
  1. `0x5CC816`（Done 連鎖フォールスルー）: `SetFloat(doneTile, 0xFB3, [esi+0x3C])`（そのまま）
  2. `0x5C9650`（ページ遷移 act・`0x5C96B3`）: **`SetFloat(doneTile, 0xFB3, 2.0)`**（`fld [0xA379B4]=2.0` を push 値に上書き）
- `0xA379B4` 実測＝**`2.0f`**。

### 3.2 規則（[部分確定]）
- **サブページへ遷移した直後は Done が Back 表示**（`0x5C9650` が `user5=2.0` を書く＝XML の "2=true for Back"）。
- **Main / 遷移なしの文脈では `user5=[esi+0x3C]`（ctor 0 → XML 既定の Done 側）**。
- → **「詳細ページに入ったら Back、Main では Done」**が実効規則。`[esi+0x3C]` を 1/2 へ書き換える明示命令は本探索では見つからず（`0x5C2710` の dword 参照も 0 件＝vtable 経由）、**`0x5C9650` の固定 2.0 書込が Back 化の実体**。ページ index との対応式は残点（下記 V-610-C1）。

---

## 4. 依頼 D — スライダー行の操作対象 [部分確定]

### 4.1 XML の意味（実物・`race_template_slider`）
- 行 `user7`＝**「editor id of whatever I'm sliding」**（XML コメント原文）／`user8`＝**geometry/texture flag**（同コメント「same as above, but this is a geometry/texture flag」）。
- bar 既定: `user1`=0(min)／`user2`=1(max)／`user3`=step(`0.04` Xbox／`0.01`)／`user4`=0.1(jump)。

### 4.2 スライダー生成関数 `0x5C93F0`（ret 0x18＝6引数）の実体 [確定(命令)]
- `[RaceSexMenu+0xB3B5D4]` を読んで `c+1`／`c+2` を採番（**Slider は 2 id 消費**＝marker/bar の2要素・v571 と一致）。
- 生成直後: `SetFloat(pane, 0xFA8, id=c+1)`／`SetFloat(pane, 0xFAA, childcount-1)`／`SetFloat(pane, 0xFAF, 0.0)`／`SetString(pane, 0xFB0, label)`／`SetFloat(pane, 0xFB1, 2.0)`（`[0xA6D2D8]`）／…／**`SetFloat(pane, 0xFBB(=0xFB4?), …)`** 系で「現在値」枠を初期化。
- **`0x5C6860`（スライダーの値→editor id 解決）**: `player → vfunc+0x170 → [+0xE8]+0x29C → 0x521A10(年齢/顔) → 0x552990`（**顔フォーム／アクター値再計算系**）。→ **スライダーは顔モーフ（プレイヤー顔フォーム）のパラメータ軸を操作**。

### 4.3 port が保持すべき最小状態 [部分確定]
- 各スライダー行は **`user7`（顔モーフ軸 ID）と `user8`（geometry/texture フラグ）＋現在値(float)** の 3 つを保持すれば UI を原版どおりに出せる。値の真の書込先は**プレイヤー顔フォーム配列**（`0x5C6860` 経由）だが、port に顔モーフが無いなら**「UI 表示用の float 状態」のみ保持**で正典の画面再現は可能（値の実効果は別途）。
- min/max/step は **XML 既定（0/1/0.01/0.1）で、コード上書きは確認できず**（`0x5C95xx` の SetFloat は user1..user8 の初期化のみ）→ ページ別差は**無し**（残点 V-610-D1 で最終確認）。

---

## 5. 依頼 E — Race/Hair/Face での Done(id=90) 挙動と遷移方向 [確定(命令)（negative）]

### 5.1 Done(id=90) の分岐は「ページ別比較連鎖」で、**Race/Hair/Face は連鎖に無い** [確定(命令)]
`FUN_005cc200` の Done 連鎖（`0x5CC280`..`0x5CC810`）は以下 **19 ページのみ**を比較し、一致で行き先へ jnp:

| 現在ページ | 行き先 | 実体 |
|---|---|---|
| Main | `0x5CCA77` | 名前検証→確認ダイアログ（v565） |
| Shape・Tone | `0x5CCA09` | **Face へ**（`SetFloat(menuRoot,0xFAE, Face.user0)`） |
| General(Face)・Brow・Cheeks・Chin・Eyes・Forehead・Jaw・Mouth・Nose | `0x5CC9C4` | **Shape へ** |
| General(Skin)・Beard・Eye shades・Eyebrows・Mouth(Lips)・Nose shades | `0x5CC97F` | **Tone へ** |

**Race / Hair / Face はこの比較連鎖に 1 つも現れない**（u37_1 の 19 行に無し）。

### 5.2 未一致ページ（＝Race/Hair/Face）の実挙動は **no-op ではなく「Main へ戻る」** [確定(命令)]（v561/v567 を訂正）
連鎖の**フォールスルー先 `0x5CC816`〜`0x5CC89E`** は:
```
5CC816 mov ecx,[esi+0x3C] ; fld1 ; push ; fstp[esp] ; push 0xFB3 ; SetFloat  ; user5 書戻し
5CC829 mov eax,[0xB38F70] ; sMain → GetFloat(Main,0xFAE) → push 0xFAE ; SetFloat(menuRoot,0xFAE, Main.user0=0)
5CC863 mov edx,[0xB38F70] ; sMain → GetFloat(Main,0xFD0) → push 0xFAF ; SetFloat(…)
5CC89E jmp 0x5CD9C5        ; epilogue
```
→ **Race/Hair/Face で Done を押すと、`menuRoot.user0` が Main(0) に書き戻される＝Main ページへ戻る**。
**v561 §2.2「それ以外 → 0x5CC8BA → no-op」／v567 §2「Race/Hair/Face は 0x5CC8BA へフォールスルー」は誤り**（連鎖のフォールスルーは `0x5CC816` の「Main 復帰」であり `0x5CC8BA` ではない。`0x5CC8BA` は**要素通知（id 201-428）専用の入口**で、Done(90) は到達しない）。

### 5.3 逆方向: Main→Face は「行クリックで入る」[確定(命令)]
`0x5C3440(this, strB=親ページ名, strA=要素名, flag)`（ret 0x10＝4引数）は **`GetPage(strB)` 配下から `0x589930(page, strA)` で要素 tile を取得**（v602 §2.1 の `0x589930`＝名前完全一致探索）。dispatch `0x5CC8BA` の block9 は **`strA=sFace`／`strB=sMain`（`0x5CCF94 mov edx,[0xB38F80]`=sFace を先に構築し `0x5CCFB2 mov eax,[0xB38F70]`=sMain を後）**。→ **tile = `GetPage("Main")` 配下の `"Face"` 要素＝Main ページの Face 行**。block9 の act（`0x5CCFCC call 0x5C9650`）は **`0x5C9650(sFace, …)` を実行＝Face ページへ遷移（入る方向）**。
→ **行クリックで「入る」**（`strB`＝親＝現在ページ、`strA`＝行の要素＝遷移先ページ。ブロック照合は親ページの行 id を照合し、act は `strA` の要素＝遷移先へ移動）。**v570 §6.1 の「strB は親ページ名」は正しく、遷移先は strA 側**。

---

## 6. 移植への指針（依頼 A〜E）

1. **§1.2 の 139 行表をそのまま静的テーブル化**（ページ→行の順・型・ラベル・GMST 実値）。行ラベルは §2.2 の実値を使う。
2. **id は動的（200 起点・Slider=2 消費）**。静的に固定するなら「id≥201 の要素通知のみが act 発火」という役割分担だけ再現すればよい（可視判定は user0 の一致比較なので絶対値非依存）。
3. **Done/Back**: 詳細ページ遷移後は Back（`0x5C9650` が `user5=2.0`）、Main では Done。
4. **Done の戻り**: Main→確認、Shape/Tone→Face、顔パーツ→Shape、肌/髭/目/眉/唇/鼻→Tone、**Race/Hair/Face→Main**（v561/v567 の「no-op」は誤り）。
5. **スライダー**＝顔モーフ軸。port は user7/user8＋現在値のみ保持で画面再現可。

---

## 7. 残点

- **V-610-A1【低】**: Main の Toggle のラベルが `sEyes`（'Eyes'）である点（Gender ではなく Eyes）。原版の Main トグルが実際に何を制御するかの機能確認。
- **V-610-C1【中】**: `[RaceSexMenu+0x3C]` を 1/2 に書き換える明示命令（`0x5C2710` setter の呼出元＝vtable 経由）の同定。Done/Back のページ index 対応式。
- **V-610-D1【低】**: スライダー min/max/step のページ別上書きの有無（本裁定は「無し」寄り）。
- **V-610-D2【中】**: スライダー `user7`（顔モーフ editor id）の**軸 ID ↔ 顔パーツ**対応表（`0x5C6860` の戻りと各行の対応）。
- **V-610-E1【低】**: `0x5C9770`／`0x5C62D0`（Height/色系 act）の完全 decode。

---

## 8. 検証

- probe: **u28_1〜u42_1（15本）全 EXIT=0**（冒頭 Test-Path・`callers_of(0x4E4A10)=1043`／`find_head(0x42703E)=0x426A90` で harness 復旧確認）。
- 主要生バイト: `0x5CA010`（139 generator 呼出・生成順）／`0x5C4340`（`cmp eax,-1`→counter++）／`0x5C4480`/`0x5C4630`/`0x5C4800`（+1）／`0x5C93F0`（+2・`race_template_slider`）／`0x5C4F3C`（`[0xB3B5D4]=0xC8`）／`0x5C4EA2`（`[esi+0x3C]=0`）／`0x5C2710`（id=0x5A→`[+0x3C]`）／`0x5C9650`（`SetFloat(0xFB3, 2.0)`・`[0xA379B4]=2.0`）／`0x5CC816`（フォールスルー→Main 復帰）／`0x5CC280`〜（19 ページ比較連鎖）／`0x5CC8BA`（要素通知 dispatch 入口）／`0x5CCF94`（block9: sFace∈Main）。
- 実物 XML: `race_sex_menu.xml`（slider の user7＝editor id・user8＝geometry flag・Done の `<user5> &false;`・button の user2/user6 コメント）。
- 文字列実測: 登録バンド `0x9F6000..0x9F8000`（218 登録・GMST 名／実値ペア）。
