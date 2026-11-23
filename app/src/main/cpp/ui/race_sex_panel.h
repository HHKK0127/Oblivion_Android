#pragma once

#include "ui_panel.h"

#include <array>
#include <string>
#include <vector>

/**
 * @brief RaceSexMenu (chargen) 21-page wizard panel — skeleton.
 *
 * Canon: docs/chargen_ui_spec.md (RE v634..v651). The original is menu 0x40C
 * RaceSexMenu. Identity is by id, NOT by name (spec §0): the 21 page names are
 * display-only; the +0x40 slot array and the +0x94 id->tile table are the
 * identity. This skeleton has no logic yet — build_page() dispatches to 21
 * per-page builders, all currently empty.
 *
 * The two transition tables mirror spec §6 and are pinned by static_assert so
 * that editing the spec without editing this file (or vice versa) fails the
 * build.
 */
class RaceSexPanel : public UIPanel {
public:
    // 21 pages, in builder order == +0x40 slot order (spec §2). The enum value
    // is the slot index (0..20), not the runtime id.
    enum Page : int {
        PAGE_MAIN = 0, PAGE_RACE, PAGE_HAIR, PAGE_FACE, PAGE_SHAPE, PAGE_TONE,
        PAGE_GENERAL_FACE, PAGE_BROW, PAGE_CHEEKS, PAGE_CHIN, PAGE_EYES,
        PAGE_FOREHEAD, PAGE_JAW, PAGE_MOUTH, PAGE_NOSE, PAGE_GENERAL_SKIN,
        PAGE_BEARD, PAGE_EYES_TEX, PAGE_EYEBROWS, PAGE_MOUTH_LIPS, PAGE_NOSE_TEX,
        PAGE_COUNT
    };

    // Random / Reset are page-level buttons on the Face page; the original has
    // no dedicated pane slot for them, so give them sentinel slot values.
    static constexpr int PAGE_RANDOM = -2;
    static constexpr int PAGE_RESET = -3;

    // Element ids on the pages, named for the FAMILY table. Values are the
    // registry display indices (v634) used only to make the table readable; the
    // real runtime identity is the id in the +0x94 table.
    enum Tile : int {
        TILE_AGE = 12, TILE_FACE = 9, TILE_HAIR = 10, TILE_RACE = 8,
        TILE_GENDER = 14, TILE_RED = 17, TILE_GREEN = 18, TILE_BLUE = 19,
        TILE_LENGTH = 20, TILE_SHAPE = 21, TILE_TONE = 22,
        TILE_GENERAL_FACE = 25, TILE_BROW = 26, TILE_CHEEKS = 27, TILE_CHIN = 28,
        TILE_EYES = 11, TILE_FOREHEAD = 29, TILE_JAW = 30, TILE_MOUTH = 31,
        TILE_NOSE = 32, TILE_GENERAL_SKIN = 33, TILE_BEARD = 35,
        TILE_EYES_TEX = 125, TILE_EYEBROWS = 36, TILE_MOUTH_LIPS = 37,
        TILE_NOSE_TEX = 126,
    };

    // chain layer (spec §6.2): arg1 == 90 (Done/Return), 19 rows, serial chain.
    // { pane, next }  next = destination page, or -1 for the commit-less row
    // (Main, whose Done is the name-validation path). Brow appears twice because
    // the original chains two distinct controls.
    struct ChainRow {
        int pane;
        int next;
    };

    // family layer (spec §6.3): one row per tab button, 28 rows, serial chain.
    // { pane, tile, target, const_unused }
    //   pane/tile = the button's (page, element) pair.
    //   target    = destination page, or -1 when the original has no page switch
    //               (finalize only) — "family membership != transition".
    //   const_unused = the [esp+0x3c]/[esp+0x48] constant the original stores;
    //               DEAD (V-649-1: no reader in HandleClick nor its finalize
    //               callees) — kept as information only, -1 when absent.
    struct FamilyRow {
        int pane;
        int tile;
        int target;
        int const_unused;
    };

