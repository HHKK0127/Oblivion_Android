#include "enchanting_system_tests.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

#include "../game/enchanting_system.h"

// Generated from the measured MGEF data; see the file header for provenance.
#include "enchanting_system_mgef_tables.inc"

namespace {

using game::EnchantingSystem;
using game::Enchantment;
using game::EnchantmentType;
using game::MagicEffect;

float nowMs() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<float, std::milli>(clock::now().time_since_epoch())
        .count();
}

bool nearlyEqual(float a, float b, float tolerance = 1e-4f) {
    return std::fabs(a - b) <= tolerance;
}

size_t codeCount() {
    return sizeof(enchanting_mgef_fixture::kExpectedCodes) /
           sizeof(enchanting_mgef_fixture::kExpectedCodes[0]);
}

size_t nameCount() {
    return sizeof(enchanting_mgef_fixture::kExpectedNames) /
           sizeof(enchanting_mgef_fixture::kExpectedNames[0]);
}

size_t rejectedCount() {
    return sizeof(enchanting_mgef_fixture::kRejectedCodes) /
           sizeof(enchanting_mgef_fixture::kRejectedCodes[0]);
}

// ---------------------------------------------------------------------------
// Synthetic ESM builder
//
// A tiny in-memory plugin carrying two ENCH records is enough to exercise the
// ESMManager -> EnchantingSystem path without shipping any Oblivion data.
// ---------------------------------------------------------------------------

void putU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void putU32(std::vector<uint8_t>& out, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }
}

void putF32(std::vector<uint8_t>& out, float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    putU32(out, bits);
}

void putTag(std::vector<uint8_t>& out, const char* tag) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>(tag[i]));
    }
}

void putSubRecord(std::vector<uint8_t>& out, const char* tag,
                  const std::vector<uint8_t>& payload) {
    putTag(out, tag);
    putU16(out, static_cast<uint16_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
}

// Record header is 20 bytes: type, dataSize, flags, formID, version + unknown.
void putRecord(std::vector<uint8_t>& out, const char* tag, uint32_t formID,
               const std::vector<uint8_t>& subRecords) {
    putTag(out, tag);
    putU32(out, static_cast<uint32_t>(subRecords.size()));
    putU32(out, 0);  // flags: uncompressed
    putU32(out, formID);
    putU32(out, 0);  // version + unknown
    out.insert(out.end(), subRecords.begin(), subRecords.end());
}

std::vector<uint8_t> textPayload(const std::string& text, bool nulTerminated) {
    std::vector<uint8_t> payload(text.begin(), text.end());
    if (nulTerminated) {
        payload.push_back(0);
    }
    return payload;
}

// EFIT is code(4) magnitude(4) area(4) duration(4) unused(4) actorValue(4).
std::vector<uint8_t> makeEfit(const char* code, float magnitude, uint32_t area,
                              uint32_t duration, uint32_t actorValue) {
    std::vector<uint8_t> efit;
    for (int i = 0; i < 4; ++i) {
        efit.push_back(static_cast<uint8_t>(code[i]));
    }
    putF32(efit, magnitude);
    putU32(efit, area);
    putU32(efit, duration);
    putU32(efit, 0);
    putU32(efit, actorValue);
    return efit;
}

std::vector<uint8_t> makeEnit(uint32_t enchantType, uint32_t chargeAmount,
                              uint32_t enchantCost, uint32_t flags) {
    std::vector<uint8_t> enit;
    putU32(enit, enchantType);
    putU32(enit, chargeAmount);
    putU32(enit, enchantCost);
    putU32(enit, flags);
    return enit;
}

// FOAT (Fortify Attribute) with actorValue 0 (Strength) is the regression case:
// actorValue 0 is a real target, not "unset".
constexpr uint32_t kFoAtFormID = 0x00000100;
constexpr uint32_t kUnknownCodeFormID = 0x00000200;

std::vector<uint8_t> buildTestPlugin() {
    std::vector<uint8_t> esm;

    // TES4 header record: no masters, no subrecords.
    putRecord(esm, "TES4", 0, {});

    // ENCH 1: two effects, the first one carrying actorValue 0.
    {
        std::vector<uint8_t> subs;
        putSubRecord(subs, "EDID", textPayload("TestEnchantment", true));
        putSubRecord(subs, "FULL", textPayload("Test Enchantment", true));
        putSubRecord(subs, "ENIT", makeEnit(2, 100, 250, 0));
        putSubRecord(subs, "EFID", textPayload("FOAT", false));
        putSubRecord(subs, "EFIT", makeEfit("FOAT", 12.5f, 0, 30, 0));
        putSubRecord(subs, "EFID", textPayload("FOSK", false));
        putSubRecord(subs, "EFIT", makeEfit("FOSK", 5.0f, 0, 10, 11));
        putRecord(esm, "ENCH", kFoAtFormID, subs);
    }

    // ENCH 2: a code that exists in no MGEF record, and no display name.
    {
        std::vector<uint8_t> subs;
        putSubRecord(subs, "EDID", textPayload("OnlyEditorID", true));
        putSubRecord(subs, "ENIT", makeEnit(0, 0, 0, 0));
        putSubRecord(subs, "EFID", textPayload("ZZZZ", false));
        putSubRecord(subs, "EFIT", makeEfit("ZZZZ", 3.0f, 0, 0, 7));
        putRecord(esm, "ENCH", kUnknownCodeFormID, subs);
    }

    return esm;
}

}  // namespace

