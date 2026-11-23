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
