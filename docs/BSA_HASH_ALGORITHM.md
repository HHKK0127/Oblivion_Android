# BSA File Hash Algorithm

> **Source**: Reverse engineering of `Oblivion.exe` (Steam version, IMAGE_BASE = 0x400000)
> **Verification**: 17/17 BSA archives, 147,629/147,629 file records, 2,567/2,567 folder records — 100% match
> **See also**: `tmp/re/s76_file_hash_analysis.md` (full analysis with disassembly evidence)

## 1. Hash Functions

### 1.1 `hash32` (0x6FA040)

```cpp
uint32_t hash32(const char* s, uint8_t n) {
    uint32_t h = 0;
    for (uint8_t i = 0; i < n; ++i) {
        h = h * 0x1003F + tolower(s[i]);
    }
    return h;
}
```

- Multiplicand: `0x1003F`
- Character folding: `tolower` (ASCII only; non-ASCII untested)
- Length parameter is `uint8_t` — values > 255 wrap to `n & 0xFF`

### 1.2 `build` (0x6FA080)

```cpp
// Builds a 64-bit hash from a name and optional extension.
// out[0..7] is a 32-byte scratch buffer; only bytes 0..4 are used.
void build(const char* name, const char* extra, uint8_t out[32]) {
    size_t n = strlen(name);
    out[0] = (n > 0) ? tolower(name[n - 1]) : 0;       // last char
    out[1] = (n > 2) ? tolower(name[n - 2]) : 0;        // second-to-last char
    out[2] = n & 0xFF;                                   // length
    out[3] = (n > 0) ? tolower(name[0]) : 0;             // first char

    uint32_t h = 0;
    if (n > 3) h = hash32(name + 1, (n - 3) & 0xFF);    // middle rolling hash
    if (extra) h += hash32(extra, strlen(extra) & 0xFF); // extension hash

    // Extension tweak table (0xB25374)
    // k is determined by case-insensitive comparison of `extra`:
    //   0: ""      (no extension)     — no tweak
    //   1: ".NIF"  ext == ".nif"      — out[1] += 0x80
    //   2: ".KF"   ext == ".kf"       — out[0] += 0x80
    //   3: ".DDS"  ext == ".dds"      — out[0] += 0x80, out[1] += 0x80
    //   4: ".WAV"  ext == ".wav"      — out[3] += 0x80
    int k = tweak_index(extra);  // 0..4
    out[3] = (out[3] + ((k & 0xFC) << 5)) & 0xFF;
    out[0] = (out[0] + ((k & 0xFE) << 6)) & 0xFF;
    out[1] = (out[1] + ((k << 7) & 0xFF)) & 0xFF;

    // Assemble 64-bit result (little-endian)
    uint64_t result = out[0] | ((uint64_t)out[1] << 8) | ((uint64_t)out[2] << 16) | ((uint64_t)out[3] << 24);
    result |= (uint64_t)h << 32;
    // out[5..7] are unused (zero)
}
```

### 1.3 Convenience Wrappers

```cpp
uint64_t file_hash(const std::string& path) {
    // Split path into base name and extension (with dot)
    // e.g., "armor/daedric/daedric_helm.nif" → base="daedric_helm", ext=".nif"
    auto [base, ext] = split_extension(path);
    uint8_t out[32] = {};
    build(base.c_str(), ext.empty() ? nullptr : ext.c_str(), out);
    return *(uint64_t*)out;
}

uint64_t folder_hash(const std::string& path) {
    // Strip trailing backslash; empty path becomes "."
    std::string p = path;
    while (!p.empty() && p.back() == '\\') p.pop_back();
    if (p.empty()) p = ".";
    uint8_t out[32] = {};
    build(p.c_str(), nullptr, out);  // extra=NULL → no tweak
    return *(uint64_t*)out;
}
```

## 2. Query Path (EXE Side)

### 2.1 Key Generation (0x6FA1B0)

```
0x6FA1B0(path, &folder_hash_out, &file_hash_out):
  1. _splitpath(path, drive, dir, fname, ext)   // 0x98443c
  2. file_hash_out = build(fname, ext)           // file hash with extension tweak
  3. folder_path = drive + dir                   // concatenation
     - if empty → "."
     - strip trailing '\'
  4. folder_hash_out = build(folder_path, NULL)  // folder hash, no tweak
```

### 2.2 Mode Dispatch (0x6FA2D0)

| mode | Input meaning | build() arguments | Use case |
|------|--------------|-------------------|----------|
| 0 | File name only | `(fname, ext)` via splitpath | File lookup (cFileName) |
| 1 | Directory path | `(drive+dir, NULL)` | Folder lookup |
| 2 | Raw string | `(raw, NULL)` | Direct key query |

### 2.3 Prefix Stripping

Before key generation, the query path is normalized:
1. Strip leading `\`
2. Strip `Data\` or `data\` prefix (5-character case-insensitive comparison)

## 3. BSA v103 File Layout

### 3.1 Header (0x24 bytes)

| Offset | Size | Field | Notes |
|--------|------|-------|-------|
| 0x00 | uint32 | magic | `0x00415342` ("BSA\0") |
| 0x04 | uint32 | version | `0x67` (103) |
| 0x08 | uint32 | folderRecordOffset | Always 0x24 (header size) |
| 0x0C | uint32 | (unknown) | Tool-specific constant; EXE writes 0 |
| 0x10 | uint32 | folderCount | Number of folder records |
| 0x14 | uint32 | fileCount | Total file count |
| 0x18 | uint32 | folderNameLength | Sum of (folder name bytes + 1) including NUL |
| 0x1C | uint32 | fileNameLength | Sum of (file name bytes + 1) = name block size |
| 0x20 | uint16 | flags | Unreliable; not used in hash computation |
| 0x22 | uint16 | (unused) | Always zero |

### 3.2 Folder Records (at offset 0x24, folderCount entries)

```
struct BSAFolderRecordV103 {
    uint64_t hash;       // folder_hash(folder_path)
    uint32_t fileCount;  // files in this folder
    uint32_t offset;     // = section_start + header.fileNameLength
};
```

### 3.3 Interleaved Data

For each folder (in order):

```
uint8_t nameLen;                    // strlen + 1 (including NUL)
char     folderName[nameLen];       // NUL-terminated, no trailing backslash
BSAFileRecordV103 files[fileCount]; // file records for this folder
```

Where:

```
struct BSAFileRecordV103 {
    uint64_t hash;   // file_hash(folder_path + "\\" + file_name)
    uint32_t size;   // bit 30 = compression toggle, bits 0-29 = size
    uint32_t offset; // absolute offset to file data in archive
};
```

### 3.4 File Name Block

After all folder blocks, a contiguous block of NUL-terminated file names
(base name with extension, no path). Parse by scanning NULs — do not
rely on `header.fileNameLength` (unreliable in some archives).

## 4. Verification Results

| Metric | Result |
|--------|--------|
| Archives tested | 17 / 17 |
| File records matched | 147,629 / 147,629 (100%) |
| Folder records matched | 2,567 / 2,567 (100%) |

## 5. Known Caveats

1. **Header 0x0C and 0x20**: Values vary across archives but are not used
   in hash computation. EXE initializes them to 0.
2. **Empty name edge case**: `build()` with `n=0` reads `name[-1]` (C quirk).
   Not triggered by any real archive data.
3. **Non-ASCII names**: `tolower` behavior is code-page dependent. All
   verified data is ASCII.
4. **Length > 255**: `hash32`'s length parameter is `uint8_t`, so
   `(n - 3) & 0xFF` wraps. No real path exceeds 255 bytes.