# REF change state `.ess` assessment

Date: 2026-11-06
Status: investigation-only; no source or canonical RE-document edits

## Result

The current repository does not contain an implementation of Oblivion's `.ess`
REF change-state records, and the available RE evidence does not establish
their complete binary layout. No speculative parser or migration code should
be added.

## Confirmed RE evidence

- `docs/RE_EXE_REPORT.md:2748-2873` documents the save-slot object and the
  `.ess` header/slot-list path.
- The retail header magic is `TES4SAVEGAME` (12 bytes), with version-gated
  header fields. The documented thresholds are `0x1c`, `0x2e`, `0x34`,
  `0x37`, `0x38`, `0x51`, and `0x52`.
- The report explicitly distinguishes `ess_file_obj_load` from a complete
  `.ess` parser: it is an object-restoration/VM-like path, not a full binary
  map.
- The report lists the unresolved areas as the compressed payload position and
  the complete binary boundaries for change forms, globals, quests, cells, and
  AI. It also states that no real `.ess` sample is present in the workspace or
  BSA extraction, so binary cross-checking is unavailable.

## Migration-side comparison

- `app/src/main/cpp/save_system/save_format_version.h:20-86` defines a custom
  `OBLV` magic, a 72-byte packed header, CRC32, and versions 1 through 5.
- `app/src/main/cpp/save_system/save_manager.cpp` serializes tagged sections
  such as `PLYR`, `INVT`, `SPLL`, `QSTS`, `WLDS`, `SCRP`, `CNTR`, and `MERC`.
- `app/src/main/cpp/save_system/save_manager.h:136-170` has pending buffers for
  lazily materialised containers and merchants, but no REF/change-form table,
  form-state delta, or `.ess` compatibility layer.
- `plan.md` identifies REF change state as the remaining Phase 69 persistence
  gap; container, quest, script-global, and merchant persistence are separate
  migration features and do not prove `.ess` compatibility.

## Decision

Keep the TODO open and blocked on a real `.ess` sample plus a confirmed
change-form boundary map. The next valid investigation step is to obtain a
user-provided or otherwise authorised save sample, record its provenance, and
parse only the header first before attempting any change-state interpretation.
