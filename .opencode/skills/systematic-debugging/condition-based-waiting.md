# Condition-Based Waiting

出典: obra/superpowers skills/systematic-debugging/condition-based-waiting.md (MIT, commit 22f0b46) https://github.com/obra/superpowers
ライセンス全文は `references/upstream-LICENSE.txt`。骨格は upstream のまま、例と規約をこのリポジトリに合わせている。

## 核心

**待ちたいのは経過時間ではなく条件。** 固定 sleep は速いマシンで通り、負荷や CI で落ちる。

## このプロジェクトでの適用（実例: smoke の settle）

`docs/smoke-tests/newgame.sh` の画面安定待ちは、#186 で**時間ピン（固定秒）から条件待ち**に
変えた。条件は「**連続同一フレーム**」— 連続 N 枚のスクリーンショットが同じ（画素差の割合が
閾値未満）になるまで撮り続ける。

- **ノイズ床は実測する**: emulator の連続 `screencap` は、静止して見えてもときどき数百画素
  動く（ステータスバーの時計）。実測 **0.0145%**（時計で 333 / 2304000 画素）。同じ画面の
  連続 4 枚が完全一致（0 画素）のこともあるので、**ときどき**動く。
- **閾値は床から離す**: 既定 **0.1%**（`SMOKE_SETTLE_DIFF_MAX=0.001`）はノイズ床の約 7 倍。
  床の近くに置くと「永遠に収束しない」に戻る。**床の 5 倍以上を既定にする。**
- **打ち切りは必ず入れる**: 収束しなければ **`exit 2`**。warning で続行すると、収束しなかった
  run が緑になり flake が戻る。
- **バイト一致は使わない**: `cmp` は「ほとんど通る」ので失敗が再現しにくい flake になる。

## 型（bash の骨子）

```bash
SETTLE_FRAMES="${SMOKE_SETTLE_FRAMES:-3}"         # 同一が何連続したら落ち着いたとみなすか
SETTLE_INTERVAL="${SMOKE_SETTLE_INTERVAL:-0.7}"   # 撮影間隔（秒）
SETTLE_DIFF_MAX="${SMOKE_SETTLE_DIFF_MAX:-0.001}" # 同一とみなす変化画素割合の上限
SETTLE_MAX="${SMOKE_SETTLE_MAX:-20}"              # 上限（秒）

streak=0; elapsed=0
capture "$PREV"                       # ループの外で 1 枚だけ取る
while :; do
  sleep "$SETTLE_INTERVAL"; elapsed=$(add "$elapsed" "$SETTLE_INTERVAL")
  capture "$SNAP"                     # ← 状態は必ずループ内で取り直す
  ratio=$(changed_ratio "$PREV" "$SNAP")
  if below "$ratio" "$SETTLE_DIFF_MAX"; then streak=$((streak+1)); else streak=1; fi
  cp "$SNAP" "$PREV"
  [ "$streak" -ge "$SETTLE_FRAMES" ] && break
  if over "$elapsed" "$SETTLE_MAX"; then
    echo "::error::画面が ${SETTLE_MAX}s 以内に落ち着きませんでした（streak=${streak}/${SETTLE_FRAMES}）"
    exit 2
  fi
done
```

## よくある失敗

- **条件を見ず sleep だけ入れる**（固定待ち）。
- **打ち切りが無い**（条件が永遠に満たされないとハングする）。
- **ループ外で状態を取って古い値を見る**（毎回取り直す）。
- **閾値をノイズ床の近くに置く**（床の 5 倍以上を既定にする）。
- **収束しなかった run を warning で通す**（flaky ゲートが戻る）。

## 固定待ちが正しい場合

対象が**タイミング挙動そのもの**（debounce、描画途中、連打間隔）のときは可。
ただし**理由をコメントに書く**。手順は「まずトリガ条件を待ち、そのうえで既知の間隔に
基づく固定待ちを置く」。
