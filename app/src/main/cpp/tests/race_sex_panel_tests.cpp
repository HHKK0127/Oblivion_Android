#include "race_sex_panel_tests.h"

#include "../ui/race_sex_panel.h"
#include "../ui/ui_slider.h"
#include "../ui/ui_toggle.h"
#include "../game/chargen_commit.h"
#include "../game/player.h"

#include <android/log.h>
#include <array>
#include <cmath>
#include <string>

#define LOG_TAG "RaceSexPanelTests"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

using Page = RaceSexPanel::Page;
using ElemType = RaceSexPanel::ElemType;

// spec §2: pane id per slot (+0x40 array), in build order.
constexpr std::array<int, RaceSexPanel::PAGE_COUNT> kPaneId = {
    0, 209, 213, 224, 229, 239, 246, 257, 264, 275, 290,
    299, 306, 315, 336, 365, 376, 389, 402, 417, 424,
};

// spec §2: item count per page.
constexpr std::array<int, RaceSexPanel::PAGE_COUNT> kCount = {
    6, 3, 6, 4, 9, 6, 5, 3, 5, 7, 4, 3, 4, 10, 14, 5, 6, 6, 7, 3, 2,
};

// spec §2: {button, toggle, text, slider} per page.
constexpr std::array<std::array<int, 4>, RaceSexPanel::PAGE_COUNT> kTypeCount = {{
    {{3, 1, 0, 2}},  // Main
    {{0, 2, 1, 0}},  // Race
    {{0, 2, 0, 4}},  // Hair
    {{4, 0, 0, 0}},  // Face
    {{9, 0, 0, 0}},  // Shape
    {{6, 0, 0, 0}},  // Tone
    {{0, 0, 0, 5}},  // General(Face)
    {{0, 0, 0, 3}},  // Brow
    {{0, 0, 0, 5}},  // Cheeks
    {{0, 0, 0, 7}},  // Chin
    {{0, 0, 0, 4}},  // Eyes
    {{0, 0, 0, 3}},  // Forehead
    {{0, 0, 0, 4}},  // Jaw
    {{0, 0, 0, 10}}, // Mouth
    {{0, 0, 0, 14}}, // Nose
    {{0, 0, 0, 5}},  // General(Skin)
    {{0, 0, 0, 6}},  // Beard
    {{0, 0, 0, 6}},  // Eye shades
    {{0, 0, 0, 7}},  // Eyebrows
    {{0, 0, 0, 3}},  // Mouth(Lips)
    {{0, 0, 0, 2}},  // Nose shades
}};

// spec §2 #1: Main item labels, in build order.
constexpr const char* kMainLabels[] = {"Race", "Face", "Hair", "Eyes", "Age", "Complexion"};

// Distinctive labels pinned across the 20 non-Main pages (transcription guard).
constexpr const char* kSpotLabels[] = {
    "Gender", "Race Description", "Hair Color",
    "General(Face)", "Nose shades", "Eye shades",
    "Mouth-Chin distance short/long", "Nose sellion lower shallow/deep",
    "Naso labial lines light/dark", "Nares small/large",
    "Lipstick dark blue/light red",
};

int typeIndex(ElemType t) {
    switch (t) {
        case ElemType::Button: return 0;
        case ElemType::Toggle: return 1;
        case ElemType::Text:   return 2;
        case ElemType::Slider: return 3;
    }
    return -1;
}

}  // namespace

void RaceSexPanelTests::record(const std::string& name, bool passed,
                               const std::string& message) {
    results.push_back({name, passed, message});
    if (passed) {
        TEST_LOGI("[PASS] %s", name.c_str());
    } else {
        TEST_LOGE("[FAIL] %s: %s", name.c_str(), message.c_str());
    }
}

int RaceSexPanelTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (r.passed) ++count;
    }
    return count;
}

int RaceSexPanelTests::getFailCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (!r.passed) ++count;
    }
    return count;
}

