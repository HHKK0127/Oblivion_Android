# Loop Configuration — Minimal Triage (Opencode)

## Active Loops

| Pattern | Cadence | Status | Command |
|---------|---------|--------|---------|
| Daily Triage | 1d | L1 report-only | `opencode run "Run loop-triage" --agent loop-triage` via cron/systemd |
| RE Serial Loop | manual (session) | L1 report-only（tmp/re + STATE.md + ai-todo のみ） | セッション内で手動反復: 残タスク並行対応 → Graphify でデータ関係性把握 → Review で解析要否判定 → 要解析ならおすすめ順に並行対応 / 不要ならループ停止 |

### RE Serial Loop ルール（2026-11-05 追加・ユーザー指示）

1. **残タスクを並行して対応する**（サブエージェント 1 回試行→upstream error で inline フォールバック。LOOP.md Budget の L1 サブエージェント 0 制約は inline 化で実質遵守）
2. **Graphify を確認してデータ同士の関係性を把握する**（`graphify query` を成果物群・台帳に対して実行し、孤立ノード/未接続残点を検出）
3. **完了後に Review を行い解析が必要な所を探す**
4. **あればおすすめ順に並行対応する、なければループを停止する**（停止時は STATE.md に停止理由を記録）
5. 各ラウンド共通: round_verify.py ALL OK 必須 / bak 連番 / ai-todo note→complete→start / STATE.md 実行ログ / supermemory 5 タイミング登録

## Human Gates

- No auto-fix until L2 checklist complete.
- All high-risk paths require human review (see docs/safety.md denylist).

## Worktrees

- Use an explicit `git worktree` and run opencode with `--dir <worktree>` for implementer runs (L2+).
- One worktree per fix attempt; discard after verifier REJECT.

## Connectors (MCP)

- MCP optional for L1 report-only loops.
- For L2+: GitHub MCP can read CI/issues; scope connectors to read + comment until trusted.

## Budget

- Max sub-agent spawns per run: 0 (L1).
- Review STATE.md daily.
- If token spend hits 80% of daily cap, switch to report-only.

## Links

- Pattern: [daily-triage](../../patterns/daily-triage.md)
- Checklist: [loop-design-checklist](../../docs/loop-design-checklist.md)
