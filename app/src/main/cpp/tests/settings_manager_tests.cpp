// SettingsManager layering tests - see settings_manager_tests.h for the rationale.

#include "settings_manager_tests.h"

#include "../system/settings_manager.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// Unmistakable replacements for the shipped Oblivion_default.ini [Audio] and
// [GamePlay] values, so an applied INI key cannot be mistaken for a default.
const std::string kFullIni =
    "[Audio]\n"
    "fDefaultMasterVolume=0.11\n"
    "fDefaultVoiceVolume=0.22\n"
    "fDefaultEffectsVolume=0.33\n"
    "fDefaultFootVolume=0.44\n"
    "fDefaultMusicVolume=0.55\n"
    "[GamePlay]\n"
    "iDifficultyLevel=77\n"
    "bGeneralSubtitles=1\n"
    "bDialogueSubtitles=0\n"
    "bCrossHair=0\n"
    "bSaveOnRest=0\n"
    "bSaveOnWait=0\n"
    "bSaveOnTravel=0\n"
    "bSaveOnInteriorExteriorSwitch=0\n"
    "bDisableDynamicCrosshair=1\n";

// Every option key settings.txt can hold - what an install predating the layering
// wrote, because saveSettings() used to rewrite all of them unconditionally.
const std::string kLegacySettings =
    "DEBUG_MODE=1\n"
    "LANGUAGE=en\n"
    "FPS_CAP=60\n"
    "AUDIO_MASTER=0.9\n"
    "AUDIO_VOICE=0.8\n"
    "AUDIO_EFFECT=0.8\n"
    "AUDIO_FOOT=0.8\n"
    "AUDIO_MUSIC=0.3\n"
    "GAMEPLAY_DIFFICULTY=50\n"
    "GAMEPLAY_GENERAL_SUBTITLES=0\n"
    "GAMEPLAY_DIALOGUE_SUBTITLES=1\n"
    "GAMEPLAY_CROSSHAIR=1\n"
    "GAMEPLAY_SAVE_ON_REST=1\n"
    "GAMEPLAY_SAVE_ON_WAIT=1\n"
    "GAMEPLAY_SAVE_ON_TRAVEL=1\n"
    "GAMEPLAY_SAVE_ON_INT_EXT_SWITCH=1\n"
    "GAMEPLAY_DISABLE_DYNAMIC_CROSSHAIR=0\n"
    "VIDEO_RESOLUTION_INDEX=0\n";

/// Written as LF in binary mode: a CRLF settings.txt would leave a trailing CR in
/// every value on the POSIX runner, where the stream does no newline translation.
void writeFile(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
}

/// Raw bytes with CR removed, so one expectation serves both runners.
std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

bool exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

/// Scratch space for the settings files. The repository's ignored tmp/ folder is
/// preferred so a failure leaves its artifact where a developer looks for it; the
/// OS temp directory is the fallback when the runner starts somewhere else.
std::string scratchRoot() {
    std::error_code ec;
    const fs::path root = fs::path("tmp") / "host_tests" / "settings_manager_cases";
    fs::create_directories(root, ec);
    if (!ec) {
        return root.string();
    }
    const fs::path fallback = fs::temp_directory_path(ec) / "oblivion_settings_manager_cases";
    fs::create_directories(fallback, ec);
    return fallback.string();
}

}  // namespace

void SettingsManagerTests::record(const std::string& name, bool passed,
                                 const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
    std::printf("[SettingsManagerTests] %s %s", passed ? "PASS" : "FAIL", name.c_str());
    if (!msg.empty()) {
        std::printf(" - %s", msg.c_str());
    }
    std::printf(" (%.1f ms)\n", ms);
}

int SettingsManagerTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (r.passed) {
            ++n;
        }
    }
    return n;
}

int SettingsManagerTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (!r.passed) {
            ++n;
        }
    }
    return n;
}

