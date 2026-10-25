"""One-way sync: local session memory ledger -> Supermemory.

The ledger at .github/memory/session-memory.json is the single source of truth.
Supermemory is a derived, read-only-by-convention search layer: entries are
pushed verbatim (no LLM extraction) so negation conditions and wording survive,
and priority/category/tags travel in metadata.

The sync is idempotent and convergent: it compares each ledger entry against the
copy already in the target space and only pushes entries that are missing, or
re-pushes entries whose content drifted after a ledger edit. It can therefore be
re-run safely at any time (e.g. after appending or correcting entries).

Usage:
    python tools/sync_memory_to_supermemory.py            # push/refresh entries
    python tools/sync_memory_to_supermemory.py --dry-run  # report only

API key resolution order:
    1. ~/.copilot/mcp-config.json -> mcpServers.supermemory.headers.Authorization
    2. environment variable SUPERMEMORY_API_KEY

Each candidate is validated with a lightweight authenticated request and the
first one that authenticates wins, so a stale environment variable cannot break
the sync. The key is never written to disk or printed by this script.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LEDGER_PATH = os.path.join(REPO_ROOT, ".github", "memory", "session-memory.json")
MCP_CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".copilot", "mcp-config.json")

API_BASE = "https://api.supermemory.ai"
DEFAULT_NAMESPACE = "opencode_project_470935b226aa5f24"
LEDGER_MARKER = "Oblivion_Android/.github/memory/session-memory.json"

BATCH_SIZE = 100
PAGE_SIZE = 100
MAX_RETRIES = 4


def key_candidates() -> list[tuple[str, str]]:
    candidates: list[tuple[str, str]] = []
    if os.path.exists(MCP_CONFIG_PATH):
        config = json.load(open(MCP_CONFIG_PATH, encoding="utf-8"))
        auth = config["mcpServers"]["supermemory"]["headers"]["Authorization"].strip()
        if auth.lower().startswith("bearer "):
            auth = auth[7:].strip()
        if auth:
            candidates.append(("mcp-config.json", auth))
    env_key = os.environ.get("SUPERMEMORY_API_KEY", "").strip()
    if env_key:
        candidates.append(("SUPERMEMORY_API_KEY", env_key))
    if not candidates:
        raise SystemExit("No API key: set SUPERMEMORY_API_KEY or configure the supermemory MCP server.")
    return candidates


def resolve_api_key(namespace: str) -> str:
    """Return the first candidate key that authenticates against the API."""
    for source, key in key_candidates():
        try:
            request_json(
                "/v4/memories/list",
                {"containerTags": [namespace], "limit": 1, "page": 1},
                key,
                retry_on_5xx=False,
            )
            return key
        except SystemExit as exc:
            print("  ignoring %s (%s)" % (source, exc), file=sys.stderr)
    raise SystemExit("No working Supermemory API key found.")


def request_json(path: str, payload: dict | None, key: str, method: str = "POST", retry_on_5xx: bool = True):
    body = None if payload is None else json.dumps(payload, ensure_ascii=False).encode("utf-8")
    req = urllib.request.Request(
        API_BASE + path,
        data=body,
        headers={"Authorization": "Bearer " + key, "Content-Type": "application/json"},
        method=method,
    )
    delay = 5
    for attempt in range(1, MAX_RETRIES + 1):
        try:
            with urllib.request.urlopen(req, timeout=300) as response:
                return json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as exc:
            detail = exc.read().decode("utf-8", "replace")
            if retry_on_5xx and exc.code in (429, 500, 502, 503, 504) and attempt < MAX_RETRIES:
                wait = int(exc.headers.get("Retry-After") or delay)
                print("  HTTP %d, retrying in %ds" % (exc.code, wait), file=sys.stderr)
                time.sleep(min(wait, 120))
                delay *= 2
                continue
            raise SystemExit("HTTP %d on %s: %s" % (exc.code, path, detail[:500]))
    raise SystemExit("retries exhausted on " + path)


def load_entries() -> list[dict]:
    with open(LEDGER_PATH, "rb") as handle:
        ledger = json.loads(handle.read().decode("utf-8"))
    return ledger["entries"]


def existing_memories(namespace: str, key: str) -> dict[str, dict]:
    """Map entryId -> {id, content} for the latest version of each pushed entry."""
    found: dict[str, dict] = {}
    page = 1
    while True:
        body = request_json(
            "/v4/memories/list",
            {"containerTags": [namespace], "limit": PAGE_SIZE, "page": page, "order": "desc", "sort": "createdAt"},
            key,
        )
        items = body.get("memoryEntries") or []
        for item in items:
            entry_id = (item.get("metadata") or {}).get("entryId")
            if not entry_id:
                continue
            current = found.get(entry_id)
            # A PATCH keeps the superseded version with isLatest=false; keep the newest one.
            if current is None or _is_newer(item, current):
                found[entry_id] = {
                    "id": item["id"],
                    "content": item.get("memory") or "",
                    "version": item.get("version") or 0,
                    "created_at": item.get("createdAt") or "",
                    "is_latest": item.get("isLatest", True),
                }
        pagination = body.get("pagination") or {}
        if page >= int(pagination.get("totalPages") or 1) or not items:
            break
        page += 1
    return found


def _is_newer(candidate: dict, current: dict) -> bool:
    if bool(candidate.get("isLatest", True)) != bool(current.get("is_latest", True)):
        return bool(candidate.get("isLatest", True))
    if (candidate.get("version") or 0) != (current.get("version") or 0):
        return (candidate.get("version") or 0) > (current.get("version") or 0)
    return (candidate.get("createdAt") or "") > (current.get("created_at") or "")


def metadata_of(entry: dict) -> dict:
    metadata = {
        "entryId": entry["id"],
        "category": entry.get("category") or "",
        "priority": entry.get("priority") or "",
        "recordedAt": entry.get("timestamp") or "",
        "source": entry.get("source") or "session-memory.json",
        "ledger": LEDGER_MARKER,
        "compressed": bool(entry.get("compressed")),
    }
    tags = entry.get("tags") or []
    if tags:
        metadata["tags"] = tags
    return metadata


def main() -> int:
    parser = argparse.ArgumentParser(description="Sync the local session memory ledger into Supermemory.")
    parser.add_argument("--namespace", default=DEFAULT_NAMESPACE, help="target Supermemory space (container tag)")
    parser.add_argument("--dry-run", action="store_true", help="report what would be pushed without writing")
    args = parser.parse_args()

    entries = load_entries()
    key = resolve_api_key(args.namespace)
    known = existing_memories(args.namespace, key)
    missing = [entry for entry in entries if entry["id"] not in known]
    drifted = [
        entry
        for entry in entries
        if entry["id"] in known and known[entry["id"]]["content"] != entry["content"]
    ]

    print("ledger entries: %d" % len(entries))
    print("already in Supermemory: %d" % (len(entries) - len(missing)))
    print("to push: %d" % len(missing))
    print("to update: %d" % len(drifted))
    if args.dry_run or not (missing or drifted):
        return 0

    pushed = 0
    for start in range(0, len(missing), BATCH_SIZE):
        batch = missing[start : start + BATCH_SIZE]
        payload = {
            "containerTag": args.namespace,
            "memories": [{"content": entry["content"], "metadata": metadata_of(entry)} for entry in batch],
        }
        body = request_json("/v4/memories", payload, key)
        pushed += len(body.get("memories") or [])
        print("  pushed %d/%d" % (pushed, len(missing)))
        time.sleep(2)

    updated = 0
    for entry in drifted:
        request_json(
            "/v4/memories",
            {
                "id": known[entry["id"]]["id"],
                "containerTag": args.namespace,
                "newContent": entry["content"],
                "metadata": metadata_of(entry),
            },
            key,
            method="PATCH",
        )
        updated += 1
        print("  updated %d/%d" % (updated, len(drifted)))
        time.sleep(1)

    print("done: %d pushed, %d updated" % (pushed, updated))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
