#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>

// Theme color (RGBA)
struct ThemeColor {
    float r, g, b, a;
    
    ThemeColor() : r(0), g(0), b(0), a(1) {}
    ThemeColor(float _r, float _g, float _b, float _a = 1.0f) 
        : r(_r), g(_g), b(_b), a(_a) {}
    ThemeColor(uint32_t hex) {
        r = ((hex >> 16) & 0xFF) / 255.0f;
        g = ((hex >> 8) & 0xFF) / 255.0f;
        b = (hex & 0xFF) / 255.0f;
        a = ((hex >> 24) & 0xFF) / 255.0f;
        if (a == 0 && hex > 0xFFFFFF) a = 1.0f;
    }
    
    uint32_t toHex() const {
        return ((uint32_t)(a * 255) << 24) |
               ((uint32_t)(r * 255) << 16) |
               ((uint32_t)(g * 255) << 8) |
               (uint32_t)(b * 255);
    }
    
    // Blending
    ThemeColor darken(float amount) const {
        return ThemeColor(r * (1.0f - amount), g * (1.0f - amount), 
                         b * (1.0f - amount), a);
    }
    
    ThemeColor lighten(float amount) const {
        return ThemeColor(
            r + (1.0f - r) * amount,
            g + (1.0f - g) * amount,
            b + (1.0f - b) * amount,
            a
        );
    }
    
    ThemeColor withAlpha(float alpha) const {
        return ThemeColor(r, g, b, alpha);
    }
    
    // Interpolation
    static ThemeColor lerp(const ThemeColor& a, const ThemeColor& b, float t) {
        return ThemeColor(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
};

// Font settings
struct ThemeFont {
    std::string family;
    int size;
    int weight; // 400=normal, 700=bold
    bool italic;
    
    ThemeFont() : size(14), weight(400), italic(false) {}
};

// Animation settings
struct ThemeAnimation {
    float duration;      // seconds
    float delay;
    std::string easing;  // "linear", "ease", "ease-in", "ease-out", "spring"
    
    ThemeAnimation() : duration(0.2f), delay(0), easing("ease-out") {}
};

// Shadow settings
struct ThemeShadow {
    float offsetX, offsetY;
    float blur;
    float spread;
    ThemeColor color;
    
    ThemeShadow() : offsetX(0), offsetY(2), blur(4), spread(0), color(0x40000000) {}
};

// Border settings
struct ThemeBorder {
    float width;
    ThemeColor color;
    float radius; // corner radius
    
    ThemeBorder() : width(1), color(0xFFCCCCCC), radius(4) {}
};

// Complete theme definition
struct Theme {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
    
    // Color palette
    ThemeColor background;
    ThemeColor surface;
    ThemeColor surfaceVariant;
    ThemeColor primary;
    ThemeColor primaryVariant;
    ThemeColor secondary;
    ThemeColor accent;
    ThemeColor error;
    ThemeColor warning;
    ThemeColor success;
    ThemeColor info;
    
    // Text colors
    ThemeColor textPrimary;
    ThemeColor textSecondary;
    ThemeColor textDisabled;
    ThemeColor textOnPrimary;
    ThemeColor textOnSecondary;
    
    // UI elements
    ThemeColor button;
    ThemeColor buttonHover;
    ThemeColor buttonPressed;
    ThemeColor buttonDisabled;
    
    ThemeColor inputBackground;
    ThemeColor inputBorder;
    ThemeColor inputFocused;
    
    ThemeColor panel;
    ThemeColor panelHeader;
    ThemeColor panelBorder;
    
    ThemeColor tab;
    ThemeColor tabActive;
    ThemeColor tabHover;
    
    ThemeColor scrollbar;
    ThemeColor scrollbarThumb;
    ThemeColor scrollbarThumbHover;
    
    // Special
    ThemeColor highlight;
    ThemeColor selection;
    ThemeColor tooltip;
    ThemeColor overlay;
    
    // Fonts
    ThemeFont fontBody;
    ThemeFont fontHeading;
    ThemeFont fontMono;
    
    // Sizes
    float spacingUnit;      // base spacing (8dp recommended)
    float borderRadius;     // base corner radius
    float shadowElevation;  // shadow intensity
    
    // Animation
    ThemeAnimation animFast;
    ThemeAnimation animNormal;
    ThemeAnimation animSlow;
    
    // Other
    ThemeShadow shadowSmall;
    ThemeShadow shadowMedium;
    ThemeShadow shadowLarge;
    
    ThemeBorder border;
    
    Theme() {
        setDefaults();
    }
    
    void setDefaults();
};

// ============================================================================
// ThemeManager - Singleton
// ============================================================================
class ThemeManager {
public:
    static ThemeManager& getInstance();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    
    // Theme management
    void registerTheme(const Theme& theme);
    void unregisterTheme(const std::string& name);
    
    Theme* getTheme(const std::string& name);
    Theme* getCurrentTheme();
    std::vector<std::string> getThemeNames() const;
    
    // Active theme switch
    bool setActiveTheme(const std::string& name, bool animate = true);
    std::string getActiveThemeName() const;
    
    // Real-time update
    void update(float deltaTime); // for animation
    
    // Color access (interpolated during animation)
    ThemeColor getColor(const std::string& colorName) const;
    
    // Color override
    void setColorOverride(const std::string& colorName, const ThemeColor& color);
    void clearOverrides();
    
    // Events
    using ThemeChangeCallback = std::function<void(const Theme& newTheme)>;
    void addChangeListener(ThemeChangeCallback callback);
    
    // Built-in theme creation
    void createBuiltinThemes();
    
private:
    ThemeManager() = default;
    ~ThemeManager() = default;
    
    ThemeColor getColorFromTheme(const Theme& theme, const std::string& name);
    
    std::map<std::string, std::unique_ptr<Theme>> themes_;
    Theme* currentTheme_ = nullptr;
    std::string currentThemeName_;
    
    // Animation state
    bool transitioning_ = false;
    float transitionProgress_ = 0.0f;
    float transitionDuration_ = 0.3f;
    Theme transitionSource_;
    Theme transitionTarget_;
    
    // Overrides
    std::map<std::string, ThemeColor> overrides_;
    
    // Listeners
    std::vector<ThemeChangeCallback> listeners_;
    
    bool initialized_ = false;
};
