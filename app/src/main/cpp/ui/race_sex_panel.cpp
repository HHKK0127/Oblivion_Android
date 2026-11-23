#include "race_sex_panel.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// Skeleton for the RaceSexMenu (chargen) 21-page wizard.
// Canon: docs/chargen_ui_spec.md. Every build_* body is intentionally empty;
// this commit adds structure only (no logic, no assets, no wiring).

RaceSexPanel::RaceSexPanel() : UIPanel("RaceSexPanel") {}

const RaceSexPanel::Element* RaceSexPanel::find_element(int id) const {
    if (id < 0 || id >= static_cast<int>(id_to_index_.size())) return nullptr;
    const int slot = id_to_index_[id];
    return slot == 0 ? nullptr : &elements_[slot - 1];
}

int RaceSexPanel::add_element(int page_slot, ElemType type, const std::string& label) {
    Element e;
    e.id = alloc_id();
    e.type = type;
    e.label = label;
    e.page_slot = page_slot;
    if (type == ElemType::Slider) {
        e.user0_id = alloc_id();  // slider consumes a second id (user0 slot)
    }
    elements_.push_back(e);
    const int idx = static_cast<int>(elements_.size());  // 1-based (0 = empty)
    id_to_index_[e.id] = idx;
    if (e.user0_id >= 0) id_to_index_[e.user0_id] = idx;
    return e.id;
}

bool RaceSexPanel::init() {
    // spec §3: clear the 21 pane slots and the id table on open. The original's
    // Init only clears +0x94; clearing +0x40 too is the safe-side deviation the
    // spec requires.
    pane_ids_.fill(-1);
    id_to_index_.fill(0);
    elements_.clear();
    counter_ = 200;
    current_page_ = PAGE_MAIN;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        build_page(i);
    }
    return true;
}

void RaceSexPanel::build_page(int page_index) {
    switch (page_index) {
        case PAGE_MAIN:         build_main();         break;
        case PAGE_RACE:         build_race();         break;
        case PAGE_HAIR:         build_hair();         break;
        case PAGE_FACE:         build_face();         break;
        case PAGE_SHAPE:        build_shape();        break;
        case PAGE_TONE:         build_tone();         break;
        case PAGE_GENERAL_FACE: build_general_face(); break;
        case PAGE_BROW:         build_brow();         break;
        case PAGE_CHEEKS:       build_cheeks();       break;
        case PAGE_CHIN:         build_chin();         break;
        case PAGE_EYES:         build_eyes();         break;
        case PAGE_FOREHEAD:     build_forehead();     break;
        case PAGE_JAW:          build_jaw();          break;
        case PAGE_MOUTH:        build_mouth();        break;
        case PAGE_NOSE:         build_nose();         break;
        case PAGE_GENERAL_SKIN: build_general_skin(); break;
        case PAGE_BEARD:        build_beard();        break;
        case PAGE_EYES_TEX:     build_eyes_tex();     break;
        case PAGE_EYEBROWS:     build_eyebrows();     break;
        case PAGE_MOUTH_LIPS:   build_mouth_lips();   break;
        case PAGE_NOSE_TEX:     build_nose_tex();     break;
        default:
            // An unknown page must never be silently skipped.
            throw std::out_of_range("RaceSexPanel::build_page: unknown page index");
    }
}

// ---------------------------------------------------------------------------
// Page builders.
//
// Items are listed in build order with their runtime id (spec §0: id is the
// identity; names are display-only). Sliders consume two ids (id and user0),
// which is why some ids are skipped. Slider range is the template default
// (spec §5): range 0..1, step 0.04, jump 0.1. The pane id is the ++counter
// value at the start of the page (Main is the exception, id 0).
// ---------------------------------------------------------------------------

void RaceSexPanel::build_main() {
    // Main (spec §2 #1, slot +0x40, page id 0). Items in build order:
    //   id=201 button  "Race"                  -> FAMILY const 0x12, target Race
    //   id=202 button  "Face"                  -> FAMILY const 0xa,  target Face
    //   id=203 button  "Hair"                  -> FAMILY const 0x10, target Hair
    //   id=204 toggle  "Eyes"                  -> NOT in FAMILY: no menu handler (v662);
    //                                             the toggle's own value flip is widget-side
    //   id=205 slider  "Age"        range 0..1 step 0.04 jump 0.1
    //                                          -> FAMILY const TBD, no page switch
    //   id=207 slider  "Complexion" range 0..1 step 0.04 jump 0.1
    //                                          -> NOT in FAMILY: no menu handler (v662);
    //                                             a slider changes on drag, not on click
    // ids 206/208 are the user0 slots of the two sliders; the next page (Race)
    // starts at id 209.
    pane_ids_[PAGE_MAIN] = 0;  // spec §2: Main id 0 (arg4=0) -> counter not consumed
    add_button(PAGE_MAIN, "Race");        // id 201
    add_button(PAGE_MAIN, "Face");        // id 202
    add_button(PAGE_MAIN, "Hair");        // id 203
    add_toggle(PAGE_MAIN, "Eyes");        // id 204
    add_slider(PAGE_MAIN, "Age");         // id 205 (+206 user0)
    add_slider(PAGE_MAIN, "Complexion");  // id 207 (+208 user0)
}

