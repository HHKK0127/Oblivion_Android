#pragma once

#include "ui_panel.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
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

        // Slider / toggle state. Slider defaults are the template values
        // (spec §5): range 0..1, step 0.04, jump 0.1. Toggles use `checked`.
        float value = 0.0f;
        float min = 0.0f;
        float max = 1.0f;
        float step = 0.04f;
        float jump = 0.1f;
        bool checked = false;

        // Random writes the user3 trait (0xFB1) three times per affected tile
        // (sentinel -> value -> 0.0); the port records the final value and the
        // write count so the host suite can pin "only (Main,Age)/(Main,Complexion)".
        float user3 = 0.0f;
        int user3_writes = 0;

        // Real widget, created by instantiate() (device-only; see
        // race_sex_panel_instantiator.cpp). Null until then, so the host suite
        // exercises the descriptor without pulling in the GL / text stack.
        std::shared_ptr<UIComponent> widget;
    };

    // +0x40: page id per slot (Main=0, others = ++counter at build time).
    int page_id(int slot) const {
        return (slot >= 0 && slot < PAGE_COUNT) ? pane_ids_[slot] : -1;
    }
    // All elements created so far, in build order.
    const std::vector<Element>& elements() const { return elements_; }
    // +0x94 lookup: element by runtime id, or nullptr when the slot is empty.
    const Element* find_element(int id) const;
    // Mutable variant, for the toggle flip in handleClick (V-631-1b).
    Element* find_element_mutable(int id);

    // Turn the descriptors into live widgets (device-only: needs a TextRenderer
    // and a GL context). Fills Element::widget and parents each widget under the
    // panel. Defined in race_sex_panel_instantiator.cpp, which the host suite
    // does not link. Slider/toggle state comes from the element fields above.
    void instantiate(TextRenderer* text_renderer);

    // --- Interaction (spec §6) ---
    // current_page_ mirrors the +0xFAE "current page" value; it is a page slot
    // (Main = 0). handleClick implements the two transition layers: arg1 == 90
    // is Done/Return (chain, §6.2); any other arg1 is a tab button id (family,
    // §6.3). Returns true when the click was consumed by either table.
    void setCurrentPage(int slot) { current_page_ = slot; }
    int currentPage() const { return current_page_; }
    void setOnDone(std::function<void()> cb) { on_done_ = std::move(cb); }
    bool handleClick(int arg1);

    // --- Random / Reset (spec §10.7) ---
    // The RNG is the MSVC CRT rand() LCG ported verbatim: the caller draws
    // [0,32767] and normalises to value = rngNext()/32767.0. setSeed() is for
    // tests; init() seeds from the clock (the original's arg==0 path).
    void setSeed(uint32_t seed) { rng_seed_ = seed; }
    uint32_t seed() const { return rng_seed_; }
    int rngNext();  // seed = seed*214013 + 2531011; return (seed>>16) & 0x7FFF
    // All sliders, builder order: value = rngNext()/32767.0f. Also writes the
    // user3 trait (sentinel -> value -> 0.0) on (Main, Age)/(Main, Complexion)
    // only (the original's 0xFB1 3-write). Re-lays out via the layout hook.
    void doRandom();
    // All sliders, builder order: value = 0.0. Re-lays out via the layout hook.
    void doReset();
    int sliderCount() const;
    // First click shows the confirm dialog and arms; an armed click runs the
    // body directly (the original's [0xB3B4C8]/[0xB3B4C9] flags). A No answer
    // clears the flag (the original's cb); a page change never does (the flags
    // are global). Both keep the page on Face.
    bool randomArmed() const { return random_armed_; }
    bool resetArmed() const { return reset_armed_; }

    // --- Race selection (spec §10.4) ---
    // One selectable race. The port fills this from
    // ESMManager::getAllRaces() filtered by (flags & 1) (playable); only the
    // formID/name/description are needed here.
    struct RaceEntry {
        uint32_t form_id = 0;
        std::string name;
        std::string description;
    };

    void setAvailableRaces(std::vector<RaceEntry> races);
    int raceCount() const { return static_cast<int>(races_.size()); }
    int selectedRaceIndex() const { return selected_race_index_; }
    void setSelectedRaceIndex(int index);  // clamped to [0, count-1]
    // Left/right of the race toggle: +1/-1, clamped at the ends (V-654-3).
    void cycleRace(int delta);
    // spec §10.4: the race toggle cycles on the tapped half of its row.
    // pane_local_x is the panel's own (canvas) x; row_width is the row width.
    // Left half = -1, right half = +1 (clamped).
    void cycleRaceFromTap(float pane_local_x, float row_width);
    uint32_t selectedRaceFormID() const;
    const std::string& selectedRaceName() const;
    const std::string& selectedRaceDescription() const;
    // The race the player currently has (the original's 0x5E3320()[+0xC]).
    uint32_t currentRaceFormID() const { return current_race_form_id_; }
    void setCurrentRaceFormID(uint32_t id) { current_race_form_id_ = id; }
    // True when the selection differs from the current race; commit() only
    // applies the race then (the original's 0x51C090 no-op guard).
    bool raceChanged() const;

    // --- Toggles (V-631-1b) ---
    // The Gender toggle (id 210) is the only toggle whose value the commit
    // consumes: Player::gender is 0=Male / 1=Female (player.h:69, the same
    // convention as face_race_presets.h Gender::MALE/FEMALE). OFF = Male, ON =
    // Female — the original's ON/OFF meaning is not measured (#199), so this
    // mapping is the port's choice and is recorded as such.
    bool genderFemale() const { return gender_female_; }
    void setGenderFemale(bool female) { gender_female_ = female; }
    // A toggle's checked state by runtime id (false for non-toggles). Lets the
    // host suite pin the flip without the device-only widget.
    bool toggleChecked(int id) const;

    // --- Name + confirm + commit (spec §10.1/§10.3) ---
    // The original rejects a name that is empty, contains a space, or is
    // unchanged from the current one (0x5CCA77). `player_name_` is the entered /
    // shown name; `current_name_` is the player's current name (the baseline).
    enum class NameCheck { Ok, Empty, HasSpace, Unchanged };
    NameCheck validateName(const std::string& name) const;
    void setCurrentName(const std::string& name) { current_name_ = name; }
    const std::string& currentName() const { return current_name_; }

    // Done(Main) flow: validate the name; notify-only on failure, otherwise ask
    // for confirmation. Returns what it did (host-testable).
    enum class DoneAction { None, Notify, Confirm };
    DoneAction handleDone();

    // Apply the confirmed state: race (only when changed) + name + flag. The
    // actual application is delegated to on_commit_ (the renderer owns the
    // Player / NpcManager).
    void commit();
    bool isConfirmed() const { return confirmed_; }

    // "%s %s?" article (V-654-2: vowel heuristic, the original's rule is not
    // measured).
    static const char* articleFor(const std::string& name);

    // Owner hooks. The renderer wires these to UIMessageBox (notify / Yes-No)
    // and to the player application.
    void setOnNotify(std::function<void(const std::string& body)> cb) {
        on_notify_ = std::move(cb);
    }
    void setOnConfirm(
        std::function<void(const std::string& body, std::function<void(bool)> done)> cb) {
        on_confirm_ = std::move(cb);
    }
    void setOnCommit(std::function<void()> cb) { on_commit_ = std::move(cb); }

    // --- Geometry (spec §9) ---
    // Units are the menu's own pixels on the 1280x960 UI canvas; the panel
    // origin is race_window's top-left, i.e. pane (0,0) before scrolling.
    static constexpr int ROW_HEIGHT = 93;       // §9.1 item height / row pitch
    static constexpr int WINDOW_WIDTH = 500;    // §9.2 race_window width
    static constexpr int WINDOW_HEIGHT = 558;   // §9.2 race_window height
    static constexpr int SCROLL_VISIBLE = 6;    // §9.2 >6 items -> scroll bar
    static constexpr int SCROLL_GUTTER = 40;    // §9.2 constant right gutter
    // §9.3 race_button_done (id 90) is a sibling of race_window inside
    // race_background (560x740 at x=70): bg (353,676) -> window-local (323,591).
    // button_long.xml gives 177x44; 323+177 = 500 (right edges align).
    static constexpr int DONE_ID = 90;
    static constexpr int DONE_WIDTH = 177;
    static constexpr int DONE_HEIGHT = 44;
    static constexpr int DONE_X = 323;
    static constexpr int DONE_Y = 591;
    // Race page elements (spec §10.4): the race toggle and its description text.
    static constexpr int RACE_TOGGLE_ID = 211;
    static constexpr int RACE_DESC_ID = 212;
    // Main page "Eyes" toggle (spec §10.6) and the Race page "Gender" toggle.
    // The Gender toggle's value is carried into the commit (V-631-1b); the other
    // toggles' effects are not measured yet (#199).
    static constexpr int EYES_TOGGLE_ID = 204;
    static constexpr int GENDER_TOGGLE_ID = 210;
    static constexpr int STYLE_TOGGLE_ID = 214;
    static constexpr int HAIR_COLOR_TOGGLE_ID = 215;
    // Face page 4 buttons (spec §10.7): builder order Shape/Tone/Random/Reset.
    static constexpr int RANDOM_BUTTON_ID = 227;
    static constexpr int RESET_BUTTON_ID = 228;
    // race_window's top-left on the canvas: race_background x=70 plus
    // race_window x=30; y = (960-740)/2 (background centred) + race_window y=85.
    static constexpr float CANVAS_WIDTH = 1280.0f;
    static constexpr float CANVAS_HEIGHT = 960.0f;
    static constexpr float PANEL_CANVAS_X = 100.0f;
    static constexpr float PANEL_CANVAS_Y = 195.0f;

    // Item children of a page == the template's me().childcount.
    int itemCount(int slot) const;
    // §9.2: race_window width minus the scroll-bar gutter when >6 items.
    int paneWidth(int slot) const;
    // listindex -> runtime id (build order), or -1 when out of range.
    int elementIdAt(int slot, int row) const;

    // §9.4: race_scroll_bar.user7 (rows scrolled). Clamped to
    // [0, max(0, itemCount(current_page_) - visibleRows)]; a change re-lays out
    // through the on-layout-changed hook (set by instantiate()).
    int scrollRows() const { return scroll_rows_; }
    void setScrollRows(int rows);

    // --- Scroll input (spec §9.7) ---
    // A touch inside the scroll-bar rect captures the drag: Δy / (93*scale) rows
    // are accumulated from the row the drag started on. The bar only exists when
    // the page overflows the window.
    bool isScrollBarHit(float screen_x, float screen_y) const;
    bool beginScrollDrag(float screen_x, float screen_y);
    void updateScrollDrag(float screen_y);
    void endScrollDrag();
    bool isScrollDragging() const { return scroll_dragging_; }

    // The device-only layout hook: instantiate() points this at applyLayout().
    // The host suite leaves it empty, so the pure logic links without the GL
    // stack.
    void setOnLayoutChanged(std::function<void()> cb) { on_layout_changed_ = std::move(cb); }

    // --- Name label (spec §10.1) ---
    // The original sets race_name (id 10) to the player's current name on every
    // build; the port shows the same string so the field is never blank.
    void setPlayerName(const std::string& name) { player_name_ = name; }
    const std::string& playerName() const { return player_name_; }

    // Pane-local (race_window) hit test -> runtime id; DONE_ID for the Done
    // button; -1 for the scroll gutter, empty rows, or a hidden panel.
    int hitTest(int x, int y) const;

    // Screen pixel -> pane-local -> hitTest(): the conversion the renderer
    // uses. Canvas -> device is a uniform `layoutScale()` plus a letterbox
    // offset; pure logic, so the host suite pins it.
    int hitTestScreen(float screen_x, float screen_y) const;
    float layoutScale() const;
    float panelOriginX() const;
    float panelOriginY() const;

    // --- Layout / draw (spec §9.5) ---
    // Device-space rectangle. The draw side and hitTestScreen() must share ONE
    // transform (canvas 1280x960 -> device via layoutScale() + race_window
    // origin), or a tap and its pixels drift apart.
    struct RectF {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    // Canvas rect of item `row` on page `slot`: (0, row*93 - scroll*93, pane_w, 93).
    RectF itemRectCanvas(int slot, int row) const;
    // The same rect in device pixels.
    RectF itemRectDevice(int slot, int row) const;
    RectF doneRectDevice() const;       // §9.3 Done (id 90), pane-external
    RectF scrollBarRectDevice() const;  // §9.3 race_scroll_bar, pane-external
    // §10.1 race_name (id 10): the name label. Canvas x = race_background 70 +
    // 560/2 (centre) = 350, y = 110 + 20 = 130 -> pane-local (250, -65).
    RectF nameRectDevice() const;

    // §9.5 row culling: UIComponent has no clip, so rows outside the window are
    // hidden instead. The default is race_scroll_bar.user8 = 6, which is also
    // race_window.height / ROW_HEIGHT (558 / 93 = 6); setVisibleRows() lets an
    // XML loader push the authored value.
    int visibleRows() const { return visible_rows_; }
    void setVisibleRows(int rows) { visible_rows_ = rows > 0 ? rows : 1; }
    bool isRowVisible(int row) const;
    // True only for the current page's in-window rows.
    bool isElementVisible(int slot, int row) const;
    // 0-based row (listindex) of an element within its page, or -1.
    int rowOfElement(int element_id) const;

    // Device-only: place the widgets built by instantiate() at the rects above
    // and hide the off-window / off-page ones; also disables the UIPanel title
    // bar. Defined in race_sex_panel_instantiator.cpp (host never calls it).
    void applyLayout();

    // TILE_* registry index -> element label (used by the family walk).
    static const char* tileLabel(int tile);

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

    // Current page (slot) and the Done callback. current_page_ starts at Main
    // (spec §2) and is reset by init(); on_done_ is invoked by the chain's Main
    // row (arg1 == 90), i.e. the chargen exit.
    int current_page_ = PAGE_MAIN;
    std::function<void()> on_done_;
    // §9.4 scroll offset in rows (race_scroll_bar.user7). 0 = top. int: the
    // original stores whole rows.
    int scroll_rows_ = 0;
    // Device-only layout hook (see setOnLayoutChanged).
    std::function<void()> on_layout_changed_;

    // Scroll-drag state (spec §9.7).
    bool scroll_dragging_ = false;
    float scroll_drag_last_y_ = 0.0f;
    float scroll_drag_accum_px_ = 0.0f;
    int scroll_drag_start_rows_ = 0;

    // Device-only widgets for the pane-external controls (spec §9.3). Created by
    // instantiate(); null until then (and on the host, which never instantiates).
    std::shared_ptr<UIComponent> done_widget_;
    std::shared_ptr<UIComponent> scroll_bar_widget_;
    std::shared_ptr<UIComponent> name_widget_;   // §10.1 race_name (id 10)

    // §9.5 visible rows: race_scroll_bar.user8 (default 6).
    int visible_rows_ = 6;
    // §10.1 the name shown on race_name (player's current name).
    std::string player_name_;
    // The player's current name, the baseline for the "unchanged" check.
    std::string current_name_;
    // V-631-1b: the Gender toggle's value (false = Male, true = Female). Seeded
    // from Player::gender on open and read back by the renderer's commit.
    bool gender_female_ = false;

    // Race selection + confirm state (spec §10.2/§10.4).
    std::vector<RaceEntry> races_;
    int selected_race_index_ = 0;
    uint32_t current_race_form_id_ = 0;
    bool confirmed_ = false;
    std::function<void(const std::string&)> on_notify_;
    std::function<void(const std::string&, std::function<void(bool)>)> on_confirm_;
    std::function<void()> on_commit_;

    // Random / Reset state (spec §10.7): the MSVC LCG seed and the per-button
    // "dialog already shown" flags (the original's [0xB3B4C8]/[0xB3B4C9]).
    uint32_t rng_seed_ = 0;
    bool random_armed_ = false;
    bool reset_armed_ = false;

    // First click shows the confirm dialog and arms; an armed click runs the
    // body. Both keep the page on Face.
    void handleRandom();
    void handleReset();

    // Find the element on a page by its label (family walk helper).
    const Element* find_on_page(int slot, const char* label) const;

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
