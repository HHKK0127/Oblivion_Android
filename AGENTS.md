# AGENTS.md — Opencode Minimal Loop

These rules are loaded by opencode before loop work.

## Loop Mode

- Start in L1 report-only mode.
- Read `STATE.md` before any triage.
- Update `STATE.md` after every loop run.
- Do not edit source code until the human explicitly enables L2.

## Safety

- Never push or merge without human approval.
- Never edit `.env`, `.env.*`, `auth/`, `payments/`, `secrets/`, or `credentials/`.
- Use a git worktree for every code-changing attempt.
- Max 3 fix attempts per item; escalate after that.
- 3 回目では、試したこと・否定された仮説・次に必要な情報を書いて止まる。

### 5. Forbidden

もっともらしく見えるがこのプロジェクトを壊すもの。

- `loadedCount==0` での早期 return（BSA 無し配置で ESM 抽出を飛ばす）
- 共有 `target/` への MSVC ツールチェーン混在（E0786）
- ESM 必須マーカーだけに頼った配置判定
- Panel レーンの関数への無断接触: `race_sex_panel*` / `ui_toggle*` / `chargen_commit*` の全体、および `engine/renderer.cpp` の chargen 系（`seedChargenRaces` / `setChargenUi` と chargen 用 draw）。
- `engine/renderer.cpp` は共用ファイル。触る前に「どの関数か」を宣言する（#161 の実例: `createTestScenario` / `loadBSAArchives` / `maybeRetryGameDataLoad` / `onGameDataLoaded` / `ensureScenarioBuilt` / `render` / `setChargenUi`）。`precheck` は Panel 所有ファイルとこのファイルの変更を WARN する。
- 実測前の claims.md 更新
- claims.md の各行は「測った値・実行したコマンド・日付」の 3 点セット。推定を書かない。

## 0. 優先順位

Safety と Loop Mode は §0 より優先する。この節の規則は、以降の節より優先する。

1. 推測で埋めない。パス・ハッシュ・ログ行・件数・テスト結果は、貼る前に一度 grep する。確認できないものは「未確認」と書く。
2. 前提が違うと思ったら、作業前に言う。黙って従わない。
3. 仮説は実装前に 1 行で書く。書けないなら調べる。否定条件も添える。
4. 測っていないものを「通った」と書かない。plausibility は correctness ではない。
5. 差分の全行が依頼に遡れること。遡れない行は別コミットに切る。

## Verification

- For L2+ changes, dispatch a verifier sub-agent after implementation.
- Run the project's documented tests before proposing a fix.
- Record test evidence in `STATE.md`.
- verifier には依頼文・差分・合格条件を渡す。実装者の要約は渡さない（要約に引っ張られる）。
- 新しい計測器・ゲートを足したら、まず陰性対照を 1 回通す。壊した入力で落ちること（または落ちない理由）を確認する。測れない計測器は 0 件で緑になる。
- 実機確認は `pm clear` から既定経路を 3 run。エミュレータは 1280x960 / 240dpi / rotation 1 固定。
- 実機 run の logcat とスクショは `docs/evidence/<ticket>/` に残し、パスをコミット本文に書く。
- 通すために検証を弱めない。アサーション緩和、スコープ縮小、網羅率の切り下げ、チェックの skip、snapshot の再基準化で緑にしない。弱めたら「弱めた」と書く。チェック自体が誤りだと思うなら、チェック名と理由を書いて止まる。
- 前セッションの STATE.md、サブエージェントの報告、要約、チェック済みの TODO は主張であって証拠ではない。証明コマンドを再実行するまで繰り返さない。要約に「完了」とあっても再実行する。
- 証明コマンドは CI 設定・マニフェスト・タスクランナーから丸ごとコピーする。記憶で組み立てるとフラグが落ち、本物のゲートが出さない赤や緑を報告する。

## STATE.md

- STATE.md は追記でなく上書き。「現在地・次・詰まり・未確認」の 4 項目のみ。
- 役割分担: STATE.md = 現在地、claims.md = 実測事実、ai-todo = 起票。同じ内容を 2 箇所に書かない。

## Rust workspace (vendored)

