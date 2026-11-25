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

  # 陰性対照: 壊した fixture で消費者が **落ちること** を確認する。
  # これが無いと「ヘッダを読んで比較しているだけ」の消費者でも緑になり得る。
  # 重要なのは「落ち方」: パーサが拒否した（`loaded:false`）のであって、
  # リンク失敗・ファイル不在・クラッシュで落ちていないこと。
  _negdir="$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/fxneg.$$")"
  mkdir -p "$_negdir"
  # 壊した fixture は **実行時に生成器の出力から派生**させる（別ファイルとして
  # コミットしない＝生成器と二重管理にしない）。
  python3 - "$FIXDIR/minimal.dds" "$_negdir/broken.dds" <<'PY'
import sys
src, dst = sys.argv[1], sys.argv[2]
data = bytearray(open(src, "rb").read())
data[0] ^= 0xFF          # DDS マジックを潰す
open(dst, "wb").write(bytes(data))
print("wrote %s (%d bytes, magic smashed)" % (dst, len(data)))
PY
  _neg_rc=0
  _neg_out="$(bash tools/host_tests/run_fixture_parse.sh "$_negdir/broken.dds" 2>&1)" || _neg_rc=$?
  if [ "$_neg_rc" = 0 ]; then
    echo "::error::陰性対照が落ちない（壊した fixture で消費者が成功した）＝検証器として不成立"
    fail=1
  elif printf '%s' "$_neg_out" | grep -q '"loaded":false'; then
    echo "OK: 陰性対照（マジックを潰した fixture）で消費者がパーサ拒否（loaded:false）で落ちた"
  else
    echo "::error::陰性対照が「パーサ拒否」以外で落ちた（リンク失敗・ファイル不在・クラッシュの疑い）"
    printf '%s\n' "$_neg_out" | tail -20
    fail=1
  fi
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
