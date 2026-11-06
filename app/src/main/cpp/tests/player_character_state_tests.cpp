#include "tests/player_character_state_tests.h"

#include "game/player_character_state.h"

#include <chrono>
#include <cstdio>
#include <cstring>

namespace game {

namespace {

float elapsed_ms(std::chrono::steady_clock::time_point start) {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float, std::milli>(now - start).count();
}

}  // namespace

void PlayerCharacterStateTests::record(const std::string& name, bool passed,
                                       const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

void PlayerCharacterStateTests::testSizeIs804() {
    auto t0 = std::chrono::steady_clock::now();
    const bool ok = (sizeof(PlayerCharacterState) == kPlayerCharacterBlockSize);
    char buf[96];
    std::snprintf(buf, sizeof(buf), "sizeof=%zu expected=0x%X",
                  sizeof(PlayerCharacterState), kPlayerCharacterBlockSize);
    record("SizeIs804", ok, buf, elapsed_ms(t0));
}

void PlayerCharacterStateTests::testInitSetsCtorBytes() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    bool ok = (s != nullptr);
    ok = ok && (s->byte115 == 1);
    ok = ok && (s->byte5A9 == 1);
    ok = ok && (s->dialogueRecursionGuard == 0);
    ok = ok && (s->viewStateFlag588 == 0);
    ok = ok && (s->byte589 == 0) && (s->byte58A == 0) && (s->byte58B == 0);
    ok = ok && (s->slot018 == 0) && (s->slot05C == 0) && (s->slot068 == 0);
    // ctor zero-loops cover +0x204..+0x56C (indices 0x81..0x15B); value-init
    // additionally zeroes the +0x200 header slot. Scan the raw bytes.
    for (int i = 4; i < 0x370; ++i) {
        ok = ok && (s->dynamicBand[i] == 0);
    }
    // The ctor dword set at +0x788 falls inside reserved739 in this port and
    // is value-initialized to zero; check it through the raw byte offset.
    {
        uint32_t at788 = 0;
        std::memcpy(&at788, reinterpret_cast<const uint8_t*>(s) + 0x788, sizeof(at788));
        ok = ok && (at788 == 0u);
    }
    record("InitSetsCtorBytes", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testDoubleInitIdempotent() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* a = PlayerCharacterGlobal::create();
    PlayerCharacterState* b = PlayerCharacterGlobal::create();
    const bool ok = (a != nullptr) && (a == b);
    record("DoubleInitIdempotent", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testDestroyClearsGlobal() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* a = PlayerCharacterGlobal::create();
    // Dirty the block with a sentinel before destroying it. The allocator may
    // legitimately hand the freed address back on the next create, so we verify
    // semantic freshness (no stale carryover) instead of pointer inequality.
    PlayerCharacterGlobal::setDynamicBandAt(kDynamicBandFirstIndex, 0xDEADBEEFu);
    PlayerCharacterGlobal::destroy();
    const bool ok = (a != nullptr) && (!PlayerCharacterGlobal::isValid()) &&
                    (PlayerCharacterGlobal::get() == nullptr);
    PlayerCharacterState* b = PlayerCharacterGlobal::create();
    const bool ok2 = ok && (b != nullptr) && PlayerCharacterGlobal::isValid() &&
                     (PlayerCharacterGlobal::dynamicBandAt(kDynamicBandFirstIndex) == 0u);
    record("DestroyClearsGlobal", ok2, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testNullSafetyBeforeCreate() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    const bool ok_get = (PlayerCharacterGlobal::get() == nullptr);
    const bool ok_at = (PlayerCharacterGlobal::dynamicBandAt(kDynamicBandFirstIndex) == 0u);
    PlayerCharacterGlobal::setDynamicBandAt(kDynamicBandFirstIndex, 0x1234u);  // must not crash
    const bool ok = ok_get && ok_at;
    record("NullSafetyBeforeCreate", ok, "", elapsed_ms(t0));
}

void PlayerCharacterStateTests::testLifecycleIsValid() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    const bool before = PlayerCharacterGlobal::isValid();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    const bool after = PlayerCharacterGlobal::isValid();
    const bool same = (PlayerCharacterGlobal::get() == s);
    PlayerCharacterGlobal::destroy();
    const bool ok = (!before) && after && same && (!PlayerCharacterGlobal::isValid());
    record("LifecycleIsValid", ok, "", elapsed_ms(t0));
}

void PlayerCharacterStateTests::testRecursionGuardRoundTrip() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    s->dialogueRecursionGuard = 3;
    const bool ok = (s->dialogueRecursionGuard == 3);
    s->dialogueRecursionGuard = 0;
    const bool ok2 = ok && (s->dialogueRecursionGuard == 0);
    record("RecursionGuardRoundTrip", ok2, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testPositionFloatsRoundTrip() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    s->posX = 12.5f;
    s->posY = -3.25f;
    s->posZ = 100.0f;
    const bool ok = (s->posX == 12.5f) && (s->posY == -3.25f) && (s->posZ == 100.0f);
    record("PositionFloatsRoundTrip", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testInCombatFlagByte() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    s->inCombatFlag738 = 1;
    const bool set_ok = (s->inCombatFlag738 == 1);
    s->inCombatFlag738 = 0;
    const bool ok = set_ok && (s->inCombatFlag738 == 0);
    record("InCombatFlagByte", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testDifficultyDefault() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    // Native ctor path leaves difficulty at 1.0f (member initializer).
    const bool ok = (s->difficulty734 == 1.0f);
    record("DifficultyDefault", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testTimeCounterIncrement() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    const uint32_t before = s->timeElapsedCounter6B0;
    s->timeElapsedCounter6B0 = before + 1;
    const bool ok = (s->timeElapsedCounter6B0 == before + 1);
    record("TimeCounterIncrement", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testDynamicBandRoundTrip() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterGlobal::create();
    const uint32_t magic = 0xDEADBEEFu;
    PlayerCharacterGlobal::setDynamicBandAt(kDynamicBandFirstIndex, magic);
    PlayerCharacterGlobal::setDynamicBandAt(kDynamicBandLastIndex, magic ^ 0x5A5A5A5Au);
    const uint32_t lo = PlayerCharacterGlobal::dynamicBandAt(kDynamicBandFirstIndex);
    const uint32_t hi = PlayerCharacterGlobal::dynamicBandAt(kDynamicBandLastIndex);
    // Mid index must stay zero after ctor value-init.
    const uint32_t mid = PlayerCharacterGlobal::dynamicBandAt(0x100);
    const bool ok = (lo == magic) && (hi == (magic ^ 0x5A5A5A5Au)) && (mid == 0u);
    record("DynamicBandRoundTrip", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testDynamicBandOutOfRangeRejected() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterGlobal::create();
    PlayerCharacterGlobal::setDynamicBandAt(0, 0xFFFFFFFFu);       // below band
    PlayerCharacterGlobal::setDynamicBandAt(0x80, 0xFFFFFFFFu);    // header slot
    PlayerCharacterGlobal::setDynamicBandAt(0x15C, 0xFFFFFFFFu);   // above band
    PlayerCharacterGlobal::setDynamicBandAt(0xFFFF, 0xFFFFFFFFu);  // far above
    const bool ok = (PlayerCharacterGlobal::dynamicBandAt(0) == 0u) &&
                    (PlayerCharacterGlobal::dynamicBandAt(0x80) == 0u) &&
                    (PlayerCharacterGlobal::dynamicBandAt(0x15C) == 0u) &&
                    (PlayerCharacterGlobal::dynamicBandAt(0xFFFF) == 0u);
    record("DynamicBandOutOfRangeRejected", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testViewStateFlagDefaultThirdPerson() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    // 0x588 == 0 means third person, the default state at load.
    const bool ok = (s->viewStateFlag588 == 0);
    record("ViewStateFlagDefaultThirdPerson", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

void PlayerCharacterStateTests::testViewStateFlagRoundTrip() {
    auto t0 = std::chrono::steady_clock::now();
    PlayerCharacterGlobal::destroy();
    PlayerCharacterState* s = PlayerCharacterGlobal::create();
    s->viewStateFlag588 = 1;  // Show1stPerson sets the flag
    const bool set_ok = (s->viewStateFlag588 == 1);
    s->viewStateFlag588 = 0;  // Show3rdPerson clears it
    const bool ok = set_ok && (s->viewStateFlag588 == 0);
    record("ViewStateFlagRoundTrip", ok, "", elapsed_ms(t0));
    PlayerCharacterGlobal::destroy();
}

bool PlayerCharacterStateTests::runAllTests() {
    results.clear();
    testSizeIs804();
    testInitSetsCtorBytes();
    testDoubleInitIdempotent();
    testDestroyClearsGlobal();
    testNullSafetyBeforeCreate();
    testLifecycleIsValid();
    testRecursionGuardRoundTrip();
    testPositionFloatsRoundTrip();
    testInCombatFlagByte();
    testDifficultyDefault();
    testTimeCounterIncrement();
    testDynamicBandRoundTrip();
    testDynamicBandOutOfRangeRejected();
    testViewStateFlagDefaultThirdPerson();
    testViewStateFlagRoundTrip();

    for (const auto& r : results) {
        std::printf("  [%s] %s (%.2f ms)%s\n", r.passed ? "PASS" : "FAIL",
                    r.testName.c_str(), r.durationMs,
                    r.message.empty() ? "" : (" - " + r.message).c_str());
    }
    return getFailCount() == 0;
}

int PlayerCharacterStateTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (r.passed) { ++n; }
    }
    return n;
}

int PlayerCharacterStateTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (!r.passed) { ++n; }
    }
    return n;
}

std::string PlayerCharacterStateTests::getSummary() const {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "PlayerCharacterStateTests: %d passed, %d failed, %d total",
                  getPassCount(), getFailCount(), static_cast<int>(results.size()));
    return std::string(buf);
}

}  // namespace game
