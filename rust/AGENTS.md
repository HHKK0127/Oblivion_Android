# AGENTS.md — rust/ (formerly oblivion-re-tools)

Rules that apply to anything under `rust/`. The crate table lives in
`rust/README.md`; the reason this workspace exists at all is in the root
`AGENTS.md` under "Rust workspace (vendored)".

## Gate

Run before touching a crate, and again before committing:

```
cargo test --offline --workspace      # from rust/
```

Baseline **176 passed / 31 suites / 0 failed**. `OBLIVION_RUST_OFFLINE=1` (or
`--offline`) is the norm: the tree must build without network. Per-crate runs
(`cargo test --offline -p nif-core`) are for the tight loop, but the workspace
run is what gates the change, because a shared type move breaks neighbours.

## What belongs here

A crate holds logic that is a pure function of its arguments: parsers over an
in-memory buffer, string and path rules, index translation, geometry merge,
decode tables. State that only exists because parsing was mixed with I/O,
zlib or C++ record types moves too, carried behind an opaque handle the way
`bsa_reader` and `nif_parser` do it.

It does **not** hold anything that owns a GL object, a Jolt body, an Android
framework handle or a thread, and no crate borrows a file descriptor from the
C++ side — `bsa-core` opens the archive file itself, behind its own handle.
An FFI hop on those paths buys no safety and costs a call on a hot one. When in
doubt, the divider is impurity, not size.

## The `-core` / `-ffi` split

- `-core` holds the behaviour and has no `unsafe`, no `libc` and no knowledge
  that C++ exists. It is testable on the host with plain `cargo test`.
- `-ffi` is a thin `extern "C"` layer: `#[repr(C)]` records, `#[no_mangle]`
  exports, and the handle type that owns whatever the records borrow.

Conventions the existing ABIs follow, and that a new one should too:

- A `Handle` wraps the parser plus POD mirrors of the last parsed block. The
  mirrors' pointers borrow from vectors stored **in the handle**, so they
  survive the move into it — the pointers are taken before the move and the
  buffers do not move with it.
- One getter writes one block. Separate storage per getter family, so one
  getter cannot invalidate another's pointers.
- On failure the out record is left untouched; the error code is the only
  signal. `NULL` and misaligned out-records fail before anything is read.
- Capacity parameters are validated in the unit the caller was told to
  allocate — that is the only unit that can be wrong in the caller's favour.
  (The `afl_merge_geometry` vertex-vs-float bug, recorded in the plan's
  capacity-unit rule, is why.)
- Layout is pinned by `const _: () = assert!(size_of::<..>() == ..)` on both
  pointer widths, and again in the C header with `NIF_FFI_STATIC_ASSERT`, and
  a third time in the C harness.

## Adding a function

1. Implement and test it in `-core` first, on the host.
2. Expose it from `-ffi` following the getter above.
3. Add the contract checks to the crate's `tests/c_abi_check.c` (NULL,
   misalignment, `NIF_ERROR_NO_BLOCK`, the capacity rule).
4. If it moves existing C++ behaviour, run the host oracle over both
   implementations on the same inputs and record every field compared. **No
   oracle, no move.**
5. Wire the CMake touch point only if it is a new `.a`; adding a function to an
   existing ABI needs nothing in CMake.
6. Update the line counts in `rust/README.md` when a crate changes size
   materially.

## Adding a crate

Copy an existing `-core`/`-ffi` pair rather than starting fresh. Register the
`-ffi` in `app/src/main/cpp/CMakeLists.txt` (one `lib<name>_ffi.a` import plus
the cargo-ndk custom command) and add its header to the include path. Six ABIs
are linked today: `plugin`, `assets`, `bsa`, `nif`, `dds`, `esm`.

## Style

- `#![deny(warnings)]` is not set, but a new warning is a review failure.
- Comments explain *why* a field is laid out the way it is, not what the field
  is — the layout is asserted three times over, so the assertion carries the
  fact and the comment carries the reason.
- Doc comments on `extern "C"` functions state the safety contract and what
  borrows from what, because C callers cannot see the Rust types.
