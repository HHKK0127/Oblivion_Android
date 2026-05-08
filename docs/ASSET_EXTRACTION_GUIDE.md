# Oblivion Asset Extraction Guide

This guide explains how to extract NIF models and DDS textures from Oblivion GOTY ISO files for use in the Android native port.

## Prerequisites

### Required Software
1. **7-Zip** or **WinRAR** or **PowerISO** - for ISO file mounting/extraction
   - Download: https://www.7-zip.org/ (free)
   - Alternative: Windows 10+ can mount ISO files natively

2. **Command-line tools** (optional but recommended)
   - PowerShell (included with Windows)
   - Or Git Bash / WSL for Unix-like experience

### Available ISO Files
- `I:\Closet\Oblivion\Oblivion GOTY 1.iso`
- `I:\Closet\Oblivion\Oblivion GOTY 2.iso`

## Step 1: Mount or Extract ISO

### Method A: Windows Native Mounting (Windows 10+)
1. Open File Explorer
2. Right-click on `Oblivion GOTY 1.iso`
3. Select "Mount"
4. A new drive letter (e.g., `D:\`) will appear
5. Navigate to the mounted drive

### Method B: 7-Zip Extraction (All Windows Versions)
1. Right-click on `Oblivion GOTY 1.iso`
2. Select "7-Zip" → "Extract to `Oblivion GOTY 1\`"
3. This extracts to: `I:\Closet\Oblivion\Oblivion GOTY 1\`

### Method C: PowerISO (GUI)
1. Open PowerISO
2. File → Open → select ISO
3. Extract files/folders as needed

## Step 2: Locate Game Assets

Inside the mounted/extracted ISO, the structure is:
```
[ISO Root]/
├── Data/
│   ├── meshes/           ← NIF files (3D models)
│   │   ├── architecture/
│   │   ├── actors/
│   │   ├── creatures/
│   │   ├── doors/
│   │   ├── furniture/
│   │   ├── landscape/
│   │   ├── misc/
│   │   ├── trees/
│   │   └── weapons/
│   │
│   ├── textures/         ← DDS files (textures)
│   │   ├── architecture/
│   │   ├── actors/
│   │   ├── creatures/
│   │   ├── fx/
│   │   ├── furniture/
│   │   ├── landscape/
│   │   ├── misc/
│   │   ├── sky/
│   │   ├── trees/
│   │   └── weapons/
│   │
│   ├── meshes.0          ← Additional mesh archive (if present)
│   ├── textures.0        ← Additional texture archive (if present)
│   └── [other game data]
│
└── [other ISO files]
```

## Step 3: Extract Specific Assets

### Extract Sample Models
We'll start with a few simple models for testing:

```powershell
# Using PowerShell (Windows)
# Change to project directory
cd "C:\Users\E1192\Projects\oblivion-android\app\src\main\assets"

# Create assets directory
mkdir -p meshes
mkdir -p textures

# If using mounted ISO (e.g., drive D:\)
# Copy specific models:
copy "D:\Data\meshes\architecture\doors\door01.nif" "meshes\"
copy "D:\Data\meshes\furniture\table01.nif" "meshes\"
copy "D:\Data\meshes\furniture\chair01.nif" "meshes\"

# Copy corresponding textures:
copy "D:\Data\textures\architecture\doors\door01.dds" "textures\"
copy "D:\Data\textures\furniture\table01.dds" "textures\"
copy "D:\Data\textures\furniture\chair01.dds" "textures\"
```

### Extract Using 7-Zip (GUI)
1. Open ISO with 7-Zip
2. Navigate to `Data\meshes\`
3. Select desired `.nif` files
4. Right-click → Extract to `C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\meshes\`

### Alternative: Batch Extract Large Collections
```powershell
# Extract all furniture models
$source = "D:\Data\meshes\furniture\"
$dest = "C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\meshes\furniture\"
mkdir -p $dest
copy-item -Path "$source*" -Destination $dest -Recurse

# Extract corresponding textures
$source = "D:\Data\textures\furniture\"
$dest = "C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\textures\furniture\"
mkdir -p $dest
copy-item -Path "$source*" -Destination $dest -Recurse
```

## Step 4: Organize Assets in Android Project

### Recommended Directory Structure
```
app/src/main/assets/
├── meshes/
│   ├── architecture/
│   │   └── doors/
│   │       ├── door01.nif
│   │       └── door02.nif
│   ├── furniture/
│   │   ├── table01.nif
│   │   └── chair01.nif
│   └── creatures/
│       └── rat.nif
│
└── textures/
    ├── architecture/
    │   └── doors/
    │       ├── door01.dds
    │       └── door02.dds
    ├── furniture/
    │   ├── table01.dds
    │   └── chair01.dds
    └── creatures/
        └── rat.dds
```

### Create Asset Catalog (Optional)
Create a manifest file listing available assets:
```
app/src/main/assets/catalog.txt:
architecture/doors/door01
furniture/table01
furniture/chair01
creatures/rat
```

## Step 5: Access Assets from C++

### In C++ Code
```cpp
// Using Android Asset Manager
#include <android/asset_manager.h>

// Example: Load mesh from assets
AAsset* asset = AAssetManager_open(assetManager, 
    "meshes/furniture/table01.nif", 
    AASSET_MODE_STREAMING);

if (asset) {
    off_t size = AAsset_getLength(asset);
    const void* buffer = AAsset_getBuffer(asset);
    
    // Parse NIF data
    // ...
    
    AAsset_close(asset);
}
```

### Via Java Interop
```java
// In GameRenderer.java or similar
AssetManager assetManager = context.getAssets();
InputStream input = assetManager.open("meshes/furniture/table01.nif");

// Pass to native code via JNI
nativeLoadAsset(buffer, size);
```

## Step 6: File Size Considerations

### Typical Asset Sizes
- Small NIF (simple furniture): 10-50 KB
- Medium NIF (building): 100-500 KB
- Large NIF (landscape): 500 KB - 5 MB

- Small DDS (256×256): 64 KB - 256 KB
- Medium DDS (512×512): 256 KB - 1 MB
- Large DDS (1024×1024): 1 MB - 4 MB

### Recommended Selection for Testing
Start with small assets:
1. **Furniture** (very small): chair, table, barrel
2. **Architecture** (small): doors, windows
3. **Weapons** (medium): sword, bow
4. **Creatures** (medium): rat, mudcrab, imp

Estimated total: 50-100 MB for a good test set

## Step 7: Asset Path Mapping

### C++ Usage
```cpp
// In AssetManager class
std::string constructMeshPath(const std::string& meshName) {
    // Input: "furniture/table01"
    // Output: "assets/meshes/furniture/table01.nif"
    return "meshes/" + meshName + ".nif";
}

// In NifParser
bool parseFile(const std::string& filePath) {
    // filePath = "assets/meshes/furniture/table01.nif"
    // Read and parse this file
}
```

## Troubleshooting

### Issue: Cannot Mount ISO
**Solution**: 
- Use 7-Zip or PowerISO instead
- Ensure file path doesn't contain special characters
- Check that ISO file is not corrupted

### Issue: File Not Found in Android APK
**Solution**:
- Verify file is in `app/src/main/assets/` folder
- Check that Gradle rebuilt project after adding files
- Run `gradle clean` and `gradle build` again

### Issue: File Too Large for APK
**Solution**:
- APK size limit on Google Play: 100 MB
- Use Download-on-First-Run feature (optional)
- Compress assets further or use asset streaming

### Issue: NIF Parse Failure
**Potential Causes**:
- NIF version mismatch (different Oblivion patch)
- File corruption during extraction
- Parser doesn't handle specific NIF variant

**Solution**:
- Try different NIF files
- Verify file size matches expectations
- Check parser logs (adb logcat)

## Advanced: Asset Streaming

For large projects, consider on-demand download:

```cpp
// Pseudo-code: Download assets on-demand
class AssetManager {
    bool ensureAssetAvailable(const std::string& name) {
        if (fileExists(name)) {
            return true;
        }
        
        // Download from server
        return downloadAsset(name);
    }
};
```

## Asset Quality Notes

### Oblivion Asset Characteristics
- **Polygon count**: 100-10,000 per model (low by modern standards)
- **Texture resolution**: 256×256 to 1024×1024 (older standard)
- **Animation capability**: Supported in NIF format
- **Level of Detail (LOD)**: Present in landscape meshes

### Optimization Recommendations
1. Use lower-resolution texture variants
2. Implement LOD system for distant objects
3. Batch similar materials together
4. Use instancing for repeated objects

## References

- **UESP File Formats Wiki**: https://en.uesp.net/wiki/Oblivion:File_formats
- **NIF Format Specification**: Available from UESP
- **DDS Format Reference**: Microsoft DirectDraw Surface specification
- **OpenMW Asset Reference**: https://github.com/OpenMW/openmw/

## Asset Extraction Checklist

- [ ] ISO file located and accessible
- [ ] ISO mounted or extracted
- [ ] Assets directory created in Android project
- [ ] Sample models extracted (3-5 NIF files)
- [ ] Corresponding textures extracted
- [ ] File permissions correct (readable)
- [ ] APK rebuild successful
- [ ] Assets appear in installed APK

---

## Next: Integration Testing

Once assets are extracted and organized:
1. Update CMakeLists.txt to include asset paths
2. Modify Renderer to load first asset
3. Test NIF parser with real data
4. Verify rendering
5. Check memory usage
6. Optimize as needed

See: `PHASE2_IMPLEMENTATION_SUMMARY.md` for integration details.
