#!/usr/bin/env python3
"""Validate .github/memory/session-memory.json, the shared memory ledger.

The ledger is rewritten wholesale by more than one tool (GitHub Copilot and
opencode), so a bad write can silently truncate the file or drop entries. This
checker catches the failure modes we have actually hit:

  * truncated / unparsable JSON (a half-written file)
  * duplicate entry ids (a re-appended entry)
  * metadata counters that no longer match the entries array
  * entries missing a required field, or with an unknown priority

Exit code 0 means the ledger is consistent. Exit code 1 means it is not and
should not be committed. Use --fix-metadata to repair the counters in place;
that only touches metadata, never entries.
"""

import argparse
import datetime
import io
import json
import os
import sys

LEDGER = os.path.join('.github', 'memory', 'session-memory.json')
REQUIRED = ('id', 'timestamp', 'priority', 'category', 'tags', 'content')
PRIORITIES = ('high', 'medium', 'low')


def load(path):
    with io.open(path, encoding='utf-8') as handle:
        text = handle.read()
    try:
        return json.loads(text)
    except ValueError as exc:
        raise SystemExit(
            'ERROR: %s is not valid JSON (%s).\n'
            '       The file is most likely a truncated write. Restore it from '
            'git or from the last good copy before continuing.' % (path, exc)
        )


def check(ledger):
    errors = []
    warnings = []

    entries = ledger.get('entries')
    if not isinstance(entries, list):
        errors.append('entries is missing or not a list')
        return errors, warnings

    seen = {}
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            errors.append('entry #%d is not an object' % index)
            continue
        ident = entry.get('id')
        if not ident:
            errors.append('entry #%d has no id' % index)
        elif ident in seen:
            errors.append('duplicate id %r (entries #%d and #%d)' % (ident, seen[ident], index))
        else:
            seen[ident] = index
        for field in REQUIRED:
            if field not in entry:
                errors.append('entry %s is missing %r' % (ident or '#%d' % index, field))
        if entry.get('priority') not in PRIORITIES:
            errors.append('entry %s has unknown priority %r' % (ident, entry.get('priority')))

    metadata = ledger.get('metadata') or {}
    if metadata.get('totalEntries') != len(entries):
        errors.append(
            'metadata.totalEntries is %r but entries holds %d'
            % (metadata.get('totalEntries'), len(entries))
        )
    compressed = sum(1 for e in entries if isinstance(e, dict) and e.get('compressed'))
    if metadata.get('compressedCount') != compressed:
        errors.append(
            'metadata.compressedCount is %r but %d entries are compressed'
            % (metadata.get('compressedCount'), compressed)
        )

    stamps = [e.get('timestamp') for e in entries if isinstance(e, dict) and e.get('timestamp')]
    if stamps:
        newest = max(stamps)
        for label, value in (('lastUpdated', ledger.get('lastUpdated')),
                             ('metadata.lastUpdated', metadata.get('lastUpdated'))):
            if value and value < newest:
                warnings.append('%s (%s) is older than the newest entry (%s)' % (label, value, newest))
        out_of_order = sum(1 for a, b in zip(stamps, stamps[1:]) if b < a)
        if out_of_order:
            warnings.append('%d entries are not in timestamp order (append-only history is fine)' % out_of_order)

    return errors, warnings


def fix_metadata(path, ledger):
    entries = ledger.get('entries') or []
    now = datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0)
    stamp = now.isoformat().replace('+00:00', 'Z')
    metadata = ledger.setdefault('metadata', {})
    metadata['totalEntries'] = len(entries)
    metadata['compressedCount'] = sum(1 for e in entries if isinstance(e, dict) and e.get('compressed'))
    metadata['lastUpdated'] = stamp
    ledger['lastUpdated'] = stamp
    with io.open(path, 'w', encoding='utf-8') as handle:
        handle.write(json.dumps(ledger, ensure_ascii=False, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('path', nargs='?', default=LEDGER, help='ledger to check (default: %s)' % LEDGER)
    parser.add_argument('--fix-metadata', action='store_true',
                        help='rewrite the metadata counters instead of only reporting them')
    args = parser.parse_args()

    ledger = load(args.path)
    errors, warnings = check(ledger)

    if args.fix_metadata:
        fix_metadata(args.path, ledger)
        errors, warnings = check(ledger)

    for warning in warnings:
        print('warning: %s' % warning)
    for error in errors:
        print('error: %s' % error)

    entries = ledger.get('entries') or []
    print('%s: %d entries, %d error(s), %d warning(s)'
          % (args.path, len(entries), len(errors), len(warnings)))
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
