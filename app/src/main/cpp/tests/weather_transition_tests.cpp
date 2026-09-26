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

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    if (!results.empty()) results.back().durationMs = totalMs;

    sys.shutdown();
    return getFailCount() == 0;
}