std::string RaceSexPanelTests::getSummary() const {
    return "RaceSexPanel Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool RaceSexPanelTests::runAllTests() {
    results.clear();

    RaceSexPanel panel;
    const bool built = panel.init();
    const std::vector<RaceSexPanel::Element>& elements = panel.elements();

    record("init() succeeds", built);

    // 21 pages / 118 items (spec §2).
    record("118 items built", elements.size() == 118,
           "got " + std::to_string(elements.size()));

    bool paneOk = true;
    std::string paneMsg;
    for (int s = 0; s < RaceSexPanel::PAGE_COUNT; ++s) {
        if (panel.page_id(s) != kPaneId[s]) {
            paneOk = false;
            paneMsg = "slot " + std::to_string(s) + ": got " +
                      std::to_string(panel.page_id(s)) + " want " +
                      std::to_string(kPaneId[s]);
            break;
        }
    }
    record("21 pane ids match spec §2", paneOk, paneMsg);

    // Per-page item counts and type mix.
    std::array<int, RaceSexPanel::PAGE_COUNT> counts{};
    std::array<std::array<int, 4>, RaceSexPanel::PAGE_COUNT> typeCounts{};
    for (int s = 0; s < RaceSexPanel::PAGE_COUNT; ++s) {
        typeCounts[s] = {{0, 0, 0, 0}};
    }
    for (const auto& e : elements) {
        if (e.page_slot < 0 || e.page_slot >= RaceSexPanel::PAGE_COUNT) continue;
        counts[e.page_slot]++;
        const int ti = typeIndex(e.type);
        if (ti >= 0) typeCounts[e.page_slot][ti]++;
    }
    bool countOk = true;
    std::string countMsg;
    for (int s = 0; s < RaceSexPanel::PAGE_COUNT; ++s) {
        if (counts[s] != kCount[s]) {
            countOk = false;
            countMsg = "slot " + std::to_string(s) + " count " +
                       std::to_string(counts[s]) + " want " + std::to_string(kCount[s]);
            break;
        }
        for (int t = 0; t < 4; ++t) {
            if (typeCounts[s][t] != kTypeCount[s][t]) {
                countOk = false;
                countMsg = "slot " + std::to_string(s) + " type " + std::to_string(t) +
                           " = " + std::to_string(typeCounts[s][t]) + " want " +
                           std::to_string(kTypeCount[s][t]);
                break;
            }
        }
        if (!countOk) break;
    }
    record("per-page counts and type mix match spec §2", countOk, countMsg);

    // Every item has an id, a non-empty label, a valid page, and slider
    // user0 invariants. Ids strictly increase in build order.
    bool elemOk = true;
    std::string elemMsg;
    int prevId = 0;
    for (const auto& e : elements) {
        if (e.id <= 0 || e.label.empty() ||
            e.page_slot < 0 || e.page_slot >= RaceSexPanel::PAGE_COUNT) {
            elemOk = false;
            elemMsg = "bad element id=" + std::to_string(e.id) + " label='" + e.label + "'";
            break;
        }
        if (e.type == ElemType::Slider) {
            if (e.user0_id != e.id + 1) {
                elemOk = false;
                elemMsg = "slider id=" + std::to_string(e.id) + " user0=" +
                          std::to_string(e.user0_id) + " want " + std::to_string(e.id + 1);
                break;
            }
        } else if (e.user0_id != -1) {
            elemOk = false;
            elemMsg = "non-slider id=" + std::to_string(e.id) + " has user0";
            break;
        }
        if (e.id <= prevId) {
            elemOk = false;
            elemMsg = "ids not increasing at " + std::to_string(e.id);
            break;
        }
        prevId = e.id;
    }
    record("every item has id + label; slider user0 = id+1", elemOk, elemMsg);

    // +0x94 lookup: ids and slider user0 slots resolve; empty slots are null.
    bool lookupOk = true;
    std::string lookupMsg;
    for (const auto& e : elements) {
        const RaceSexPanel::Element* a = panel.find_element(e.id);
        if (a == nullptr || a->id != e.id) {
            lookupOk = false;
            lookupMsg = "find_element(" + std::to_string(e.id) + ") failed";
            break;
        }
        if (e.user0_id >= 0) {
            const RaceSexPanel::Element* b = panel.find_element(e.user0_id);
            if (b == nullptr || b->id != e.id) {
                lookupOk = false;
                lookupMsg = "user0 lookup " + std::to_string(e.user0_id) + " failed";
                break;
            }
        }
    }
    if (lookupOk && (panel.find_element(100) != nullptr || panel.find_element(429) != nullptr)) {
        lookupOk = false;
        lookupMsg = "unused slot is not null";
    }
    record("+0x94 lookup resolves ids and user0 slots", lookupOk, lookupMsg);

    // Main labels in build order.
    bool mainOk = (elements.size() >= 6);
    std::string mainMsg;
    if (mainOk) {
        for (int i = 0; i < 6; ++i) {
            if (elements[i].label != kMainLabels[i]) {
                mainOk = false;
                mainMsg = "Main[" + std::to_string(i) + "]='" + elements[i].label +
                          "' want '" + kMainLabels[i] + "'";
                break;
            }
        }
        // Main item ids: 201..208 with sliders at 205/207 (user0 206/208).
        const int expectIds[6] = {201, 202, 203, 204, 205, 207};
        for (int i = 0; i < 6 && mainOk; ++i) {
            if (elements[i].id != expectIds[i]) {
                mainOk = false;
                mainMsg = "Main[" + std::to_string(i) + "] id " +
                          std::to_string(elements[i].id) + " want " +
                          std::to_string(expectIds[i]);
            }
        }
    }
    record("Main six items (id 201-208) match spec §2 #1", mainOk, mainMsg);

    // Distinctive labels are present somewhere.
    bool spotOk = true;
    std::string spotMsg;
    for (const char* want : kSpotLabels) {
        bool found = false;
        for (const auto& e : elements) {
            if (e.label == want) { found = true; break; }
        }
        if (!found) {
            spotOk = false;
            spotMsg = std::string("missing label '") + want + "'";
            break;
        }
    }
    record("distinctive labels present", spotOk, spotMsg);

    // Element descriptor state (spec §5 template defaults) and the widget slot
    // stays null until instantiate() (device-only) runs.
    bool stateOk = true;
    std::string stateMsg;
    for (const auto& e : elements) {
        if (e.widget != nullptr) {
            stateOk = false;
            stateMsg = "widget not null before instantiate()";
            break;
        }
        if (e.type == ElemType::Slider) {
            if (e.min != 0.0f || e.max != 1.0f || e.step != 0.04f || e.jump != 0.1f) {
                stateOk = false;
                stateMsg = "slider id " + std::to_string(e.id) + " range/step/jump mismatch";
                break;
            }
        } else if (e.type == ElemType::Toggle) {
            if (e.checked) {
                stateOk = false;
                stateMsg = "toggle id " + std::to_string(e.id) + " starts checked";
                break;
            }
        }
    }
    record("descriptor state (spec §5) + widget null pre-instantiate", stateOk, stateMsg);

    // --- transition tables (spec §6) ---
    // chain: 19 rows, pane in range, next is -1 or a valid page. Exactly one -1
    // (Main). Hub/leaf counts: 2 -> Face, 10 -> Shape (Brow appears twice), 6 -> Tone.
    bool chainOk = (RaceSexPanel::CHAIN.size() == 19);
    std::string chainMsg;
    int toFace = 0, toShape = 0, toTone = 0, chainMinus1 = 0;
    for (const auto& r : RaceSexPanel::CHAIN) {
        if (r.pane < 0 || r.pane >= RaceSexPanel::PAGE_COUNT) {
            chainOk = false; chainMsg = "chain pane out of range"; break;
        }
        if (r.next == -1) {
            ++chainMinus1;
        } else if (r.next < 0 || r.next >= RaceSexPanel::PAGE_COUNT) {
            chainOk = false; chainMsg = "chain next out of range"; break;
        } else if (r.next == Page::PAGE_FACE) {
            ++toFace;
        } else if (r.next == Page::PAGE_SHAPE) {
            ++toShape;
        } else if (r.next == Page::PAGE_TONE) {
            ++toTone;
        }
    }
    if (chainOk && (chainMinus1 != 1 || toFace != 2 || toShape != 10 || toTone != 6)) {
        chainOk = false;
        chainMsg = "chain shape: -1=" + std::to_string(chainMinus1) +
                   " Face=" + std::to_string(toFace) +
                   " Shape=" + std::to_string(toShape) +
                   " Tone=" + std::to_string(toTone);
    }
    record("chain table walks to hub/leaf (spec §6.2)", chainOk, chainMsg);

    // family: 28 rows. Random/Reset use the sentinel pane values PAGE_RANDOM /
    // PAGE_RESET (no dedicated pane slot), so accept those alongside real pages.
    // Target is -1 or a valid page; exactly 8 are -1 (the finalize-only sites:
    // Random/Reset/Main-Age/Hair x4/Race-Gender).
    auto paneInRange = [](int pane) {
        return (pane >= 0 && pane < RaceSexPanel::PAGE_COUNT) ||
               pane == RaceSexPanel::PAGE_RANDOM || pane == RaceSexPanel::PAGE_RESET;
    };
    bool familyOk = (RaceSexPanel::FAMILY.size() == 28);
    std::string familyMsg;
    int famMinus1 = 0;
    for (const auto& r : RaceSexPanel::FAMILY) {
        if (!paneInRange(r.pane)) {
            familyOk = false; familyMsg = "family pane out of range"; break;
        }
        if (r.target == -1) {
            ++famMinus1;
        } else if (r.target < 0 || r.target >= RaceSexPanel::PAGE_COUNT) {
            familyOk = false; familyMsg = "family target out of range"; break;
        }
    }
    if (familyOk && famMinus1 != 8) {
        familyOk = false;
        familyMsg = "family -1 count " + std::to_string(famMinus1) + " want 8";
    }
    record("family table walks 28 tab transitions (spec §6.3)", familyOk, familyMsg);

    // --- widget behaviour (UISlider / UIToggle) ---
    // These widgets are compiled into the suite (run_host_tests.sh SOURCES);
    // RaceSexPanel::instantiate() itself is device-only and not exercised here.
    {
        UISlider slider("test");
        slider.setRange(0.0f, 1.0f, 0.04f, 0.1f);
        slider.setValue(5.0f);
        const bool clampHi = (slider.getValue() == 1.0f);
        slider.setValue(-3.0f);
        const bool clampLo = (slider.getValue() == 0.0f);
        record("UISlider clamps setValue to [min,max]", clampHi && clampLo,
               "hi=" + std::to_string(clampHi) + " lo=" + std::to_string(clampLo));

        slider.setValue(0.0f);
        slider.stepValue(+1);
        const bool stepUp = std::fabs(slider.getValue() - 0.04f) < 1e-6f;
        slider.setValue(1.0f);
        slider.stepValue(+1);
        const bool stepClamp = (slider.getValue() == 1.0f);
        record("UISlider stepValue moves by step (0.04) and clamps",
               stepUp && stepClamp, "v=" + std::to_string(slider.getValue()));
    }
    {
        UIToggle toggle("test");
        const bool start = !toggle.isChecked();
        toggle.toggleChecked();
        const bool on = toggle.isChecked();
        toggle.toggleChecked();
        const bool off = !toggle.isChecked();
        record("UIToggle flips checked on operation", start && on && off);
    }

    // --- V-631-1b: toggles flip on tap, and the Gender value reaches the commit ---
    // claims.md splits the five toggles: bool flips (204 Eyes, 210 Gender) vs
    // index cyclers (211 Race, 214 Style, 215 Hair Color). Only the bool pair
    // must flip here; a cycler that also flipped would write a bool where an
    // index is expected.
    {
        RaceSexPanel panel3;
        panel3.init();
        const int boolToggles[] = {RaceSexPanel::EYES_TOGGLE_ID,
                                   RaceSexPanel::GENDER_TOGGLE_ID};
        bool allFlip = true;
        std::string flipMsg;
        for (int id : boolToggles) {
            const bool before = panel3.toggleChecked(id);
            panel3.handleClick(id);
            const bool after = panel3.toggleChecked(id);
            if (before || !after) {
                allFlip = false;
                flipMsg += " id=" + std::to_string(id);
            }
        }
        record("toggle: bool toggles (204/210) flip on tap (V-631-1b)", allFlip, flipMsg);

        const int cyclerToggles[] = {RaceSexPanel::RACE_TOGGLE_ID,
                                     RaceSexPanel::STYLE_TOGGLE_ID,
                                     RaceSexPanel::HAIR_COLOR_TOGGLE_ID};
        bool cyclersClean = true;
        std::string cyclerMsg;
        for (int id : cyclerToggles) {
            panel3.handleClick(id);
            if (panel3.toggleChecked(id)) {
                cyclersClean = false;
                cyclerMsg += " id=" + std::to_string(id);
            }
        }
        record("toggle: index cyclers (211/214/215) do not flip (claims.md)",
               cyclersClean, cyclerMsg);
    }
    {
        // Gender: the flip is carried as the panel's value (OFF = Male).
        RaceSexPanel g;
        g.init();
        const bool maleStart = !g.genderFemale();
        g.handleClick(RaceSexPanel::GENDER_TOGGLE_ID);
        const bool female = g.genderFemale();
        g.handleClick(RaceSexPanel::GENDER_TOGGLE_ID);
        const bool backToMale = !g.genderFemale();
        record("toggle: Gender flips the panel value (OFF=Male, ON=Female)",
               maleStart && female && backToMale);
    }
    {
        // The commit consumes the Gender value. Player::gender is 0=Male/1=Female
        // (player.h:69); npcMgr is null so the race path is skipped and no ESM is
        // needed.
        Player player;
        chargen::CommitInput in;
        in.gender = 1;
        const chargen::CommitResult r = chargen::commit(nullptr, player, in);
        record("chargen::commit writes Player::gender (V-631-1b)",
               r.genderApplied && player.gender == 1,
               "applied=" + std::to_string(r.genderApplied) +
                   " gender=" + std::to_string(player.gender));
        const chargen::CommitResult r2 = chargen::commit(nullptr, player, in);
        record("chargen::commit leaves gender alone when unchanged", !r2.genderApplied);
    }

    // --- handleClick transition engine (spec §6; pure logic) ---
    {
        RaceSexPanel panel2;
        panel2.init();

        // leaf (Nose) Done -> up one level to Shape.
        panel2.setCurrentPage(Page::PAGE_NOSE);
        const bool consumed = panel2.handleClick(90);
        record("handleClick: leaf Done -> Shape",
               consumed && panel2.currentPage() == Page::PAGE_SHAPE);

        // Main Done -> the commit flow (spec §10.3), not a bare close.
        panel2.setCurrentPage(Page::PAGE_MAIN);
        int confirms = 0;
        panel2.setCurrentName("Bendu Olo");
        panel2.setPlayerName("Martin");
        panel2.setOnConfirm([&confirms](const std::string&, std::function<void(bool)> done) {
            ++confirms;
            done(true);
        });
        panel2.handleClick(90);
        record("handleClick: Main Done runs the commit flow",
               confirms == 1 && panel2.isConfirmed() && panel2.currentPage() == Page::PAGE_MAIN,
               "confirms=" + std::to_string(confirms));

        // family row with target == -1 (Age, id 205) -> consumed, page unchanged.
        panel2.setCurrentPage(Page::PAGE_MAIN);
        const bool ageConsumed = panel2.handleClick(205);
        record("handleClick: family target -1 keeps page",
               ageConsumed && panel2.currentPage() == Page::PAGE_MAIN);

        // Done with no chain row (Race) -> fall through to Main.
        panel2.setCurrentPage(Page::PAGE_RACE);
        panel2.handleClick(90);
        record("handleClick: unmatched Done -> Main",
               panel2.currentPage() == Page::PAGE_MAIN);

        // Main "Eyes" toggle (204) and "Complexion" slider (207) are NOT in
        // FAMILY and have no click handler in the original (v662): the click is
        // not consumed and the page stays Main (family no-match -> return, not
        // the chain tail's Main return).
        panel2.setCurrentPage(Page::PAGE_MAIN);
        const bool eyesConsumed = panel2.handleClick(204);
        const bool compConsumed = panel2.handleClick(207);
        record("handleClick(204/207): not in family -> unconsumed, page stays Main",
               !eyesConsumed && !compConsumed && panel2.currentPage() == Page::PAGE_MAIN);
    }

    // --- name / confirm / commit + race toggle (spec §10.2/§10.3/§10.4) ---
    {
        // Helper: a panel with two races and callbacks that record what happened.
        auto makePanel = [](RaceSexPanel& p, int& notifies, int& confirms, bool& lastYes,
                            int& commits) {
            p.init();
            p.setAvailableRaces({{0x100, "Imperial", "The Imperial race."},
                                 {0x101, "Orc", "The Orc race."}});
            p.setCurrentRaceFormID(0x100);
            p.setCurrentName("Bendu Olo");
            p.setPlayerName("Bendu Olo");
            p.setOnNotify([&notifies](const std::string&) { ++notifies; });
            p.setOnConfirm([&confirms, &lastYes](const std::string& body,
                                                 std::function<void(bool)> done) {
                ++confirms;
                lastYes = body.find("?") != std::string::npos;  // a confirm prompt
                done(true);
            });
            p.setOnCommit([&commits]() { ++commits; });
        };

        // (a) unchanged name -> notify only, no commit.
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            makePanel(p, notifies, confirms, lastYes, commits);
            p.setCurrentPage(Page::PAGE_MAIN);
            const auto action = p.handleDone();
            record("done: unchanged name -> notify, no commit",
                   action == RaceSexPanel::DoneAction::Notify && notifies == 1 &&
                       confirms == 0 && commits == 0 && !p.isConfirmed(),
                   "notify=" + std::to_string(notifies) + " commit=" + std::to_string(commits));
        }

        // (b) name with a space -> notify only.
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            makePanel(p, notifies, confirms, lastYes, commits);
            p.setPlayerName("Bendu Olo Jr");
            const auto action = p.handleDone();
            record("done: name with a space -> notify, no commit",
                   action == RaceSexPanel::DoneAction::Notify && notifies == 1 && commits == 0,
                   "notify=" + std::to_string(notifies) + " commit=" + std::to_string(commits));
        }

        // (c) empty name -> notify only.
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            makePanel(p, notifies, confirms, lastYes, commits);
            p.setPlayerName("");
            const auto action = p.handleDone();
            record("done: empty name -> notify, no commit",
                   action == RaceSexPanel::DoneAction::Notify && notifies == 1 && commits == 0);
        }

        // (d) valid new name -> confirm; Yes -> commit.
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            makePanel(p, notifies, confirms, lastYes, commits);
            p.setPlayerName("Martin");
            const auto action = p.handleDone();
            record("done: valid name -> confirm -> Yes commits",
                   action == RaceSexPanel::DoneAction::Confirm && lastYes && confirms == 1 &&
                       commits == 1 && p.isConfirmed(),
                   "confirm=" + std::to_string(confirms) + " commit=" + std::to_string(commits));
        }

        // (e) valid new name -> confirm; No -> no commit.
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            p.init();
            p.setAvailableRaces({{0x100, "Imperial", "d"}, {0x101, "Orc", "d"}});
            p.setCurrentRaceFormID(0x100);
            p.setCurrentName("Bendu Olo");
            p.setPlayerName("Martin");
            p.setOnConfirm([&confirms](const std::string&, std::function<void(bool)> done) {
                ++confirms;
                done(false);  // No
            });
            p.setOnCommit([&commits]() { ++commits; });
            const auto action = p.handleDone();
            record("done: confirm No -> no commit",
                   action == RaceSexPanel::DoneAction::Confirm && confirms == 1 && commits == 0 &&
                       !p.isConfirmed(),
                   "confirm=" + std::to_string(confirms) + " commit=" + std::to_string(commits));
        }

        // (f) article heuristic (V-654-2).
        record("articleFor: vowel -> an, consonant -> a",
               std::string(RaceSexPanel::articleFor("Orc")) == "an" &&
                   std::string(RaceSexPanel::articleFor("Martin")) == "a" &&
                   std::string(RaceSexPanel::articleFor("")) == "a");

        // (g) race toggle: clamp at both ends, no-op when unchanged.
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            makePanel(p, notifies, confirms, lastYes, commits);
            p.setSelectedRaceIndex(0);
            p.cycleRace(-1);  // left end clamps
            const bool clampLo = (p.selectedRaceIndex() == 0);
            p.setSelectedRaceIndex(p.raceCount() - 1);
            p.cycleRace(+1);  // right end clamps
            const bool clampHi = (p.selectedRaceIndex() == p.raceCount() - 1);
            record("race toggle: -1/+1 clamp at the ends", clampLo && clampHi,
                   "lo=" + std::to_string(clampLo) + " hi=" + std::to_string(clampHi));

            p.setSelectedRaceIndex(0);
            const bool sameNoop = !p.raceChanged();  // 0x100 == current 0x100
            p.setSelectedRaceIndex(1);
            const bool changed = p.raceChanged();     // 0x101 != current
            record("race: same as current -> no-op, different -> changed",
                   sameNoop && changed && p.selectedRaceFormID() == 0x101 &&
                       p.selectedRaceName() == "Orc");
        }

        // (h) tap halves: left -> -1, right -> +1, clamped at the ends.
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            makePanel(p, notifies, confirms, lastYes, commits);
            const float w = 500.0f;  // 2 races -> index [0,1]
            p.setSelectedRaceIndex(1);
            p.cycleRaceFromTap(10.0f, w);       // left half -> -1
            const bool leftDown = (p.selectedRaceIndex() == 0);
            p.cycleRaceFromTap(10.0f, w);       // left end clamps
            const bool leftClamp = (p.selectedRaceIndex() == 0);
            p.cycleRaceFromTap(w - 1.0f, w);    // right half -> +1
            const bool rightUp = (p.selectedRaceIndex() == 1);
            p.cycleRaceFromTap(w - 1.0f, w);    // right end clamps
            const bool rightClamp = (p.selectedRaceIndex() == 1);
            p.setSelectedRaceIndex(0);
            p.cycleRaceFromTap(w * 0.5f, w);    // midpoint = right half
            const bool midRight = (p.selectedRaceIndex() == 1);
            record("race tap: left -1 / right +1, clamped",
                   leftDown && leftClamp && rightUp && rightClamp && midRight,
                   "L=" + std::to_string(leftDown) + " R=" + std::to_string(rightUp));
        }

        // (i) handleClick(211) is consumed and does nothing (spec §10.4).
        {
            RaceSexPanel p;
            int notifies = 0, confirms = 0, commits = 0;
            bool lastYes = false;
            makePanel(p, notifies, confirms, lastYes, commits);
            p.setSelectedRaceIndex(0);
            p.setCurrentPage(Page::PAGE_RACE);
            const bool consumed = p.handleClick(RaceSexPanel::RACE_TOGGLE_ID);
            record("handleClick(211): consumed, no page change, no cycle",
                   consumed && p.currentPage() == Page::PAGE_RACE &&
                       p.selectedRaceIndex() == 0);
        }
    }

    // --- Random / Reset (spec §10.7; pure logic) ---
    {
        // The 90 slider values, in builder order.
        auto sliderValues = [](const RaceSexPanel& p) {
            std::vector<float> v;
            for (const auto& e : p.elements()) {
                if (e.type == ElemType::Slider) v.push_back(e.value);
            }
            return v;
        };

        // sliderCount == 90 (spec §2: 90 sliders over the 21 pages).
        {
            RaceSexPanel p;
            p.init();
            record("random/reset: sliderCount == 90", p.sliderCount() == 90,
                   std::to_string(p.sliderCount()));
        }

        // (a) Random: first click asks once and leaves values alone; Yes changes
        // them; the flag stays set, so the second click runs with no dialog.
        {
            RaceSexPanel p;
            p.init();
            p.setSeed(12345);
            p.setCurrentPage(Page::PAGE_FACE);
            int confirms = 0;
            std::function<void(bool)> pending;
            p.setOnConfirm([&confirms, &pending](const std::string&,
                                                 std::function<void(bool)> done) {
                ++confirms;
                pending = std::move(done);
            });
            const auto before = sliderValues(p);
            const bool consumed = p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);
            record("random: first click -> confirm once, values unchanged",
                   consumed && confirms == 1 && sliderValues(p) == before &&
                       p.randomArmed() && p.currentPage() == Page::PAGE_FACE,
                   "confirms=" + std::to_string(confirms));

            if (pending) pending(true);
            const auto afterYes = sliderValues(p);
            record("random: Yes -> values change, page stays Face",
                   afterYes != before && p.currentPage() == Page::PAGE_FACE);

            const int confirmsBefore = confirms;
            p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);
            record("random: second click -> no dialog, immediate change",
                   confirms == confirmsBefore && sliderValues(p) != afterYes,
                   "confirms=" + std::to_string(confirms));
        }

        // (a2) the flag is set BEFORE the dialog is shown: a click that arrives
        // while the first dialog is still open runs the body directly (no second
        // confirm). This pins the "armed=true -> on_confirm_" order.
        {
            RaceSexPanel p;
            p.init();
            p.setSeed(4242);
            p.setCurrentPage(Page::PAGE_FACE);
            int confirms = 0;
            std::function<void(bool)> pending;
            p.setOnConfirm([&confirms, &pending](const std::string&,
                                                 std::function<void(bool)> done) {
                ++confirms;
                pending = std::move(done);
            });
            const auto before = sliderValues(p);
            p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);  // dialog 1 (left open)
            const bool armedAfterFirst = p.randomArmed();
            p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);  // flag already set -> body
            record("random: flag set before the dialog (2nd click skips it)",
                   confirms == 1 && armedAfterFirst && sliderValues(p) != before,
                   "confirms=" + std::to_string(confirms));
        }

        // (b) Random: No leaves values alone and clears the flag (the original's
        // cb clears [0xB3B4C8] on a non-Yes answer), so the next click asks again.
        {
            RaceSexPanel p;
            p.init();
            p.setSeed(777);
            p.setCurrentPage(Page::PAGE_FACE);
            int confirms = 0;
            std::function<void(bool)> pending;
            p.setOnConfirm([&confirms, &pending](const std::string&,
                                                 std::function<void(bool)> done) {
                ++confirms;
                pending = std::move(done);
            });
            const auto before = sliderValues(p);
            p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);
            if (pending) pending(false);  // No
            record("random: No -> values unchanged, flag cleared",
                   confirms == 1 && sliderValues(p) == before && !p.randomArmed());
            p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);
            record("random: after No the next click asks again",
                   confirms == 2 && p.randomArmed(),
                   "confirms=" + std::to_string(confirms));
        }

        // (c) Random: fixed seed reproduces the draw; all values in [0,1].
        {
            auto draw = [&sliderValues](uint32_t seed) {
                RaceSexPanel p;
                p.init();
                p.setSeed(seed);
                p.setCurrentPage(Page::PAGE_FACE);
                std::function<void(bool)> pending;
                p.setOnConfirm([&pending](const std::string&,
                                          std::function<void(bool)> done) {
                    pending = std::move(done);
                });
                p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);
                if (pending) pending(true);
                return sliderValues(p);
            };
            const auto a = draw(2026);
            const auto b = draw(2026);
            const auto c = draw(2027);
            bool inRange = (a.size() == 90);
            for (float v : a) {
                if (!(v >= 0.0f && v <= 1.0f)) inRange = false;
            }
            record("random: 90 values in [0,1], fixed seed reproducible",
                   inRange && a == b && a != c, "n=" + std::to_string(a.size()));
        }

        // (d) user3 (0xFB1) 3-writes land only on (Main, Age)/(Main, Complexion).
        {
            RaceSexPanel p;
            p.init();
            p.setSeed(5);
            p.setCurrentPage(Page::PAGE_FACE);
            std::function<void(bool)> pending;
            p.setOnConfirm([&pending](const std::string&,
                                      std::function<void(bool)> done) {
                pending = std::move(done);
            });
            p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);
            if (pending) pending(true);
            int touched = 0;
            bool onlyTwo = true;
            for (const auto& e : p.elements()) {
                if (e.user3_writes == 0) continue;
                ++touched;
                const bool isMainAge =
                    (e.page_slot == Page::PAGE_MAIN && e.label == "Age");
                const bool isMainComp =
                    (e.page_slot == Page::PAGE_MAIN && e.label == "Complexion");
                if (!(isMainAge || isMainComp) || e.user3_writes != 3 ||
                    e.user3 != 0.0f) {
                    onlyTwo = false;
                }
            }
            record("random: user3 3-writes only on (Main,Age)/(Main,Complexion)",
                   touched == 2 && onlyTwo, "touched=" + std::to_string(touched));
        }

        // (e) Reset: Yes zeroes every slider and keeps the page on Face.
        {
            RaceSexPanel p;
            p.init();
            p.setSeed(99);
            p.setCurrentPage(Page::PAGE_FACE);
            int confirms = 0;
            std::function<void(bool)> pending;
            p.setOnConfirm([&confirms, &pending](const std::string&,
                                                 std::function<void(bool)> done) {
                ++confirms;
                pending = std::move(done);
            });
            // Randomize first so the sliders are non-zero.
            p.handleClick(RaceSexPanel::RANDOM_BUTTON_ID);
            if (pending) pending(true);
            bool anyNonZero = false;
            for (float v : sliderValues(p)) {
                if (v != 0.0f) anyNonZero = true;
            }
            // Reset: first click asks, Yes zeroes.
            p.handleClick(RaceSexPanel::RESET_BUTTON_ID);
            if (pending) pending(true);
            const auto resetVals = sliderValues(p);
            bool allZero = (resetVals.size() == 90);
            for (float v : resetVals) {
                if (v != 0.0f) allZero = false;
            }
            record("reset: Yes -> all 90 sliders 0.0, page stays Face",
                   anyNonZero && allZero && p.currentPage() == Page::PAGE_FACE,
                   "n=" + std::to_string(resetVals.size()));
        }
    }

    // --- race application to the Player (spec §10.8; pure logic) ---
    {
        // AV 0x0E = Blade, 0x12 = Heavy Armor, 0x0C = Armorer (canonical order).
        Player p;  // struct defaults = the chargen base (10 skills / 40 attrs)
        Player::RaceBonus a;
        a.skillBonuses = {{0x0E, 10}, {0x12, 5}};  // Blade +10, Heavy Armor +5
        a.spells = {0xAAA, 0xBBB};
        p.applyRace(a);
        record("race: skill bonuses applied on the base",
               p.skills.Blade == 20 && p.skills.HeavyArmor == 15 &&
                   p.skills.Armorer == 10 && p.knownSpells.size() == 2,
               "Blade=" + std::to_string(p.skills.Blade) +
                   " HA=" + std::to_string(p.skills.HeavyArmor) +
                   " known=" + std::to_string(p.knownSpells.size()));

        // Re-applying the SAME race must not double the bonus or the spells.
        p.applyRace(a);
        record("race: same race re-apply -> no double bonus / no dup spells",
               p.skills.Blade == 20 && p.skills.HeavyArmor == 15 &&
                   p.knownSpells.size() == 2,
               "Blade=" + std::to_string(p.skills.Blade) +
                   " known=" + std::to_string(p.knownSpells.size()));

        // Switching race drops the previous bonus and spells.
        Player::RaceBonus b;
        b.skillBonuses = {{0x0C, 7}};  // Armorer +7
        b.spells = {0xCCC};
        p.applyRace(b);
        record("race: switch -> previous bonus/spells gone, new applied",
               p.skills.Blade == 10 && p.skills.HeavyArmor == 10 &&
                   p.skills.Armorer == 17 && p.knownSpells.size() == 1 &&
                   p.knownSpells[0] == 0xCCC,
               "Armorer=" + std::to_string(p.skills.Armorer) +
                   " known=" + std::to_string(p.knownSpells.size()));

        // An unused slot (0xFF) or an out-of-window AV is ignored.
        Player q;
        Player::RaceBonus c;
        c.skillBonuses = {{0xFF, 50}, {0x00, 50}, {0x21, 50}};
        q.applyRace(c);
        record("race: 0xFF / out-of-window AV ignored",
               q.skills.Armorer == 10 && q.skills.Blade == 10 &&
                   q.skills.Speechcraft == 10,
               "Armorer=" + std::to_string(q.skills.Armorer));

        // Attributes and vitals revert to the chargen base.
        Player r;
        r.attributes.Strength = 99;
        r.maxHealth = 5.0f;
        r.applyRace({});
        record("race: attributes/vitals revert to the base",
               r.attributes.Strength == Player::BASE_ATTRIBUTE &&
                   r.maxHealth == Player::BASE_MAX_HEALTH &&
                   r.health == r.maxHealth && r.skills.Blade == Player::BASE_SKILL,
               "Str=" + std::to_string(r.attributes.Strength) +
                   " HP=" + std::to_string(static_cast<int>(r.maxHealth)));
    }

    // --- geometry + hitTest (spec §9; pure logic) ---
    {
        RaceSexPanel g;
        g.init();
        g.setVisible(true);

        // §9.2: the gutter is a constant 40px, only when >6 items.
        const int mainW = g.paneWidth(Page::PAGE_MAIN);    // 6 items
        const int shapeW = g.paneWidth(Page::PAGE_SHAPE);  // 9 items
        const int noseW = g.paneWidth(Page::PAGE_NOSE);    // 14 items
        record("paneWidth: 500 without gutter, 460 with (Main/Shape/Nose)",
               mainW == 500 && shapeW == 460 && noseW == 460,
               "main=" + std::to_string(mainW) + " shape=" + std::to_string(shapeW) +
                   " nose=" + std::to_string(noseW));

        // §9.4: listindex -> id, in build order.
        record("elementIdAt: Main row 0/5",
               g.elementIdAt(Page::PAGE_MAIN, 0) == 201 &&
                   g.elementIdAt(Page::PAGE_MAIN, 5) == 207 &&
                   g.elementIdAt(Page::PAGE_MAIN, 6) == -1);

        // §9.1 row 0 -> first item id.
        g.setCurrentPage(Page::PAGE_MAIN);
        record("hitTest: row 0 -> first item id", g.hitTest(10, 0) == 201,
               "got " + std::to_string(g.hitTest(10, 0)));

        // row == count -> -1 (6 items * 93).
        record("hitTest: row == count -> -1", g.hitTest(10, 6 * 93) == -1);

        // §9.4 scroll rows 2: the same y lands two rows lower. Main has only 6
        // items (max scroll 0), so use Shape (9 items, max scroll 3).
        g.setCurrentPage(Page::PAGE_SHAPE);
        g.setScrollRows(2);
        record("hitTest: scroll_rows 2 -> row 2", g.hitTest(10, 0) == 232,
               "got " + std::to_string(g.hitTest(10, 0)));
        // Above the scrolled viewport -> -1.
        record("hitTest: y above scrolled viewport -> -1", g.hitTest(10, -200) == -1);
        g.setScrollRows(0);
        g.setCurrentPage(Page::PAGE_MAIN);

        // §9.2 right gutter (scroll bar side) -> -1; the same x is live on a
        // 6-item page (no gutter) so the difference is the gutter itself.
        const bool gutterMain = (g.hitTest(499, 0) == 201);
        g.setCurrentPage(Page::PAGE_SHAPE);
        const bool gutterShape = (g.hitTest(470, 0) == -1) && (g.hitTest(455, 0) == 230);
        record("hitTest: x >= pane_w -> -1 (gutter only with >6 items)",
               gutterMain && gutterShape);

        // §9.3 Done (id 90) rect, checked before the pane.
        g.setCurrentPage(Page::PAGE_MAIN);
        record("hitTest: Done rect -> 90",
               g.hitTest(RaceSexPanel::DONE_X, RaceSexPanel::DONE_Y) == 90 &&
                   g.hitTest(RaceSexPanel::DONE_X + RaceSexPanel::DONE_WIDTH - 1,
                             RaceSexPanel::DONE_Y + RaceSexPanel::DONE_HEIGHT - 1) == 90);
        record("hitTest: outside Done rect -> not 90",
               g.hitTest(RaceSexPanel::DONE_X - 1, RaceSexPanel::DONE_Y) != 90 &&
                   g.hitTest(RaceSexPanel::DONE_X,
                             RaceSexPanel::DONE_Y + RaceSexPanel::DONE_HEIGHT) != 90);

        // Hidden panel ignores everything.
        g.setVisible(false);
        record("hitTest: hidden panel -> -1", g.hitTest(10, 0) == -1 &&
                                                  g.hitTest(RaceSexPanel::DONE_X,
                                                            RaceSexPanel::DONE_Y) == -1);
        g.setVisible(true);

        // Canvas -> device conversion: 4:3 1280x960 is scale 1, no offset.
        RaceSexPanel s;
        s.setScreenSize(1280, 960);
        s.init();
        s.setVisible(true);
        s.setCurrentPage(Page::PAGE_MAIN);
        record("hitTestScreen: 1280x960 scale 1, origin (100,195)",
               s.hitTestScreen(110.0f, 195.0f) == 201 &&
                   s.hitTestScreen(100.0f, 195.0f) == 201 &&
                   s.hitTestScreen(99.0f, 195.0f) == -1);
        // 1920x1080: scale 1.125, letterbox x offset 240 -> origin (352.5, 219.375).
        s.setScreenSize(1920, 1080);
        record("hitTestScreen: 1920x1080 letterbox offset",
               std::fabs(s.layoutScale() - 1.125f) < 1e-6f &&
                   std::fabs(s.panelOriginX() - 352.5f) < 1e-3f &&
                   std::fabs(s.panelOriginY() - 219.375f) < 1e-3f &&
                   s.hitTestScreen(362.5f, 219.375f) == 201);
    }

    // --- layout / draw rects + row culling (spec §9.5; pure logic) ---
    {
        const auto close = [](float a, float b) { return std::fabs(a - b) < 1e-3f; };

        RaceSexPanel p;
        p.setScreenSize(1280, 960);  // scale 1, origin (100,195)
        p.init();
        p.setVisible(true);
        p.setCurrentPage(Page::PAGE_MAIN);

        // Canvas rect (spec §9.1): y = row*93 - scroll*93. Shape (9 items) can
        // scroll; Main (6 items) is clamped to 0.
        const auto c0 = p.itemRectCanvas(Page::PAGE_MAIN, 0);
        const auto c5 = p.itemRectCanvas(Page::PAGE_MAIN, 5);
        record("itemRectCanvas: item 0 y=0, item 5 y=465",
               close(c0.y, 0.0f) && close(c5.y, 465.0f),
               "y0=" + std::to_string(c0.y) + " y5=" + std::to_string(c5.y));
        p.setCurrentPage(Page::PAGE_SHAPE);
        p.setScrollRows(2);
        const auto cs = p.itemRectCanvas(Page::PAGE_SHAPE, 0);
        record("itemRectCanvas: scroll=2 -> item 0 y=-186", close(cs.y, -186.0f),
               "y=" + std::to_string(cs.y));
        p.setScrollRows(0);
        p.setCurrentPage(Page::PAGE_MAIN);

        // Device rect: canvas -> layoutScale + panelOrigin. 1280x960 is 1:1.
        const auto d0 = p.itemRectDevice(Page::PAGE_MAIN, 0);
        const auto d5 = p.itemRectDevice(Page::PAGE_MAIN, 5);
        record("itemRectDevice: 1280x960 item 0 (100,195,500,93), item 5 y=660",
               close(d0.x, 100.0f) && close(d0.y, 195.0f) && close(d0.w, 500.0f) &&
                   close(d0.h, 93.0f) && close(d5.y, 660.0f),
               "d0=(" + std::to_string(d0.x) + "," + std::to_string(d0.y) + "," +
                   std::to_string(d0.w) + "," + std::to_string(d0.h) + ") d5y=" +
                   std::to_string(d5.y));
        p.setCurrentPage(Page::PAGE_SHAPE);
        p.setScrollRows(2);
        const auto ds = p.itemRectDevice(Page::PAGE_SHAPE, 0);
        record("itemRectDevice: scroll=2 -> device y=9", close(ds.y, 9.0f),
               "y=" + std::to_string(ds.y));
        p.setScrollRows(0);
        p.setCurrentPage(Page::PAGE_MAIN);

        // 1920x1080: everything scales by 1.125 and shifts by the origin.
        p.setScreenSize(1920, 1080);
        const auto h0 = p.itemRectDevice(Page::PAGE_MAIN, 0);
        record("itemRectDevice: 1920x1080 item 0 (352.5,219.375,562.5,104.625)",
               close(h0.x, 352.5f) && close(h0.y, 219.375f) && close(h0.w, 562.5f) &&
                   close(h0.h, 104.625f),
               "(" + std::to_string(h0.x) + "," + std::to_string(h0.y) + "," +
                   std::to_string(h0.w) + "," + std::to_string(h0.h) + ")");
        p.setScreenSize(1280, 960);

        // Pane width reaches the item width (500 without gutter / 460 with).
        record("item width reflects paneWidth (500 Main / 460 Shape)",
               close(p.itemRectCanvas(Page::PAGE_MAIN, 0).w, 500.0f) &&
                   close(p.itemRectCanvas(Page::PAGE_SHAPE, 0).w, 460.0f));

        // §9.5 culling: visibleRows = race_scroll_bar.user8 = 6 = 558/93.
        record("visibleRows = 6 (558/93 == race_scroll_bar.user8)",
               p.visibleRows() == 6, std::to_string(p.visibleRows()));
        p.setCurrentPage(Page::PAGE_SHAPE);  // 9 items
        record("culling: scroll=0 keeps rows 0..5, hides row 6",
               p.isElementVisible(Page::PAGE_SHAPE, 0) &&
                   p.isElementVisible(Page::PAGE_SHAPE, 5) &&
                   !p.isElementVisible(Page::PAGE_SHAPE, 6) &&
                   !p.isElementVisible(Page::PAGE_SHAPE, 8));
        p.setScrollRows(2);
        record("culling: scroll=2 hides rows 0/1, keeps 2..7, hides 8",
               !p.isElementVisible(Page::PAGE_SHAPE, 0) &&
                   !p.isElementVisible(Page::PAGE_SHAPE, 1) &&
                   p.isElementVisible(Page::PAGE_SHAPE, 2) &&
                   p.isElementVisible(Page::PAGE_SHAPE, 7) &&
                   !p.isElementVisible(Page::PAGE_SHAPE, 8));
        p.setScrollRows(0);

        // Non-current pages are never draw targets.
        record("culling: non-current page is hidden",
               !p.isElementVisible(Page::PAGE_MAIN, 0) &&
                   !p.isElementVisible(Page::PAGE_NOSE, 0) &&
                   p.isElementVisible(Page::PAGE_SHAPE, 0));

        // rowOfElement == build order within the page.
        record("rowOfElement: Main Race=0, Complexion=5",
               p.rowOfElement(201) == 0 && p.rowOfElement(207) == 5 &&
                   p.rowOfElement(99999) == -1);

        // Done / scroll bar rects are the same transform hitTestScreen() uses.
        p.setCurrentPage(Page::PAGE_MAIN);
        const auto done = p.doneRectDevice();
        const int doneHit = p.hitTestScreen(done.x + done.w * 0.5f, done.y + done.h * 0.5f);
        record("doneRectDevice center -> hitTestScreen == 90", doneHit == 90,
               "hit=" + std::to_string(doneHit));
        p.setCurrentPage(Page::PAGE_SHAPE);  // 9 items -> 460 pane, scroll gutter
        const auto bar = p.scrollBarRectDevice();
        const int barHit = p.hitTestScreen(bar.x + bar.w * 0.5f, bar.y + bar.h * 0.5f);
        record("scrollBarRectDevice center -> hitTestScreen == -1 (gutter)", barHit == -1,
               "hit=" + std::to_string(barHit));
    }

    // --- scroll input (spec §9.7; pure logic) ---
    {
        const auto close = [](float a, float b) { return std::fabs(a - b) < 1e-3f; };
        (void)close;

        RaceSexPanel p;
        p.setScreenSize(1280, 960);  // scale 1
        p.init();
        p.setVisible(true);

        // clamp: [0, count - visibleRows].
        p.setCurrentPage(Page::PAGE_MAIN);  // 6 items -> max 0
        p.setScrollRows(-1);
        const bool mainLo = (p.scrollRows() == 0);
        p.setScrollRows(999);
        record("setScrollRows: count<=6 stays 0", mainLo && p.scrollRows() == 0,
               "v=" + std::to_string(p.scrollRows()));

        p.setCurrentPage(Page::PAGE_SHAPE);  // 9 items -> max 3
        p.setScrollRows(999);
        const bool shapeHi = (p.scrollRows() == 3);
        p.setScrollRows(-1);
        const bool shapeLo = (p.scrollRows() == 0);
        record("setScrollRows: Shape clamps to [0,3]", shapeHi && shapeLo,
               "hi=" + std::to_string(shapeHi) + " lo=" + std::to_string(shapeLo));

        // The layout hook fires only on an actual change.
        int layouts = 0;
        p.setOnLayoutChanged([&layouts]() { ++layouts; });
        p.setScrollRows(0);  // no change
        const bool noChange = (layouts == 0);
        p.setScrollRows(1);  // change
        const bool oneChange = (layouts == 1);
        p.setScrollRows(1);  // no change
        record("setScrollRows: re-layout only on change",
               noChange && oneChange && layouts == 1, "layouts=" + std::to_string(layouts));

        // Bar hit-testing.
        p.setCurrentPage(Page::PAGE_SHAPE);
        p.setScrollRows(0);
        const auto bar = p.scrollBarRectDevice();
        const float bx = bar.x + bar.w * 0.5f;
        const float by = bar.y + 10.0f;
        record("isScrollBarHit: bar centre captured", p.isScrollBarHit(bx, by));
        record("isScrollBarHit: a pane row is not the bar",
               !p.isScrollBarHit(100.0f, 300.0f));
        p.setCurrentPage(Page::PAGE_MAIN);
        const bool noBar6 = !p.isScrollBarHit(bx, by);
        p.setCurrentPage(Page::PAGE_SHAPE);
        record("isScrollBarHit: 6-item page has no bar", noBar6);

        // +93px drag = +1 row (scale 1), and back.
        p.setScrollRows(0);
        const bool captured = p.beginScrollDrag(bx, by);
        p.updateScrollDrag(by + 93.0f);
        const bool dragDown = (p.scrollRows() == 1);
        p.updateScrollDrag(by);
        const bool dragBack = (p.scrollRows() == 0);
        p.endScrollDrag();
        record("drag: +93px -> +1 row, back -> 0", captured && dragDown && dragBack,
               "down=" + std::to_string(dragDown) + " back=" + std::to_string(dragBack));

        // -93px -> -1 row (from scroll 2).
        p.setScrollRows(2);
        p.beginScrollDrag(bx, by);
        p.updateScrollDrag(by - 93.0f);
        const bool dragUp = (p.scrollRows() == 1);
        p.endScrollDrag();
        record("drag: -93px -> -1 row", dragUp, "v=" + std::to_string(p.scrollRows()));

        // A drag that does not start on the bar is not captured.
        record("beginScrollDrag: off-bar -> false",
               !p.beginScrollDrag(100.0f, 300.0f) && !p.isScrollDragging());

        // scale 1.125: the row pitch scales with layoutScale.
        p.setScreenSize(1920, 1080);
        p.setCurrentPage(Page::PAGE_SHAPE);
        p.setScrollRows(0);
        const auto bar2 = p.scrollBarRectDevice();
        const float bx2 = bar2.x + bar2.w * 0.5f;
        const float by2 = bar2.y + 10.0f;
        p.beginScrollDrag(bx2, by2);
        p.updateScrollDrag(by2 + 104.625f);  // exactly one row at 1.125
        const bool dragHi = (p.scrollRows() == 1);
        p.updateScrollDrag(by2 + 93.0f);     // 93px < one row -> still rounds to 1
        const bool drag93 = (p.scrollRows() == 1);
        p.endScrollDrag();
        record("drag: 1.125 scale (104.625px / 93px) -> +1", dragHi && drag93,
               "hi=" + std::to_string(dragHi) + " 93=" + std::to_string(drag93));
        p.setScreenSize(1280, 960);

        // Rounding rule (spec §9.7): half a row rounds away from zero.
        p.setCurrentPage(Page::PAGE_SHAPE);
        const auto bar3 = p.scrollBarRectDevice();
        const float bx3 = bar3.x + bar3.w * 0.5f;
        const float by3 = bar3.y + 10.0f;
        p.setScrollRows(1);
        p.beginScrollDrag(bx3, by3);
        p.updateScrollDrag(by3 + 46.5f);  // exactly half a row -> +1
        const bool halfUp = (p.scrollRows() == 2);
        p.endScrollDrag();
        p.setScrollRows(1);
        p.beginScrollDrag(bx3, by3);
        p.updateScrollDrag(by3 - 46.5f);  // half a row up -> -1
        const bool halfDown = (p.scrollRows() == 0);
        p.endScrollDrag();
        record("drag: half a row rounds away from zero (+1/-1)", halfUp && halfDown,
               "up=" + std::to_string(halfUp) + " down=" + std::to_string(halfDown));

        // The drag updates the culling.
        p.setCurrentPage(Page::PAGE_SHAPE);
        p.setScrollRows(0);
        const bool row0Before = p.isElementVisible(Page::PAGE_SHAPE, 0);
        p.setScrollRows(2);
        const bool rowsAfter = !p.isElementVisible(Page::PAGE_SHAPE, 0) &&
                               p.isElementVisible(Page::PAGE_SHAPE, 2);
        record("drag re-culls rows", row0Before && rowsAfter);

        // A page switch resets the scroll (user7 is shared).
        p.setScrollRows(2);
        p.handleClick(90);  // Shape Done -> Face
        record("page switch resets scroll to 0",
               p.currentPage() == Page::PAGE_FACE && p.scrollRows() == 0,
               "page=" + std::to_string(p.currentPage()) +
                   " scroll=" + std::to_string(p.scrollRows()));

        // A tap on the bar is captured, so Done(90) must not fire (the renderer's
        // fall-through only runs when beginScrollDrag returns false).
        RaceSexPanel q;
        q.setScreenSize(1280, 960);
        q.init();
        q.setVisible(true);
        int done = 0;
        q.setOnDone([&done]() { ++done; });
        q.setCurrentPage(Page::PAGE_SHAPE);
        const auto qbar = q.scrollBarRectDevice();
        const float qbx = qbar.x + qbar.w * 0.5f;
        const float qby = qbar.y + qbar.h * 0.5f;
        const bool qCaptured = q.beginScrollDrag(qbx, qby);
        if (!qCaptured) {
            const int id = q.hitTestScreen(qbx, qby);
            if (id >= 0) q.handleClick(id);
        }
        q.endScrollDrag();
        record("bar tap: captured, Done(90) not fired",
               qCaptured && done == 0 && q.currentPage() == Page::PAGE_SHAPE,
               "captured=" + std::to_string(qCaptured) + " done=" + std::to_string(done));
    }

    // --- name label + visibleRows source (spec §10.1 / §9.5b) ---
    {
        const auto close = [](float a, float b) { return std::fabs(a - b) < 1e-3f; };

        RaceSexPanel p;
        p.setScreenSize(1280, 960);  // scale 1, origin (100,195)
        p.init();
        p.setVisible(true);

        // §10.1: the name shown on race_name comes from the player.
        p.setPlayerName("Bendu Olo");
        record("setPlayerName stores the name", p.playerName() == "Bendu Olo");

        // race_name canvas (250,-65) -> device (350,130,425,30) at 1:1.
        const auto nr = p.nameRectDevice();
        record("nameRectDevice: 1280x960 (350,130,425,30)",
               close(nr.x, 350.0f) && close(nr.y, 130.0f) && close(nr.w, 425.0f) &&
                   close(nr.h, 30.0f),
               "(" + std::to_string(nr.x) + "," + std::to_string(nr.y) + "," +
                   std::to_string(nr.w) + "," + std::to_string(nr.h) + ")");

        // §9.5b: visibleRows defaults to race_scroll_bar.user8 = 6 and can be set.
        record("visibleRows defaults to 6 (user8)", p.visibleRows() == 6,
               std::to_string(p.visibleRows()));
        p.setCurrentPage(Page::PAGE_NOSE);  // 14 items
        p.setVisibleRows(4);
        record("setVisibleRows drives the culling",
               p.visibleRows() == 4 && p.isElementVisible(Page::PAGE_NOSE, 3) &&
                   !p.isElementVisible(Page::PAGE_NOSE, 4));
    }

    return getFailCount() == 0;
}
