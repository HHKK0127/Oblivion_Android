# -*- coding: utf-8 -*-
import io, re
bad = re.compile(r'(?<![.\w])\?\$?AV')
cn = re.compile('否则|这个|已经|我们|可以|然后|里面|现在|因为|所以')
for p in [
    r"C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v569_p3_p5_rows_ruling.md",
    r"C:\Users\hiroki.kogarumai\Oblivion_Android\STATE.md",
    r"C:\Users\hiroki.kogarumai\Oblivion_Android\.github\memory\session-memory.json",
]:
    s = io.open(p, encoding="utf-8").read()
    print("%-42s dotAV=%d CN=%d FFFD=%d len=%d" % (p.split("\\")[-1], len(bad.findall(s)), len(cn.findall(s)), s.count("\ufffd"), len(s)))
# row545 sanity in ledger
s = io.open(r"C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\TODO_re49.md", encoding="utf-8").read()
i = s.index("| 545 |"); j = s.index("\n", i)
seg = s[i:j]
print("row545 pipes=%d #161=%d #157=%d" % (seg.count("|"), seg.count("#161"), seg.count("#157")))
print("v569 doc title has #157:", "#157 フォロー" in io.open(r"C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v569_p3_p5_rows_ruling.md", encoding="utf-8").read())
