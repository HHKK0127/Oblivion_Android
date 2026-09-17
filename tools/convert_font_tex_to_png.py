#!/usr/bin/env python3
"""
Convert Oblivion .tex font textures to PNG format.

.tex format (Oblivion NiPixelData):
  - uint32 width
  - uint32 height
  - uint8[width*height*4] RGBA8 pixel data

Also parses .fnt glyph definitions and outputs a JSON atlas descriptor.
"""

import struct
import sys
import os
import json

def read_tex(filepath):
    """Parse .tex file, return (width, height, rgba_bytes)."""
    with open(filepath, 'rb') as f:
        data = f.read()
    width = struct.unpack_from('<I', data, 0)[0]
    height = struct.unpack_from('<I', data, 4)[0]
    pixel_data = data[8:]
    expected = width * height * 4
    if len(pixel_data) != expected:
        print(f"WARNING: {filepath}: expected {expected} pixel bytes, got {len(pixel_data)}")
    return width, height, pixel_data

def read_fnt(filepath):
    """Parse .fnt file, return dict with font info and glyph list."""
    with open(filepath, 'rb') as f:
        data = f.read()

    # Header: float fontSize, uint32, uint32, char[64] textureName
    font_size = struct.unpack_from('<f', data, 0)[0]

    # Texture name (null-terminated, 64 bytes starting at offset 12)
    tex_name_raw = data[12:76]
    tex_name = tex_name_raw.split(b'\x00')[0].decode('ascii', errors='replace')

    # Glyph records: 56 bytes each (14 floats), starting at offset 76
    # Structure per record:
    #   +00: u0, v0, u1, v1       (16 bytes) - primary UV rect
    #   +16: u0b, v0b, u1b, v1b   (16 bytes) - shadow/highlight UV rect
    #   +32: width, height         (8 bytes)  - pixel dimensions
    #   +40: bearing_x, bearing_y  (8 bytes)  - bearing offsets
    #   +48: advance               (4 bytes)  - horizontal advance
    #   +52: padding               (4 bytes)
    GLYPH_SIZE = 56
    header_size = 76
    glyph_count = (len(data) - header_size) // GLYPH_SIZE

    glyphs = []
    for i in range(glyph_count):
        off = header_size + i * GLYPH_SIZE
        fvals = struct.unpack_from('<14f', data, off)

        # Primary UV rect
        u0 = fvals[0]
        v0 = fvals[1]
        u1 = fvals[2]
        v1 = fvals[3]

        # Shadow UV rect
        u0b = fvals[4]
        v0b = fvals[5]
        u1b = fvals[6]
        v1b = fvals[7]

        # Pixel metrics
        width = fvals[8]
        height = fvals[9]
        bearing_x = fvals[10]
        bearing_y = fvals[11]
        advance = fvals[12]

        # Valid glyph: width and height must be reasonable pixel values
        # Some glyphs have v0==v1 in UV (height=0 in UV) but valid pixel height
        if width >= 1.0 and height >= 1.0 and width <= 200.0 and height <= 200.0:
            glyphs.append({
                'index': i,
                'width': width,
                'height': height,
                'bearing_x': bearing_x,
                'bearing_y': bearing_y,
                'advance': advance,
                'u0': u0, 'v0': v0, 'u1': u1, 'v1': v1,
                'u0b': u0b, 'v0b': v0b, 'u1b': u1b, 'v1b': v1b,
            })

    return {
        'font_size': font_size,
        'tex_name': tex_name,
        'glyph_count': glyph_count,
        'valid_glyphs': len(glyphs),
        'glyphs': glyphs,
    }

def tex_to_png(tex_path, png_path):
    """Convert .tex to .png using PIL."""
    try:
        from PIL import Image
    except ImportError:
        print("ERROR: Pillow not installed. Run: pip install Pillow")
        return False

    width, height, rgba = read_tex(tex_path)
    img = Image.frombytes('RGBA', (width, height), rgba)
    img.save(png_path)
    print(f"  {os.path.basename(tex_path)} -> {os.path.basename(png_path)}: {width}x{height}, {os.path.getsize(png_path)} bytes")
    return True

def main():
    src_dir = sys.argv[1] if len(sys.argv) > 1 else r"D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\fonts"
    dst_dir = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "app", "src", "main", "assets", "fonts"
    )

    os.makedirs(dst_dir, exist_ok=True)

    tex_files = [f for f in os.listdir(src_dir) if f.endswith('.tex')]
    fnt_files = [f for f in os.listdir(src_dir) if f.endswith('.fnt')]

    print(f"Source: {src_dir}")
    print(f"Dest:   {dst_dir}")
    print()

    # Convert .tex -> .png
    print("=== TEX -> PNG Conversion ===")
    for tex_name in sorted(tex_files):
        tex_path = os.path.join(src_dir, tex_name)
        png_name = tex_name.replace('.tex', '.png')
        png_path = os.path.join(dst_dir, png_name)
        tex_to_png(tex_path, png_path)

    # Parse .fnt -> .json
    print("\n=== FNT Analysis ===")
    all_fonts = {}
    for fnt_name in sorted(fnt_files):
        fnt_path = os.path.join(src_dir, fnt_name)
        info = read_fnt(fnt_path)
        font_key = fnt_name.replace('.fnt', '')
        all_fonts[font_key] = info

        print(f"\n{fnt_name}:")
        print(f"  Font size: {info['font_size']}")
        print(f"  Texture: {info['tex_name']}")
        print(f"  Total glyph slots: {info['glyph_count']}")
        print(f"  Valid glyphs: {info['valid_glyphs']}")

        if info['glyphs']:
            g = info['glyphs'][0]
            print(f"  First glyph: index={g['index']}, {g['width']}x{g['height']}, "
                  f"advance={g['advance']}, UV=({g['u0']:.4f},{g['v0']:.4f})-({g['u1']:.4f},{g['v1']:.4f})")

    # Save atlas descriptor
    atlas_path = os.path.join(dst_dir, "font_atlas.json")
    with open(atlas_path, 'w', encoding='utf-8') as f:
        json.dump(all_fonts, f, indent=2, ensure_ascii=False)
    print(f"\nAtlas descriptor saved: {atlas_path}")

if __name__ == '__main__':
    main()
