#!/usr/bin/env python3
"""
extract_vegetation.py - Extract tree placement data from ESM files
Phase 51: SpeedTree vegetation system

Extracts LAND records and generates tree placement data for the
SpeedTreeManager to use at runtime.

Usage:
    python extract_vegetation.py <esm_file> [--output <output_dir>] [--density <float>]

Output:
    - vegetation_data.json: Tree positions, types, and metadata
    - tree_types.json: Tree type definitions from ESM TREE records
"""

import argparse
import json
import os
import struct
import sys
import math
import random
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ============================================================================
# ESM Record Types
# ============================================================================

# Oblivion ESM record type constants
RECORD_LAND = b'LAND'
RECORD_TREE = b'TREE'
RECORD_CELL = b'CELL'
RECORD_WRLD = b'WRLD'

# LAND sub-record types
LAND_VHGT = b'VHGT'  # Vertex height data
LAND_VCLR = b'VCLR'  # Vertex colors
LAND_VTXT = b'VTXT'  # Texture layers


class EsmVegetationExtractor:
    """Extracts vegetation-related data from Oblivion ESM files."""

    def __init__(self, esm_path: str, density: float = 0.5, seed: int = 42):
        self.esm_path = esm_path
        self.density = density
        self.seed = seed
        self.tree_types: Dict[int, dict] = {}
        self.land_records: List[dict] = []
        self.rng = random.Random(seed)

    def extract(self) -> Tuple[List[dict], List[dict]]:
        """Extract tree types and land data from ESM file."""
        print(f"Extracting vegetation data from: {self.esm_path}")

        try:
            with open(self.esm_path, 'rb') as f:
                self._parse_esm(f)
        except FileNotFoundError:
            print(f"Warning: ESM file not found: {self.esm_path}")
            print("Generating placeholder vegetation data...")
            self._generate_placeholder_data()

        # Generate tree placements from land data
        tree_placements = self._generate_placements()

        return list(self.tree_types.values()), tree_placements

    def _parse_esm(self, f):
        """Parse ESM file for TREE and LAND records."""
        # Read TES4 header
        header = f.read(4)
        if header != b'TES4':
            print(f"Warning: Not a valid ESM file (header: {header})")
            self._generate_placeholder_data()
            return

        # Skip header size and flags
        f.read(8)

        # Parse records
        while True:
            record_type = f.read(4)
            if len(record_type) < 4:
                break

            record_size_bytes = f.read(4)
            if len(record_size_bytes) < 4:
                break

            record_size = struct.unpack('<I', record_size_bytes)[0]

            # Skip flags
            f.read(4)

            if record_type == RECORD_TREE:
                self._parse_tree_record(f, record_size - 12)
            elif record_type == RECORD_LAND:
                self._parse_land_record(f, record_size - 12)
            else:
                # Skip unknown records
                f.seek(record_size - 12, 1)

    def _parse_tree_record(self, f, data_size: int):
        """Parse a TREE record for tree type data."""
        data = f.read(data_size)
        if len(data) < data_size:
            return

        # Extract EDID (editor ID) and NAME (model path)
        tree_type = {
            'typeId': len(self.tree_types) + 1,
            'editorId': '',
            'modelPath': '',
            'minHeight': 3.0,
            'maxHeight': 12.0,
            'billboardWidth': 4.0,
            'billboardHeight': 8.0,
        }

        # Simple sub-record parsing
        offset = 0
        while offset < len(data) - 4:
            sub_type = data[offset:offset + 4]
            sub_size = struct.unpack('<H', data[offset + 4:offset + 6])[0] if offset + 6 <= len(data) else 0
            offset += 6

            if offset + sub_size > len(data):
                break

            if sub_type == b'EDID':
                tree_type['editorId'] = data[offset:offset + sub_size].rstrip(b'\x00').decode('utf-8', errors='replace')
            elif sub_type == b'MODL':
                tree_type['modelPath'] = data[offset:offset + sub_size].rstrip(b'\x00').decode('utf-8', errors='replace')

            offset += sub_size

        if tree_type['editorId'] or tree_type['modelPath']:
            self.tree_types[tree_type['typeId']] = tree_type

    def _parse_land_record(self, f, data_size: int):
        """Parse a LAND record for heightmap data."""
        data = f.read(data_size)
        if len(data) < data_size:
            return

        land = {
            'cellX': 0,
            'cellY': 0,
            'heightmap': [[0.0] * 33 for _ in range(33)],
            'hasData': True,
        }

        # Parse sub-records for cell coordinates and height data
        offset = 0
        while offset < len(data) - 4:
            sub_type = data[offset:offset + 4]
            sub_size_bytes = data[offset + 4:offset + 6]
            if len(sub_size_bytes) < 2:
                break
            sub_size = struct.unpack('<H', sub_size_bytes)[0]
            offset += 6

            if offset + sub_size > len(data):
                break

            if sub_type == b'DATA':
                # Cell coordinates (int32 x, int32 y)
                if sub_size >= 8:
                    cx, cy = struct.unpack('<ii', data[offset:offset + 8])
                    land['cellX'] = cx
                    land['cellY'] = cy

            offset += sub_size

        self.land_records.append(land)

    def _generate_placeholder_data(self):
        """Generate placeholder data when ESM is not available."""
        # Create default tree types
        default_types = [
            {'typeId': 1, 'editorId': 'TreeOak01', 'modelPath': 'meshes/trees/oak01.nif',
             'minHeight': 5.0, 'maxHeight': 12.0, 'billboardWidth': 5.0, 'billboardHeight': 10.0},
            {'typeId': 2, 'editorId': 'TreePine01', 'modelPath': 'meshes/trees/pine01.nif',
             'minHeight': 8.0, 'maxHeight': 18.0, 'billboardWidth': 4.0, 'billboardHeight': 14.0},
            {'typeId': 3, 'editorId': 'TreeMaple01', 'modelPath': 'meshes/trees/maple01.nif',
             'minHeight': 4.0, 'maxHeight': 10.0, 'billboardWidth': 6.0, 'billboardHeight': 9.0},
            {'typeId': 4, 'editorId': 'TreeElm01', 'modelPath': 'meshes/trees/elm01.nif',
             'minHeight': 6.0, 'maxHeight': 14.0, 'billboardWidth': 5.0, 'billboardHeight': 12.0},
            {'typeId': 5, 'editorId': 'Bush01', 'modelPath': 'meshes/trees/bush01.nif',
             'minHeight': 1.0, 'maxHeight': 3.0, 'billboardWidth': 2.0, 'billboardHeight': 2.5},
        ]

        for t in default_types:
            self.tree_types[t['typeId']] = t

        # Create placeholder land cells (3x3 area around origin)
        for cy in range(-1, 2):
            for cx in range(-1, 2):
                land = {
                    'cellX': cx,
                    'cellY': cy,
                    'heightmap': [[0.0] * 33 for _ in range(33)],
                    'hasData': True,
                }

                # Generate procedural heightmap
                for gy in range(33):
                    for gx in range(33):
                        wx = cx * 4096.0 + (gx / 32.0) * 4096.0
                        wy = cy * 4096.0 + (gy / 32.0) * 4096.0
                        h = math.sin(wx * 0.001) * 20.0 + math.cos(wy * 0.001) * 15.0
                        h += math.sin(wx * 0.005 + wy * 0.003) * 5.0
                        land['heightmap'][gy][gx] = h

                self.land_records.append(land)

    def _generate_placements(self) -> List[dict]:
        """Generate tree placements from land data."""
        placements = []

        for land in self.land_records:
            if not land['hasData']:
                continue

            cell_trees = self._place_trees_in_cell(land)
            placements.extend(cell_trees)

        print(f"Generated {len(placements)} tree placements")
        return placements

    def _place_trees_in_cell(self, land: dict) -> List[dict]:
        """Place trees within a single LAND cell."""
        trees = []
        cell_size = 4096.0
        world_x_base = land['cellX'] * cell_size
        world_z_base = land['cellY'] * cell_size

        # Number of candidate positions
        area = cell_size * cell_size
        num_candidates = max(1, min(int(area * self.density / 10000.0 * 10), 500))

        for i in range(num_candidates):
            # Random position within cell
            local_x = self.rng.random() * cell_size
            local_z = self.rng.random() * cell_size

            world_x = world_x_base + local_x
            world_z = world_z_base + local_z

            # Density noise check
            density_val = self._density_noise(world_x, world_z)
            if self.rng.random() > density_val:
                continue

            # Sample height from heightmap
            grid_x = (local_x / cell_size) * 32.0
            grid_z = (local_z / cell_size) * 32.0
            gx = min(int(grid_x), 31)
            gz = min(int(grid_z), 31)
            height = land['heightmap'][gz][gx]

            # Select tree type
            type_ids = list(self.tree_types.keys())
            if not type_ids:
                continue
            tree_type_id = self.rng.choice(type_ids)

            # Random rotation and scale
            rotation = self.rng.random() * 2.0 * math.pi
            scale = 0.7 + self.rng.random() * 0.6

            trees.append({
                'typeId': tree_type_id,
                'positionX': world_x,
                'positionY': height,
                'positionZ': world_z,
                'rotation': rotation,
                'scale': scale,
            })

        return trees

    def _density_noise(self, x: float, z: float) -> float:
        """Simple noise function for density variation."""
        n = math.sin(x * 0.001 + self.seed) * 0.3 + math.cos(z * 0.001 + self.seed) * 0.3
        n += math.sin(x * 0.005 + z * 0.003) * 0.2
        return max(0.0, min(1.0, (n + 0.8) * 0.5))


