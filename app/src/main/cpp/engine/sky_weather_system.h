#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_SKY "SkyWeather"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_SKY(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_SKY, __VA_ARGS__)
#else
#define LOGD_SKY(...) do {} while(0)
#endif
#define LOGI_SKY(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SKY, __VA_ARGS__)

// ============================================================================
// Sky & Weather System
// Phase 56: Oblivion's sky dome, day/night cycle, weather states
// ============================================================================

namespace engine {

// Time of day (Oblivion uses 24-hour cycle)
enum class TimeOfDay : uint8_t {
    DAWN = 0,       // 5:00 - 7:00
    MORNING,        // 7:00 - 10:00
    MIDDAY,         // 10:00 - 14:00
    AFTERNOON,      // 14:00 - 17:00
    DUSK,           // 17:00 - 19:00
    EVENING,        // 19:00 - 21:00
    NIGHT,          // 21:00 - 5:00
    COUNT
};

// Weather types (from Oblivion.esm WTHR records)
enum class WeatherType : uint8_t {
    CLEAR = 0,
    CLOUDY,
    FOGGY,
    OVERCAST,
    RAIN,
    THUNDER,
    SNOW,
    BLIZZARD,
    COUNT
};

// Sky color gradient (zenith to horizon)
struct SkyGradient {
    float zenith[3] = {0.2f, 0.4f, 0.8f};      // Top of sky
    float horizon[3] = {0.6f, 0.7f, 0.9f};      // Horizon
    float ambient[3] = {0.3f, 0.3f, 0.4f};      // Ambient light
};

// Sun/Moon parameters
struct CelestialBody {
    float position[3] = {0.0f, 1.0f, 0.0f};    // Direction
    float color[3] = {1.0f, 0.95f, 0.8f};
    float intensity = 1.0f;
    float size = 1.0f;                           // Angular size
};

// Star field parameters
struct StarField {
    float density = 1000.0f;     // Number of stars
    float brightness = 1.0f;
    float twinkleSpeed = 2.0f;
    bool visible = false;
};

// Cloud layer
struct CloudLayer {
    float coverage = 0.3f;       // 0.0 = clear, 1.0 = overcast
    float density = 0.5f;
    float speed = 0.1f;          // Movement speed
    float direction[2] = {1.0f, 0.0f};
    float altitude = 2000.0f;
    float color[3] = {0.9f, 0.9f, 0.95f};
    float thickness = 500.0f;
};

// Weather state
struct WeatherState {
    WeatherType type = WeatherType::CLEAR;
    SkyGradient sky;
    CelestialBody sun;
    CelestialBody moon;
    StarField stars;
    CloudLayer clouds;
    float windSpeed = 0.0f;
    float windDirection[2] = {1.0f, 0.0f};
    float temperature = 20.0f;   // Celsius
    float visibility = 1000.0f;  // Fog distance
    float transitionTime = 30.0f; // Seconds to transition
};

// ============================================================================
// SkyWeatherSystem - manages sky dome and weather
// ============================================================================

class SkyWeatherSystem {
public:
    static SkyWeatherSystem& instance() {
        static SkyWeatherSystem inst;
        return inst;
    }

    void init() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Initialize default weather states
        initWeatherPresets();

        // Set initial state
        currentWeather_ = weatherPresets_[static_cast<size_t>(WeatherType::CLEAR)];
        targetWeather_ = currentWeather_;

        // Set initial time
        gameTime_ = 10.0f; // 10:00 AM
        timeScale_ = 30.0f; // 30x real time (1 real second = 30 game seconds)

        initialized_ = true;
        LOGI_SKY("SkyWeatherSystem initialized");
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = false;
    }

    void update(float dt) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return;

        // Advance game time
        gameTime_ += dt * timeScale_;
        if (gameTime_ >= 24.0f) {
            gameTime_ -= 24.0f;
            dayCount_++;
        }

        // Update time of day
        updateTimeOfDay();

        // Weather transition
        if (transitionProgress_ < 1.0f) {
            transitionProgress_ += dt / currentWeather_.transitionTime;
            transitionProgress_ = std::min(1.0f, transitionProgress_);
            interpolateWeather();
        }

        // Update sun position
        updateSunPosition();

        // Update moon position (opposite of sun)
        updateMoonPosition();

