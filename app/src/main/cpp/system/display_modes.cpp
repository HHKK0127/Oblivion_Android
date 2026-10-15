#include "display_modes.h"

#include <algorithm>

namespace display_modes {
namespace {

/// The five presets authored in the original's VideoDisplayMenu (RE §36.6). They
/// are kept in the authored order rather than sorted, because that is the order
/// the menu shows them in when no display report is available.
const Mode kPresets[] = {
    {800, 600}, {1080, 607}, {1024, 768}, {1280, 607}, {1280, 720},
};

std::vector<Mode> g_modes(std::begin(kPresets), std::end(kPresets));

bool is_ascending(const Mode& a, const Mode& b) {
    if (a.width != b.width) {
        return a.width < b.width;
    }
    return a.height < b.height;
}

}  // namespace

void set_supported(const int* widths, const int* heights, int count) {
    if (!widths || !heights || count <= 0) {
        reset_to_presets();
        return;
    }

    std::vector<Mode> modes;
    modes.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const Mode mode{widths[i], heights[i]};
        if (mode.width <= 0 || mode.height <= 0) {
            continue;
        }
        if (mode.width * mode.height < kMinPixelCount) {
            continue;
        }
        const bool duplicate = std::any_of(
            modes.begin(), modes.end(), [&mode](const Mode& other) {
                return other.width == mode.width && other.height == mode.height;
            });
        if (!duplicate) {
            modes.push_back(mode);
        }
    }

    if (modes.empty()) {
        reset_to_presets();
        return;
    }

    std::sort(modes.begin(), modes.end(), is_ascending);
    g_modes = std::move(modes);
}

void reset_to_presets() {
    g_modes.assign(std::begin(kPresets), std::end(kPresets));
}

const std::vector<Mode>& all() {
    return g_modes;
}

int count() {
    return static_cast<int>(g_modes.size());
}

const Mode* at(int index) {
    if (index < 0 || index >= count()) {
        return nullptr;
    }
    return &g_modes[static_cast<std::size_t>(index)];
}

std::string label(int index) {
    const Mode* mode = at(index);
    if (!mode) {
        return std::string();
    }
    return std::to_string(mode->width) + " x " + std::to_string(mode->height);
}

bool is_widescreen(const Mode& mode) {
    return mode.height * 4 < mode.width * 3;
}

int index_of(int width, int height) {
    for (int i = 0; i < count(); ++i) {
        if (g_modes[static_cast<std::size_t>(i)].width == width &&
            g_modes[static_cast<std::size_t>(i)].height == height) {
            return i;
        }
    }
    return -1;
}

}  // namespace display_modes