void RaceSexPanel::build_race() {
    // Race (slot +0x44, page id 209).
    pane_ids_[PAGE_RACE] = alloc_id();          // 209
    add_toggle(PAGE_RACE, "Gender");            // 210 (FAMILY target -1)
    add_toggle(PAGE_RACE, "Race");              // 211
    add_text(PAGE_RACE, "Race Description");    // 212
}

void RaceSexPanel::build_hair() {
    // Hair (slot +0x48, page id 213).
    pane_ids_[PAGE_HAIR] = alloc_id();          // 213
    add_toggle(PAGE_HAIR, "Style");             // 214
    add_toggle(PAGE_HAIR, "Hair Color");        // 215
    add_slider(PAGE_HAIR, "Red");               // 216 (+217)
    add_slider(PAGE_HAIR, "Green");             // 218 (+219)
    add_slider(PAGE_HAIR, "Blue");              // 220 (+221)
    add_slider(PAGE_HAIR, "Length");            // 222 (+223)
}

void RaceSexPanel::build_face() {
    // Face (slot +0x4c, page id 224).
    pane_ids_[PAGE_FACE] = alloc_id();          // 224
    add_button(PAGE_FACE, "Shape");             // 225 (FAMILY target Shape)
    add_button(PAGE_FACE, "Tone");              // 226 (FAMILY target Tone)
    add_button(PAGE_FACE, "Random");            // 227 (FAMILY target -1)
    add_button(PAGE_FACE, "Reset");             // 228 (FAMILY target -1)
}

void RaceSexPanel::build_shape() {
    // Shape (slot +0x50, page id 229).
    pane_ids_[PAGE_SHAPE] = alloc_id();         // 229
    add_button(PAGE_SHAPE, "General(Face)");    // 230
    add_button(PAGE_SHAPE, "Brow");             // 231
    add_button(PAGE_SHAPE, "Cheeks");           // 232
    add_button(PAGE_SHAPE, "Chin");             // 233
    add_button(PAGE_SHAPE, "Eyes");             // 234
    add_button(PAGE_SHAPE, "Forehead");         // 235
    add_button(PAGE_SHAPE, "Jaw");              // 236
    add_button(PAGE_SHAPE, "Mouth");            // 237
    add_button(PAGE_SHAPE, "Nose");             // 238
}

void RaceSexPanel::build_tone() {
    // Tone (slot +0x54, page id 239).
    pane_ids_[PAGE_TONE] = alloc_id();          // 239
    add_button(PAGE_TONE, "General(Skin)");     // 240
    add_button(PAGE_TONE, "Beard");             // 241
    add_button(PAGE_TONE, "Eye shades");        // 242
    add_button(PAGE_TONE, "Eyebrows");          // 243
    add_button(PAGE_TONE, "Mouth(Lips)");       // 244
    add_button(PAGE_TONE, "Nose shades");       // 245
}

void RaceSexPanel::build_general_face() {
    // General(Face) (slot +0x58, page id 246). 5 sliders.
    pane_ids_[PAGE_GENERAL_FACE] = alloc_id();  // 246
    add_slider(PAGE_GENERAL_FACE, "Face brow/nose/chin ratio");           // 247 (+248)
    add_slider(PAGE_GENERAL_FACE, "Face forehead/sellion/nose ratio");    // 249 (+250)
    add_slider(PAGE_GENERAL_FACE, "Face heavy/light");                    // 251 (+252)
    add_slider(PAGE_GENERAL_FACE, "Face round/gaunt");                    // 253 (+254)
    add_slider(PAGE_GENERAL_FACE, "Face thin/wide");                      // 255 (+256)
}

