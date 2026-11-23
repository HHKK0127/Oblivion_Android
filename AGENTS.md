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

## Verification

- For L2+ changes, dispatch a verifier sub-agent after implementation.
- Run the project's documented tests before proposing a fix.
- Record test evidence in `STATE.md`.

When a repository's documentation and its existing code disagree, follow the conventions used in the existing code, and in your final reply point out each inconsistency you found between the documentation and the code.

## Rust workspace (vendored)

- The pure-logic layers behind the six C ABIs live in `rust/` at the repository root (15 crates; vendored from the former sibling `oblivion-re-tools` checkout on 2026-11-09). `app/src/main/cpp/CMakeLists.txt` builds them with cargo-ndk; `OBLIVION_RUST_DIR` / `OBLIVION_RUST_OFFLINE` (and the matching `-Poblivion.rust.*` Gradle properties) override defaults.
- Gate before touching `rust/`: `cargo test --offline --workspace` run from `rust/` (baseline 194 passed / 31 suites) - the same command CI runs for the host archives. Per-crate details: `rust/AGENTS.md`; crate table: `rust/README.md`.
- Never move parse behaviour without the host oracle that pins it (`tmp/esm_oracle`, `tmp/nif_oracle` pattern; harnesses are local-only).

## Loop Protocol (ユーザー指示・2026-11-10 以降必須)

- **作製はオリジナルの作りを厳守**: 正典 [確定] ＋ 実データ検証分のみを実装する。推測実装は禁止（docs と code が矛盾するときは code + 実データ優先、という既存規則もここに従う）。
- **作製は並行して行う**: 複数レーンを同時に進める（コード変更は worktree 分離を維持）。
- **都度レビュー**: 各実装単位の直後に verifier サブエージェントでレビューを入れ、指摘を反映してから次に進む。
- **ループ運用**: 手が止まったら（人間ゲート待ちなど）次にできる項目へ移る。セッションが止まったら**数時間後に再度ループを再開**する（再開時は `STATE.md` と ai-todo の open タスクを確認して継続）。
- **解析は ReverseEngineering セッションへ依頼**: RE が必要な未確定事項は ai-todo に `#RE` タグで起票して委譲する（例: #122, #123）。推測で埋めない。

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

## precheck（コミット前検査）

- コミット前に **`python scripts/precheck.py`** を実行する（引数なし＝git 変更ファイル、`--all`＝追跡下全体、ファイル列指定も可）。**終了コード 1 で失敗**。
- 検査 3 種: **CN**（簡体字専用字の明示リスト。`所以`・`使用`/`可能`/`点` 等の日本語語彙は除外）/ **U+FFFD**（置換文字）/ **dotAV**（ドット `.`/`．`/`。` の直後に `AV` が続く mojibake パターン）。
- 旧 precheck は `%TEMP%` にあり 2026-11-22 の掃除で消失した。以後は **`scripts/precheck.py` を正**とし、手動 3 検査は行わない。
- `scripts/precheck.py` 自身は CN リスト（簡体字）を含むため、スクリプトは自分自身を検査対象から除外する。
