#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""nif_oracle_check.py — 実物 fixture NIF を dump_rust に通し、コミット済み期待値と比較する。

期待値（docs/evidence/fixtures/nif/oracle/<markers|cover>/…/*.nif.txt）は元 C++ パーサ
（dump_orig）の出力で、リポジトリに固定してある。dump_rust の出力が 1 バイトでも違えば
exit 2。これで oracle は「ローカルの手順」ではなく「検査可能な固定値」になる。

完全性: fixtures/nif/{markers,cover} 配下の **全 .nif に対応する期待値**が無ければ FAIL。
fixture に NIF を足してダンプを忘れた分が静かに未検査になるのを防ぐ。

--update は期待値を焼き直す（dump_orig がローカルにあるときだけ）。赤いときは焼き直す前に
赤い理由を 1 行書くこと。

前提: scripts/build_nif_oracle.sh で dump_rust を作っておく。
使い方:
  python scripts/nif_oracle_check.py
  python scripts/nif_oracle_check.py --update
"""
import argparse
import os
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIX = os.path.join(REPO, "docs", "evidence", "fixtures", "nif")
ORACLE = os.path.join(FIX, "oracle")
ROOTS = ("markers", "cover")
DUMP_RUST = os.path.join(REPO, "tmp", "nif_oracle", "dump_rust")
DUMP_ORIG = os.path.join(REPO, "tmp", "nif_oracle", "dump_orig")


def find_exe(path):
    for cand in (path + ".exe", path):
        if os.path.isfile(cand):
            return cand
    return None


def run(exe_path, nif):
    r = subprocess.run([exe_path, nif], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return r.stdout


def fixture_nifs():
    """fixtures/nif/{markers,cover} 配下の .nif（fixtures/nif からの相対パス）。"""
    out = []
    for root in ROOTS:
        base = os.path.join(FIX, root)
        for dp, _dirs, names in os.walk(base):
            for name in sorted(names):
                if name.lower().endswith(".nif"):
                    rel = os.path.relpath(os.path.join(dp, name), FIX).replace("\\", "/")
                    out.append(rel)
    return sorted(out)


def main():
    ap = argparse.ArgumentParser(description="NIF オラクル（dump_rust vs 固定期待値）")
    ap.add_argument("--update", action="store_true",
                    help="期待値を dump_orig で焼き直す（赤い理由を書いてから）")
    args = ap.parse_args()

    nifs = fixture_nifs()
    if not nifs:
        sys.exit("fixture NIF が 0 件です: %s/{%s}" % (FIX, ",".join(ROOTS)))

    if args.update:
        do = find_exe(DUMP_ORIG)
        if not do:
            sys.exit("--update には dump_orig が要ります（scripts/build_nif_oracle.sh）。")
        for rel in nifs:
            dst = os.path.join(ORACLE, rel + ".txt")
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            with open(dst, "w", encoding="utf-8", newline="\n") as f:
                f.write(run(do, os.path.join(FIX, rel)))
        print("updated %d expected dump(s) in %s" % (len(nifs), ORACLE))
        return 0

    dr = find_exe(DUMP_RUST)
    if not dr:
        sys.exit("dump_rust が見つかりません。scripts/build_nif_oracle.sh を実行してください。")

    same, diff, missing = 0, [], []
    for rel in nifs:
        ref = os.path.join(ORACLE, rel + ".txt")
        if not os.path.isfile(ref):
            missing.append(rel)      # 完全性: fixture の NIF に期待値が無い
            continue
        with open(ref, encoding="utf-8", errors="replace") as f:
            want = f.read()
        if run(dr, os.path.join(FIX, rel)) == want:
            same += 1
        else:
            diff.append(rel)

    print("nif_oracle_check: %d same / %d diff / %d missing (of %d)"
          % (same, len(diff), len(missing), len(nifs)))
    for rel in diff[:8]:
        print("  DIFF: %s" % rel)
    for rel in missing[:8]:
        print("  MISSING(expected dump): %s" % rel)
    if missing:
        print("::error:: 期待値の無い fixture NIF があります（--update で生成）")
    if diff or missing:
        print("::error:: dump_rust が固定期待値と一致しません")
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
