"""
Debug: check the raw bytes at file_record offset to understand BSA v103 compressed format.
"""
from pathlib import Path
import struct
import zlib

BSA_PATH = Path(r'D:\Cargo\Oblivion_Android\BSA\bsa_Original\Oblivion - Textures - Compressed.bsa')

with BSA_PATH.open('rb') as f:
    # Read 36-byte header (BSA v103)
    hdr = f.read(36)
    magic, ver, dir_off, flags, ndir, nfiles, dnames_len, fnames_len, fflags = struct.unpack('<4s8I', hdr)
    print(f'magic={magic} ver={ver} dir_off={dir_off} flags=0x{flags:x}')
    print(f'ndir={ndir} nfiles={nfiles}')
    print(f'compressed_by_default = {(flags & 0x004) != 0}')

    # Skip to dir_off
    f.seek(dir_off)
    # Read directory records: hash(8) + file_count(4) + name_offset(4) = 16 bytes each
    dirs = []
    for i in range(ndir):
        rec = f.read(16)
        h, cnt, noff = struct.unpack('<QII', rec)
        dirs.append((h, cnt, noff))

    # Now read directory name block (length dnames_len)
    dnames_block_start = f.tell()
    print(f'\nDirectory name block starts at {dnames_block_start}, length {dnames_len}')

    # The actual format: name_block has names terminated by \x00
    # Each directory's name_offset is relative to this block start
    name_block = f.read(dnames_len)
    dir_names = []
    pos = 0
    for h, cnt, noff in dirs:
        end = name_block.find(b'\x00', noff)
        name = name_block[noff:end].decode('ascii', errors='ignore')
        dir_names.append(name)

    # Print first 5 directory names
    for i in range(5):
        print(f'  dir[{i}]={dir_names[i]} count={dirs[i][1]}')

    # Now we're after the directory name block
    file_records_start = f.tell()
    print(f'\nFile records start at {file_records_start}')

    # Skip file records (16 bytes each)
    f.seek(file_records_start + 16 * nfiles)

    # File names block
    fnames_block_start = f.tell()
    print(f'File names block starts at {fnames_block_start}, length {fnames_len}')

    # Read all file names
    fnames_block = f.read(fnames_len)
    file_names = []
    pos = 0
    while pos < len(fnames_block):
        end = fnames_block.find(b'\x00', pos)
        if end == -1:
            break
        file_names.append(fnames_block[pos:end].decode('ascii', errors='ignore'))
        pos = end + 1
    print(f'Parsed {len(file_names)} file names (expected {nfiles})')

    # Now find a small file (e.g., loading_symbol.dds) to inspect
    target = None
    for i, dn in enumerate(dir_names):
        if dn.lower() == 'textures\\menus\\loading':
            # Get file records for this directory
            print(f'\nFound loading dir at index {i}')
            for j in range(dirs[i][1]):
                rec_off = file_records_start + 16 * (sum(dirs[k][1] for k in range(i)) + j)
                f.seek(rec_off)
                h, sz, off = struct.unpack('<QII', f.read(16))
                # Get the file name (need to find the right index in file_names)
                # File names are in directory order
                file_idx = sum(dirs[k][1] for k in range(i)) + j
                fname = file_names[file_idx] if file_idx < len(file_names) else f'unknown_{j}'
                print(f'  {fname}: size=0x{sz:x} offset={off}')
                if fname.lower() == 'loading_symbol.dds':
                    target = (off, sz & 0x3fffffff, fname, 'compressed' if sz & 0x40000000 else 'uncompressed')
                if fname.lower() == 'loading_background.dds':
                    target = (off, sz & 0x3fffffff, fname, 'compressed' if sz & 0x40000000 else 'uncompressed')

            break

    if target is None:
        # just use first file
        f.seek(file_records_start)
        h, sz, off = struct.unpack('<QII', f.read(16))
        target = (off, sz & 0x3fffffff, 'first', 'compressed' if sz & 0x40000000 else 'uncompressed')

    print(f'\nInspecting {target[2]} at offset={target[0]} real_size={target[1]} state={target[3]}')

    # Read the raw data
    f.seek(target[0])
    raw = f.read(target[1] + 32)
    print(f'First 32 bytes (hex): {raw[:32].hex()}')
    print(f'First 4 bytes (ascii): {raw[:4]}')

    # Try different decompressions
    # Method 1: skip 4 bytes (original_size), zlib decompress the rest
    if target[3] == 'compressed' and len(raw) > 4:
        try:
            dec = zlib.decompress(raw[4:])
            print(f'\nzlib.decompress after skipping 4 bytes: OK, got {len(dec)} bytes')
            print(f'  magic: {dec[:4]}')
        except zlib.error as e:
            print(f'\nzlib.decompress fail: {e}')

        # Method 2: try raw deflate
        try:
            dec = zlib.decompress(raw[4:], -15)
            print(f'\nraw deflate after skipping 4 bytes: OK, got {len(dec)} bytes')
            print(f'  magic: {dec[:4]}')
        except zlib.error as e:
            print(f'\nraw deflate fail: {e}')

        # Method 3: read original_size from first 4 bytes
        if len(raw) >= 4:
            orig_sz = struct.unpack('<I', raw[:4])[0]
            print(f'\noriginal_size in first 4 bytes: {orig_sz}')
