# v566 — U9 裁定書: 左上ブロック群の描画源特定（ai-todo #156）

> 本ファイルは v566 の 2 種の下書きを統合した正本（`row542`・`note542` は本ファイルを参照）。

## 1. 結論（絞り込み結果）


| 手順 | 結果 | 判定 |
|---|---|---|
| ① 2D の矩形・枠描画はどこにあるか | **すべて `ui/` の `UIDrawHelper::drawColoredQuad` / `drawBorder` 経由**。`engine/` 側でこれを使うのは `renderer.cpp:8492`（`UIDrawHelper::cleanup`）だけで**描画呼び出しは0件** → クラスターは必ず `->render()` のどれか | [確定(命令)] |
| ② `renderer.cpp` の `->render()` は全部で 68 箇所 | 全列挙済み（下表 §2） | [確定(命令)] |
| ③ 5380-5664 の無効化で残る描画 | **タイトル/起動/ローディング/キャラ作成パス（4749-5055・`return` で抜ける）** と**フレーム末尾（5679/5683/5688・`isVisible()` ガード）** のみ | [確定(命令)] |
| ④ シーンパス（5300-5380）に 2D 矩形描画はあるか | **無い**。`worldManager`/`renderInteriorObjects`/`renderStaticObjects`/`renderTerrainMeshes`/`renderWater`/`renderDistantLod`/`renderSkyDome`/`worldEntities` は 3D のみ。`renderPlaceholderEntities`（5741）は **イコサ球**（シェーダは `uMVP*uColor` のみ・アウトライン無し）→ **矩形＋輪郭ではない** | [確定(命令)] |
| ⑤ `retroFilter->renderToScreen()`（5785-5787）が原因か | **除外**。全画面クワッド1枚（頂点 `-1..1`）＋ composite シェーダは素のパススルー（`FragColor = texture(inputTexture, vTexCoord)`）＝**グリッド/ブロックを生成しない** | [確定(命令)] |
| ⑥ debug 系（`debugHUD`/`debugDraw`/`npcDebugVisualizer`/`worldDebugInfo`/`performanceGraph`/`DebugMenu`/`gameConsole`/`toggleAllDebugSystems` 4系統） | すべて5432-5446・5679-5683＝**無効化領域内か `isVisible()` ガード**。さらに `aabbVisualization`/`npcOverlay`/`touchTrail` は **toggle のみで参照ゼロのデッドフラグ**（読み手が無い） | [確定(命令)] |
| ⑦ それでも残る 2D 描画 | **9 系統のみ**（§2）。この中で「**複数の矩形を縦に積む（＝行）**」を満たすのは **`characterCreationUI` のみ** | [部分確定] |

**→ 最有力候補: `characterCreationUI->render()`（`renderer.cpp:5055`・タイトルパス／`6043`・ゲームパス）**

---

## 2. 残った 9 系統（すべて4749-5055・タイトルパス、`return` で抜ける）


| 行 | 対象 | 「行を積む」か |
|---|---|---|
| 4749 | `launcherScreen->render()` | あり（リスト） |
| 4779 | `loadingScreen->render()` | 進捗バー1本 |
| **4946** | `titleScreen->render()` | メニュー横並び（行ではない） |
| 5020 | `saveLoadUI->render()` | あり（セーブ行） |
| 5023-5041 | `optionsUI` 系 7 種 | あり（設定行） |
| 5046 | `uiPauseMenu->render()` | ボタン縦積み |
| 5049 | `uiMessageBox->render()` | 中央 |
| **5055** | **`characterCreationUI->render()`** | **✓ 行ループ（`PlaceholderAssets::drawPanel`）** |

ゲームパス側の同系統は 5464-5493・5643（＝無効化領域内）。

---

## 3. 排除の過程（静的解析で潰した候補）


### 3.1 2D の矩形描画はすべて `ui/` の `UIDrawHelper` 経由

`engine/` 配下で `UIDrawHelper::drawColoredQuad` / `drawBorder` を呼ぶのは **`renderer.cpp:8492`（`cleanup` のみ）**。
→ **矩形＋輪郭を描くのは必ず `ui/` の `->render()` 経由** [確定]。

### 3.2 `renderer.cpp` の `->render()` は全部で68箇所（機械列挙）

