# v353 — #124 裁定: RaceSexMenu の page（user0）対応表とスライダー集合

日期: 2026-11-11（ReverseEngineering レーン・ses_e55f5e1e1ffe 継続）
対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（読み取り専用）／原本 XML `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\chargen\race_sex_menu.xml`
走査: `%TEMP%\opencode\v353_1.py .. v353_9.py`（out 同梱。v307_common 経由）

## 0. 結論（依頼①②③）

| 依頼 | 裁定 |
|---|---|
| ① page 対応表 | **[確定]**（構成・順序・可见機構）— page＝**clone ウィンドウ id を user0 に写す仕組み**（連番0..20ではない）。21 pane の spawn 順と名称が確定（下表） |
| ② スライダー集合の決定規則 | **[確定]** レース/性別で**集合は変わらない**（init 時に全 90 本を静的 spawn）。変化するのは runtime の**有効/無効（user13=0xFBB）**と FaceGen モーフ可否のみ |
| ③ toggle(id102) の用途 | **[確定]** 性別選択専用ではない — 5個のうち Gender 選択は `sGender` toggle 1個（Race ページ）。残り= Race サイクル/Style/Hair Color/Eyes の**左右クリック型セレクタ**（XML L417-430 `user0=index・clicked 左右で増減`） |

## 1. 機構 [確定]

- 要素 spawn ラッパ（FUN_005CA010 初期化から呼ばれる）:
  - `AddPane 0x5C4340`（21 callers）／`AddButton 0x5C4480`（22）／`AddToggle 0x5C4630`（5）／`AddText 0x5C4800`（1）／`AddSlider 0x5C93F0`（90）
  - いずれも `0x585410(親, テンプレート名文字列, 0)` で `race_template_*` を clone → trait 設定 → 親 `[+0x94]` インデックス配列に格納。ID はグローバル clone カウンタ `[0xB3B5D4]` 採番。
  - テンプレート名文字列: `race_template_pane`=0xA6D2DC / `button`=0xA6D2F4 / `toggle`=0xA6D318 / `text`=0xA6D330 / `slider`=0xA6D3D0（EXE 内唯一の参照-site は上表ラッパ内）。
  - 各 spawn の trait 書込（v353_2）: `0xFA8`=id / `0xFAA`=listindex(親 childcount−1) / **`0xFAE`=user0** / `0xFAF`=user1 / `0xFB0`=user2(label 文字列) / `0xFB4`=user6。
- ページ可視化（XML [確定]）: pane の `<visible> me().user0 == RaceSexMenu.user0`。pane 側 user0 は spawn 時 `[esp+0x14]`（＝id と同一値）＝**ページ鍵は pane の clone id**。初期 user0=0 の pane（Main）がまず表示される。
- ページ切替の実装 [確定]: nav ボタンハンドラ帯 0x5C5B36/0x5C5C5E/0x5C5F2E/0x5C61B5/0x5C62FC ほか（v353_3a）が `[0xB38Fxx]` ラベルキーで 0x5C3110 を引き → 該当 pane handle → その user0(0xFAE float＝pane id) をメニュー user0（[menu+0x4] の trait obj、0x5CC2B0 群で GetTrait 使用確認）へ SetTrait(0x58CEB0, 0xFAE)。**0x5C006F/0x5C1B80〜0x5C1C23 群の 0xFAE/0xFAF 書込は別メニュー（id 0x416・v353_9b で 0x589B70(0x416) 取得確認。メニュー型の正式同定は [未確定]）であり RaceSexMenu(0x40C) のものではない**（chargen 帯に複数メニューが同居している点に注意）。
- スクロール（XML id20 [確定]）: user8=6 同時表示・矢印 step1・bar jump5・最大値=メニュー user1−6。`user1`=「現在表示 pane の項目数」（XML L13 コメント原文）。
- パネル数=21。合計項目 = 90 slider + 22 button + 5 toggle + 1 text = 118（v353_4 callers 計数と一致）。

## 2. page→pane→中身 対応表（spawn 順＝v353_6 機械抽出／ラベル＝v353_7 で 0xB38F70..0xB39440 の静的初期化文字列を解決）

