"""
Direct Oblivion BSA v103 extractor for menu DDS files.
Handles zlib-compressed BSA properly.
"""
import struct
import os
import sys
import zlib

def read_c_string(f):
    """Read a null-terminated ASCII string from the file."""
    name_bytes = b''
    while True:
        b = f.read(1)
        if not b or b == b'\x00':
            break
        name_bytes += b
    return name_bytes.decode('ascii', errors='ignore')


def extract_oblivion_bsa(bsa_path, output_dir, target_filenames):
    """Extract specific files from an Oblivion v103 BSA."""
    os.makedirs(output_dir, exist_ok=True)

    with open(bsa_path, 'rb') as f:
        # Header (36 bytes)
        header = f.read(36)
        magic = header[:4]
        if magic != b'BSA\x00':
            raise ValueError(f"Not a BSA file (magic: {magic})")
        version, archive_flags, folder_count, file_count, \
            folder_names_len, file_names_len, file_flags = struct.unpack('<7I', header, 4)

        print(f"BSA version: {version}")
        print(f"Archive flags: {hex(archive_flags)}")
        print(f"Folders: {folder_count}, Files: {file_count}")

        has_folder_names = (archive_flags & 0x01) != 0
        has_file_names = (archive_flags & 0x02) != 0
        compressed_by_default = (archive_flags & 0x04) != 0
        print(f"  folder names: {has_folder_names}, file names: {has_file_names}, compressed: {compressed_by_default}")

        target_lower = {n.lower() for n in target_filenames}
        extracted = 0
        matches_found = []

        # Read folder records
        folders = []
        for i in range(folder_count):
            data = f.read(16)
            name_hash, num_files, folder_offset = struct.unpack('<QII', data)
            folders.append({'hash': name_hash, 'num_files': num_files, 'offset': folder_offset})

        # Read folder name block
        folder_name_block_start = f.tell()
        folder_names = []
        if has_folder_names:
            for folder in folders:
                folder['name'] = read_c_string(f)

        # File records and names
        file_records_all = []
        for folder in folders:
            folder['files'] = []
            for i in range(folder['num_files']):
                data = f.read(16)
                file_hash, file_size, file_offset = struct.unpack('<QII', data)
                folder['files'].append({
                    'hash': file_hash,
                    'size': file_size,
                    'offset': file_offset,
                    'folder': folder.get('name', ''),
                })

        if has_file_names:
            for folder in folders:
                for fr in folder['files']:
                    fr['name'] = read_c_string(f)

        # Process file records
        all_files = []
        for folder in folders:
            all_files.extend(folder.get('files', []))

        print(f"\nTotal file records: {len(all_files)}")
        print(f"Searching for {len(target_lower)} target filenames...")

        # Match files
        for fr in all_files:
            name = fr.get('name', '')
            if name.lower() in target_lower:
                matches_found.append(fr)
                print(f"  Match: {fr['folder']}/{name}")

        # Extract
        for fr in matches_found:
            try:
                saved_pos = f.tell()
                f.seek(fr['offset'])
                raw_data = f.read(fr['size'])
                file_data = raw_data

                # Check compression flag in upper bits
                is_compressed = compressed_by_default
                if fr['size'] & 0x40000000:
                    is_compressed = True

                if is_compressed and len(raw_data) > 4:
                    try:
                        file_data = zlib.decompress(raw_data[4:])
                    except zlib.error as e:
                        print(f"    zlib fail: {e}, trying raw")
                        file_data = raw_data[4:]
                elif is_compressed:
                    file_data = raw_data

                # Build output path
                folder_path = fr['folder'].replace('\\', '/')
                if folder_path:
                    out_dir = os.path.join(output_dir, folder_path)
                else:
                    out_dir = output_dir
                os.makedirs(out_dir, exist_ok=True)

                out_path = os.path.join(out_dir, fr['name'])
                with open(out_path, 'wb') as out_f:
                    out_f.write(file_data)

                extracted += 1
                print(f"  Extracted: {fr['folder']}/{fr['name']} ({len(file_data)} bytes)")

                f.seek(saved_pos)
            except Exception as e:
                print(f"  Error extracting {fr.get('name', '?')}: {e}")

        print(f"\nExtracted {extracted}/{len(matches_found)} files")
        return extracted


if __name__ == '__main__':
    bsa_path = r'D:\Cargo\Oblivion_Android\BSA\bsa_Original\Oblivion - Textures - Compressed.bsa'
    output_dir = r'C:\Users\hiroki.kogarumai\Oblivion_Android\textures_extracted'

    target_files = [
        'loading_background.dds',
        'tes_oblivion_logo_final.dds',
        'tes_oblivion_logo_bink.dds',
        'loading_symbol.dds',
        'load_in_game_default.dds',
        'InGameDefault.dds',
        'MainMenuBackdrop.dds',
    ]

    extract_oblivion_bsa(bsa_path, output_dir, target_files)
