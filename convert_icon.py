#!/usr/bin/env python3
"""
Convert a single icon image to Android icon format for all DPI levels
"""

from PIL import Image
import os

def resize_icon_for_android(source_path, output_base_dir):
    """
    Resize the source icon to all Android DPI levels

    DPI levels:
    - ldpi: 36x36
    - mdpi: 48x48
    - hdpi: 72x72
    - xhdpi: 96x96
    - xxhdpi: 144x144
    - xxxhdpi: 192x192
    """

    sizes = {
        'ldpi': 36,
        'mdpi': 48,
        'hdpi': 72,
        'xhdpi': 96,
        'xxhdpi': 144,
        'xxxhdpi': 192
    }

    # Open source icon
    try:
        icon = Image.open(source_path)
        print(f"[INFO] Source icon loaded: {source_path}")
        print(f"[INFO] Original size: {icon.size}")
    except Exception as e:
        print(f"[ERROR] Failed to open icon: {e}")
        return False

    # Convert to RGBA if not already
    if icon.mode != 'RGBA':
        icon = icon.convert('RGBA')

    # Generate resized versions for each DPI
    print("[*] Generating icon versions for all DPI levels...")
    print("=" * 50)

    for dpi, size in sizes.items():
        # Resize icon using high-quality resampling
        resized = icon.resize((size, size), Image.Resampling.LANCZOS)

        # Create output directory
        mipmap_dir = os.path.join(output_base_dir, f'mipmap-{dpi}')
        os.makedirs(mipmap_dir, exist_ok=True)

        # Save icon
        icon_path = os.path.join(mipmap_dir, 'ic_launcher.png')
        resized.save(icon_path, 'PNG')
        print(f"[OK] {dpi:8} ({size:3}x{size:3}) -> {icon_path}")

    # Create round versions
    print("\n[*] Creating round icon versions...")
    print("=" * 50)

    for dpi, size in sizes.items():
        # Resize icon
        resized = icon.resize((size, size), Image.Resampling.LANCZOS)

        # Create circular mask
        mask = Image.new('L', (size, size), 0)
        mask_draw = __import__('PIL.ImageDraw', fromlist=['ImageDraw']).Draw(mask)
        mask_draw.ellipse([0, 0, size, size], fill=255)

        # Apply mask
        resized.putalpha(mask)

        # Save round icon
        mipmap_dir = os.path.join(output_base_dir, f'mipmap-{dpi}')
        icon_path = os.path.join(mipmap_dir, 'ic_launcher_round.png')
        resized.save(icon_path, 'PNG')
        print(f"[OK] {dpi:8} (round)    -> {icon_path}")

    print("\n" + "=" * 50)
    print("[SUCCESS] Icon conversion complete!")
    print("[INFO] Icons saved to app/src/main/res/mipmap-*")
    return True

if __name__ == '__main__':
    source_icon = "C:\\Users\\E1192\\Desktop\\icon.png"
    output_dir = "app\\src\\main\\res"

    if not os.path.exists(source_icon):
        print(f"[ERROR] Source icon not found: {source_icon}")
        exit(1)

    success = resize_icon_for_android(source_icon, output_dir)
    exit(0 if success else 1)
