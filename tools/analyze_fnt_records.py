import struct
import sys

fonts_dir = 'D:/Cargo/Other/Oblivion/BSA/bsa_Extraction/fonts/'
fname = sys.argv[1] if len(sys.argv) > 1 else 'kingthings_regular'

with open(fonts_dir + fname + '.fnt', 'rb') as f:
    data = f.read()

# 255 records at offset 344, stride 56 bytes
# Fields: [bearing_x, advance, padding, u0, v0, u1, v0, u0, v1, u1, v1, px_w, px_h, padding]

print(f'=== {fname} ASCII 32-126 glyph data ===')
for rec in range(32, 127):
    off = 344 + rec * 56
    v = struct.unpack_from('<14f', data, off)
    has_uv = v[3] != 0.0
    ch = chr(rec)
    if has_uv:
        print(f'  [{rec:3d} {ch:1s}] bearing={v[0]:6.1f} adv={v[1]:6.1f} w={v[11]:5.1f} h={v[12]:5.1f} UV=({v[3]:.4f},{v[4]:.4f},{v[5]:.4f},{v[7]:.4f})')
    elif v[0] != 0.0 or v[1] != 0.0:
        print(f'  [{rec:3d} {ch:1s}] bearing={v[0]:6.1f} adv={v[1]:6.1f} (no UV)')

print()
# Also dump daedric
fname2 = 'daedric_font'
with open(fonts_dir + fname2 + '.fnt', 'rb') as f:
    data2 = f.read()
print(f'=== {fname2} ASCII 32-126 glyph data ===')
for rec in range(32, 127):
    off = 344 + rec * 56
    v = struct.unpack_from('<14f', data2, off)
    has_uv = v[3] != 0.0
    ch = chr(rec)
    if has_uv:
        print(f'  [{rec:3d} {ch:1s}] bearing={v[0]:6.1f} adv={v[1]:6.1f} w={v[11]:5.1f} h={v[12]:5.1f} UV=({v[3]:.4f},{v[4]:.4f},{v[5]:.4f},{v[7]:.4f})')

print()
print('=== All 5 fonts: record count with UV ===')
for fn in ['kingthings_regular', 'kingthings_shadowed', 'handwritten', 'tahoma_bold_small', 'daedric_font']:
    with open(fonts_dir + fn + '.fnt', 'rb') as f:
        d = f.read()
    count = sum(1 for r in range(255) if struct.unpack_from('<f', d, 344 + r * 56 + 12)[0] != 0.0)
    first = next((r for r in range(32, 256) if struct.unpack_from('<f', d, 344 + r * 56 + 12)[0] != 0.0), -1)
    last = next((r for r in range(254, 31, -1) if struct.unpack_from('<f', d, 344 + r * 56 + 12)[0] != 0.0), -1)
    print(f'  {fn}: {count} glyphs, ASCII range {first}-{last}')
