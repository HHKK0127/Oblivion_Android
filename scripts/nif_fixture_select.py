#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""nif_fixture_select.py — CI 用の型被覆 NIF fixture を決定的に選ぶ。

コーパス（既定 tools/BSAFileExtractor/extracted/meshes）から、全型を被覆する
最小ファイル集合を貪欲法で選ぶ。決定性のため候補はパス昇順、同点はパス順で解決。
毎回同じ manifest が出るので差分が読める。

- markers（meshes 直下 = editor/marker 27 本）は必ず含める。NiWireframeProperty の
  唯一の供給源で、meshes_extracted が落としていた分そのもの。被覆計算には混ぜない。
- 評価関数は「新規型数 / sqrt(バイト数)」。1 ファイル上限・総量上限で打ち切る。
- --add は型被覆の貪欲集合とは別に、**レイアウト被覆用**の固定集合を cover/ に加える
  （衝突形状の署名多様性で選ぶ。同一署名 20 本ではレイアウトも型も増えない）。
  manifest では貪欲分とレイアウト分を別セクションに分ける。

使い方:
  python scripts/nif_fixture_select.py --corpus tools/BSAFileExtractor/extracted/meshes \
      --manifest docs/evidence/fixtures/nif/manifest.txt --copy-to docs/evidence/fixtures/nif \
      --add scripts/data/nif_layout_add.txt
