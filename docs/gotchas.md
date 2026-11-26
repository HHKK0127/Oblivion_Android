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

## 5. `nif_header_info.block_type_count` は型名テーブル長（`num_objects` ではない）

- **症状**: C++ shim の `NIFParser::getHeader().blockTypeNames` が実体より多く、後ろに空文字が
  並ぶ（例: 実 8 型のファイルで 15 件、`bt[8..14]=` が空）。オラクルで `dump_rust` が
  `blockTypes=15`、`dump_orig` が `blockTypes=8` と食い違う。
- **原因**: `nif-ffi/src/lib.rs` の `nif_get_header_info` が `block_type_count` に
  `header.block_type_indices.len()`（＝ブロック数＝`num_objects`）を入れていた。C++ shim はこの
  値を**型名テーブルのループ回数**に使うため、`num_objects != 型数` のファイルで後ろを読み過ぎる。
  Oblivion の実メッシュは 8032 中 6896 本（86%）で両者が異なる。
- **正しい手順**: `block_type_count` は `header.block_type_names.len()`（型名テーブル長）にする。
  ブロック数が要る箇所は `num_objects` を使う。`nif_ffi.h` のフィールドにもその旨を書く。
- **確認方法**: C ABI の `nobj != ntyp` ケース（3 blocks / 2 types で `num_objects==3` かつ
  `block_type_count==2`）と、marker 27 本のオラクル `27/27`。lodnif 100 本は `nobj==ntyp` なので
  このバグを見抜けない（100/100 のまま）。

## 6. `scripts/build_host_rust.sh` を WSL の bash で走らせない

- **症状**: `rust/target/` が丸ごと消える（`gnu/` と `cmake/` の `lib*_ffi.a` が消え、次回の
  install が古い/空の `.a` でリンクする）。
- **原因**: WSL の `bash`（`C:\WINDOWS\system32\bash.exe`）は `uname -s` が `Linux` を返すため、
  スクリプトが Linux 分岐に入り `cargo clean --target-dir target` を実行する。Windows 用の `.a` は
  作らないまま `target/` を消す。
- **正しい手順**: Git Bash（`C:\Program Files\Git\bin\bash.exe`）で実行する。スクリプトは
  `WSL_DISTRO_NAME` を見て WSL を拒否する（`OBLIVION_ALLOW_WSL=1` で上書き可）。`cargo clean` は
  `--clean`（または `OBLIVION_RUST_CLEAN=1`）のときだけ走る。CI は `run_host_tests.sh` が
  `CI=true` を見て `--clean` を付ける。
- **確認方法**: `bash --version` に `system32\bash.exe`、`uname -r` に `microsoft` が出れば WSL。
  Git Bash なら `uname -s` が `MINGW64_NT-...`。

## 7. Rust の `.a` を消しても Gradle の native タスクは UP-TO-DATE

- **症状**: `rust/target/cmake/**/lib*_ffi.a` を消して `./gradlew assembleDebug` しても、
  古い `libnative-lib.so`（前回ビルドの mtime）が APK に積まれる。実機検証が修正前のコードで
  空振りする。
- **原因**: Rust の `.a` は Gradle の追跡入力ではない。CMake のスタンプが UP-TO-DATE のままだと
  native タスクが再実行されず、`app/build/intermediates/{cmake,cxx}` の古い `.so` を拾い直す。
- **正しい手順**: `scripts/build_apk.sh` を使う（`app/.cxx` と
  `app/build/intermediates/{cmake,cxx}` を消してから `assembleDebug`）。
- **確認方法**: ビルド開始時刻より新しい `libnative-lib.so` が 3 本（ABI 数）以上あること。
  `scripts/build_apk.sh` がこれを検査し、3 未満なら exit 1。

## 8. ダンプの float は `%a` でなく生ビットで出す

- **症状**: legacy NetImmerse の空ジオメトリ（例 `marker_divine` / `marker_temple` /
  `marker_travel`）で、ダンプの float が Linux と Windows で食い違う（`-0x1.f8p-144` /
  `scale=nan`）。CI の `nif-oracle` が 27 中 3 本だけ赤になる。
- **原因**: `%a` の非正規化・NaN の表記が CRT 依存（glibc と MSVC ランタイム）。値は同一で、
  テキストだけ違う。
- **正しい手順**: float は IEEE-754 の生ビット（`%08x`）で出す（`tools/nif_oracle/dump.cpp`
  の `f32()`）。
- **確認方法**: Linux CI（`nif-oracle` ジョブ）で 27/27。
