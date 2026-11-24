#!/usr/bin/env bash
# newgame.sh — 既定経路のスモークゲート（タイトル -> chargen 到達）。
#
# 目的: 「既定経路が実際に繋がっているか」を機械判定する。ホストテストでは落ちない
# （2026-11 の停滞の主因が「既定経路の実機検証 0」だったため、ここを完了条件に入れる）。
# 実際にこのゲートは初回実行で master の SIGABRT（bad_weak_ptr）を検出した。
#
# 設計:
#   - 画面サイズ・密度・向きを固定する。ゴールデンは 32x18 グリッドなので解像度に敏感で、
#     エミュレータのスキンが変わると全セルがずれて偽陽性になる。終了時に reset する。
#   - タップ座標は決め打ちしない。title_screen の `Menu hit rect [i] '<label>': x=[..] y=[..]`
#     をログから読み、その中央を叩く（解像度を変えても追随する）。
#   - 起動直後は旧サイズ（実測 1920x1080）で 1 度レイアウトされるため、上書き後の幅で
#     レイアウトされた矩形を待つ（古い矩形を掴むとタップが外れる）。
#   - ログは **ストリーミングでファイルへ落とす**。既定のリングバッファは 256 KiB しかなく、
#     DistantLodManager 等の多量ログでマーカー行がポーリング前に消える（実測 2026-11-23）。
#   - 指標ログが出なければ失敗。「データが無いので ok」を許さない。
#   - スクショが空なら失敗（`|| true` で握り潰さない）。
#
# 前提: エミュレータ（`wm size` を書き換えるため）。物理端末では使わない。
#
# 使い方:
#   bash docs/smoke-tests/newgame.sh [APK]
# 環境変数:
#   SMOKE_TIMEOUT_S  各段の待ち秒数（既定 180）
#   SMOKE_SETTLE_S   マーカー後に画面が落ち着くまでの秒数（既定 2）
#   OUT_DIR          成果物の出力先（既定 artifacts）
#   SMOKE_EXPECTED_WIDTH  wm size で固定する幅（既定 1280）
set -euo pipefail

APK="${1:-}"
PKG="com.example.oblivion"
NEW_GAME_INDEX=1                 # menuItems: 0=Continue 1=New Game 2=Load 3=Options 4=Credits 5=Exit
MARKER_TAP="Menu selection: New Game"
# chargen が開いた決定的マーカー（renderer.cpp:5011）。Panel 既定の経路で必ず出る。
MARKER_CHARGEN="Game started! Opening character creation"
TIMEOUT_S="${SMOKE_TIMEOUT_S:-180}"
SETTLE_S="${SMOKE_SETTLE_S:-2}"
OUT_DIR="${OUT_DIR:-artifacts}"
EXPECTED_W="${SMOKE_EXPECTED_WIDTH:-1280}"

ADB="$(command -v adb || command -v adb.exe || true)"
if [ -z "$ADB" ]; then
  # Windows は LOCALAPPDATA（バックスラッシュを / へ正規化）、その他は環境変数の SDK を見る。
  for base in "${LOCALAPPDATA//\\//}" "${ANDROID_SDK_ROOT:-}" "${ANDROID_HOME:-}"; do
    [ -n "$base" ] || continue
    for cand in "$base/Android/Sdk/platform-tools/adb.exe" \
                "$base/Android/Sdk/platform-tools/adb" \
                "$base/platform-tools/adb.exe" \
                "$base/platform-tools/adb"; do
      if [ -x "$cand" ]; then ADB="$cand"; break 2; fi
    done
  done
fi
if ! command -v "$ADB" >/dev/null 2>&1 && [ ! -x "$ADB" ]; then
  echo "::error::adb が見つかりません（PATH か LOCALAPPDATA/Android/Sdk/platform-tools）"
  exit 1
fi

# 対象端末を決める。複数接続時は emulator を優先する。
# このスクリプトは `wm size` を書き換えるため、物理端末で走ると表示設定が残る
# （2026-11-23 に実際に起こした事故）。よって emulator 以外は明確に拒否する。
if [ -n "${ANDROID_SERIAL:-}" ]; then
  TARGET="$ANDROID_SERIAL"
else
  # adb の出力は Windows では CRLF なので、比較前に \r を落とす。
  DEVICES="$("$ADB" devices | tr -d '\r' | awk 'NR>1 && $2=="device" {print $1}')"
  TARGET="$(printf '%s\n' "$DEVICES" | grep -m1 '^emulator-' || true)"
  if [ -z "$TARGET" ]; then
    TARGET="$(printf '%s\n' "$DEVICES" | head -n 1 || true)"
  fi
fi
if [ -z "$TARGET" ]; then
  echo "::error::対象端末が見つかりません"; "$ADB" devices; exit 1
