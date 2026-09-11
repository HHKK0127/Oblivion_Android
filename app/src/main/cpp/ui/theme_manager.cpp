#include "theme_manager.h"
#include <android/log.h>
#include <algorithm>

#define LOG_TAG "ThemeManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Default theme values
void Theme::setDefaults() {
    spacingUnit = 8.0f;
    borderRadius = 4.0f;
    shadowElevation = 1.0f;
    
    // Material Design 3 Dark defaults
    background = ThemeColor(0xFF1C1B1F);
    surface = ThemeColor(0xFF313033);
    surfaceVariant = ThemeColor(0xFF49454F);
    primary = ThemeColor(0xFFD0BCFF);
    primaryVariant = ThemeColor(0xFFCCC2DC);
    secondary = ThemeColor(0xFF625B71);
    accent = ThemeColor(0xFFD0BCFF);
    error = ThemeColor(0xFFF2B8B5);
    warning = ThemeColor(0xFFF9DEDC);
    success = ThemeColor(0xFFAAC9FF);
    info = ThemeColor(0xFF9ECBFF);
    
    textPrimary = ThemeColor(0xFFE6E1E5);
    textSecondary = ThemeColor(0xFF938F99);
    textDisabled = ThemeColor(0xFF605D62);
    textOnPrimary = ThemeColor(0xFF381E72);
    textOnSecondary = ThemeColor(0xFF332D41);
    
    button = surface;
    buttonHover = surfaceVariant;
    buttonPressed = primary.withAlpha(0.3f);
    buttonDisabled = surface.withAlpha(0.5f);
    
    inputBackground = surfaceVariant;
    inputBorder = ThemeColor(0xFF49454F);
    inputFocused = primary;
    
    panel = surface;
    panelHeader = surfaceVariant;
    panelBorder = ThemeColor(0xFF49454F);
    
    tab = surface;
    tabActive = primary;
    tabHover = surfaceVariant;
    
    scrollbar = surfaceVariant;
    scrollbarThumb = ThemeColor(0xFF605D62);
    scrollbarThumbHover = ThemeColor(0xFF938F99);
    
    highlight = primary.withAlpha(0.3f);
    selection = primary.withAlpha(0.2f);
    tooltip = ThemeColor(0xFF313033);
    overlay = ThemeColor(0x80000000);
    
    animFast.duration = 0.1f;
    animNormal.duration = 0.2f;
    animSlow.duration = 0.3f;
}

ThemeManager& ThemeManager::getInstance() {
    static ThemeManager instance;
    return instance;
}

bool ThemeManager::initialize() {
    if (initialized_) return true;
    
    // Create default themes
    createBuiltinThemes();
    
    // Set default as active
    setActiveTheme("Dark Modern", false);
    
    initialized_ = true;
    LOGD("ThemeManager initialized");
    return true;
}

void ThemeManager::shutdown() {
    themes_.clear();
    currentTheme_ = nullptr;
    initialized_ = false;
}

