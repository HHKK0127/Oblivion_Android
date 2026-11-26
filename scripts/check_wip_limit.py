#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_wip_limit.py — 未マージブランチの WIP と滞留の可視化。

背景:
  エージェントは書くのが速いので「書くコスト」はほぼゼロに見える。そのため
  同じ機能を 2 回書くことも、ブランチを滞留させたまま次を始めることも「タダ」
  に見える。しかし本当に高い資源は **人間の注意とマージ帯域** で、そこは 1 本の
  まま。結果、書く速度にマージと検証が追いつかず滞留が生まれ、滞留したまま
  次の作業が始まる（chargen の二重実装はこの典型）。

  そこで未マージブランチを **最終コミット日** で active / 中間 / stale に分ける。
  33 本を一律に数えると「多すぎる」しか言えないが、分けると「どれが生きていて
  どれが死んでいるか」が毎回出る。滞留一覧はそのまま棚卸しキューになる。

検査:
  1. 未マージブランチを最終コミット日で分類:
     - active（<= --active-days 日。既定 7）: 生きている WIP。上限判定はこれのみ。
     - 中間（active と stale の間）: 参考表示。
     - stale（> --stale-days 日。既定 30）: 滞留。件数と一覧を常に出す（黙らせない）。
  2. --overlap: 未マージブランチ同士が同じファイルを変更している組（二重実装の検出）
  3. docs/claims.md があれば、宣言された占有（lane → paths）を表示
  4. open PR 数（gh が使えれば併記のみ。判定には使わない）

判定は active のみの二段しきい値:
  warn  = --limit（既定 5）: 超えたら警告。既定は exit 0（--strict で exit 1）。
  block = --block-limit（既定 10）: 超えたら exit 1（作業を止める）。
  現状 active 9 は warn 帯なので通り、10 本目で止まる。active が 5 以下に落ちたら
  block を 5 へ下げる（ラチェット）。

終了コード:
  0 = OK、または warn のみ
  1 = active が block 超過（常時）、または warn 超過かつ --strict

使い方:
  python scripts/check_wip_limit.py
  python scripts/check_wip_limit.py --base origin/master --overlap
  python scripts/check_wip_limit.py --active-days 7 --stale-days 30 --limit 5 --block-limit 10

注意:
  ローカル専用ブランチ（origin 未 push）はローカル実行でしか見えない。CI の
  fresh clone は origin の枝しか数えないため、CI の active はローカルのそれより
  小さくなる。CI は「push 済みの滞留」の検出器であり、未 push の WIP は
  ローカル実行で見る。
"""
import argparse
import json
import os
import subprocess
import sys
from datetime import datetime, timezone

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_LIMIT = 5
DEFAULT_BLOCK_LIMIT = 10
DEFAULT_ACTIVE_DAYS = 7
DEFAULT_STALE_DAYS = 30
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


def commit_age_days(ref):
    """最終コミットからの経過日数。取得できなければ None。"""
    iso = git("log", "-1", "--format=%cI", ref)
    if not iso:
        return None
    try:
        dt = datetime.fromisoformat(iso)
    except ValueError:
        return None
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    return (datetime.now(timezone.utc) - dt).days


def open_pr_count():
    """open PR 数。gh が使えなければ None（併記のみ・判定には使わない）。"""
    try:
        r = subprocess.run(
            ["gh", "pr", "list", "--state", "open", "--json", "number"],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
            timeout=20,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    if r.returncode != 0:
        return None
    try:
        return len(json.loads(r.stdout or "[]"))
    except ValueError:
        return None


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


def show_section(title, rows):
    if not rows:
        return
    print("\n%s:" % title)
    for ref, n, age in rows:
        age_s = "%3dd" % age if age is not None else "  ?"
        print("  - %-45s %s  %s commits ahead" % (ref, age_s, n if n >= 0 else "?"))


def main():
    ap = argparse.ArgumentParser(description="未マージブランチの WIP / 滞留チェック")
    ap.add_argument("--base", default=None, help="基準ブランチ（既定: master/main を自動判定）")
    ap.add_argument("--limit", type=int, default=DEFAULT_LIMIT,
                    help="active（直近）ブランチの warn 上限（既定 %d）" % DEFAULT_LIMIT)
    ap.add_argument("--block-limit", type=int, default=DEFAULT_BLOCK_LIMIT,
                    help="active の block 上限（超過で exit 1。既定 %d）" % DEFAULT_BLOCK_LIMIT)
    ap.add_argument("--active-days", type=int, default=DEFAULT_ACTIVE_DAYS,
                    help="active とみなす最終コミットからの日数（既定 %d）" % DEFAULT_ACTIVE_DAYS)
    ap.add_argument("--stale-days", type=int, default=DEFAULT_STALE_DAYS,
                    help="stale（滞留）とみなす日数（既定 %d）" % DEFAULT_STALE_DAYS)
    ap.add_argument("--overlap", action="store_true",
                    help="未マージブランチ同士のファイル重複（二重実装）を検出")
    ap.add_argument("--strict", action="store_true",
                    help="active が上限超過のとき終了コード 1 を返す（既定は警告のみ）")
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
        unmerged.append((ref, ahead_count(ref, base), commit_age_days(ref)))

    active = sorted([x for x in unmerged if x[2] is not None and x[2] <= args.active_days],
                    key=lambda kv: (kv[2], kv[0]))
    middle = sorted([x for x in unmerged if x[2] is not None
                     and args.active_days < x[2] <= args.stale_days],
                    key=lambda kv: (kv[2], kv[0]))
    stale = sorted([x for x in unmerged if x[2] is not None and x[2] > args.stale_days],
                   key=lambda kv: (-kv[2], kv[0]))
    unknown = sorted([x for x in unmerged if x[2] is None], key=lambda kv: kv[0])

    print("check_wip_limit: base=%s / 未マージ=%d / active(<=%dd)=%d / 中間=%d / "
          "stale(>%dd)=%d / warn=%d block=%d"
          % (base, len(unmerged), args.active_days, len(active),
             len(middle), args.stale_days, len(stale), args.limit, args.block_limit))

    show_section("active（生きている WIP）", active)
    show_section("中間", middle)
    show_section("stale（滞留・棚卸しキュー）", stale)
    show_section("日付不明", unknown)

    prs = open_pr_count()
    print("\nopen PR: %s（併記のみ・判定には使わない）"
          % ("未取得" if prs is None else prs))

    claims = read_claims()
    if claims:
        print("\ndocs/claims.md（占有の宣言）:")
        for line in claims:
            print("  " + line)

    if args.overlap and unmerged:
        refs = [r for r, _, _ in unmerged]
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

    n = len(active)
    if n > args.block_limit:
        msg = ("WIP 上限超過（block）: active %d 本 > block %d。滞留 %d 本。"
               "新しい作業を始める前にマージするか、閉じるブランチを決めること。"
               % (n, args.block_limit, len(stale)))
        print("\n" + msg)
        print("::error::" + msg)
        return 1
    if n > args.limit:
        msg = ("WIP 注意（warn）: active %d 本 > warn %d（block %d）。滞留 %d 本。"
               % (n, args.limit, args.block_limit, len(stale)))
        print("\n" + msg)
        print("::warning::" + msg)
        return 1 if args.strict else 0
    print("\nOK: active %d 本（warn %d / block %d 以内）/ stale %d 本"
          % (n, args.limit, args.block_limit, len(stale)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
