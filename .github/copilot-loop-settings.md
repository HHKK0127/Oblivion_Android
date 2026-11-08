# GitHub Copilot Loop Settings

These settings apply to GitHub Copilot sessions in this repository. They are
independent from the OpenCode loop settings in `AGENTS.md`,
`loop-constraints.md`, and `loop-budget.md`.

## User-directed engineering rules

- Preserve the original game's structure, data flow, assets, and behavior as
  the primary constraint. Do not replace an unknown original structure with a
  convenient redesign.
- Put any behavior, data layout, formula, asset mapping, or UI state whose
  original structure is not evidenced on hold. Record the exact unknown,
  evidence checked, impact, and requested analysis-session follow-up.
- After creating or changing an implementation, review it against the
  original structure, repository patterns, tests, and runtime evidence. Add
  newly discovered follow-up work to `TODO.md` before moving on.
- Create TODO items automatically when they have an objective completion test
  and do not require a human product or design decision. Mark human decisions
  explicitly and keep dependent implementation work blocked until recorded.

## Memory, graph, and recovery

- Treat **Supermemory** as durable cross-session memory for verified decisions,
  constraints, and lessons. Use the configured namespace and never store
  secrets or speculative claims.
- Treat **Graphify** as the relationship and provenance layer connecting
  decisions, files, evidence, TODOs, and unresolved questions. Do not treat a
  Graphify edge as proof; preserve `EXTRACTED`, `INFERRED`, and `AMBIGUOUS`
  distinctions.
- When recording a durable item, keep the same stable topic or identifier
  across Supermemory and Graphify where possible, and check that related
  memories refer to the same evidence instead of creating duplicates or
  contradictions.
- After an interruption or temporary loop stop, read the latest `STATE.md`,
  relevant `TODO.md` items, latest session checkpoint, and `git status` before
  acting. Resume from the first verified actionable item without repeating
  verified commands or edits.
- Periodically checkpoint material changes to Supermemory and refresh the
  Graphify relationship view when architecture, evidence, TODO dependencies,
  or unresolved decisions change.

## Operating modes

- Start each new task in **L1 report-only mode**.
- L1 may inspect files, analyze the task, run read-only checks, and report a
  plan or findings.
- Enter **L2 implementation mode** only when the user explicitly requests a
  code or configuration change.
- Do not infer L2 authorization from a request for status, review, or
  explanation alone.
- When L2 is explicitly authorized, remain in L2 for the same bounded task
  until the task is verified, blocked, or its scope changes.
- In autopilot mode, continue through independent subtasks that are already
  part of the approved scope. Do not wait for a confirmation between normal
  implementation, verification, and state-recording steps.

## Loop cycle

Every implementation cycle follows this order:

1. Read `STATE.md` and the applicable instruction files.
2. Select one highest-priority actionable item with a clear completion test.
3. Inspect the relevant code and existing patterns before editing.
4. Make the smallest complete change for that item.
5. Run the smallest relevant build, test, or runtime check.
6. Record evidence, risks, and remaining work in `STATE.md`.
7. Reassess the next item only after the current item is verified.

Do not start a second implementation item while the current item has an
unresolved build, test, or runtime failure.

## Before and during work

- Read `STATE.md` before triage or implementation.
- Read applicable repository instruction files before editing.
- Search for existing patterns before adding new logic.
- Keep independent read-only investigations parallel where practical.
- Do not repeat the same tool call when its result is already available.
- Stop and reassess when the same error or action repeats twice without new
  evidence.
- If a command fails, first classify it as environment, build, test, runtime,
  or implementation failure. Change the diagnostic approach before retrying;
  do not blindly rerun the same command more than twice.

## Change limits

- Keep each implementation attempt focused on one root cause or one requested
  feature.
- Allow at most three materially different fix attempts for one issue.
- After three unsuccessful attempts, stop implementation and report the
  blocker, evidence, and decision needed from the user.
- Do not edit unrelated files or rewrite unrelated existing changes.

## Autopilot budget and recovery

- One loop run may contain at most three materially different fix attempts for
  one root cause and at most five implementation items before a checkpoint.
- A failed build or runtime check consumes one attempt only when the failure is
  caused by the current change; pre-existing failures must be recorded
  separately.
- After the limit is reached, preserve the evidence in `STATE.md`, summarize
  the blocker, and continue only with independent in-scope work that does not
  depend on the blocked item.
- Never hide a failure with a fallback, silent catch, disabled assertion, or
  skipped verification.

## Context compression and checkpoints

- Create a compact checkpoint after 15 substantive turns, before a long
  independent investigation, or when the working context is becoming
  constrained.
- A checkpoint must retain: current objective, completed changes, exact files,
  verification commands/results, known failures, decisions, and the next
  bounded action.
- Compress completed work to one line in `STATE.md`; keep unresolved risks and
  evidence explicit.
- Do not create planning or checkpoint Markdown files in the repository.
  Session artifacts belong in the Copilot session-state files directory.

## Verification

- Run the smallest documented test or build that covers the change.
- For Android/native changes, use the documented native build and APK
  verification loop when applicable.
- For emulator-facing changes, verify the affected user action with logcat or
  an equivalent runtime check.
- For Android UI/gameplay changes, prefer this evidence chain:
  `assembleDebug` -> `adb install -r` -> clean launch -> affected input ->
  screenshot/logcat -> crash scan. Store screenshots under `screenshots/` and
  logs under `tmp/`.
- Record the change, evidence, remaining risks, and known failures in
  `STATE.md`.
- Do not claim completion when verification is incomplete.

## Problem and improvement ledger

- Every discovered problem must be classified as `blocker`, `high`, `medium`,
  or `low`, with file/line or runtime evidence.
- Record one recommended next action and one explicit non-action for unresolved
  speculation.
- Keep user-visible problems and improvement proposals together in `STATE.md`;
  do not create duplicate issue lists in unrelated documents.
- Prefer fixes backed by original game assets, runtime evidence, or established
  repository behavior. Mark any approximation as an approximation.

## Git and review safety

- Periodic commits are allowed after a verified milestone when the change set
  has been reviewed, contains no secrets, and stays within the approved scope.
- Push, merge, and force-update history only when the user explicitly requests
  that exact Git operation.
- Preserve unrelated working-tree changes.
- Do not create a branch, worktree, stash, reset, or rebase unless requested
  or required by an explicit task instruction.
- Do not close issues or mark pull requests ready without user approval.
- When a pull request is explicitly requested, show the proposed scope and
  verification status before creating or updating it.
- A periodic commit must not include failing or unreviewed work. Commits that
  include `docs/RE_EXE_REPORT.md` remain separately gated by explicit user
  approval under the RE analysis policy.

## Stop conditions

Stop and report instead of continuing when:

- the user must choose between materially different behaviors or designs;
- required runtime evidence, original assets, or analysis results are missing;
- a safety-sensitive or destructive action lacks explicit approval;
- the same failure persists after three distinct fix attempts;
- the requested result would require guessing an unresolved specification.
- the next action would expand the task beyond the approved scope;
- all remaining items are blocked on missing user decisions or unavailable
  runtime evidence.

## Persistent task record

`STATE.md` is the Copilot task state and verification record. The OpenCode
loop logs are reference-only for Copilot and must not be modified as part of
ordinary Copilot work.

At the end of each verified loop cycle, update `STATE.md` with:

- `Completed`
- `Verification`
- `Known issues`
- `Next action`

Keep the entry concise and append new dated evidence rather than rewriting
older historical records.
