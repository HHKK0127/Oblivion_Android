# v566 — U9 裁定書: 左上ブロック群の描画源特定（ai-todo #156）

対象: 移植側コード（`app/src/main/cpp/`）。根拠はコード実読（静的解析）。原版側の前提は `tmp/re/v562_u6_u4_assets_topleft_ruling.md`（原版に左上要素は無い＝[確定(XML)]）。
判定: `[確定(命令)]`（移植コードの実読による）／`[部分確定]`

---

## 1. 結論

| 問い | 判定 | 回答 |
|---|---|---|
| 7〜8 行の彩色矩形＋黒い輪郭を出す描画関数はどれか | **[確定(命令)]** | **`characterCreationUI->render()`**（`renderer.cpp:5455`＝タイトル/キャラ作成パス、`renderer.cpp:6043`＝ゲーム内パス）→ `UICharacterCreation::render()`（`ui_character_creation.cpp:263`）→ `PlaceholderAssets::drawPanel()`（`ui/placeholder_assets.cpp:84`）→ **`UIDrawHelper::drawColoredQuad`（色の塗り）＋`UIDrawHelper::drawBorder`（輪郭）**。 |
| なぜ**左上**に出るのか | **[確定(命令)]** | **`UICharacterCreation` に `setPosition()`/`setSize()` が一度も呼ばれない**ため、`UIComponent::position`/`size` が既定の **(0,0)** のまま。→ `getContentSize()` が**負値**になり、全ての `drawPanel(x, y, 負の幅, h)` が**左端にクリップされた細い帯**として描画される（§3）。 |
| なぜ 5380-5664 を無効化しても消えないのか | **[確定(命令)]** | その無効化領域は**ゲーム内パス**のオーバーレイであり、クラスターの実行点は**タイトル/キャラ作成パスの `renderer.cpp:5455`**（`return` で抜けるブロック内）。**別コードパスなので無効化は効かない。** |
| 修正方針 | **[部分確定]** | `characterCreationUI` に `setPosition()`/`setSize()` を与える（他の UI が `setScreenSize()` オーバーライドでやっているのと同じ）。**ただし本裁定は描画源の特定が目的で、実装は移植レーンの判断に委ねる。** |

---

## 2. 排除の過程（静的解析で潰した候補）

### 2.1 2D の矩形描画はすべて `ui/` の `UIDrawHelper` 経由

`engine/` 配下で `UIDrawHelper::drawColoredQuad` / `drawBorder` を呼ぶのは **`renderer.cpp:8492`（`cleanup` のみ）**。
→ **矩形＋輪郭を描くのは必ず `ui/` の `->render()` 経由** [確定]。

### 2.2 `renderer.cpp` の `->render()` は全部で68箇所（機械列挙）

| 帯 | 行番号 | 内容 | 判定 |
|---|---|---|---|
| タイトル/起動/ローディング/メニュー/キャラ作成パス | 4749–5055（**`return` で抜ける**） | launcherScreen / loadingScreen / titleScreen / saveLoadUI / options 系 / settingsUI / uiPauseMenu / uiMessageBox / **characterCreationUI** | **クラスター候補** |
| シーンパス | 5313 | `worldManager->render()`（3D のみ） | 2D 矩形を描かない → **除外** |
| ゲーム内オーバーレイ | 5417–5643 | questUI / inventoryUI / debugHUD / debugDraw / メニュー群 / uiSystem / virtualController / HUD 群 / floatingText / characterCreationUI | **ユーザーが無効化済み → 除外** |
| フレーム末尾 | 5679 / 5683 / 5688 | debugMenu / gameConsole / uiMessageBox | すべて `isVisible()` ガード → **除外** |

### 2.3 シーンパスは 2D 矩形を描かない [確定]

`renderPlaceholderEntities()`（`renderer.cpp:5741`）は**視点を差し替えたイコサ球**を3Dで描くだけ（シェーダ `placeholderVertexSrc`/`placeholderFragmentSrc` は `uMVP` と `uColor` のみ、**アウトライン無し**）。
`retroFilter->renderToScreen()`（`retro_filter.cpp:563`）は `quadVAO`（フルスクリーン −1..1、`retro_filter.cpp:146`）を1枚描くのみで、**コンポジットシェーダはパススルー**（`retro_filter.cpp:360` `FragColor = texture(inputTexture, vTexCoord)`）→ **ブロックを生成しない**。
→ **シーンパスは除外**（ユーザーの仮説②は棄却）。

---

## 3. 根因: `getContentSize()` が負になる [確定(命令)]

```
renderer.cpp:907-911
    characterCreationUI = std::make_unique<UICharacterCreation>();
    characterCreationUI->initialize(textRenderer.get());
    characterCreationUI->setScreenSize(screenWidth, screenHeight);   ← これだけ
```

`characterCreationUI->` の**全21箇所を列挙**して確認: `initialize / setScreenSize / setESMManager / setOnConfirm / setOnNameRejected / open / update / render / isVisible / onTouchDown / isNameInputActive / setPlayerName` のみ。**`setPosition()`・`setSize()` は1度も呼ばれない。**

