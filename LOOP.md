# Loop Configuration — Minimal Triage (Opencode)

## Active Loops

| Pattern | Cadence | Status | Command |
|---------|---------|--------|---------|
| Daily Triage | 1d | L1 report-only | `opencode run "Run loop-triage" --agent loop-triage` via cron/systemd |

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
