# Oblivion Android - Development Build

## Current Version
0.9.10 (Phase 63 - Development Build)

## Status
App is not yet launchable. Version will remain at 0.x until the app can successfully start and run.

---

## Phase 58-63: Asset Optimization & Compression

### Asset Optimization Systems

- **AssetExtractor**: External storage asset management
- **TextureCompressor**: ASTC 4x4/6x6/8x8, ETC2 RGB/RGBA support
- **AudioCompressor**: WAV to OGG/MP3/AAC conversion
- **LODSystem**: Distance-based mesh detail levels
- **Batch compression scripts**: Offline asset compression tools

### Technical Specifications
- **Language**: C++17
- **Graphics**: OpenGL ES 3.0
- **Platform**: Android 10+ (API 25+)
- **Physics**: Jolt Physics
- **Audio**: OpenAL-Soft
- **Architecture**: arm64-v8a, armeabi-v7a, x86, x86_64
- **Architecture**: arm64-v8a, armeabi-v7a, x86, x86_64

## Known Issues

### APK Size
The current APK is approximately 1.1 GB due to bundled game assets:
- **Textures**: 854 MB (PNG files)
- **Models**: 234 MB (NIF files)
- **Audio**: 112 MB (WAV files)
- **Native libraries**: 50 MB (4 architectures)

Note: App store distribution is not planned, so APK size optimization is not a priority.

## Installation

### Prerequisites
- Android 10 or higher
- 2 GB RAM minimum
- 2 GB storage space

### Steps
1. Download the APK
2. Enable "Install from unknown sources" if needed
3. Install the APK
4. Launch the application

## Development Notes

### Build Configuration
- **Minification**: Enabled for release builds
- **Resource shrinking**: Enabled
- **JNI libs**: Legacy packaging enabled

### Code Metrics
- **C++ Code**: 35,000+ lines
- **Java/Kotlin Code**: 1,100+ lines
- **Total Project**: 48,000+ lines

## Future Work

### Phase 63+: Final Release Preparation
- Execute asset compression on device
- Optimize APK size
- Final testing and validation

### Phase 64+: Production Release
- Remove development assets from APK
- Implement asset download system
- App store preparation

## Credits

### Original Game
- **The Elder Scrolls IV: Oblivion** by Bethesda Game Studios
- **Gamebryo** engine by Gamebase Co., Ltd.
- **Havok** physics by Havok

### Android Port
- **Oblivion Android** project
- **Jolt Physics** for physics simulation
- **OpenAL-Soft** for audio
- **Android NDK** for native development

## License

This project is for educational and research purposes only.
The Elder Scrolls IV: Oblivion is a trademark of Bethesda Softworks LLC.
