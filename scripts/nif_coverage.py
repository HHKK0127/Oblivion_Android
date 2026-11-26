#!/usr/bin/env python3
"""nif_coverage.py — NIF ブロック型の「オラクル検証済み」網羅率を実コーパスに対して測る。

check_imports.py（ttom795）と同じ発想で、対象が import ではなくブロック型。

3 つの集合を区別する（混同しない）:
  * corpus_types  — 実コーパスの生 NIF ヘッダにある型（語彙。実在する型だけ）。
  * impl_covered  — nif-core の `walk_block_body` の match 腕に名前がある型。
                    名前の有無しか見ないので「実装が正しい」ことの証拠にはならない。
  * oracle_verified — out_orig の差分オラクルがバイト検証した型（`bt[n]=` 由来）。
                    **これが本当の穴**。unverified = corpus_types - oracle_verified。

型の供給元（corpus）は 3 通り:
  * 既定: コーパス内の生 NIF のヘッダ（ブロック型表）を読む。
  * --types-from DIR: ディレクトリ内のテキスト（期待値ダンプ等）から型名らしき語を拾う。
  * --types-list FILE: 1 行 1 型のテキストをそのまま語彙にする。

限界:
  - impl_covered は「名前がある」だけ。フィールドの正しさは対象外。
  - oracle_verified は out_orig のダンプが対象にした型のみ。オラクルを増やすまで
    増えない（soft ラチェット）。
  - コーパスは展開済みディレクトリ。BSA 直読みは未対応。

終了コード:
  0 = ok / 1 = スクリプト異常（引数・欠如・語彙 0）/ 2 = hard ゲート後退。

使い方:
  python scripts/nif_coverage.py --corpus meshes_extracted --oracle tmp/nif_oracle
  python scripts/nif_coverage.py --corpus meshes_extracted --oracle tmp/nif_oracle \
      --min-impl 119 --max-stray 0 --min-oracle 2
  python scripts/nif_coverage.py --corpus meshes_extracted --show NiTriStrips
"""
import argparse
import json
import os
import re
import sys
from collections import Counter, defaultdict

MAGIC = (b"NetImmerse File Format", b"Gamebryo File Format")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_OF_CORPUS = os.path.join(REPO, "scripts", "data", "known_out_of_corpus.txt")
DEFAULT_CORPUS = os.path.join("tmp", "nif_oracle")
DEFAULT_IMPL = os.path.join("rust", "crates", "nif-core", "src")
SOURCE_EXT = (".rs", ".c", ".cc", ".cpp", ".h", ".hpp")

LITERAL = re.compile(r'"([A-Za-z][A-Za-z0-9_]{1,40})"')

# walk_block_body の match 腕にある型名。`"NiTriShape" | "NiTriStrips" =>` のような
# 複数名の腕も拾えるよう、行頭アンカーではなく本体全体から引用名を抜く。
HANDLER_RE = re.compile(r'"([A-Za-z_]\w*)"')
# match 腕のパターン（"A" | "B" =>）。本体の文字列リテラル（"Vector3" 等）は
# `=>` の直前ではないので拾わない。入れ子 match の腕も拾う（多段の分岐で扱う型を
# 落とさないため）。複数行にまたがるパターンも \s が改行を跨ぐので拾える。
ARM_RE = re.compile(r'(?:"[A-Za-z_]\w*"\s*\|?\s*)+\s*=>')
ORACLE_BT = re.compile(r"^\s*bt\[\d+\]=(\S+)", re.M)

BLOCK_PREFIX = ("Ni", "BS", "bhk", "Bhk", "Root", "Avoid", "BSX", "BSFade",
                "BSEffect", "BSValue", "BSOrdered", "BSMultiBound")
TOKEN = re.compile(r"\b([A-Za-z][A-Za-z0-9_]{2,40})\b")
TEXT_EXT = (".json", ".txt", ".csv", ".md", ".yaml", ".yml")