// Built-in theme creation
void ThemeManager::createBuiltinThemes() {
    // 1. Dark Modern (Material Design 3 Dark)
    {
        Theme t;
        t.name = "Dark Modern";
        t.description = "Modern dark theme based on Material Design 3";
        t.author = "System";
        // Uses defaults
        registerTheme(t);
    }
    
    // 2. Oblivion Classic (game-themed)
    {
        Theme t;
        t.name = "Oblivion Classic";
        t.description = "Elder Scrolls inspired golden theme";
        t.author = "System";
        
        // Brown and gold colors
        t.background = ThemeColor(0xFF1A1510);  // Dark brown
        t.surface = ThemeColor(0xFF2A2018);     // Brown
        t.surfaceVariant = ThemeColor(0xFF3A3020);
        t.primary = ThemeColor(0xFFD4AF37);       // Gold
        t.primaryVariant = ThemeColor(0xFFC5A028);
        t.secondary = ThemeColor(0xFF8B7355);    // Leather
        t.accent = ThemeColor(0xFFFFD700);      // Gold
        t.error = ThemeColor(0xFF8B0000);        // Dark red
        t.warning = ThemeColor(0xFFFFA500);      // Orange
        t.success = ThemeColor(0xFF228B22);      // Forest green
        
        t.textPrimary = ThemeColor(0xFFE8DCC8);  // Parchment
        t.textSecondary = ThemeColor(0xFFB8A898);
        t.textOnPrimary = ThemeColor(0xFF1A1510);
        
        // Decorative border
        t.border.width = 2.0f;
        t.border.color = ThemeColor(0xFFD4AF37);
        t.border.radius = 4.0f;
        
        registerTheme(t);
    }
    
    // 3. Light Clean
    {
        Theme t;
        t.name = "Light Clean";
        t.description = "Clean and minimal light theme";
        t.author = "System";
        
        t.background = ThemeColor(0xFFFFFFFF);
        t.surface = ThemeColor(0xFFF5F5F5);
        t.surfaceVariant = ThemeColor(0xFFE8E8E8);
        t.primary = ThemeColor(0xFF1976D2);
        t.secondary = ThemeColor(0xFF424242);
        t.accent = ThemeColor(0xFF00BCD4);
        t.error = ThemeColor(0xFFD32F2F);
        t.warning = ThemeColor(0xFFFFA000);
        t.success = ThemeColor(0xFF388E3C);
        
        t.textPrimary = ThemeColor(0xFF212121);
        t.textSecondary = ThemeColor(0xFF757575);
        t.textDisabled = ThemeColor(0xFFBDBDBD);
        
        t.button = t.surface;
        t.buttonHover = ThemeColor(0xFFE0E0E0);
        
        t.borderRadius = 4.0f;
        
        registerTheme(t);
    }
    
    // 4. High Contrast (accessibility)
    {
        Theme t;
        t.name = "High Contrast";
        t.description = "High contrast for accessibility";
        t.author = "System";
        
        t.background = ThemeColor(0xFF000000);
        t.surface = ThemeColor(0xFF000000);
        t.primary = ThemeColor(0xFFFFFF00);  // Yellow
        t.secondary = ThemeColor(0xFF00FFFF); // Cyan
        t.accent = ThemeColor(0xFFFF00FF);   // Magenta
        t.error = ThemeColor(0xFFFF0000);
        
        t.textPrimary = ThemeColor(0xFFFFFFFF);
        t.textSecondary = ThemeColor(0xFFFFFF00);
        t.textOnPrimary = ThemeColor(0xFF000000);
        
        t.border.width = 2.0f;
        t.border.color = ThemeColor(0xFFFFFFFF);
        t.borderRadius = 0.0f;  // No rounded corners
        
        registerTheme(t);
    }
    
    // 5. Cyberpunk Neon
    {
        Theme t;
        t.name = "Cyberpunk Neon";
        t.description = "Neon cyberpunk aesthetic";
        t.author = "System";
        
        t.background = ThemeColor(0xFF0A0A16);
        t.surface = ThemeColor(0xFF151525);
        t.surfaceVariant = ThemeColor(0xFF202035);
        t.primary = ThemeColor(0xFF00F0FF);    // Cyan neon
        t.secondary = ThemeColor(0xFFFF00A0);   // Pink neon
        t.accent = ThemeColor(0xFF39FF14);      // Green neon
        t.error = ThemeColor(0xFFFF003C);
        t.warning = ThemeColor(0xFFFFD700);
        
        t.textPrimary = ThemeColor(0xFFE0E0FF);
        t.textSecondary = ThemeColor(0xFF8080A0);
        
        // Neon effect shadows
        t.shadowSmall.color = ThemeColor(0x8000F0FF);
        t.shadowSmall.blur = 8.0f;
        
        t.borderRadius = 2.0f;
        
        registerTheme(t);
    }
    
    LOGD("Created %zu builtin themes", themes_.size());
}

void ThemeManager::registerTheme(const Theme& theme) {
    auto t = std::make_unique<Theme>(theme);
    themes_[theme.name] = std::move(t);
    LOGD("Registered theme: %s", theme.name.c_str());
}

void ThemeManager::unregisterTheme(const std::string& name) {
    if (currentTheme_ && currentTheme_->name == name) {
        LOGE("Cannot unregister active theme: %s", name.c_str());
        return;
    }
    themes_.erase(name);
}