ラベル key 名 (INI 名) = 表示名: sMain=Main, sRace=Race, sFace=Face, sHair=Hair, sEyes=Eyes, sAge=Age, sComplexion=Complexion, sGender=Gender, sRaceDescription=Race Description, sStyle=Style, sRed/sGreen/sBlue, sLength=Length, sShape=Shape, sTone=Tone, sRandom=Random, sReset=Reset, sGeneralFace=General(Face), sBrow=Brow, sCheeks=Cheeks, sChin=Chin, sForehead=Forehead, sJaw=Jaw, sMouth=Mouth, sNose=Nose, sGeneralSkin=General(Skin), sSkin=Skin, sBeard=Beard, sEyebrows=Eyebrows, sMouthLips=Mouth(Lips), sLips=Lips, sColor=Hair Color, sHairColor0..15（INI 名そのまま）, iHairColor00..15, sNoNameCharacter="You must first enter a valid name.", sEnterName="Enter character name."

| # | pane (spawn@ / key) | 項目（spawn@0x5CA420..0x5CBB79・v353_6 全量） |
|---|---|---|
| 0 | Main @0x5CA448 [0xB38F70] →handle[+0x40] | Button: Race・Face・Hair（nav3）／Toggle: Eyes／Slider: Age(0xB38F98)・Complexion(0xB38FA0) |
| 1 | Race @0x5CA5A4 [0xB38F78] →[+0x44] | Toggle: Gender(0xB38FA8)・Race(0xB38F78=レース名サイクル)／Text: Race Description(0xB38FB0) |
| 2 | Hair @0x5CA62D [0xB38F88] →[+0x48] | Toggle: Style(0xB38FB8)・Hair Color(0xB39330)／Slider: Red・Green・Blue・Length(0xB38FC0..FD8) |
| 3 | Face @0x5CA726 [0xB38F80] →[+0x4C] | Button: Shape・Tone・Random・Reset(0xB38FE0..FF8) |
| 4 | Shape @0x5CA85B [0xB38FE0] →[+0x50] | Button×9: Face(戻る)・General(Face)・Brow・Cheeks・Chin・Eyes・Forehead・Jaw・Mouth・Nose（0xB39000..038） |
| 5 | Tone @0x5CAAE4 [0xB38FE8] →[+0x54] | Button×6-7: General(Skin)・Skin・Beard・Eyebrows・Mouth(Lips)・Eyes(shades)・Nose(shades)（0xB39040..060, 0xB38F90, 0xB39320/328）。**label 帰属の 1-2 件 [部分確定]**（back ボタンラベルと混在） |
| 6 | General(Face) @0x5CACA1 [0xB39000] | Slider×5: Face brow/nose/chin ratio・forehead/sellion/nose ratio・heavy/light・round/gaunt・thin/wide（0xB39070..090） |
| 7 | Brow @0x5CAD84 [0xB39008] | Slider×3: Brow Ridge high/low・Inner up/down・Outer up/down（0xB39098..0A8） |
| 8 | Cheeks @0x5CAE1B [0xB39010] | Slider×5: Cheekbones low/high・shallow/pronounced・thin/wide・Cheeks concave/convex・round/gaunt（0xB390B0..0D0） |
| 9 | Chin @0x5CAEFE [0xB39018] | Slider×7: Chin forward/backward・pronounced/recessed・retracted/jutting・shallow/deep・small/large・tall/short・wide/thin（0xB390D8..108） |
| 10 | Eyes @0x5CB02D [0xB38F90] | Slider×4: Eyes down/up・small/large・tilt inward/outward・together/apart（0xB39110..128） |
| 11 | Forehead @0x5CB0EA [0xB39020] | Slider×3: Forehead small/large・tall/short・tilt forward/back（0xB39130..140） |
| 12 | Jaw @0x5CB181 [0xB39028] | Slider×4: Jaw retracted/jutting・wide/thin・Jaw-Neck slope・Jawline concave/convex（0xB39148..160） |
| 13 | Mouth @0x5CB23E [0xB39030] | Slider×10: drawn/pursed・happy/sad・high/low・lips deflated/inflated・lips large/small・lips puckered/retracted・protruding/retracted・tilt up/down・underbite/overbite・Mouth-Chin distance（0xB39168..1B0） |
| 14 | Nose @0x5CB3DF [0xB39038] | Slider×14: bridge shallow/deep・bridge short/long・down/up・flat/pointed・nostril tilt・nostrils small/large・nostrils wide/thin・region concave/convex・sellion down/up・sellion shallow/deep・sellion lower shallow/deep・sellion thin/wide・short/long・tilt down/up（0xB391B8..220） |
| 15 | General(Skin) @0x5CB618 [0xB39040] | Slider×5: Skin flushed/pale・shade dark/light・tint orange/blue・tint purple/yellow・Cheek blush light/red（0xB39228..248） |
| 16 | Beard @0x5CB6FB [0xB39050] | Slider×6: Beard flushed/pale・light/dark・cheeks・circle・goatee・moustache light/dark（0xB39250..278） |
| 17 | Eye shades @0x5CB819 [0xB39320] | Slider×6: Eye sockets bruised/bright・dark/light・Eyelids light/dark・pale/red・Eyeliner・Eyeshadow（0xB39280..2A8） |
| 18 | Eyebrows @0x5CB937 [0xB39058] | Slider×7: dark/light・low/high・thick/thin・very thin/thick・lower・outer・upper（0xB392C0..2F0） |
| 19 | Mouth(Lips) @0x5CBA7E [0xB39060] | Slider×3: Lips flushed/pale・Lipstick dark red/light blue・dark blue/light red（0xB392F8..308） |
| 20 | Nose shades @0x5CBB21 [0xB39328] | Slider×2: Naso labial lines light/dark・Nares small/large（0xB39310..318） |