- The pure-logic layers behind the six C ABIs live in `rust/` at the repository root (15 crates; vendored from the former sibling `oblivion-re-tools` checkout on 2026-11-09). `app/src/main/cpp/CMakeLists.txt` builds them with cargo-ndk; `OBLIVION_RUST_DIR` / `OBLIVION_RUST_OFFLINE` (and the matching `-Poblivion.rust.*` Gradle properties) override defaults.
- Gate before touching `rust/`: `cargo test --offline --workspace` run from `rust/` - the same command CI runs for the host archives. Per-crate details: `rust/AGENTS.md`; crate table: `rust/README.md`. The pass-count baseline lives in `STATE.md` (it rots here every time a test is added).
- Never move parse behaviour without the host oracle that pins it (`tmp/esm_oracle`, `tmp/nif_oracle` pattern; harnesses are local-only).

## Loop Protocol (ユーザー指示・2026-11-10 以降必須)

- **作製はオリジナルの作りを厳守**: 正典 [確定] ＋ 実データ検証分のみを実装する。推測実装は禁止（docs と code が矛盾するときは code + 実データ優先、という既存規則もここに従う）。
- **作製は並行して行う**: 複数レーンを同時に進める（コード変更は worktree 分離を維持）。
- **都度レビュー**: 各実装単位の直後に verifier サブエージェントでレビューを入れ、指摘を反映してから次に進む。
- **ループ運用**: 手が止まったら（人間ゲート待ちなど）次にできる項目へ移る。セッションが止まったら**数時間後に再度ループを再開**する（再開時は `STATE.md` と ai-todo の open タスクを確認して継続）。
- **解析は ReverseEngineering セッションへ依頼**: RE が必要な未確定事項は ai-todo に `#RE` タグで起票して委譲する（例: #122, #123）。推測で埋めない。
- 同一関数を両レーンで触る必要が出たら、先に landing させる側を決めて宣言する。両方で書かない。

## 算術・引用・根拠（再発防止）

- 範囲・個数・ストライド・終端の算術は暗算せず、python で検算してコマンドと出力を引用する。範囲は常に半開区間 `[start, end)` で書く。
- 引用は tool が実際に表示した文字列のみ。表示されていない値・記憶由来の値は「未確認」と明記する。
- 各結論に知り方（命令で確認 / データで確認 / 実行で観測 / 推測）を添える。判定語彙は 5 段階のまま変更しない。
  - （注意）この「知り方」は**判定列には使わない**。5 段階判定の列は不変とし、「知り方」は判定に至る**散文の中だけ**で使う（2 つの語彙を混ぜない）。
- 長い出力はファイルに落として slice で読む。全文を文脈に貼らない。
- 訂正時は旧記述を削除せず `[確定(命令)(negative)]` として残す（既存規則の再掲）。

## 作業ディレクトリ（probe/抽出物・再発防止）

