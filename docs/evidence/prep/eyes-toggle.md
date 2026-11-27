# V-631-1b Eyes トグルの素材

対象は **chargen のトグル 5 つ**（spec §2: toggle 5）。実装はしない。

## 1. 5 トグルの識別子・座標・期待値

device 座標は §共通レイアウト（`s = 0.90625`、`panelOrigin = (150.625, 176.719)`、
`h = 84.28`）。Race / Hair ページの row 0/1 は Main と同じ device 矩形になる。

| # | page | id | ラベル | row | device 中心 | 期待挙動（ON/OFF で何が変わるか） | 状態 |
|---|---|---|---|---|---|---|---|
| 1 | Main | **204** | Eyes | 3 | (377, 472) | **未確定** | **未実装**（`handleClick(204)` は false、値も反転しない） |
| 2 | Race | **210** | Gender | 0 | (377, 219) | **未確定**（男女の切替と推定されるが未計測） | **未実装**（FAMILY target=-1 で消費されるが値は反転しない） |
| 3 | Race | **211** | Race | 1 | (377, 303) | 左右タップで race を巡回（ラベルに選択 race 名） | 実装済み（`cycleRaceFromTap`） |
| 4 | Hair | **214** | Style | 0 | (377, 219) | **未確定** | **未実装**（FAMILY 外） |
| 5 | Hair | **215** | Hair Color | 1 | (377, 303) | **未確定** | **未実装**（FAMILY 外） |

FAMILY（`race_sex_panel.h:105-135`）に入るトグルは **210 Gender（target=-1）だけ**。
204 / 214 / 215 は FAMILY 外で `handleClick` が false を返す。211 は renderer が
`RACE_TOGGLE_ID` を特別扱いする。

## 2. 反転の実装点（file:line）

| 役割 | 場所 | 現状 |
|---|---|---|
| 値の反転 API | `ui/ui_toggle.h:23` `void toggleChecked() { checked_ = !checked_; }` | **呼び出し元が 0 件**（renderer も panel も呼ばない。`tests/race_sex_panel_tests.cpp:383,385` のみ） |
| 値の設定 API | `ui/ui_toggle.h:21` `setChecked(bool)` | `race_sex_panel_instantiator.cpp:64` が instantiate 時に**一方向**で呼ぶだけ |
| 記述子の状態 | `ui/race_sex_panel.h:165` `bool checked = false;`（`Element`） | 初期値 false 固定。`add_toggle` に checked 引数は無い |
| パネル側の参照 | `ui/race_sex_panel_instantiator.cpp:148-151` | `RACE_TOGGLE_ID`(211) のときだけラベルを設定。**checked は読まない** |
| タッチ経路 | `engine/renderer.cpp:8368-8389` | `hitTestScreen`→`handleClick(id)` を呼ぶだけ。トグルなら反転、という分岐が無い |
| メニューハンドラ | `ui/race_sex_panel.cpp:427-442` | FAMILY 不一致は false（ページ不変）。トグルの値は「widget 側の責務」とコメントされている（`:438-441`） |

**つまり**: 「値の反転は widget 側の責務」と spec は言うが、**widget 側の反転を呼ぶ
コードが存在しない**。これが V-631-1b の実体。

反転を入れる場所の候補は 2 つ（実装時に選ぶ）:
1. `renderer.cpp` の Panel タッチ分岐で、`hitTestScreen` が toggle を返したら
   `toggleChecked()` を呼ぶ（描画の即時性が高い）。
2. `RaceSexPanel::handleClick(id)` の中で、`Element.type == Toggle` なら
   `e.checked = !e.checked` にして `widget` へ反映する（host テストで検証できる）。

どちらも **Panel 所有ファイル**に触るため、実装前に宣言が要る。

## 3. Ghidra で確認したいこと（未実施）

- `0xB39440`（V-653-1 と同じ調べ方）周辺の**トグル処理**: 各トグルの ON/OFF が
  どの trait / グローバル / モーフへ書かれるか。特に 204 Eyes は `0x5CC4A5` の
  chain 判定と `0x5CD3FC`/`0x5CD459` の (Shape,Eyes) tile だけが `[0xB38F90]` を
  参照する（v662）。ON/OFF の意味は未確定。
- HandleClick `0x5CC200` の arg1 分岐（90/10/98 のみが即値比較、v660）に
  204/210/214/215 が無いことの再確認。

**2026-11-27 に `ghidra-mcp` `list_instances` を実行 → `{"instances":[]}`
（起動中インスタンス無し）。Ghidra 確認は未実施。**

## 4. 完了条件との対応

「計測器（前提 2）で 5 トグルすべての差分が採れる」:
- 計測器は `--tap x y` でタップ前後の画素差分を出す。5 トグルの device 中心座標は
  上表のとおり算出済みで、**すべて計測可能**。
- 現ビルドの期待値: 204 / 210 / 214 / 215 は**反転しないので 0**、211 はラベルが
  変わるので**小さな非 0**（race 名の文字数に依存）。
- **ON/OFF で何が変わるか（期待値の内容）は §3 が未確定**なので、いまは
  「差分が採れる」ところまで。実装後の期待値は RE で埋める。

## 未確認

- 5 トグルの ON/OFF の意味（§3・Ghidra 未実施）
- Gender 210 が男女切替だという推定は未計測
