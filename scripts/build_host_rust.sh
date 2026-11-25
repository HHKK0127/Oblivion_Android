#!/usr/bin/env bash
# build_host_rust.sh - ホスト側 Rust FFI アーカイブ（lib*_ffi.a）を決定論的に作る。
#
# 背景（2026-11-25 実測・再発防止）:
#   共有 `target/` に **別ツールチェーン**の proc-macro 成果物が残っていると、GNU で
#   コンパイルしたときに次で落ちる:
#
#     error[E0786]: found invalid metadata files for crate `thiserror_impl`
#     = note: invalid metadata version found:
#       ...\rust\target\gnu\release\build\thiserror-impl\<hash>\thiserror_impl-<hash>.dll
#
#   原因はアーティファクトの種類ではなく **target ディレクトリの混在**（MSVC 製の
#   proc-macro `.dll` を GNU が読む）。`--target` を付けても proc-macro は
#   `target/<CARGO_TARGET_DIR>/release` 側に出るため、同じ dir を共有すると再発する。
#
#   加えて PATH 上の rustc が scoop（MSVC）だと、GNU ツールチェーンを指定しても
#   MSVC の `.lib` を作ってしまい、`lib*.a` を要求する run_host_tests.sh から見えない。
#
# 方針:
#   * **ホスト triple を実行時に取る**（CI の Linux では素通しし、Windows 固有の
#     `--target` を持ち込まない）。
#   * Windows(Git Bash/MinGW) のみ: rustup の GNU 実体へ RUSTC/toolchain を固定し、
#     `--target <gnu-triple>` ＋ `CARGO_TARGET_DIR=target/gnu`（MSVC の `target/release`
#     と混ぜない。run_host_tests.sh の探索候補 1 番目）。
#   * **staleness を潰す**: 出力 `.a` より新しい Rust ソースがあれば作り直す。
#     古いライブラリでテストが緑になる（静かに壊れる）のを防ぐ。
#
# 出力先（run_host_tests.sh が探す形）:
#   Windows: rust/target/gnu/<gnu-triple>/release/lib*_ffi.a
#   それ以外: rust/target/release/lib*_ffi.a
#
# 使い方: scripts/build_host_rust.sh
#   OBLIVION_RUST_FORCE_REBUILD=1 で staleness 判定を無視して作り直す。
set -euo pipefail

cd "$(dirname "$0")/.."                     # repo root
RUST_DIR="${OBLIVION_RUST_DIR:-$(pwd)/rust}"
[ -f "$RUST_DIR/Cargo.toml" ] || { echo "::error::rust workspace が見つかりません: $RUST_DIR"; exit 1; }

CRATES=(-p assets-ffi -p bsa-ffi -p plugin-ffi -p esm-ffi -p nif-ffi -p dds-ffi)

# --- ホスト triple を実行時に取る ------------------------------------------
HOST_TRIPLE=""
if command -v rustc >/dev/null 2>&1; then
  HOST_TRIPLE="$(rustc -vV 2>/dev/null | sed -n 's/^host: //p')"
fi
if [ -z "$HOST_TRIPLE" ]; then
  for _rustc in "$HOME/.cargo/bin/rustc" "$HOME/.rustup/toolchains/"*"/bin/rustc"; do
    [ -x "$_rustc" ] || continue
    HOST_TRIPLE="$("$_rustc" -vV 2>/dev/null | sed -n 's/^host: //p')"
    [ -n "$HOST_TRIPLE" ] && break
  done
fi

# --- プラットフォーム分岐 ----------------------------------------------------
CARGO_BIN="$(command -v cargo || true)"
[ -n "$CARGO_BIN" ] || CARGO_BIN="$HOME/.cargo/bin/cargo"

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    # Windows: 常に GNU を明示する（PATH の scoop=MSVC に負けないため）。
    GNU_TRIPLE="x86_64-pc-windows-gnu"
    for _tc in "$HOME"/.rustup/toolchains/*-pc-windows-gnu; do
      [ -x "$_tc/bin/rustc.exe" ] || continue
      export RUSTC="$_tc/bin/rustc.exe"
      export RUSTUP_TOOLCHAIN="$(basename "$_tc")"
      GNU_TRIPLE="$(basename "$_tc" | sed 's/^[^-]*-//')"
      break
    done
    CARGO_BIN="$HOME/.cargo/bin/cargo.exe"
    [ -x "$CARGO_BIN" ] || CARGO_BIN="$(command -v cargo)"
    TARGET_ARGS=(--target "$GNU_TRIPLE")
    TARGET_DIR="target/gnu"
    OUT_DIR="$TARGET_DIR/$GNU_TRIPLE/release"
    ;;
  *)
    # Linux/macOS(CI): そのまま。ホスト triple を指定しない（指定すると死ぬ）。
    TARGET_ARGS=()
    TARGET_DIR="target"
    OUT_DIR="target/release"
    ;;
esac

echo "build_host_rust: host-triple=${HOST_TRIPLE:-unknown} dir=$OUT_DIR ${TARGET_ARGS[*]:-} rustc=${RUSTC:-<PATH>}"

LIB="$RUST_DIR/$OUT_DIR/libassets_ffi.a"

# --- staleness 判定（古い .a で緑になるのを防ぐ） ----------------------------
needs_build=1
if [ -f "$LIB" ] && [ "${OBLIVION_RUST_FORCE_REBUILD:-}" != "1" ]; then
  newer="$(find "$RUST_DIR"/crates/*/src "$RUST_DIR"/crates/*/Cargo.toml \
                 "$RUST_DIR/Cargo.toml" "$RUST_DIR/Cargo.lock" \
                 -newer "$LIB" -print -quit 2>/dev/null || true)"
  if [ -z "$newer" ]; then
    echo "build_host_rust: 既存の $OUT_DIR/lib*_ffi.a は最新（作り直さない）"
    exit 0
  fi
  echo "build_host_rust: ソースが新しいので作り直す（$newer）"
fi
[ "$needs_build" = "1" ] || exit 0

cd "$RUST_DIR"
export CARGO_TARGET_DIR="$TARGET_DIR"
# 混在を断つ: この dir だけを消す（他ツールチェーンのキャッシュは触らない）。
"$CARGO_BIN" clean --target-dir "$TARGET_DIR" >/dev/null 2>&1 || true
"$CARGO_BIN" build --release ${TARGET_ARGS[@]+"${TARGET_ARGS[@]}"} "${CRATES[@]}"

# 期待した形で出たかを確認する（見つからなければ失敗）。
missing=0
for _n in assets bsa plugin esm nif dds; do
  if [ ! -f "$RUST_DIR/$OUT_DIR/lib${_n}_ffi.a" ]; then
    echo "::error::lib${_n}_ffi.a が $OUT_DIR にありません（ツールチェーン混在の可能性）"
    missing=1
  fi
done
[ "$missing" = "0" ] || exit 1
echo "build_host_rust: ok ($OUT_DIR/lib*_ffi.a)"
