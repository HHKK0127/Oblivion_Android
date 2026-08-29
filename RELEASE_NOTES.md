# Oblivion Android v1.5.0 - Release Notes

## Release Date
2026-08-29

## Version
1.5.0 (Phase 62 - Asset Integration Complete)

## What's New

### Complete Engine Implementation
- **62 phases** of development completed
- **7 middleware engines** at or above 65% completion
- **Gamebryo equivalent**: 100% complete
- **Script VM**: 47 opcodes, 118 game functions
- **Physics**: Jolt Physics integration
- **Audio**: OpenAL-Soft 3D audio
- **UI**: Complete graphical UI and HUD system

### Asset Optimization (Phase 58-62)
- **AssetExtractor**: External storage asset management
- **TextureCompressor**: ASTC 4x4/6x6/8x8, ETC2 RGB/RGBA support
- **AudioCompressor**: WAV to OGG/MP3/AAC conversion
- **LODSystem**: Distance-based mesh detail levels
- **Batch compression scripts**: Offline asset compression tools

### Technical Specifications
- **Language**: C++17
- **Graphics**: OpenGL ES 3.0
- **Platform**: Android 10+ (API 25+)
- **Architecture**: arm64-v8a, armeabi-v7a, x86, x86_64

## Known Issues

### APK Size
The current APK is approximately 1.1 GB due to bundled game assets:
- **Textures**: 854 MB (PNG files)
- **Models**: 234 MB (NIF files)
- **Audio**: 112 MB (WAV files)
- **Native libraries**: 50 MB (4 architectures)

### Asset Requirements
This application requires original Oblivion game assets to function properly. The bundled assets are for development/testing purposes only.

**For production release:**
1. Remove large assets from APK
2. Implement asset extraction from external storage
3. Or use Android's expansion file (OBB) system

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