void RaceSexPanel::build_brow() {
    // Brow (slot +0x5c, page id 257). 3 sliders.
    pane_ids_[PAGE_BROW] = alloc_id();          // 257
    add_slider(PAGE_BROW, "Brow Ridge high/low");          // 258 (+259)
    add_slider(PAGE_BROW, "Brow Ridge Inner up/down");     // 260 (+261)
    add_slider(PAGE_BROW, "Brow Ridge Outer up/down");     // 262 (+263)
}

void RaceSexPanel::build_cheeks() {
    // Cheeks (slot +0x60, page id 264). 5 sliders.
    pane_ids_[PAGE_CHEEKS] = alloc_id();        // 264
    add_slider(PAGE_CHEEKS, "Cheekbones low/high");                 // 265 (+266)
    add_slider(PAGE_CHEEKS, "Cheekbones shallow/pronounced");       // 267 (+268)
    add_slider(PAGE_CHEEKS, "Cheekbones thin/wide");                // 269 (+270)
    add_slider(PAGE_CHEEKS, "Cheeks concave/convex");               // 271 (+272)
    add_slider(PAGE_CHEEKS, "Cheeks round/gaunt");                  // 273 (+274)
}

void RaceSexPanel::build_chin() {
    // Chin (slot +0x64, page id 275). 7 sliders.
    pane_ids_[PAGE_CHIN] = alloc_id();          // 275
    add_slider(PAGE_CHIN, "Chin forward/backward");       // 276 (+277)
    add_slider(PAGE_CHIN, "Chin pronounced/recessed");    // 278 (+279)
    add_slider(PAGE_CHIN, "Chin retracted/jutting");      // 280 (+281)
    add_slider(PAGE_CHIN, "Chin shallow/deep");           // 282 (+283)
    add_slider(PAGE_CHIN, "Chin small/large");            // 284 (+285)
    add_slider(PAGE_CHIN, "Chin tall/short");             // 286 (+287)
    add_slider(PAGE_CHIN, "Chin wide/thin");              // 288 (+289)
}

void RaceSexPanel::build_eyes() {
    // Eyes (slot +0x68, page id 290). 4 sliders.
    pane_ids_[PAGE_EYES] = alloc_id();          // 290
    add_slider(PAGE_EYES, "Eyes down/up");                  // 291 (+292)
    add_slider(PAGE_EYES, "Eyes small/large");              // 293 (+294)
    add_slider(PAGE_EYES, "Eyes tilt inward/outward");      // 295 (+296)
    add_slider(PAGE_EYES, "Eyes together/apart");           // 297 (+298)
}

void RaceSexPanel::build_forehead() {
    // Forehead (slot +0x6c, page id 299). 3 sliders.
    pane_ids_[PAGE_FOREHEAD] = alloc_id();      // 299
    add_slider(PAGE_FOREHEAD, "Forehead small/large");           // 300 (+301)
    add_slider(PAGE_FOREHEAD, "Forehead tall/short");            // 302 (+303)
    add_slider(PAGE_FOREHEAD, "Forehead tilt forward/back");     // 304 (+305)
}

void RaceSexPanel::build_jaw() {
    // Jaw (slot +0x70, page id 306). 4 sliders.
    pane_ids_[PAGE_JAW] = alloc_id();           // 306
    add_slider(PAGE_JAW, "Jaw retracted/jutting");      // 307 (+308)
    add_slider(PAGE_JAW, "Jaw wide/thin");              // 309 (+310)
    add_slider(PAGE_JAW, "Jaw-Neck slope high/low");    // 311 (+312)
    add_slider(PAGE_JAW, "Jawline concave/convex");     // 313 (+314)
}

void RaceSexPanel::build_mouth() {
    // Mouth (slot +0x74, page id 315). 10 sliders.
    pane_ids_[PAGE_MOUTH] = alloc_id();         // 315
    add_slider(PAGE_MOUTH, "Mouth drawn/pursed");                  // 316 (+317)
    add_slider(PAGE_MOUTH, "Mouth happy/sad");                     // 318 (+319)
    add_slider(PAGE_MOUTH, "Mouth high/low");                      // 320 (+321)
    add_slider(PAGE_MOUTH, "Mouth lips deflated/inflated");        // 322 (+323)
    add_slider(PAGE_MOUTH, "Mouth lips large/small");              // 324 (+325)
    add_slider(PAGE_MOUTH, "Mouth lips puckered/retracted");       // 326 (+327)
    add_slider(PAGE_MOUTH, "Mouth protruding/retracted");          // 328 (+329)
    add_slider(PAGE_MOUTH, "Mouth tilt up/down");                  // 330 (+331)
    add_slider(PAGE_MOUTH, "Mouth underbite/overbite");            // 332 (+333)
    add_slider(PAGE_MOUTH, "Mouth-Chin distance short/long");      // 334 (+335)
}

