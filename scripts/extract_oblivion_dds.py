"""
Direct BSA v103 extractor for Oblivion.
Parses the BSA header manually and extracts target DDS files,
handling zlib-compressed files with 4-byte original_size prefix.
"""
import struct
import zlib
from pathlib import Path

BSA_PATH = Path(r'D:\Cargo\Oblivion_Android\BSA\bsa_Original\Oblivion - Textures - Compressed.bsa')
OUT_DIR = Path(r'C:\Users\hiroki.kogarumai\Oblivion_Android\textures_extracted')

SIZE_MASK = 0x3fffffff
COMPRESSED_MASK = 0x40000000

# Files we want (lowercase for matching)
TARGETS = {
    'loading_background.dds',
    'tes_oblivion_logo_final.dds',
    'tes_oblivion_logo_bink.dds',
    'loading_symbol.dds',
    'load_in_game_default.dds',
    'ingamedefault.dds',
    'mainmenubackdrop.dds',
    'test_loadgamebackdrop01.dds',
    'test_loadgamebackdrop02.dds',
    'test_loadgamebackdrop03.dds',
    'test_loadgamebackdrop04.dds',
    'test_loadgamebackdrop05.dds',
    'test_loadgamebackdrop06.dds',
    'loadgamebackdrop.dds',
}

def read_cstring(block: bytes, offset: int) -> str:
    end = block.find(b'\x00', offset)
    if end == -1:
        end = len(block)
    return block[offset:end].decode('ascii', errors='replace')


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    with BSA_PATH.open('rb') as f:
        # --- Header (36 bytes) ---
        hdr = f.read(36)
        magic, ver, dir_off, flags, ndir, nfiles, dnames_len, fnames_len, fflags = struct.unpack('<4s8I', hdr)
        assert magic == b'BSA\x00', f'Not BSA: {magic}'
        assert ver == 103, f'Version {ver} != 103'

        has_dir_names = bool(flags & 0x001)
        has_file_names = bool(flags & 0x002)
        compressed_by_default = bool(flags & 0x004)
        print(f'BSA v{ver}: {ndir} dirs, {nfiles} files, compressed={compressed_by_default}')

        # --- Directory records (16 bytes each) ---
        # Note: directory_names_length is a legacy field. Construct's PascalString(VarInt)
        # already includes the name length, so no separate dir name block exists.
        dir_recs = []
        for _ in range(ndir):
            h, cnt, noff = struct.unpack('<QII', f.read(16))
            dir_recs.append({'hash': h, 'count': cnt, 'name_off': noff})

        # --- Parse directory blocks: for each dir, PascalString(VarInt) + file_records ---
        all_files = []
        for di, dr in enumerate(dir_recs):
            # VarInt length (1-byte if < 0x80, else multi-byte)
            b0 = f.read(1)[0]
            if b0 < 0x80:
                name_len = b0
            else:
                # 2-byte VarInt
                b1 = f.read(1)[0]
                name_len = (b0 & 0x7f) | (b1 << 7)

            name = f.read(name_len).decode('ascii', errors='replace').rstrip('\x00 ')
            if di < 3:
                print(f'dir[{di}] name="{name}" (len={name_len})')

            for _ in range(dr['count']):
                fh, fs, fo = struct.unpack('<QII', f.read(16))
                all_files.append({
                    'hash': fh,
                    'size': fs,
                    'offset': fo,
                    'dir_name': name,
                })

        # --- File names block ---
        fnames_block = f.read(fnames_len)
        pos = 0
        for i, af in enumerate(all_files):
            end = fnames_block.find(b'\x00', pos)
            if end == -1:
                af['name'] = f'unknown_{i}'
            else:
                af['name'] = fnames_block[pos:end].decode('ascii', errors='replace').rstrip('\x00 ')
                pos = end + 1

        print(f'Parsed {len(all_files)} file records')

        # Sanity check: print first 5 file records
        for af in all_files[:5]:
            print(f'  {af["dir_name"]}/{af.get("name", "?")} (size=0x{af["size"]:x}, off={af["offset"]})')

        # --- Extract target files ---
        extracted = 0
        seen_names = set()
        for af in all_files:
            name_lower = af['name'].lower()
            if name_lower not in TARGETS:
                continue
            if name_lower in seen_names:
                continue
            seen_names.add(name_lower)

            real_size = af['size'] & SIZE_MASK
            is_compressed = compressed_by_default != bool(af['size'] & COMPRESSED_MASK)

            f.seek(af['offset'])
            raw = f.read(real_size)

            if is_compressed and len(raw) > 4:
                orig_size = struct.unpack('<I', raw[:4])[0]
                compressed_data = raw[4:]
                try:
                    data = zlib.decompress(compressed_data)
                except zlib.error:
                    try:
                        data = zlib.decompress(compressed_data, -15)  # raw deflate
                    except zlib.error as e:
                        print(f'  [ERR] {af["dir_name"]}/{af["name"]}: {e}')
                        continue
            else:
                # Uncompressed: skip 4-byte original_size prefix if present
                if len(raw) > 4:
                    data = raw
                else:
                    data = raw

            out_path = OUT_DIR / af['dir_name'].replace('\\', '/') / af['name']
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_bytes(data)
            extracted += 1
            print(f'  [OK] {af["dir_name"]}/{af["name"]} ({len(data)} bytes)')

        print(f'\nDone: {extracted}/{len(TARGETS)} files extracted')


if __name__ == '__main__':
    main()
