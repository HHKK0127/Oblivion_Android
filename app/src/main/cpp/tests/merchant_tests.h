#pragma once

// Merchant / barter layer tests.
//
// Covers the shop path added in Phase 68: base item resolution
// (game/item_catalog), shop construction from an NPC_ record with CNTO stock
// (game/merchant_factory), the two-direction trade primitives (game/barter)
// and the player inventory helpers they depend on.
//
// The suite builds a synthetic ESM image in memory so it runs without the real
// game data:
//   TES4 header
//   NPC_ 0x00001000  EDID "TestMerchant"  FULL "Collected Goods"
//                    CNTO Gold 250 / 0x00002000 3 / 0x00002001 1 / 0x00003000 2
//   NPC_ 0x00001001  EDID "EdidOnlyTrader"  CNTO Gold 0 / 0x00002000 1
//   NPC_ 0x00001002  EDID "EmptyTrader"     no CNTO
//   NPC_ 0x00001003  EDID "ClassedNpc"      CNAM 0x00004000
//   NPC_ 0x00001004  EDID "EdidClassNpc"    CNAM 0x00004001
//   NPC_ 0x00001005  EDID "StringClassNpc"  CNAM "AB" (legacy string form)
//   NPC_ 0x00001006  EDID "MissingClassNpc" CNAM 0x00007000 (no such class)
//   CLAS 0x00004000  EDID "TestWarrior"   FULL "Warrior"
//   CLAS 0x00004001  EDID "TestOnlyEdid"  (no FULL, so EDID is the name)
//   MISC 0x00002000  EDID "TestApple"  FULL "Test Apple"  DATA value 5 weight 0.5
//   MISC 0x00002001  EDID "TestGem"                      DATA value 250 weight 1.25
//
// The CLAS records are emitted after the actors they describe, so the suite
// also pins that CNAM resolution happens once the whole file is decoded.
//
// 0x00003000 has no record at all, so it exercises the unresolved placeholder
// path that decides whether a data gap is visible or silently priced at zero.

#include <string>
#include <vector>

struct MerchantTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class MerchantTests {
public:
    bool runAllTests();
    const std::vector<MerchantTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<MerchantTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};
