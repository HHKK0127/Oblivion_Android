#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""nif_oracle_select.py — オラクル fixture に足す NIF を軸ごとに決定的に選ぶ。

型被覆（nif_fixture_select.py の貪欲集合）で埋まらない穴を、軸ごとに別の基準で埋める。
軸を混ぜない（floor も軸ごとに持つ: types / files / versions）。除外対象はどの軸でも
「manifest の markers 節＋型被覆節」に固定する（fixture ディレクトリを見ない）ので、
自分が出力した分を足した後に再実行しても同じ結果になる。

軸:
  shape   — 衝突形状 bhk*Shape の署名（集合）が異なる代表を選ぶ。同一署名では
            レイアウトも型も増えない。頻度上位 --count は必ず、残りは --extra-bytes の
            予算内で安いものから。
  version — B にあるが fixture（markers＋型被覆）に無い NIF バージョンの最小代表を選ぶ。
  nobj    — num_objects のスケールを踏む。全体最大 nobj の 1 本は必ず、残りは
            --max-rep-bytes 以下の高 nobj を上位から --count まで。

除外が 0 件なら exit 2（manifest を読めていない疑い。除外が効かないと fixture 内の
ファイルを選んでしまう）。

使い方:
  python scripts/nif_oracle_select.py --axis shape \
      --out scripts/data/nif_oracle_shape.txt --extra-bytes 262144
  python scripts/nif_oracle_select.py --axis version \
      --out scripts/data/nif_oracle_version.txt
  python scripts/nif_oracle_select.py --axis nobj \
      --out scripts/data/nif_oracle_nobj.txt --count 19 --max-rep-bytes 307200