class Reader:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def take(self, n):
        if self.p + n > len(self.d):
            raise EOFError("truncated")
        b = self.d[self.p:self.p + n]
        self.p += n
        return b

    def u8(self):
        return self.take(1)[0]

    def u16(self):
        return int.from_bytes(self.take(2), "little")

    def u32(self):
        return int.from_bytes(self.take(4), "little")

    def byte_string(self):
        """BzString: u8 長（終端 NUL 込み）＋本体。"""
        return self.take(self.u8()).decode("ascii", "replace")

    def line(self):
        e = self.d.find(b"\n", self.p)
        if e < 0:
            raise EOFError("no header line")
        s = self.d[self.p:e]
        self.p = e + 1
        return s

    def sized(self):
        n = self.u32()
        if n > 512:
            raise ValueError("sized string too long")
        return self.take(n).decode("ascii", "replace")


def block_types(path):
    """ヘッダのブロック型表だけを読む。以降は解析しない。

    レイアウトは nif-core の ``read_header``（rust/crates/nif-core/src/header.rs）
    に一致させる。20.0.0.x は version の後に endian(u8)・user_version(u32)・
    num_objects(u32)・unknown_field(u32)・BzString×3 が続き、型数は u16。
    """
    with open(path, "rb") as f:
        r = Reader(f.read())
    magic = bytearray()
    while len(magic) < 255:
        b = r.u8()
        if b == 0x0A or b == 0:
            break
        magic.append(b)
    magic = bytes(magic)
    legacy = b"NetImmerse File Format" in magic
    if not legacy and b"Gamebryo File Format" not in magic:
        raise ValueError("not a NIF")
    ver = r.u32()
    if ver < 0x05000001:                 # 5.0.0.1 未満は型テーブル無し
        r.u32()                          # num_objects
        return ver, []
    if legacy:
        r.u32()                          # num_objects
        if ver == 0x0A000102:            # 唯一の NetImmerse BS stream header
            r.u32()                      # unknown field
            for _ in range(3):
                r.byte_string()
    else:
        if ver >= 0x14000000:
            r.u8()                       # endianness byte
        r.u32()                          # user version
        r.u32()                          # num_objects
        r.u32()                          # unknown field (BS stream version)
        for _ in range(3):
            r.byte_string()              # creator / process / export script
    ntypes = r.u16()
    if ntypes > 4096:
        raise ValueError("implausible block type count")
    return ver, [r.sized() for _ in range(ntypes)]


def scan_corpus(corpus):
    files = 0
    types = Counter()        # 型 -> 出現ファイル数
    errors = Counter()
    for root, _, names in os.walk(corpus):
        for name in sorted(names):
            if not name.lower().endswith(".nif"):
                continue
            path = os.path.join(root, name)
            files += 1
            try:
                _, ts = block_types(path)
            except (EOFError, ValueError, OSError) as e:
                errors[type(e).__name__] += 1
                continue
            for t in set(ts):
                types[t] += 1
    return files, types, errors


def types_from_text(path):
    """期待値ダンプからブロック型名らしき語を拾う。"""
    types = Counter()
    for root, _, names in os.walk(path):
        for name in names:
            if not name.lower().endswith(TEXT_EXT):
                continue
            with open(os.path.join(root, name), encoding="utf-8", errors="replace") as f:
                for tok in TOKEN.findall(f.read()):
                    if tok.startswith(BLOCK_PREFIX):
                        types[tok] += 1
    return types


def types_from_list(path):
    with open(path, encoding="utf-8") as f:
        return Counter({l.strip(): 1 for l in f if l.strip()})


def load_types(args):
    if args.types_list:
        return 0, types_from_list(args.types_list), Counter()
    files, types, errors = scan_corpus(args.corpus)
    if args.types_from and not types:
        # ディレクトリに NIF が無い場合はダンプから拾い直す。
        return files, types_from_text(args.types_from), errors
    return files, types, errors


