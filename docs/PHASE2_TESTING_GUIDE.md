# Phase 2 Testing & Validation Guide

## Overview
This guide provides step-by-step instructions for validating Phase 2 components and verifying the asset loading pipeline.

## Test Environment Setup

### Prerequisites
- Android device or emulator (API 29+)
- Android Studio with Android Debugger
- Phase 2 APK built and installed
- Sample Oblivion assets extracted (see ASSET_EXTRACTION_GUIDE.md)

### Device Preparation
```bash
# Connect device via USB
adb devices

# Grant necessary permissions (if needed)
adb shell pm grant com.example.oblivion android.permission.READ_EXTERNAL_STORAGE
adb shell pm grant com.example.oblivion android.permission.WRITE_EXTERNAL_STORAGE

# Push assets to device
adb push app/src/main/assets/meshes /sdcard/Android/data/com.example.oblivion/files/
adb push app/src/main/assets/textures /sdcard/Android/data/com.example.oblivion/files/
```

## Component Unit Tests

### Test 1: Binary File Reading (NIF Parser)
**Objective**: Verify binary data reading utilities

**Test Code**:
```cpp
#include "assets/nif_parser.h"

void testBinaryReading() {
    // Create test NIF file with known values
    std::vector<uint8_t> testData;
    
    // Add test uint32
    uint32_t testInt = 0x12345678;
    testData.push_back((testInt >> 0) & 0xFF);
    testData.push_back((testInt >> 8) & 0xFF);
    testData.push_back((testInt >> 16) & 0xFF);
    testData.push_back((testInt >> 24) & 0xFF);
    
    // Test readUInt32
    NifParser parser;
    size_t offset = 0;
    uint32_t result = parser.readUInt32(testData, offset);
    
    ASSERT(result == 0x12345678);
    ASSERT(offset == 4);
    
    LOGD("✓ Binary reading test PASSED");
}
```

**Validation Criteria**:
- ✓ Integer values read correctly
- ✓ Offset incremented properly
- ✓ Float conversions accurate
- ✓ Vector3 parsing correct

### Test 2: NIF Header Parsing
**Objective**: Validate NIF file header recognition

**Test Code**:
```cpp
void testNifHeaderParsing() {
    NifParser parser;
    
    // Test with valid Oblivion NIF file
    bool success = parser.parseFile("assets/meshes/furniture/table01.nif");
    
    if (success) {
        const auto& meshes = parser.getMeshes();
        ASSERT(!meshes.empty());
        LOGD("✓ NIF parsing test PASSED - Loaded %zu meshes", meshes.size());
    } else {
        LOGE("✗ NIF parsing test FAILED: %s", parser.getLastError().c_str());
    }
}
```

**Validation Criteria**:
- ✓ Correct magic number detected
- ✓ Version information extracted
- ✓ Mesh data populated
- ✓ Error messages descriptive

### Test 3: DDS Texture Loading
**Objective**: Verify DDS texture loading and GPU upload

**Test Code**:
```cpp
void testDdsLoading() {
    DdsLoader loader;
    
    GLuint texId = loader.loadTexture("assets/textures/furniture/table01.dds");
    
    if (texId != 0) {
        // Verify texture properties
        GLint width, height;
        glBindTexture(GL_TEXTURE_2D, texId);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        
        LOGD("✓ DDS loading test PASSED - Texture: %ux%u", width, height);
        
        // Cleanup
        glDeleteTextures(1, &texId);
    } else {
        LOGE("✗ DDS loading test FAILED: %s", loader.getLastError().c_str());
    }
}
```

**Validation Criteria**:
- ✓ Texture uploaded to GPU
- ✓ Correct dimensions detected
- ✓ Mipmaps generated
- ✓ No GL errors

### Test 4: Asset Manager Caching
**Objective**: Validate cache functionality and memory management

**Test Code**:
```cpp
void testAssetManagerCache() {
    AssetManager mgr;
    mgr.initialize("assets");
    mgr.setMaxCacheSize(100 * 1024 * 1024);  // 100 MB
    
    // Test 1: Load and cache
    auto mesh1 = mgr.loadMesh("furniture/table01");
    ASSERT(mesh1 != nullptr);
    ASSERT(mgr.getCachedMeshCount() == 1);
    LOGD("✓ Loaded 1 mesh into cache");
    
    // Test 2: Reference counting
    auto mesh1_again = mgr.getMesh("furniture/table01");
    ASSERT(mesh1_again == mesh1);  // Same pointer
    LOGD("✓ Cache hit retrieves same pointer");
    
    // Test 3: Load multiple
    auto mesh2 = mgr.loadMesh("furniture/chair01");
    auto mesh3 = mgr.loadMesh("doors/door01");
    ASSERT(mgr.getCachedMeshCount() == 3);
    LOGD("✓ Loaded 3 meshes total");
    
    // Test 4: Unload and eviction
    mgr.unloadMesh("furniture/table01");
    LOGD("✓ Unloaded mesh from cache");
    
    // Test 5: Memory tracking
    size_t memUsage = mgr.getTotalMemoryUsage();
    LOGD("✓ Total memory usage: %zu bytes", memUsage);
}
```

