"""Push Copilot session memories into Supermemory verbatim.

Companion to tools/sync_memory_to_supermemory.py. That script mirrors the shared
opencode ledger (.github/memory/session-memory.json) into Supermemory; this one
records memories authored by GitHub Copilot directly, without touching the
ledger.

Why not the MCP add_memory tool: it runs the content through an LLM before
storing, which drops negation conditions, exact numbers and confidence markers.
The 2026-10-22 reconciliation measured 0/580 verbatim matches for that path.
This script instead uses POST /v4/memories, which stores the content unchanged
and carries the provenance in metadata.

Usage:
    python tools/push_copilot_memory.py tmp/copilot_memories.json
    python tools/push_copilot_memory.py tmp/copilot_memories.json --dry-run

The input file is a JSON array of ledger-schema entries:
    [{"id", "timestamp", "priority", "category", "tags"[], "content"}, ...]

The sync is idempotent and convergent: entries already present are compared by
content and only missing entries are pushed, or drifted ones re-pushed. Re-run
safely at any time.

API key resolution matches sync_memory_to_supermemory.py:
    1. ~/.copilot/mcp-config.json -> mcpServers.supermemory.headers.Authorization
    2. environment variable SUPERMEMORY_API_KEY
The key is never written to disk or printed.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import sys
import time

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
SYNC_MODULE = os.path.join(TOOLS_DIR, "sync_memory_to_supermemory.py")

SOURCE_LABEL = "copilot-direct"


def load_sync_module():
    spec = importlib.util.spec_from_file_location("sync_memory_to_supermemory", SYNC_MODULE)
    if spec is None or spec.loader is None:
        raise SystemExit("cannot load " + SYNC_MODULE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_entries(path: str) -> list[dict]:
    with open(path, "rb") as handle:
        entries = json.loads(handle.read().decode("utf-8"))
    if not isinstance(entries, list):
        raise SystemExit("expected a JSON array of entries in " + path)
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            raise SystemExit("entry %d is not an object" % index)
        for field in ("id", "content"):
            if not entry.get(field):
                raise SystemExit("entry %d is missing required field %r" % (index, field))
    return entries


def metadata_of(entry: dict) -> dict:
    metadata = {
        "entryId": entry["id"],
        "category": entry.get("category") or "",
        "priority": entry.get("priority") or "",
        "recordedAt": entry.get("timestamp") or "",
        "source": entry.get("source") or SOURCE_LABEL,
        "ledger": SOURCE_LABEL,
        "compressed": bool(entry.get("compressed")),
    }
    tags = entry.get("tags") or []
    if tags:
        metadata["tags"] = tags
    return metadata


def main() -> int:
    parser = argparse.ArgumentParser(description="Push Copilot memories into Supermemory verbatim.")
    parser.add_argument("entries", help="path to a JSON array of ledger-schema entries")
    parser.add_argument("--namespace", default=None, help="target Supermemory space (container tag)")
    parser.add_argument("--dry-run", action="store_true", help="report what would be pushed without writing")
    args = parser.parse_args()

    sync = load_sync_module()
    namespace = args.namespace or sync.DEFAULT_NAMESPACE
    entries = load_entries(args.entries)

    key = sync.resolve_api_key(namespace)
    known = sync.existing_memories(namespace, key)

    missing = [entry for entry in entries if entry["id"] not in known]
    drifted = [
        entry
        for entry in entries
        if entry["id"] in known and known[entry["id"]]["content"] != entry["content"]
    ]

    print("entries in input: %d" % len(entries))
    print("already in Supermemory: %d" % (len(entries) - len(missing)))
    print("to push: %d" % len(missing))
    print("to update: %d" % len(drifted))
    for entry in missing:
        print("  + %s (%d chars)" % (entry["id"], len(entry["content"])))
    for entry in drifted:
        print("  ~ %s (%d chars)" % (entry["id"], len(entry["content"])))

    if args.dry_run or not (missing or drifted):
        return 0

    pushed = 0
    for start in range(0, len(missing), sync.BATCH_SIZE):
        batch = missing[start : start + sync.BATCH_SIZE]
        payload = {
            "containerTag": namespace,
            "memories": [{"content": entry["content"], "metadata": metadata_of(entry)} for entry in batch],
        }
        body = sync.request_json("/v4/memories", payload, key)
        pushed += len(body.get("memories") or [])
        print("  pushed %d/%d" % (pushed, len(missing)))
        time.sleep(2)

    updated = 0
    for entry in drifted:
        sync.request_json(
            "/v4/memories",
            {
                "id": known[entry["id"]]["id"],
                "containerTag": namespace,
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
