#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""nif_layout_select.py — レイアウト被覆用の追加 NIF を決定的に選ぶ。

型被覆（nif_fixture_select.py の貪欲集合）とは選定基準が違う。同じ型でも**異なる
レイアウト**を読めるかを見るため、衝突形状の署名が異なる代表を 1 つずつ選ぶ。

署名 = そのファイルが含む bhk*Shape 型の集合（bhkBoxShape 等）。同一署名の 20 本では
レイアウトも型も増えないので、署名が異なるものを優先する。決定的（署名は含むファイル
数の多い順、各署名から最小サイズ・パス昇順の代表）。

除外対象は「manifest.txt の markers 節＋型被覆節」に固定する（fixture ディレクトリを
見ない）。これで、自分が出力したレイアウト分を足した後に再実行しても同じ 20 本になる。

使い方:
  python scripts/nif_layout_select.py \
      --manifest docs/evidence/fixtures/nif/manifest.txt \
      --out scripts/data/nif_layout_add.txt
"""
import argparse
import os
import sys
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "scripts"))
from nif_coverage import block_types  # noqa: E402

DEFAULT_CORPUS = os.path.join("tools", "BSAFileExtractor", "extracted", "meshes")


def type_coverage_paths(manifest):
    """manifest の markers 節＋型被覆節にある B 相対パス（レイアウト節は含めない）。"""
    out = set()
    section = None
    with open(manifest, encoding="utf-8") as f:
        for line in f:
            s = line.rstrip("\n")
            if s.startswith("# markers"):
                section = "markers"
                continue
            if s.startswith("# cover"):
                section = "layout" if "レイアウト" in s else "cover"
                continue
            if s.startswith("#") or not s.strip():
                if s.startswith("#") and "supply" in s:
                    section = None
                continue
            if section == "layout":
                continue
            if s.startswith("markers/"):
                out.add(s[len("markers/"):])
            elif s.startswith("cover/"):
                out.add(s[len("cover/"):])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--corpus", default=DEFAULT_CORPUS)
    ap.add_argument("--manifest", default=os.path.join(
        "docs", "evidence", "fixtures", "nif", "manifest.txt"),
        help="除外対象（markers＋型被覆）を読む manifest")
    ap.add_argument("--out", default=os.path.join("scripts", "data", "nif_layout_add.txt"))
    ap.add_argument("--count", type=int, default=20)
    args = ap.parse_args()

    exclude = type_coverage_paths(args.manifest)
    groups = defaultdict(list)   # 署名 -> [(size, rel)]
    bhk_files = 0
    for dp, _dirs, names in os.walk(args.corpus):
        for nm in sorted(names):
            if not nm.lower().endswith(".nif"):
                continue
            full = os.path.join(dp, nm)
            rel = os.path.relpath(full, args.corpus).replace("\\", "/")
            if rel in exclude:
                continue
            try:
                _, ts = block_types(full)
            except Exception:  # noqa: BLE001
                continue
            if "bhkCollisionObject" not in ts:
                continue
            bhk_files += 1
            shapes = tuple(sorted(t for t in ts if t.startswith("bhk") and "Shape" in t))
            groups[shapes].append((os.path.getsize(full), rel))

    print("bhkCollisionObject を含むファイル（除外後）:", bhk_files)
    print("衝突形状の署名（異なり）:", len(groups))

    order = sorted(groups.items(), key=lambda kv: -len(kv[1]))
    picked = []
    for shapes, rows in order:
        cands = sorted(rows)          # (size, rel) 昇順 → 同サイズはパス昇順
        if not cands:
            continue
        picked.append((shapes, cands[0]))
        if len(picked) >= args.count:
            break

    print("\n形状別の代表 %d 本（署名 / ファイル）:" % args.count)
    for i, (shapes, (sz, rel)) in enumerate(picked, 1):
        print("  %2d. %-6d B  %-58s %s"
              % (i, sz, rel, ",".join(s[3:] for s in shapes)))

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# レイアウト被覆用の追加 NIF（B 相対パス）。nif_fixture_select.py --add で固定投入する。\n")
        f.write("#\n")
        f.write("# cover/ の型被覆用 greedy 集合とは選定基準が違う: こちらは「衝突形状の署名が異なる」\n")
        f.write("# 代表を 1 つずつ選ぶ（同一署名 20 本ではレイアウトも型も増えない）。型は増えない\n")
        f.write("# （衝突系の型は既に corpus 119 に居ることを実測済み）。\n")
        f.write("# このファイルは scripts/nif_layout_select.py が生成する（手で編集しない）。\n")
        for _shapes, (_sz, rel) in picked:
            f.write(rel + "\n")
    print("wrote", args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