| 帯 | 行番号 | 内容 | 判定 |
|---|---|---|---|
| タイトル/起動/ローディング/メニュー/キャラ作成パス | 4749–5055（**`return` で抜ける**） | launcherScreen / loadingScreen / titleScreen / saveLoadUI / options 系 / settingsUI / uiPauseMenu / uiMessageBox / **characterCreationUI** | **クラスター候補** |
| シーンパス | 5313 | `worldManager->render()`（3D のみ） | 2D 矩形を描かない → **除外** |
| ゲーム内オーバーレイ | 5417–5643 | questUI / inventoryUI / debugHUD / debugDraw / メニュー群 / uiSystem / virtualController / HUD 群 / floatingText / characterCreationUI | **ユーザーが無効化済み → 除外** |
| フレーム末尾 | 5679 / 5683 / 5688 | debugMenu / gameConsole / uiMessageBox | すべて `isVisible()` ガード → **除外** |

### 3.3 シーンパスは 2D 矩形を描かない [確定]

`renderPlaceholderEntities()`（`renderer.cpp:5741`）は**視点を差し替えたイコサ球**を3Dで描くだけ（シェーダ `placeholderVertexSrc`/`placeholderFragmentSrc` は `uMVP` と `uColor` のみ、**アウトライン無し**）。
`retroFilter->renderToScreen()`（`retro_filter.cpp:563`）は `quadVAO`（フルスクリーン −1..1、`retro_filter.cpp:146`）を1枚描くのみで、**コンポジットシェーダはパススルー**（`retro_filter.cpp:360` `FragColor = texture(inputTexture, vTexCoord)`）→ **ブロックを生成しない**。
→ **シーンパスは除外**（ユーザーの仮説②は棄却）。

---

## 4. 最有力候補の根拠: `UICharacterCreation` は **`size`/`position` を一度も受け取っていない**


### 4.1 呼び出しの全数確認（`renderer.cpp` の `characterCreationUI->` 全21箇所）

`initialize` / `setScreenSize` / `setESMManager` / `setOnConfirm` / `setOnNameRejected` / `open` / `update` / `render` / `isVisible` / `onTouchDown` / `isNameInputActive` / `setPlayerName` のみ。
**`setPosition()` も `setSize()` も1度も呼ばれない** [確定(命令)]。

### 4.2 既定値の連鎖

```
UIComponent  : glm::vec2 position;  glm::vec2 size;   → 初期化子なし＝(0,0)   (ui_component.h:208-209)
UIPanel ctor : titleBarHeight = 40.0f, contentMargin = 10.0f              (ui_panel.cpp:9,13)
UIPanel::initialize() : 色と枠のみ、size/position は触らない              (ui_panel.cpp:24-33)
UIComponent::onScreenResize() : screenWidth/screenHeight のみ             (ui_component.cpp:166-172)
  ↑ `setScreenSize()` はただの転送 (ui_component.h:164)
```

→ **`size = (0,0)` / `position = (0,0)` が確定** [確定(命令)]。

### 4.3 そこから導かれる座標（実計算）

```
getContentPosition() = absPos + (contentMargin, titleBarHeight + contentMargin)   (ui_panel.cpp:150)
                     = (0 + 10, 0 + 40 + 10) = (10, 50)
getContentSize()     = (size.x - 2*margin, size.y - titleBarHeight - 2*margin)    (ui_panel.cpp:155)
                     = (0 - 20, 0 - 40 - 20) = (-20, -60)   ← ★負
```

`UICharacterCreation` の各描画:

| 描画 | コード | 結果 |
|---|---|---|
| タブ 5 枚 | `tabW = getContentSize().x / 5 = -4`、`drawPanel(cp.x + i*tabW, ty, tabW-1, 32)` | x = 10, 6, 2, −2, −6／幅 **−5** → **左端にクリップされた細い帯** |
| ラベル | `drawPanel(cp.x+20=30, cy=90, getContentSize().x-40=−60, 40)` | x 30→−30 → **画面で 0..30px の帯** |
| 入力欄 | `drawPanel(30, 135, −60, 48)` | 同上・2 本目 |
| 情報テキスト | `renderText(..., 40, 200, ...)` | 描画される（テキストも左寄せ） |
| Confirm/Cancel | `by = cp.y + contentSize.y - 44 = 50 - 60 - 44 = **−54**` | **画面外（負 y）→ 見えない** |
| パネル背景・タイトルバー・閉じるボタン | `drawColoredQuad(absPos, size=(0,0))` | **退化して描画されない**（＝「枠の無いブロック群」に一致） |

