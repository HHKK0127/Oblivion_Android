#!/usr/bin/env python3
"""
Convert PNG textures to WebP format for better compression.
Requires: Pillow (pip install Pillow)
"""

import os
import sys
from pathlib import Path

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Warning: Pillow not installed. Install with: pip install Pillow")

def convert_png_to_webp(input_path, output_path, quality=80):
    """Convert PNG to WebP."""
    if not HAS_PIL:
        return False
    
    try:
        img = Image.open(input_path)
        img.save(output_path, 'WEBP', quality=quality)
        return True
    except Exception as e:
        print(f"Error converting {input_path}: {e}")
        return False

def batch_convert(assets_dir, quality=80, dry_run=True):
    """Batch convert PNG to WebP."""
    converted = []
    failed = []
    total_saved = 0
    
    for root, dirs, files in os.walk(assets_dir):
        for file in files:
            if file.lower().endswith('.png'):
                input_path = os.path.join(root, file)
                output_path = os.path.join(root, file[:-4] + '.webp')
                
                input_size = os.path.getsize(input_path)
                
                if dry_run:
                    # Estimate WebP size (typically 25-35% smaller)
                    estimated_size = int(input_size * 0.7)
                    saved = input_size - estimated_size
                    total_saved += saved
                    converted.append((input_path, input_size, estimated_size, saved))
                else:
                    if convert_png_to_webp(input_path, output_path, quality):
                        output_size = os.path.getsize(output_path)
                        saved = input_size - output_size
                        total_saved += saved
                        converted.append((input_path, input_size, output_size, saved))
                    else:
                        failed.append(input_path)
    
    return converted, failed, total_saved

def main():
    if len(sys.argv) < 2:
        print("Usage: python convert_to_webp.py <assets_dir> [--execute] [--quality=80]")
        sys.exit(1)
    
    assets_dir = sys.argv[1]
    dry_run = '--execute' not in sys.argv
    quality = 80
    
    for arg in sys.argv:
        if arg.startswith('--quality='):
            quality = int(arg.split('=')[1])
    
    print("=== PNG to WebP Conversion ===")
    print(f"Directory: {assets_dir}")
    print(f"Quality: {quality}")
    print(f"Mode: {'Execute' if not dry_run else 'Dry Run'}")
    print()
    
    converted, failed, total_saved = batch_convert(assets_dir, quality, dry_run)
    
    print(f"Textures processed: {len(converted)}")
    print(f"Failed: {len(failed)}")
    print(f"Total saved: {total_saved / 1024 / 1024:.2f} MB")
    print()
    
    if converted:
        print("Top 10 conversions by savings:")
        converted.sort(key=lambda x: -x[3])
        for path, orig_size, new_size, saved in converted[:10]:
            print(f"  {saved / 1024:.2f} KB saved: {os.path.basename(path)}")
            print(f"    {orig_size / 1024:.2f} KB -> {new_size / 1024:.2f} KB")
    
    if failed:
        print(f"\nFailed conversions: {len(failed)}")
        for path in failed[:5]:
            print(f"  {path}")

if __name__ == '__main__':
    main()
