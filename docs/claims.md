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

## 引き渡し（2026-11-24・Panel レーン）→ **同日 landing 済み**

`initialize()` の無描画修正と chargen の確定経路を **1 つの引き渡しに束ねて** Panel レーンへ渡した。**同日中に master へ landing した**（当方の作業ツリーの差分がそのまま取り込まれた）。

当初の引き渡し文:

```
claim: Panel レーン
対象: race_sex_panel.cpp の initialize()（無描画修正）＋ renderer.cpp の setOnCommit
内容: (a) initialize() で描画が成立すること (b) on_commit_ で Legacy と同順に
      HP/Magicka/Stamina/level/8属性/21スキルを適用し、最後に startGame() を呼ぶ
完了条件: 実機で New Game → chargen → Done → Yes → セル侵入まで到達し、その画面で golden #2 を採取
```

landing 後の実態（`git show HEAD:…` で確認）:
- (a) **済**: `renderer.cpp` の `setChargenUi` が `raceSexPanel->initialize();` → `init();` の順で呼ぶ。`instantiate()` も各 widget に `initialize()`。
- (b) **一部**: 共有 `app/src/main/cpp/game/chargen_commit.{h,cpp}` が追加され、`player.raceFormID` と `NpcManager::applyRaceToPlayer` まで実装。**ATTR ボーナスと health は未配線**（`V-664-2`/`V-664-3`・RE 待ち）で、**`startGame()` 到達の完了条件は未達**。
- 当方の未コミット差分（claims.md の本節・`docs/evidence/README.md`・`docs/reviews/2026-11-23-handoff.md` §9・`docs/smoke-tests/newgame.sh` の追加タップ・`docs/evidence/golden/newgame-no-esm-prefix.json`）は、landing 済み分を壊さない追補として別途コミットする。

根拠（当方の実測・2026-11-24）:
- `UIComponent::render()` は `if (!visible || !initialized) return;`（`ui_component.cpp:35`）で、`initialized` の既定は false（`ui_component.h:230`）。`RaceSexPanel` は `initialize()` を呼んでおらず、**chargen が 1 ピクセルも描かれない**（マーカーは出るので気づきにくい）。
- `UIComponent::initialize()` は `UIDrawHelper::initialize()`（GL）を呼ぶため、**`init()` ではなく renderer 側**（`setChargenUi`）で呼ぶこと。`init()` はホストテストが多数回呼ぶ（`race_sex_panel_tests.cpp:119` ほか）。
- 2026-11-24 09:53 のコミットが当方のビルド中に `renderer.cpp` を動かし、**同一ツリー同時編集**が実際に起きた。以後、当方は `race_sex_panel*` / `renderer.cpp` の chargen 配線を触らない。

## 滞留の棚卸し（この表を縮めるのが最優先の仕事）
上表の「未マージ（滞留）」は 7 ブランチ・21 コミット。上限 3 本に対して超過している。
推奨する処理順:

1. `chargen-handwritten` を**閉じる**（superseded。残す価値がある差分だけ cherry-pick）。
2. `title-intro` の 4 ブランチを 1 本にまとめてマージ（同一ファイル群）。
3. `loading` の 2 ブランチを 1 本にまとめてマージ。
4. `chargen-ime` をマージ。
5. 以後は「マージしてから次を始める」を守る。
