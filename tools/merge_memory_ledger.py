#!/usr/bin/env python3
"""Append entries that exist only in another memory ledger into the project ledger.

The project ledger (.github/memory/session-memory.json) is the canonical store
for this repository, but the machine-local opencode ledger
(~/.config/opencode/memory/session-memory.json) accumulated entries that were
never copied back. Those entries are invisible to anyone reading the project
ledger, are not covered by the repository's review history, and disappear when
the machine is replaced, so this tool appends the missing ones.

Safety rules, learned from the 2026-10-22 truncation (61 entries lost):

  * the target is re-read immediately before writing, never from a cached copy
  * entries are appended only; existing entries are never edited or reordered
  * the write goes to a temporary file and is moved into place atomically
  * a copy of the previous target is kept under tmp/ before the write
  * the result is checked with the same rules as validate_memory_ledger.py

The local ledger is shared across repositories, so entries tagged with a
project other than --project are skipped by default. Pass
--include-other-projects to import them anyway.

The tool is idempotent: re-running it after a successful merge reports zero
missing entries and changes nothing.
"""

import argparse
import datetime
import io
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import validate_memory_ledger as validator  # noqa: E402  (path set above)

TARGET = os.path.join('.github', 'memory', 'session-memory.json')
SOURCE = os.path.join(os.path.expanduser('~'), '.config', 'opencode', 'memory', 'session-memory.json')
PROJECT = 'Oblivion_Android'
BACKUP_DIR = 'tmp'
ID_DATE = re.compile(r'(\d{4})(\d{2})(\d{2})$')
ISO_DATE = re.compile(r'^\d{4}-\d{2}-\d{2}$')


def stamp_now():
    now = datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0)
    return now.isoformat().replace('+00:00', 'Z')


def read_text(path):
    with io.open(path, encoding='utf-8', newline='') as handle:
        return handle.read()


def load(path):
    return json.loads(read_text(path))


def serialize(ledger):
    """Match the ledger's on-disk style: 2-space indent, CRLF, no final newline."""
    return json.dumps(ledger, ensure_ascii=False, indent=2).replace('\n', '\r\n')


def write_text(path, text):
    with io.open(path, 'w', encoding='utf-8', newline='') as handle:
        handle.write(text)


def label_for(path):
    """Path with the home directory collapsed, so the label stays portable."""
    home = os.path.expanduser('~')
    full = os.path.abspath(path)
    if full.startswith(home):
        return '~' + full[len(home):].replace(os.sep, '/')
    return full.replace(os.sep, '/')


def project_tags(entry):
    tags = entry.get('tags')
    if not isinstance(tags, list):
        return []
    return [str(tag)[len('project:'):] for tag in tags if str(tag).startswith('project:')]


def belongs_to(entry, project):
    """Return (keep, other_project_names) for an entry that names other projects."""
    others = [name for name in project_tags(entry) if name.lower() != project.lower()]
    return not others, others


def derive_timestamp(entry):
    date = entry.get('date')
    if isinstance(date, str) and ISO_DATE.match(date):
        return '%sT00:00:00Z' % date
    match = ID_DATE.search(str(entry.get('id') or ''))
    if match:
        return '%s-%s-%sT00:00:00Z' % match.groups()
    return None


def normalize(entry, source_label, project):
    """Fill in the fields the validator requires, without touching content."""
    merged = dict(entry)
    notes = []
    if not merged.get('timestamp'):
        derived = derive_timestamp(merged)
        if derived is None:
            derived = stamp_now()
            notes.append('timestamp set to the merge time (the entry carries no date)')
        else:
            notes.append('timestamp derived from the entry date')
        merged['timestamp'] = derived
    if not merged.get('priority'):
        merged['priority'] = 'medium'
        notes.append('priority defaulted to medium')
    if not merged.get('category'):
        merged['category'] = 'imported'
        notes.append('category defaulted to imported')
    if not isinstance(merged.get('tags'), list):
        merged['tags'] = []
    if not isinstance(merged.get('content'), str):
        merged['content'] = '' if merged.get('content') is None else str(merged['content'])
    merged['mergedFrom'] = source_label
    merged['mergedAt'] = stamp_now()
    if notes:
        merged['mergedNotes'] = notes
    return merged


