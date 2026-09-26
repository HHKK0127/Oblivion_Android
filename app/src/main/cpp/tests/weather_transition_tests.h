#pragma once

// Weather transition tests.
//
// Covers the P13 fix: SkyWeatherSystem::setWeather() used to lerp the live
// currentWeather_ toward targetWeather_ by the absolute progress t on every
// frame. That converges geometrically, so a 30-second transition reached 90%
// of the target in about 1.5 seconds. The transition now interpolates from a
// snapshot (startWeather_) taken when the transition begins, which produces a
// linear ramp that actually lasts transitionTime seconds.

#include <string>
#include <vector>

struct WeatherTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class WeatherTransitionTests {
public:
    bool runAllTests();
    const std::vector<WeatherTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<WeatherTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};
