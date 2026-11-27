#!/usr/bin/env bash
# repro-esm-race.sh — loose Oblivion.esm の部分読みレース（#186）の再現と確認。
#
# 何を試すか:
#   アプリを ESM 不在で起動し、1 Hz の maybeRetryGameDataLoad が回っている間に
#   アプリの data dir へ ESM を「ゆっくり」書き込む（実機でコピー中のファイルを
#   読んでしまう状況の再現）。修正前は書き込み途中のファイルが受理され、
#   `[OK] Loaded Oblivion.esm` のサイズが実サイズと違う（= 壊れた世界が静かに
#   立ち上がる）。修正後はサイズが落ち着くまで読まず、完了後にだけ受理する。
#
# 判定（PASS 条件）:
#   1. 書き込み完了より前に `Loaded Oblivion.esm` / `accepted esm size` が出ない。
#   2. 書き込み完了後に `accepted esm size=<実サイズ>` が出る。
#
# 使い方:
#   ADB=<adb のパス> bash scripts/repro-esm-race.sh
#   ESM_SRC=/data/local/tmp/Oblivion.esm   （world-readable な実 ESM を置く）
#
# 前提:
#   - ADB は root（emulator-5554 は `adb root` 済み）。`run-as` でアプリ data dir を触る。
#   - data dir の ESM は消してから書き直す。終了時に実サイズの ESM を戻す（trap）。
set -uo pipefail

ADB=( "${ADB:-adb}" -s "${ANDROID_SERIAL:-emulator-5554}" )
PKG="com.example.oblivion"
DATA="/data/user/0/$PKG/files/data"
SRC="${ESM_SRC:-/data/local/tmp/Oblivion.esm}"
OUT="${OUT_DIR:-artifacts}"
LOG="$OUT/repro-esm-race.log"
CHUNK=$((1024 * 1024))          # 1 MiB
CHUNK_SLEEP="${CHUNK_SLEEP:-0.2}"
STABLE_WAIT_S="${STABLE_WAIT_S:-180}"
LAUNCH_WAIT_S="${LAUNCH_WAIT_S:-8}"

fail() { echo "::error::$*"; exit 2; }

mkdir -p "$OUT"
"${ADB[@]}" wait-for-device >/dev/null
command -v grep >/dev/null || fail "grep が必要"

REAL_SIZE=$("${ADB[@]}" shell "stat -c %s $SRC 2>/dev/null || wc -c < $SRC" | tr -d '\r\n ')
[[ "$REAL_SIZE" =~ ^[0-9]+$ ]] || fail "ESM の実サイズを取得できません: $SRC"
CHUNKS=$(( REAL_SIZE / CHUNK ))
REST_OFF=$(( CHUNKS * CHUNK ))

echo "== repro #186: loose ESM partial-read race =="
echo "  source: $SRC ($REAL_SIZE bytes, $CHUNKS chunks of 1 MiB)"

restore_full_esm() {
  echo "  restoring a complete ESM"
  "${ADB[@]}" shell am force-stop "$PKG" >/dev/null 2>&1
  "${ADB[@]}" shell "rm -f $DATA/Oblivion.esm" >/dev/null 2>&1
  "${ADB[@]}" shell "run-as $PKG sh -c 'cat $SRC > files/data/Oblivion.esm'" >/dev/null 2>&1
  local now
  now=$("${ADB[@]}" shell "stat -c %s $DATA/Oblivion.esm" | tr -d '\r\n ')
  echo "  restored size: $now (expected $REAL_SIZE)"
  if [ "$now" != "$REAL_SIZE" ]; then
    echo "::warning::ESM の復元サイズが実サイズと違います: $now != $REAL_SIZE"
  fi
}
trap restore_full_esm EXIT

# 1. アプリを止めて ESM を消す（消えたことを確認する。残っていると append で倍になる）。
"${ADB[@]}" shell am force-stop "$PKG" >/dev/null 2>&1
"${ADB[@]}" shell "rm -f $DATA/Oblivion.esm" >/dev/null 2>&1
if "${ADB[@]}" shell "test -e $DATA/Oblivion.esm && echo present" | grep -q present; then
  fail "Oblivion.esm を消せませんでした（root 権限と SELinux を確認）"
fi
echo "  ESM removed"

# 2. ESM 不在で起動し、リトライが回り始めるまで待つ。
: > "$LOG"
"${ADB[@]}" logcat -c >/dev/null 2>&1
"${ADB[@]}" logcat -v time > "$LOG" 2>/dev/null &
LOGPID=$!
"${ADB[@]}" shell monkey -p "$PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1
echo "  app launched without ESM; waiting ${LAUNCH_WAIT_S}s for the retry loop"
sleep "$LAUNCH_WAIT_S"

# 3. ゆっくり書き込む（1 MiB ずつ append）。増えている最中を掴ませる。
echo "  slow write: 1 MiB every ${CHUNK_SLEEP}s"
"${ADB[@]}" shell "run-as $PKG sh -c '
  i=0
  while [ \$i -lt $CHUNKS ]; do
    dd if=$SRC bs=$CHUNK skip=\$i count=1 2>/dev/null >> files/data/Oblivion.esm
    i=\$((i+1))
    sleep $CHUNK_SLEEP
  done
  dd if=$SRC bs=1 skip=$REST_OFF count=$(( $REAL_SIZE - $REST_OFF )) 2>/dev/null >> files/data/Oblivion.esm
'" || fail "書き込みコマンドが失敗しました"
echo "  write complete; data dir size: $("${ADB[@]}" shell "stat -c %s $DATA/Oblivion.esm" | tr -d '\r\n ')"

# 4. 書き込み完了後、受理されるまで待つ（サイズ安定 1.5s + パース）。
echo "  waiting up to ${STABLE_WAIT_S}s for acceptance after the copy finished"
deadline=$(( $(date +%s) + STABLE_WAIT_S ))
accepted=""
while [ "$(date +%s)" -lt "$deadline" ]; do
  accepted=$(grep -F "accepted esm size=" "$LOG" | tail -1)
  [ -n "$accepted" ] && break
  sleep 3
done
kill "$LOGPID" 2>/dev/null || true
wait "$LOGPID" 2>/dev/null || true

echo "  --- acceptance lines ---"
grep -F "Loaded Oblivion.esm" "$LOG" | sed 's/^/    /'
echo "  --- rejection lines (expected while the copy was growing) ---"
grep -F "Oblivion.esm rejected" "$LOG" | tail -3 | sed 's/^/    /'

# 5. 判定。
#    (a) 実サイズ以外で受理した行が 1 つも無い（= 部分読みの受理が無い）。
#    (b) 実サイズで受理した行がある。
if grep -F "Loaded Oblivion.esm" "$LOG" | grep -qF -v "accepted esm size=$REAL_SIZE"; then
  echo "::error::実サイズ以外で受理された記録があります（部分読み）:"
  grep -F "Loaded Oblivion.esm" "$LOG" | grep -F -v "accepted esm size=$REAL_SIZE" | sed 's/^/    /'
  exit 2
fi
if [ -z "$accepted" ]; then
  fail "コピー完了後に ESM が受理されませんでした（ログ: $LOG）"
fi
if ! echo "$accepted" | grep -qF "accepted esm size=$REAL_SIZE"; then
  fail "受理サイズが実サイズと違います: $accepted (expected $REAL_SIZE)"
fi

echo "PASS: コピー中は受理せず、完了後にだけ $REAL_SIZE bytes で受理した"
echo "log: $LOG"