Theme* ThemeManager::getTheme(const std::string& name) {
    auto it = themes_.find(name);
    if (it != themes_.end()) return it->second.get();
    return nullptr;
}

Theme* ThemeManager::getCurrentTheme() {
    return currentTheme_;
}

std::vector<std::string> ThemeManager::getThemeNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : themes_) {
        names.push_back(name);
    }
    return names;
}

bool ThemeManager::setActiveTheme(const std::string& name, bool animate) {
    auto* theme = getTheme(name);
    if (!theme) {
        LOGE("Theme not found: %s", name.c_str());
        return false;
    }
    
    if (currentTheme_ && animate) {
        // Start animation
        transitionSource_ = *currentTheme_;
        transitionTarget_ = *theme;
        transitionProgress_ = 0.0f;
        transitioning_ = true;
    }
    
    currentTheme_ = theme;
    currentThemeName_ = name;
    
    // Notify listeners
    for (auto& cb : listeners_) {
        cb(*theme);
    }
    
    LOGD("Active theme changed to: %s", name.c_str());
    return true;
}

std::string ThemeManager::getActiveThemeName() const {
    return currentThemeName_;
}

void ThemeManager::update(float deltaTime) {
    if (!transitioning_) return;
    
    transitionProgress_ += deltaTime / transitionDuration_;
    if (transitionProgress_ >= 1.0f) {
        transitionProgress_ = 1.0f;
        transitioning_ = false;
    }
}

ThemeColor ThemeManager::getColor(const std::string& colorName) const {
    if (!currentTheme_) return ThemeColor();
    
    // Check overrides
    auto it = overrides_.find(colorName);
    if (it != overrides_.end()) return it->second;
    
    // During transition, interpolate between source and target
    if (transitioning_) {
        ThemeColor src = const_cast<ThemeManager*>(this)->getColorFromTheme(transitionSource_, colorName);
        ThemeColor tgt = const_cast<ThemeManager*>(this)->getColorFromTheme(transitionTarget_, colorName);
        return ThemeColor::lerp(src, tgt, transitionProgress_);
    }
    
    return const_cast<ThemeManager*>(this)->getColorFromTheme(*currentTheme_, colorName);
}

ThemeColor ThemeManager::getColorFromTheme(const Theme& theme, const std::string& name) {
    if (name == "background") return theme.background;
    if (name == "surface") return theme.surface;
    if (name == "surfaceVariant") return theme.surfaceVariant;
    if (name == "primary") return theme.primary;
    if (name == "primaryVariant") return theme.primaryVariant;
    if (name == "secondary") return theme.secondary;
    if (name == "accent") return theme.accent;
    if (name == "error") return theme.error;
    if (name == "warning") return theme.warning;
    if (name == "success") return theme.success;
    if (name == "info") return theme.info;
    if (name == "textPrimary") return theme.textPrimary;
    if (name == "textSecondary") return theme.textSecondary;
    if (name == "textDisabled") return theme.textDisabled;
    if (name == "textOnPrimary") return theme.textOnPrimary;
    if (name == "textOnSecondary") return theme.textOnSecondary;
    if (name == "button") return theme.button;
    if (name == "buttonHover") return theme.buttonHover;
    if (name == "buttonPressed") return theme.buttonPressed;
    if (name == "buttonDisabled") return theme.buttonDisabled;
    if (name == "panel") return theme.panel;
    if (name == "panelHeader") return theme.panelHeader;
    if (name == "panelBorder") return theme.panelBorder;
    if (name == "tab") return theme.tab;
    if (name == "tabActive") return theme.tabActive;
    if (name == "tabHover") return theme.tabHover;
    if (name == "scrollbar") return theme.scrollbar;
    if (name == "scrollbarThumb") return theme.scrollbarThumb;
    if (name == "highlight") return theme.highlight;
    if (name == "selection") return theme.selection;
    if (name == "overlay") return theme.overlay;
    
    return ThemeColor();
}

void ThemeManager::setColorOverride(const std::string& colorName, const ThemeColor& color) {
    overrides_[colorName] = color;
}

void ThemeManager::clearOverrides() {
    overrides_.clear();
}

void ThemeManager::addChangeListener(ThemeChangeCallback callback) {
    listeners_.push_back(callback);
}