void RaceSexPanel::build_nose() {
    // Nose (slot +0x78, page id 336). 14 sliders.
    pane_ids_[PAGE_NOSE] = alloc_id();          // 336
    add_slider(PAGE_NOSE, "Nose bridge shallow/deep");           // 337 (+338)
    add_slider(PAGE_NOSE, "Nose bridge short/long");             // 339 (+340)
    add_slider(PAGE_NOSE, "Nose down/up");                       // 341 (+342)
    add_slider(PAGE_NOSE, "Nose flat/pointed");                  // 343 (+344)
    add_slider(PAGE_NOSE, "Nose nostril tilt down/up");          // 345 (+346)
    add_slider(PAGE_NOSE, "Nose nostrils small/large");          // 347 (+348)
    add_slider(PAGE_NOSE, "Nose nostrils wide/thin");            // 349 (+350)
    add_slider(PAGE_NOSE, "Nose region concave/convex");         // 351 (+352)
    add_slider(PAGE_NOSE, "Nose sellion down/up");               // 353 (+354)
    add_slider(PAGE_NOSE, "Nose sellion shallow/deep");          // 355 (+356)
    add_slider(PAGE_NOSE, "Nose sellion lower shallow/deep");    // 357 (+358)
    add_slider(PAGE_NOSE, "Nose sellion thin/wide");             // 359 (+360)
    add_slider(PAGE_NOSE, "Nose short/long");                    // 361 (+362)
    add_slider(PAGE_NOSE, "Nose tilt down/up");                  // 363 (+364)
}

void RaceSexPanel::build_general_skin() {
    // General(Skin) (slot +0x7c, page id 365). 5 sliders.
    pane_ids_[PAGE_GENERAL_SKIN] = alloc_id();  // 365
    add_slider(PAGE_GENERAL_SKIN, "Skin flushed/pale");          // 366 (+367)
    add_slider(PAGE_GENERAL_SKIN, "Skin shade dark/light");      // 368 (+369)
    add_slider(PAGE_GENERAL_SKIN, "Skin tint orange/blue");      // 370 (+371)
    add_slider(PAGE_GENERAL_SKIN, "Skin tint purple/yellow");    // 372 (+373)
    add_slider(PAGE_GENERAL_SKIN, "Cheek blush light/red");      // 374 (+375)
}

void RaceSexPanel::build_beard() {
    // Beard (slot +0x80, page id 376). 6 sliders.
    pane_ids_[PAGE_BEARD] = alloc_id();         // 376
    add_slider(PAGE_BEARD, "Beard flushed/pale");             // 377 (+378)
    add_slider(PAGE_BEARD, "Beard light/dark");               // 379 (+380)
    add_slider(PAGE_BEARD, "Beard cheeks light/dark");        // 381 (+382)
    add_slider(PAGE_BEARD, "Beard circle light/dark");        // 383 (+384)
    add_slider(PAGE_BEARD, "Beard goatee light/dark");        // 385 (+386)
    add_slider(PAGE_BEARD, "Beard moustache light/dark");     // 387 (+388)
}

void RaceSexPanel::build_eyes_tex() {
    // Eye shades (slot +0x84, page id 389). 6 sliders.
    pane_ids_[PAGE_EYES_TEX] = alloc_id();      // 389
    add_slider(PAGE_EYES_TEX, "Eye sockets bruised/bright");     // 390 (+391)
    add_slider(PAGE_EYES_TEX, "Eye sockets dark/light");         // 392 (+393)
    add_slider(PAGE_EYES_TEX, "Eyelids light/dark");             // 394 (+395)
    add_slider(PAGE_EYES_TEX, "Eyelids pale/red");               // 396 (+397)
    add_slider(PAGE_EYES_TEX, "Eyeliner light/dark");            // 398 (+399)
    add_slider(PAGE_EYES_TEX, "Eyeshadow light/dark");           // 400 (+401)
}

void RaceSexPanel::build_eyebrows() {
    // Eyebrows (slot +0x88, page id 402). 7 sliders.
    pane_ids_[PAGE_EYEBROWS] = alloc_id();      // 402
    add_slider(PAGE_EYEBROWS, "Eyebrows dark/light");            // 403 (+404)
    add_slider(PAGE_EYEBROWS, "Eyebrows low/high");              // 405 (+406)
    add_slider(PAGE_EYEBROWS, "Eyebrows thick/thin");            // 407 (+408)
    add_slider(PAGE_EYEBROWS, "Eyebrows very thin/thick");       // 409 (+410)
    add_slider(PAGE_EYEBROWS, "Eyebrows lower light/dark");      // 411 (+412)
    add_slider(PAGE_EYEBROWS, "Eyebrows outer light/dark");      // 413 (+414)
    add_slider(PAGE_EYEBROWS, "Eyebrows upper dark/light");      // 415 (+416)
}