- probe ハーネス（`v307_common.py`）・抽出 XML・裁定書は **`%TEMP%` に置かない**（掃除で消える。2026-11-22 に `%TEMP%\opencode` の `v307_common.py` と `bsa_chargen\*.xml` が消失）。`D:\re\` を使い、`D:\re\MANIFEST.sha256` で sha256 検証する。消えたら BSA（`Oblivion - Misc.bsa`）から再抽出し sha256 照合する。
- `race_sex_menu.xml` の正典: raw md5 `d367341334fa89101ba4fc077edb52fe` / 22156B / CRLF / 705 行（`D:\re\XML_CANONICAL.txt`）。
- `docs/evidence/fixtures/` 以外にバイナリ（BSA/ESM/DDS/NIF）を置かない。

## precheck（コミット前検査）

- コミット前に **`python scripts/precheck.py`** を実行する（引数なし＝git 変更ファイル、`--all`＝追跡下全体、ファイル列指定も可、`--strict`＝レーン WARN を FAIL に昇格）。**FAIL（テキスト検出・構造検査・`--all` の baseline 増加）は終了コード 2**。WARN は exit 0。
- テキスト検査 3 種: **CN**（簡体字専用字の明示リスト。`所以`・`使用`/`可能`/`点` 等の日本語語彙は除外）/ **U+FFFD**（置換文字）/ **dotAV**（**MSVC RTTI 名の `?` 落ち検出**。`\.\?AV<name>@@` は許可し、`?` が落ちた `\.AV<name>` を FAIL。2026-11-23 実測 `.?AV…`=1473 / `.AV…`=23）。
- 構造検査 3 種: **中間ファイル**（`.bak`/`.orig`/`.rej`/`*~`。追跡下＋未追跡の両方）/ **versionCode 単調増加**（変更された `build.gradle` を HEAD と比較。未変更は skip）/ **レーン**（`race_sex_panel*`・`ui_toggle*`・`chargen_commit*`・`engine/renderer.cpp` の変更を WARN。`--strict` で FAIL）。
- 旧 precheck は `%TEMP%` にあり 2026-11-22 の掃除で消失した。以後は **`scripts/precheck.py` を正**とし、手動検査は行わない。
- **CN リストの保守**: 手書きの簡体字リストは誤検出しやすい。除外判断のたびに **`python scripts/precheck.py --audit`**（`CN_CHARS ∩ 常用漢字`）を 1 回回し、交差があればその字を外す（例: 据 U+636E は常用漢字のため 2026-11-26 に除外）。常用漢字は `scripts/data/joyo_kanji.txt`（2136 字）。
- `scripts/precheck.py` 自身は CN リスト（簡体字）を含むため、スクリプトは自分自身を検査対象から除外する。

## 版番号（version）

- versionName は Release タグを切るときだけ変える。内部作業では上げない。
- versionCode の単調増加は `python scripts/precheck.py` が検査する（比較相手は常に HEAD。未変更ファイルは skip）。
- タグと Release が無い版番号を README に書かない。

## 1. 着手前

- 触るファイルと、それを呼ぶファイルを読む。
- 既存の書き方に合わせる。緑地で書き直さない。
- 2 案あるなら両方とトレードオフを出す。自明な 1 行修正は除く。
- 合格条件を先に書く。「動く」ではなく、コマンド・期待値・観測点で書く。
- 矛盾の裁定は種類で分ける。実データ／バイナリ由来の事実（レイアウト、値、挙動）は code が正とし、spec を後追いで直す。要求・既定値・受入条件は code が正とは限らないので、黙って寄せずチケットに起票して人間が決める。
- 自明・可逆（typo、ローカル変数名、ログ 1 行）は聞かずに進める。質問は注意を削るので、迷いの大きさで選ぶ。

## 2. 実装

- 依頼に無い機能・抽象化・設定項目を足さない。
- 自分の変更で不要になった import / 変数 / 関数は消す。既存の死コードは消さず報告に書く（掃除は独立チケット）。
- エラー処理は起きうる失敗にだけ書く。NIF/BSA の壊れたデータは起きうる側。
- 200 行が 50 行になるなら書き直す。

## 3. 検証

- 検証が落ちたら、テストではなく原因を直す。golden が赤いとき `--update` で焼き直す前に、赤い理由を 1 行書く。
- ログ・スタックトレースは最後まで読む。途中で読むのをやめると誤診する。
- 画面の変更は画素差分で示す（before / after と差分値）。
- 実機で見ていないものは「実機未確認」と明記する。ホスト緑を完了と呼ばない。
- 指標を出すとき、それが何を測っているかを 1 行添える。測っていない範囲を「100%」と書かない。
- 静的解析の成功を実行の成功と呼ばない。パースが通ったこととゲームが動いたことは別。実行を語るなら実行の証拠を出す。
- 「未確認」を放置して出荷しない。実機未確認の項目は、確認するか、壊れている前提で扱う。ホスト緑＋実機未確認は「動く」ではない。

## 4. 報告

- 見出しの数だけ事実を書く。埋まらない見出しを作らない。
- 数値には出典（実行したコマンド / ログ行）を併記する。
- 失敗・未実施・判断待ちを、成功と同じ位置に書く。末尾に小さくまとめない。

## 6. Learnings の運用

- 訂正されたら、終了前に 1 行追記する。抽象（「〜に注意」）ではなく具体（「〜のときは必ず〜する」）で書く。
- 既存行で足りるなら追記せず、その行を締める。
- 数週間ごとに削る。各行に「消したらミスが起きるか」を問い、起きない行を消す。
- 上限 300 行。超えたら削るのが先、足すのが後。
- ドメイン知識（症状/原因/手順/確認方法）は `docs/gotchas.md` に置く。ここ（Learnings）は振る舞いだけ。