void EnchantingSystemTests::record(const std::string& name, bool passed,
                                   const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
    std::cout << "[EnchantingSystemTests] " << (passed ? "PASS" : "FAIL") << " " << name;
    if (!msg.empty()) {
        std::cout << " - " << msg;
    }
    std::cout << " (" << ms << " ms)" << std::endl;
}

int EnchantingSystemTests::getPassCount() const {
    int count = 0;
    for (const EnchantingSystemTestResult& result : results) {
        if (result.passed) {
            ++count;
        }
    }
    return count;
}

int EnchantingSystemTests::getFailCount() const {
    return static_cast<int>(results.size()) - getPassCount();
}

std::string EnchantingSystemTests::getSummary() const {
    std::ostringstream out;
    out << "Enchanting System Test Results\n";
    out << "Total: " << results.size() << " | Pass: " << getPassCount()
        << " | Fail: " << getFailCount();
    return out.str();
}

// ---------------------------------------------------------------------------

void EnchantingSystemTests::testEnumShape() {
    const float start = nowMs();
    const int count = static_cast<int>(MagicEffect::COUNT);
    const int unknown = static_cast<int>(MagicEffect::UNKNOWN);
    // The fixture lists every enumerator except COUNT, so COUNT has to equal the
    // number of those enumerators and follow the last of them; a family added
    // without a name row breaks this.
    const int enumerators = static_cast<int>(nameCount());
    const int lastEnumerator =
        static_cast<int>(enchanting_mgef_fixture::kExpectedNames[enumerators - 1].effect);

    std::ostringstream message;
    const bool ok = unknown == 0 && count == enumerators &&
                    lastEnumerator == count - 1;
    if (ok) {
        message << "COUNT=" << count << " (" << enumerators
                << " named enumerators), UNKNOWN=" << unknown << ", " << codeCount()
                << " codes";
    } else {
        message << "expected UNKNOWN=0, COUNT=" << enumerators
                << " and last enumerator " << (enumerators - 1) << ", got UNKNOWN="
                << unknown << ", COUNT=" << count << ", last=" << lastEnumerator;
    }
    record("EnumShape", ok, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testEffectCodeMapping() {
    const float start = nowMs();
    size_t mismatches = 0;
    std::string firstFailure;
    bool reachable[static_cast<int>(MagicEffect::COUNT)] = {};

    for (size_t i = 0; i < codeCount(); ++i) {
        const auto& expected = enchanting_mgef_fixture::kExpectedCodes[i];
        const MagicEffect actual = EnchantingSystem::magicEffectFromCode(expected.code);
        reachable[static_cast<int>(actual)] = true;
        if (actual != expected.effect) {
            ++mismatches;
            if (firstFailure.empty()) {
                firstFailure = std::string(expected.code) + " -> " +
                               EnchantingSystem::magicEffectName(actual) +
                               " (expected " +
                               EnchantingSystem::magicEffectName(expected.effect) + ")";
            }
        }
    }

    // Every family the shipped code set uses has to be reachable; a code that
    // maps to UNKNOWN also hides the effect from gameplay.
    size_t unreachable = 0;
    for (size_t i = 0; i < nameCount(); ++i) {
        const MagicEffect effect = enchanting_mgef_fixture::kExpectedNames[i].effect;
        if (effect != MagicEffect::UNKNOWN && !reachable[static_cast<int>(effect)]) {
            ++unreachable;
            if (firstFailure.empty()) {
                firstFailure = std::string("no code maps to ") +
                               EnchantingSystem::magicEffectName(effect);
            }
        }
    }

    std::ostringstream message;
    if (mismatches == 0 && unreachable == 0) {
        message << codeCount() << " codes mapped, " << (nameCount() - 1)
                << " families reachable";
    } else {
        message << mismatches << " wrong mappings, " << unreachable
                << " unreachable families; first: " << firstFailure;
    }
    record("EffectCodeMapping", mismatches == 0 && unreachable == 0, message.str(),
           nowMs() - start);
}

void EnchantingSystemTests::testRejectedEffectCodes() {
    const float start = nowMs();
    size_t mismatches = 0;
    std::string firstFailure;

    for (size_t i = 0; i < rejectedCount(); ++i) {
        const char* code = enchanting_mgef_fixture::kRejectedCodes[i];
        const MagicEffect actual = EnchantingSystem::magicEffectFromCode(code);
        if (actual != MagicEffect::UNKNOWN) {
            ++mismatches;
            if (firstFailure.empty()) {
                firstFailure = std::string(code) + " -> " +
                               EnchantingSystem::magicEffectName(actual);
            }
        }
    }

    std::ostringstream message;
    if (mismatches == 0) {
        message << rejectedCount()
                << " invented codes resolve to UNKNOWN";
    } else {
        message << mismatches << " of " << rejectedCount()
                << " invented codes resolved to a real effect; first: " << firstFailure;
    }
    record("RejectedEffectCodes", mismatches == 0, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testMalformedEffectCodes() {
    const float start = nowMs();
    // EFID is a fixed 4 character editorID code. Anything else is not an effect.
    const char* cases[] = {"", "FOA", "FOATT", "foat", "FoaT", "FOA ", " FOA", "FOA1"};
    size_t mismatches = 0;
    std::string firstFailure;

    for (const char* code : cases) {
        const MagicEffect actual = EnchantingSystem::magicEffectFromCode(code);
        if (actual != MagicEffect::UNKNOWN) {
            ++mismatches;
            if (firstFailure.empty()) {
                firstFailure = std::string("\"") + code + "\" -> " +
                               EnchantingSystem::magicEffectName(actual);
            }
        }
    }

    std::ostringstream message;
    if (mismatches == 0) {
        message << (sizeof(cases) / sizeof(cases[0]))
                << " malformed codes rejected";
    } else {
        message << mismatches << " malformed codes accepted; first: " << firstFailure;
    }
    record("MalformedEffectCodes", mismatches == 0, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testEffectNameTable() {
    const float start = nowMs();
    size_t mismatches = 0;
    std::string firstFailure;

    // magicEffectName() indexes a positional table, so a reordered enumerator
    // that kept its old entry renames every effect after it. Compare every pair.
    for (size_t i = 0; i < nameCount(); ++i) {
        const auto& expected = enchanting_mgef_fixture::kExpectedNames[i];
        const char* actual = EnchantingSystem::magicEffectName(expected.effect);
        if (std::strcmp(actual, expected.name) != 0) {
            ++mismatches;
            if (firstFailure.empty()) {
                firstFailure = std::string("enum index ") + std::to_string(i) +
                               " expected \"" + expected.name + "\", got \"" +
                               actual + "\"";
            }
        }
    }

    // Out of range values must not read past the table.
    const MagicEffect pastEnd = static_cast<MagicEffect>(200);
    const MagicEffect beyondCount = static_cast<MagicEffect>(
        static_cast<int>(MagicEffect::COUNT));
    const bool rangeSafe =
        std::strcmp(EnchantingSystem::magicEffectName(pastEnd), "Unknown") == 0 &&
        std::strcmp(EnchantingSystem::magicEffectName(beyondCount), "Unknown") == 0;

    std::ostringstream message;
    if (mismatches == 0 && rangeSafe) {
        message << nameCount() << " names aligned with the enumerators";
    } else {
        message << mismatches << " misaligned names, out of range "
                << (rangeSafe ? "safe" : "unsafe") << "; first: " << firstFailure;
    }
    record("EffectNameTable", mismatches == 0 && rangeSafe, message.str(),
           nowMs() - start);
}

void EnchantingSystemTests::testEnchantmentDefaults() {
    const float start = nowMs();
    const Enchantment enchant;

    std::ostringstream message;
    const bool ok = enchant.id == 0 && enchant.name.empty() &&
                    enchant.type == EnchantmentType::CAST_ONCE &&
                    enchant.effect == MagicEffect::UNKNOWN && enchant.actorValue == 0 &&
                    enchant.magnitude == 0.0f && enchant.duration == 0 &&
                    enchant.charges == 0 && enchant.currentCharges == 0 &&
                    enchant.enchantCost == 0.0f;
    if (ok) {
        message << "defaults are zeroed with effect UNKNOWN";
    } else {
        message << "unexpected default: type=" << static_cast<int>(enchant.type)
                << " effect=" << static_cast<int>(enchant.effect)
                << " actorValue=" << enchant.actorValue;
    }
    record("EnchantmentDefaults", ok, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testCreateEnchantment() {
    const float start = nowMs();
    EnchantingSystem system;

    const Enchantment fire =
        system.createEnchantment(MagicEffect::FIRE_DAMAGE, 5.0f, 10,
                                EnchantmentType::CAST_ON_STRIKE);
    const Enchantment unknown =
        system.createEnchantment(MagicEffect::UNKNOWN, 1.0f, 0,
                                 EnchantmentType::CAST_ONCE);

    std::ostringstream message;
    const bool ok = fire.name == "Fire Damage" &&
                    fire.effect == MagicEffect::FIRE_DAMAGE &&
                    fire.type == EnchantmentType::CAST_ON_STRIKE &&
                    nearlyEqual(fire.magnitude, 5.0f) && fire.duration == 10 &&
                    fire.id != 0 &&
                    unknown.name == "Unknown" &&
                    unknown.effect == MagicEffect::UNKNOWN &&
                    unknown.type == EnchantmentType::CAST_ONCE;
    if (ok) {
        message << "name from effect table, magnitude/duration/type stored";
    } else {
        message << "got name=\"" << fire.name << "\" mag=" << fire.magnitude
                << " dur=" << fire.duration << " id=" << fire.id;
    }
    record("CreateEnchantment", ok, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testEffectMagnitude() {
    const float start = nowMs();
    EnchantingSystem system;

    struct Case {
        MagicEffect effect;
        float base;
        uint32_t skill;
        float expected;
    };
    const Case cases[] = {
        // Fortify / restore / cure scale with the skill bonus only.
        {MagicEffect::FORTIFY_HEALTH, 10.0f, 0, 10.0f},
        {MagicEffect::FORTIFY_HEALTH, 10.0f, 100, 15.0f},
        {MagicEffect::RESTORE_MAGICKA, 20.0f, 50, 25.0f},
        // Hostile effects add 20% on top of the skill bonus.
        {MagicEffect::FIRE_DAMAGE, 10.0f, 0, 12.0f},
        {MagicEffect::FIRE_DAMAGE, 10.0f, 100, 18.0f},
        {MagicEffect::ABSORB_HEALTH, 10.0f, 0, 12.0f},
        // Resist / weakness effects are capped at 100%.
        {MagicEffect::RESIST_FIRE, 100.0f, 0, 100.0f},
        {MagicEffect::RESIST_FIRE, 100.0f, 100, 100.0f},
        {MagicEffect::RESIST_FIRE, 50.0f, 100, 75.0f},
        {MagicEffect::WEAKNESS_FROST, 80.0f, 100, 100.0f},
        // Everything else falls back to the skill bonus alone.
        {MagicEffect::UNKNOWN, 10.0f, 100, 15.0f},
        {MagicEffect::SUMMON, 10.0f, 100, 15.0f},
        {MagicEffect::INVISIBILITY, 10.0f, 100, 15.0f},
    };

    size_t mismatches = 0;
    std::string firstFailure;
    for (const Case& testCase : cases) {
        const float actual =
            system.getEffectMagnitude(testCase.effect, testCase.base, testCase.skill);
        if (!nearlyEqual(actual, testCase.expected)) {
            ++mismatches;
            if (firstFailure.empty()) {
                std::ostringstream out;
                out << EnchantingSystem::magicEffectName(testCase.effect) << " base "
                    << testCase.base << " skill " << testCase.skill << " -> " << actual
                    << " (expected " << testCase.expected << ")";
                firstFailure = out.str();
            }
        }
    }

    std::ostringstream message;
    if (mismatches == 0) {
        message << (sizeof(cases) / sizeof(cases[0])) << " magnitudes as expected";
    } else {
        message << mismatches << " of " << (sizeof(cases) / sizeof(cases[0]))
                << " wrong; first: " << firstFailure;
    }
    record("EffectMagnitude", mismatches == 0, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testEffectDuration() {
    const float start = nowMs();
    EnchantingSystem system;

    struct Case {
        MagicEffect effect;
        uint32_t base;
        uint32_t skill;
        uint32_t expected;
    };
    const Case cases[] = {
        {MagicEffect::FORTIFY_HEALTH, 10, 0, 10},
        {MagicEffect::FORTIFY_HEALTH, 10, 100, 15},
        {MagicEffect::FORTIFY_ATTRIBUTE, 20, 50, 25},
        // Utility effects add another 50% on top of the skill bonus.
        {MagicEffect::INVISIBILITY, 10, 0, 15},
        {MagicEffect::INVISIBILITY, 10, 100, 22},
        {MagicEffect::NIGHT_EYE, 20, 0, 30},
        {MagicEffect::WATER_BREATHING, 10, 100, 22},
        // Damage and everything else use the skill bonus alone.
        {MagicEffect::FIRE_DAMAGE, 10, 100, 15},
        {MagicEffect::UNKNOWN, 10, 100, 15},
    };

    size_t mismatches = 0;
    std::string firstFailure;
    for (const Case& testCase : cases) {
        const uint32_t actual =
            system.getEffectDuration(testCase.effect, testCase.base, testCase.skill);
        if (actual != testCase.expected) {
            ++mismatches;
            if (firstFailure.empty()) {
                std::ostringstream out;
                out << EnchantingSystem::magicEffectName(testCase.effect) << " base "
                    << testCase.base << " skill " << testCase.skill << " -> " << actual
                    << " (expected " << testCase.expected << ")";
                firstFailure = out.str();
            }
        }
    }

    std::ostringstream message;
    if (mismatches == 0) {
        message << (sizeof(cases) / sizeof(cases[0])) << " durations as expected";
    } else {
        message << mismatches << " of " << (sizeof(cases) / sizeof(cases[0]))
                << " wrong; first: " << firstFailure;
    }
    record("EffectDuration", mismatches == 0, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testEnchantCost() {
    const float start = nowMs();
    EnchantingSystem system;

    auto make = [&system](MagicEffect effect, float magnitude, uint32_t duration,
                          EnchantmentType type) {
        return system.createEnchantment(effect, magnitude, duration, type);
    };

    // Base cost is 10, plus magnitude * per-effect multiplier, plus
    // duration * per-effect multiplier, then the type and skill multipliers.
    struct Case {
        MagicEffect effect;
        float magnitude;
        uint32_t duration;
        EnchantmentType type;
        float expected;
    };
    const Case cases[] = {
        // Fortify Health: magnitude x1.0, duration x1.5, base 10.
        {MagicEffect::FORTIFY_HEALTH, 10.0f, 0, EnchantmentType::CAST_ON_STRIKE, 20.0f},
        {MagicEffect::FORTIFY_HEALTH, 10.0f, 0, EnchantmentType::CAST_ONCE, 10.0f},
        {MagicEffect::FORTIFY_HEALTH, 10.0f, 0, EnchantmentType::CAST_ON_USE, 16.0f},
        {MagicEffect::FORTIFY_HEALTH, 10.0f, 0, EnchantmentType::CONSTANT_EFFECT, 40.0f},
        {MagicEffect::FORTIFY_HEALTH, 10.0f, 20, EnchantmentType::CAST_ON_STRIKE, 50.0f},
        // Fire Damage: magnitude x2.0, duration x0.5.
        {MagicEffect::FIRE_DAMAGE, 10.0f, 20, EnchantmentType::CAST_ON_STRIKE, 40.0f},
        // No magnitude and no duration still costs a fraction of the base cost.
        {MagicEffect::FORTIFY_HEALTH, 0.0f, 0, EnchantmentType::CAST_ONCE, 5.0f},
    };

    size_t mismatches = 0;
    std::string firstFailure;
    for (const Case& testCase : cases) {
        const Enchantment enchant =
            make(testCase.effect, testCase.magnitude, testCase.duration, testCase.type);
        const float actual = system.getEnchantCost(enchant);
        if (!nearlyEqual(actual, testCase.expected)) {
            ++mismatches;
            if (firstFailure.empty()) {
                std::ostringstream out;
                out << EnchantingSystem::magicEffectName(testCase.effect) << " type "
                    << static_cast<int>(testCase.type) << " -> " << actual
                    << " (expected " << testCase.expected << ")";
                firstFailure = out.str();
            }
        }
    }

    // Skill 100 halves the cost.
    const Enchantment scaled = make(MagicEffect::FORTIFY_HEALTH, 10.0f, 0,
                                    EnchantmentType::CAST_ON_STRIKE);
    system.setEnchantSkill(100);
    const float reduced = system.getEnchantCost(scaled);
    system.setEnchantSkill(0);
    const bool skillOk = nearlyEqual(reduced, 10.0f);

    std::ostringstream message;
    if (mismatches == 0 && skillOk) {
        message << (sizeof(cases) / sizeof(cases[0]))
                << " costs as expected, skill 100 halves the cost";
    } else {
        message << mismatches << " of " << (sizeof(cases) / sizeof(cases[0]))
                << " wrong, skill 100 cost " << reduced << " (expected 10); first: "
                << firstFailure;
    }
    record("EnchantCost", mismatches == 0 && skillOk, message.str(), nowMs() - start);
}

void EnchantingSystemTests::testEnchantmentsFromEsm() {
    const float start = nowMs();
    const std::vector<uint8_t> plugin = buildTestPlugin();

    oblivion::ESMManager manager;
    const bool loaded =
        manager.loadPluginFromMemory("test_enchanting.esm", plugin.data(),
                                     plugin.size());
    if (!loaded) {
        record("EnchantmentsFromEsm", false,
               "synthetic plugin failed to load", nowMs() - start);
        return;
    }

    const auto& decoded = manager.getAllEnchantments();
    std::string failure;
    if (decoded.size() != 2) {
        failure = "decoded " + std::to_string(decoded.size()) + " ENCH records, expected 2";
    } else {
        const oblivion::EnchantmentData& first = decoded[0];
        if (first.formID != kFoAtFormID || first.editorID != "TestEnchantment" ||
            first.fullName != "Test Enchantment" || first.enchantType != 2 ||
            first.chargeAmount != 100 || first.enchantCost != 250) {
            failure = "ENIT/EDID/FULL fields wrong on the first record";
        } else if (first.effectCodes.size() != 2 ||
                   first.effectCodes[0] != "FOAT" || first.effectCodes[1] != "FOSK") {
            failure = "EFID codes not decoded";
        } else if (first.effectMagnitudes.size() != 2 ||
                   !nearlyEqual(first.effectMagnitudes[0], 12.5f) ||
                   !nearlyEqual(first.effectMagnitudes[1], 5.0f)) {
            failure = "EFIT magnitudes not decoded";
        } else if (first.effectDurations.size() != 2 || first.effectDurations[0] != 30 ||
                   first.effectDurations[1] != 10) {
            failure = "EFIT durations not decoded";
        } else if (first.effectActorValues.size() != 2 ||
                   first.effectActorValues[0] != 0 || first.effectActorValues[1] != 11) {
            failure = "EFIT actorValue not decoded (0 must survive)";
        }
    }
    if (!failure.empty()) {
        record("EnchantmentsFromEsm", false, failure, nowMs() - start);
        return;
    }

    EnchantingSystem system;
    system.loadEnchantmentsFromESM(manager);
    const auto& systemEnchants = system.getAllEnchantments();
    if (systemEnchants.size() != 2) {
        record("EnchantmentsFromEsm", false,
               "EnchantingSystem kept " + std::to_string(systemEnchants.size()) +
                   " of 2 enchantments",
               nowMs() - start);
        return;
    }

    const Enchantment& fortify = systemEnchants.at(kFoAtFormID);
    const Enchantment& unknownCode = systemEnchants.at(kUnknownCodeFormID);

    const bool ok =
        fortify.name == "Test Enchantment" &&
        fortify.type == EnchantmentType::CAST_ON_STRIKE && fortify.charges == 100 &&
        fortify.currentCharges == 100 && nearlyEqual(fortify.enchantCost, 250.0f) &&
        fortify.effect == MagicEffect::FORTIFY_ATTRIBUTE && fortify.actorValue == 0 &&
        nearlyEqual(fortify.magnitude, 12.5f) && fortify.duration == 30 &&
        unknownCode.name == "OnlyEditorID" &&
        unknownCode.type == EnchantmentType::CAST_ONCE &&
        unknownCode.effect == MagicEffect::UNKNOWN &&
        unknownCode.actorValue == 7;

    std::ostringstream message;
    if (ok) {
        message << "2 enchantments loaded; FOAT actorValue 0 preserved";
    } else {
        message << "got name=\"" << fortify.name << "\" effect="
                << static_cast<int>(fortify.effect) << " actorValue="
                << fortify.actorValue << " magnitude=" << fortify.magnitude
                << " duration=" << fortify.duration << " type="
                << static_cast<int>(fortify.type) << " charges=" << fortify.charges
                << ", second name=\"" << unknownCode.name << "\" effect="
                << static_cast<int>(unknownCode.effect) << " actorValue="
                << unknownCode.actorValue;
    }
    record("EnchantmentsFromEsm", ok, message.str(), nowMs() - start);
}

// ---------------------------------------------------------------------------

bool EnchantingSystemTests::runAllTests() {
    results.clear();

    testEnumShape();
    testEffectCodeMapping();
    testRejectedEffectCodes();
    testMalformedEffectCodes();
    testEffectNameTable();
    testEnchantmentDefaults();
    testCreateEnchantment();
    testEffectMagnitude();
    testEffectDuration();
    testEnchantCost();
    testEnchantmentsFromEsm();

    return getFailCount() == 0;
}
