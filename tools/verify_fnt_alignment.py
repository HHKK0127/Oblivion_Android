import struct
import sys

fonts_dir = 'D:/Cargo/Other/Oblivion/BSA/bsa_Extraction/fonts/'

# Load all fonts
fonts = {}
for fn in ['kingthings_regular', 'kingthings_shadowed', 'handwritten', 'tahoma_bold_small', 'daedric_font']:
    with open(fonts_dir + fn + '.fnt', 'rb') as f:
        fonts[fn] = f.read()

# Check: is there an 8-byte preamble between header(76) and glyph records(56)?
# If glyph records start at 352 (= 344 + 8), then 14632-352 = 14280 = 255*56 EXACT
print("=== Checking preamble at offset 344-352 ===")
for fn in ['kingthings_regular', 'daedric_font']:
    data = fonts[fn]
    preamble = struct.unpack_from('<2f', data, 344)
    print(f"  {fn}: offset 344 = {preamble[0]:.6f}, offset 348 = {preamble[1]:.6f}")

# Check if the 8 bytes at 344-351 are a count or version field
print()
print("=== Hex dump 340-360 ===")
data = fonts['kingthings_regular']
for i in range(340, 360, 4):
    v = struct.unpack_from('<f', data, i)[0]
    print(f"  +{i}: {data[i]:02X} {data[i+1]:02X} {data[i+2]:02X} {data[i+3]:02X}  float={v:.6f}")

# Now verify: does rec0 at offset 352 make more sense than at 344?
print()
print("=== kingthings_regular: records at offset 352 (56-byte stride) ===")
for rec in range(31, 36):
    off = 352 + rec * 56
    v = struct.unpack_from('<14f', data, off)
    ch = chr(rec) if 32 <= rec < 127 else f'#{rec}'
    has_uv = v[3] != 0.0
    print(f"  rec{rec:03d} ({ch:>3s}) +{off:05d}: [{v[0]:7.2f},{v[1]:7.2f},{v[2]:7.2f}] UV=({v[3]:.4f},{v[4]:.4f},{v[5]:.4f},{v[6]:.4f},{v[7]:.4f},{v[8]:.4f},{v[9]:.4f},{v[10]:.4f}) w={v[11]:5.1f} h={v[12]:5.1f} [{v[13]:.2f}]")

# Compare: does offset 344 rec32 match or offset 352 rec32?
print()
print("=== Compare: space glyph (rec32) at different offsets ===")
for start_off in [344, 352]:
    off = start_off + 32 * 56
    v = struct.unpack_from('<14f', data, off)
    print(f"  start={start_off} rec32 +{off}: {['%.4f'%x for x in v]}")