fi
case "$TARGET" in
  emulator-*) : ;;
  *)
    echo "::error::対象がエミュレータではありません（$TARGET）。wm size を書き換えるため中止します。"
    echo "  → エミュレータを起動するか、ANDROID_SERIAL=emulator-5554 を指定してください。"
    exit 1
    ;;
esac
# adb は ANDROID_SERIAL を尊重するが、WSL から Windows の adb.exe を呼ぶ場合は
# Linux 側の環境変数が interop で渡らない（実測 2026-11-23）。よって `-s` を明示する
# ラッパを噛ませ、以降の呼び出しはすべてこの端末へ向ける。
WRAP_DIR="$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/adbwrap.$$")"
mkdir -p "$WRAP_DIR"
printf '#!/bin/sh\nexec "%s" -s "%s" "$@"\n' "$ADB" "$TARGET" > "$WRAP_DIR/adb"
chmod +x "$WRAP_DIR/adb"
ADB="$WRAP_DIR/adb"
echo "target device: $TARGET"

mkdir -p "$OUT_DIR"
LOGCAT_FILE="$OUT_DIR/smoke-newgame.log"
LOGCAT_PID=""

cleanup() {
  if [ -n "$LOGCAT_PID" ]; then
    kill "$LOGCAT_PID" >/dev/null 2>&1 || true
  fi
  # 表示の上書きは物理端末に残るため、必ず戻す。
  "$ADB" shell wm size reset >/dev/null 2>&1 || true
  "$ADB" shell wm density reset >/dev/null 2>&1 || true
  # 端末固定のラッパを片付ける。
  if [ -n "${WRAP_DIR:-}" ]; then
    rm -rf "$WRAP_DIR" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

log_dump() {
  echo "--- 関連ログ（Menu / chargen / Title） ---"
  grep -E "Menu hit|Menu selection|character creation|Game started|TitleScreen|tombstone" "$LOGCAT_FILE" 2>/dev/null | tail -n 30 || true
  echo "--- ログ末尾 ---"
  tail -n 40 "$LOGCAT_FILE" 2>/dev/null || true
}

echo "== smoke: newgame =="
echo "adb: $ADB"

"$ADB" wait-for-device

# 1) 表示条件を固定する（ゴールデンの前提）。
"$ADB" shell wm size 1280x960
"$ADB" shell wm density 240
"$ADB" shell settings put system accelerometer_rotation 0
"$ADB" shell settings put system user_rotation 1
# 反映前にアプリが起動すると旧サイズでレイアウトし、矩形とタップがずれる（実測）。
sleep 3
echo "display fixed: 1280x960 / 240dpi / rotation 1"

if [ -n "$APK" ]; then
  if [ ! -s "$APK" ]; then
    echo "::error::APK が無いか空です: $APK"
    exit 1
  fi
  echo "install: $APK"
  "$ADB" install -r -t "$APK"
fi

# リングバッファを広げてから、ストリーミングでファイルへ落とす。
"$ADB" logcat -G 16M >/dev/null 2>&1 || true
"$ADB" logcat -c >/dev/null 2>&1 || true
"$ADB" logcat -v time > "$LOGCAT_FILE" 2>/dev/null &
LOGCAT_PID=$!

"$ADB" shell am force-stop "$PKG" || true
"$ADB" shell monkey -p "$PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1 || true

# 2) 幅 EXPECTED_W でレイアウトされたメニュー矩形を待つ（起動直後の旧サイズを弾く）。
deadline=$(( $(date +%s) + TIMEOUT_S ))
rect_line=""
screen_size=""
while [ "$(date +%s)" -lt "$deadline" ]; do
  screen_line="$(grep -F "Menu hit layout" "$LOGCAT_FILE" 2>/dev/null | tail -n 1 || true)"
  screen_size="$(printf '%s' "$screen_line" | sed -n 's/.*(screen \([0-9]*x[0-9]*\)).*/\1/p')"
  if [ -n "$screen_size" ] && [ "${screen_size%%x*}" = "$EXPECTED_W" ]; then
    sleep 1  # 同期的に出る rect 行を確実に拾う
    rect_line="$(grep -F "Menu hit rect [${NEW_GAME_INDEX}]" "$LOGCAT_FILE" 2>/dev/null | tail -n 1 || true)"
    [ -n "$rect_line" ] && break
  fi
  sleep 2
done

if [ -z "$rect_line" ]; then
  echo "::error::幅 ${EXPECTED_W} でレイアウトされたメニュー矩形（Menu hit rect [${NEW_GAME_INDEX}]）が ${TIMEOUT_S}s 以内に出ませんでした（最後の screen=${screen_size:-unknown}）"
  log_dump
  exit 1
fi
echo "rect: $rect_line"
echo "screen: ${screen_size:-unknown}"

# 負値も取れるようにする（x=[-10..200] のような行で壊れないため）。
x1=$(printf '%s' "$rect_line" | sed -n 's/.*x=\[\(-\?[0-9.]*\)\.\..*/\1/p')
x2=$(printf '%s' "$rect_line" | sed -n 's/.*x=\[-\?[0-9.]*\.\.\(-\?[0-9.]*\)\].*/\1/p')
y1=$(printf '%s' "$rect_line" | sed -n 's/.*y=\[\(-\?[0-9.]*\)\.\..*/\1/p')
y2=$(printf '%s' "$rect_line" | sed -n 's/.*y=\[-\?[0-9.]*\.\.\(-\?[0-9.]*\)\].*/\1/p')
if [ -z "$x1" ] || [ -z "$x2" ] || [ -z "$y1" ] || [ -z "$y2" ]; then
  echo "::error::メニュー矩形を解析できませんでした: $rect_line"
  log_dump
  exit 1
fi
tap_x=$(awk -v a="$x1" -v b="$x2" 'BEGIN{printf "%d", (a+b)/2}')
tap_y=$(awk -v a="$y1" -v b="$y2" 'BEGIN{printf "%d", (a+b)/2}')
echo "tap New Game at ($tap_x, $tap_y)"

# 境界検査: 矩形がアプリの画面内にあるか。外ならタップがずれるので先に落とす。
if [ -n "$screen_size" ]; then
  scr_w="${screen_size%x*}"
  scr_h="${screen_size#*x}"
  if awk -v a="$x2" -v w="$scr_w" 'BEGIN{exit !(a > w)}' ||
     awk -v a="$y2" -v h="$scr_h" 'BEGIN{exit !(a > h)}'; then
    echo "::error::メニュー矩形がアプリ画面（$screen_size）の外にあります: $rect_line"
    echo "  → 表示サイズの上書きがアプリに反映されていません（'$ADB' shell wm size を確認）"
    log_dump
    exit 1
  fi
fi

# 3) New Game を叩く。
"$ADB" shell input tap "$tap_x" "$tap_y"

deadline=$(( $(date +%s) + TIMEOUT_S ))
tapped=0
while [ "$(date +%s)" -lt "$deadline" ]; do
  if grep -Fq "$MARKER_TAP" "$LOGCAT_FILE" 2>/dev/null; then tapped=1; break; fi
  sleep 2
done
if [ "$tapped" != "1" ]; then
  echo "::error::New Game の選択（'$MARKER_TAP'）が ${TIMEOUT_S}s 以内に確認できませんでした（タップが外れた可能性）"
  log_dump
  exit 1
fi
echo "tap confirmed: $MARKER_TAP"

# 4) chargen が開いたマーカーを待つ（固定待ちでフェード途中を掴まないため）。
deadline=$(( $(date +%s) + TIMEOUT_S ))
opened=0
while [ "$(date +%s)" -lt "$deadline" ]; do
  if grep -Fq "$MARKER_CHARGEN" "$LOGCAT_FILE" 2>/dev/null; then opened=1; break; fi
  sleep 1
done
if [ "$opened" != "1" ]; then
  echo "::error::chargen が開いたマーカー（'$MARKER_CHARGEN'）が ${TIMEOUT_S}s 以内に出ませんでした"
  log_dump
  exit 1
fi
echo "chargen opened: $MARKER_CHARGEN"

sleep "$SETTLE_S"
"$ADB" exec-out screencap -p > "$OUT_DIR/smoke-newgame.png" 2>/dev/null || true

if [ ! -s "$OUT_DIR/smoke-newgame.png" ]; then
  echo "::error::スクリーンショットが空です: $OUT_DIR/smoke-newgame.png"
  log_dump
  exit 1
fi

# 任意の追加タップ（"x,y"）。chargen 表示後に 1 回だけ押して 2 枚目を撮る。
# 例: SMOKE_EXTRA_TAP=122,379 bash docs/smoke-tests/newgame.sh   # Main の Eyes トグル
if [ -n "${SMOKE_EXTRA_TAP:-}" ]; then
  tx="${SMOKE_EXTRA_TAP%,*}"
  ty="${SMOKE_EXTRA_TAP#*,}"
  echo "extra tap: ($tx, $ty)"
  "$ADB" shell input tap "$tx" "$ty"
  sleep 2
  "$ADB" exec-out screencap -p > "$OUT_DIR/smoke-newgame-after-tap.png" 2>/dev/null || true
  [ -s "$OUT_DIR/smoke-newgame-after-tap.png" ] || { echo "::error::2 枚目が空です"; exit 1; }
fi

echo "PASS: 既定経路が chargen まで繋がった"
echo "screenshot: $OUT_DIR/smoke-newgame.png"