void RaceSexPanel::build_mouth_lips() {
    // Mouth(Lips) (slot +0x8c, page id 417). 3 sliders.
    pane_ids_[PAGE_MOUTH_LIPS] = alloc_id();    // 417
    add_slider(PAGE_MOUTH_LIPS, "Lips flushed/pale");                // 418 (+419)
    add_slider(PAGE_MOUTH_LIPS, "Lipstick dark red/light blue");     // 420 (+421)
    add_slider(PAGE_MOUTH_LIPS, "Lipstick dark blue/light red");     // 422 (+423)
}

void RaceSexPanel::build_nose_tex() {
    // Nose shades (slot +0x90, page id 424). 2 sliders.
    pane_ids_[PAGE_NOSE_TEX] = alloc_id();      // 424
    add_slider(PAGE_NOSE_TEX, "Naso labial lines light/dark");   // 425 (+426)
    add_slider(PAGE_NOSE_TEX, "Nares small/large");              // 427 (+428)
}

// ---------------------------------------------------------------------------
// Interaction (spec §6). Pure logic, host-testable.
// ---------------------------------------------------------------------------

const RaceSexPanel::Element* RaceSexPanel::find_on_page(int slot, const char* label) const {
    for (const auto& e : elements_) {
        if (e.page_slot == slot && e.label == label) return &e;
    }
    return nullptr;
}

const char* RaceSexPanel::tileLabel(int tile) {
    switch (tile) {
        case TILE_AGE: return "Age";
        case TILE_FACE: return "Face";
        case TILE_HAIR: return "Hair";
        case TILE_RACE: return "Race";
        case TILE_GENDER: return "Gender";
        case TILE_RED: return "Red";
        case TILE_GREEN: return "Green";
        case TILE_BLUE: return "Blue";
        case TILE_LENGTH: return "Length";
        case TILE_SHAPE: return "Shape";
        case TILE_TONE: return "Tone";
        case TILE_GENERAL_FACE: return "General(Face)";
        case TILE_BROW: return "Brow";
        case TILE_CHEEKS: return "Cheeks";
        case TILE_CHIN: return "Chin";
        case TILE_EYES: return "Eyes";
        case TILE_FOREHEAD: return "Forehead";
        case TILE_JAW: return "Jaw";
        case TILE_MOUTH: return "Mouth";
        case TILE_NOSE: return "Nose";
        case TILE_GENERAL_SKIN: return "General(Skin)";
        case TILE_BEARD: return "Beard";
        case TILE_EYES_TEX: return "Eye shades";
        case TILE_EYEBROWS: return "Eyebrows";
        case TILE_MOUTH_LIPS: return "Mouth(Lips)";
        case TILE_NOSE_TEX: return "Nose shades";
    }
    return "";
}

bool RaceSexPanel::handleClick(int arg1) {
    // The race toggle (id 211) is not a page transition: its value change is
    // driven by the tapped half (cycleRaceFromTap), so handleClick consumes it
    // and does nothing (spec §10.4). The renderer routes id 211 to
    // cycleRaceFromTap before calling handleClick.
    if (arg1 == RACE_TOGGLE_ID) return true;

    // Layer 1: chain (spec §6.2). arg1 == 90 is Done/Return; the row whose pane
    // is the current page decides. Main's row (next == -1) is the chargen exit;
    // no match falls through to Main.
    if (arg1 == 90) {
        for (const auto& row : CHAIN) {
            if (row.pane == current_page_) {
                if (row.next == -1) {
                    // Main: the commit path (spec §10.3) — validate the name,
                    // notify-only on failure, otherwise ask for confirmation.
                    handleDone();
                } else {
                    // §9.7: user7 is shared by every page, so a page switch starts
                    // at the top rather than inheriting the previous offset.
                    current_page_ = row.next;
                    scroll_rows_ = 0;
                    if (on_layout_changed_) on_layout_changed_();
                }
                return true;
            }
        }
        current_page_ = PAGE_MAIN;
        scroll_rows_ = 0;
        if (on_layout_changed_) on_layout_changed_();
        return true;
    }

    // Layer 2: family (spec §6.3). arg1 is a tab button id; the row whose
    // (pane, tile) element carries that id decides. target == -1 = finalize
    // only (Random/Reset/Age/Hair/Race-Gender): no page change.
    for (const auto& row : FAMILY) {
        const Element* e = find_on_page(row.pane, tileLabel(row.tile));
        if (e && e->id == arg1) {
            if (row.target != -1) {
                current_page_ = row.target;
                scroll_rows_ = 0;
                if (on_layout_changed_) on_layout_changed_();
            }
            return true;
        }
    }
    // Elements absent from FAMILY (Main "Eyes" toggle id 204, "Complexion"
    // slider id 207) have no menu handler (v662): this returns false and the
    // page does not change. A toggle's value flip and a slider's drag are the
    // widget's own responsibility, independent of this handler.
    return false;
}

