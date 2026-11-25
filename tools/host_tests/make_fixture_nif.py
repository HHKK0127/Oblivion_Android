#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_fixture_nif.py - docs/evidence/fixtures/minimal.nif を決定論的に生成する。

レイアウトの出所: rust/crates/nif-core/src/lib.rs の受理済みテスト
`shape_data_decodes_vertices_colors_uvs_and_triangles`（Gamebryo 20.0.0.5、
NiTriShape + NiTriShapeData の 2 ブロック）が組むバイト列を **逐語で踏襲**する。
ここではそれを **コミットするフィクスチャ** として固定し、
scripts/run_fixture_gates.sh が「生成器の再実行で差分が出ないこと」と
「実パーサが同じブロック数・頂点を返すこと」を CI で見る。

範囲は **nif-core が読む最小のブロック列（ヘッダ + 1 メッシュ）** のみ。
シェーダ・スキン・アニメは対象外（shape_body の shader=0 / skin=0xFFFFFFFF）。

時刻・乱数・絶対パスを使わない（再実行しても同一バイト列）。
"""
import os
import struct

MAGIC = "Gamebryo File Format, Version 20.0.0.5"
VERSION = 0x1400_0005        # 20.0.0.5
USER_VERSION = 11            # テストと同じ
BS_VERSION = 11              # テストと同じ（ヘッダの unknown フィールド）


def u16(v):
    return struct.pack("<H", v & 0xFFFF)


def u32(v):
    return struct.pack("<I", v & 0xFFFFFFFF)


def f32(v):
    return struct.pack("<f", v)


def sized(text):
    """u32 length + bytes (nif-core の sized())."""
    b = text.encode("ascii")
    return u32(len(b)) + b


def bz(text):
    """BzString: u8 length counting the terminating NUL, then the body."""
    b = text.encode("ascii")
    return bytes([len(b) + 1]) + b + b"\x00"


def magic_line(text):
    return text.encode("ascii") + b"\n"


def gamebryo_header(magic, version, user_version, unknown, type_names, type_indices):
    out = bytearray()
    out += magic_line(magic)
    out += u32(version)
    if version >= 0x1400_0000:
        out += b"\x01"          # endian byte
    out += u32(user_version)
    out += u32(len(type_indices))
    out += u32(unknown)
    out += bz("ccafasso")
    out += bz("")
    out += bz("")
    out += u16(len(type_names))
    for name in type_names:
        out += sized(name)
    for index in type_indices:
        out += u16(index)
    out += u32(0)               # numGroups
    return bytes(out)


def shape_body(name, data_ref, property_refs):
    """NiTriShape / NiTriStrips scene block (transform は固定: T=(1,2,3), I, scale 2)."""
    body = bytearray()
    body += sized(name)
    body += u32(0)              # extra data count
    body += u32(0xFFFFFFFF)     # controller
    body += u16(0)              # flags
    for value in (1.0, 2.0, 3.0):
        body += f32(value)      # translation
    for value in (1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0):
        body += f32(value)      # rotation (identity)
    body += f32(2.0)            # scale
    body += u32(len(property_refs))
    for ref in property_refs:
        body += u32(ref)
    body += u32(0xFFFFFFFF)     # collision object
    body += u32(data_ref)
    body += u32(0xFFFFFFFF)     # skin instance
    body += b"\x00"             # no shader
    return bytes(body)


def shape_data_body(vertices, normals, colors, uvs, triangles):
    """NiTriShapeData: shared geometry header + triangle tail."""
    body = bytearray()
    body += u32(0)              # group id (since 10.1.0.114)
    body += u16(len(vertices))
    body += b"\x00"             # keep flags
    body += b"\x00"             # compress flags
    body += b"\x01"             # has vertices
    for v in vertices:
        for value in v:
            body += f32(value)
    body += u16(1 if uvs else 0)   # data flags: one UV set
    body += bytes([1 if normals else 0])
    if normals:
        for n in normals:
            for value in n:
                body += f32(value)
    body += bytes(16)           # bounding sphere
    body += bytes([1 if colors else 0])
    if colors:
        for c in colors:
            for value in c:
                body += f32(value)
    if uvs:
        for t in uvs:
            for value in t:
                body += f32(value)
    body += u16(0)              # consistency flags
    body += u32(0)              # additional data (since 20.0.0.4)
    body += u16(len(triangles))
    body += u32(len(triangles) * 3)   # num triangle points
    body += b"\x01"             # has triangles (since 10.1.0.0)
    for tri in triangles:
        for index in tri:
            body += u16(index)
    body += u16(0)              # match groups
    return bytes(body)


def build():
    header = gamebryo_header(
        MAGIC, VERSION, USER_VERSION, BS_VERSION,
        ["NiTriShape", "NiTriShapeData"], [0, 1],
    )
    out = bytearray(header)
    out += shape_body("Tri", 1, [])
    out += shape_data_body(
        [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
        [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]],
        [[1.0, 0.0, 0.0, 1.0], [0.0, 1.0, 0.0, 1.0], [0.0, 0.0, 1.0, 0.5]],
        [[0.0, 0.0], [1.0, 0.0], [0.0, 1.0]],
        [[0, 1, 2]],
    )
    out += u32(1)               # trailer value count
    out += u32(0)               # trailer value
    return bytes(out)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    outdir = os.path.join(root, "docs", "evidence", "fixtures")
    os.makedirs(outdir, exist_ok=True)
    path = os.path.join(outdir, "minimal.nif")
    data = build()
    with open(path, "wb") as f:
        f.write(data)
    print("wrote %s (%d bytes)" % (path, len(data)))


if __name__ == "__main__":
    main()