def plan(target, source, project, include_other, source_label):
    known = {entry.get('id') for entry in target.get('entries') or []}
    incoming = []
    skipped = []
    for entry in source.get('entries') or []:
        ident = entry.get('id')
        if not ident or ident in known:
            continue
        keep, others = belongs_to(entry, project)
        if not keep and not include_other:
            skipped.append((ident, others))
            continue
        incoming.append(normalize(entry, source_label, project))
    incoming.sort(key=lambda item: (item.get('timestamp') or '', item.get('id') or ''))
    return incoming, skipped


def merge(target_path, source_path, project, include_other, dry_run):
    target = load(target_path)
    source = load(source_path)
    source_label = label_for(source_path)
    incoming, skipped = plan(target, source, project, include_other, source_label)

    print('target : %s (%d entries)' % (target_path, len(target.get('entries') or [])))
    print('source : %s (%d entries)' % (source_label, len(source.get('entries') or [])))
    print('missing: %d entry/entries' % len(incoming))
    for entry in incoming:
        print('  + %-52s %s' % (entry.get('id'), entry.get('timestamp')))
    if skipped:
        print('skipped: %d entry/entries tagged for another project' % len(skipped))
        for ident, others in skipped:
            print('  - %-52s %s' % (ident, ', '.join(others)))

    if not incoming:
        print('nothing to do')
        return 0
    if dry_run:
        print('dry run: the target was not modified')
        return 0

    if not os.path.isdir(BACKUP_DIR):
        os.makedirs(BACKUP_DIR)
    backup = os.path.join(BACKUP_DIR, 'session-memory.pre-merge-%s.json'
                          % stamp_now().replace(':', '').replace('-', ''))
    write_text(backup, read_text(target_path))
    print('backup : %s' % backup)

    # Re-read the target so a concurrent writer's entries are not dropped.
    fresh = load(target_path)
    fresh_ids = {entry.get('id') for entry in fresh.get('entries') or []}
    added = [entry for entry in incoming if entry.get('id') not in fresh_ids]
    fresh.setdefault('entries', []).extend(added)

    entries = fresh['entries']
    now = stamp_now()
    metadata = fresh.setdefault('metadata', {})
    metadata['totalEntries'] = len(entries)
    metadata['compressedCount'] = sum(1 for e in entries if isinstance(e, dict) and e.get('compressed'))
    metadata['lastUpdated'] = now
    if 'entry_count' in metadata:
        metadata['entry_count'] = len(entries)
    fresh['lastUpdated'] = now

    errors, warnings = validator.check(fresh)
    for warning in warnings:
        print('warning: %s' % warning)
    if errors:
        for error in errors:
            print('error: %s' % error)
        print('refusing to write: the merged ledger failed validation')
        return 1

    temp_path = target_path + '.merge-tmp'
    write_text(temp_path, serialize(fresh))
    os.replace(temp_path, target_path)
    print('wrote  : %s (%d entries, %d added)' % (target_path, len(entries), len(added)))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--source', default=SOURCE, help='ledger to import from (default: %s)' % SOURCE)
    parser.add_argument('--target', default=TARGET, help='ledger to append to (default: %s)' % TARGET)
    parser.add_argument('--project', default=PROJECT, help='project name used by the project: tag filter')
    parser.add_argument('--include-other-projects', action='store_true',
                        help='also import entries tagged for a different project')
    parser.add_argument('--dry-run', action='store_true', help='report the missing entries without writing')
    args = parser.parse_args()

    if not os.path.exists(args.source):
        raise SystemExit('ERROR: %s does not exist' % args.source)
    if not os.path.exists(args.target):
        raise SystemExit('ERROR: %s does not exist' % args.target)
    return merge(args.target, args.source, args.project, args.include_other_projects, args.dry_run)


if __name__ == '__main__':
    sys.exit(main())