def load_oracle_types(oracle_dir):
    """差分オラクル（out_orig）がバイト検証した型集合。"""
    types = set()
    out_orig = os.path.join(oracle_dir, "out_orig")
    if not os.path.isdir(out_orig):
        return types
    for name in sorted(os.listdir(out_orig)):
        if not name.endswith(".nif.txt"):
            continue
        with open(os.path.join(out_orig, name), encoding="utf-8", errors="replace") as f:
            for m in ORACLE_BT.finditer(f.read()):
                types.add(m.group(1))
    return types


def walk_block_body_region(impl_dir):
    """walk.rs の `fn walk_block_body` 本体（次の行頭 fn まで・行コメント除去）。"""
    path = os.path.join(impl_dir, "walk.rs")
    if not os.path.isfile(path):
        return ""
    with open(path, encoding="utf-8", errors="replace") as f:
        text = f.read()
    m = re.search(r"fn\s+walk_block_body\b", text)
    if not m:
        return ""
    rest = text[m.end():]
    nxt = re.search(r"(?m)^\s*fn\s", rest)
    body = rest[:nxt.start()] if nxt else rest
    return "\n".join(line.split("//", 1)[0] for line in body.splitlines())


def outermost_match_arms(body):
    """match 腕のパターン部（`"A" | "B" =>` の `=>` より前）を返す。

    全深さの腕を対象にする（入れ子 match で扱う型を落とさない）。本体の文字列
    リテラル（"Vector3" 等）は `=>` の直前ではないので入らない。これが無いと
    handler 本体に "Vector3" を 1 つ足すだけで stray が増え、型の変化と無関係に
    ゲートが落ちる。複数行パターンも ARM_RE の \s が改行を跨ぐので拾える。
    """
    return [m.group(0) for m in ARM_RE.finditer(body)]


def scan_impl(impl_dir):
    """walk_block_body の match 腕パターンにある型名（本体のリテラルは拾わない）。"""
    body = walk_block_body_region(impl_dir)
    return set(HANDLER_RE.findall("\n".join(outermost_match_arms(body))))