        // Update star visibility
        currentWeather_.stars.visible = (currentTimeOfDay_ == TimeOfDay::NIGHT ||
                                          currentTimeOfDay_ == TimeOfDay::EVENING);

        // Update cloud movement
        currentWeather_.clouds.direction[0] += currentWeather_.windSpeed * dt * 0.01f;
    }

    // --- Weather control ---

    void setWeather(WeatherType type, float transitionTime = 30.0f) {
        std::lock_guard<std::mutex> lock(mutex_);
        targetWeather_ = weatherPresets_[static_cast<size_t>(type)];
        targetWeather_.transitionTime = transitionTime;
        transitionProgress_ = 0.0f;
        LOGI_SKY("Weather transition to %d in %.1f seconds",
                 static_cast<int>(type), transitionTime);
    }

    void setWeatherImmediate(WeatherType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        currentWeather_ = weatherPresets_[static_cast<size_t>(type)];
        targetWeather_ = currentWeather_;
        transitionProgress_ = 1.0f;
    }

    WeatherType getCurrentWeatherType() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentWeather_.type;
    }

    // --- Time control ---

    void setGameTime(float hours) {
        std::lock_guard<std::mutex> lock(mutex_);
        gameTime_ = std::fmod(hours, 24.0f);
    }

    float getGameTime() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return gameTime_;
    }

    void setTimeScale(float scale) {
        std::lock_guard<std::mutex> lock(mutex_);
        timeScale_ = scale;
    }

    TimeOfDay getTimeOfDay() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentTimeOfDay_;
    }

    uint32_t getDayCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return dayCount_;
    }

    // --- Sky data access ---

    const WeatherState& getCurrentWeather() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentWeather_;
    }

    // Get sun direction for lighting
    void getSunDirection(float& x, float& y, float& z) const {
        std::lock_guard<std::mutex> lock(mutex_);
        x = currentWeather_.sun.position[0];
        y = currentWeather_.sun.position[1];
        z = currentWeather_.sun.position[2];
    }

    // Get ambient color based on time of day
    void getAmbientColor(float& r, float& g, float& b) const {
        std::lock_guard<std::mutex> lock(mutex_);
        r = currentWeather_.sky.ambient[0];
        g = currentWeather_.sky.ambient[1];
        b = currentWeather_.sky.ambient[2];
    }

    // Get fog parameters for PostProcessPipeline
    float getFogDistance() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentWeather_.visibility;
    }

    void getFogColor(float& r, float& g, float& b) const {
        std::lock_guard<std::mutex> lock(mutex_);
        r = currentWeather_.sky.horizon[0];
        g = currentWeather_.sky.horizon[1];
        b = currentWeather_.sky.horizon[2];
    }

    // Generate sky shader
    std::string generateSkyShader() const {
        std::string src;
        src += "#version 300 es\n";
        src += "precision highp float;\n";
        src += "in vec3 vDirection;\n";
        src += "out vec4 fragColor;\n";
        src += "uniform vec3 uZenithColor;\n";
        src += "uniform vec3 uHorizonColor;\n";
        src += "uniform vec3 uSunDir;\n";
        src += "uniform vec3 uSunColor;\n";
        src += "uniform float uSunIntensity;\n";
        src += "uniform float uTime;\n";
        src += "\nvoid main() {\n";
        src += "    vec3 dir = normalize(vDirection);\n";
        src += "    float y = dir.y * 0.5 + 0.5;\n";
        src += "    vec3 sky = mix(uHorizonColor, uZenithColor, pow(y, 0.4));\n";
        src += "    // Sun disc\n";
        src += "    float sunDot = max(dot(dir, normalize(uSunDir)), 0.0);\n";
        src += "    float sunDisc = smoothstep(0.9995, 0.9998, sunDot);\n";
        src += "    float sunGlow = pow(max(sunDot, 0.0), 64.0) * 0.5;\n";
        src += "    sky += uSunColor * (sunDisc * uSunIntensity + sunGlow);\n";
        src += "    // Horizon glow\n";
        src += "    float horizonGlow = pow(1.0 - abs(dir.y), 8.0);\n";
        src += "    sky += uSunColor * horizonGlow * 0.2;\n";
        src += "    fragColor = vec4(sky, 1.0);\n";
        src += "}\n";
        return src;
    }

