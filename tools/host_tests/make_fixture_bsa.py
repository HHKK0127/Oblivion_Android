#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_fixture_bsa.py - docs/evidence/fixtures/minimal.bsa を決定論的に生成する。

レイアウトの出所: rust/crates/bsa-core/src/{format.rs,hash.rs} と、受理済みテスト
rust/crates/bsa-core/tests/parse.rs の build() が組むバイト列を逐語で踏襲する。

中身は **tools/host_tests/make_fixture_nif.py が作る最小 NIF** を
`meshes\\test.nif` として 1 本だけ入れる。これで消費側は
「BSA を開く → findFile → 取り出す → NIFParser で parse する」という
**BSA → NIF の連鎖**を 1 つのフィクスチャで通せる。

範囲: 1 フォルダ・1 ファイル・非圧縮・名前テーブルあり（Oblivion 0x67）。
時刻・乱数・絶対パスを使わない（再実行しても同一バイト列）。
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_fixture_nif  # noqa: E402  (同ディレクトリの生成器を再利用)

BSA_MAGIC = 0x0041_5342          # 'BSA\0' (little-endian 'BSA\x00')
OB_HEADER_VERSION = 0x67
FLAG_HAS_FOLDER_NAMES = 0x1
FLAG_HAS_FILE_NAMES = 0x2
HEADER_SIZE = 36
FOLDER_RECORD_SIZE = 16
FILE_RECORD_SIZE = 16
HASH_MULTIPLIER = 0x0010_03F


def u32(v):
    return struct.pack("<I", v & 0xFFFFFFFF)


def u64(v):
    return struct.pack("<Q", v & 0xFFFFFFFFFFFFFFFF)


# --- hash.rs の逐語移植 -----------------------------------------------------


def _lower(byte):
    return byte + 0x20 if 0x41 <= byte <= 0x5A else byte


def _hash32(bs):
    h = 0
    for byte in bs:
        h = (h * HASH_MULTIPLIER + _lower(byte)) & 0xFFFFFFFF
    return h


def _extension_tweak(ext):
    if not ext or ext[0:1] != b".":
        return 0
    if len(ext) == 4:
        a, b, c = _lower(ext[1]), _lower(ext[2]), _lower(ext[3])
        if (a, b, c) == (ord("n"), ord("i"), ord("f")):
            return 1
        if (a, b, c) == (ord("d"), ord("d"), ord("s")):
            return 3
        if (a, b, c) == (ord("w"), ord("a"), ord("v")):
            return 4
        return 0
    if len(ext) == 3 and _lower(ext[1]) == ord("k") and _lower(ext[2]) == ord("f"):
        return 2
    return 0


def _c_string(bs):
    nul = bs.find(b"\x00")
    return bs if nul < 0 else bs[:nul]


def _build_hash(name, extra):
    name = _c_string(name)
    extra = _c_string(extra)
    length = len(name)
    low = [0, 0, 0, 0]
    low[0] = _lower(name[length - 1]) if length > 0 else 0
    low[1] = _lower(name[length - 2]) if length > 2 else 0
    low[2] = length & 0xFF
    low[3] = _lower(name[0]) if length > 0 else 0
    high = 0
    if length > 3:
        count = (length - 3) & 0xFF
        high = _hash32(name[1:1 + count])
    if extra:
        high = (high + _hash32(extra[:len(extra) & 0xFF])) & 0xFFFFFFFF
    tweak = _extension_tweak(extra)
    low[3] = (low[3] + ((tweak & 0xFC) << 5)) & 0xFF
    low[0] = (low[0] + ((tweak & 0xFE) << 6)) & 0xFF
    low[1] = (low[1] + ((tweak << 7) & 0xFF)) & 0xFF
    return (low[0] | (low[1] << 8) | (low[2] << 16) | (low[3] << 24) | (high << 32))


def compute_folder_hash(path):
    return _build_hash(path, b"")


def compute_file_hash(path):
    sep = max(path.rfind(b"\\"), path.rfind(b"/"))
    name = path[sep + 1:] if sep >= 0 else path
    dot = name.rfind(b".")
    stem, extension = (name[:dot], name[dot:]) if dot >= 0 else (name, b"")
    return _build_hash(stem, extension)


def build(folders, flags):
    has_folder_names = bool(flags & FLAG_HAS_FOLDER_NAMES)
    has_file_names = bool(flags & FLAG_HAS_FILE_NAMES)

    file_count = sum(len(f["files"]) for f in folders)
    folder_name_length = sum(1 + len(f["name"]) + 1 for f in folders) if has_folder_names else 0
    file_name_length = (sum(len(fl["name"]) + 1 for f in folders for fl in f["files"])
                        if has_file_names else 0)

    records_end = HEADER_SIZE + len(folders) * FOLDER_RECORD_SIZE
    block_offsets = []
    cursor = records_end
    for f in folders:
        block_offsets.append(cursor)
        if has_folder_names:
            cursor += 1 + len(f["name"]) + 1
        cursor += len(f["files"]) * FILE_RECORD_SIZE
    name_block_offset = cursor
    data_start = name_block_offset + file_name_length

    payloads = []
    sizes = []
    for f in folders:
        for fl in f["files"]:
            payload = fl["data"]           # 非圧縮（archive 既定も toggle も無し）
            sizes.append(len(payload))
            payloads.append(payload)

    out = bytearray()
    out += u32(BSA_MAGIC)
    out += u32(OB_HEADER_VERSION)
    out += u32(HEADER_SIZE)
    out += u32(flags)
    out += u32(len(folders))
    out += u32(file_count)
    out += u32(folder_name_length)
    out += u32(file_name_length)
    out += u32(0)                          # file_flags

    for index, f in enumerate(folders):
        out += u64(compute_folder_hash(f["name"]))
        out += u32(len(f["files"]))
        out += u32(block_offsets[index] + file_name_length)

    data_cursor = data_start
    size_iter = iter(sizes)
    for f in folders:
        if has_folder_names:
            out += bytes([len(f["name"]) + 1])
            out += f["name"]
            out += b"\x00"
        for fl in f["files"]:
            size = next(size_iter)
            out += u64(compute_file_hash(fl["name"]))
            out += u32(size)
            out += u32(data_cursor)
            data_cursor += size

    if has_file_names:
        for f in folders:
            for fl in f["files"]:
                out += fl["name"]
                out += b"\x00"

    assert len(out) == data_start, (len(out), data_start)
    for payload in payloads:
        out += payload
    return bytes(out)


def build_fixture():
    folders = [{
        "name": b"meshes",
        "files": [{"name": b"test.nif", "data": make_fixture_nif.build()}],
    }]
    return build(folders, FLAG_HAS_FOLDER_NAMES | FLAG_HAS_FILE_NAMES)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    outdir = os.path.join(root, "docs", "evidence", "fixtures")
    os.makedirs(outdir, exist_ok=True)
    path = os.path.join(outdir, "minimal.bsa")
    data = build_fixture()
    with open(path, "wb") as f:
        f.write(data)
    print("wrote %s (%d bytes)" % (path, len(data)))


if __name__ == "__main__":
    main()
