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
| real-data-gates | 本セッション | `scripts/run_fixture_gates.sh`, `.github/workflows/real-data-gates.yml`, `tools/host_tests/{make_fixture_dds.py,make_fixture_nif.py,make_fixture_bsa.py,fixture_parse_main.cpp,run_fixture_parse.sh}`, `docs/evidence/fixtures/minimal.{dds,nif,bsa}` | **済（d765ab2c）** | **ローカル検証済み＋CI green**（`real-data-gates` success）。陽性: DDS 16x16/mips1/DXT1/128B・NIF blocks=2/vertices=3/triangles=1・BSA→NIF opened/files=1/found/extracted/nifBlocks=2/vertices=3。**陰性 4 件**（DDS/NIF magic・NIF 型差し替え・BSA magic）が期待の落ち方で落ちる。NIF 生成器は `nif-core` の承認テストと **SHA256 一致**（`7EE12A60…`）。**Panel レーンの `make_fixture_esm.py`/`newgame_smoke.cpp`/`run_smoke.sh`/`android.yml` は触らない**。`run_host_tests.sh` には本レーンが 2 行（`build_host_rust.sh` 呼出＋`-lsynchronization`）を入れた（Panel 非干渉）。**限界**: 合成はパーサ挙動の固定のみ・実データ互換は証明しない |
| #161-loadbsa | 本セッション | `app/src/main/cpp/engine/renderer.cpp`, `renderer.h`（`loadBSAArchives()`/`maybeRetryGameDataLoad()`/`seedChargenRaces()` ＋ `setChargenUi()`/`setOnCommit()`）＋ `game/chargen_commit.cpp`（commit の HP/属性ログ 1 行のみ） | master（269e594c） | **済**。実機 3 run 検証済み（emulator-5554）: ①ESM のみ＝`Loaded 0/22`・`Extracted Oblivion.esm`・`RACE=15`・`Record 1112136`、Done→Yes で `Race applied to player: Argonian (skillBonuses=7, spells=3)`・`chargen commit applied: race=0x00023FE9 hp=100/100 str=40 end=40`・`startGame`＋世界/UI。②ESM+`Oblivion - Meshes.bsa`＝`Loaded 1/22`・`Extracted`・`Loaded file from BSA *.nif`×5・`Mesh uploaded`×3。③`pm clear`＝ハングせず・`Oblivion.esm not found`＋`Retrying game data load`・ダイアログ1回・`chargen commit blocked`・`startGame` 出ず。`str/end=40`・`hp=100` は base（#204 以降は `str/end` が race `ATTR` の性別行に変わる）。late-load 固有の残点は #161 note（NPC/クエスト重複・リトライ上限後の復帰・`gameDataMissing_` リセット時期）。Panel レーンの欠陥は #180/#181/#182 に起票 |
| #184-fix / late-load / 版番号 | 本セッション | `scripts/build_host_rust.sh`, `app/src/main/cpp/engine/renderer.cpp`（late-load 重複）, `app/build.gradle`, `README.md`, `CHANGELOG.md`, `RELEASE_NOTES.md`, `.github/workflows/android.yml`, `AGENTS.md`, `docs/gotchas.md`, `STATE.md` | **済** | #184（`.a` 再ビルドの E0463＝HOME 不一致で RUSTC 未設定＋scoop cargo）＝`31685060`、ゲート 2 回 exit 0。late-load 重複＝実機 A/B PASS（再入で重複なし・ESM 常在でテスト NPC 生成スキップ・NPC 2→105）。版番号＝`0.1.0`/`1000`＋CI 整合ゲートを導出等価から floor へ。AGENTS に §0〜§6＋`docs/gotchas.md` |
| #183 golden 再採取 | 本セッション | `scripts/compare_golden.py`, `docs/smoke-tests/newgame.sh`, `docs/evidence/golden/newgame-no-esm-prefix.json` | **済（`8a8ca792`・2026-11-27）** | **この行は `8c33a1b6` の記録（ESM 条件で採取・10.4%・#161 後で想定どおり）を置き換える。`8c33a1b6` は履歴に残す（誤記録の経緯ごと `git log` で追えるように）。** 並行レーンの未コミット WIP 3 ファイル（mtime 31h・停滞）を `wip/golden-lane`（`1f575195`）に保全し、退行（CI 依存の JSON 比較経路の欠落）を復元して feature を `22a295df` で着地。**いったん ESM 有りで採取（`2c46b359`）が条件違い（golden の前提は no-ESM）と判明→revert（`4d64da15`）**。原因は条件が golden にも CLI にも無かったこと。恒久対策: `newgame.sh --no-esm`（ゲームデータを run-as で一時退避・復元・ログ検証）＋`compare_golden.py` の `meta.esm`／`--require esm=absent`（`f076d198`）。no-ESM で採り直し（`8a8ca792`）: 旧 golden（`ee256c96`・#161 前）との差 508/576 (88.2%) は `269e594c`（#161）が `seedChargenRaces` に足した「Game data was not found」ダイアログが主因（「#161 ではなく startGame/Unchanged」という前提は不正確）。run 2・run 3 とも `mean=0.00/0.01 changed=0/576` の 2 run 連続一致、`--require esm=absent` 通過。実機 emulator-5554 / 1280x960 / 240dpi / rotation 1 / Git Bash。終了コードも規約どおり（`c7802781`: ゲート失敗=2 / ツール異常=1）。**限界**: `Default path smoke` は CI で skip（hypervisor 無し）なので 2 run 一致はローカル証拠のみ |

### Panel レーンに残っている項目（当方は `race_sex_panel*` / `ui_toggle*` / `chargen_commit*` を触らない。`renderer.cpp` は #161 行が担当）

| 区分 | 項目 |
|---|---|
| **残** | `V-664-3`: `on_commit_`（共有 `game/chargen_commit.cpp`）で Legacy と同順に **race 由来の health** を適用する（`V-664-3` の式待ち）。**属性・skill・HP/MP/SP の base 適用は #204 で `applyRace` が実施済み**（属性は RACE `ATTR` の性別行を代入。`V-664-2` close）。**`startGame()` は #161 行が `renderer.cpp` の `setOnCommit`（Panel 経路）に入れた＝landed。Panel レーンは同じ 1 行を書かないこと（二重実装の回避）**。完了条件は実機でセル侵入まで到達し、その画面で golden #2 を採取（golden #1 は V-664 完了後に再採取して `newgame-no-esm.json` へ改名） |
| **一部済 / 残** | **`UIToggle` のクリック処理**（`ui_toggle.cpp` は 31 行で `render()` のみ）。**bool 反転 = 204 Eyes / 210 Gender は実装済み**（2026-11-27・`RaceSexPanel::handleClick`。host テスト＋実機 probe。Gender は `CommitInput.gender` → `Player::gender` まで通した）。**index 巡回 = 211 Race は spec §10.4 で確定**（`race_template_toggle_left/_right` で ±1、既存 `cycleRaceFromTap`）／**214 Style・215 Hair Color の扱いは不明**（spec §10 に 214/215 の記述が無い。claims の「巡回」は実装の無い仕様記述なので決め打ちしない。bool だった場合に逆の実装を積む。**`#201`（RE）で確定してから実装**）。`handleClick` は 204/210 だけ反転する（211 は renderer が `cycleRaceFromTap` へ回すので二重処理にならない）。`#195` は 214/215 に呼び出し元が付いた時点で閉じる。完了条件: 実機で Eyes/Gender を押して画素変化が出ること／211・214/215 が二重処理にならないこと（Race 押下で index が 2 進まない） |
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