private:
    SkyWeatherSystem() = default;

    bool initialized_ = false;
    float gameTime_ = 10.0f;
    float timeScale_ = 30.0f;
    uint32_t dayCount_ = 0;
    TimeOfDay currentTimeOfDay_ = TimeOfDay::MIDDAY;

    WeatherState currentWeather_;
    WeatherState targetWeather_;
    float transitionProgress_ = 1.0f;

    std::array<WeatherState, static_cast<size_t>(WeatherType::COUNT)> weatherPresets_;

    mutable std::mutex mutex_;

    void updateTimeOfDay() {
        if (gameTime_ >= 5.0f && gameTime_ < 7.0f)
            currentTimeOfDay_ = TimeOfDay::DAWN;
        else if (gameTime_ >= 7.0f && gameTime_ < 10.0f)
            currentTimeOfDay_ = TimeOfDay::MORNING;
        else if (gameTime_ >= 10.0f && gameTime_ < 14.0f)
            currentTimeOfDay_ = TimeOfDay::MIDDAY;
        else if (gameTime_ >= 14.0f && gameTime_ < 17.0f)
            currentTimeOfDay_ = TimeOfDay::AFTERNOON;
        else if (gameTime_ >= 17.0f && gameTime_ < 19.0f)
            currentTimeOfDay_ = TimeOfDay::DUSK;
        else if (gameTime_ >= 19.0f && gameTime_ < 21.0f)
            currentTimeOfDay_ = TimeOfDay::EVENING;
        else
            currentTimeOfDay_ = TimeOfDay::NIGHT;
    }

    void updateSunPosition() {
        // Sun arc based on game time
        float angle = (gameTime_ - 6.0f) / 12.0f * 3.1415926f; // 6AM = horizon, 12PM = zenith
        currentWeather_.sun.position[0] = std::cos(angle);
        currentWeather_.sun.position[1] = std::sin(angle);
        currentWeather_.sun.position[2] = 0.3f;

        // Sun color based on elevation
        if (currentWeather_.sun.position[1] < 0.2f) {
            // Sunrise/sunset - orange
            currentWeather_.sun.color[0] = 1.0f;
            currentWeather_.sun.color[1] = 0.5f;
            currentWeather_.sun.color[2] = 0.2f;
        } else {
            // Daytime - white/yellow
            currentWeather_.sun.color[0] = 1.0f;
            currentWeather_.sun.color[1] = 0.95f;
            currentWeather_.sun.color[2] = 0.8f;
        }

        // Intensity based on elevation
        currentWeather_.sun.intensity = std::max(0.0f, currentWeather_.sun.position[1]) * 1.5f;
    }

    void updateMoonPosition() {
        // Moon opposite to sun
        currentWeather_.moon.position[0] = -currentWeather_.sun.position[0];
        currentWeather_.moon.position[1] = -currentWeather_.sun.position[1];
        currentWeather_.moon.position[2] = -currentWeather_.sun.position[2];
        currentWeather_.moon.color[0] = 0.7f;
        currentWeather_.moon.color[1] = 0.75f;
        currentWeather_.moon.color[2] = 0.9f;
        currentWeather_.moon.intensity = std::max(0.0f, -currentWeather_.sun.position[1]) * 0.3f;
    }

    void interpolateWeather() {
        float t = transitionProgress_;
        // Lerp sky colors
        for (int i = 0; i < 3; i++) {
            currentWeather_.sky.zenith[i] = lerp(currentWeather_.sky.zenith[i],
                                                  targetWeather_.sky.zenith[i], t);
            currentWeather_.sky.horizon[i] = lerp(currentWeather_.sky.horizon[i],
                                                   targetWeather_.sky.horizon[i], t);
            currentWeather_.sky.ambient[i] = lerp(currentWeather_.sky.ambient[i],
                                                    targetWeather_.sky.ambient[i], t);
        }
        // Lerp fog/visibility
        currentWeather_.visibility = lerp(currentWeather_.visibility,
                                           targetWeather_.visibility, t);
        // Lerp wind
        currentWeather_.windSpeed = lerp(currentWeather_.windSpeed,
                                          targetWeather_.windSpeed, t);
        // Lerp clouds
        currentWeather_.clouds.coverage = lerp(currentWeather_.clouds.coverage,
                                                targetWeather_.clouds.coverage, t);

        if (transitionProgress_ >= 1.0f) {
            currentWeather_.type = targetWeather_.type;
        }
    }

    static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    void initWeatherPresets() {
        // Clear
        auto& clear = weatherPresets_[static_cast<size_t>(WeatherType::CLEAR)];
        clear.type = WeatherType::CLEAR;
        clear.sky.zenith[0] = 0.2f; clear.sky.zenith[1] = 0.4f; clear.sky.zenith[2] = 0.8f;
        clear.sky.horizon[0] = 0.6f; clear.sky.horizon[1] = 0.7f; clear.sky.horizon[2] = 0.9f;
        clear.clouds.coverage = 0.2f;
        clear.visibility = 1000.0f;
        clear.windSpeed = 1.0f;

        // Cloudy
        auto& cloudy = weatherPresets_[static_cast<size_t>(WeatherType::CLOUDY)];
        cloudy.type = WeatherType::CLOUDY;
        cloudy.sky.zenith[0] = 0.4f; cloudy.sky.zenith[1] = 0.45f; cloudy.sky.zenith[2] = 0.55f;
        cloudy.sky.horizon[0] = 0.5f; cloudy.sky.horizon[1] = 0.55f; cloudy.sky.horizon[2] = 0.6f;
        cloudy.clouds.coverage = 0.6f;
        cloudy.visibility = 800.0f;
        cloudy.windSpeed = 3.0f;

        // Foggy
        auto& foggy = weatherPresets_[static_cast<size_t>(WeatherType::FOGGY)];
        foggy.type = WeatherType::FOGGY;
        foggy.sky.zenith[0] = 0.5f; foggy.sky.zenith[1] = 0.5f; foggy.sky.zenith[2] = 0.5f;
        foggy.sky.horizon[0] = 0.6f; foggy.sky.horizon[1] = 0.6f; foggy.sky.horizon[2] = 0.6f;
        foggy.clouds.coverage = 0.8f;
        foggy.visibility = 100.0f;
        foggy.windSpeed = 0.5f;

        // Rain
        auto& rain = weatherPresets_[static_cast<size_t>(WeatherType::RAIN)];
        rain.type = WeatherType::RAIN;
        rain.sky.zenith[0] = 0.25f; rain.sky.zenith[1] = 0.28f; rain.sky.zenith[2] = 0.35f;
        rain.sky.horizon[0] = 0.35f; rain.sky.horizon[1] = 0.38f; rain.sky.horizon[2] = 0.42f;
        rain.clouds.coverage = 0.9f;
        rain.visibility = 400.0f;
        rain.windSpeed = 5.0f;

        // Thunder
        auto& thunder = weatherPresets_[static_cast<size_t>(WeatherType::THUNDER)];
        thunder.type = WeatherType::THUNDER;
        thunder.sky.zenith[0] = 0.15f; thunder.sky.zenith[1] = 0.15f; thunder.sky.zenith[2] = 0.2f;
        thunder.sky.horizon[0] = 0.2f; thunder.sky.horizon[1] = 0.2f; thunder.sky.horizon[2] = 0.25f;
        thunder.clouds.coverage = 1.0f;
        thunder.visibility = 300.0f;
        thunder.windSpeed = 8.0f;

        // Snow
        auto& snow = weatherPresets_[static_cast<size_t>(WeatherType::SNOW)];
        snow.type = WeatherType::SNOW;
        snow.sky.zenith[0] = 0.5f; snow.sky.zenith[1] = 0.55f; snow.sky.zenith[2] = 0.6f;
        snow.sky.horizon[0] = 0.7f; snow.sky.horizon[1] = 0.75f; snow.sky.horizon[2] = 0.8f;
        snow.clouds.coverage = 0.7f;
        snow.visibility = 500.0f;
        snow.windSpeed = 4.0f;
        snow.temperature = -5.0f;

        // Blizzard
        auto& blizzard = weatherPresets_[static_cast<size_t>(WeatherType::BLIZZARD)];
        blizzard.type = WeatherType::BLIZZARD;
        blizzard.sky.zenith[0] = 0.6f; blizzard.sky.zenith[1] = 0.65f; blizzard.sky.zenith[2] = 0.7f;
        blizzard.sky.horizon[0] = 0.8f; blizzard.sky.horizon[1] = 0.85f; blizzard.sky.horizon[2] = 0.9f;
        blizzard.clouds.coverage = 1.0f;
        blizzard.visibility = 50.0f;
        blizzard.windSpeed = 15.0f;
        blizzard.temperature = -15.0f;
    }
};

} // namespace engine
