# v367 — RaceSexMenu レイアウト寸法の裁定（#142）

- 採番: **v367**（#142 専用、probe prefix v367_1）
- 対象: Oblivion.exe v1.2.0416 ＋ bsa 抽出済み `Data\Menus\CharGen\race_sex_menu.xml`（705行全読）／`prefabs\button_long.xml`／`prefabs\generic_background.xml`
- 依頼: ai-todo #142（#96 の仕様裁定材料。①Create/Cancel ボタン寸法・②タブ列幅・③パネル高さと下部空白・④コンテンツ開始位置と行ピッチ）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は原本 XML または EXE 命令のみ。

---

## §1 裁定（依頼4点）

### ① Create/Cancel ボタンの寸法と配置 — **[確定]**（port 側 105×32/scale0.65/2ボタン/間隔16 はすべて原本不一致）
- **原本は Done/Return の単一ボタン**（Back は同一要素 `race_button_done`(id90) の user5 切替＝独立ボタンなし。#130 Phase A と整合）**[確定(XML)]**。
- **寸法＝`button_long.xml` の 177×44**（`<include src="button_long.xml"/>` が幅高を供給）**[確定(XML)]**。
- **配置（race_background＝560×740 座標系）**:
  - `x = parent.width − me.width − 30` → **560−177−30 = 353**（**パネル右端からのインセット 30px**）
  - `y = parent.height − me.height − 20` → **740−44−20 = 676**（**下端インセット 20px**）
  - → **Done ボタン = 177×44 @(353, 676)** [確定(XML+計算)]
- **ラベル**: `cont_button_exit_text`＝**font = user2 = 1・justify = center・scale 属性なし（=1.0 相当）** [確定(XML)]。色=117(+128 when enabled) 系。
- **「ボタン同士の間隔」「左ボタン」「contentW−226」は原本に存在しない [確定負]**（ボタン1個のため間隔概念なし）。
- button_long 補助: user20=1（look=通常ボタン）・user21/22/23=117/59/33 は xbox 表示時の override のみ。

### ② タブ列の幅 — **[確定負: 原本にタブ列が存在しない]**
- **XML 全705行に tab 要素なし [確定]**。page 可視化は `user0`（＝「what pane/page is currently visible」L12 コメント）と pane の `visible: me().user0 == RaceSexMenu.user0`（L250-253）のみ。
- **EXE 側もタブ生成なし [確定(否定側)]**: `0x5CA010`（RaceSexMenu 初期化）のテンプレ生成ラッパは `0x5C4340`('race_template_pane')／`0x5C4480`('race_template_button')／`0x5C4630`／`0x5C93F0`(slider)／`0x5C4800` のみ（v353 のテンプレ名一覧にも tab 系なし）。
- **page 導線＝ページ内 nav ボタン行（race_template_button）＋ラベルキー 'Main'/'Race'/'Hair'/'Face'（v353 §5）** [確定(v353 再掲)]。
- → **port の contentW/5 均等5タブ列は原本に比較対象が存在しない**（構造差＝タブ列は port 発明）[確定]。

### ③ パネル全体の高さと下部の空白 — **[確定]**（port の約935px 高＋約900px 漆黒空白は原本不一致）
- **パネル（race_background）＝幅 user0=560 × 高 user1=740**（generic_background の user0/1＝幅/高コメント L8-12 で確定）**[確定(XML)]**。
- **配置: `x = 70` 固定・`y = (screen().height − 740) / 2`（画面中央）** [確定(XML)]。（補足: screen 高 600 では y=−70 で上端クリップ＝原本の解像度依存挙動 [確定(XML)+観察]・対処は未特定 V-367-2）
- **内側 `race_window`(id1)＝500×558 @(30, 85)・depth8・clipwindow** [確定(XML)]。
- **垂直配分（パネル座標）**: race_name y=20（center, wrapwidth425）→ separator 400×3 @(75,75) → **window 85..643** → **Done 676..720** → パネル下端 740。
- → **window 下端からパネル下端までの97px は Done ボタンで占有され、「コンテンツ110px＋以下900px の漆黒空白」は存在しない [確定]**。

### ④ コンテンツ開始位置と行ピッチ — **[確定(XML)]**（port の y=0/45/110 のうち45のみ対応）
- **行ピッチ＝93px**（全テンプレ共通）:
  - `race_template_button_layout`(id101): `height 93`・`y = listindex × height(93)`・`width = parent.width`・repeatvertical [確定]
  - `race_template_toggle`(id102): `height 93`・`y = listindex × 93` [確定]
  - `race_template_slider`(id103): `height 93`・`y = listindex × 93`・repeathorizontal [確定]
  - `race_template_text`(id104): `y = listindex × 93 + 20`・font 1・wrapwidth = parent.width [確定]
