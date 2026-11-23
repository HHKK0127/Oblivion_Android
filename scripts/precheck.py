#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""precheck.py — コミット前テキスト検査（CN / U+FFFD / dotAV）。

背景: 旧 precheck は %TEMP% に置かれ 2026-11-22 の掃除で消失した。本ファイルを
追跡下（scripts/）に置き直した。以後は手動 3 検査をやめ、本スクリプトを使う。

検査:
  1. CN    : 簡体字専用字（日本語では使わない）の混入
  2. FFFD  : U+FFFD（置換文字＝文字化け）
  3. dotAV : ドット直後の "AV"（.avd/.avi/。AV8 等の英数字サフィックスは除外）

使い方:
  python scripts/precheck.py                 # 変更ファイルの「追加行」のみ検査（コミット対象）
  python scripts/precheck.py FILE [FILE...]  # 指定ファイルを全文検査
  python scripts/precheck.py --all           # git 追跡下の全文書を検査（baseline 比較）

終了コード: 0 = OK / 1 = 検出あり（--all は baseline からの増加のみ 1）。
"""
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 簡体字専用字（日本語では使わない字のみ）。拡張はこの集合へ追加する。
# 「所以」等の日本語複合語・「使用/可能/点」等の日本語語彙は誤検出のため除外済み。
# 注意: 日本語漢字（降/都/防/述/除/追/学/系/件/片/査/声/宝/没 等）は入れない。
CN_CHARS = set(
    "这么话见对说们过还进运动认问题张应该简单电脑网络软关图据错误检测试让给别"
    "东乐书买卖车马鸟鱼龙岁处备变头实审导层师带帮广庆义议记讨训论设访证评识诉"
    "词译语请读谁课调谈谢贝负财责贤败货质购贯贴贵贷费贸资赋赏赔赞赠红绿蓝黑边"
    "达远连"
)

FFFD = "\ufffd"
# ドット直後の "AV"。ただし .avd/.avi/。AV8 のような英数字サフィックスは除外（誤検出防止）。
DOTAV = re.compile(r"(?:\.|．|。)\s*[Aa][Vv](?![A-Za-z0-9])")

TEXT_EXT = {
    ".md", ".txt", ".py", ".c", ".h", ".cpp", ".hpp", ".rs", ".json", ".xml",
    ".yml", ".yaml", ".toml", ".ps1", ".sh", ".bat", ".cmd", ".kt", ".java",
    ".gradle", ".cfg", ".ini", ".csv",
}

# --all の既知ヒット数 baseline（回帰防止）。2026-11-23 実測:
#   CN=228（日本語文書への簡体字混入。実在）/ FFFD=3（文字化け実在）/ dotAV=26（誤検出寄り）
# --all はこの数を超えたときのみ FAIL する（既存分は許容・増加を検出）。
ALL_BASELINE = 257


def is_text(path):
    return os.path.splitext(path)[1].lower() in TEXT_EXT


def git_out(args):
    try:
        return subprocess.run(
            ["git"] + args, cwd=REPO, capture_output=True, text=True,
            encoding="utf-8", errors="replace",
        ).stdout
    except Exception:
        return ""


def changed_files():
    files = []
    for line in git_out(["status", "--porcelain"]).splitlines():
        if len(line) < 4:
            continue
        path = line[3:].strip()
        if path.endswith("/") or ".bak-" in path or path.endswith(".bak"):
            continue
        files.append(path)
    return files


def all_tracked_files():
    return [os.path.join(REPO, p) for p in git_out(["ls-files"]).splitlines() if p.strip()]


def is_untracked(rel):
    return git_out(["ls-files", "--error-unmatch", "--", rel]).strip() == ""


def added_lines(rel):
    """変更ファイルの追加行（untracked は全文）。"""
    full = os.path.join(REPO, rel)
    if is_untracked(rel):
        try:
            with open(full, encoding="utf-8", errors="replace") as f:
                return f.read().splitlines()
        except Exception:
            return []
    out = git_out(["diff", "-U0", "--", rel]) + "\n" + git_out(["diff", "-U0", "--cached", "--", rel])
    return [l[1:] for l in out.splitlines() if l.startswith("+") and not l.startswith("+++")]


def scan_lines(lines):
    hits = []
    for n, line in enumerate(lines, 1):
        for ch in line:
            if ch in CN_CHARS:
                hits.append((n, "CN", ch))
                break
        if FFFD in line:
            hits.append((n, "FFFD", "U+FFFD"))
        m = DOTAV.search(line)
        if m:
            hits.append((n, "dotAV", m.group(0)))
    return hits


def check_file(path):
    if not is_text(path) or not os.path.isfile(path):
        return []
    if os.path.abspath(path) == os.path.abspath(__file__):
        return []  # 本スクリプトは CN リストを含むため除外
    with open(path, encoding="utf-8", errors="replace") as f:
        return scan_lines(f.read().splitlines())


def main(argv):
    all_mode = "--all" in argv
    if all_mode:
        files = all_tracked_files()
    else:
        args = [a for a in argv[1:] if not a.startswith("--")]
        files = [os.path.abspath(a) for a in args] if args else None

    total = 0

    def emit(rel, hits):
        nonlocal total
        for n, kind, what in hits:
            print("%s:%d: [%s] %r" % (rel, n, kind, what))
            total += 1

    if all_mode:
        for path in files:
            rel = os.path.relpath(path, REPO)
            emit(rel, check_file(path))
        if total > ALL_BASELINE:
            print("precheck: FAIL --all (%d > baseline %d; increase detected)"
                  % (total, ALL_BASELINE))
            return 1
        print("precheck: OK --all (%d hit(s) <= baseline %d; pre-existing)"
              % (total, ALL_BASELINE))
        return 0

    if files is not None:  # 明示ファイル列 → 全文検査
        for path in files:
            try:
                rel = os.path.relpath(path, REPO)
            except ValueError:
                rel = path
            emit(rel, check_file(path))
    else:  # 変更ファイル → 追加行のみ検査
        self_rel = os.path.relpath(os.path.abspath(__file__), REPO).replace("\\", "/")
        for rel in changed_files():
            if not is_text(rel) or rel.replace("\\", "/") == self_rel:
                continue
            emit(rel, scan_lines(added_lines(rel)))

    if total:
        print("precheck: FAIL (%d hit(s))" % total)
        return 1
    print("precheck: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
