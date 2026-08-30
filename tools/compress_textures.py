#!/usr/bin/env python3
"""
Texture compression utility for Oblivion Android.
Converts PNG textures to ASTC/ETC2 compressed formats.
"""

import os
import sys
import struct
import zlib
from pathlib import Path

# ASTC block sizes (width, height, bits per pixel)
ASTC_PROFILES = {
    '4x4': (4, 4, 8.0),
    '5x4': (5, 4, 6.4),
    '5x5': (5, 5, 5.12),
    '6x5': (6, 5, 4.27),
    '6x6': (6, 6, 3.56),
    '8x5': (8, 5, 3.2),
    '8x6': (8, 6, 2.67),
    '8x8': (8, 8, 2.0),
    '10x5': (10, 5, 2.56),
    '10x6': (10, 6, 2.13),
    '10x8': (10, 8, 1.6),
    '10x10': (10, 10, 1.28),
    '12x10': (12, 10, 1.07),
    '12x12': (12, 12, 0.89),
}

def create_astc_header(width, height, block_w, block_h):
    """Create ASTC file header."""
    # ASTC magic number
    magic = b'\x13\xab\xa1\x5c'
    
    # Block size
    block_size_x = block_w - 4
    block_size_y = block_h - 4
    
    # Image size (little-endian, 24-bit)
    size_x = struct.pack('<I', width)[:3]
    size_y = struct.pack('<I', height)[:3]
    size_z = struct.pack('<I', 1)[:3]  # 2D texture
    
    return magic + bytes([block_size_x, block_size_y]) + size_x + size_y + size_z

def compress_texture_astc(input_path, output_path, profile='8x8'):
    """Compress texture to ASTC format."""
    if profile not in ASTC_PROFILES:
        print(f"Unknown ASTC profile: {profile}")
        return False
    
    block_w, block_h, bpp = ASTC_PROFILES[profile]
    
    # Read PNG (simplified - would need PIL/Pillow in production)
    # For now, create placeholder
    print(f"  ASTC {profile} compression: {input_path}")
    print(f"  Block size: {block_w}x{block_h}, BPP: {bpp}")
    
    # In production, would use:
    # from PIL import Image
    # img = Image.open(input_path)
    # width, height = img.size
    # ... compress using astcenc or similar
    
    return True

def compress_texture_etc2(input_path, output_path):
    """Compress texture to ETC2 format."""
    print(f"  ETC2 compression: {input_path}")
    
    # In production, would use etc2comp or similar
    return True

def analyze_texture_sizes(assets_dir):
    """Analyze texture sizes and recommend compression profiles."""
    textures = []
    
    for root, dirs, files in os.walk(assets_dir):
        for file in files:
            if file.lower().endswith('.png'):
                path = os.path.join(root, file)
                size = os.path.getsize(path)
                rel_path = os.path.relpath(path, assets_dir)
                textures.append((rel_path, size))
    
    # Sort by size
    textures.sort(key=lambda x: -x[1])
    
    print("=== Texture Analysis ===")
    print(f"Total textures: {len(textures)}")
    print(f"Total size: {sum(s for _, s in textures) / 1024 / 1024:.2f} MB")
    print()
    
    # Recommend profiles based on size
    recommendations = {
        '4x4': [],    # Highest quality
        '6x6': [],    # Medium quality
        '8x8': [],    # Lower quality
        '10x10': [],  # Low quality
        '12x12': [],  # Lowest quality
    }
    
    for path, size in textures:
        size_mb = size / 1024 / 1024
        if size_mb > 2.0:
            recommendations['4x4'].append((path, size))
        elif size_mb > 1.0:
            recommendations['6x6'].append((path, size))
        elif size_mb > 0.5:
            recommendations['8x8'].append((path, size))
        elif size_mb > 0.1:
            recommendations['10x10'].append((path, size))
        else:
            recommendations['12x12'].append((path, size))
    
    for profile, items in recommendations.items():
        if items:
            total_size = sum(s for _, s in items)
            print(f"ASTC {profile}: {len(items)} textures, {total_size / 1024 / 1024:.2f} MB")
    
    return textures

def main():
    if len(sys.argv) < 2:
        print("Usage: python compress_textures.py <assets_dir> [--analyze]")
        sys.exit(1)
    
    assets_dir = sys.argv[1]
    
    if '--analyze' in sys.argv:
        analyze_texture_sizes(assets_dir)
    else:
        print("Texture compression utility")
        print("Use --analyze to analyze texture sizes")

if __name__ == '__main__':
    main()
