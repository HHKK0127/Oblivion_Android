#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""compare_golden.py — 画面のゴールデン比較（数値で落とす）。

目的:
  「描画順・UV・ブレンド・文字寸法の誤り」はスクリーンショットの目視では見つからない。
  原版（または承認済みの基準画面）との差を数値にして、閾値超過で CI を落とす。

方式（なぜ生の PNG をコミットしないか）:
  スクリーンショットには Bethesda のアセット（テクスチャ）が写る。このリポジトリは
  それらを再配布しない方針（.gitignore の Bethesda アセット節）。そこでゴールデンは
  **縮小した輝度グリッド（32x18 の整数）を JSON で保存**する。これは派生物であり
  元アートの再配布にならない。比較はこのグリッドで行う。

来歴（meta）:
  画素差と「環境差」を混同しないため、golden には撮影条件を埋める。PNG 自体は
  アセットなので未コミットであり、JSON から実行を辿れるようにしておく必要がある。

    {"meta": {"taken_at", "commit", "app_version",
              "wm_size", "density", "rotation", "source_png_sha256"},
     "grid_w": 32, "grid_h": 18, "cells": [...]}

  `--require` は wm_size/density/rotation が現環境と違えば **環境不一致** として、
  画素差とは別のメッセージで落とす（密度が変わっただけの赤を延々と追わないため）。

使い方:
  # 基準を承認（人がレビューしてからコミットする）
  python scripts/compare_golden.py artifacts/smoke-newgame.png --update

  # 比較（CI）
  python scripts/compare_golden.py artifacts/smoke-newgame.png --require \
      --diff artifacts/smoke-newgame.diff.png

  # ホスト smoke の JSON（tools/host_tests/newgame_smoke）は完全一致で比較する
  python scripts/compare_golden.py out.json --require \
      --golden docs/evidence/golden/newgame-no-esm.json
  python scripts/compare_golden.py out.json --update   # 採取

終了コード:
  0 = 一致（または --update 成功） / 1 = 閾値超過・環境不一致、または --require で基準が無い
