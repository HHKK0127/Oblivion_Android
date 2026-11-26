#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""precheck.py — コミット前検査（テキスト 3 種＋構造 3 種）。

背景: 旧 precheck は %TEMP% に置かれ 2026-11-22 の掃除で消失した。本ファイルを
追跡下（scripts/）に置き直した。以後は手動 3 検査をやめ、本スクリプトを使う。

検査:
  1. CN    : 簡体字専用字（日本語では使わない）の混入
  2. FFFD  : U+FFFD（置換文字＝文字化け）
  3. dotAV : ドット直後の "AV"（.avd/.avi/。AV8 等の英数字サフィックスは除外）
  4. 中間ファイル: .bak / .orig / .rej / *~（追跡下と未追跡の両方）
  5. versionCode: 変更された build.gradle で単調増加（比較相手は常に HEAD）
  6. レーン    : Panel 所有ファイルと共用 renderer.cpp の変更を WARN（--strict で FAIL）

使い方:
  python scripts/precheck.py                 # 変更ファイルの「追加行」＋構造検査（コミット対象）
  python scripts/precheck.py FILE [FILE...]  # 指定ファイルを全文検査（テキストのみ）
  python scripts/precheck.py --all           # git 追跡下の全文書を検査（baseline 比較）
  python scripts/precheck.py --strict        # レーン WARN を FAIL に昇格
  python scripts/precheck.py --audit         # CN_CHARS ∩ 常用漢字（誤検出候補）を一覧

