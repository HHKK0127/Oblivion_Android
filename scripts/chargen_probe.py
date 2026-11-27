#!/usr/bin/env python3
"""chargen_probe.py — chargen パネルのタップ反応を画素差分で測る。

1 回のタップについて「タップ前 / タップ後」のスクリーンショットを取り、
変化画素数 / 総画素数 を出す。chargen 専用の golden（`docs/evidence/golden/chargen-*.json`）
と突き合わせるときは `--golden` を渡す。世界の golden（`newgame-no-esm-prefix.json`）
とは**別ファイル**（条件が違うため混ぜない）。

前提（呼び出し側の責務）:
  - アプリは chargen パネルを表示し、スクロール 0 の状態にしてあること。
    chargen までの導線は `docs/evidence/prep/chargen-probe-baseline.md` を参照。
  - `--tap` の座標は device px。Main ページの item 中心は同 baseline の表にある。

例:
  # Eyes トグル（Main row 3）: 反転しないので 0 が期待値
  python scripts/chargen_probe.py --name eyes_main --tap 377 472 \
      --golden docs/evidence/golden/chargen-taps.json

  # Race ボタン（Main row 0）: Race ページへ遷移するので非 0
  python scripts/chargen_probe.py --name race_main --tap 377 219 \
      --golden docs/evidence/golden/chargen-taps.json

  # 2 枚の PNG を直接比較（adb を使わない・自己テスト用）
  python scripts/chargen_probe.py --before a.png --after b.png

終了コード: 0 = 一致 / 2 = golden 不一致 / 1 = ツール異常（PIL 不在・PNG 不在など）
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
import time

DEFAULT_GOLDEN = os.path.join("docs", "evidence", "golden", "chargen-taps.json")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def adb_path() -> str:
    """adb を探す（LOCALAPPDATA の SDK を優先、無ければ PATH）。"""
    cand = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Android", "Sdk",
                        "platform-tools", "adb.exe")
    if os.path.exists(cand):
        return cand
    return "adb"


def screencap(serial: str, out_path: str) -> None:
    adb = adb_path()
    with open(out_path, "wb") as fh:
        subprocess.run([adb, "-s", serial, "exec-out", "screencap", "-p"],
                       stdout=fh, check=True)


def load_gray(path: str):
    try:
        from PIL import Image
    except ImportError:
        print("::error::Pillow が必要です（python -m pip install pillow numpy）")
        sys.exit(1)
    if not os.path.exists(path):
        print("::error::画像がありません: %s" % path)
        sys.exit(1)
    return Image.open(path).convert("L")


def diff_metrics(before_path: str, after_path: str, threshold: int):
    import numpy as np
    a = load_gray(before_path)
    b = load_gray(after_path)
    if a.size != b.size:
        print("::error::サイズが違います: %s != %s" % (a.size, b.size))
        sys.exit(1)
    aa = np.asarray(a, dtype=np.int16)
    bb = np.asarray(b, dtype=np.int16)
    delta = np.abs(aa - bb)
    changed = int((delta > threshold).sum())
    total = int(delta.size)
    mean = float(delta.mean())
    return changed, total, mean


def check_rule(rule: str, changed: int):
    if rule == "eq0":
        return changed == 0
    if rule == "gt1000":
        return changed > 1000
    print("::error::未知の rule: %s" % rule)
    sys.exit(1)


def main() -> int:
    ap = argparse.ArgumentParser(description="chargen のタップ反応を画素差分で測る")
    ap.add_argument("--tap", nargs=2, type=int, metavar=("X", "Y"),
                    help="device px のタップ座標")
    ap.add_argument("--name", help="golden のキー（--golden と一緒に使う）")
    ap.add_argument("--serial", default=os.environ.get("ANDROID_SERIAL", "emulator-5554"))
    ap.add_argument("--wait", type=float, default=1.0, help="タップ後の待ち秒（既定 1.0）")
    ap.add_argument("--threshold", type=int, default=8,
                    help="変化とみなす |Δ| の下限（既定 8。無音の静止画面は 0 になる）")
    ap.add_argument("--before", help="タップ前の PNG（adb を使わない比較用）")
    ap.add_argument("--after", help="タップ後の PNG（adb を使わない比較用）")
    ap.add_argument("--golden", default=DEFAULT_GOLDEN, help="chargen golden の JSON")
    ap.add_argument("--update", action="store_true", help="golden を今の測定値で更新する")
    ap.add_argument("--out-dir", default=os.path.join("artifacts", "chargen-probe"),
                    help="スクリーンショットの保存先")
    args = ap.parse_args()

    if args.before and args.after:
        before, after = args.before, args.after
    elif args.tap:
        os.makedirs(args.out_dir, exist_ok=True)
        before = os.path.join(args.out_dir, "before.png")
        after = os.path.join(args.out_dir, "after.png")
        screencap(args.serial, before)
        adb = adb_path()
        subprocess.run([adb, "-s", args.serial, "shell", "input", "tap",
                        str(args.tap[0]), str(args.tap[1])], check=True)
        time.sleep(args.wait)
        screencap(args.serial, after)
    else:
        ap.error("--tap X Y か --before/--after が必要です")

    changed, total, mean = diff_metrics(before, after, args.threshold)
    print("changed=%d/%d ratio=%.4f mean=%.4f threshold=%d"
          % (changed, total, changed / total, mean, args.threshold))

    if not args.name and not args.update:
        return 0

    golden = {}
    if os.path.exists(args.golden):
        with open(args.golden, encoding="utf-8") as fh:
            golden = json.load(fh)
    golden.setdefault("meta", {})
    golden.setdefault("taps", {})

    if args.update:
        if not args.name:
            print("::error::--update には --name が必要です")
            return 1
        golden["taps"][args.name] = {
            "x": args.tap[0] if args.tap else None,
            "y": args.tap[1] if args.tap else None,
            "changed": changed,
            "rule": "eq0" if changed == 0 else "gt1000",
            "measured_with": "threshold=%d" % args.threshold,
        }
        os.makedirs(os.path.dirname(args.golden) or ".", exist_ok=True)
        with open(args.golden, "w", encoding="utf-8") as fh:
            json.dump(golden, fh, ensure_ascii=False, indent=2, sort_keys=True)
            fh.write("\n")
        print("updated golden: %s (%s changed=%d)" % (args.golden, args.name, changed))
        return 0

    entry = golden.get("taps", {}).get(args.name)
    if not entry:
        print("::error::golden に %s がありません（--update で作成）" % args.name)
        return 2
    ok = check_rule(entry["rule"], changed)
    print("%s: rule=%s expected_by_rule -> %s (golden changed=%s)"
          % (args.name, entry["rule"], "PASS" if ok else "FAIL", entry.get("changed")))
    return 0 if ok else 2


if __name__ == "__main__":
    sys.exit(main())