"""
import argparse
import datetime
import hashlib
import json
import os
import re
import subprocess
import sys

GRID_W, GRID_H = 32, 18
DEFAULT_GOLDEN = os.path.join("docs", "evidence", "golden", "newgame-no-esm-prefix.json")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# smoke スクリプトが書く表示条件のサイドカー（比較時に環境一致を見るため）。
ENV_SIDECAR = os.path.join("artifacts", "smoke-newgame.env")


def git_short_commit():
    try:
        r = subprocess.run(["git", "-C", REPO, "rev-parse", "--short", "HEAD"],
                           capture_output=True, text=True, encoding="utf-8")
        return r.stdout.strip() or "unknown"
    except OSError:
        return "unknown"


def app_version():
    try:
        with open(os.path.join(REPO, "app", "build.gradle"), encoding="utf-8") as fh:
            m = re.search(r"versionName\s+'([^']+)'", fh.read())
        return m.group(1) if m else "unknown"
    except OSError:
        return "unknown"


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_sidecar(path):
    """smoke が書く `key=value` を読む。無ければ空。"""
    out = {}
    if not os.path.exists(path):
        return out
    try:
        with open(path, encoding="utf-8") as fh:
            for line in fh:
                if "=" in line and not line.startswith("#"):
                    k, v = line.strip().split("=", 1)
                    out[k.strip()] = v.strip()
    except OSError:
        pass
    return out


def load_gray(path):
    try:
        from PIL import Image
    except ImportError:
        print("::error::Pillow が必要です（python -m pip install pillow numpy）")
        sys.exit(2)
    if not os.path.exists(path):
        print("::error::候補画像がありません: %s" % path)
        sys.exit(2)
    return Image.open(path).convert("L").resize((GRID_W, GRID_H), Image.BOX)


def digest(im):
    import numpy as np
    a = np.asarray(im, dtype=np.float32).reshape(-1)
    return {"grid_w": GRID_W, "grid_h": GRID_H,
            "cells": [int(round(float(v))) for v in a]}


def load_json(path):
    with open(path, encoding="utf-8") as fh:
        return json.load(fh)


def json_differences(golden, cand, path="$"):
    """golden と cand の最初の相違点を列挙する（JSON 完全一致モード用）。"""
    out = []
    if type(golden) is not type(cand):
        out.append("%s: 型 %s != %s" % (path, type(golden).__name__, type(cand).__name__))
        return out
    if isinstance(golden, dict):
        for k in sorted(set(golden) | set(cand)):
            if k not in golden:
                out.append("%s.%s: candidate のみ" % (path, k))
            elif k not in cand:
                out.append("%s.%s: golden のみ" % (path, k))
            else:
                out += json_differences(golden[k], cand[k], "%s.%s" % (path, k))
    elif isinstance(golden, list):
        if len(golden) != len(cand):
            out.append("%s: 要素数 %d != %d" % (path, len(golden), len(cand)))
        for i, (g, c) in enumerate(zip(golden, cand)):
            out += json_differences(g, c, "%s[%d]" % (path, i))
    elif golden != cand:
        out.append("%s: %r != %r" % (path, golden, cand))
    return out


def compare_json(args):
    """ホスト smoke の JSON 出力（newgame_smoke）の完全一致比較。

    画面（PNG）のグリッド比較とは別系統。meta の環境チェックは撮影条件の話なので
    JSON 状態比較には適用しない。
    """
    if not os.path.exists(args.candidate):
        print("::error::候補 JSON がありません: %s" % args.candidate)
        return 2
    try:
        cand = load_json(args.candidate)
    except (json.JSONDecodeError, ValueError) as e:
        print("::error::候補 JSON を解析できません（%s）: %s" % (args.candidate, e))
        return 2
    if args.update:
        os.makedirs(os.path.dirname(args.golden) or ".", exist_ok=True)
        with open(args.golden, "w", encoding="utf-8") as fh:
            json.dump(cand, fh, ensure_ascii=False, indent=1)
            fh.write("\n")
        print("updated golden: %s" % args.golden)
        print("→ 内容をレビューしてコミットすること（これが以後の基準になる）。")
        return 0

    if not os.path.exists(args.golden):
        msg = ("ゴールデンがありません: %s。"
               "ホスト smoke を 1 度回し、"
               "`python scripts/compare_golden.py <out.json> --update` で作成してコミットすること。"
               % args.golden)
        if args.require:
            print("::error::" + msg)
            return 1
        print("::warning::" + msg)
        return 0

    golden = load_json(args.golden)
    if golden == cand:
        print("OK: JSON ゴールデンと一致")
        return 0

    diffs = json_differences(golden, cand)
    print("::error::JSON ゴールデンと相違（%d 点）" % len(diffs))
    for d in diffs[:20]:
        print("  " + d)
    if len(diffs) > 20:
        print("  ... 他 %d 点" % (len(diffs) - 20))
    return 1


def write_diff(golden_cells, cand_im, out_path):
    import numpy as np
    from PIL import Image
    g = np.asarray(golden_cells, dtype=np.float32).reshape(GRID_H, GRID_W)
    c = np.asarray(cand_im, dtype=np.float32).reshape(GRID_H, GRID_W)
    d = np.abs(g - c)
    heat = np.clip(d * 8.0, 0, 255).astype(np.uint8)
    Image.fromarray(heat, mode="L").resize((GRID_W * 16, GRID_H * 16), Image.NEAREST).save(out_path)


def effective_env(args):
    """現在の撮影条件を決める（優先: 引数 > サイドカー > unknown）。"""
    side = read_sidecar(os.path.join(REPO, ENV_SIDECAR))
    return {
        "wm_size": args.wm_size or side.get("wm_size") or "unknown",
        "density": args.density or side.get("density") or "unknown",
        "rotation": args.rotation or side.get("rotation") or "unknown",
    }


def main():
    ap = argparse.ArgumentParser(description="画面のゴールデン比較")
    ap.add_argument("candidate",
                    help="比較する候補: 画面スモークの PNG、またはホスト smoke の out.json")
    ap.add_argument("--golden", default=DEFAULT_GOLDEN, help="基準 JSON（既定 %s）" % DEFAULT_GOLDEN)
    ap.add_argument("--update", action="store_true", help="候補を基準として書き出す（人がレビューしてからコミット）")
    ap.add_argument("--require", action="store_true", help="基準が無ければ失敗（既定は警告のみ）")
    ap.add_argument("--max-mean", type=float, default=4.0, help="セル平均差の上限（既定 4.0/255）")
    ap.add_argument("--cell-threshold", type=float, default=12.0, help="「変化した」と数えるセル差（既定 12.0）")
    ap.add_argument("--max-changed-ratio", type=float, default=0.05, help="変化セル率の上限（既定 0.05）")
    ap.add_argument("--diff", default=None, help="差分ヒートマップの出力先")
    ap.add_argument("--wm-size", default=None, help="現環境の表示サイズ（例 1280x960）")
    ap.add_argument("--density", default=None, help="現環境の密度（例 240）")
    ap.add_argument("--rotation", default=None, help="現環境の回転（例 1）")
    ap.add_argument("--allow-env-mismatch", action="store_true",
                    help="環境不一致でも画素比較へ進む（既定は環境不一致で落とす）")
    args = ap.parse_args()

    # 候補が JSON ならホスト smoke の状態比較（PNG は従来のグリッド比較）。
    if args.candidate.lower().endswith(".json"):
        return compare_json(args)

    im = load_gray(args.candidate)
    cand = digest(im)
    env = effective_env(args)

    if args.update:
        cand["meta"] = {
            "taken_at": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
            "commit": git_short_commit(),
            "app_version": app_version(),
            "wm_size": env["wm_size"],
            "density": env["density"],
            "rotation": env["rotation"],
            "source_png_sha256": sha256_of(args.candidate),
        }
        os.makedirs(os.path.dirname(args.golden) or ".", exist_ok=True)
        with open(args.golden, "w", encoding="utf-8") as fh:
            json.dump(cand, fh, ensure_ascii=False, indent=1)
            fh.write("\n")
        print("updated golden: %s" % args.golden)
        print("  meta: %s" % json.dumps(cand["meta"], ensure_ascii=False))
        print("→ 内容をレビューしてコミットすること（これが以後の基準になる）。")
        return 0

    if not os.path.exists(args.golden):
        msg = ("ゴールデンがありません: %s。"
               "エミュレータで 1 度スモークを回し、"
               "`python scripts/compare_golden.py <screenshot> --update` で作成してコミットすること。"
               % args.golden)
        if args.require:
            print("::error::" + msg)
            return 1
        print("::warning::" + msg)
        return 0

    with open(args.golden, encoding="utf-8") as fh:
        golden = json.load(fh)

    # 来歴による環境チェック（画素差と混同しない）。
    gmeta = golden.get("meta") or {}
    if gmeta:
        mismatches = []
        for key in ("wm_size", "density", "rotation"):
            gv, cv = gmeta.get(key, "unknown"), env.get(key, "unknown")
            if gv != "unknown" and cv != "unknown" and gv != cv:
                mismatches.append("%s: golden=%s current=%s" % (key, gv, cv))
        print("golden meta: taken_at=%s commit=%s app_version=%s wm=%s dpi=%s rot=%s"
              % (gmeta.get("taken_at", "unknown"), gmeta.get("commit", "unknown"),
                 gmeta.get("app_version", "unknown"), gmeta.get("wm_size", "unknown"),
                 gmeta.get("density", "unknown"), gmeta.get("rotation", "unknown")))
        if mismatches and not args.allow_env_mismatch:
            print("::error::環境不一致（画素差ではなく撮影条件の違い）: " + " / ".join(mismatches))
            print("  → 同じ表示条件で撮り直すか、その条件が正しいなら --update で更新する。")
            return 1

    if (cand["grid_w"], cand["grid_h"]) != (golden.get("grid_w"), golden.get("grid_h")):
        print("::error::グリッド形状が不一致: golden=%sx%s candidate=%sx%s"
              % (golden.get("grid_w"), golden.get("grid_h"), cand["grid_w"], cand["grid_h"]))
        return 1

    import numpy as np
    g = np.array(golden["cells"], dtype=np.float32)
    c = np.array(cand["cells"], dtype=np.float32)
    if g.shape != c.shape:
        print("::error::セル数が不一致: golden=%s candidate=%s" % (g.shape, c.shape))
        return 1

    d = np.abs(g - c)
    mean = float(d.mean())
    changed = int((d > args.cell_threshold).sum())
    ratio = changed / float(d.size)

    if args.diff:
        try:
            write_diff(golden["cells"], im, args.diff)
            print("diff heatmap: %s" % args.diff)
        except Exception as e:  # noqa: BLE001 - 失敗しても比較結果は返す
            print("::warning::差分画像を書けませんでした: %s" % e)

    print("golden mean=%.2f changed=%d/%d (%.1f%%)  [limits mean<=%.2f ratio<=%.2f]"
          % (mean, changed, d.size, ratio * 100.0, args.max_mean, args.max_changed_ratio))

    if mean > args.max_mean or ratio > args.max_changed_ratio:
        print("::error::ゴールデンと差があります（mean=%.2f, changed=%.1f%%）" % (mean, ratio * 100.0))
        print("  意図した変更なら: python scripts/compare_golden.py %s --update" % args.candidate)
        return 1
    print("OK: ゴールデンと一致")
    return 0


if __name__ == "__main__":
    sys.exit(main())