// ---------------------------------------------------------------------------
// Geometry (spec §9). Pure logic, host-testable.
// ---------------------------------------------------------------------------

int RaceSexPanel::itemCount(int slot) const {
    int n = 0;
    for (const auto& e : elements_) {
        if (e.page_slot == slot) ++n;
    }
    return n;
}

int RaceSexPanel::paneWidth(int slot) const {
    // spec §9.2. The shipped template reads:
    //   parent().width - ( childcount > 6 ? ... )
    // where the `>` result is the engine boolean (&true;=2, &false;=1 — the
    // canonical XML spells this out: "<user5> &false; </user5> <!-- 1 (false)
    // ... 2 (true) -->"), so `<sub> 1 </sub>` maps 2/1 to 1/0 and `<mul> 40
    // </mul>` makes the shrink a CONSTANT 40px scroll-bar gutter, not 40 per
    // item. Reading it as (childcount-1)*40 drives Nose (14 items) to -20 and
    // makes the page untappable, which the shipped menu does not do.
    return WINDOW_WIDTH - (itemCount(slot) > SCROLL_VISIBLE ? SCROLL_GUTTER : 0);
}

int RaceSexPanel::elementIdAt(int slot, int row) const {
    if (row < 0) return -1;
    int i = 0;
    for (const auto& e : elements_) {
        if (e.page_slot != slot) continue;
        if (i == row) return e.id;
        ++i;
    }
    return -1;
}

int RaceSexPanel::hitTest(int x, int y) const {
    if (!isVisible()) return -1;

    // §9.3 Done first: race_button_done is a sibling of race_window (outside
    // the pane/scroll gutter) and its XML id is 90, i.e. the chain argument.
    if (x >= DONE_X && x < DONE_X + DONE_WIDTH && y >= DONE_Y &&
        y < DONE_Y + DONE_HEIGHT) {
        return DONE_ID;
    }

    // §9.2 the right gutter holds the scroll bar, so it is not part of the pane.
    const int pane_w = paneWidth(current_page_);
    if (x < 0 || x >= pane_w) return -1;

    // §9.1 inverse of y = listindex*93 - scroll_rows*93 (floor, so a negative
    // value never truncates up to row 0).
    const int ey = static_cast<int>(
        std::floor(static_cast<float>(y) + scroll_rows_ * static_cast<float>(ROW_HEIGHT)));
    if (ey < 0) return -1;
    const int row = ey / ROW_HEIGHT;
    if (row >= itemCount(current_page_)) return -1;
    return elementIdAt(current_page_, row);
}

float RaceSexPanel::layoutScale() const {
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);
    if (sw <= 0.0f || sh <= 0.0f) return 1.0f;
    const float s = std::min(sw / CANVAS_WIDTH, sh / CANVAS_HEIGHT);
    return s > 0.0f ? s : 1.0f;
}

float RaceSexPanel::panelOriginX() const {
    // Centred canvas + race_window's canvas x (race_background 70 + window 30).
    const float s = layoutScale();
    return (static_cast<float>(screenWidth) - CANVAS_WIDTH * s) * 0.5f + PANEL_CANVAS_X * s;
}

float RaceSexPanel::panelOriginY() const {
    const float s = layoutScale();
    return (static_cast<float>(screenHeight) - CANVAS_HEIGHT * s) * 0.5f + PANEL_CANVAS_Y * s;
}

int RaceSexPanel::hitTestScreen(float screen_x, float screen_y) const {
    const float s = layoutScale();
    const float px = (screen_x - panelOriginX()) / s;
    const float py = (screen_y - panelOriginY()) / s;
    return hitTest(static_cast<int>(std::floor(px)), static_cast<int>(std::floor(py)));
}

// ---------------------------------------------------------------------------
// Layout / draw (spec §9.5). Pure logic: the host suite pins the rects and the
// visibility set; the device-only applyLayout() consumes them.
// ---------------------------------------------------------------------------

