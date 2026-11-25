#!/usr/bin/env bash
# run_fixture_parse.sh - 合成フィクスチャを **リポジトリの実パーサ**に通して期待値を固定する。
#
# run_smoke.sh と同じ「source して再利用」方式（run_host_tests.sh を SOURCE_ONLY で読み、
# その INCLUDES/DEFINES/RUST_LIBS をそのまま使う）。Panel レーンのファイルは編集しない。
#
# 使い方: tools/host_tests/run_fixture_parse.sh
# 終了コード: ツールのもの（0 ok / 1 期待値不一致 / 2 読めず）＋ 1 on build failure
set -euo pipefail

cd "$(dirname "$0")/../.."   # repo root

# ホスト Rust アーカイブ（lib*_ffi.a）を先に保証する。run_host_tests.sh は .a が無いと
# 即エラーで抜けるため source より前に必要。混在ツールチェーンの E0786 再発防止として
# scripts/build_host_rust.sh に集約してある（既存 .a があれば作り直さない）。
bash scripts/build_host_rust.sh

# RUST_LIB_DIR / INCLUDES / DEFINES / EXTRA_LIBS / RUST_LIBS / SOURCES を取り込む
# （ビルドやユニットテスト実行はしない）。
HOST_TESTS_SOURCE_ONLY=1 source tools/host_tests/run_host_tests.sh

BIN="${TMPDIR:-/tmp}/host_tests/fixture_parse"
mkdir -p "$(dirname "$BIN")"

# まず最小構成で試し、リンクに失敗したら smoke と同じ「全ソース − ランナー main」へ落とす。
# 最小構成が通ればビルドは数秒で終わる（全ソースは 40 分超）。
CURATED=(
  tools/host_tests/host_stub_syms.cpp
  tools/host_tests/host_gl_stubs.cpp
  app/src/main/cpp/assets/dds_loader.cpp
  tools/host_tests/fixture_parse_main.cpp
)

FULL=()
for _src in "${SOURCES[@]}"; do
  case "$_src" in
    tools/host_tests/host_runner_main.cpp) continue ;;
  esac
  FULL+=("$_src")
done
FULL+=(tools/host_tests/fixture_parse_main.cpp)

build() {
  g++ -std=c++17 -O0 -ffunction-sections -fdata-sections \
      "${DEFINES[@]}" "${INCLUDES[@]}" "$@" \
      "${RUST_LIBS[@]}" ${EXTRA_LIBS[@]+"${EXTRA_LIBS[@]}"} \
      -Wl,--gc-sections -o "$BIN" -lz
}

echo "::group::Build fixture parse" >&2
if build "${CURATED[@]}" 2>/tmp/fixture_parse_curated.err; then
  :
else
  echo "curated 構成でリンクできず。全ソース（ランナー main 差し替え）で再試行する" >&2
  cat /tmp/fixture_parse_curated.err >&2 || true
  build "${FULL[@]}"
fi
echo "::endgroup::" >&2

echo "::group::Run fixture parse" >&2
"$BIN" "$@"
echo "::endgroup::" >&2