std::string SettingsManagerTests::getSummary() const {
    return "SettingsManager Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool SettingsManagerTests::runAllTests() {
    results.clear();

    const std::string root = scratchRoot();

    /// Recreate one scratch case directory. Empty arguments leave that file absent.
    const auto makeCase = [&root](const std::string& name, const std::string& settings,
                                  const std::string& ini) {
        const fs::path dir = fs::path(root) / name;
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir, ec);
        const std::string path = dir.string();
        if (!settings.empty()) {
            writeFile(path + "/settings.txt", settings);
        }
        if (!ini.empty()) {
            writeFile(path + "/Oblivion.ini", ini);
        }
        return path;
    };

    auto t0 = std::chrono::steady_clock::now();
    const auto elapsed = [&t0]() {
        return std::chrono::duration<float, std::milli>(
                   std::chrono::steady_clock::now() - t0)
            .count();
    };
    const auto check = [this, &elapsed](const std::string& name, bool passed) {
        record(name, passed, passed ? "" : "condition was false", elapsed());
    };
    const auto check_str = [this, &elapsed](const std::string& name, const std::string& got,
                                            const std::string& want) {
        const bool ok = got == want;
        record(name, ok, ok ? "" : "got \"" + got + "\", want \"" + want + "\"", elapsed());
    };
    const auto check_int = [this, &elapsed](const std::string& name, int got, int want) {
        const bool ok = got == want;
        record(name, ok, ok ? "" : "got " + std::to_string(got) + ", want " +
                                        std::to_string(want),
               elapsed());
    };
    const auto check_real = [this, &elapsed](const std::string& name, float got, float want) {
        const bool ok = std::fabs(got - want) < 1e-4f;
        char msg[96];
        std::snprintf(msg, sizeof(msg), "got %.6f, want %.6f", static_cast<double>(got),
                      static_cast<double>(want));
        record(name, ok, ok ? "" : msg, elapsed());
    };

    // 1. Nothing on disk: the shipped defaults, and no file conjured up.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("defaults", "", "");
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        check("initialize() succeeds", settings.initialize());
        check("debug mode is on by default", settings.isDebugModeEnabled());
        check_str("language is en by default", settings.getLanguage(), "en");
        check_int("fps cap is 60 by default", settings.getFpsCap(), 60);
        const AudioVolumeSettings& audio = settings.audioVolumes();
        check_real("master volume defaults to 1.0", audio.master, 1.0f);
        check_real("voice volume defaults to 0.8", audio.voice, 0.8f);
        check_real("effects volume defaults to 0.8", audio.effect, 0.8f);
        check_real("foot volume defaults to 0.8", audio.foot, 0.8f);
        check_real("music volume defaults to 0.3", audio.music, 0.3f);
        const GameplaySettings& gameplay = settings.gameplaySettings();
        check_int("difficulty defaults to 50", gameplay.difficulty, 50);
        check("general subtitles are off by default", !gameplay.generalSubtitles);
        check("dialogue subtitles are on by default", gameplay.dialogueSubtitles);
        check("the crosshair is on by default", gameplay.crosshair);
        check("save on rest defaults to on", gameplay.saveOnRest);
        check("save on wait defaults to on", gameplay.saveOnWait);
        check("save on travel defaults to on", gameplay.saveOnTravel);
        check("save on interior/exterior switch defaults to on",
              gameplay.saveOnInteriorExteriorSwitch);
        check("the dynamic crosshair is not disabled by default",
              !gameplay.disableDynamicCrosshair);
        check_int("the first resolution is selected by default",
                  settings.videoSettings().resolution_index, 0);
        check("initialize() writes no settings.txt of its own",
              !exists(dir + "/settings.txt"));
    }

    // 2. Every mapped INI key reaches the option it feeds.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("ini_all_keys", "", kFullIni);
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        check("initialize() succeeds with an INI present", settings.initialize());
        const AudioVolumeSettings& audio = settings.audioVolumes();
        check_real("fDefaultMasterVolume feeds AUDIO_MASTER", audio.master, 0.11f);
        check_real("fDefaultVoiceVolume feeds AUDIO_VOICE", audio.voice, 0.22f);
        check_real("fDefaultEffectsVolume feeds AUDIO_EFFECT", audio.effect, 0.33f);
        check_real("fDefaultFootVolume feeds AUDIO_FOOT", audio.foot, 0.44f);
        check_real("fDefaultMusicVolume feeds AUDIO_MUSIC", audio.music, 0.55f);
        const GameplaySettings& gameplay = settings.gameplaySettings();
        check_int("iDifficultyLevel feeds GAMEPLAY_DIFFICULTY", gameplay.difficulty, 77);
        check("bGeneralSubtitles feeds GAMEPLAY_GENERAL_SUBTITLES", gameplay.generalSubtitles);
        check("bDialogueSubtitles feeds GAMEPLAY_DIALOGUE_SUBTITLES",
              !gameplay.dialogueSubtitles);
        check("bCrossHair feeds GAMEPLAY_CROSSHAIR", !gameplay.crosshair);
        check("bSaveOnRest feeds GAMEPLAY_SAVE_ON_REST", !gameplay.saveOnRest);
        check("bSaveOnWait feeds GAMEPLAY_SAVE_ON_WAIT", !gameplay.saveOnWait);
        check("bSaveOnTravel feeds GAMEPLAY_SAVE_ON_TRAVEL", !gameplay.saveOnTravel);
        // The one GamePlay toggle the shipped file omits: it comes from the compiled-in
        // registry, and the player's file still overrides it.
        check("bSaveOnInteriorExteriorSwitch overrides the compiled-in default",
              !gameplay.saveOnInteriorExteriorSwitch);
        check("bDisableDynamicCrosshair feeds GAMEPLAY_DISABLE_DYNAMIC_CROSSHAIR",
              gameplay.disableDynamicCrosshair);
        check("reading the INI writes no settings.txt", !exists(dir + "/settings.txt"));
    }

    // 3. The INI keeps answering after the player changes an unrelated option. This
    // is the regression the ownership rule exists for: an unconditional save would
    // copy all fourteen INI values into settings.txt and freeze them there.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("ini_survives_a_later_save", "", kFullIni);
        {
            SettingsManager settings(dir + "/settings.txt");
            settings.setDataPath(dir);
            settings.initialize();
            settings.setFpsCap(90);
        }
        const std::string written = readFile(dir + "/settings.txt");
        check_str("settings.txt holds only the key the player changed", written,
                  "FPS_CAP=90\n");
        check("the INI audio values were not copied into settings.txt",
              !contains(written, "AUDIO_"));
        check("the INI gameplay values were not copied into settings.txt",
              !contains(written, "GAMEPLAY_"));
        {
            SettingsManager settings(dir + "/settings.txt");
            settings.setDataPath(dir);
            settings.initialize();
            check_real("the INI still answers after a restart",
                       settings.audioVolumes().master, 0.11f);
            check_int("the INI still answers for gameplay after a restart",
                      settings.gameplaySettings().difficulty, 77);
            check_int("settings.txt keeps the key it owns", settings.getFpsCap(), 90);
        }
    }

    // 4. settings.txt wins for the keys it holds and only those.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("settings_txt_wins",
                                         "DEBUG_MODE=0\nLANGUAGE=ja\nAUDIO_MASTER=0.9\n"
                                         "GAMEPLAY_CROSSHAIR=0\n",
                                         kFullIni);
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        settings.initialize();
        check("DEBUG_MODE from settings.txt shadows the default", !settings.isDebugModeEnabled());
        check_str("LANGUAGE from settings.txt shadows the default", settings.getLanguage(), "ja");
        check_real("AUDIO_MASTER from settings.txt shadows the INI",
                   settings.audioVolumes().master, 0.9f);
        check("GAMEPLAY_CROSSHAIR from settings.txt shadows the INI",
              !settings.gameplaySettings().crosshair);
        check_real("a key settings.txt does not hold still comes from the INI",
                   settings.audioVolumes().voice, 0.22f);
        check_int("gameplay keys settings.txt does not hold still come from the INI",
                  settings.gameplaySettings().difficulty, 77);
    }

    // 5. A value settings.txt cannot use does not claim the key, so the INI - not
    // the default - answers for it.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("unusable_settings_value",
                                         "AUDIO_MASTER=abc\nFPS_CAP=9999\n"
                                         "GAMEPLAY_DIFFICULTY=abc\n",
                                         kFullIni);
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        settings.initialize();
        check_real("an unreadable AUDIO_MASTER falls through to the INI",
                   settings.audioVolumes().master, 0.11f);
        check_int("an unreadable GAMEPLAY_DIFFICULTY falls through to the INI",
                  settings.gameplaySettings().difficulty, 77);
        check_int("an out-of-range FPS_CAP keeps its default", settings.getFpsCap(), 60);
    }

    // 6. An INI value outside the accepted range is refused rather than clamped.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("ini_out_of_range", "",
                                         "[Audio]\n"
                                         "fDefaultMasterVolume=5.0000\n"
                                         "fDefaultVoiceVolume=-1.0\n"
                                         "[GamePlay]\n"
                                         "iDifficultyLevel=200\n");
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        settings.initialize();
        check_real("a master volume above 1.0 is refused", settings.audioVolumes().master, 1.0f);
        check_real("a negative voice volume is refused", settings.audioVolumes().voice, 0.8f);
        check_int("a difficulty above 100 is refused",
                  settings.gameplaySettings().difficulty, 50);
    }

    // 7. Only <dataPath>/Oblivion.ini is consulted.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("ini_for_another_data_path", "", kFullIni);
        const std::string other = makeCase("some_other_data_path", "", "");
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(other);
        settings.initialize();
        check_real("an INI outside the data path is not read",
                   settings.audioVolumes().master, 1.0f);
    }

    // 8. A pre-layering settings.txt holds every key, so it hides the whole INI.
    // Documented rather than desired: it lasts only until that file is rewritten.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("legacy_settings_owns_everything", kLegacySettings,
                                         kFullIni);
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        settings.initialize();
        check_real("a legacy settings.txt takes priority over the INI",
                   settings.audioVolumes().master, 0.9f);
        check_int("... for gameplay too", settings.gameplaySettings().difficulty, 50);
        check_int("... and for video", settings.videoSettings().resolution_index, 0);
    }

    // 9. Each setter claims exactly its own keys, so the file the player ends up with
    // records only what they actually changed.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("setters_claim_their_keys", "", "");
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        settings.initialize();
        settings.setDebugMode(false);
        check_str("setDebugMode() writes DEBUG_MODE alone", readFile(dir + "/settings.txt"),
                  "DEBUG_MODE=0\n");
        settings.setLanguage("ja");
        check_str("setLanguage() adds LANGUAGE", readFile(dir + "/settings.txt"),
                  "DEBUG_MODE=0\nLANGUAGE=ja\n");
        VideoSettings video;
        video.resolution_index = 3;
        settings.setVideoSettings(video);
        check_str("setVideoSettings() adds VIDEO_RESOLUTION_INDEX",
                  readFile(dir + "/settings.txt"),
                  "DEBUG_MODE=0\nLANGUAGE=ja\nVIDEO_RESOLUTION_INDEX=3\n");
    }

    // 10. A menu that changes the audio sliders claims all five channels at once, so
    // the whole audio block stops following the INI from then on.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("audio_setter_claims_five", "", kFullIni);
        const std::string path = dir + "/settings.txt";
        {
            SettingsManager settings(path);
            settings.setDataPath(dir);
            settings.initialize();
            AudioVolumeSettings volumes;
            volumes.master = 0.5f;
            volumes.voice = 0.5f;
            volumes.effect = 0.5f;
            volumes.foot = 0.5f;
            volumes.music = 0.5f;
            settings.setAudioVolumes(volumes);
        }
        const std::string written = readFile(path);
        check("setAudioVolumes() writes all five AUDIO_ keys",
              contains(written, "AUDIO_MASTER=0.5") && contains(written, "AUDIO_VOICE=0.5") &&
                  contains(written, "AUDIO_EFFECT=0.5") && contains(written, "AUDIO_FOOT=0.5") &&
                  contains(written, "AUDIO_MUSIC=0.5"));
        check("setAudioVolumes() leaves the gameplay keys to the INI",
              !contains(written, "GAMEPLAY_"));
        {
            SettingsManager settings(path);
            settings.setDataPath(dir);
            settings.initialize();
            check_real("the volume the player set survives a restart",
                       settings.audioVolumes().master, 0.5f);
            check_int("the gameplay options still come from the INI",
                      settings.gameplaySettings().difficulty, 77);
        }
    }

    // 11. A key answered only by the built-in default table is not treated as
    // file-supplied, so a key the file omits keeps the value resetToDefaults() set.
    {
        t0 = std::chrono::steady_clock::now();
        const std::string dir = makeCase("ini_partial", "",
                                         "[Audio]\n"
                                         "fDefaultMasterVolume=0.11\n");
        SettingsManager settings(dir + "/settings.txt");
        settings.setDataPath(dir);
        settings.initialize();
        check_real("the one key the file spells is applied",
                   settings.audioVolumes().master, 0.11f);
        check_real("a key the file omits keeps the shipped default",
                   settings.audioVolumes().music, 0.3f);
        check_int("gameplay keeps its shipped defaults", settings.gameplaySettings().difficulty,
                  50);
    }

    // A green run leaves no artifacts behind; a failing one keeps them for inspection.
    if (getFailCount() == 0) {
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    std::printf("\n%s\n", getSummary().c_str());
    return getFailCount() == 0;
}
