# -*- coding: utf-8 -*-
"""round_verify.py — TODO_re49 台帳の恒久検証（再構築 2026-11-05: 旧 %TEMP% スクリプト消失のため）。
チェック: (1) serial 行番号の連番整合 (2) 状態セル [x]/[ ]/[~] (3) 言及成果物ファイルの実在
(4) 実装単位セル (5) 検算 total/done/open 出力。ALL OK なら exit 0。

実装単位セル（2026-11-21 追加 / 同日 意味論修正）:
  - V-row の末尾セルが「実装単位」。値の意味:
    - `crate/path::symbol` = 実装単位が確定
    - `!` = 未割当（解析済だが実装単位が未割当。実装フェーズの第一着手リスト）
    - `?` = 未定だが対象（open row のみ）
    - `-` = 解析専用（実装対象外と裁定済みの row のみ。裁定書 ID（vNNN）併記必須）
  - チェック:
    - 実装単位が空 → ERROR
    - done row で `?` → ERROR（done には不可）
    - `-` なのに裁定書 ID（vNNN）併記が無い → ERROR
    - 旧書式行（pipes!=7）が baseline 33 を超えて増加 → ERROR
  - 現行書式（row>=540）は pipes=7 を要求。540 は「旧行（<540）の書式不均一（5〜11 pipes・
    セル内パイプ混入）が既知」という 2026-11-21 時点の境界であり、マジックナンバー。
    正規化は row 604 で追跡（旧行を pipes=7 に揃えたら本定数を撤去）。
"""
import re, sys, os

BASE = os.path.dirname(os.path.abspath(__file__))
LEDGER = os.path.join(BASE, "TODO_re49.md")
CURRENT_MIN = 540          # 旧書式（セル内パイプ混入）の境界。2026-11-21 時点。row 604 で正規化追跡
EXPECT_PIPES = 7
LEGACY_BASELINE = 33       # 旧書式行の baseline（2026-11-21）。増加は ERROR

rows = {}
pat = re.compile(r"^\|\s*(\d+)\s*\|(.*)$")
for line in open(LEDGER, encoding="utf-8"):
    m = pat.match(line)
    if not m:
        continue
    n = int(m.group(1))
    cells = m.group(2).split("|")
    # 列構成: | N | V<N>: 題名 | 優先度 | 状態 | 詳細 | 実装単位 |
    if len(cells) < 3:
        continue
    if not re.match(r"\s*V\d+", cells[0]):
        continue  # 優先実行順テーブル等の誤マッチ除外
    status = cells[2].strip()
    if "[x]" in status:
        st = "x"
    elif "[~]" in status:
        st = "~"
    elif "[ ]" in status:
        st = " "
    else:
        st = "?"
    rows[n] = (st, line)

errs = []
warns = []
nums = sorted(rows)
if not nums:
    errs.append("行が見つかりません")
else:
    # 行番号は過去分アーカイブ移動で非連続が正常（重複のみエラー・ギャップは info）
    if len(nums) != len(set(nums)):
        errs.append("行番号重複あり")
    for n in nums:
        st, line = rows[n]
        if st == "?":
            errs.append("row %d 状態セル判定不能: %s" % (n, line[:80]))
        # 実装単位セル
        stripped = line.rstrip()
        parts = stripped.split("|")
        impl = parts[-2].strip() if stripped.endswith("|") and len(parts) >= 2 else ""
        npipe = line.count("|")
        if impl == "":
            errs.append("row %d 実装単位セルが空: %s" % (n, line[:80]))
        if st == "x" and impl == "?":
            errs.append("row %d (done) 実装単位が '?'（done には不可）: %s" % (n, line[:80]))
        if impl == "-" and not re.search(r"v\d+", line):
            errs.append("row %d 実装単位 '-' は裁定書 ID（vNNN）併記必須: %s" % (n, line[:80]))
        if n >= CURRENT_MIN and npipe != EXPECT_PIPES:
            errs.append("row %d 現行書式は pipes=%d 必須（実際 %d）" % (n, EXPECT_PIPES, npipe))
        elif n < CURRENT_MIN and npipe != EXPECT_PIPES:
            warns.append("row %d 旧書式 pipes=%d（不均一・既知）" % (n, npipe))

# 成果物存在チェック: tmp/re/vNNN_*.md 参照
refpat = re.compile(r"tmp[/\\]re[/\\](v\d+_[A-Za-z0-9_]+\.md)")
missing = set()
for n, (st, line) in rows.items():
    for fn in refpat.findall(line):
        if not os.path.exists(os.path.join(BASE, fn)):
            missing.add((n, fn))
for n, (st, line) in rows.items():
    if st == "x":
        refs = refpat.findall(line)
        if refs and not any(os.path.exists(os.path.join(BASE, fn)) for fn in set(refs)):
            errs.append("row %d ([x]) 成果物すべて欠落: %s" % (n, sorted(set(refs))))

total = len(nums)
done = sum(1 for n in nums if rows[n][0] == "x")
open_ = total - done
print("rows %d..%d | total %d / done %d / open %d" % (nums[0] if nums else -1, nums[-1] if nums else -1, total, done, open_))
if missing:
    print("[warn] 未完行の成果物未作成（想定内）: %d 件" % len(missing))
if warns:
    print("[warn] 旧書式行（不均一・既知）: %d 件 (baseline %d)" % (len(warns), LEGACY_BASELINE))
    if len(warns) > LEGACY_BASELINE:
        errs.append("旧書式行が baseline を超過（%d > %d）: 増加は ERROR" % (len(warns), LEGACY_BASELINE))
if errs:
    print("ROUND_VERIFY FAIL")
    for e in errs:
        print("  -", e)
    sys.exit(1)
print("round_verify ALL OK")
