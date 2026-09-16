"""Manual zlib decompression test."""
import struct
import zlib
from pathlib import Path

BSA = Path(r'D:\Cargo\Oblivion_Android\BSA\bsa_Original\Oblivion - Textures - Compressed.bsa')
with BSA.open('rb') as f:
    # Read header
    hdr = f.read(36)
    magic, ver, dir_off, flags, ndir, nfiles, dnames_len, fnames_len, fflags = struct.unpack('<4s8I', hdr)

    SIZE_MASK = 0x3fffffff
    COMPRESSED_MASK = 0xc0000000

    # Skip to first dir block
    f.seek(36 + 16 * ndir)
    len_byte = f.read(1)[0]
    name = f.read(len_byte).decode('utf-8')
    rec = f.read(16)
    fh, fs, fo = struct.unpack('<QII', rec)
    
    real_size = fs & SIZE_MASK
    has_compressed_bit = bool(fs & COMPRESSED_MASK)
    compressed_by_default = bool(flags & 0x004)
    
    print(f'Dir: {name.strip(chr(0))}')
    print(f'  size=0x{fs:x} real_size={real_size} compressed_bit={has_compressed_bit}')
    print(f'  offset={fo} compressed_by_default={compressed_by_default}')
    
    # In bethesda-structs:
    # if archive_flags.files_compressed != (size & COMPRESSED_MASK): use compressed
    # files_compressed=True, COMPRESSED_MASK & size = 0 -> True != False -> use compressed
    
    use_compressed = (compressed_by_default != has_compressed_bit)
    print(f'  use_compressed = {use_compressed}')
    
    # Read data
    f.seek(fo)
    data = f.read(real_size)
    print(f'  Read {len(data)} bytes')
    print(f'  First 8: {data[:8].hex()}')
    
    if use_compressed:
        # compressed_file_struct: original_size(4) + Compressed(GreedyBytes, "zlib")
        orig_size = struct.unpack('<I', data[:4])[0]
        compressed_data = data[4:]
        print(f'  original_size={orig_size}, compressed_data_len={len(compressed_data)}')
        
        # Try standard zlib
        try:
            dec = zlib.decompress(compressed_data)
            print(f'  zlib OK: {len(dec)} bytes')
            print(f'  First 4: {dec[:4]}')
        except zlib.error as e:
            print(f'  zlib FAIL: {e}')
        
        # Try raw deflate
        try:
            dec = zlib.decompress(compressed_data, -15)
            print(f'  raw deflate OK: {len(dec)} bytes')
        except zlib.error as e:
            print(f'  raw deflate FAIL: {e}')
        
        # Try decompressobj
        try:
            d = zlib.decompressobj()
            dec = d.decompress(compressed_data)
            print(f'  decompressobj OK: {len(dec)} bytes')
        except zlib.error as e:
            print(f'  decompressobj FAIL: {e}')
    else:
        # Uncompressed - first 4 bytes = original_size
        print(f'  First 4 (as uint32): {struct.unpack("<I", data[:4])[0]}')
