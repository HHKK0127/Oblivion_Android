#!/usr/bin/env python3
"""
Generate Oblivion app icons in multiple resolutions
Creates a simple Oblivion-themed icon (red and gold flame)
"""

from PIL import Image, ImageDraw
import os
import math

def create_oblivion_icon(size):
    """Create an Oblivion-themed icon"""
    # Create a new image with transparent background
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Color scheme: Red and Gold
    gold = (255, 215, 0, 255)
    red = (220, 20, 20, 255)
    dark_red = (150, 0, 0, 255)

    # Draw outer circle (background)
    margin = size * 0.1
    circle_bbox = [margin, margin, size - margin, size - margin]
    draw.ellipse(circle_bbox, fill=(30, 30, 30, 255), outline=gold, width=2)

    # Draw flame-like symbol (stylized Oblivion portal)
    center_x = size / 2
    center_y = size / 2

    # Draw concentric circles to create a portal effect
    for i in range(3, 0, -1):
        radius = (size * 0.25) * (i / 3)
        r_int = int(radius)
        bbox = [
            int(center_x - r_int),
            int(center_y - r_int),
            int(center_x + r_int),
            int(center_y + r_int)
        ]

        if i == 3:
            draw.ellipse(bbox, outline=red, width=max(1, int(size * 0.05)))
        elif i == 2:
            draw.ellipse(bbox, outline=gold, width=max(1, int(size * 0.04)))
        else:
            draw.ellipse(bbox, fill=red)

    # Draw flame points (triangular shapes)
    flame_height = size * 0.3
    flame_width = size * 0.15

    # Top flame
    points_top = [
        (center_x, center_y - flame_height),
        (center_x - flame_width, center_y - flame_height * 0.5),
        (center_x + flame_width, center_y - flame_height * 0.5)
    ]
    draw.polygon(points_top, fill=gold, outline=red)

    # Side flames (left and right)
    angle_offset = math.pi / 3  # 60 degrees
    for angle_mult in [-1, 1]:
        angle = angle_mult * angle_offset
        x = center_x + math.sin(angle) * flame_height
        y = center_y - math.cos(angle) * flame_height

        x2 = center_x + math.sin(angle + 0.5) * flame_width
        y2 = center_y - math.cos(angle + 0.5) * flame_width

        x3 = center_x + math.sin(angle - 0.5) * flame_width
        y3 = center_y - math.cos(angle - 0.5) * flame_width

        points_side = [(x, y), (x2, y2), (x3, y3)]
        draw.polygon(points_side, fill=gold, outline=red)

    return img

def generate_all_sizes():
    """Generate icons for all required Android sizes"""
    sizes = {
        'ldpi': 36,      # 120 dpi
        'mdpi': 48,      # 160 dpi
        'hdpi': 72,      # 240 dpi
        'xhdpi': 96,     # 320 dpi
        'xxhdpi': 144,   # 480 dpi
        'xxxhdpi': 192   # 640 dpi
    }

    base_path = r'C:\Users\E1192\Projects\oblivion-android\app\src\main\res'

    print("[*] Oblivion Icon Generator")
    print("=" * 50)

    for dpi, size in sizes.items():
        # Create icon
        icon = create_oblivion_icon(size)

        # Create directory if it doesn't exist
        mipmap_dir = os.path.join(base_path, f'mipmap-{dpi}')
        os.makedirs(mipmap_dir, exist_ok=True)

        # Save icon
        icon_path = os.path.join(mipmap_dir, 'ic_launcher.png')
        icon.save(icon_path, 'PNG')

        print(f"[OK] {dpi:8} ({size:3}x{size:3}) -> {icon_path}")

    # Also create a round version for newer Android
    print("\n[*] Creating round versions...")

    for dpi, size in sizes.items():
        # Create round icon
        icon = create_oblivion_icon(size)

        # Add rounded corners
        mask = Image.new('L', (size, size), 0)
        mask_draw = ImageDraw.Draw(mask)
        mask_draw.ellipse([0, 0, size, size], fill=255)

        # Apply mask
        icon.putalpha(mask)

        # Save round icon
        mipmap_dir = os.path.join(base_path, f'mipmap-{dpi}')
        icon_path = os.path.join(mipmap_dir, 'ic_launcher_round.png')
        icon.save(icon_path, 'PNG')

        print(f"[OK] {dpi:8} (round)    -> {icon_path}")

    print("\n" + "=" * 50)
    print("[SUCCESS] Icon generation complete!")
    print("[INFO] Icons saved to app/src/main/res/mipmap-*")

if __name__ == '__main__':
    try:
        generate_all_sizes()
    except Exception as e:
        print(f"[ERROR] {e}")
        import traceback
        traceback.print_exc()
