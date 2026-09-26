#pragma once

#include <cstdint>
#include <cctype>
#include <string>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <android/log.h>
// loadFromESM() consumes WTHR records, so the NAM0 field indices it indexes
// with come from the ESM reader's WeatherData layout.
#include "../assets/esm_reader.h"

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

// Phase 66 P19: one time-of-day colour set from a WTHR record's NAM0 subrecord.
// Oblivion stores four of these per weather (Sunrise, Day, Sunset, Night) and
// blends them by the clock. Rendering only the Day set is what made noon and
// midnight look identical.
struct SkyTimeSet {
    float upperSky[3] = {0.2f, 0.4f, 0.8f};
    float fog[3] = {0.6f, 0.7f, 0.9f};
    float clouds[3] = {0.9f, 0.9f, 0.95f};
};

// NAM0 order, which is also the order of the key hours below.
enum SkyTimeSetIndex : int {
    SKY_SUNRISE = 0,
    SKY_DAY = 1,
    SKY_SUNSET = 2,
    SKY_NIGHT = 3,
    SKY_TIME_SET_COUNT = 4
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
        startWeather_ = currentWeather_;

        // Set initial time
        gameTime_ = 10.0f; // 10:00 AM
        timeScale_ = 30.0f; // 30x real time (1 real second = 30 game seconds)

        // Seed every value derived from the clock so frame 0 already matches the
        // time of day instead of holding whatever the presets defaulted to.
        updateTimeOfDay();
        updateSunPosition();
        updateMoonPosition();
        updateSkyFromTimeOfDay();

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

        // Advance game time. timeScale_ follows Oblivion's convention of game
        // minutes per real minute (30 = the default, i.e. a full day in 48 real
        // minutes), so one real second is timeScale_/3600 hours. The old code added
        // dt*timeScale_ straight to the hour counter -- 30 hours per real second,
        // which raced a whole day past in about a second.
        const float hours = dt * timeScale_ / 3600.0f;
        if (hours > 0.0f) {
            const float total = gameTime_ + hours;
            const float days = std::floor(total / 24.0f);
            gameTime_ = total - days * 24.0f;
            dayCount_ += static_cast<uint32_t>(days);
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

        // Re-derive the sky from the clock now that the sun's elevation (and so
        // the daylight factor) is known for this frame.
        updateSkyFromTimeOfDay();

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
        // Snapshot the current state so the transition interpolates from a fixed
        // origin. Lerping currentWeather_ toward targetWeather_ in place would
        // converge geometrically and finish in a fraction of transitionTime.
        startWeather_ = currentWeather_;
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
        startWeather_ = currentWeather_;
        transitionProgress_ = 1.0f;
        // The sky colours are derived, so they have to be recomputed here or the
        // console's "setweather" would leave the previous sky on screen.
        updateSkyFromTimeOfDay();
    }

