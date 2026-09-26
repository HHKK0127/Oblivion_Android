// Weather transition tests - see weather_transition_tests.h for the rationale.
//
// Drives SkyWeatherSystem through a scripted transition at a fixed 60 Hz step
// and asserts that the ramp is linear in time rather than the geometric decay
// the pre-P13 code produced.

#include "weather_transition_tests.h"

#include "../assets/esm_reader.h"
#include "../engine/sky_weather_system.h"

#include <chrono>
#include <cmath>

namespace {

constexpr float kStep = 1.0f / 60.0f;

// Advance the system by `seconds` at a fixed 60 Hz step.
void advance(engine::SkyWeatherSystem& sys, float seconds) {
    const int steps = static_cast<int>(seconds / kStep + 0.5f);
    for (int i = 0; i < steps; i++) sys.update(kStep);
}

bool nearlyEqual(float a, float b, float eps = 0.02f) {
    return std::fabs(a - b) < eps;
}

}  // namespace

void WeatherTransitionTests::record(const std::string& name, bool passed,
                                    const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int WeatherTransitionTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int WeatherTransitionTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string WeatherTransitionTests::getSummary() const {
    return "Weather Transition Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool WeatherTransitionTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    auto& sys = engine::SkyWeatherSystem::instance();
    sys.init();

    // Phase 66 P19: the sky colour is now a function of the clock, so the ramp
    // assertions below only hold while time is held still. Freeze the clock and
    // pin the hour; a separate test covers the clock advancing.
    sys.setTimeScale(0.0f);
    sys.setGameTime(10.0f);

    // Start from a known state: clear weather, no transition in flight.
    sys.setWeatherImmediate(engine::WeatherType::CLEAR);
    const float clearZenith = sys.getCurrentWeather().sky.zenith[2];

    // Begin a 30-second transition to a weather with a different sky colour.
    sys.setWeather(engine::WeatherType::CLOUDY, 30.0f);
    const float targetZenith = sys.getCurrentWeather().sky.zenith[2];  // still clear at t=0

    // Halfway through the transition the value must be about halfway between
    // the start and the target. The pre-P13 code was already ~99% of the way
    // there after 2 seconds, so this is the regression guard.
    advance(sys, 15.0f);
    const float midZenith = sys.getCurrentWeather().sky.zenith[2];

    // Sample the target preset by switching immediately in a scratch run.
    sys.setWeatherImmediate(engine::WeatherType::CLOUDY);
    const float cloudyZenith = sys.getCurrentWeather().sky.zenith[2];

    const float expectedMid = clearZenith + (cloudyZenith - clearZenith) * 0.5f;
    const bool midOk = nearlyEqual(midZenith, expectedMid, 0.03f);
    record("MidpointIsHalfway", midOk,
           midOk ? "zenith at 15s is halfway between start and target"
                 : "zenith at 15s is not halfway (transition is not linear)");

    // The transition must not have finished early: at 15s of a 30s ramp the
    // value must still be measurably short of the target.
    const bool notFinishedEarly = std::fabs(midZenith - cloudyZenith) > 0.02f;
    record("NotFinishedEarly", notFinishedEarly,
           notFinishedEarly ? "still transitioning at 15s"
                            : "transition collapsed early (geometric decay)");

    // After the full duration the transition must have completed and the
    // current weather type must have switched to the target.
    sys.setWeatherImmediate(engine::WeatherType::CLEAR);
    sys.setWeather(engine::WeatherType::CLOUDY, 30.0f);
    advance(sys, 31.0f);
    const bool typeSwitched =
        sys.getCurrentWeatherType() == engine::WeatherType::CLOUDY;
    record("CompletesAfterDuration", typeSwitched,
           typeSwitched ? "weather type switched after 30s"
                        : "weather type did not switch after the full duration");

    const bool reachedTarget =
        nearlyEqual(sys.getCurrentWeather().sky.zenith[2], cloudyZenith, 0.01f);
    record("ReachesTargetValue", reachedTarget,
           reachedTarget ? "zenith reached the target value"
                         : "zenith did not reach the target value");

    // setWeatherImmediate must apply the preset with no ramp at all.
    sys.setWeatherImmediate(engine::WeatherType::CLEAR);
    const bool immediate =
        nearlyEqual(sys.getCurrentWeather().sky.zenith[2], clearZenith, 0.001f) &&
        sys.getCurrentWeatherType() == engine::WeatherType::CLEAR;
    record("ImmediateAppliesAtOnce", immediate,
           immediate ? "immediate switch applied the preset"
                     : "immediate switch did not apply the preset");

    // --- Phase 66 P19: sky and light follow the clock ------------------------

    // Noon and midnight must not look the same. Before P19 only the NAM0 Day set
    // was ever loaded, so every hour produced the same zenith colour.
    sys.setWeatherImmediate(engine::WeatherType::CLEAR);
    sys.setGameTime(12.0f);
    const auto noon = sys.getCurrentWeather();
    const float noonDaylight = sys.getDaylight();

    sys.setGameTime(0.0f);
    const auto midnight = sys.getCurrentWeather();
    const float midnightDaylight = sys.getDaylight();

    const bool skyDiffers =
        std::fabs(noon.sky.zenith[2] - midnight.sky.zenith[2]) > 0.10f;
    record("NoonSkyDiffersFromMidnight", skyDiffers,
           skyDiffers ? "zenith colour changes between noon and midnight"
                      : "zenith colour is identical at noon and midnight");

    const bool ambientDiffers =
        std::fabs(noon.sky.ambient[0] - midnight.sky.ambient[0]) > 0.05f;
    record("NoonAmbientDiffersFromMidnight", ambientDiffers,
           ambientDiffers ? "ambient light changes between noon and midnight"
                          : "ambient light is identical at noon and midnight");

    // Midnight must be the darker of the two, not merely different.
    const bool midnightDarker =
        midnight.sky.ambient[0] < noon.sky.ambient[0] &&
        midnight.sun.intensity < noon.sun.intensity;
    record("MidnightIsDarker", midnightDarker,
           midnightDarker ? "midnight is darker than noon"
                          : "midnight is not darker than noon");

    const bool daylightCurve =
        noonDaylight > 0.9f && midnightDaylight < 0.05f;
    record("DaylightFollowsSun", daylightCurve,
           daylightCurve ? "daylight is 1 at noon and 0 at midnight"
                         : "daylight factor does not follow the sun");

    // The clock must advance at Oblivion's rate: timeScale is game minutes per
    // real second, so 60 real seconds at the default 30 must be 30 game minutes.
    // The pre-P19 code added dt*timeScale to the hour counter, i.e. 30 hours.
    sys.setGameTime(6.0f);
    sys.setTimeScale(30.0f);
    advance(sys, 60.0f);
    const float advanced = sys.getGameTime();
    const bool clockRateOk = nearlyEqual(advanced, 6.5f, 0.05f);
    record("ClockUsesOblivionRate", clockRateOk,
           clockRateOk ? "60s at timescale 30 advanced 30 game minutes"
                       : "clock advanced " + std::to_string(advanced - 6.0f) +
                             " hours instead of 0.5");
    sys.setTimeScale(0.0f);

    // The clock must survive wrapping past midnight rather than sticking above 24.
    sys.setGameTime(23.5f);
    sys.setTimeScale(30.0f);
    advance(sys, 120.0f);  // one game hour
    const float wrapped = sys.getGameTime();
    const bool wrapOk = wrapped >= 0.0f && wrapped < 1.0f;
    record("ClockWrapsPastMidnight", wrapOk,
           wrapOk ? "clock wrapped into the next day"
                  : "clock did not wrap (read " + std::to_string(wrapped) + ")");
    sys.setTimeScale(0.0f);

    // Overcast used to have no preset at all, so it reported itself as CLEAR and
    // blended the clear sky curve.
    sys.setWeatherImmediate(engine::WeatherType::OVERCAST);
    const bool overcastIsItself =
        sys.getCurrentWeatherType() == engine::WeatherType::OVERCAST;
    record("OvercastHasItsOwnPreset", overcastIsItself,
           overcastIsItself ? "overcast keeps its own weather type"
                            : "overcast collapsed onto another preset");

    // An unknown hour must still produce a usable sky: the four time sets blend
    // with weights that sum to one everywhere, including at the 00:00 seam.
    bool everyHourLit = true;
    for (int h = 0; h < 24; h++) {
        sys.setGameTime(static_cast<float>(h));
        const auto w = sys.getCurrentWeather();
        const float sum = w.sky.zenith[0] + w.sky.zenith[1] + w.sky.zenith[2];
        if (!std::isfinite(sum) || sum <= 0.0f || sum > 3.0f) everyHourLit = false;
    }
    record("SkyIsFiniteAllDay", everyHourLit,
           everyHourLit ? "all 24 hours produce a finite, non-black sky"
                        : "some hour produced an invalid sky colour");

    // The console resolves "storm"/"fog"/... which are not WTHR editor IDs, so
    // this mapping is the only thing standing between the debug buttons and a
    // silently ignored command.
    const bool namesResolve =
        engine::SkyWeatherSystem::weatherTypeFromName("clear") ==
            engine::WeatherType::CLEAR &&
        engine::SkyWeatherSystem::weatherTypeFromName("storm") ==
            engine::WeatherType::THUNDER &&
        engine::SkyWeatherSystem::weatherTypeFromName("fog") ==
            engine::WeatherType::FOGGY &&
        engine::SkyWeatherSystem::weatherTypeFromName("overcast") ==
            engine::WeatherType::OVERCAST &&
        engine::SkyWeatherSystem::weatherTypeFromName("blizzard") ==
            engine::WeatherType::BLIZZARD &&
        engine::SkyWeatherSystem::weatherTypeFromName("BlightClouds") ==
            engine::WeatherType::BLIZZARD &&
        engine::SkyWeatherSystem::weatherTypeFromName("nonsense") ==
            engine::WeatherType::COUNT;
    record("ConsoleWeatherNamesResolve", namesResolve,
           namesResolve ? "console names map onto weather presets"
                        : "a console weather name did not resolve");

    sys.setGameTime(10.0f);
    sys.setWeatherImmediate(engine::WeatherType::CLEAR);

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    if (!results.empty()) results.back().durationMs = totalMs;

    sys.shutdown();
    return getFailCount() == 0;
}
