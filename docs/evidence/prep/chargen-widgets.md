# chargen ウィジェット仕様表（順序・座標・型・期待挙動・状態）

正典: `docs/chargen_ui_spec.md`（§2 ページ表・§9 geometry・§10 確定経路）。
実装: `app/src/main/cpp/ui/race_sex_panel.{h,cpp}`・`race_sex_panel_instantiator.cpp`。
座標は **canvas** と **device** を分けて書く（`applyLayout` が両者を同じ変換で結ぶ）。

## 共通レイアウト（`[確定(データ)]`）

- canvas: `CANVAS = 1280x960`、panel 原点 `PANEL_CANVAS = (100, 195)`、`ROW_HEIGHT = 93`、
  `WINDOW_WIDTH = 500`、`SCROLL_VISIBLE = 6`、`SCROLL_GUTTER = 40`（`race_sex_panel.h:293-317`）
- アプリのサーフェス: **1280x870**（実測。smoke ログ `Menu hit layout (screen 1280x870)`）
- `s = min(1280/1280, 870/960) = 0.90625`、`panelOrigin = (150.625, 176.719)`
- item の device 矩形: `x = panelOriginX`、`y = panelOriginY + (row*93 - scroll*93)*s`、
  `w = paneWidth(slot)*s`、`h = 93*s = 84.28`
  - `paneWidth = 500 - (itemCount > 6 ? 40 : 0)`（Main は 6 item なので 500）
- Done: canvas `(323, 591) 177x44`、id **90**
- スクロールバー: `itemCount > 6` のページのみ。canvas `(480, 25) 20x508`

## ページ別ウィジェット

状態の語彙: **実装済み**（描画＋期待挙動）/ **未実装**（描画はあるが挙動が無い）/
**不明**（期待挙動が未確定）。

### 1. Main（page id 0・slot +0x40・6 item）

| row | id | 型 | ラベル | device 中心 | 期待挙動 | 状態 |
|---|---|---|---|---|---|---|
| 0 | 201 | button | Race | (377, 219) | Race ページへ | 実装済み |
| 1 | 202 | button | Face | (377, 303) | Face ページへ | 実装済み |
| 2 | 203 | button | Hair | (377, 387) | Hair ページへ | 実装済み |
| 3 | 204 | toggle | Eyes | (377, 472) | 値を反転（ON/OFF で何が変わるか） | **未実装**（`handleClick(204)` は false、`toggleChecked()` は誰も呼ばない） |
| 4 | 205 | slider | Age | (377, 556) | ドラッグで 0..1 | 実装済み（値のみ。morph 未適用＝V-654-4） |
| 5 | 207 | slider | Complexion | (377, 640) | ドラッグで 0..1 | 同上 |

`205`/`207` は user0 slot として `206`/`208` を消費する。

### 2. Race（id 209・3 item）

| row | id | 型 | ラベル | 期待挙動 | 状態 |
|---|---|---|---|---|---|
| 0 | 210 | toggle | Gender | 値を反転（男女の切替） | **未実装**（FAMILY target=-1 で消費されるが値が反転しない） |
| 1 | 211 | toggle | Race | 左右タップで race を巡回（`cycleRaceFromTap`） | 実装済み |
| 2 | 212 | text | Race Description | 選択 race の説明文 | 実装済み |

### 3. Hair（id 213・6 item）

| row | id | 型 | ラベル | 期待挙動 | 状態 |
|---|---|---|---|---|---|
| 0 | 214 | toggle | Style | 値を反転（髪型の切替） | **未実装**（FAMILY 外） |
| 1 | 215 | toggle | Hair Color | 値を反転（色の切替） | **未実装**（FAMILY 外） |
| 2 | 216 | slider | Red | ドラッグ | 実装済み（値のみ） |
| 3 | 218 | slider | Green | ドラッグ | 実装済み（値のみ） |
| 4 | 220 | slider | Blue | ドラッグ | 実装済み（値のみ） |
| 5 | 222 | slider | Length | ドラッグ | 実装済み（値のみ） |

### 4. Face（id 224・4 item）

| row | id | 型 | ラベル | 期待挙動 | 状態 |
|---|---|---|---|---|---|
| 0 | 225 | button | Shape | Shape ページへ | 実装済み |
| 1 | 226 | button | Tone | Tone ページへ | 実装済み |
| 2 | 227 | button | Random | 確認ダイアログ→全スライダー乱数 | 実装済み（v663） |
| 3 | 228 | button | Reset | 確認ダイアログ→全スライダー 0 | 実装済み（v663） |

### 5. Shape（id 229・button 9）

`230 General(Face)` → 246 / `231 Brow` → 257 / `232 Cheeks` → 264 / `233 Chin` → 275 /
`234 Eyes` → 290 / `235 Forehead` → 299 / `236 Jaw` → 306 / `237 Mouth` → 315 /
`238 Nose` → 336。すべてページ遷移。**実装済み**。

### 6. Tone（id 239・button 6）