RaceSexPanel::RectF RaceSexPanel::itemRectCanvas(int slot, int row) const {
    RectF r;
    r.x = 0.0f;
    r.y = static_cast<float>(row * ROW_HEIGHT) - scroll_rows_ * static_cast<float>(ROW_HEIGHT);
    r.w = static_cast<float>(paneWidth(slot));
    r.h = static_cast<float>(ROW_HEIGHT);
    return r;
}

RaceSexPanel::RectF RaceSexPanel::itemRectDevice(int slot, int row) const {
    const RectF c = itemRectCanvas(slot, row);
    const float s = layoutScale();
    RectF r;
    r.x = panelOriginX() + c.x * s;
    r.y = panelOriginY() + c.y * s;
    r.w = c.w * s;
    r.h = c.h * s;
    return r;
}

RaceSexPanel::RectF RaceSexPanel::doneRectDevice() const {
    const float s = layoutScale();
    RectF r;
    r.x = panelOriginX() + static_cast<float>(DONE_X) * s;
    r.y = panelOriginY() + static_cast<float>(DONE_Y) * s;
    r.w = static_cast<float>(DONE_WIDTH) * s;
    r.h = static_cast<float>(DONE_HEIGHT) * s;
    return r;
}

RaceSexPanel::RectF RaceSexPanel::scrollBarRectDevice() const {
    // race_scroll_bar (spec §9.3): x = parent.w-20, y = 25, w = 20, h = parent.h-50
    // with parent = race_window (500x558).
    const float s = layoutScale();
    RectF r;
    r.x = panelOriginX() + static_cast<float>(WINDOW_WIDTH - 20) * s;
    r.y = panelOriginY() + 25.0f * s;
    r.w = 20.0f * s;
    r.h = static_cast<float>(WINDOW_HEIGHT - 50) * s;
    return r;
}

RaceSexPanel::RectF RaceSexPanel::nameRectDevice() const {
    // §10.1 race_name (id 10): justify centre at canvas x=350, y=130, wrapwidth
    // 425. Relative to the race_window origin (100,195) that is (250,-65).
    const float s = layoutScale();
    RectF r;
    r.x = panelOriginX() + 250.0f * s;
    r.y = panelOriginY() + (-65.0f) * s;
    r.w = 425.0f * s;
    r.h = 30.0f * s;
    return r;
}

bool RaceSexPanel::isRowVisible(int row) const {
    if (row < 0) return false;
    return row >= scroll_rows_ && row < scroll_rows_ + visibleRows();
}

bool RaceSexPanel::isElementVisible(int slot, int row) const {
    return slot == current_page_ && isRowVisible(row);
}

