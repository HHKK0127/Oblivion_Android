#pragma once

#include <string>
#include <vector>

/**
 * @brief Resolution candidates for the video menus (RE §36).
 *
 * The original does not read this list from XML. VideoDisplayMenu enumerates the
 * adapter's display modes through Direct3D9 (IDirect3D9::GetAdapterModeCount /
 * EnumAdapterModes - not EnumDisplaySettings), keeps only the distinct
 * (width, height) pairs whose pixel count reaches 307200, and presents them in
 * ascending order. Android has the same information in
 * Display.getSupportedModes(), which GameRenderer forwards through
 * nativeSetDisplayModes().
 *
 * Until that call arrives - and on any device that reports nothing usable - the
 * list holds the five presets the original's VideoDisplayMenu ships with, so the
 * menus stay populated either way.
 *
 * An earlier reading of the executable (RE §30.3) concluded the list was a static
 * code table because EnumDisplaySettings is absent from the imports. RE §36
 * supersedes that: the enumeration goes through the D3D9 COM vtable instead.
 */
namespace display_modes {

struct Mode {
    int width = 0;
    int height = 0;
};

/// The original's minimum accepted pixel count, 0x4B000 (RE §36.3). 640x480
/// passes on equality.
constexpr int kMinPixelCount = 307200;

/// Replace the list with the reported modes. Entries that are not positive, do
/// not reach kMinPixelCount or repeat an earlier (width, height) pair are
/// dropped, and the survivors are sorted ascending by width then height. A null
/// or empty report restores the presets instead.
void set_supported(const int* widths, const int* heights, int count);

/// Restore the five built-in presets.
void reset_to_presets();

const std::vector<Mode>& all();
int count();

/// Null when index is out of range.
const Mode* at(int index);

/// "800 x 600", or an empty string when index is out of range.
std::string label(int index);

/// The original marks a mode widescreen when height * 4 < width * 3 (RE §36.6).
bool is_widescreen(const Mode& mode);

/// Index of an exact (width, height) match, or -1.
int index_of(int width, int height);

}  // namespace display_modes