各層の既定値:
* `UIComponent::position` / `size` — **初期化子なし＝`(0,0)`**（`ui_component.h:208-209`）
* `UIPanel` コンストラクタ — `titleBarHeight = 40.0f`、`contentMargin = 10.0f` を設定するが **`size`/`position` は設定しない**（`ui_panel.cpp:5-18`）
* `UIPanel::initialize()` — 色と枠線幅のみ（`ui_panel.cpp:24-33`）
* `setScreenSize()` → `UIComponent::onScreenResize()` — **`screenWidth/screenHeight` のみ**、`size` には触れない（`ui_component.cpp:166-172`）

→ 実効値:
```
getAbsolutePosition()  = (0, 0)
getContentPosition()   = (0 + 10,        0 + 40 + 10) = (10, 50)
getContentSize()       = (0 - 2*10,      0 - 40 - 2*10) = (-20, -60)   ← 負
```

この負の `getContentSize()` が各描画にそのまま入る:

| 描画 | 呼び出し | 実効値 | 見え方 |
|---|---|---|---|
| タブ（`renderTabButtons` :311） | `drawPanel(cp.x + i*tabW, ty, tabW-1, 32, …)` `tabW = getContentSize().x/5 = -4` | x = 10,6,2,−2,−6／幅 −5 | **左端の細い帯**（0..10px） |
| 属性行（`renderAttributesTab` :361-380） | `drawPanel(cp.x+20, cy, getContentSize().x-40 = -60, 36, …)` **`for (i < 8)`** | (30, y) から幅 −60 → **0..30px にクリップ** | **左端の 8 行の帯** |
| スキル行（`renderSkillsTab` :398-205） | 同型・`for (i < 21)` | 0..30px | 21 行 |
| 種族行（`renderRaceTab` :457） | 同型・`availableRaces_` 分 | 0..30px | N 行 |
| 名前タブ（`renderNameTab` :325/:333） | 幅 −60 | (30, 90) / (30, 135) | 2 行 |
| 確定/中止ボタン（:530/:536） | `getConfirmButtonY() = 50 + (-60) - 44 = -54` | **画面外** | **見えない** |
| パネル背景・タイトルバー・閉じるボタン | `size = (0,0)` / 負座標 | 退化 | **描画されない** |

→ **「パネル枠なしで、左端に彩色の帯が縦に積まれる」**＝症状と一致。

**行数**: 既定タブ `NAME`（`open()` :102 で `currentTab = NAME`）ならタブ帯＋名前2パネル＋情報テキスト ≈ **4〜7 行**、`ATTRIBUTES` タブならヘッダ1＋**属性8行**、`SKILLS` なら21行。
→ **「7〜8行」はタブに依存**（ATTRIBUTES ならちょうど 8、NAME なら 5＋2=7）。**ここが V-566-1 の残点**（画面状態で行数が変われば確定）。

---

## 4. 候補の棄却理由（ユーザー提示 4 候補）

| 候補 | 裁定 | 根拠 |
|---|---|---|
| ① `retroFilter` のポストプロセス | **[確定負]** | `renderToScreen` はフルスクリーンクワッド1枚（`retro_filter.cpp:146-151`）。コンポジットはパススルー（`:360`）。ブロックを生成しない。 |
| ② `renderPlaceholderEntities` | **[確定負]** | 3D イコサ球のみ（`renderer.cpp:5741`）。シェーダにアウトラインなし。スクリーン固定の矩形を描かない。 |
| ③ メニュー領域（4700-5020）の未ガード描画 | **[部分的に肯定→特定]** | その帯にあり、**中身は `characterCreationUI->render()`（5455・現行番号）**。他の8系統はすべて `isVisible()` ガードで、表示中のみ描画。 |
| ④ `UISystem` の既定可視パネル | **[確定負]** | ユーザーが `uiSystem->render()` 無効化で消えていないこと＋`UICharacterSheet` は `setPosition`/`setSize` 済み（`renderer.cpp:2856-2858`）で中央配置、かつ `setVisible(false)` 初期＋`openmenu stats` で表示。 |

---

## 5. 確定のための1ステップ検証（推測実装を避ける）

```cpp
// renderer.cpp:5455 と renderer.cpp:6043 の 2 箇所
-            characterCreationUI->render();
+            // characterCreationUI->render();   // U9 source confirmation
```
**この2行をコメントアウトしてビルド**:
* ブロックが消える → **描画源は確定**（本裁定の結論どおり）。
* 消えない → `titleScreen->render()`(4946) → `loadingScreen->render()`(4779) → `launcherScreen->render()`(4749) の順に1行ずつ（いずれもタイトルパス内で、同じ手順で1回ずつで決着）。

補助（1行で呼び出し元を直接出す場合）: `ui/ui_draw_helper.cpp:343` `drawColoredQuad` の先頭に caller 記録を1行追加 → logcat の `quad caller=` を集めると、実行している `->render()` のアドレスが1フレームで確定する。

---

## 6. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-566-1 | 「7〜8行」の正確な内訳＝どのタブ（`NAME`=5+2 か `ATTRIBUTES`=8 か `RACE`=N か）で観測したか。タブ切替で行数が変われば確定 | 中 |
| V-566-2 | `getContentPosition()`/`getContentSize()` の宣言先（`ui_component.h` か `ui_panel.h` か）と `anchor` 既定値（`getAbsolutePosition()` のオフセット有無） | 低 |
| V-566-3 | 同種の「`setSize` 未呼び出し」を他 UI にも横断確認するか（`UICharacterSheet` は呼び出し済み・他は調査未） | 中 |
