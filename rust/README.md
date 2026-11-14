# rust/ workspace (formerly oblivion-re-tools)

Pure-logic layers behind the C++ device tree, vendored into this repository on
2026-11-09. The C++ side keeps its original headers and reaches these crates
through six small C ABIs, so no caller changed when the bodies moved.

## Crates

Nine pure-logic crates carry the behaviour; six `-ffi` crates expose it as a C
ABI. Nothing here owns a GL object, a Jolt body, an Android framework handle or
a thread, and no crate borrows a file descriptor from the C++ side — `bsa-core`
opens the archive file itself, behind its own handle. That is the line that
decides what moves.

| Crate | Lines | What it holds |
| --- | ---: | --- |
| `nif-core` | 6,859 | Gamebryo/NetImmerse NIF container parser (header, object array, block walker, geometry, skin, keyframe data) |
| `nif-ffi` | 1,773 | C ABI for `nif-core`, so the C++ `NIFParser` shim can decode meshes through Rust |
| `bsa-core` | 1,770 | Oblivion `.bsa` archive reader (header, folder/file records, name table, zlib extraction) |
| `esm-ffi` | 1,598 | C ABI for `esm-parser`, so the C++ Android layer can index Oblivion ESM/ESP plugins from Rust |
| `esm-parser` | 899 | Oblivion ESM record parser (MGEF, Race, Class, BirthSign, WTHR, CLMT, etc.) |
| `plugin-parser` | 829 | Oblivion plugin load-order handling: the `plugins.txt` grammar and FormID translation into load-order space |
| `bsa-ffi` | 711 | C ABI for `bsa-core`, so the C++ `BSArchive` wrapper can read Bethesda archives through Rust |
| `assets-core` | 672 | Oblivion Data-folder path expansion and NIF geometry merging, with no device or renderer dependency |
| `plugin-ffi` | 627 | C ABI for `plugin-parser`, so the C++ Android layer can rebase FormIDs through Rust |
| `dds-core` | 573 | DDS texture parser and DXT/uncompressed decoder (header parse, BC1/2/3, mask unpack) |
| `assets-ffi` | 417 | C ABI for `assets-core`, so the C++ Android asset layer can resolve paths and merge geometry through Rust |
| `dds-ffi` | 404 | C ABI for `dds-core`, so the C++ `DDSLoader` shim can parse and decode textures through Rust |
| `vm-handlers` | 373 | Oblivion Script VM handler recovery via capstone (362 MISS functions, trampoline `0x4FAE80` / dispatcher `0x4FDE30`) |
| `cli` | 286 | Unified CLI for Oblivion RE tools |
| `ess-parser` | 39 | Oblivion `.ess` SaveGame parser (SaveGame `0x465130` / LoadGame `0x465860` layout) |

Totals: **17,830 lines across 29 `.rs` files.**

## The six C ABIs

| ABI | Header | Built by |
| --- | --- | --- |
| `plugin_ffi` | `rust/crates/plugin-ffi/include/plugin_ffi.h` | `libplugin_ffi.a` |
| `assets_ffi` | `rust/crates/assets-ffi/include/assets_ffi.h` | `libassets_ffi.a` |
| `bsa_ffi` | `rust/crates/bsa-ffi/include/bsa_ffi.h` | `libbsa_ffi.a` |
| `nif_ffi` | `rust/crates/nif-ffi/include/nif_ffi.h` | `libnif_ffi.a` |
| `dds_ffi` | `rust/crates/dds-ffi/include/dds_ffi.h` | `libdds_ffi.a` |
| `esm_ffi` | `rust/crates/esm-ffi/include/esm_ffi.h` | `libesm_ffi.a` |

`app/src/main/cpp/CMakeLists.txt` builds all six with `cargo ndk` in one custom
command per ABI and imports them as `IMPORTED STATIC` targets. `OBLIVION_RUST_DIR`
defaults to this directory and `OBLIVION_RUST_OFFLINE` runs cargo with
`--offline`; the matching `-Poblivion.rust.*` Gradle properties override both.
The sources are globbed with `CONFIGURE_DEPENDS`, so a change under any crate
triggers a rebuild.

## Gate

Before touching anything here, run from `rust/`:

```
cargo test --offline --workspace
```

Baseline **176 passed / 31 suites / 0 failed** — the same command CI runs for
the host archives. An empty suite is still a suite; the count is 31, not the
number of non-zero lines.

## Oracles

Never move parse behaviour without the host oracle that pins it. Each ported
function is checked by running the original C++ and the Rust over the same
inputs and comparing every output field; the harnesses (`tmp/esm_oracle`,
`tmp/nif_oracle` and the others) are local-only and are not committed. A Rust
regression therefore fails the same assertions the C++ code had, and the C ABI
gets its own C11 harness for the contracts the oracle cannot see — NULL
handling, `BUFFER_TOO_SMALL` without writes, error-code numbering, and the
capacity-unit rules.

See `rust/AGENTS.md` for the conventions that apply when adding to a crate.