    // chain rows == spec §6.2 (19). If the spec row count changes, change both.
    static constexpr std::array<ChainRow, 19> CHAIN = {{
        // pane                     next
        {PAGE_MAIN,                -1},            // Main: commit-less (Done = name validation)
        {PAGE_SHAPE,               PAGE_FACE},     // Shape -> Face
        {PAGE_TONE,                PAGE_FACE},     // Tone  -> Face
        {PAGE_GENERAL_FACE,        PAGE_SHAPE},    // Face-group -> Shape
        {PAGE_BROW,                PAGE_SHAPE},
        {PAGE_CHEEKS,              PAGE_SHAPE},
        {PAGE_CHIN,                PAGE_SHAPE},
        {PAGE_EYES,                PAGE_SHAPE},
        {PAGE_FOREHEAD,            PAGE_SHAPE},
        {PAGE_JAW,                 PAGE_SHAPE},
        {PAGE_BROW,                PAGE_SHAPE},    // Brow again (second control)
        {PAGE_MOUTH,               PAGE_SHAPE},
        {PAGE_NOSE,                PAGE_SHAPE},
        {PAGE_GENERAL_SKIN,        PAGE_TONE},     // Skin-group -> Tone
        {PAGE_BEARD,               PAGE_TONE},
        {PAGE_EYES_TEX,            PAGE_TONE},
        {PAGE_EYEBROWS,            PAGE_TONE},
        {PAGE_MOUTH_LIPS,          PAGE_TONE},
        {PAGE_NOSE_TEX,            PAGE_TONE},
    }};

    // family rows == spec §6.3 (28).
    static constexpr std::array<FamilyRow, 28> FAMILY = {{
        // pane                 tile                    target             const_unused
        {PAGE_MAIN,             TILE_AGE,               -1,                -1},   // Age: no switch (finalize)
        {PAGE_MAIN,             TILE_FACE,              PAGE_FACE,         0xa},
        {PAGE_MAIN,             TILE_HAIR,              PAGE_HAIR,         0x10},
        {PAGE_MAIN,             TILE_RACE,              PAGE_RACE,         0x12},
        {PAGE_RACE,             TILE_GENDER,            -1,                -1},   // TBD
        {PAGE_HAIR,             TILE_RED,               -1,                -1},   // TBD
        {PAGE_HAIR,             TILE_GREEN,             -1,                -1},   // TBD
        {PAGE_HAIR,             TILE_BLUE,              -1,                -1},   // TBD
        {PAGE_HAIR,             TILE_LENGTH,            -1,                -1},   // TBD
        {PAGE_FACE,             TILE_SHAPE,             PAGE_SHAPE,        0xc},
        {PAGE_FACE,             TILE_TONE,              PAGE_TONE,         0xe},
        {PAGE_SHAPE,            TILE_GENERAL_FACE,      PAGE_GENERAL_FACE, 0x14},
        {PAGE_SHAPE,            TILE_BROW,              PAGE_BROW,         0x16},
        {PAGE_SHAPE,            TILE_CHEEKS,            PAGE_CHEEKS,       0x18},
        {PAGE_SHAPE,            TILE_CHIN,              PAGE_CHIN,         0x1a},
        {PAGE_SHAPE,            TILE_EYES,              PAGE_EYES,         0x1c},
        {PAGE_SHAPE,            TILE_FOREHEAD,          PAGE_FOREHEAD,     0x1e},
        {PAGE_SHAPE,            TILE_JAW,               PAGE_JAW,          0x20},
        {PAGE_SHAPE,            TILE_MOUTH,             PAGE_MOUTH,        0x22},
        {PAGE_SHAPE,            TILE_NOSE,              PAGE_NOSE,         0x24},
        {PAGE_TONE,             TILE_GENERAL_SKIN,      PAGE_GENERAL_SKIN, 0x26},
        {PAGE_TONE,             TILE_BEARD,             PAGE_BEARD,        0x28},
        {PAGE_TONE,             TILE_EYES_TEX,          PAGE_EYES_TEX,     0x2a},
        {PAGE_TONE,             TILE_EYEBROWS,          PAGE_EYEBROWS,     0x2c},
        {PAGE_TONE,             TILE_MOUTH_LIPS,        PAGE_MOUTH_LIPS,   0x2e},
        {PAGE_TONE,             TILE_NOSE_TEX,          PAGE_NOSE_TEX,     0x30},
        {PAGE_RANDOM,           TILE_FACE,              -1,                -1},   // Random: no page switch
        {PAGE_RESET,            TILE_FACE,              -1,                -1},   // Reset: no page switch
    }};

