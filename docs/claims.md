# docs/claims.md — 占有の宣言（WIP 上限）

## なぜ必要か

エージェントは書くのが速いので「書くコスト」はほぼゼロに見える。だから同じ機能を 2 回書くことも、
ブランチを滞留させたまま次を始めることも「タダ」に見える。しかし本当に高い資源は
**人間の注意とマージ帯域**で、そこは 1 本のまま。

2026-11 の実例: chargen を 2 レーン（手書き21ページ／widget 化）で別々に実装し、片方は 21 コミット
滞留した。直接原因は「どのセッションがどのファイルを触るか」の宣言が無かったこと。

## ルール

1. **着手前に宣言する。** 新しい作業を始める前に、下の表へ 1 行足す（lane / owner / paths / branch）。
2. **WIP 上限**: 未マージブランチは **レーンあたり 1 本、全体で 3 本**まで。
3. **新しい作業はマージ後にしか始めない。** 上限に達しているときは、まずマージかクローズを決める。
4. **同じ paths を 2 レーンが宣言しない。** 重複が必要なら、先に統合順を決めて片方を閉じる。
5. **完了の定義**は「ホスト緑＋verifier 承認」ではなく「**実機で確認済み＋master にマージ済み**」。
   ホストテストは完了条件の一部であって、完了そのものではない。
6. **CI の `wip-limit` は当面 warn-only**（`continue-on-error: true`、`check_wip_limit.py` は既定で exit 0）。
   未マージ 33 本に対して上限 3 なので、初回から必ず超過する。33 本を 3 本に減らすのは 30 回の
   マージで、ゴールデン採取とは時間軸が違う。加えて `release.yml` は「緑の CI」を要求するため、
   ここを blocking にするとリリースも止まる。
   **滞留が上限内（3 本以下）に収まった時点で `continue-on-error` を外し、`--strict` を付けて
   blocking に切り替える。** それまでは `::warning::` の注釈だけを残す。

機械チェック: `python scripts/check_wip_limit.py --overlap`
（未マージ本数が上限を超えたら警告。`--strict` で終了コード 1。CI は
`.github/workflows/android.yml` の `wip-limit` ジョブで実行する。）

## 宣言の書式

| lane | owner | paths（glob） | branch | status |
|---|---|---|---|---|
| （機能名） | （セッション） | （触るパス） | （ブランチ） | active / blocked / superseded / merged |

## 現在の占有（2026-11-23 時点）

| lane | owner | paths（glob） | branch | status |
|---|---|---|---|---|
| chargen-panel | 並行セッション | `app/src/main/cpp/ui/race_sex_panel*`, `app/src/main/cpp/tests/race_sex_panel_tests.cpp`, `docs/chargen_ui_spec.md` | master 済み | merged（既定 `ChargenUi::Panel`・実験的） |
| chargen-handwritten | 本セッション | `app/src/main/cpp/ui/ui_character_creation.*` | `feat-chargen-21-pages` | **superseded（chargen-panel に置換。閉じる）** |
| loading | 本セッション | `app/src/main/cpp/ui/loading_screen.*` | `feat-loading-canon`, `feat-loading-menu` | 未マージ（滞留） |
| title-intro | 本セッション | `app/src/main/cpp/ui/title_screen.*`, `app/src/main/java/com/example/oblivion/GameRenderer.kt`, `AssetExtractor.kt` | `fix-title-video-166`, `fix-chargen-intro-movie`, `fix-title-video-assets-fallback`, `fix-title-touch-preload` | 未マージ（滞留） |
| chargen-ime | 本セッション | `app/src/main/jni_bridge.cpp`, `CharacterNameBridge.kt` | `fix-chargen-ime-inset` | 未マージ（滞留） |
| re | RE セッション | `tmp/re/**`, `D:\re\**` | —（成果は master へ `re vXXX:` コミット） | active |
| process | 本セッション | `scripts/**`, `docs/claims.md`, `docs/CI_PORT_FROM_BYROREDUX.md`, `docs/smoke-tests/**`, `.github/workflows/**` | 未コミット | active |

## 滞留の棚卸し（この表を縮めるのが最優先の仕事）

上表の「未マージ（滞留）」は 7 ブランチ・21 コミット。上限 3 本に対して超過している。
推奨する処理順:

1. `chargen-handwritten` を**閉じる**（superseded。残す価値がある差分だけ cherry-pick）。
2. `title-intro` の 4 ブランチを 1 本にまとめてマージ（同一ファイル群）。
3. `loading` の 2 ブランチを 1 本にまとめてマージ。
4. `chargen-ime` をマージ。
5. 以後は「マージしてから次を始める」を守る。
