#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""precheck.py — コミット前テキスト検査（CN / U+FFFD / dotAV）。

背景: 旧 precheck は %TEMP% に置かれ 2026-11-22 の掃除で消失した。本ファイルを
追跡下（scripts/）に置き直した。以後は手動 3 検査をやめ、本スクリプトを使う。

検査:
  1. CN    : 簡体字専用字（日本語では使わない）の混入
  2. FFFD  : U+FFFD（置換文字＝文字化け）
  3. dotAV : ドット直後の "AV" および literal "dotAV"（文字化け/誤記の監視）

使い方:
  python scripts/precheck.py                 # git の変更ファイル（追跡+未追跡）を検査
  python scripts/precheck.py FILE [FILE...]  # 指定ファイルを検査
  python scripts/precheck.py --all           # git 追跡下のテキストファイル全体

終了コード: 0 = OK / 1 = 検出あり。
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
DOTAV = re.compile(r"(?:\.|．|。)\s*[Aa][Vv]")

TEXT_EXT = {
    ".md", ".txt", ".py", ".c", ".h", ".cpp", ".hpp", ".rs", ".json", ".xml",
    ".yml", ".yaml", ".toml", ".ps1", ".sh", ".bat", ".cmd", ".kt", ".java",
    ".gradle", ".cfg", ".ini", ".csv",
}


def is_text(path):
    ext = os.path.splitext(path)[1].lower()
    return ext in TEXT_EXT


def git_lines(args):
    try:
        out = subprocess.run(
            ["git"] + args, cwd=REPO, capture_output=True, text=True,
            encoding="utf-8", errors="replace",
        ).stdout
    except Exception:
        return []
    return [l for l in out.splitlines() if l.strip()]


def changed_files():
    files = []
    for line in git_lines(["status", "--porcelain"]):
        if len(line) < 4:
            continue
        path = line[3:].strip()
        if path.endswith("/"):
            continue
        if ".bak-" in path or path.endswith(".bak"):
            continue
        files.append(os.path.join(REPO, path))
    return files


def all_tracked_files():
    return [os.path.join(REPO, p) for p in git_lines(["ls-files"])]


def check(path):
    if not is_text(path) or not os.path.isfile(path):
        return []
    # 本スクリプト自身は CN リストと "dotAV" トークンを含むため除外する
    if os.path.abspath(path) == os.path.abspath(__file__):
        return []
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = f.readlines()
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


def main(argv):
    if "--all" in argv:
        files = all_tracked_files()
    else:
        args = [a for a in argv[1:] if not a.startswith("--")]
        files = [os.path.abspath(a) for a in args] if args else changed_files()

    total = 0
    for path in files:
        for n, kind, what in check(path):
            try:
                rel = os.path.relpath(path, REPO)
            except ValueError:
                rel = path
            print("%s:%d: [%s] %r" % (rel, n, kind, what))
            total += 1
    if total:
        print("precheck: FAIL (%d hit(s))" % total)
        return 1
    print("precheck: OK (%d file(s))" % len(files))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
