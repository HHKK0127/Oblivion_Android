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
- Gate before touching `rust/`: `cargo test --offline --workspace` run from `rust/` (baseline 176 passed / 31 suites) - the same command CI runs for the host archives. Per-crate details: `rust/AGENTS.md`; crate table: `rust/README.md`.
- Never move parse behaviour without the host oracle that pins it (`tmp/esm_oracle`, `tmp/nif_oracle` pattern; harnesses are local-only).

## Loop Protocol (ユーザー指示・2026-11-10 以降必須)

- **作製はオリジナルの作りを厳守**: 正典 [確定] ＋ 実データ検証分のみを実装する。推測実装は禁止（docs と code が矛盾するときは code + 実データ優先、という既存規則もここに従う）。
- **作製は並行して行う**: 複数レーンを同時に進める（コード変更は worktree 分離を維持）。
- **都度レビュー**: 各実装単位の直後に verifier サブエージェントでレビューを入れ、指摘を反映してから次に進む。
- **ループ運用**: 手が止まったら（人間ゲート待ちなど）次にできる項目へ移る。セッションが止まったら**数時間後に再度ループを再開**する（再開時は `STATE.md` と ai-todo の open タスクを確認して継続）。
- **解析は ReverseEngineering セッションへ依頼**: RE が必要な未確定事項は ai-todo に `#RE` タグで起票して委譲する（例: #122, #123）。推測で埋めない。