**結果: 画面左上に、色付き背景＋2px 枠線の矩形が 5+2 = 7 本（＋情報テキスト1行で8行）縦に積まれる。** → **「左上に7〜8行の彩色矩形＋黒い輪郭」に一致** [確定(命令)]

（`renderAttributesTab()` ならヘッダ1＋属性8行＝9本、`renderSkillsTab()` なら21本と、**タブによって行数が変わる**点がこの説明の予測でもある＝**確認材料になる**）

### 4.4 なぜ 5380-5664 の無効化で消えないのか

`characterCreationUI->render()` の**タイトルパス側（5053-5055）は無効化領域の外**。ゲームパス側の 6043 を消しても、タイトルパス側が描き続ける [確定(命令)]。**この不一致が U9 の「無効化しても残存」を完全に説明する。**

---

## 5. 候補の棄却理由（ユーザー提示 4 候補）


| 候補 | 裁定 | 根拠 |
|---|---|---|
| ① `retroFilter` のポストプロセス | **[確定負]** | `renderToScreen` はフルスクリーンクワッド1枚（`retro_filter.cpp:146-151`）。コンポジットはパススルー（`:360`）。ブロックを生成しない。 |
| ② `renderPlaceholderEntities` | **[確定負]** | 3D イコサ球のみ（`renderer.cpp:5741`）。シェーダにアウトラインなし。スクリーン固定の矩形を描かない。 |
| ③ メニュー領域（4700-5020）の未ガード描画 | **[部分的に肯定→特定]** | その帯にあり、**中身は `characterCreationUI->render()`（5455・現行番号）**。他の8系統はすべて `isVisible()` ガードで、表示中のみ描画。 |
| ④ `UISystem` の既定可視パネル | **[確定負]** | ユーザーが `uiSystem->render()` 無効化で消えていないこと＋`UICharacterSheet` は `setPosition`/`setSize` 済み（`renderer.cpp:2856-2858`）で中央配置、かつ `setVisible(false)` 初期＋`openmenu stats` で表示。 |

---

## 6. 描画源を1回で確定させる手順（推測実装を避けるため）


**A. 最小の一手（1行）** — 候補の当否を即座に判定:
```
renderer.cpp:5455 と 6043 の characterCreationUI->render(); をコメントアウトしてビルド
→ 消えるなら確定（§3 の説明どおり）
```

**B. 決定的（どの候補でも当てはまる）** — 2D 矩形はすべて `UIDrawHelper::drawColoredQuad` に集まるので、**呼び出し元を1回記録**すれば特定できる:
```cpp
// ui_draw_helper.cpp:343 drawColoredQuad() の先頭に1行
LOGI("quad <- %p (%.0f,%.0f %.0fx%.0f)", __builtin_return_address(0), x, y, w, h);
```
→ logcat の `quad <- 0x........` を数フレーム分収集し、**座標が x≈0..30 / y≈50..200 かつ幅が負**の行を特定 → そのアドレスを `addr2line`/シンボルで関数に変換＝**確定**。
（矩形は1つずつなので、1フレームで全候補のアドレスが揃う。修正後にログを外す）

**C. 補助**: タブを切り替えて**行数が 7→9→21 と変わる**なら §3 の mechanism 確定（Attributes=8 行、Skills=21 行）。

---

## 7. 修正の方向性（実装は未着手・確定後に行うこと）


`UICharacterCreation` に `onScreenResize()` をオーバーライドし、`setSize()` / `setPosition()` を呼ぶ（他の UI が `setScreenSize` オーバーライドでやっているのと同じ型）。少なくとも:
* `getContentSize().x > 0`（＝行の幅が正）
* `getConfirmButtonY() > 0`（＝確認ボタンが画面内）

が満たされるようにする。**この裁定は描画源の特定が目的で、修正自体は未実施。**

---

## 8. 残点


| ID | 内容 | 重要度 |
|---|---|---|
| V-566-1 | 「7〜8行」の正確な内訳＝どのタブ（`NAME`=5+2 か `ATTRIBUTES`=8 か `RACE`=N か）で観測したか。タブ切替で行数が変われば確定 | 中 |
| V-566-2 | `getContentPosition()`/`getContentSize()` の宣言先（`ui_component.h` か `ui_panel.h` か）と `anchor` 既定値（`getAbsolutePosition()` のオフセット有無） | 低 |
| V-566-3 | 同種の「`setSize` 未呼び出し」を他 UI にも横断確認するか（`UICharacterSheet` は呼び出し済み・他は調査未） | 中 |