**Validation Criteria**:
- ✓ Cache stores pointers correctly
- ✓ Reference counting works
- ✓ LRU eviction triggers properly
- ✓ Memory limits enforced
- ✓ No memory leaks

### Test 5: Mesh Rendering
**Objective**: Verify mesh rendering with loaded data

**Test Code**:
```cpp
void testMeshRendering() {
    // Create test vertices
    std::vector<Vertex> vertices = {
        {{-1, -1, 0}, {1, 0, 0}, {0, 0}},
        {{1, -1, 0}, {0, 1, 0}, {1, 0}},
        {{0, 1, 0}, {0, 0, 1}, {0.5, 1}}
    };
    
    std::vector<uint16_t> indices = {0, 1, 2};
    
    // Initialize mesh
    Mesh mesh;
    bool success = mesh.init(vertices, indices);
    ASSERT(success);
    
    // Test rendering
    glm::mat4 model, view, projection;
    // ... set up matrices ...
    
    mesh.render(model, view, projection, shaderProgramId);
    LOGD("✓ Mesh rendered successfully");
    
    // Cleanup
    mesh.cleanup();
}
```

**Validation Criteria**:
- ✓ Vertex data uploaded correctly
- ✓ Index buffer created
- ✓ VAO/VBO properly bound
- ✓ Render call completes without errors

## Integration Tests

### Integration Test 1: Full Asset Pipeline
**Objective**: Test complete flow from file to rendered geometry

**Steps**:
1. Load NIF file with NifParser
2. Extract vertex/index data from NifMeshData
3. Create Vertex array with proper format
4. Create Mesh object with vertex data
5. Render mesh in main loop
6. Verify visual output

**Expected Result**: 
- Model appears on screen in correct position
- Model rotates smoothly (if using existing rotation)
- No rendering glitches or artifacts

### Integration Test 2: Memory Pressure Test
**Objective**: Verify memory management under load

**Test Procedure**:
1. Load 10+ different meshes in sequence
2. Monitor memory usage via adb logcat
3. Track LRU eviction events
4. Verify cache never exceeds limit
5. Verify no crashes due to memory exhaustion

**Expected Result**:
```
AssetManager: Loaded mesh 1/10 - Total memory: 5 MB
AssetManager: Loaded mesh 2/10 - Total memory: 12 MB
...
AssetManager: Cache full, evicting LRU mesh
AssetManager: Loaded mesh 10/10 - Total memory: 98 MB
```

### Integration Test 3: Asset Loading Performance
**Objective**: Measure performance characteristics

**Metrics to Track**:
- Time to parse NIF file: `tparse_ms`
- Time to upload to GPU: `tupload_ms`
- Total load time: `ttotal_ms`
- Memory per mesh: `Mbytes`
- Cache hit rate: `%hits`

**Test Code**:
```cpp
void performanceTest() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    auto mesh = assetMgr.loadMesh("furniture/table01");
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    );
    
    LOGD("Asset loading time: %lld ms", duration.count());
    LOGD("Memory usage: %zu bytes", assetMgr.getTotalMemoryUsage());
}
```

**Expected Performance**:
- Parse NIF: 50-100 ms
- Upload texture: 10-20 ms
- Total load: 100-150 ms
- Memory per mesh: 1-10 MB (excluding textures)

## Diagnostic Tests

### Diagnostic 1: Logcat Monitoring
```bash
# Monitor all logs
adb logcat | grep "NifParser\|DdsLoader\|AssetManager"

# Clear and monitor fresh
adb logcat -c
adb logcat | grep "Phase2"
```

### Diagnostic 2: Memory Profiling
```bash
# Monitor memory usage in real-time
adb shell dumpsys meminfo com.example.oblivion | grep -E "TOTAL|Native|Heap"

# Dump memory snapshot
adb shell am dumpheap com.example.oblivion /data/dump.hprof
adb pull /data/dump.hprof
```

