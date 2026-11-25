#!/usr/bin/env bash
# run_fixture_gates.sh — 合成フィクスチャのゲート（実データ無しでパーサ回帰を判定）。
#
# 背景:
#   Bethesda のアセットはリポジトリに入らない（.gitignore の Bethesda 節）。だから
#   「データ欠如＝失敗」にすると CI は永久に赤になる。そこで **自作バイト列の最小
#   フィクスチャ** を使い、パーサの挙動を固定する。
#
# 検査（3 点セットのうち「生成器とフィクスチャの drift」を担う）:
#   1. 生成器（tools/host_tests/make_fixture_*.py）を再実行し、**コミット済みの
#      フィクスチャとバイト一致**すること。生成器が壊れればフィクスチャも壊れる＝二重に守る。
#   2. フィクスチャが 0 件なら**失敗**（欠如を黙って通さない。--require と同じ扱い）。
#   3. 実コーパス（tmp/*_oracle）の有無は **別ラベル**で報告する。合成フィクスチャは
#      パーサ挙動の固定のみで、**実データ互換は証明しない**（混同しない）。
#
# 終了コード: 0 = OK / 1 = 失敗
set -euo pipefail

cd "$(dirname "$0")/.."   # repo root

FIXDIR="docs/evidence/fixtures"
fail=0

shopt -s nullglob
gens=(tools/host_tests/make_fixture_*.py)
if [ ${#gens[@]} -eq 0 ]; then
  echo "::error::生成器（tools/host_tests/make_fixture_*.py）がありません"
  exit 1
fi

echo "== regenerate fixtures =="
for g in "${gens[@]}"; do
  echo "  $g"
  python3 "$g"
done

# 生成器とコミット済みフィクスチャの drift。
if ! git diff --quiet -- "$FIXDIR"; then
  echo "::error::フィクスチャが生成器と一致しません（生成器かフィクスチャが drift している）"
  git diff -- "$FIXDIR" || true
  fail=1
else
  echo "OK: 生成器の再実行でフィクスチャに差分なし"
fi

# フィクスチャの存在（欠如は失敗）。
shopt -s nullglob
files=("$FIXDIR"/*)
if [ ${#files[@]} -eq 0 ]; then
  echo "::error::$FIXDIR にフィクスチャが 0 件（欠如は失敗として扱う）"
  exit 1
fi
echo "fixtures: ${#files[@]} 件"
for f in "${files[@]}"; do
  printf '  %s  %s bytes\n' "$(basename "$f")" "$(wc -c < "$f" | tr -d ' ')"
done

# 期待値の assert: フィクスチャを **リポジトリの実パーサ** に通す。
# フィクスチャ欠如や期待値の変化は失敗として扱う（--require と同じ）。
if [ "${FIXTURE_PARSE:-on}" != "off" ]; then
  echo "== parse fixtures with the real parsers =="
  if ! bash tools/host_tests/run_fixture_parse.sh; then
    echo "::error::フィクスチャの parse 期待値が不一致（またはツールのビルド失敗）"
    fail=1
  fi

  # 陰性対照: 壊した fixture で消費者が **落ちること** と、その **落ち方** を確認する。
  # これが無いと「ヘッダを読んで比較しているだけ」の消費者でも緑になり得る。
  # 壊した fixture は **実行時に生成器の出力から派生**させる（別ファイルとして
  # コミットしない＝生成器と二重管理にしない）。
  _negdir="$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/fxneg.$$")"
  mkdir -p "$_negdir"

  # 壊し方ごとの派生（runtime・コミットしない）。
  python3 - "$FIXDIR" "$_negdir" <<'PY'
import sys
fix, neg = sys.argv[1], sys.argv[2]

def load(name):
    return bytearray(open("%s/%s" % (fix, name), "rb").read())

def save(name, data):
    open("%s/%s" % (neg, name), "wb").write(bytes(data))
    print("wrote %s/%s (%d bytes)" % (neg, name, len(data)))

d = load("minimal.dds"); d[0] ^= 0xFF; save("broken.dds", d)          # DDS magic
n = load("minimal.nif"); n[0] ^= 0xFF; save("broken_magic.nif", n)    # NIF magic
n2 = load("minimal.nif")                                              # NIF block type swap
i = bytes(n2).find(b"NiTriShape")
assert i >= 0, "NiTriShape not found in the fixture"
n2[i + 1] = ord("X")                                                  # NiTriShape -> XiTriShape
save("swapped_type.nif", n2)
b = load("minimal.bsa"); b[0] ^= 0xFF; save("broken.bsa", b)          # BSA magic
PY

  # expect_fail <label> <出力に含むべき文字列> <run_fixture_parse.sh への引数...>
  expect_fail() {
    _label="$1"; _marker="$2"; shift 2
    _neg_rc=0
    _neg_out="$(bash tools/host_tests/run_fixture_parse.sh "$@" 2>&1)" || _neg_rc=$?
    if [ "$_neg_rc" = 0 ]; then
      echo "::error::陰性対照[$_label] が落ちない（壊した fixture で成功した）＝検証器として不成立"
      fail=1
    elif printf '%s' "$_neg_out" | grep -qF "$_marker"; then
      echo "OK: 陰性対照[$_label] が期待どおり落ちた（出力に '$_marker'）"
    else
      echo "::error::陰性対照[$_label] が期待の落ち方をしない（'$_marker' が出力に無い）"
      printf '%s\n' "$_neg_out" | tail -20
      fail=1
    fi
  }

  expect_fail "dds-magic" '"loaded":false' "$_negdir/broken.dds" "$FIXDIR/minimal.nif" "$FIXDIR/minimal.bsa"
  expect_fail "nif-magic" '"loaded":false' "$FIXDIR/minimal.dds" "$_negdir/broken_magic.nif" "$FIXDIR/minimal.bsa"
  expect_fail "nif-type-swap" '"hasNiTriShape":0' "$FIXDIR/minimal.dds" "$_negdir/swapped_type.nif" "$FIXDIR/minimal.bsa"
  expect_fail "bsa-magic" '"opened":false' "$FIXDIR/minimal.dds" "$FIXDIR/minimal.nif" "$_negdir/broken.bsa"

  rm -rf "$_negdir"
fi

# 実コーパスは別ラベル（スキップ）。合成フィクスチャと混同しない。
if [ -d tmp/nif_oracle ] || [ -d tmp/esm_oracle ] || [ -d tmp/bsa_oracle ] || [ -d tmp/dds_oracle ]; then
  echo "note: 実コーパス（tmp/*_oracle）が存在する。実データ互換のゲートは別ジョブ／ローカルの領分"
  echo "      （本スクリプトは合成フィクスチャのみを見る）"
else
  echo "::warning::実コーパス（tmp/*_oracle）が無いため実データ互換ゲートはスキップ（合成フィクスチャは別物）"
fi

exit "$fail"
