# docs/claims.md — 占有の宣言（WIP 上限）

## なぜ必要か

エージェントは書くのが速いので「書くコスト」はほぼゼロに見える。だから同じ機能を 2 回書くことも、
ブランチを滞留させたまま次を始めることも「タダ」に見える。しかし本当に高い資源は
**人間の注意とマージ帯域**で、そこは 1 本のまま。

2026-11 の実例: chargen を 2 レーン（手書き21ページ／widget 化）で別々に実装し、片方は 21 コミット
滞留した。さらに 2026-11-24 には、当方が作った修正が**渡す前に**並行レーンへ landing 済みだった
（同一作業ツリーの同時編集）。

## ルール

1. **作業を始める前に `git log --oneline -20` と `git status` で master を確認する。**
   これで「作ってから気づく」が「見てから決める」になる。
2. **着手前に宣言する。** 新しい作業を始める前に、下の表へ 1 行足す（lane / owner / paths / branch）。
3. **WIP 上限**: 未マージブランチは **レーンあたり 1 本、全体で 3 本**まで。
4. **新しい作業はマージ後にしか始めない。** 上限に達しているときは、まずマージかクローズを決める。
5. **同じ paths を 2 レーンが宣言しない。** 重複が必要なら、先に統合順を決めて片方を閉じる。
6. **完了の定義**は「ホスト緑＋verifier 承認」ではなく「**実機で確認済み＋master にマージ済み**」。
   ホストテストは完了条件の一部であって、完了そのものではない。
7. **claim には開いている項目だけを書く。** 済んだ項目を残すと初日で嘘の文書になる。
8. **CI の `wip-limit` は当面 warn-only**（`continue-on-error: true`、`check_wip_limit.py` は既定で exit 0）。
   滞留が上限内（3 本以下）に収まった時点で `continue-on-error` を外し `--strict` で blocking にする。

機械チェック: `python scripts/check_wip_limit.py --overlap`
（未マージ本数が上限を超えたら警告。`--strict` で終了コード 1。CI は
`.github/workflows/android.yml` の `wip-limit` ジョブで実行する。）

## 宣言の書式

| lane | owner | paths（glob） | branch | status |
|---|---|---|---|---|
| （機能名） | （セッション） | （触るパス） | （ブランチ） | active / blocked / superseded / merged |

## 現在の占有

| lane | owner | paths（glob） | branch | status |
|---|---|---|---|---|
| chargen-panel | 並行セッション | `app/src/main/cpp/ui/race_sex_panel*`, `app/src/main/cpp/ui/ui_toggle*`, `app/src/main/cpp/game/chargen_commit*`（`renderer.cpp` の chargen 配線は #161 行へ引き渡し済み） | master 済み | active（下記の残のみ） |
| ci-gates / golden | 本セッション→並行レーンが landing | `scripts/**`, `docs/evidence/**`, `docs/smoke-tests/**`, `.github/workflows/**`, `docs/claims.md` | master 済み | merged |
| re | RE セッション | `tmp/re/**`, `D:\re\**` | —（成果は master へ `re vXXX:` コミット） | active |
| real-data-gates | 本セッション | `scripts/run_fixture_gates.sh`, `.github/workflows/real-data-gates.yml`, `tools/host_tests/{make_fixture_dds.py,fixture_parse_main.cpp,run_fixture_parse.sh}`, `docs/evidence/fixtures/minimal.dds`（BSA/NIF はこれから） | 着手中 | active。**DDS はローカル検証済み**（陽性: 16x16/mips1/DXT1/128B 一致、陰性: マジックを潰すと消費者が落ちる。検証者 = Git Bash＋GNU ツールチェーン）。**BSA/NIF は未着手**。**Panel レーンの `make_fixture_esm.py` / `newgame_smoke.cpp` / `run_smoke.sh` / `android.yml` は触らない**（同レーンが landing 済み）。`run_host_tests.sh` には本レーンが 2 行（`build_host_rust.sh` 呼出＋`-lsynchronization`）を入れた（Panel 非干渉） |
| #161-loadbsa | 本セッション | `app/src/main/cpp/engine/renderer.cpp`, `renderer.h`（`loadBSAArchives()`/`maybeRetryGameDataLoad()`/`seedChargenRaces()` ＋ `setChargenUi()`/`setOnCommit()`）＋ `game/chargen_commit.cpp`（commit の HP/属性ログ 1 行のみ） | 未コミット（master 直） | active。**実機 3 run 検証済み**（emulator-5554）: ①ESM のみ＝`Loaded 0/22`・`Extracted Oblivion.esm`・`RACE=15`・`Record 1112136`、Done→Yes で `Race applied to player: Argonian (skillBonuses=7, spells=3)`・`chargen commit applied: race=0x00023FE9 hp=100/100 str=40 end=40`・`startGame`＋世界/UI。②ESM+`Oblivion - Meshes.bsa`＝`Loaded 1/22`・`Extracted`・`Loaded file from BSA *.nif`×5・`Mesh uploaded`×3。③`pm clear`＝ハングせず・`Oblivion.esm not found`＋`Retrying game data load`・ダイアログ1回・`chargen commit blocked`・`startGame` 出ず。`str/end=40`・`hp=100` は base＝`V-664-2` の実機証拠。late-load 固有の残点は #161 note（NPC/クエスト重複・リトライ上限後の復帰・`gameDataMissing_` リセット時期）。Panel レーンの欠陥は #180/#181/#182 に起票 |

