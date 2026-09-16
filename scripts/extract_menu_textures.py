"""
Extract menu textures from Oblivion BSA files.
Extracts DDS files referenced by menus/options/main_menu.xml and other menu XMLs.
"""
import struct
import os
import sys
import zlib

def read_bsa_header(f):
    """Read BSA header. Returns dict with header fields."""
    header = f.read(36)
    magic = header[:4]
    if magic != b'BSA\x00':
        raise ValueError(f"Not a BSA file (magic: {magic})")
    version, archive_flags, folder_count, file_count, \
        folder_names_len, file_names_len, file_flags = struct.unpack_from('<7I', header, 4)
    return {
        'version': version,
        'archive_flags': archive_flags,
        'folder_count': folder_count,
        'file_count': file_count,
        'folder_names_len': folder_names_len,
        'file_names_len': file_names_len,
        'file_flags': file_flags,
    }

def read_bsa_folders(f, header):
    """Read folder records from BSA."""
    folders = []
    for _ in range(header['folder_count']):
        data = f.read(16)
        name_hash, file_count, offset = struct.unpack_from('<QIH', data[:14])
        # offset is 4 bytes, but the struct is Q (8) + I (4) + I (4) = 16
        # Actually: hash (8) + file_count (4) + offset (4) = 16 bytes
        folders.append({
            'file_count': file_count,
            'offset': offset,
        })
    return folders

def extract_loading_textures(bsa_path, output_dir):
    """Extract DDS files from Oblivion BSA using direct byte parsing."""
    os.makedirs(output_dir, exist_ok=True)

    with open(bsa_path, 'rb') as f:
        header = read_bsa_header(f)
        print(f"BSA version: {header['version']}")
        print(f"Archive flags: {hex(header['archive_flags'])}")
        print(f"File flags: {hex(header['file_flags'])}")
        print(f"Folders: {header['folder_count']}, Files: {header['file_count']}")

        has_folder_names = (header['archive_flags'] & 0x01) != 0
        has_file_names = (header['archive_flags'] & 0x02) != 0
        compressed_by_default = (header['archive_flags'] & 0x04) != 0
        print(f"Has folder names: {has_folder_names}, file names: {has_file_names}, compressed: {compressed_by_default}")

        # Read folder records (each is 16 bytes for Oblivion v103)
        folders = []
        for _ in range(header['folder_count']):
            data = f.read(16)
            name_hash, file_count, offset = struct.unpack_from('<QII', data)
            folders.append({'hash': name_hash, 'file_count': file_count, 'offset': offset})

        # Target DDS filenames
        target_lower = {
            'loading_background.dds',
            'tes_oblivion_logo_final.dds',
            'tes_oblivion_logo_bink.dds',
            'loading_symbol.dds',
            'load_in_game_default.dds',
            'ingamedefault.dds',
            'loading_save_lines.dds',
            'loading_save_center_fold.dds',
            'loading_save_wide_frame.dds',
            'mainmenubackdrop.dds',
        }

        extracted = 0

        for fi, folder in enumerate(folders):
            # Read folder name
            folder_name = ""
            if has_folder_names:
                name_bytes = b''
                while True:
                    b = f.read(1)
                    if b == b'\x00' or len(b) == 0:
                        break
                    name_bytes += b
                folder_name = name_bytes.decode('ascii', errors='ignore')

            # Read file records for this folder (each is 16 bytes)
            file_records = []
            for _ in range(folder['file_count']):
                data = f.read(16)
                name_hash, size, offset = struct.unpack_from('<QII', data)
                file_records.append({'hash': name_hash, 'size': size, 'offset': offset})

            # Read file names if present
            if has_file_names:
                for fr in file_records:
                    name_bytes = b''
                    while True:
                        b = f.read(1)
                        if b == b'\x00' or len(b) == 0:
                            break
                        name_bytes += b
                    fr['name'] = name_bytes.decode('ascii', errors='ignore')

            # Check if any file matches our targets
            for fr in file_records:
                fname = fr.get('name', '').lower()
                if fname not in target_lower:
                    continue

                saved_pos = f.tell()
                try:
                    f.seek(fr['offset'])
                    raw_data = f.read(fr['size'])
                    file_data = raw_data
                    is_compressed = compressed_by_default
                    if fr['size'] & 0xC0000000:
                        is_compressed = not is_compressed
                    actual_size = fr['size'] & 0x3FFFFFFF

                    if is_compressed and len(raw_data) > 4:
                        orig_size = struct.unpack_from('<I', raw_data[:4])[0]
                        try:
                            file_data = zlib.decompress(raw_data[4:])
                        except Exception:
                            file_data = raw_data[4:]

                    out_subdir = os.path.join(output_dir, folder_name.replace('\\', '/'))
                    os.makedirs(out_subdir, exist_ok=True)
                    out_path = os.path.join(out_subdir, fr['name'])
                    with open(out_path, 'wb') as out_f:
                        out_f.write(file_data)
                    extracted += 1
                    print(f"Extracted: {folder_name}/{fr['name']} ({len(file_data)} bytes)")
                except Exception as e:
                    print(f"Error extracting {fr.get('name', '?')}: {e}")
                finally:
                    f.seek(saved_pos)

            if (fi + 1) % 200 == 0:
                print(f"Processed {fi+1}/{header['folder_count']} folders...")

        print(f"\nDone! Extracted {extracted} files to {output_dir}")

if __name__ == '__main__':
    bsa_path = r'D:\Cargo\Oblivion_Android\BSA\Oblivion - Textures.bsa'
    output_dir = r'C:\Users\hiroki.kogarumai\Oblivion_Android\textures_extracted'
    
    if not os.path.exists(bsa_path):
        print(f"BSA not found: {bsa_path}")
        sys.exit(1)
    
    extract_loading_textures(bsa_path, output_dir)