"""
import argparse
import os
import sys
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "scripts"))
from nif_coverage import Reader, block_types  # noqa: E402

DEFAULT_CORPUS = os.path.join("tools", "BSAFileExtractor", "extracted", "meshes")
DEFAULT_MANIFEST = os.path.join("docs", "evidence", "fixtures", "nif", "manifest.txt")


def header(path):
    """(version, num_objects, ntypes, legacy) を返す。nif_core read_header と同じ手順。"""
    with open(path, "rb") as f:
        r = Reader(f.read())
    magic = bytearray()
    while len(magic) < 255:
        b = r.u8()
        if b in (0x0A, 0):
            break
        magic.append(b)
    magic = bytes(magic)
    legacy = b"NetImmerse File Format" in magic
    if not legacy and b"Gamebryo File Format" not in magic:
        raise ValueError("not a NIF")
    ver = r.u32()
    if ver < 0x05000001:
        return ver, r.u32(), 0, legacy
    if legacy:
        nobj = r.u32()
        if ver == 0x0A000102:
            r.u32()
            for _ in range(3):
                r.byte_string()
    else:
        if ver >= 0x14000000:
            r.u8()
        r.u32()
        nobj = r.u32()
        r.u32()
        for _ in range(3):
            r.byte_string()
    ntypes = r.u16()
    return ver, nobj, ntypes, legacy


def type_coverage_paths(manifest):
    """manifest の markers 節＋型被覆節にある B 相対パス（追加被覆節は含めない）。"""
    out = set()
    section = None
    with open(manifest, encoding="utf-8") as f:
        for line in f:
            s = line.rstrip("\n")
            if s.startswith("# markers"):
                section = "markers"
                continue
            if s.startswith("# cover"):
                section = "cover" if "型被覆" in s else "extra"
                continue
            if s.startswith("#"):
                section = None
                continue
            if not s.strip() or section == "extra":
                continue
            if s.startswith("markers/"):
                out.add(s[len("markers/"):])
            elif s.startswith("cover/"):
                out.add(s[len("cover/"):])
    return out


def walk_corpus(corpus, exclude):
    """fixture 外の (rel, version, nobj, size, types) を返す。読めないファイルは飛ばす。"""
    rows = []
    for dp, _dirs, names in os.walk(corpus):
        for nm in sorted(names):
            if not nm.lower().endswith(".nif"):
                continue
            full = os.path.join(dp, nm)
            rel = os.path.relpath(full, corpus).replace(os.sep, "/")
            if rel in exclude:
                continue
            try:
                ver, nobj, _nt, _lg = header(full)
                _v, ts = block_types(full)
            except Exception:  # noqa: BLE001
                continue
            rows.append((rel, ver, nobj, os.path.getsize(full), set(ts)))
    return rows


def select_shape(rows, count, extra_bytes):
    groups = defaultdict(list)   # 署名 -> [(size, rel)]
    for rel, _ver, _nobj, sz, ts in rows:
        if "bhkCollisionObject" not in ts:
            continue
        shapes = tuple(sorted(t for t in ts if t.startswith("bhk") and "Shape" in t))
        groups[shapes].append((sz, rel))
    order = sorted(groups.items(), key=lambda kv: -len(kv[1]))
    picked = []
    for _shapes, lst in order[:count]:
        picked.append(sorted(lst)[0])
    extra = 0
    for _shapes, lst in order[count:]:
        sz, rel = sorted(lst)[0]
        if extra + sz > extra_bytes:
            continue
        picked.append((sz, rel))
        extra += sz
    return [(rel, "shape") for _sz, rel in picked], extra


def select_version(corpus, exclude, rows):
    covered = set()
    for rel in exclude:
        try:
            covered.add(header(os.path.join(corpus, rel))[0])
        except Exception:  # noqa: BLE001
            pass
    by_ver = defaultdict(list)
    for rel, ver, _nobj, sz, _ts in rows:
        by_ver[ver].append((sz, rel))
    picked = []
    for ver in sorted(by_ver):
        if ver in covered:
            continue
        sz, rel = min(by_ver[ver])
        picked.append((ver, sz, rel))
    return picked


def select_nobj(rows, count, max_rep_bytes):
    srt = sorted(rows, key=lambda r: (r[2], r[3], r[0]), reverse=True)  # nobj desc
    picked = []
    seen = set()
    if srt:
        picked.append((srt[0][2], srt[0][3], srt[0][0]))   # 全体最大 nobj は必ず
        seen.add(srt[0][0])
    for rel, _ver, nobj, sz, _ts in srt:
        if len(picked) >= count:
            break
        if rel in seen or sz > max_rep_bytes:
            continue
        picked.append((nobj, sz, rel))
        seen.add(rel)
    return picked


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--axis", required=True, choices=("shape", "version", "nobj"))
    ap.add_argument("--corpus", default=DEFAULT_CORPUS)
    ap.add_argument("--manifest", default=DEFAULT_MANIFEST)
    ap.add_argument("--out", required=True)
    ap.add_argument("--count", type=int, default=20,
                    help="shape: 頻度上位この数は必ず / nobj: 最大を含めてこの数まで")
    ap.add_argument("--extra-bytes", type=int, default=0, metavar="B",
                    help="shape: 残りを足す総バイト予算")
    ap.add_argument("--max-rep-bytes", type=int, default=300 * 1024, metavar="B",
                    help="nobj: 最大以外の代表に許すサイズ上限")
    args = ap.parse_args()

    exclude = type_coverage_paths(args.manifest)
    if not exclude:
        print("除外対象（markers＋型被覆）が 0 件。manifest を読めていない疑い。"
              "このままだと fixture 内のファイルを選んでしまうので中止する。",
              file=sys.stderr)
        return 2
    rows = walk_corpus(args.corpus, exclude)
    print("axis=%s 除外 %d 件（markers＋型被覆）・残り %d 件"
          % (args.axis, len(exclude), len(rows)))

    if args.axis == "shape":
        picked, extra = select_shape(rows, args.count, args.extra_bytes)
        rels = [r for r, _t in picked]
        print("  形状: %d 本（上位 %d ＋ 予算 %d B で %d 本 / 追加 %d B）"
              % (len(rels), args.count, args.extra_bytes, len(rels) - args.count, extra))
        header_note = [
            "# 形状軸: 衝突形状 bhk*Shape の署名が異なる代表。頻度上位 %d は必ず、" % args.count,
            "# 残りは追加 %d B の予算内で安いものから。" % args.extra_bytes,
        ]
    elif args.axis == "version":
        ver_pick = select_version(args.corpus, exclude, rows)
        rels = [r for _v, _s, r in ver_pick]
        for ver, sz, rel in ver_pick:
            print("  バージョン 0x%08X: %7dB %s" % (ver, sz, rel))
        header_note = [
            "# バージョン軸: fixture（markers＋型被覆）に無い NIF バージョンの最小代表。",
        ]
    else:  # nobj
        nobj_pick = select_nobj(rows, args.count, args.max_rep_bytes)
        rels = [r for _n, _s, r in nobj_pick]
        print("  nobj: %d 本（最大 1 本 ＋ %d B 以下の上位 %d 本）"
              % (len(rels), args.max_rep_bytes, len(rels) - 1))
        for nobj, sz, rel in nobj_pick:
            print("    nobj=%-6d %9dB  %s" % (nobj, sz, rel))
        header_note = [
            "# nobj 軸: num_objects のスケールを踏む。全体最大 nobj の 1 本は必ず、",
            "# 残りは %d B 以下の高 nobj を上位から %d 本まで。" % (args.max_rep_bytes, args.count),
        ]

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# オラクル fixture の追加 NIF（B 相対パス）。nif_fixture_select.py --add で固定投入する。\n")
        for line in header_note:
            f.write(line + "\n")
        f.write("# このファイルは scripts/nif_oracle_select.py が生成する（手で編集しない）。\n")
        for rel in rels:
            f.write(rel + "\n")
    print("  wrote %s (%d 本)" % (args.out, len(rels)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