### Panel レーンに残っている項目（当方は `race_sex_panel*` / `ui_toggle*` / `chargen_commit*` を触らない。`renderer.cpp` は #161 行が担当）

| 区分 | 項目 |
|---|---|
| **残** | `V-664-2` / `V-664-3`: `on_commit_`（共有 `game/chargen_commit.cpp`）で Legacy と同順に **HP/Magicka/Stamina/level/8 属性/21 スキル**を適用する。**`startGame()` は #161 行が `renderer.cpp` の `setOnCommit`（Panel 経路）に入れた＝landed。Panel レーンは同じ 1 行を書かないこと（二重実装の回避）**。完了条件は実機でセル侵入まで到達し、その画面で golden #2 を採取（golden #1 は V-664 完了後に再採取して `newgame-no-esm.json` へ改名） |
| **残** | **`UIToggle` にクリック処理が無い**（`ui_toggle.cpp` は 31 行で `render()` のみ）。影響は race_sex_panel の **5 トグル全部**（Eyes 204 / Gender 210 / Race 211 / Style 214 / Hair Color 215。race_sex_panel 以外の消費者は 0）。**2 種に分けること**: (1) **bool 反転** = 204, 210（widget 内で `checked` をトグル）(2) **index 巡回** = 211, 214, 215（左右半分で ±1、既存 `cycleRaceFromTap` と同経路。**widget 内で反転しない**）。汎用の反転を足すと 211 で反転と巡回が二重に走る（値が bool でなく、`handleClick` が 211 を先に消費するため順序次第で握り潰し）。完了条件: 実機で Eyes(122,379) と Gender を押して画素変化が出ること／211・214・215 が二重処理にならないこと（Race 押下で index が 2 進まない） |
| **済** | `initialize()` の無描画修正（landed 済み・`2e550a68`） |
| **済** | `bad_weak_ptr` クラッシュ（`shared_ptr` 所有・landed 済み・`0e9b8aef`） |

## 環境の罠（再発防止・2026-11-25 実測）

**症状**: ホスト Rust を GNU で組むと
`error[E0786]: found invalid metadata files for crate 'thiserror_impl'`（`invalid metadata version found: ...\build\thiserror-impl\<hash>\thiserror_impl-<hash>.dll`）。
**原因**: 共有 `target/` に**別ツールチェーン**（scoop=MSVC）の proc-macro 成果物が残っていた。
アーティファクトの種類ではなく **target ディレクトリの混在**が原因。`--target` を付けても
proc-macro は `target/<dir>/release` 側に出るため、同じ dir を共有すると再発する。

**対処**: `scripts/build_host_rust.sh` に集約（toolchain/RUSTC を rustup の GNU 実体へ固定・
`--target x86_64-pc-windows-gnu`・`CARGO_TARGET_DIR=target/gnu`・ビルド前にその dir だけ clean）。
既に `lib*_ffi.a` があれば作り直さない。

**規則**: 既定の `target/` を GNU ホストビルドに使わない（MSVC と同じ dir を共有しない）。

**症状 2（MinGW のリンク）**: ホストのリンクが
`undefined reference to 'WaitOnAddress' / 'WakeByAddressSingle'`（約 50 件）で落ちる。
**原因**: Rust std の Windows 同期プリミティブは MinGW では `libkernel32.a` ではなく
**`libsynchronization.a`** にある。`run_host_tests.sh` の `EXTRA_LIBS` に入っていなかった。
**対処**: MinGW の `EXTRA_LIBS` に **`-lsynchronization`** を追加（実測 2026-11-25:
追加前 308 undefined → 追加後 0、`dds-ffi 0.1.0` が起動）。

**配線（本レーンが実装・Panel 非干渉）**: `tools/host_tests/run_host_tests.sh` に
(1) アーカイブ探索の前に `bash scripts/build_host_rust.sh >&2`（`OBLIVION_RUST_SKIP_BUILD` で無効化可）、
(2) `EXTRA_LIBS` へ `-lsynchronization`。`run_fixture_parse.sh` からも同スクリプトを呼ぶ。

## 滞留の棚卸し（この表を縮めるのが最優先の仕事）

未マージブランチの棚卸しは `python scripts/check_wip_limit.py --overlap` が列挙する。