def load_out_of_corpus():
    """実装にあるが意図的にコーパス外の型（typo 検出のため stray から除外）。"""
    names = set()
    try:
        with open(OUT_OF_CORPUS, encoding="utf-8") as f:
            for line in f:
                line = line.split("#", 1)[0].strip()
                if line:
                    names.add(line)
    except OSError:
        pass
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--corpus", default=DEFAULT_CORPUS)
    ap.add_argument("--impl", default=DEFAULT_IMPL)
    ap.add_argument("--oracle", default=None, metavar="DIR",
                    help="差分オラクルの out_orig/*.nif.txt から検証済み型を取る")
    ap.add_argument("--types-from", default=None, metavar="DIR",
                    help="生 NIF が無いとき、このディレクトリのテキストから型を拾う")
    ap.add_argument("--types-list", default=None, metavar="FILE",
                    help="1 行 1 型のテキストを語彙にする（コーパスを読まない）")
    ap.add_argument("--min-impl", type=int, default=None,
                    help="impl_covered がこれを下回ったら exit 2（hard ラチェット）")
    ap.add_argument("--max-stray", type=int, default=None,
                    help="stray がこれを上回ったら exit 2（hard）")
    ap.add_argument("--max-missing", type=int, default=None,
                    help="missing（コーパスにあるが handler の無い型）がこれを上回ったら exit 2（hard）")
    ap.add_argument("--min-oracle", type=int, default=None,
                    help="oracle_verified がこれを下回ったら警告（soft ラチェット）")
    ap.add_argument("--top", type=int, default=10)
    ap.add_argument("--show", default=None, metavar="SUBSTR")
    ap.add_argument("--json", default=None)
    args = ap.parse_args()

    if not args.types_list and not os.path.isdir(args.corpus):
        sys.exit("corpus not found: %s" % args.corpus)
    if not os.path.isdir(args.impl):
        sys.exit("impl not found: %s" % args.impl)

    files, types, errors = load_types(args)
    if not types:
        sys.exit("no block types found under %s" % args.corpus)

    known = set(types)
    impl_types = scan_impl(args.impl)
    impl_covered = known & impl_types
    out_of_corpus = load_out_of_corpus()
    stray = impl_types - known - out_of_corpus
    excluded = (impl_types - known) & out_of_corpus
    missing = known - impl_covered  # コーパスにあるが handler の無い型

    oracle_types = load_oracle_types(args.oracle) if args.oracle else set()
    oracle_verified = known & oracle_types
    unverified = known - oracle_types

    if args.show:
        for t in sorted(n for n in known if args.show in n):
            marks = []
            marks.append("IMPL" if t in impl_covered else "----")
            marks.append("ORAC" if t in oracle_verified else "----")
            print("%s %s  %-40s %3d files" % (marks[0], marks[1], t, types[t]))
        return

    print("corpus    : %s" % args.corpus)
    print("files     : %s (%d unreadable)"
          % ("-" if args.types_list else files, sum(errors.values())))
    for k, v in errors.most_common():
        print("            %-12s %d" % (k, v))
    # 見出しはオラクル被覆（本当の穴）。impl_covered は補助。
    print("== oracle_verified / corpus_types = %d / %d (%.1f%%) =="
          % (len(oracle_verified), len(known),
             100.0 * len(oracle_verified) / len(known) if known else 0.0))
    print("corpus_types    : %d distinct" % len(known))
    print("impl_covered    : %d (%.1f%%)"
          % (len(impl_covered), 100.0 * len(impl_covered) / len(known)))
    print("missing         : %d（コーパスにあるが handler が無い型）" % len(missing))
    if missing:
        for t in sorted(missing):
            print("    - %s" % t)
    if args.oracle:
        print("oracle_verified : %d" % len(oracle_verified))
        print("unverified      : %d (%.1f%%)"
              % (len(unverified), 100.0 * len(unverified) / len(known)))
        if unverified:
            print("\n未検証（頻度降順・オラクル未達）:")
            for t, n in sorted(((t, types[t]) for t in unverified),
                               key=lambda x: -x[1])[:args.top]:
                print("  %-42s %4d files" % (t, n))
    if stray:
        print("\nstray（walk_block_body にあるがコーパスに無い型）: %d" % len(stray))
        for t in sorted(stray)[:args.top]:
            print("  %s" % t)
    if excluded:
        print("\n除外（known_out_of_corpus.txt・実装にあるがコーパス外）: %d" % len(excluded))

    if args.json:
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump({"corpus": args.corpus, "files": files,
                       "corpus_types": len(known),
                       "impl_covered": len(impl_covered),
                       "oracle_verified": len(oracle_verified),
                       "unverified": {t: types[t] for t in sorted(unverified)},
                       "stray": sorted(stray)}, f, ensure_ascii=False, indent=1)

    failed = 0
    if args.min_impl is not None and len(impl_covered) < args.min_impl:
        print("\n::error:: impl_covered %d < %d（実装の後退）" % (len(impl_covered), args.min_impl))
        failed = 1
    if args.max_stray is not None and len(stray) > args.max_stray:
        print("\n::error:: stray %d > %d（コーパス外の型が増えた）" % (len(stray), args.max_stray))
        failed = 1
    if args.max_missing is not None and len(missing) > args.max_missing:
        print("\n::error:: missing %d > %d（handler の無い型が増えた）"
              % (len(missing), args.max_missing))
        failed = 1
    if args.min_oracle is not None and len(oracle_verified) < args.min_oracle:
        # soft ラチェット: オラクルは local-only なので警告に留める。
        print("\n::warning:: oracle_verified %d < %d（オラクルを増やしたら上げる）"
              % (len(oracle_verified), args.min_oracle))
    if failed:
        sys.exit(2)


if __name__ == "__main__":
    main()