### Diagnostic 3: OpenGL Debug
```cpp
// Add to renderer for GL error checking
void checkGLErrors(const char* location) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOGE("GL Error at %s: 0x%04X", location, error);
    }
}
```

## Automated Test Suite

### Create JNI Test Interface
```cpp
// In jni_bridge.cpp - add test functions
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_oblivion_GameRenderer_nativeRunPhase2Tests(
    JNIEnv* env, jobject obj) {
    
    // Run all unit tests
    testBinaryReading();
    testNifHeaderParsing();
    testDdsLoading();
    testAssetManagerCache();
    testMeshRendering();
    
    return JNI_TRUE;
}
```

### Call from Java
```java
// In GameRenderer.java
public boolean runPhase2Tests() {
    return nativeRunPhase2Tests();
}

// Add to MainActivity for testing
if (BuildConfig.DEBUG) {
    gameRenderer.runPhase2Tests();
}
```

## Milestone Verification

### M2-1: Display First NIF Mesh
**Verification Steps**:
1. [ ] Extract sample NIF file from Oblivion ISO
2. [ ] Place in `app/src/main/assets/meshes/`
3. [ ] Verify NIF parser reads file successfully (check logcat)
4. [ ] Verify mesh appears on screen
5. [ ] Verify correct position and orientation
6. [ ] Take screenshot as proof

**Success Criteria**:
```
✓ Logcat shows "NifParser: Successfully parsed NIF file"
✓ Screen shows 3D model (not just black or error)
✓ Model is positioned correctly (center of screen or rotates)
✓ No GL errors or crashes
```

### M2-2: Apply DDS Textures
**Verification Steps**:
1. [ ] Extract sample DDS texture from Oblivion ISO
2. [ ] Place in `app/src/main/assets/textures/`
3. [ ] Link texture path in NIF mesh data
4. [ ] Load texture with DdsLoader
5. [ ] Apply to mesh (update shaders if needed)
6. [ ] Verify texture appears on model

**Success Criteria**:
```
✓ Logcat shows "DdsLoader: Texture loaded - 512x512"
✓ Model displays with color/details from texture
✓ No stretching or distortion
✓ Proper texture coordinate mapping
```

### M2-3: Verify Memory Caching
**Verification Steps**:
1. [ ] Load 5+ different meshes
2. [ ] Monitor memory with `adb shell dumpsys meminfo`
3. [ ] Verify cache doesn't exceed 512 MB
4. [ ] Unload meshes and verify memory released
5. [ ] Trigger LRU eviction and verify handled correctly
6. [ ] Load meshes again and verify cache hits

**Success Criteria**:
```
✓ Memory stays under 512 MB limit
✓ LRU eviction occurs when cache full
✓ Evicted meshes are reloadable
✓ Cache hit rate > 80% for repeated loads
✓ No memory leaks (memory decreases after unload)
```

## Troubleshooting During Testing

### Issue: "NIF Parse Failed" Error
**Diagnosis**:
```bash
adb logcat | grep "NifParser"
# Check for specific error messages
```

**Solutions**:
- Verify file is valid NIF (magic number check)
- Try different NIF file
- Check file permissions
- Verify path is correct

### Issue: Texture Not Appearing
**Diagnosis**:
- Check if DDS file loaded (logcat)
- Verify texture coordinates in mesh data
- Check shader texture sampler binding

### Issue: Low Frame Rate During Loading
**Solution**:
- Use background thread for asset loading
- Implement progress callbacks
- Stream assets asynchronously

### Issue: Out of Memory Errors
**Solution**:
- Reduce cache size limit
- Load fewer assets simultaneously
- Implement aggressive LRU eviction
- Compress textures

## Performance Optimization Checklist

- [ ] Profile asset loading times
- [ ] Measure memory usage per asset
- [ ] Identify bottlenecks (parsing vs GPU upload)
- [ ] Implement async loading if needed
- [ ] Verify cache hit rates
- [ ] Monitor frame rate impact
- [ ] Test on multiple device types

## Sign-Off Criteria

Phase 2 is complete when:
- ✅ All unit tests pass
- ✅ All integration tests pass
- ✅ M2-1 verified (NIF mesh displays)
- ✅ M2-2 verified (textures apply)
- ✅ M2-3 verified (memory caching works)
- ✅ Performance meets targets
- ✅ No critical bugs remain
- ✅ Code is documented

---

See also:
- PHASE2_IMPLEMENTATION_SUMMARY.md
- ASSET_EXTRACTION_GUIDE.md
- Architecture documentation