- **行内の y オフセット**:
  - ラベル（button/toggle/slider label）: **font 3 @(0,0)** [確定]
  - **スライダーバー: y = 45**（幅 = parent − 左右矢印幅・x = 左矢印幅）→ **port の 45 と一致 [部分一致]** [確定]
  - スライダーマーカ: y=25・高64（幅 = text幅+30, max 85／marker2 = 104×64 @ x=parent.w+15−104）[確定]
  - toggle 矢印: **左 124×36 @(0,35)／右 121×36 @(w−121,35)**（非対称幅に注意）[確定]
  - toggle value text: wrapwidth250・x=w/2・y=(h−text.h)/2+5 [確定]
  - button text: **wrapwidth490 @(w/2, 25)** [確定]
  - **port の y=110 に対応する原本値は存在しない**（説明/テキストは行内 +20・93 行ピッチ）[確定(否定側)]
- **パネル上部の開始位置**: race_name y=20 → separator y=75 → window y=85（パネル座標）[確定]。**コンテンツ（pane 行）は window 先頭（y=85 起点）から `y = −scroll.user7×93` でスクロール** [確定(XML)]。
- **スクロールバー(id20)**: x = parent.w − 20・y = 25・height = parent.h − 50・depth20・**同時表示 user8=6**・arrow step user3=1・bar jump user4=5・max = RaceSexMenu.user1 − 6 [確定(XML)]。
- **race_pane(id100)幅の式**: `parent.width − f(childcount, gt 6, ×40)`（原文 L254-262）— **評価順序は DTD 不在で [未確定]（V-367-1）**。pane y = `−race_scroll_bar.user7 × 93` [確定(XML)]。

## §2 証拠（probe v367_1・EXE）
- **`0x5CA010`（RaceSexMenu 初期化）**: `FindMenu(0x40C)`（→ **0x40C = RaceSexMenu [推定高]**・caller `0x57B8D9`）→ `0x590420('Data\Menus\CharGen\race_sex_menu.xml')` 読込（失敗時 `'Race/Sex Menu Creation Failed... Are your menu and art resources'`）→ `GetFloat(0xFA5)(stackingtype)×2 → SetFloat(0xFAB)`（v366 の stackingtype 分岐系と同一）→ **16× `0x402E30("%s")` を `menu+0x8F4..+0x92C`（stride4）へ格納** → テンプレ生成ラッパ呼び出し（0x5C4340×3→`[esi+0x40/44/48]`／0x5C4480×3（page 引数 0/1/2 相当のローカル）／0x5C4630×4／0x5C93F0×6／0x5C4800×1）**＝タブ生成なし** [確定(命令)]。
- **`0x5C4340`**: `'race_template_pane'`(0xA6D2DC) を `0x585410` で解決（失敗時 -1→`[0xB3B5D4]` 採番）→ `fild(id) → SetFloat(0xFAE)`（**page = pane element id**・v353 再掲）＋ SetFloat(0xFA8/0xFAA/0xFAE…) [確定(命令)]。`0x5C4480`＝`'race_template_button'`(0xA6D2F4) ラッパ。
- **band 内に寸法即値 push（500/558/560/740/177/44/93）= 0件 [確定(否定側)] → EXE に寸法ハードコードなし＝原本寸法はすべて XML 由来**。
- 依存文字列: 0xA6973C=`Data\Menus\CharGen\race_sex_menu.xml`／0xA6D2DC=`race_template_pane`／0xA6D2F4=`race_template_button`／0xA2F864=`%s`／0xA6D30C=`'          '`（10 spaces）。

## §3 既存成果物との整合
- **v353（#124）と完全整合**: page＝pane id（0x5C4340 の fild→0xFAE）・**21 pane/90 slider を 0x5CA010 が毎回静的 spawn**・テンプレ名一覧・scroll user8=6・ラベルキー 'Main'/'Race'/'Hair'/'Face' — 本稿は v353 が未特定だった**寸法軸**を補完。
- **#130 Phase A と整合**: Back＝id90 の user5 切替（単一ボタン）→ ①の「2ボタン不採用」の根拠。

## §4 残点 **V-367-1..5**
1. **race_pane width 式の評価順序**（`parent − f(childcount>6, ×40)`・DTD 不在）＝race リスト等の実効幅【中・port 再現に必要】
2. **screen 高<740 時の原本挙動**（600 で上端クリップ・原本の対処=最小解像度/別式）【低】
3. **`menu+0x8F4..0x92C` の16×'%s' スロットの用途**（16＝何の個数か）【低】
4. **`0x5C4630`/`0x5C4800` のテンプレ対応と引数意味**（toggle/text ラッパか）【低】
5. **port `getContentStartY()` の110 に対応する原本要素**（存在しない可能性が高いが、Name ページ固有要素の完全列挙は pane spawn 個別確認に依存）【低】

## §5 検証・自己批評
- probe: **v367_1 全 EXIT=0**。XML 3ファイル（race_sex_menu 705行全読・button_long・generic_background）読了。
- 判定の揺れなし: ②③④は XML 単独で確定可能だった点、EXE 偵が必要だったのは「タブ生成の有無（否定側）」「寸法ハードコードの不在（否定側）」「0x40C 同定」の3点。
- docs との不整合: なし（新規裁定。port のマジックナンバーとの差分は §1 に列挙）。
- L1 制約: docs/GT/ESM 変更なし・移植側ソース変更なし。
