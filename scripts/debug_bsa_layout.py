"""Debug the actual bytes at file record offset."""
import struct
from pathlib import Path

BSA = Path(r'D:\Cargo\Oblivion_Android\BSA\bsa_Original\Oblivion - Textures - Compressed.bsa')
with BSA.open('rb') as f:
    f.seek(0)
    hdr = f.read(36)
    magic, ver, dir_off, flags, ndir, nfiles, dnames_len, fnames_len, fflags = struct.unpack('<4s8I', hdr)

    f.seek(36 + 16 * ndir)  # 17476
    len_byte = f.read(1)[0]
    name = f.read(len_byte).decode('utf-8')
    print(f'first dir name: "{name}"')
    rec = f.read(16)
    fh, fs, fo = struct.unpack('<QII', rec)
    print(f'  file[0] hash=0x{fh:x} size=0x{fs:x} offset={fo}')
    print(f'    real_size (masked) = {fs & 0x3fffffff}')
    print(f'    compressed_bit = {bool(fs & 0x40000000)}')

    f.seek(fo)
    first32 = f.read(32)
    print(f'    first 32 bytes: {first32.hex()}')
    print(f'    first 4 bytes (size): {struct.unpack("<I", first32[:4])[0]}')
