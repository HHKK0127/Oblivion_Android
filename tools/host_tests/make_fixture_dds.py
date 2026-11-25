#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_fixture_dds.py - docs/evidence/fixtures/minimal.dds を決定論的に生成する。

レイアウトの出所: app/src/main/cpp/tests/dds_upload_tests.cpp:27-57
（ホストスイートが同じバイト列を組んで DDS パーサに通している＝受理される形）。
ここではそれを **コミットするフィクスチャ** として固定し、
scripts/run_fixture_gates.sh が「生成器の再実行で差分が出ないこと」を CI で見る。

「ヘッダのみ」ではなく **最小の 1 レベル（16x16 DXT1 = 128B）を含む**。ヘッダだけでは
ローダがレベルを持たず読み込みに失敗するため、受理される最小構成にしている。

時刻・乱数・絶対パスを使わない（再実行しても同一バイト列）。
"""
import os
import struct

DDS_MAGIC = 0x20534444          # 'DDS '
DDPF_FOURCC = 0x4
FOURCC_DXT1 = 0x31545844        # 'DXT1'
DDSCAPS_TEXTURE = 0x1000

HEADER = 128                    # magic(4) + dwSize(124)
LEVEL = 128                     # 16x16 DXT1 = 8 bytes/block * (4/4 blocks) * ... = 128B


def build():
    data = bytearray(HEADER + LEVEL)

    def u32(off, v):
        struct.pack_into("<I", data, off, v & 0xFFFFFFFF)

    u32(0, DDS_MAGIC)
    u32(4, 124)                 # dwSize
    u32(8, 0x21007)             # CAPS|HEIGHT|WIDTH|PIXELFORMAT|MIPMAPCOUNT
    u32(12, 16)                 # dwHeight
    u32(16, 16)                 # dwWidth
    u32(20, 128)                # dwPitchOrLinearSize
    u32(28, 1)                  # dwMipMapCount

    pf = 76                     # DDS_PIXELFORMAT offset
    u32(pf + 0, 32)             # dwSize
    u32(pf + 4, DDPF_FOURCC)
    u32(pf + 8, FOURCC_DXT1)
    u32(108, DDSCAPS_TEXTURE)

    # 決定論的な非ゼロのブロック本体（パーサの圧縮データ経路を通す）。
    for i in range(HEADER, len(data)):
        data[i] = (i - HEADER) & 0xFF

    return bytes(data)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    outdir = os.path.join(root, "docs", "evidence", "fixtures")
    os.makedirs(outdir, exist_ok=True)
    path = os.path.join(outdir, "minimal.dds")
    data = build()
    with open(path, "wb") as f:
        f.write(data)
    print("wrote %s (%d bytes)" % (path, len(data)))


if __name__ == "__main__":
    main()
