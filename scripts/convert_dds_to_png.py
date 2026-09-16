"""
Convert extracted DDS files to PNG and place into Android assets.
"""
from PIL import Image
from pathlib import Path

SRC_DIR = Path(r'C:\Users\hiroki.kogarumai\Oblivion_Android\textures_extracted\textures\menus\loading')
DST_DIR = Path(r'C:\Users\hiroki.kogarumai\Oblivion_Android\app\src\main\assets\textures\ui')

# Map source DDS to destination PNG name
CONVERT_MAP = {
    'loading_background.dds':      'loading_background.png',
    'tes_oblivion_logo_final.dds': 'tes_oblivion_logo_final.png',
    'tes_oblivion_logo_bink.dds':  'tes_oblivion_logo_bink.png',
    'loading_symbol.dds':          'loading_symbol.png',
    'load_in_game_default.dds':    'load_in_game_default.png',
}

def main():
    DST_DIR.mkdir(parents=True, exist_ok=True)

    for src_name, dst_name in CONVERT_MAP.items():
        src_path = SRC_DIR / src_name
        dst_path = DST_DIR / dst_name

        if not src_path.exists():
            print(f'[SKIP] {src_name} not found')
            continue

        try:
            img = Image.open(src_path)
            img.save(dst_path, 'PNG')
            size_kb = dst_path.stat().st_size / 1024
            print(f'[OK] {src_name} -> {dst_name}  ({img.size[0]}x{img.size[1]}, {size_kb:.0f}KB)')
        except Exception as e:
            print(f'[ERR] {src_name}: {e}')

if __name__ == '__main__':
    main()