def main():
    parser = argparse.ArgumentParser(
        description='Extract vegetation data from Oblivion ESM files')
    parser.add_argument('esm_file', help='Path to ESM file')
    parser.add_argument('--output', '-o', default='./vegetation_output',
                        help='Output directory (default: ./vegetation_output)')
    parser.add_argument('--density', '-d', type=float, default=0.5,
                        help='Tree density multiplier (default: 0.5)')
    parser.add_argument('--seed', '-s', type=int, default=42,
                        help='Random seed for reproducible generation (default: 42)')

    args = parser.parse_args()

    # Create output directory
    os.makedirs(args.output, exist_ok=True)

    # Extract vegetation data
    extractor = EsmVegetationExtractor(args.esm_file, args.density, args.seed)
    tree_types, placements = extractor.extract()

    # Write tree types
    types_path = os.path.join(args.output, 'tree_types.json')
    with open(types_path, 'w') as f:
        json.dump(tree_types, f, indent=2)
    print(f"Wrote {len(tree_types)} tree types to {types_path}")

    # Write vegetation placements
    data_path = os.path.join(args.output, 'vegetation_data.json')
    with open(data_path, 'w') as f:
        json.dump({
            'version': 1,
            'density': args.density,
            'seed': args.seed,
            'treeCount': len(placements),
            'trees': placements,
        }, f, indent=2)
    print(f"Wrote {len(placements)} tree placements to {data_path}")

    # Summary
    print(f"\n=== Vegetation Extraction Summary ===")
    print(f"Tree types: {len(tree_types)}")
    print(f"Tree placements: {len(placements)}")
    print(f"Density: {args.density}")
    print(f"Seed: {args.seed}")
    print(f"Output: {args.output}")


if __name__ == '__main__':
    main()
