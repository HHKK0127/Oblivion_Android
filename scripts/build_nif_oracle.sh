#!/usr/bin/env bash
# build_nif_oracle.sh — NIF 差分オラクルの harness（dump_rust / dump_orig）をビルドする。
#
# 背景: オラクルはローカル専用（tmp/nif_oracle は gitignore）で、ビルド経路が文書化
# されていないと再現しない（2026-11-26 に dump_rust.exe が 9 日古いままそれと気づかず
# 使った）。ここで固定する。
#
# 入力:
#   tools/nif_oracle/{dump.cpp, log_stub.cpp, android/log.h}  — コミット済み driver
#   tmp/nif_oracle/orig_src/nif_parser.*  — 元 C++ パーサ（ローカル専用・任意。あれば dump_orig を作る）
#   app/src/main/cpp/{assets,include}, rust/crates/nif-ffi/include  — 現行実装
# 出力:
#   tmp/nif_oracle/dump_rust[.exe]  — 現行の Rust 裏付け NIFParser 版
#   tmp/nif_oracle/dump_orig[.exe]  — 元の C++ パーサ版（orig_src があるときだけ）
#
# Windows(Git Bash) は g++ ＋ MinGW の Windows 同期 lib（-lsynchronization 等）。
# Linux(CI) は素の g++。
# 使い方: scripts/build_nif_oracle.sh
set -euo pipefail

cd "$(dirname "$0")/.."                     # repo root
DRIVER=tools/nif_oracle
OUT=tmp/nif_oracle
[ -f "$DRIVER/dump.cpp" ] || {
  echo "::error::$DRIVER/dump.cpp がありません" >&2
  exit 1
}
mkdir -p "$OUT"

# ホスト Rust アーカイブ（libnif_ffi.a）を最新にする。WSL ガードは build_host_rust.sh 側。
bash scripts/build_host_rust.sh >&2

CXX="${CXX:-g++}"
EXTRA_LIBS=()
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    LIB="rust/target/gnu/x86_64-pc-windows-gnu/release/libnif_ffi.a"
    # Rust std の Windows 同期プリミティブは libsynchronization.a にある（gotchas #3）。
    EXTRA_LIBS=(-lsynchronization -lntdll -lws2_32 -lbcrypt -luserenv)
    EXE=".exe"
    ;;
  *)
    LIB="rust/target/release/libnif_ffi.a"
    EXE=""
    ;;
esac
[ -f "$LIB" ] || { echo "::error::$LIB がありません" >&2; exit 1; }

"$CXX" -std=c++17 -O2 \
  -I "$DRIVER" -I app/src/main/cpp/assets -I app/src/main/cpp/include \
  -I rust/crates/nif-ffi/include \
  "$DRIVER/dump.cpp" app/src/main/cpp/assets/nif_parser.cpp "$DRIVER/log_stub.cpp" \
  "$LIB" ${EXTRA_LIBS[@]+"${EXTRA_LIBS[@]}"} \
  -o "$OUT/dump_rust$EXE"
echo "built $OUT/dump_rust$EXE"

# dump_orig は元の C++ パーサ（orig_src/nif_parser.h を先に見つけさせる）。
# nif_types.h / nif_block_type_map.* は orig_src に無いので app/assets から補う。
if [ -f "$OUT/orig_src/nif_parser.cpp" ]; then
  "$CXX" -std=c++17 -O2 \
    -I "$OUT/orig_src" -I app/src/main/cpp/assets -I app/src/main/cpp/include -I "$DRIVER" \
    "$DRIVER/dump.cpp" "$OUT/orig_src/nif_parser.cpp" app/src/main/cpp/assets/nif_block_type_map.cpp \
    "$DRIVER/log_stub.cpp" \
    -o "$OUT/dump_orig$EXE"
  echo "built $OUT/dump_orig$EXE"
fi
