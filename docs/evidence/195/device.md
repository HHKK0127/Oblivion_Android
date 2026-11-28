# #195 実機確認 — UIToggle の index 巡回（Hair 214/215・Race 211）

- **日付**: 2026-11-28
- **端末**: emulator-5554（`sdk_gphone64_x86_64`）
- **画面**: `wm size` 実出力 = `Physical size: 1920x1200` / `Override size: 1280x960`。`wm density` = `Physical density: 240`。`user_rotation 1` / `accelerometer_rotation 0`。＝ `#204` と同じ変換（`s=0.90625`, origin=(150.625,176.719)）に固定。
- **APK の鮮度**: `app/build/outputs/apk/debug/app-debug.apk` の mtime = `2026-11-28 22:27:00`（ビルド開始 22:17 より後）。端末 `dumpsys package com.example.oblivion` の `lastUpdateTime=2026-11-28 13:27:31`（端末 TZ、ホスト 22:27 相当）＝ 導入済みは新ビルド。`BUILD SUCCESSFUL in 9m 55s`（renderer.cpp もコンパイル通過）。

## 手順（座標は device px）

| 操作 | 座標 | 結果 |
|---|---|---|
| タイトル New | (485, 682) | `Menu selection: New Game`（logcat） |
| ESM ロード | — | `Loading game data...` → chargen Panel（Main） |
| Main → Hair | (377, 387) | Hair ページへ |
| 214 左半分 | (300, 219) | row0 のマーカーがセル1へ |
| 214 右半分 | (500, 219) | row0 のマーカーがセル0へ |
| Done | (523, 732) | Main へ |
| Main → Race | (377, 219) | Race ページへ |
| 211 左半分 | (264, 303) | 説明文が末尾 race（Imperial）へ |

## 結果（画素解析・PIL）

マーカー（青）の x 範囲を行ごとに実測:

| 画像 | row0（214 Style） | row1（215 HairColor） |
|---|---|---|
| 初期 | (127, 250) = セル0 | (127, 250) = セル0 |
| 214 左タップ | **(251, 374) = セル1** | (127, 250) 不変 |
| 214 右タップ | **(127, 250) = セル0** | (127, 250) 不変 |

- **214 は 0→1→0 で往復**（span 0..1 の wrap）。**215 は独立**（触っていないので不変）。
- **211 の wrap**: 初期の説明文は Argonian（先頭 race）、`(264,303)` を 1 回タップ後は Imperial（末尾 race）に変化。**先頭で左 → 末尾へ回り込む**を確認。

## 合格条件との対応

| 条件 | 結果 |
|---|---|
| 214/215 の行に左右の矢印とマーカーが出る | **✓**（row0/row1 に矢印＋青マーカー） |
| 214 を左→右でマーカーが 0↔1 で動く | **✓**（画素で 0→1→0） |
| 211 で先頭のまま左 → 末尾に回り込む | **✓**（説明文 Argonian → Imperial） |

## 未確認・残（申し送り）

- **211 のマーカーが動かない（要修正候補）**: `RaceSexPanel::render()` は index cycler のマーカーを `Element.index` から描くが、**211 の index は `selected_race_index_` にあり `Element.index` は常に 0**。よって Race トグルのマーカーは選択が変わってもセル0のまま（説明文と race 名は正しく変わる）。修正案: `render()` で `RACE_TOGGLE_ID` だけ `selected_race_index_` / `raceCount()` を使う（2 行）。
- **toggle widget の描画矩形が item 矩形と不一致（要調査）**: 実測で 214/215 の widget は x≈100..401（w≈301）、矢印高≈53。一方 `itemRectDevice` の計算は x=150.6, w=453, h=84。タップの当たり判定（`hitTestScreen`）は item 矩形側で正しく動く（row が一致）が、**描画側の位置/幅が合っていない**。`UIToggle` の `size`/`getAbsolutePosition()` のどこでずれるか未特定。
- 実機で見えているのは placeholder の四角/バー（最終アートは後続）。