    WeatherType getCurrentWeatherType() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentWeather_.type;
    }

    // --- Time control ---

    void setGameTime(float hours) {
        std::lock_guard<std::mutex> lock(mutex_);
        gameTime_ = std::fmod(hours, 24.0f);
        if (gameTime_ < 0.0f) gameTime_ += 24.0f;

        // Apply immediately rather than waiting for the next update(), so the
        // console's "settime" changes the world on the same frame it is typed.
        updateTimeOfDay();
        updateSunPosition();
        updateMoonPosition();
        updateSkyFromTimeOfDay();
    }

    float getGameTime() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return gameTime_;
    }

    // 1.0 in full daylight, 0.0 once the sun is below the horizon. Exposed so a
    // host test can pin the day/night curve without a device.
    float getDaylight() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return daylight_;
    }

    // Game minutes per real minute, as in Oblivion's "set timescale" command, so
    // 30 is the retail default and 0 freezes the clock (which is what the host
    // tests use to keep the derived sky colours constant while they measure the
    // weather ramp).
    void setTimeScale(float scale) {
        std::lock_guard<std::mutex> lock(mutex_);
        timeScale_ = std::max(0.0f, scale);
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

    // ========================================================================
    // Phase 66: Drive the weather presets from real Oblivion.esm WTHR records.
    //
    // The ESM stores 37 WTHR records, each with four time-of-day sky colour
    // sets (NAM0: Sunrise/Day/Sunset/Night) plus fog distances (FNAM) and a
    // wind speed (DATA). We classify each record into one of the eight
    // WeatherType buckets using its editorID (Oblivion's naming is stable:
    // "Clear", "Cloudy", "Fog", "Overcast", "Rain", "Thunderstorm", "Snow",
    // "Blight") and then blend the real colours into the matching preset.
    //
    // Colours in the ESM are BGRA uint32; we convert to linear-ish float RGB.
    // ========================================================================
    template <typename WeatherList>
    int loadFromESM(const WeatherList& weathers) {
        std::lock_guard<std::mutex> lock(mutex_);

        int applied = 0;
        for (const auto& w : weathers) {
            const WeatherType type = classifyWeather(w.editorID);
            if (type == WeatherType::COUNT) continue;

            auto& preset = weatherPresets_[static_cast<size_t>(type)];
            auto& curve = skyCurve_[static_cast<size_t>(type)];

            // NAM0 is field-major: 10 colour fields, each holding the four
            // Sunrise/Day/Sunset/Night slot values (BGRA). We keep the sky-upper
            // and clouds-upper fields per slot so the clock can blend the
            // record's real sunrise/day/sunset/night colours instead of painting
            // the same sky at noon and at midnight. A field the record does not
            // define keeps the hand-authored fallback.
            bool slotProvided[SKY_TIME_SET_COUNT] = {false, false, false, false};
            for (int s = 0; s < SKY_TIME_SET_COUNT; ++s) {
                const uint32_t upper = w.nam0[oblivion::NAM0_FIELD_SKY_UPPER].slot[s];
                const uint32_t fog = w.nam0[oblivion::NAM0_FIELD_FOG].slot[s];
                const uint32_t clouds = w.nam0[oblivion::NAM0_FIELD_CLOUDS_UPPER].slot[s];
                slotProvided[s] = (upper != 0);
                if (upper != 0) bgraToRgb(upper, curve[s].upperSky);
                if (fog != 0) bgraToRgb(fog, curve[s].fog);
                if (clouds != 0) bgraToRgb(clouds, curve[s].clouds);
            }

            // The Day slot is also the preset's flat colour, which stays the
            // reference for anything that inspects a preset rather than the sky.
            const uint32_t dayUpper = w.nam0[oblivion::NAM0_FIELD_SKY_UPPER].slot[SKY_DAY];
            const uint32_t dayFog = w.nam0[oblivion::NAM0_FIELD_FOG].slot[SKY_DAY];
            const uint32_t dayClouds = w.nam0[oblivion::NAM0_FIELD_CLOUDS_UPPER].slot[SKY_DAY];
            if (dayUpper != 0) {
                bgraToRgb(dayUpper, preset.sky.zenith);
            }
            if (dayFog != 0) {
                bgraToRgb(dayFog, preset.sky.horizon);
            }
            if (dayClouds != 0) {
                bgraToRgb(dayClouds, preset.clouds.color);
            }

            // A set the record omitted must be re-derived from the colours we just
            // loaded, otherwise it would still be built from the placeholder preset
            // and the sky would jump to an unrelated hue at dusk.
            for (int s = 0; s < SKY_TIME_SET_COUNT; ++s) {
                if (s != SKY_DAY && !slotProvided[s]) deriveSkyTimeSet(preset, s, curve[s]);
            }

            // Fog distances: FNAM day near/far. Oblivion stores these in game
            // units; the renderer treats visibility as the far plane distance.
            if (w.fogDayFar > 0.0f) {
                preset.visibility = w.fogDayFar;
            }

            // Wind speed (DATA offset 0, 0-255 scale).
            preset.windSpeed = static_cast<float>(w.windSpeed) * 0.1f;

            applied++;
        }

        // Re-seed the live state from the (now ESM-backed) presets so the very
        // first frame already reflects real data.
        currentWeather_ = weatherPresets_[static_cast<size_t>(currentWeather_.type)];
        targetWeather_ = currentWeather_;
        updateSkyFromTimeOfDay();

        LOGI_SKY("SkyWeatherSystem: applied %d WTHR records from ESM", applied);
        return applied;
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
    WeatherState startWeather_;
    float transitionProgress_ = 1.0f;

    std::array<WeatherState, static_cast<size_t>(WeatherType::COUNT)> weatherPresets_;

    // Per-weather NAM0 curve: [WeatherType][Sunrise|Day|Sunset|Night].
    std::array<std::array<SkyTimeSet, SKY_TIME_SET_COUNT>,
               static_cast<size_t>(WeatherType::COUNT)> skyCurve_;

    // Fraction of the day's light reaching the ground: 1 at noon, 0 after dark.
    float daylight_ = 1.0f;

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

        // Daylight factor for the sky and ambient curves. Smoothstepped over
        // roughly +/-1 hour of the horizon crossing so dawn and dusk fade instead
        // of snapping from night to noon.
        const float elevation = currentWeather_.sun.position[1];
        const float t = std::clamp((elevation + 0.30f) / 0.60f, 0.0f, 1.0f);
        daylight_ = t * t * (3.0f - 2.0f * t);
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

    // --- Phase 66 P19: sky and ambient light as a function of the clock -------

    // Tent weight for one of the four NAM0 key hours. The keys sit six hours
    // apart on the 24-hour circle, so tents of half-width six sum to exactly one
    // at every clock value: no normalisation and no seam at midnight.
    float skyTimeWeight(float keyHour) const {
        float d = std::fabs(gameTime_ - keyHour);
        if (d > 12.0f) d = 24.0f - d;
        return std::max(0.0f, 1.0f - d / 6.0f);
    }

    // The three non-Day sets derived from a preset's own colours, so the clock
    // always has something plausible to blend towards when a WTHR record does not
    // supply a set. `out` is only touched when `set` names a non-Day entry.
    static void deriveSkyTimeSet(const WeatherState& preset, int set, SkyTimeSet& out) {
        for (int i = 0; i < 3; ++i) {
            switch (set) {
                case SKY_SUNRISE:
                    out.upperSky[i] = lerp(preset.sky.zenith[i], kSunriseTint[i], 0.55f) * 0.85f;
                    out.fog[i] = lerp(preset.sky.horizon[i], kSunriseTint[i], 0.75f);
                    out.clouds[i] = lerp(preset.clouds.color[i], kSunriseTint[i], 0.45f);
                    break;
                case SKY_SUNSET:
                    out.upperSky[i] = lerp(preset.sky.zenith[i], kSunsetTint[i], 0.60f) * 0.60f;
                    out.fog[i] = lerp(preset.sky.horizon[i], kSunsetTint[i], 0.80f) * 0.85f;
                    out.clouds[i] = lerp(preset.clouds.color[i], kSunsetTint[i], 0.50f) * 0.80f;
                    break;
                default:  // SKY_NIGHT
                    // Keep a trace of the preset so overcast stays overcast, but
                    // collapse towards a dark cold sky rather than daytime blue.
                    out.upperSky[i] = preset.sky.zenith[i] * 0.10f + kNightSky[i];
                    out.fog[i] = preset.sky.horizon[i] * 0.12f + kNightSky[i];
                    out.clouds[i] = preset.clouds.color[i] * 0.18f;
                    break;
            }
        }
    }

    // Fill a whole curve from the preset alone. Used at startup, before any WTHR
    // record has been read.
    void buildDefaultSkyCurve(WeatherType type) {
        const auto& preset = weatherPresets_[static_cast<size_t>(type)];
        auto& curve = skyCurve_[static_cast<size_t>(type)];

        for (int i = 0; i < 3; ++i) {
            curve[SKY_DAY].upperSky[i] = preset.sky.zenith[i];
            curve[SKY_DAY].fog[i] = preset.sky.horizon[i];
            curve[SKY_DAY].clouds[i] = preset.clouds.color[i];
        }
        for (int s = 0; s < SKY_TIME_SET_COUNT; ++s) {
            if (s != SKY_DAY) deriveSkyTimeSet(preset, s, curve[s]);
        }
    }

    // Blend the four time sets of the current (or in-flight) weather and store the
    // result in currentWeather_, which is what the renderer reads. Called every
    // frame; also called directly by setGameTime/setWeatherImmediate so a console
    // command takes effect without waiting for the next frame.
    void updateSkyFromTimeOfDay() {
        const size_t from = static_cast<size_t>(startWeather_.type);
        const size_t to = static_cast<size_t>(targetWeather_.type);
        const float t = transitionProgress_;

        const float weights[SKY_TIME_SET_COUNT] = {
            skyTimeWeight(kSkyKeyHour[SKY_SUNRISE]),
            skyTimeWeight(kSkyKeyHour[SKY_DAY]),
            skyTimeWeight(kSkyKeyHour[SKY_SUNSET]),
            skyTimeWeight(kSkyKeyHour[SKY_NIGHT]),
        };

        float upperSky[3] = {0.0f, 0.0f, 0.0f};
        float fog[3] = {0.0f, 0.0f, 0.0f};
        float clouds[3] = {0.0f, 0.0f, 0.0f};
        for (int s = 0; s < SKY_TIME_SET_COUNT; ++s) {
            const auto& a = skyCurve_[from][static_cast<size_t>(s)];
            const auto& b = skyCurve_[to][static_cast<size_t>(s)];
            for (int i = 0; i < 3; ++i) {
                upperSky[i] += weights[s] * lerp(a.upperSky[i], b.upperSky[i], t);
                fog[i] += weights[s] * lerp(a.fog[i], b.fog[i], t);
                clouds[i] += weights[s] * lerp(a.clouds[i], b.clouds[i], t);
            }
        }

        for (int i = 0; i < 3; ++i) {
            currentWeather_.sky.zenith[i] = upperSky[i];
            currentWeather_.sky.horizon[i] = fog[i];
            currentWeather_.clouds.color[i] = clouds[i];

            // Ambient light is sky bounce by day and a cold moonlight floor by
            // night. Deriving it from the blended sky keeps the fill light
            // consistent with what the player is looking at instead of the
            // hard-coded constant that made midnight as bright as noon.
            const float skyBounce = 0.5f * lerp(upperSky[i], 1.0f, 0.5f);
            currentWeather_.sky.ambient[i] = lerp(kNightAmbient[i], skyBounce, daylight_);
        }
    }

    void interpolateWeather() {
        float t = transitionProgress_;
        // Interpolate from the snapshot taken when the transition started, not
        // from the partially-updated current state.
        //
        // Sky colours are deliberately absent here: zenith/horizon/clouds/ambient
        // are owned by updateSkyFromTimeOfDay(), which blends the two weathers'
        // NAM0 curves and the clock together. Lerping them here as well would just
        // be overwritten a few lines later.
        // Lerp fog/visibility
        currentWeather_.visibility = lerp(startWeather_.visibility,
                                           targetWeather_.visibility, t);
        // Lerp wind
        currentWeather_.windSpeed = lerp(startWeather_.windSpeed,
                                          targetWeather_.windSpeed, t);
        // Lerp clouds
        currentWeather_.clouds.coverage = lerp(startWeather_.clouds.coverage,
                                                targetWeather_.clouds.coverage, t);

        if (transitionProgress_ >= 1.0f) {
            currentWeather_.type = targetWeather_.type;
        }
    }

    static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // Key hour of each NAM0 set: sunrise 06:00, day 12:00, sunset 18:00, night 00:00.
    static constexpr float kSkyKeyHour[SKY_TIME_SET_COUNT] = {6.0f, 12.0f, 18.0f, 0.0f};
    static constexpr float kSunriseTint[3] = {1.00f, 0.62f, 0.35f};
    static constexpr float kSunsetTint[3] = {1.00f, 0.45f, 0.25f};
    static constexpr float kNightSky[3] = {0.02f, 0.03f, 0.07f};
    static constexpr float kNightAmbient[3] = {0.10f, 0.11f, 0.17f};

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

        // Overcast. This bucket had no preset at all, so it silently kept the
        // default-constructed state whose `type` is CLEAR: setweather overcast
        // reported itself as clear and blended the clear sky curve.
        auto& overcast = weatherPresets_[static_cast<size_t>(WeatherType::OVERCAST)];
        overcast.type = WeatherType::OVERCAST;
        overcast.sky.zenith[0] = 0.45f; overcast.sky.zenith[1] = 0.47f; overcast.sky.zenith[2] = 0.50f;
        overcast.sky.horizon[0] = 0.55f; overcast.sky.horizon[1] = 0.57f; overcast.sky.horizon[2] = 0.60f;
        overcast.clouds.coverage = 0.85f;
        overcast.visibility = 600.0f;
        overcast.windSpeed = 3.0f;

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

        // Every preset needs all four time sets before anything can blend them;
        // loadFromESM() overwrites the entries a WTHR record actually provides.
        for (int t = 0; t < static_cast<int>(WeatherType::COUNT); ++t) {
            buildDefaultSkyCurve(static_cast<WeatherType>(t));
        }
    }

    // Map an Oblivion WTHR editorID onto one of the eight preset buckets.
    static WeatherType classifyWeather(const std::string& editorID) {
        // Lower-case copy for case-insensitive matching.
        std::string id;
        id.reserve(editorID.size());
        for (char c : editorID) {
            id.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        if (id.find("blight") != std::string::npos) return WeatherType::BLIZZARD;
        if (id.find("thunder") != std::string::npos) return WeatherType::THUNDER;
        if (id.find("snow") != std::string::npos) return WeatherType::SNOW;
        if (id.find("rain") != std::string::npos) return WeatherType::RAIN;
        if (id.find("fog") != std::string::npos) return WeatherType::FOGGY;
        if (id.find("overcast") != std::string::npos) return WeatherType::OVERCAST;
        if (id.find("cloud") != std::string::npos) return WeatherType::CLOUDY;
        if (id.find("clear") != std::string::npos) return WeatherType::CLEAR;
        return WeatherType::COUNT;
    }

    // Console-facing helpers. These live in the public interface because
    // `renderer.cpp` resolves `setweather <name>` through them and the tests pin
    // the name table, while `classifyWeather` stays private because it only ever
    // serves WTHR editor IDs from inside this class.
public:
    // Console names ("storm", "fog", ...) are shorter than the WTHR editor IDs
    // classifyWeather expects, so resolve those first and only then fall back to
    // substring matching for real editor IDs. Returns WeatherType::COUNT when the
    // name matches nothing.
    static WeatherType weatherTypeFromName(const std::string& name) {
        std::string id;
        id.reserve(name.size());
        for (char c : name) {
            id.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        if (id == "clear" || id == "sunny") return WeatherType::CLEAR;
        if (id == "cloudy" || id == "clouds") return WeatherType::CLOUDY;
        if (id == "overcast") return WeatherType::OVERCAST;
        if (id == "fog" || id == "foggy") return WeatherType::FOGGY;
        if (id == "rain" || id == "rainy") return WeatherType::RAIN;
        if (id == "storm" || id == "thunder" || id == "thunderstorm") {
            return WeatherType::THUNDER;
        }
        if (id == "snow" || id == "snowy") return WeatherType::SNOW;
        if (id == "blizzard") return WeatherType::BLIZZARD;
        return classifyWeather(id);
    }

    // Inverse of weatherTypeFromName(), for console output and the HUD.
    static const char* weatherTypeName(WeatherType type) {
        switch (type) {
            case WeatherType::CLEAR:    return "clear";
            case WeatherType::CLOUDY:   return "cloudy";
            case WeatherType::FOGGY:    return "fog";
            case WeatherType::OVERCAST: return "overcast";
            case WeatherType::RAIN:     return "rain";
            case WeatherType::THUNDER:  return "storm";
            case WeatherType::SNOW:     return "snow";
            case WeatherType::BLIZZARD: return "blizzard";
            default:                    return "unknown";
        }
    }

private:
    // ESM colours are packed 0x00BBGGRR (byte 0 = R, byte 1 = G, byte 2 = B,
    // byte 3 reserved). Despite the historical name this reads the bytes in
    // RGB order; do not swap the shifts when "fixing" it.
    static void bgraToRgb(uint32_t bgra, float out[3]) {
        const float b = static_cast<float>((bgra >> 16) & 0xFF) / 255.0f;
        const float g = static_cast<float>((bgra >> 8) & 0xFF) / 255.0f;
        const float r = static_cast<float>(bgra & 0xFF) / 255.0f;
        out[0] = r;
        out[1] = g;
        out[2] = b;
    }
};

} // namespace engine

