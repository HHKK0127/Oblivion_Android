# docs/gotchas.md — ドメイン知識（症状 / 原因 / 正しい手順 / 確認方法）

AGENTS.md の Learnings（振る舞い）とは別。ここは**ドメイン知識**を固定書式で置く。
**確認方法が書けない項目は載せない**（載せた時点で腐る）。

---

## 1. NIF ヘッダで `unknown_field` と BzString×3 を飛ばさない

- **症状**: NIF のヘッダからブロック型表を読むと `num_block_types` が巨大（例 11）になり、
  `sized string too long` で落ちる。生 NIF コーパスで「型が 0 件」になる。
- **原因**: `Gamebryo File Format` 20.0.0.x のヘッダは version の後に
  `endian(u8) → user_version(u32) → num_objects(u32) → unknown_field(u32) → creator/process/export の BzString×3`
  が続く。`unknown_field` と 3 文字列を飛ばさずに型数を読むと位置がずれる。
- **正しい手順**: `rust/crates/nif-core/src/header.rs::read_header` に一致させる
  （`unknown_field` を 1 つ読み、BzString を 3 つ読んでから型表へ）。
- **確認方法**: `python scripts/nif_coverage.py --corpus meshes_extracted` が
  `files: 8005 (0 unreadable)` を出す（1 件でも `unreadable` ならずれている）。

## 2. NIF の `num_block_types` は u16

- **症状**: 型数を u32 で読むと、`num_objects` の次の値（例 11 = user version の残り）を
  型数と誤認する。
- **原因**: NIF の `num_block_types` は **u16**（`header.rs:165` の `reader.u16()`）。
- **正しい手順**: u16 で読む。
- **確認方法**: `python scripts/nif_coverage.py --corpus meshes_extracted` の
  `corpus_types` が妥当な値（118〜119）になる。

## 3. `-lsynchronization` は `*windows-gnu` の case のみ

- **症状**: Linux CI のホストテストのリンクが `-lsynchronization` で失敗する
  （そのライブラリが無い）。
- **原因**: Rust std が **MinGW ホストでのみ** winsock/ntdll/userenv/synchronization を
  引き込む。Linux では不要。
- **正しい手順**: `tools/host_tests/run_host_tests.sh` の `EXTRA_LIBS` へ
  `*windows-gnu` の case の中だけで追加する。
- **確認方法**: Windows(Git Bash/MinGW) 側で `-lsynchronization` を外すとリンクが
  308 undefined に戻る（＝Windows では必須）。Linux では外しても緑のまま
  （＝Linux では不要）。「Linux で緑」だけでは `*windows-gnu` 限定の証拠にならない。

## 4. 共有 `target/` に proc-macro の MSVC `.dll` を混ぜない

- **症状**: GNU ツールチェーンでコンパイルすると
  `error[E0786]: found invalid metadata files for crate thiserror_impl`（
  MSVC 製の proc-macro `.dll` を読む）。
- **原因**: 共有 `target/` に別ツールチェーンの proc-macro 成果物が混在。
  `--target` を付けても proc-macro は `target/<CARGO_TARGET_DIR>/release` 側に出るため、
  同じ dir を共有すると再発する。
- **正しい手順**: `scripts/build_host_rust.sh` で `CARGO_TARGET_DIR=target/gnu` に分離し、
  GNU の実体（`rustup which rustc/cargo --toolchain <gnu>`）を `RUSTC` に固定する。
- **確認方法**: `OBLIVION_RUST_FORCE_REBUILD=1 bash scripts/build_host_rust.sh` が exit 0 で
  `target/gnu/x86_64-pc-windows-gnu/release/lib*_ffi.a` を 6 本出す。