    static_assert(CHAIN.size() == 19,
                  "chain rows must match docs/chargen_ui_spec.md 6.2");
    static_assert(FAMILY.size() == 28,
                  "family rows must match docs/chargen_ui_spec.md 6.3");

    // --- Element registry (spec §0: identity is the id, not the name) ---
    // The original stores tiles in menu+0x40 (21 pane slots) and menu+0x94
    // (500 id->tile). This panel mirrors both: pane_ids_ is the +0x40 array,
    // id_to_index_ is the +0x94 array, elements_ owns the tiles. Ids are
    // assigned by the runtime counter (spec §2): start 200, pane id =
    // ++counter except Main (id 0, arg4=0, counter not consumed), item id =
    // ++counter, and a slider consumes two ids (its own and the user0 slot).
    enum class ElemType { Button, Toggle, Text, Slider };

    struct Element {
        int id = -1;          // runtime id (spec §0)
        int user0_id = -1;    // slider only: the second id consumed (user0 slot)
        ElemType type = ElemType::Button;
        std::string label;    // Tile::name (0xBBA); display-only (spec §0)
        int page_slot = -1;   // owning page (0..20)
    };

    // +0x40: page id per slot (Main=0, others = ++counter at build time).
    int page_id(int slot) const {
        return (slot >= 0 && slot < PAGE_COUNT) ? pane_ids_[slot] : -1;
    }
    // All elements created so far, in build order.
    const std::vector<Element>& elements() const { return elements_; }
    // +0x94 lookup: element by runtime id, or nullptr when the slot is empty.
    const Element* find_element(int id) const;

    RaceSexPanel();
    ~RaceSexPanel() override = default;

    // Build the panel: create the 21 pages and wire the transitions.
    bool init();

    // Build one page. 21-branch switch; the default throws so an unimplemented
    // page can never be silently skipped.
    void build_page(int page_index);

private:
    // Runtime id counter (spec §2): start 200.
    int alloc_id() { return ++counter_; }
    // Create one element on a page and register it in the +0x94 table; returns
    // its id. A slider consumes a second id (user0 slot), recorded in user0_id
    // and also registered (both ids resolve to the same element).
    int add_element(int page_slot, ElemType type, const std::string& label);
    void add_button(int slot, const std::string& label) { add_element(slot, ElemType::Button, label); }
    void add_toggle(int slot, const std::string& label) { add_element(slot, ElemType::Toggle, label); }
    void add_text(int slot, const std::string& label) { add_element(slot, ElemType::Text, label); }
    void add_slider(int slot, const std::string& label) { add_element(slot, ElemType::Slider, label); }

    // +0x40 (21 pane ids; -1 = empty) / +0x94 (500 slots; 0 = empty, else
    // element index + 1) / the tiles themselves / the runtime counter.
    std::array<int, PAGE_COUNT> pane_ids_{};
    std::array<int, 500> id_to_index_{};
    std::vector<Element> elements_;
    int counter_ = 200;

    // 21 per-page builders. All empty in the skeleton; Main's six elements carry
    // their id/label/range in comments (see race_sex_panel.cpp::build_main).
    void build_main();
    void build_race();
    void build_hair();
    void build_face();
    void build_shape();
    void build_tone();
    void build_general_face();
    void build_brow();
    void build_cheeks();
    void build_chin();
    void build_eyes();
    void build_forehead();
    void build_jaw();
    void build_mouth();
    void build_nose();
    void build_general_skin();
    void build_beard();
    void build_eyes_tex();
    void build_eyebrows();
    void build_mouth_lips();
    void build_nose_tex();
};