"""
import argparse
import math
import os
import shutil
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "scripts"))
from nif_coverage import block_types  # noqa: E402


def scan(corpus):
    """rel -> (型集合, バイト数)。読めないファイルは飛ばす。"""
    info = {}
    for dp, _dirs, names in os.walk(corpus):
        for name in sorted(names):
            if not name.lower().endswith(".nif"):
                continue
            full = os.path.join(dp, name)
            rel = os.path.relpath(full, corpus).replace("\\", "/")
            try:
                _, ts = block_types(full)
            except Exception:  # noqa: BLE001
                continue
            info[rel] = (set(ts), os.path.getsize(full))
    return info


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--corpus",
                    default=os.path.join("tools", "BSAFileExtractor", "extracted", "meshes"))
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--copy-to", default=None)
    ap.add_argument("--max-file-bytes", type=int, default=300 * 1024)
    ap.add_argument("--limit-bytes", type=int, default=2 * 1024 * 1024)
    ap.add_argument("--supply-map", default=None,
                    help="型 -> 供給ファイルの対応を別ファイルにも書き出す")
    ap.add_argument("--add", default=None, metavar="LISTFILE",
                    help="レイアウト被覆用に固定で加える B 相対パスのリスト（# コメント可）")
    args = ap.parse_args()

    info = scan(args.corpus)
    all_types = set()
    for ts, _ in info.values():
        all_types |= ts
    print("corpus files=%d types=%d" % (len(info), len(all_types)))

    # markers = meshes 直下（相対パスに / が無い）。被覆計算には混ぜない。
    markers = sorted(r for r in info if "/" not in r)
    marker_types = set()
    for r in markers:
        marker_types |= info[r][0]
    print("markers=%d types=%d" % (len(markers), len(marker_types)))

    covered = set(marker_types)
    selected = list(markers)
    total_bytes = sum(info[r][1] for r in markers)

    while True:
        remaining = all_types - covered
        if not remaining:
            break
        best = None
        for rel in sorted(info):  # パス昇順（決定性）
            if rel in selected:
                continue
            ts, sz = info[rel]
            if sz > args.max_file_bytes:
                continue
            gain = len(ts & remaining)
            if gain == 0:
                continue
            score = gain / math.sqrt(sz)
            cand = (score, gain, -sz, rel)  # 同点はパス昇順
            if best is None or cand > best:
                best = cand
        if best is None:
            print("WARN: これ以上被覆できない。残り %d 型: %s"
                  % (len(remaining), sorted(remaining)[:8]))
            break
        rel = best[3]
        if total_bytes + info[rel][1] > args.limit_bytes:
            print("WARN: 総量上限 %d に達した。残り %d 型" % (args.limit_bytes, len(remaining)))
            break
        selected.append(rel)
        covered |= info[rel][0]
        total_bytes += info[rel][1]

    greedy_cover = sorted(r for r in set(selected) if r not in markers)
    markers = sorted(markers)

    # レイアウト被覆用の固定集合。型被覆の貪欲集合とは選定基準が違う（署名多様性）。
    # 型は増えない（増えてもよい）が、同じ型の異なるレイアウトを読めるかを見る。
    # 貪欲が既に選んだ分は layout から外す（manifest の重複表示を防ぐ）。
    layout = []
    if args.add:
        with open(args.add, encoding="utf-8") as f:
            for line in f:
                line = line.split("#", 1)[0].strip()
                if not line:
                    continue
                rel = line.replace("\\", "/")
                if rel not in info:
                    sys.exit("--add のパスがコーパスに無い: %s" % rel)
                if rel not in markers and rel not in greedy_cover:
                    layout.append(rel)
        layout = sorted(set(layout))
    cover = sorted(set(greedy_cover) | set(layout))

    print("selected=%d (markers=%d cover=%d greedy=%d layout=%d) bytes=%d covered=%d/%d"
          % (len(markers) + len(cover), len(markers), len(cover),
             len(greedy_cover), len(layout),
             total_bytes, len(covered), len(all_types)))

    # fixture 内の相対パス（markers/ と cover/ を分ける）
    fixmap = {}
    for r in markers:
        fixmap[r] = "markers/" + r
    for r in cover:
        fixmap[r] = "cover/" + r

    # supply map: 型 -> 供給ファイル。1 本でも消えると型が死ぬ。
    supply = {}
    for r in markers + cover:
        for t in info[r][0]:
            supply.setdefault(t, []).append(fixmap[r])
    for t in supply:
        supply[t] = sorted(supply[t])

    def supply_lines():
        out = ["# supply map（型 -> 供給ファイル。1 本でも消えると型が死ぬ）"]
        for t in sorted(supply):
            files = supply[t]
            mark = "  *唯一の供給元" if len(files) == 1 else ""
            out.append("# %-40s %s%s" % (t, ", ".join(files), mark))
        return out

    os.makedirs(os.path.dirname(os.path.abspath(args.manifest)), exist_ok=True)
    with open(args.manifest, "w", encoding="utf-8", newline="\n") as f:
        f.write("# CI 用 NIF fixture の manifest（nif_fixture_select.py が生成・パス昇順）\n")
        f.write("# markers（meshes 直下・必ず含める / NiWireframeProperty の供給源）:\n")
        for r in markers:
            f.write("markers/%s\n" % r)
        f.write("# cover（型被覆。貪欲法で選んだ最小集合。markers と役割が違う）:\n")
        for r in greedy_cover:
            f.write("cover/%s\n" % r)
        f.write("# cover（レイアウト被覆。衝突形状の署名多様性で選んだ固定集合。--add 由来）:\n")
        for r in layout:
            f.write("cover/%s\n" % r)
        f.write("\n")
        for line in supply_lines():
            f.write(line + "\n")
    print("wrote manifest:", args.manifest)

    if args.supply_map:
        with open(args.supply_map, "w", encoding="utf-8", newline="\n") as f:
            for line in supply_lines():
                f.write(line + "\n")
        print("wrote supply map:", args.supply_map)

    if args.copy_to:
        for r in markers:
            dst = os.path.join(args.copy_to, "markers", r)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(os.path.join(args.corpus, r), dst)
        for r in cover:
            dst = os.path.join(args.copy_to, "cover", r)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(os.path.join(args.corpus, r), dst)
        print("copied to:", args.copy_to)


if __name__ == "__main__":
    sys.exit(main())