`240 General(Skin)` → 365 / `241 Beard` → 376 / `242 Eye shades` → 389 /
`243 Eyebrows` → 402 / `244 Mouth(Lips)` → 417 / `245 Nose shades` → 424。
すべてページ遷移。**実装済み**。

### 7–21. スライダー専用ページ（すべて 型=slider・期待=ドラッグで 0..1・状態=実装済み（値のみ））

| page | id | item（id は先頭のみ・+2 ずつ） |
|---|---|---|
| General(Face) | 246 | 247 Face brow/nose/chin ratio / 249 Face forehead/sellion/nose ratio / 251 Face heavy/light / 253 Face round/gaunt / 255 Face thin/wide |
| Brow | 257 | 258 Brow Ridge high/low / 260 Brow Ridge Inner up/down / 262 Brow Ridge Outer up/down |
| Cheeks | 264 | 265 Cheekbones low/high / 267 …shallow/pronounced / 269 …thin/wide / 271 Cheeks concave/convex / 273 Cheeks round/gaunt |
| Chin | 275 | 276 Chin forward/backward / 278 …pronounced/recessed / 280 …retracted/jutting / 282 …shallow/deep / 284 …small/large / 286 …tall/short / 288 …wide/thin |
| Eyes | 290 | 291 Eyes down/up / 293 Eyes small/large / 295 Eyes tilt inward/outward / 297 Eyes together/apart |
| Forehead | 299 | 300 Forehead small/large / 302 …tall/short / 304 …tilt forward/back |
| Jaw | 306 | 307 Jaw retracted/jutting / 309 Jaw wide/thin / 311 Jaw-Neck slope high/low / 313 Jawline concave/convex |
| Mouth | 315 | 316 Mouth drawn/pursed / 318 …happy/sad / 320 …high/low / 322 …lips deflated/inflated / 324 …lips large/small / 326 …lips puckered/retracted / 328 …protruding/retracted / 330 …tilt up/down / 332 …underbite/overbite / 334 …Chin distance short/long |
| Nose | 336 | 337 Nose bridge shallow/deep / 339 …bridge short/long / 341 …down/up / 343 …flat/pointed / 345 …nostril tilt down/up / 347 …nostrils small/large / 349 …nostrils wide/thin / 351 …region concave/convex / 353 …sellion down/up / 355 …sellion shallow/deep / 357 …sellion lower shallow/deep / 359 …sellion thin/wide / 361 …short/long / 363 …tilt down/up |
| General(Skin) | 365 | 366 Skin flushed/pale / 368 …shade dark/light / 370 …tint orange/blue / 372 …tint purple/yellow / 374 Cheek blush light/red |
| Beard | 376 | 377 Beard flushed/pale / 379 …light/dark / 381 …cheeks light/dark / 383 …circle light/dark / 385 …goatee light/dark / 387 …moustache light/dark |
| Eye shades | 389 | 390 Eye sockets bruised/bright / 392 …dark/light / 394 Eyelids light/dark / 396 …pale/red / 398 Eyeliner light/dark / 400 Eyeshadow light/dark |
| Eyebrows | 402 | 403 Eyebrows dark/light / 405 …low/high / 407 …thick/thin / 409 …very thin/thick / 411 …lower light/dark / 413 …outer light/dark / 415 …upper dark/light |
| Mouth(Lips) | 417 | 418 Lips flushed/pale / 420 Lipstick dark red/light blue / 422 Lipstick dark blue/light red |
| Nose shades | 424 | 425 Naso labial lines light/dark / 427 Nares small/large |

### 118 の外にある要素

| 要素 | 場所 | 期待挙動 | 状態 |
|---|---|---|---|
| `race_name`（名前ラベル・id 10） | canvas `(250, -65)` | プレイヤー名の表示と入力 | **表示のみ実装**（`setPlayerName`）。**入力は未配線**（#181） |
| Done（id 90） | canvas `(323, 591) 177x44` | 名前検証→確認→commit | 実装済み |

## 「不明」＝ Ghidra 確認項目（別節）

1. **各トグルの ON/OFF の意味**（5 つ）: Eyes 204 / Gender 210 / Style 214 / Hair Color 215
   （Race 211 は巡回で判明済み）。何が変わるか（trait、モーフ、テクスチャ、性別）が未確定。
   調べ方: `0xB39440`（V-653-1）と同じ手順 — 命令で `0x5CDF29` 系の user0 読み手と
   HandleClick の該当分岐を追う。
2. **スライダー → 顔モーフ軸の対応**（V-654-4）: 現在は builder 順 1:1 の近似で、
   実データ未確認。
3. **名前入力の原作挙動**: RE #123（組み込みエディタ・リアルタイム書込）は受領済みだが、
   Panel の `race_name` との結線は未確定。
4. **Age / Complexion の効果**: 実装済みは値の保持のみ。何を変えるかは未確定。

## 未確認

- 上記「不明」4 件はすべて **Ghidra / RE 待ち**。2026-11-27 に `ghidra-mcp`
  `list_instances` を実行 → `{"instances":[]}`（起動中インスタンス無し）で**確認できていない**。
- スライダー値が実際に描画へ効いているか（morph 未適用のため画素に出ない）は未確認。