- Hair Color 16 本（sHairColor0..15＝0xB339338..3B0）は初期化で menu[+0x8F0..0x92C] に BSString 格納のみ（spawn 呼び出しなし）→ **スライダーとして未 spawn**（INI/ヘアカラーインデックス用データ）。iHairColor00..15（0xB393B8..430）も同枠の RGB パッケージ文字列。
- モーフ対応: 上表ラベル文字列群は s77 §1.5 の 0xA62xxx-0xA63xxx 帯＝**FaceGen INI 名そのもの**（head5/eyes4+6/眉7/鼻14/口10/髭6＋ratio系）。v351 §1.2 の個数表と整合（眉7・鼻15=14+sellion系・口10・髭6）。

## 3. 依頼②の規則 [確定]

- **集合は race/sex で変化しない**: 全 pane/スライダーは FUN_005CA010 の init で毎回 21 個・90 本を静的 spawn（分岐なし）。
- race/sex の効果は runtime 反映のみ:
  - **user13（0xFBB）= enabled**: 書込 site は chargen 帯の 0x5C3574/0x5C3639/0x5C369C（ページ切替時・1.0f 設定）と 0x5CBE00/0x5CBE4A/0x5CBE94（0/1 群）、0x5DA367/0x5DA5D9（外見反映 fn）。XML 側でも toggle/slider 親が `user13` で target・マーカ alpha を制御（L404/565）。
  - レース選択ハンドラ 0x5CDA30（v353_8a）: TESRace リスト `[0xB33A98+0x44]` を `[form+0x70] bit0`（＝playable）で走査、選択 race の name/desc を trait 0xFB0/0xFAE 経由で反映 → **list 側は race 共通・項目の enabled のみ race 依存**。
- **[未確定1]** レース/性別で「無効化される具体的なスライダー集合」の静的裁定（0x5C3574 群の条件分岐先と race flag bit の対応。実機 .ess 依存分は #82 ゲート）。
- **[未確定2]** 各スライダーの user7（0xFB5=スライド対象 editor id）バインド値の一覧（spawn 時 SetTrait 列の追跡が途中）。s77 §1.3 INI 名→モーフ実適用の接続細部（v351 §1.3-3 の続）とセットで次回。
- **[未確定3]** Tone ページのボタン 6-7 個の正確なラベル帰属（back ボタン「Face/Tone」混在。v353_6 の globals 2 件列挙が back ラベル＋item ラベルの順で併記されるため、1-2 件の対応みは parent レジスタ追跡で最終確定可）。

## 4. #123 との接続（おまけ [確定]）

- 名前検証メッセージ: `[0xB39438]` sNoNameCharacter = **"You must first enter a valid name."**／`[0xB39440]` sEnterName = "Enter character name." → v352 §0③ の 0x5CCA77 検証分岐が送るテキスト（Done 失敗時）。
- `[0xB38F70..0xB38FA8]` の label キー群は v352-28 の 0x5C3110 key→member 対応表（0xB38F70→[+0x40]/0xB38F78→[+0x44]/0xB38F88→[+0x48]…）と同一アドレス＝**対応表のキーはページラベル文字列**（Done 時「ラベル文字列→ページ handle」引く）。v352 §5[未確定3] の 0x5C3110 entry 意味は本稿で解消: key 0xB38F70='Main'/F78='Race'/F88='Hair'/F80='Face' → member = 各 pane handle フィールド。[確定]

成果物採番: v353_1..9（tmp/re/v353_chargen_page_ruling.md 本稿）。関連: #97/v351 §1.3、#123/v352、#128束①（次）。
