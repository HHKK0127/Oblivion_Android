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
#   bash docs/smoke-tests/newgame.sh [APK] [--no-esm]
# --no-esm:
#   アプリの data dir にある loose ゲームデータ（*.esm/*.esp/*.bsa）を一時的に隠して
#   起動する。CI の smoke は実データを積まない（no-ESM）ので、golden を採る条件を
#   ローカルでも 1 か所で再現するため。終了時に元へ戻す（trap）。
# 環境変数:
#   SMOKE_TIMEOUT_S  各段の待ち秒数（既定 180）
#   SMOKE_SETTLE_FRAMES    画面が落ち着いたとみなす「連続同一 screencap」の回数（既定 3）
#   SMOKE_SETTLE_INTERVAL  撮影間隔 秒（既定 0.7）
#   SMOKE_SETTLE_DIFF_MAX  「同一」とみなす変化画素の割合の上限（既定 0.001 = 0.1%）
#   SMOKE_SETTLE_MAX       安定待ちの上限 秒（既定 20。超えたら warning で打ち切る）
#   OUT_DIR          成果物の出力先（既定 artifacts）
#   SMOKE_EXPECTED_WIDTH  wm size で固定する幅（既定 1280）
set -euo pipefail

APK=""
NO_ESM=0
for _arg in "$@"; do
  case "$_arg" in
    --no-esm) NO_ESM=1 ;;
    *)        APK="$_arg" ;;
  esac
done
PKG="com.example.oblivion"
NEW_GAME_INDEX=1                 # menuItems: 0=Continue 1=New Game 2=Load 3=Options 4=Credits 5=Exit
MARKER_TAP="Menu selection: New Game"
# chargen が開いた決定的マーカー（renderer.cpp:5011）。Panel 既定の経路で必ず出る。
MARKER_CHARGEN="Game started! Opening character creation"
TIMEOUT_S="${SMOKE_TIMEOUT_S:-180}"
# 安定待ちは「秒」ではなく「連続 N 回の screencap が同一」。時間依存だと、速いマシンでは
# 1 秒で足り、遅いマシンでは 2 秒でも足りなくなる。フレーム（画面の静止）で待てば速度に
# 依存しない。chargen の 1 フレームだけ message box がパネルの下に隠れる（renderer.cpp:5151
# のタイトル分岐が message box を chargen より先に描く）ので、その一過性フレームを
# 跨ぐには N 連続一致が要る。
SETTLE_FRAMES="${SMOKE_SETTLE_FRAMES:-3}"       # 同一が何連続したら落ち着いたとみなすか
SETTLE_INTERVAL="${SMOKE_SETTLE_INTERVAL:-0.7}" # 撮影間隔（秒）
SETTLE_DIFF_MAX="${SMOKE_SETTLE_DIFF_MAX:-0.001}" # 同一とみなす変化画素割合の上限
SETTLE_MAX="${SMOKE_SETTLE_MAX:-20}"            # 上限（秒）。超えたら warning で打ち切る
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

# --no-esm: アプリ data dir の loose ゲームデータを一時的に隠す（CI と同じ no-ESM 条件）。
# ファイル名は実測（emulator-5554 の data dir）に合わせる。隠し漏れは下のログ検証で落とす。
GODATA_FILES=("Oblivion.esm" "Oblivion - Meshes.bsa" "Oblivion.esp")
hide_game_data() {
  local f
  for f in "${GODATA_FILES[@]}"; do
    "$ADB" shell "run-as $PKG mv 'files/data/$f' 'files/data/$f.noesm-bak' 2>/dev/null || true" >/dev/null 2>&1 || true
  done
}
restore_game_data() {
  local f
  for f in "${GODATA_FILES[@]}"; do
    "$ADB" shell "run-as $PKG mv 'files/data/$f.noesm-bak' 'files/data/$f' 2>/dev/null || true" >/dev/null 2>&1 || true
  done
}

