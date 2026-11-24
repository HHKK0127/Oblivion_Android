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

  この方式で検出できるもの: 文字寸法の倍率違い、下降字の欠落、要素の欠落・移動、
  背景の入れ替わり、全体的な色調のずれ。検出できないもの: 数 px の細部、UV の微小ずれ。

使い方:
  # 基準を承認（人がレビューしてからコミットする）
  python scripts/compare_golden.py artifacts/smoke-newgame.png --update

  # 比較（CI）
  python scripts/compare_golden.py artifacts/smoke-newgame.png --require \
      --diff artifacts/smoke-newgame.diff.png

終了コード:
  0 = 一致（または --update 成功） / 1 = 閾値超過、または --require で基準が無い
"""
import argparse
import json
import os
import sys

GRID_W, GRID_H = 32, 18
# golden #1 は ESM 不在前提（CI の smoke は実データを積まない）。
# 実データを供給したら正当な差として赤くなるので、その時点で再採取する。
DEFAULT_GOLDEN = os.path.join("docs", "evidence", "golden", "newgame-no-esm.json")


def load_gray(path):
    try:
        from PIL import Image
    except ImportError:
        print("::error::Pillow が必要です（python -m pip install pillow numpy）")
        sys.exit(2)
    if not os.path.exists(path):
        print("::error::候補画像がありません: %s" % path)
        sys.exit(2)
    im = Image.open(path).convert("L").resize((GRID_W, GRID_H), Image.BOX)
    return im


def digest(im):
    import numpy as np
    a = np.asarray(im, dtype=np.float32).reshape(-1)
    return {"grid_w": GRID_W, "grid_h": GRID_H,
            "cells": [int(round(float(v))) for v in a]}


def write_diff(golden_cells, cand_im, out_path):
    import numpy as np
    from PIL import Image
    g = np.asarray(golden_cells, dtype=np.float32).reshape(GRID_H, GRID_W)
    c = np.asarray(cand_im, dtype=np.float32).reshape(GRID_H, GRID_W)
    d = np.abs(g - c)
    # 差を 0..255 に増幅して可視化（8 倍）。
    heat = np.clip(d * 8.0, 0, 255).astype(np.uint8)
    Image.fromarray(heat, mode="L").resize((GRID_W * 16, GRID_H * 16), Image.NEAREST).save(out_path)


def main():
    ap = argparse.ArgumentParser(description="画面のゴールデン比較")
    ap.add_argument("candidate", help="比較する PNG（スモークのスクリーンショット）")
    ap.add_argument("--golden", default=DEFAULT_GOLDEN, help="基準 JSON（既定 %s）" % DEFAULT_GOLDEN)
    ap.add_argument("--update", action="store_true", help="候補を基準として書き出す（人がレビューしてからコミット）")
    ap.add_argument("--require", action="store_true", help="基準が無ければ失敗（既定は警告のみ）")
    ap.add_argument("--max-mean", type=float, default=4.0, help="セル平均差の上限（既定 4.0/255）")
    ap.add_argument("--cell-threshold", type=float, default=12.0, help="「変化した」と数えるセル差（既定 12.0）")
    ap.add_argument("--max-changed-ratio", type=float, default=0.05, help="変化セル率の上限（既定 0.05）")
    ap.add_argument("--diff", default=None, help="差分ヒートマップの出力先")
    args = ap.parse_args()

    im = load_gray(args.candidate)
    cand = digest(im)

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

    import numpy as np
    g = np.array(golden["cells"], dtype=np.float32)
    c = np.array(cand["cells"], dtype=np.float32)
    if g.shape != c.shape:
        print("::error::グリッド形状が不一致: golden=%s candidate=%s" % (g.shape, c.shape))
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