終了コード: 0 = OK（WARN 含む）/ 2 = FAIL（テキスト検出・構造検査・
--all の baseline 増加）。失敗の種類は 2 に統一し、出力の FAIL: / WARN: で区別する。
"""
import fnmatch
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 簡体字専用字（日本語では使わない字のみ）。拡張はこの集合へ追加する。
# 「所以」等の日本語複合語・「使用/可能/点」等の日本語語彙は誤検出のため除外済み。
# 注意: 日本語漢字（降/都/防/述/除/追/学/系/件/片/査/声/宝/没 等）は入れない。
# 据（U+636E）は常用漢字（据える/据え置き）のため 2026-11-26 に除外した（--audit で検出）。
CN_CHARS = set(
    "这么话见对说们过还进运动认问题张应该简单电脑网络软关图错误检测试让给别"
    "东乐书买卖车马鸟鱼龙岁处备变头实审导层师带帮广庆义议记讨训论设访证评识诉"
    "词译语请读谁课调谈谢贝负财责贤败货质购贯贴贵贷费贸资赋赏赔赞赠红绿蓝黑边"
    "达远连"
)

FFFD = "\ufffd"

# 行単位の CN 許可マーカー。行内にこの文字列を含む行は CN 判定をスキップする。
# 付ける条件は 1 つだけ: その行を読めば理由が分かるときだけ
# （例: 過去の文字化け修正ログが簡体字を例として引用している行）。理由が読めない
# 行に付けると、次に本物の混入が来ても素通りする。
CN_ALLOW = "precheck: allow-cn"
# dotAV: MSVC RTTI 名の "?" 落ち検出。`.?AV<Name>@@` は許可し、`?` が落ちた `.AV<Name>`（直後が大文字）を FAIL。
# （2026-11-23 実測: `.?AV...`=1473 件 / `.AV<大文字>...`=少数。`.AV<name>`・`.AV_STAMINA`・裸の `.AV` は誤検出のため除外）
DOTAV = re.compile(r"(?<!\?)\.AV[A-Z]")

# 検査 4: 中間ファイル。追跡下は ls-files、未追跡/staged は status --porcelain で見る
# （片方だけだと経路が丸ごと抜ける）。
INTERMEDIATE_PATTERNS = ("*.bak", "*.orig", "*.rej", "*~")

# 検査 4 の既知 baseline。追跡下 .bak 5 本は 2026-11-26 に削除した（履歴には
# 残る）ため、現在は空。新規の中間ファイルはすべて FAIL にする。
INTERMEDIATE_BASELINE = set()

# 検査 6: Panel レーン所有ファイル（判定はファイル名だけで機械的に導ける形にする）。
PANEL_PREFIXES = ("race_sex_panel", "ui_toggle", "chargen_commit")

TEXT_EXT = {
    ".md", ".txt", ".py", ".c", ".h", ".cpp", ".hpp", ".rs", ".json", ".xml",
    ".yml", ".yaml", ".toml", ".ps1", ".sh", ".bat", ".cmd", ".kt", ".java",
    ".gradle", ".cfg", ".ini", ".csv",
}

# --all の既知ヒット数 baseline（回帰防止）。2026-11-26 実測:
#   CN=163 / FFFD=3 / dotAV=0 = 166。据（U+636E）を CN_CHARS から外した効果が
#   -51（旧 228 から。据 は tmp/re の RE 裁定文書に多用されていた）。
# --all はこの数を超えたときのみ FAIL する（既存分は許容・増加を検出）。
ALL_BASELINE = 166

# 常用漢字 2136 字（scripts/data/joyo_kanji.txt）。CN_CHARS は「日本語では使わない
# 字」だけを持つべきなので、--audit で交差（＝誤検出候補）を一覧にする。手書き
# リストの根本対策: 除外判断のたびに --audit を 1 回回す。
JOYO_PATH = os.path.join(REPO, "scripts", "data", "joyo_kanji.txt")


def load_joyo():
    try:
        with open(JOYO_PATH, encoding="utf-8") as f:
            return set(f.read().split())
    except OSError:
        return set()


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
    for line in git_out(["-c", "core.quotepath=false", "status", "--porcelain"]).splitlines():
        if len(line) < 4:
            continue
        path = line[3:].strip()
        if " -> " in path:  # リネームは新側だけを見る
            path = path.split(" -> ", 1)[1].strip()
        if path.endswith("/") or ".bak-" in path or path.endswith(".bak"):
            continue
        files.append(path)
    return files


def intermediate_files():
    """検査 4: .bak/.orig/.rej/*~ の残存（追跡下＋未追跡/staged の両方）。

    git ls-files は index を、git status --porcelain は作業ツリーの状態を返す。
    削除（D）は「残っていない」ので除外し、最後に実在するものだけを対象にする。
    """
    found = []
    for p in git_out(["ls-files", "--"] + list(INTERMEDIATE_PATTERNS)).splitlines():
        p = p.strip()
        if p and p not in found:
            found.append(p)
    for line in git_out(["-c", "core.quotepath=false", "status", "--porcelain"]).splitlines():
        if len(line) < 4 or "D" in line[:2]:  # 削除は残存ではない
            continue
        p = line[3:].strip()
        if " -> " in p:
            p = p.split(" -> ", 1)[1].strip()
        if not any(fnmatch.fnmatch(p, pat) for pat in INTERMEDIATE_PATTERNS):
            continue
        if p and p not in found:
            found.append(p)
    return [p for p in found
            if p not in INTERMEDIATE_BASELINE
            and os.path.exists(os.path.join(REPO, p))]


def version_code_failures():
    """検査 5: 変更された build.gradle の versionCode が HEAD より増えているか。"""
    fails = []
    for rel in git_out(["ls-files", "--", "*build.gradle*"]).splitlines():
        rel = rel.strip()
        base = os.path.basename(rel)
        if not (base == "build.gradle" or base.startswith("build.gradle.")):
            continue
        # 未変更なら比較しない（HEAD と同値でも単調増加違反ではない）。
        if not git_out(["diff", "--name-only", "HEAD", "--", rel]).strip():
            continue
        head = git_out(["show", "HEAD:" + rel])
        if not head:
            continue  # 新規ファイル
        try:
            with open(os.path.join(REPO, rel), encoding="utf-8", errors="replace") as f:
                cur_text = f.read()
        except OSError:
            continue
        hm = re.search(r"versionCode\s+(\d+)", head)
        cm = re.search(r"versionCode\s+(\d+)", cur_text)
        if not hm or not cm:
            continue
        head_code, cur_code = int(hm.group(1)), int(cm.group(1))
        if cur_code <= head_code:
            fails.append(
                "versionCode が増えていません: %s (HEAD=%d, 現在=%d)"
                % (rel, head_code, cur_code))
    return fails


def lane_warnings():
    """検査 6: Panel 所有ファイルと共用 renderer.cpp の変更を WARN（--strict で FAIL）。"""
    warns = []
    files = changed_files()
    panel = [f for f in files
             if any(os.path.basename(f).startswith(p) for p in PANEL_PREFIXES)]
    if panel:
        warns.append("Panel レーン所有: %s — 合意済みか確認" % ", ".join(panel))
    if any(f.replace("\\", "/").endswith("engine/renderer.cpp") for f in files):
        warns.append("共用ファイル: renderer.cpp — 触った関数をコミット本文に書いたか確認")
    return warns


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
        if CN_ALLOW in line:
            continue  # 行単位の許可（理由がその行を読めば分かるときだけ付ける）
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
    if "--audit" in argv:
        joyo = load_joyo()
        inter = sorted(CN_CHARS & joyo)
        print("--audit: CN_CHARS=%d / 常用漢字=%d / 交差=%d"
              % (len(CN_CHARS), len(joyo), len(inter)))
        if not joyo:
            print("  警告: %s を読めませんでした（常用漢字リスト未取得）" % JOYO_PATH)
            return 0
        if not inter:
            print("  交差なし（CN_CHARS は簡体字専用字のみ）")
            return 0
        for ch in inter:
            print("  誤検出候補（日本語で使う字）: %s  U+%04X" % (ch, ord(ch)))
        print("  → これらは CN_CHARS から外す（『日本語では使わない字』の定義に反する）。")
        return 0

    all_mode = "--all" in argv
    strict = "--strict" in argv
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

    structural = 0
    warns = 0
    if not all_mode and files is None:  # 既定モード（コミット前）だけ構造検査を走らせる
        for p in intermediate_files():
            print("FAIL: 中間ファイルが残っています: %s（削除するか .gitignore へ）" % p)
            structural += 1
        for msg in version_code_failures():
            print("FAIL: " + msg)
            structural += 1
        for msg in lane_warnings():
            if strict:
                print("FAIL: " + msg)
                structural += 1
            else:
                print("WARN: " + msg)
                warns += 1

    if all_mode:
        for path in files:
            rel = os.path.relpath(path, REPO)
            emit(rel, check_file(path))
        if total > ALL_BASELINE:
            print("precheck: FAIL --all (%d > baseline %d; increase detected)"
                  % (total, ALL_BASELINE))
            return 2
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

    if structural:
        print("precheck: FAIL (%d structural, %d text hit(s))" % (structural, total))
        return 2
    if total:
        print("precheck: FAIL (%d hit(s))" % total)
        return 2
    if warns:
        print("precheck: OK (%d warning(s))" % warns)
        return 0
    print("precheck: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
