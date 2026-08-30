#!/usr/bin/env python3
"""
Resize oversized textures for mobile devices.
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

# Maximum texture sizes by category
MAX_SIZES = {
    'menus': 2048,
    'textures': 1024,
    'sky': 512,
    'trees': 512,
    'rocks': 512,
    'oblivion': 1024,
    'clutter': 512,
    'architecture': 1024,
    'characters': 1024,
    'weapons': 512,
    'armor': 512,
    'effects': 512,
}

def get_category(path):
    """Get texture category from path."""
    parts = path.lower().split(os.sep)
    for category in MAX_SIZES:
        if category in parts:
            return category
    return 'textures'

def resize_texture(input_path, output_path, max_size):
    """Resize texture to maximum size."""
    if not HAS_PIL:
        return False
    
    try:
        img = Image.open(input_path)
        width, height = img.size
        
        # Check if resize is needed
        if width <= max_size and height <= max_size:
            return False
        
        # Calculate new size maintaining aspect ratio
        ratio = min(max_size / width, max_size / height)
        new_width = int(width * ratio)
        new_height = int(height * ratio)
        
        # Resize with high quality
        img_resized = img.resize((new_width, new_height), Image.LANCZOS)
        
        # Save
        img_resized.save(output_path, 'PNG', optimize=True)
        return True
    except Exception as e:
        print(f"Error resizing {input_path}: {e}")
        return False

def batch_resize(assets_dir, dry_run=True):
    """Batch resize textures."""
    resized = []
    total_saved = 0
    
    for root, dirs, files in os.walk(assets_dir):
        for file in files:
            if file.lower().endswith('.png'):
                input_path = os.path.join(root, file)
                rel_path = os.path.relpath(input_path, assets_dir)
                size = os.path.getsize(input_path)
                category = get_category(rel_path)
                max_size = MAX_SIZES.get(category, 1024)
                
                # Check if resize is needed (estimate from file size)
                # PNG is roughly 4 bytes per pixel (RGBA)
                estimated_pixels = size / 4
                estimated_dimension = int(estimated_pixels ** 0.5)
                
                if estimated_dimension > max_size:
                    # Calculate expected savings
                    ratio = max_size / estimated_dimension
                    new_size = int(size * ratio * ratio)
                    saved = size - new_size
                    total_saved += saved
                    
                    resized.append((rel_path, size, new_size, saved, max_size))
    
    return resized, total_saved

def main():
    if len(sys.argv) < 2:
        print("Usage: python resize_textures.py <assets_dir> [--execute]")
        sys.exit(1)
    
    assets_dir = sys.argv[1]
    dry_run = '--execute' not in sys.argv
    
    print("=== Texture Resize Analysis ===")
    print(f"Directory: {assets_dir}")
    print(f"Mode: {'Execute' if not dry_run else 'Dry Run'}")
    print()
    
    resized, total_saved = batch_resize(assets_dir, dry_run)
    
    print(f"Textures to resize: {len(resized)}")
    print(f"Total savings: {total_saved / 1024 / 1024:.2f} MB")
    print()
    
    if resized:
        print("Top 15 textures to resize:")
        resized.sort(key=lambda x: -x[3])
        for path, orig_size, new_size, saved, max_dim in resized[:15]:
            print(f"  {saved / 1024:.2f} KB saved: {path}")
            print(f"    {orig_size / 1024:.2f} KB -> {new_size / 1024:.2f} KB (max: {max_dim})")
    
    # Summary by category
    print()
    print("=== Savings by Category ===")
    category_savings = {}
    for path, orig_size, new_size, saved, max_dim in resized:
        category = get_category(path)
        if category not in category_savings:
            category_savings[category] = {'count': 0, 'saved': 0}
        category_savings[category]['count'] += 1
        category_savings[category]['saved'] += saved
    
    for category, data in sorted(category_savings.items(), key=lambda x: -x[1]['saved']):
        print(f"  {category}: {data['count']} textures, {data['saved'] / 1024 / 1024:.2f} MB")

if __name__ == '__main__':
    main()
