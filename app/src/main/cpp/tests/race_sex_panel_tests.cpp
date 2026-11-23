#include "race_sex_panel_tests.h"

#include "../ui/race_sex_panel.h"
#include "../ui/ui_slider.h"
#include "../ui/ui_toggle.h"

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

    // --- handleClick transition engine (spec §6; pure logic) ---
    {
        RaceSexPanel panel2;
        panel2.init();
        int done = 0;
        panel2.setOnDone([&done]() { ++done; });

        // leaf (Nose) Done -> up one level to Shape.
        panel2.setCurrentPage(Page::PAGE_NOSE);
        const bool consumed = panel2.handleClick(90);
        record("handleClick: leaf Done -> Shape",
               consumed && panel2.currentPage() == Page::PAGE_SHAPE);

        // Main Done -> onDone fired, page stays Main.
        panel2.setCurrentPage(Page::PAGE_MAIN);
        panel2.handleClick(90);
        record("handleClick: Main Done fires onDone",
               done == 1 && panel2.currentPage() == Page::PAGE_MAIN);

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

        // §9.4 scroll rows 2: the same y lands two rows lower.
        g.setScrollRows(2.0f);
        record("hitTest: scroll_rows 2 -> row 2", g.hitTest(10, 0) == 203,
               "got " + std::to_string(g.hitTest(10, 0)));
        // Above the scrolled viewport -> -1.
        record("hitTest: y above scrolled viewport -> -1", g.hitTest(10, -200) == -1);
        g.setScrollRows(0.0f);

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

        // Canvas rect (spec §9.1): y = row*93 - scroll*93.
        const auto c0 = p.itemRectCanvas(Page::PAGE_MAIN, 0);
        const auto c5 = p.itemRectCanvas(Page::PAGE_MAIN, 5);
        record("itemRectCanvas: item 0 y=0, item 5 y=465",
               close(c0.y, 0.0f) && close(c5.y, 465.0f),
               "y0=" + std::to_string(c0.y) + " y5=" + std::to_string(c5.y));
        p.setScrollRows(2.0f);
        const auto cs = p.itemRectCanvas(Page::PAGE_MAIN, 0);
        record("itemRectCanvas: scroll=2 -> item 0 y=-186", close(cs.y, -186.0f),
               "y=" + std::to_string(cs.y));
        p.setScrollRows(0.0f);

        // Device rect: canvas -> layoutScale + panelOrigin. 1280x960 is 1:1.
        const auto d0 = p.itemRectDevice(Page::PAGE_MAIN, 0);
        const auto d5 = p.itemRectDevice(Page::PAGE_MAIN, 5);
        record("itemRectDevice: 1280x960 item 0 (100,195,500,93), item 5 y=660",
               close(d0.x, 100.0f) && close(d0.y, 195.0f) && close(d0.w, 500.0f) &&
                   close(d0.h, 93.0f) && close(d5.y, 660.0f),
               "d0=(" + std::to_string(d0.x) + "," + std::to_string(d0.y) + "," +
                   std::to_string(d0.w) + "," + std::to_string(d0.h) + ") d5y=" +
                   std::to_string(d5.y));
        p.setScrollRows(2.0f);
        const auto ds = p.itemRectDevice(Page::PAGE_MAIN, 0);
        record("itemRectDevice: scroll=2 -> device y=9", close(ds.y, 9.0f),
               "y=" + std::to_string(ds.y));
        p.setScrollRows(0.0f);

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
        p.setScrollRows(2.0f);
        record("culling: scroll=2 hides rows 0/1, keeps 2..7, hides 8",
               !p.isElementVisible(Page::PAGE_SHAPE, 0) &&
                   !p.isElementVisible(Page::PAGE_SHAPE, 1) &&
                   p.isElementVisible(Page::PAGE_SHAPE, 2) &&
                   p.isElementVisible(Page::PAGE_SHAPE, 7) &&
                   !p.isElementVisible(Page::PAGE_SHAPE, 8));
        p.setScrollRows(0.0f);

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

    return getFailCount() == 0;
}
