#!/usr/bin/env bash
# build_apk.sh — 古い .so を積まない debug APK ビルド。
#
# 背景（2026-11-26 実測）: Rust の .a を消しても Gradle の native タスクは UP-TO-DATE の
# まま古い libnative-lib.so を積む（.a は Gradle の追跡入力ではない）。APK が修正前の
# native コードで作られ、実機検証が空振りする。手で消す運用は忘れるので、ここで固定する。
#
# 対策: ネイティブの中間物を消してから assembleDebug し、全 ABI の libnative-lib.so が
# ビルド開始より新しいことを検証する（3 本未満なら exit 1）。
#
# 使い方: scripts/build_apk.sh
set -euo pipefail

cd "$(dirname "$0")/.."                     # repo root

start=$(date +%s)
rm -rf app/.cxx app/build/intermediates/cmake app/build/intermediates/cxx
./gradlew assembleDebug

# 検証: 全 ABI の libnative-lib.so が start より新しいこと（ABI は 3 本）。
count=$(find app/build -name 'libnative-lib.so' -newermt "@$start" | wc -l | tr -d ' ')
echo "fresh libnative-lib.so: $count"
if [ "$count" -lt 3 ]; then
  echo "::error::libnative-lib.so が $count 本しか更新されていません（3 未満）。" >&2
  echo "  古い .so を積んだ APK です。app/.cxx と intermediates を消して再実行してください。" >&2
  exit 1
fi
echo "build_apk: ok"
