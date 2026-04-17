# Oblivion Android - Installation Guide

## Prerequisites

- **Android Device**: Android 10.0 (API 29) or higher
- **Free Storage**: Minimum 500 MB for APK and assets
- **RAM**: Minimum 2 GB (3+ GB recommended)
- **Device Compatibility**: ARM64-v8a or ARMv7 architecture

## Installation Methods

### Method 1: Direct APK Installation (Recommended for Testing)

1. **Transfer APK to Device**
   ```bash
   adb install -r app/build/outputs/apk/release/app-release.apk
   ```

2. **Or via USB File Transfer**
   - Connect device via USB
   - Transfer APK to device storage
   - Open file manager, locate APK, tap to install
   - Grant permissions when prompted

### Method 2: Google Play Store (Future Release)

- Coming soon to Google Play Store
- Search for "Oblivion Android"
- Tap Install

## Post-Installation Setup

### First Launch
1. Launch app from home screen
2. Wait for TitleScreen to appear (3 seconds)
3. Tap "Start Game" to enter world
4. Game will initialize all systems (~5 seconds)

### Permissions
The app requires:
- **No special permissions** for current version
- Future versions may require:
  - Storage (for cloud saves)
  - Network (for potential online features)

## Troubleshooting

### App Crashes on Launch
**Solution**: 
- Uninstall completely: `adb uninstall -k com.example.oblivion`
- Reinstall APK fresh
- Ensure Android version is 10.0+

### Low FPS (Below 30)
**Solution**:
- Close other apps using significant resources
- Device may have insufficient RAM
- Lower device population or draw distance (future version)

### Installation Failed: "App not installed"
**Solution**:
- Check device has 500+ MB free storage
- Ensure device architecture is ARM64 or ARMv7
- Try: `adb install -r --grant-all-permissions app-release.apk`

### Device Not Detected by ADB
**Solution**:
- Enable USB Debugging: Settings > Developer Options > USB Debugging
- Install Android USB drivers for device manufacturer
- Try different USB cable or port

## Supported Devices (Tested)

| Device | OS | Status | Notes |
|--------|----|---------| ------|
| Amazon Fire Tablet | Android 9 | ✅ Stable | 30GB+ storage, 60 FPS |
| Xiaomi 24018RPACG | Android 16 | ✅ Stable | High-res (2032×3048), 60 FPS |
| Generic ARM64 | Android 10-14 | ✅ Expected | Varies by device specs |

## Uninstallation

```bash
adb uninstall com.example.oblivion
```

Or via device: Settings > Apps > Oblivion > Uninstall

## Performance Expectations

### Minimum Device (Android 10, 2GB RAM, ARM64)
- Initial Load Time: 20-30 seconds
- FPS: 30 fps (stable)
- Memory Usage: 150-200 MB
- Battery Drain: ~2-3% per hour

### Standard Device (Android 12, 4GB RAM, ARM64)
- Initial Load Time: 10-15 seconds
- FPS: 60 fps (stable)
- Memory Usage: 200-250 MB
- Battery Drain: ~1-2% per hour

### High-End Device (Android 14, 8GB+ RAM, ARM64)
- Initial Load Time: 5-10 seconds
- FPS: 60 fps (stable, high quality)
- Memory Usage: 250-350 MB
- Battery Drain: ~1-1.5% per hour

## Important Notes

⚠️ **This is an Experimental Port**
- Features are still in development
- Save data may not be persistent across versions
- UI is text-based (not fully polished)
- Some Oblivion features may not be fully implemented

🎮 **Game Progress**
- Single-player only
- No multiplayer features
- Save/Load coming in future versions
- Current max play time tested: 1+ hour stable

## Support

For issues or feedback:
- Check [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for common problems
- Review [GAMEPLAY.md](GAMEPLAY.md) for gameplay guide
- See [PERFORMANCE_REPORT.md](PERFORMANCE_REPORT.md) for detailed metrics

---

**Last Updated**: 2026-04-17  
**Version**: Phase 6 Release Candidate