cleanup() {
  if [ -n "$LOGCAT_PID" ]; then
    kill "$LOGCAT_PID" >/dev/null 2>&1 || true
  fi
  # --no-esm で隠したゲームデータは必ず戻す（失敗しても表示は戻す）。
  if [ "${NO_ESM:-0}" = "1" ]; then
    restore_game_data
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

# 撮影条件をサイドカーに残す（compare_golden.py が環境一致の検査に使う）。
{
  echo "# display conditions under which this smoke ran"
  echo "wm_size=1280x960"
  echo "density=240"
  echo "rotation=1"
  if [ "$NO_ESM" = "1" ]; then
    echo "esm=absent"
  else
    echo "esm=present"
  fi
} > "$OUT_DIR/smoke-newgame.env"

if [ -n "$APK" ]; then
  if [ ! -s "$APK" ]; then
    echo "::error::APK が無いか空です: $APK"
    exit 1
  fi
  echo "install: $APK"
  "$ADB" install -r -t "$APK"
fi

# --no-esm: APK 設置後に loose ゲームデータを隠す（次回終了時に戻す）。
if [ "$NO_ESM" = "1" ]; then
  echo "no-esm mode: hiding loose game data (${GODATA_FILES[*]})"
  hide_game_data
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

# --no-esm の条件検証: ESM を隠したのに ESM ワールドが構築されていたら、golden を
# 焼く前に中止する（条件違いの golden を焼かないため）。
if [ "$NO_ESM" = "1" ]; then
  if grep -Fq "Building world from ESM data" "$LOGCAT_FILE" 2>/dev/null; then
    echo "::error::--no-esm なのに ESM ワールドが構築されました（隠し漏れ）。中止。"
    log_dump
    exit 1
  fi
  if grep -Fq "Extracted Oblivion.esm" "$LOGCAT_FILE" 2>/dev/null; then
    echo "::error::--no-esm なのに Oblivion.esm が読まれました（隠し漏れ）。中止。"
    log_dump
    exit 1
  fi
  if grep -Fq "createTestScenario: fallback NPCs/quests created" "$LOGCAT_FILE" 2>/dev/null; then
    echo "no-esm 条件 OK: ESM ワールド未構築・demo ブロック実行"
  else
    echo "no-esm 条件 OK: ESM ワールド未構築（demo マーカーは出ていない。要確認）"
  fi
fi

# 安定待ち: 連続 SETTLE_FRAMES 回「画面が同一」になるまで撮り続ける。
# 同一判定は**画素差の割合**（閾値付き）。バイト一致（cmp）は使えない: ステータスバー等が
# 毎回数百画素だけ動き PNG がバイト一致しない（実測 333/2304000 = 0.014%）。一方、
# 一過性フレーム（message box がパネルの下に隠れる 1 フレーム）は 70% 級に動くので、
# 0.1% の閾値で明確に分かれる。
SNAP="$OUT_DIR/smoke-newgame.png"
PREV="$OUT_DIR/.settle-prev.png"
PYBIN="$(command -v python || command -v python3 || true)"
if [ -z "$PYBIN" ]; then
  echo "::warning::python が無いため安定待ちをスキップします（compare_golden にも python が要ります）"
fi
# $1,$2 の変化画素の割合（|Δ|>8）を stdout に出す。python 不在なら 1.0（=変化）を返す。
screen_changed_ratio() {
  [ -n "$PYBIN" ] || { echo 1.0; return; }
  "$PYBIN" - "$1" "$2" <<'PY' 2>/dev/null || echo 1.0
import sys
from PIL import Image
import numpy as np
a = np.asarray(Image.open(sys.argv[1]).convert("L"), dtype=np.int16)
b = np.asarray(Image.open(sys.argv[2]).convert("L"), dtype=np.int16)
if a.shape != b.shape:
    print(1.0)
    raise SystemExit
print(float((np.abs(a - b) > 8).mean()))
PY
}
streak=0
elapsed=0
"$ADB" exec-out screencap -p > "$PREV" 2>/dev/null || true
while :; do
  sleep "$SETTLE_INTERVAL"
  elapsed=$(awk -v a="$elapsed" -v b="$SETTLE_INTERVAL" 'BEGIN{printf "%.2f", a+b}')
  "$ADB" exec-out screencap -p > "$SNAP" 2>/dev/null || true
  ratio=1.0
  if [ -s "$SNAP" ] && [ -s "$PREV" ]; then
    ratio="$(screen_changed_ratio "$PREV" "$SNAP")"
  fi
  if awk -v r="$ratio" -v m="$SETTLE_DIFF_MAX" 'BEGIN{exit !(r <= m)}'; then
    streak=$((streak + 1))
  else
    streak=1   # いま撮った 1 枚を新しい run の 1 枚目として数える
  fi
  cp "$SNAP" "$PREV" 2>/dev/null || true
  [ "$streak" -ge "$SETTLE_FRAMES" ] && break
  if awk -v a="$elapsed" -v b="$SETTLE_MAX" 'BEGIN{exit !(a >= b)}'; then
    echo "::warning::安定待ちが ${SETTLE_MAX}s を超えました（streak=${streak}/${SETTLE_FRAMES}, ratio=${ratio}）"
    break
  fi
done
rm -f "$PREV"
echo "settled: ${streak} 連続同一（interval=${SETTLE_INTERVAL}s, elapsed=${elapsed}s, diff<=${SETTLE_DIFF_MAX}）"

if [ ! -s "$SNAP" ]; then
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
