#include "race_sex_panel.h"

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
// Page builders (skeleton: empty bodies).
//
// Items are listed in build order with their runtime id (spec §0: id is the
// identity; names are display-only). Sliders consume two ids (id and user0),
// which is why some ids are skipped. Slider range is the template default
// (spec §5): range 0..1, step 0.04, jump 0.1.
// ---------------------------------------------------------------------------

void RaceSexPanel::build_main() {
    // Main (spec §2 #1, slot +0x40, page id 0). Items in build order:
    //   id=201 button  "Race"                  -> FAMILY const 0x12, target Race
    //   id=202 button  "Face"                  -> FAMILY const 0xa,  target Face
    //   id=203 button  "Hair"                  -> FAMILY const 0x10, target Hair
    //   id=204 toggle  "Eyes"                  -> NOT in FAMILY (action TBD)
    //   id=205 slider  "Age"        range 0..1 step 0.04 jump 0.1
    //                                          -> FAMILY const TBD, no page switch
    //   id=207 slider  "Complexion" range 0..1 step 0.04 jump 0.1
    //                                          -> NOT in FAMILY (action TBD)
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
    // Race (slot +0x44, page id 209). toggle Gender / toggle Race / text Race
    // Description. (item ids: see spec §2; TBD when implemented)
}

void RaceSexPanel::build_hair() {
    // Hair (slot +0x48, page id 213). toggle Style / toggle Hair Color /
    // sliders Red, Green, Blue, Length.
}

void RaceSexPanel::build_face() {
    // Face (slot +0x4c, page id 224). buttons Shape / Tone / Random / Reset.
}

void RaceSexPanel::build_shape() {
    // Shape (slot +0x50, page id 229). buttons General(Face) / Brow / Cheeks /
    // Chin / Eyes / Forehead / Jaw / Mouth / Nose.
}

void RaceSexPanel::build_tone() {
    // Tone (slot +0x54, page id 239). buttons General(Skin) / Beard /
    // Eye shades / Eyebrows / Mouth(Lips) / Nose shades.
}

void RaceSexPanel::build_general_face() {
    // General(Face) (slot +0x58, page id 246). 5 sliders.
}

void RaceSexPanel::build_brow() {
    // Brow (slot +0x5c, page id 257). 3 sliders.
}

void RaceSexPanel::build_cheeks() {
    // Cheeks (slot +0x60, page id 264). 5 sliders.
}

void RaceSexPanel::build_chin() {
    // Chin (slot +0x64, page id 275). 7 sliders.
}

void RaceSexPanel::build_eyes() {
    // Eyes (slot +0x68, page id 290). 4 sliders.
}

void RaceSexPanel::build_forehead() {
    // Forehead (slot +0x6c, page id 299). 3 sliders.
}

void RaceSexPanel::build_jaw() {
    // Jaw (slot +0x70, page id 306). 4 sliders.
}

void RaceSexPanel::build_mouth() {
    // Mouth (slot +0x74, page id 315). 10 sliders.
}

void RaceSexPanel::build_nose() {
    // Nose (slot +0x78, page id 336). 14 sliders.
}

void RaceSexPanel::build_general_skin() {
    // General(Skin) (slot +0x7c, page id 365). 5 sliders.
}

void RaceSexPanel::build_beard() {
    // Beard (slot +0x80, page id 376). 6 sliders.
}

void RaceSexPanel::build_eyes_tex() {
    // Eye shades (slot +0x84, page id 389). 6 sliders.
}

void RaceSexPanel::build_eyebrows() {
    // Eyebrows (slot +0x88, page id 402). 7 sliders.
}

void RaceSexPanel::build_mouth_lips() {
    // Mouth(Lips) (slot +0x8c, page id 417). 3 sliders.
}

void RaceSexPanel::build_nose_tex() {
    // Nose shades (slot +0x90, page id 424). 2 sliders.
}
