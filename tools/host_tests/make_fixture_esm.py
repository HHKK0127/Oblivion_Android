#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_fixture_esm.py - docs/evidence/fixtures/minimal.esm を決定論的に生成する。

newgame host smoke (tools/host_tests/newgame_smoke.cpp) 用の最小 ESM:
  TES4 ヘッダ + RACE(2) + CLAS(1) + BSGN(1) の GRUP。

RACE は playable bit と skill bonus (DATA) / spell (SPLO) / 属性 (ATTR) を持つ。
時刻・乱数・絶対パスを使わない（再実行しても同一バイト列）。

レイアウト（RE v664 実測）:
  record  = type(4) + size(4) + flags(4) + formID(4) + vc(4) + payload
  sub     = tag(4) + size(2) + payload
  GRUP    = "GRUP" + size(4, header 20 を含む) + label(4) + type(4) + stamp(2) + unknown(2) + body
  RACE DATA (36B) = 7 x (int8 skillAV, int8 bonus) + 2 reserved + 4 float + int32 flags(bit0=playable)
  RACE ATTR (16B) = 8 male u8 + 8 female u8 (AttrIndex 順 Strength..Luck, V-664-2)
"""
import os
import struct

# --- record / sub / group builders -----------------------------------------


def u32(v):
    return struct.pack("<I", v & 0xFFFFFFFF)


def sub(tag, payload):
    return tag.encode("ascii") + struct.pack("<H", len(payload)) + payload


def zstr(s):
    return s.encode("utf-8") + b"\x00"


def rec(tag, formid, payload, flags=0):
    return (tag.encode("ascii") + u32(len(payload)) + u32(flags) +
            u32(formid) + u32(0) + payload)


def grup(label, body):
    return (b"GRUP" + u32(len(body) + 20) + label.encode("ascii") + u32(0) +
            b"\x00\x00\x00\x00" + body)


# --- records ----------------------------------------------------------------


def race_data(skills, playable):
    """RACE DATA (36B): 7 x (int8 skillAV, int8 bonus) + 2 reserved + 4 float + int32 flags."""
    b = bytearray()
    for av, bonus in skills:
        b += bytes([av & 0xFF, bonus & 0xFF])
    b += b"\x00\x00"
    b += struct.pack("<4f", 1.0, 1.0, 1.0, 1.0)
    b += u32(1 if playable else 0)
    assert len(b) == 36, len(b)
    return bytes(b)


def attr_payload(male, female):
    """RACE ATTR (16B): 8 male u8 then 8 female u8, AttrIndex order Strength..Luck."""
    b = bytes(male) + bytes(female)
    assert len(b) == 16, len(b)
    return bytes(b)


def race(formid, edid, full, desc, skills, spells, playable=True, attr_mf=None):
    p = sub("EDID", zstr(edid)) + sub("FULL", zstr(full)) + sub("DESC", zstr(desc))
    if attr_mf is not None:
        p += sub("ATTR", attr_payload(*attr_mf))
    for s in spells:
        p += sub("SPLO", u32(s))
    p += sub("DATA", race_data(skills, playable))
    return rec("RACE", formid, p)


def clas(formid, edid, full):
    return rec("CLAS", formid, sub("EDID", zstr(edid)) + sub("FULL", zstr(full)))


def bsgn(formid, edid, full):
    return rec("BSGN", formid, sub("EDID", zstr(edid)) + sub("FULL", zstr(full)))


def build():
    # TestRaceA: Blade(0x0E)+10, HeavyArmor(0x12)+5, 2 racial spells. playable.
    # ATTR male 1..8 / female 11..18: all eight differ, so the host smoke golden
    # pins the ATTR mapping 1:1 (distinct values expose += / field swap / offset
    # / male-female inversion; a duplicate-heavy row like Imperial's would not).
    race_a = race(0x00000010, "TestRaceA", "Test Race A", "A test race (A).",
                  [(0x0E, 10), (0x12, 5)] + [(0xFF, 0)] * 5,
                  [0x00000100, 0x00000101], True,
                  attr_mf=([1, 2, 3, 4, 5, 6, 7, 8], [11, 12, 13, 14, 15, 16, 17, 18]))
    # TestRaceB: Armorer(0x0C)+7, 1 racial spell. playable.
    race_b = race(0x00000011, "TestRaceB", "Test Race B", "A test race (B).",
                  [(0x0C, 7)] + [(0xFF, 0)] * 6,
                  [0x00000102], True)
    # TestRaceC: not playable (bit0=0) - must be filtered out.
    race_c = race(0x00000012, "TestRaceC", "Test Race C", "Not playable.",
                  [(0x0E, 1)] + [(0xFF, 0)] * 6, [], False)

    out = rec("TES4", 0, b"")
    out += grup("RACE", race_a + race_b + race_c)
    out += grup("CLAS", clas(0x00000020, "TestClass", "Test Class"))
    out += grup("BSGN", bsgn(0x00000030, "TestBirthsign", "Test Birthsign"))
    return out


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    outdir = os.path.join(root, "docs", "evidence", "fixtures")
    os.makedirs(outdir, exist_ok=True)
    path = os.path.join(outdir, "minimal.esm")
    data = build()
    with open(path, "wb") as f:
        f.write(data)
    print("wrote %s (%d bytes)" % (path, len(data)))


if __name__ == "__main__":
    main()
