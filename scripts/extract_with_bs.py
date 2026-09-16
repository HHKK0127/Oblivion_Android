"""
Extract Oblivion menu DDS files using bethesda-structs library.
Falls back to raw deflate when zlib fails.
"""
import sys
import zlib
from pathlib import Path, PureWindowsPath

from bethesda_structs.archive.bsa import BSAArchive

# Add a deflate fallback at module level by monkey-patching
import construct
_orig_decode = construct.Compressed._decode
def _patched_decode(self, obj, context, path):
    try:
        return _orig_decode(self, obj, context, path)
    except zlib.error:
        return zlib.decompress(obj, -15)  # raw deflate fallback
construct.Compressed._decode = _patched_decode

BSA_PATH = Path(r'D:\Cargo\Oblivion_Android\BSA\bsa_Original\Oblivion - Textures - Compressed.bsa')
OUT_DIR  = Path(r'C:\Users\hiroki.kogarumai\Oblivion_Android\textures_extracted')

# Files to extract (lowercase compare)
TARGETS = [
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
]
TARGETS_LOWER = [t.lower() for t in TARGETS]

def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    print(f'Loading BSA: {BSA_PATH.name}')
    archive = BSAArchive.parse_file(BSA_PATH)
    print(f'  files_compressed={archive.container.header.archive_flags.files_compressed}')

    found = 0
    total = 0
    errors = 0
    for f in archive.iter_files():
        total += 1
        name = f.filepath.name.lower()
        if name in TARGETS_LOWER:
            try:
                rel = str(f.filepath).replace('\\', '/')
                out_path = OUT_DIR / rel
                out_path.parent.mkdir(parents=True, exist_ok=True)
                out_path.write_bytes(f.data)
                print(f'  [OK] {rel} ({len(f.data)} bytes)')
                found += 1
            except Exception as e:
                print(f'  [ERR] {f.filepath}: {e}')
                errors += 1
        if total % 2000 == 0:
            print(f'  scanned {total} files...')
    print(f'\nDone. found={found}, errors={errors}, scanned={total}')


if __name__ == '__main__':
    main()
