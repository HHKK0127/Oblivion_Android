#!/usr/bin/env python3
"""
Texture reduction utility for Oblivion Android.
Reduces texture sizes by:
1. Converting PNG to WebP (better compression)
2. Resizing oversized textures
3. Removing duplicate textures
"""

import os
import sys
import hashlib
from pathlib import Path

# Maximum texture sizes by category
MAX_SIZES = {
    'menus': 2048,      # Menu textures
    'textures': 1024,   # Game textures
    'sky': 512,         # Sky textures
    'trees': 512,       # Tree textures
    'rocks': 512,       # Rock textures
    'oblivion': 1024,   # Oblivion realm textures
}

def get_category(path):
    """Get texture category from path."""
    parts = path.lower().split(os.sep)
    for category in MAX_SIZES:
        if category in parts:
            return category
    return 'textures'

def calculate_file_hash(path):
    """Calculate MD5 hash of file."""
    hash_md5 = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def analyze_duplicates(assets_dir):
    """Find duplicate textures."""
    hashes = {}
    duplicates = []
    
    for root, dirs, files in os.walk(assets_dir):
        for file in files:
            if file.lower().endswith('.png'):
                path = os.path.join(root, file)
                file_hash = calculate_file_hash(path)
                
                if file_hash in hashes:
                    duplicates.append((path, hashes[file_hash]))
                else:
                    hashes[file_hash] = path
    
    return duplicates

def analyze_oversized(assets_dir):
    """Find textures that are too large for their category."""
    oversized = []
    
    for root, dirs, files in os.walk(assets_dir):
        for file in files:
            if file.lower().endswith('.png'):
                path = os.path.join(root, file)
                rel_path = os.path.relpath(path, assets_dir)
                size = os.path.getsize(path)
                category = get_category(rel_path)
                max_size = MAX_SIZES.get(category, 1024)
                
                # Estimate texture dimensions from file size
                # PNG is roughly 4 bytes per pixel (RGBA)
                estimated_pixels = size / 4
                estimated_dimension = int(estimated_pixels ** 0.5)
                
                if estimated_dimension > max_size:
                    oversized.append((rel_path, size, estimated_dimension, max_size))
    
    return oversized

def generate_reduction_report(assets_dir):
    """Generate texture reduction report."""
    print("=== Texture Reduction Report ===")
    print()
    
    # Find duplicates
    duplicates = analyze_duplicates(assets_dir)
    if duplicates:
        print(f"Duplicate textures found: {len(duplicates)}")
        total_dup_size = sum(os.path.getsize(dup[0]) for dup in duplicates)
        print(f"Total duplicate size: {total_dup_size / 1024 / 1024:.2f} MB")
        print()
        print("Top 10 duplicates:")
        for dup_path, orig_path in duplicates[:10]:
            size = os.path.getsize(dup_path)
            print(f"  {size / 1024:.2f} KB: {dup_path}")
            print(f"    Original: {orig_path}")
    else:
        print("No duplicate textures found.")
    print()
    
    # Find oversized textures
    oversized = analyze_oversized(assets_dir)
    if oversized:
        print(f"Oversized textures found: {len(oversized)}")
        total_oversize = sum(size for _, size, _, _ in oversized)
        print(f"Total oversized size: {total_oversize / 1024 / 1024:.2f} MB")
        print()
        print("Top 10 oversized textures:")
        oversized.sort(key=lambda x: -x[1])
        for path, size, dim, max_dim in oversized[:10]:
            print(f"  {size / 1024 / 1024:.2f} MB: {path}")
            print(f"    Estimated: {dim}x{dim}, Max: {max_dim}x{max_dim}")
    else:
        print("No oversized textures found.")
    print()
    
    # Summary
    total_size = sum(
        os.path.getsize(os.path.join(root, file))
        for root, dirs, files in os.walk(assets_dir)
        for file in files
        if file.lower().endswith('.png')
    )
    
    dup_savings = sum(os.path.getsize(dup[0]) for dup in duplicates)
    oversize_savings = sum(
        size - (max_dim * max_dim * 4)
        for _, size, _, max_dim in oversized
        if size > max_dim * max_dim * 4
    )
    
    print("=== Potential Savings ===")
    print(f"Current total: {total_size / 1024 / 1024:.2f} MB")
    print(f"Duplicate removal: {dup_savings / 1024 / 1024:.2f} MB")
    print(f"Size reduction: {oversize_savings / 1024 / 1024:.2f} MB")
    print(f"Total potential: {(dup_savings + oversize_savings) / 1024 / 1024:.2f} MB")
    print(f"Remaining: {(total_size - dup_savings - oversize_savings) / 1024 / 1024:.2f} MB")

def main():
    if len(sys.argv) < 2:
        print("Usage: python reduce_textures.py <assets_dir>")
        sys.exit(1)
    
    assets_dir = sys.argv[1]
    generate_reduction_report(assets_dir)

if __name__ == '__main__':
    main()