int RaceSexPanel::rowOfElement(int element_id) const {
    const Element* e = find_element(element_id);
    if (!e) return -1;
    int row = 0;
    for (const Element& x : elements_) {
        if (x.page_slot != e->page_slot) continue;
        if (x.id == e->id) return row;
        ++row;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Scroll input (spec §9.7). Pure logic: the drag math and the clamp are
// host-testable; only the re-layout behind setScrollRows is device-only.
// ---------------------------------------------------------------------------

void RaceSexPanel::setScrollRows(int rows) {
    const int max_rows = std::max(0, itemCount(current_page_) - visibleRows());
    if (rows < 0) rows = 0;
    if (rows > max_rows) rows = max_rows;
    if (rows == scroll_rows_) return;  // re-layout only on an actual change
    scroll_rows_ = rows;
    if (on_layout_changed_) on_layout_changed_();
}

bool RaceSexPanel::isScrollBarHit(float screen_x, float screen_y) const {
    if (!isVisible()) return false;
    // §9.2: the bar only exists when the page overflows the window.
    if (itemCount(current_page_) <= SCROLL_VISIBLE) return false;
    const RectF r = scrollBarRectDevice();
    return screen_x >= r.x && screen_x < r.x + r.w && screen_y >= r.y &&
           screen_y < r.y + r.h;
}

bool RaceSexPanel::beginScrollDrag(float screen_x, float screen_y) {
    if (!isScrollBarHit(screen_x, screen_y)) return false;
    scroll_dragging_ = true;
    scroll_drag_last_y_ = screen_y;
    scroll_drag_accum_px_ = 0.0f;
    scroll_drag_start_rows_ = scroll_rows_;
    return true;
}

void RaceSexPanel::updateScrollDrag(float screen_y) {
    if (!scroll_dragging_) return;
    scroll_drag_accum_px_ += screen_y - scroll_drag_last_y_;
    scroll_drag_last_y_ = screen_y;
    // One row is 93 canvas px; on screen that is ROW_HEIGHT * layoutScale().
    const float row_px = static_cast<float>(ROW_HEIGHT) * layoutScale();
    if (row_px <= 0.0f) return;
    // Round half away from zero (std::llround): a drag of half a row or more
    // moves one row, and the sign is symmetric. llround avoids lround's
    // long-range edge case.
    const float rows = scroll_drag_accum_px_ / row_px;
    setScrollRows(scroll_drag_start_rows_ + static_cast<int>(std::llround(rows)));
}

void RaceSexPanel::endScrollDrag() { scroll_dragging_ = false; }

// ---------------------------------------------------------------------------
// Race selection (spec §10.4). Pure logic: the index, the clamp and the name
// are host-testable; the widgets are refreshed by applyLayout().
// ---------------------------------------------------------------------------

void RaceSexPanel::setAvailableRaces(std::vector<RaceEntry> races) {
    races_ = std::move(races);
    selected_race_index_ = 0;
}

void RaceSexPanel::setSelectedRaceIndex(int index) {
    const int count = raceCount();
    if (count <= 0) {
        selected_race_index_ = 0;
        return;
    }
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;
    selected_race_index_ = index;
}

void RaceSexPanel::cycleRace(int delta) {
    // The original's end behaviour (wrap vs clamp) is not measured; clamp
    // (V-654-3).
    setSelectedRaceIndex(selected_race_index_ + delta);
}

void RaceSexPanel::cycleRaceFromTap(float pane_local_x, float row_width) {
    // spec §10.4: the left half steps back, the right half steps forward.
    cycleRace(pane_local_x >= row_width * 0.5f ? +1 : -1);
}

uint32_t RaceSexPanel::selectedRaceFormID() const {
    if (selected_race_index_ < 0 || selected_race_index_ >= raceCount()) return 0;
    return races_[selected_race_index_].form_id;
}

const std::string& RaceSexPanel::selectedRaceName() const {
    static const std::string kEmpty;
    if (selected_race_index_ < 0 || selected_race_index_ >= raceCount()) return kEmpty;
    return races_[selected_race_index_].name;
}

const std::string& RaceSexPanel::selectedRaceDescription() const {
    static const std::string kEmpty;
    if (selected_race_index_ < 0 || selected_race_index_ >= raceCount()) return kEmpty;
    return races_[selected_race_index_].description;
}

bool RaceSexPanel::raceChanged() const {
    const uint32_t id = selectedRaceFormID();
    return id != 0 && id != current_race_form_id_;
}

// ---------------------------------------------------------------------------
// Name + confirm + commit (spec §10.3).
// ---------------------------------------------------------------------------

RaceSexPanel::NameCheck RaceSexPanel::validateName(const std::string& name) const {
    if (name.empty()) return NameCheck::Empty;
    for (char c : name) {
        if (c == ' ' || c == '\t') return NameCheck::HasSpace;
    }
    if (name == current_name_) return NameCheck::Unchanged;
    return NameCheck::Ok;
}

const char* RaceSexPanel::articleFor(const std::string& name) {
    // V-654-2: the original's rule ([0xB38660]/[0xB38668]) is not measured; use
    // the vowel heuristic.
    if (name.empty()) return "a";
    switch (name[0]) {
        case 'a': case 'e': case 'i': case 'o': case 'u':
        case 'A': case 'E': case 'I': case 'O': case 'U':
            return "an";
        default:
            return "a";
    }
}

RaceSexPanel::DoneAction RaceSexPanel::handleDone() {
    const NameCheck check = validateName(player_name_);
    if (check != NameCheck::Ok) {
        // Notify only; no commit (spec §10.3).
        std::string body;
        switch (check) {
            case NameCheck::Empty: body = "You must enter a name."; break;
            case NameCheck::HasSpace: body = "Names cannot contain spaces."; break;
            case NameCheck::Unchanged: body = "That is already your name."; break;
            default: break;
        }
        if (on_notify_) on_notify_(body);
        return DoneAction::Notify;
    }
    if (on_confirm_) {
        const std::string body =
            std::string(articleFor(player_name_)) + " " + player_name_ + "?";
        on_confirm_(body, [this](bool yes) {
            if (yes) commit();
        });
        return DoneAction::Confirm;
    }
    return DoneAction::None;
}

void RaceSexPanel::commit() {
    confirmed_ = true;
    // The owner applies the race (only when changed) and the name, then closes.
    if (on_commit_) on_commit_();
}

