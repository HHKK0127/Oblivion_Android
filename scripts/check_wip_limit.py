#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_wip_limit.py — 未マージブランチの WIP 上限チェック（滞留の可視化）。

背景:
  エージェントは書くのが速いので「書くコスト」はほぼゼロに見える。そのため
  同じ機能を 2 回書くことも、ブランチを滞留させたまま次を始めることも「タダ」
  に見える。しかし本当に高い資源は **人間の注意とマージ帯域** で、そこは 1 本の
  まま。結果、書く速度にマージと検証が追いつかず滞留が生まれ、滞留したまま
  次の作業が始まる（chargen の二重実装はこの典型）。

  そこで滞留を機械的に数え、上限を超えたら警告する。

検査:
  1. 未マージブランチ数（base にマージされていないブランチ）
  2. --overlap: 未マージブランチ同士が同じファイルを変更している組（二重実装の検出）
  3. docs/claims.md があれば、宣言された占有（lane → paths）を表示

終了コード:
  0 = OK、または警告のみ（既定）
  1 = 上限超過かつ --strict

使い方:
  python scripts/check_wip_limit.py
  python scripts/check_wip_limit.py --strict --limit 3
  python scripts/check_wip_limit.py --base origin/master --overlap
"""
import argparse
import os
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_LIMIT = 3
DEFAULT_BASE_CANDIDATES = ("master", "main", "origin/master", "origin/main")


def git(*args):
    """git を実行して stdout を返す。失敗時は空文字。"""
    try:
        r = subprocess.run(
            ["git", "-C", REPO] + list(args),
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
    except OSError:
        return ""
    if r.returncode != 0:
        return ""
    return r.stdout.strip()


def resolve_base(explicit):
    if explicit:
        return explicit
    for cand in DEFAULT_BASE_CANDIDATES:
        if git("rev-parse", "--verify", "--quiet", cand + "^{commit}"):
            return cand
    return "HEAD"


def list_branches():
    """ローカル＋リモートのブランチ名を重複なく返す。"""
    names = []
    for pattern, skip in (("refs/heads", ()), ("refs/remotes/origin", ("HEAD",))):
        out = git("for-each-ref", "--format=%(refname:short)", pattern)
        for line in out.splitlines():
            name = line.strip()
            if not name:
                continue
            short = name[len("origin/"):] if name.startswith("origin/") else name
            if short in skip or short.endswith("/HEAD"):
                continue
            names.append(name)
    # ローカルを優先して重複を除く。
    seen = set()
    ordered = []
    for name in names:
        short = name[len("origin/"):] if name.startswith("origin/") else name
        if short in seen:
            continue
        seen.add(short)
        ordered.append(name)
    return ordered


def is_merged(ref, base):
    r = subprocess.run(
        ["git", "-C", REPO, "merge-base", "--is-ancestor", ref, base],
        capture_output=True,
    )
    return r.returncode == 0


def ahead_count(ref, base):
    out = git("rev-list", "--count", "%s..%s" % (base, ref))
    try:
        return int(out)
    except ValueError:
        return -1


def changed_files(ref, base):
    out = git("diff", "--name-only", "%s...%s" % (base, ref))
    return set(l.strip() for l in out.splitlines() if l.strip())


def read_claims():
    path = os.path.join(REPO, "docs", "claims.md")
    if not os.path.exists(path):
        return None
    lines = []
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            line = line.rstrip("\n")
            if line.startswith("|") and "---" not in line:
                lines.append(line)
    return lines


def main():
    ap = argparse.ArgumentParser(description="未マージブランチの WIP 上限チェック")
    ap.add_argument("--base", default=None, help="基準ブランチ（既定: master/main を自動判定）")
    ap.add_argument("--limit", type=int, default=DEFAULT_LIMIT,
                    help="未マージブランチの上限（既定 %d）" % DEFAULT_LIMIT)
    ap.add_argument("--overlap", action="store_true",
                    help="未マージブランチ同士のファイル重複（二重実装）を検出")
    ap.add_argument("--strict", action="store_true",
                    help="上限超過時に終了コード 1 を返す（既定は警告のみ）")
    args = ap.parse_args()

    base = resolve_base(args.base)
    if not git("rev-parse", "--verify", "--quiet", base + "^{commit}"):
        print("check_wip_limit: base '%s' を解決できません" % base)
        return 0

    unmerged = []
    for ref in list_branches():
        if ref == base or ref == base.replace("origin/", ""):
            continue
        if is_merged(ref, base):
            continue
        unmerged.append((ref, ahead_count(ref, base)))

    unmerged.sort(key=lambda kv: (-kv[1], kv[0]))

    print("check_wip_limit: base=%s / 未マージブランチ=%d / 上限=%d"
          % (base, len(unmerged), args.limit))
    for ref, n in unmerged:
        print("  - %-45s %s commits ahead" % (ref, n if n >= 0 else "?"))

    claims = read_claims()
    if claims:
        print("\ndocs/claims.md（占有の宣言）:")
        for line in claims:
            print("  " + line)

    over = len(unmerged) > args.limit

    if args.overlap and unmerged:
        refs = [r for r, _ in unmerged]
        files = {}
        for ref in refs:
            fs = changed_files(ref, base)
            if 0 < len(fs) <= 200:
                files[ref] = fs
        pairs = []
        ordered = sorted(files)
        for i in range(len(ordered)):
            for j in range(i + 1, len(ordered)):
                shared = files[ordered[i]] & files[ordered[j]]
                if shared:
                    pairs.append((ordered[i], ordered[j], sorted(shared)))
        if pairs:
            print("\n重複（同じファイルを変更している未マージブランチ）:")
            for a, b, shared in pairs[:10]:
                shown = ", ".join(shared[:4]) + (" …" if len(shared) > 4 else "")
                print("  ! %s <-> %s : %s" % (a, b, shown))
            if len(pairs) > 10:
                print("  … 他 %d 組" % (len(pairs) - 10))
            print("  → 同じ機能の二重実装の疑い。docs/claims.md の占有宣言と統合順を確認すること。")

    if over:
        msg = ("WIP 上限超過: 未マージ %d 本（上限 %d）。新しい作業を始める前に"
               "マージするか、閉じるブランチを決めること。" % (len(unmerged), args.limit))
        print("\n" + msg)
        # GitHub Actions のアノテーション（あれば PR 画面に出る）。
        print("::warning::" + msg)
        if args.strict:
            return 1
    else:
        print("\nOK: 未マージ %d 本（上限 %d 以内）" % (len(unmerged), args.limit))
    return 0


if __name__ == "__main__":
    sys.exit(main())
