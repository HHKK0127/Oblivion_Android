// ============================================================================
// gmst_string_tests.cpp — Host tests for the GMST loader-parity layers
// See gmst_string_tests.h for scope. Canon: tmp/re/v60_esm_only_keys.md [確定].
// ============================================================================

#include "tests/gmst_string_tests.h"

#include "game/gmst_defaults.h"

#include <cstring>

namespace oblivion {
namespace gmst_string {

namespace {

// The EXE-registered default for sSpecialization (binary VA 0xA5E1A8).
// Written as adjacent literals so the source text matches the header table.
const char* const kSpecializationDefault =
    "You will gain +10 in all specialization skills. "
    "Those skills will also increase faster.";

// The value carried by Oblivion.esm GMST 0x18E6; publishGmstOverrides()
// pushes this over the default at load time.
const char* const kSpecializationEsm =
    "Choose your Specialization. Specialized skills get "
    "+10 and increase faster.";

// The four fRegionGenTex* keys exist in the ESM but the EXE never registers
// them (v60 canon): lookups must miss on BOTH the float and string tables.
const char* const kRegionGenTexKeys[] = {
    "fRegionGenTexPlacedNotMatch",
    "fRegionGenTexPlacedMatch",
    "fRegionGenTexGenNotMatch",
    "fRegionGenTexGenMatch",
};

}  // namespace

GmstStringTests::GmstStringTests() = default;

bool GmstStringTests::runAllTests() {
    results.clear();
    test_string_default_present();
    test_string_default_verbatim();
    test_string_unknown_key();
    test_region_gen_tex_keys_absent();
    test_string_override_publish();
    test_string_override_clear();
    test_string_override_unknown_noop();
    test_string_override_replacement();
    test_float_layer_defaults();
    test_float_layer_override_roundtrip();
    test_float_layer_unknown_sentinel();

    for (const auto& r : results) {
        if (!r.passed) {
            return false;
        }
    }
    return true;
}

void GmstStringTests::test_string_default_present() {
    std::string msg;
    bool ok = gmst::hasStringKey("sSpecialization");
    if (ok) {
        const std::size_t idx = gmst::stringIndexOf("sSpecialization");
        if (idx >= gmst::kStringEntryCount) {
            ok = false;
            msg = "stringIndexOf returned kStringEntryCount";
        }
    } else {
        msg = "hasStringKey(sSpecialization) is false";
    }
    record("string_default_present", ok, msg);
}

void GmstStringTests::test_string_default_verbatim() {
    std::string msg;
    const char* got = gmst::getString("sSpecialization");
    const bool ok = std::strcmp(got, kSpecializationDefault) == 0;
    if (!ok) {
        msg = std::string("getString returned \"") + got + "\"";
    }
    record("string_default_verbatim", ok, msg);
}

void GmstStringTests::test_string_unknown_key() {
    std::string msg;
    bool ok = !gmst::hasStringKey("sNoSuchSetting");
    if (ok && gmst::getString("sNoSuchSetting")[0] != '\0') {
        ok = false;
        msg = "getString on unknown key is not empty";
    }
    if (ok && gmst::stringIndexOf("sNoSuchSetting") != gmst::kStringEntryCount) {
        ok = false;
        msg = "stringIndexOf on unknown key is not kStringEntryCount";
    }
    record("string_unknown_key", ok, msg);
}

void GmstStringTests::test_region_gen_tex_keys_absent() {
    std::string msg;
    bool ok = true;
    for (const char* key : kRegionGenTexKeys) {
        if (gmst::hasStringKey(key) || gmst::hasKey(key)) {
            ok = false;
            msg = std::string(key) + " must not be registered (v60 canon)";
            break;
        }
    }
    record("region_gen_tex_keys_absent", ok, msg);
}

void GmstStringTests::test_string_override_publish() {
    std::string msg;
    gmst::setStringOverride("sSpecialization", kSpecializationEsm);
    const char* live = gmst::getStringLive("sSpecialization");
    bool ok = std::strcmp(live, kSpecializationEsm) == 0;
    if (ok) {
        ok = gmst::hasStringOverride("sSpecialization");
        if (!ok) {
            msg = "hasStringOverride is false after publish";
        }
    } else {
        msg = std::string("getStringLive returned \"") + live + "\"";
    }
    gmst::clearStringOverride("sSpecialization");
    record("string_override_publish", ok, msg);
}

void GmstStringTests::test_string_override_clear() {
    std::string msg;
    gmst::setStringOverride("sSpecialization", kSpecializationEsm);
    gmst::clearStringOverride("sSpecialization");
    bool ok = !gmst::hasStringOverride("sSpecialization");
    if (ok) {
        const char* live = gmst::getStringLive("sSpecialization");
        ok = std::strcmp(live, kSpecializationDefault) == 0;
        if (!ok) {
            msg = std::string("getStringLive after clear returned \"") + live +
                  "\" instead of the default";
        }
    } else {
        msg = "hasStringOverride still true after clear";
    }
    record("string_override_clear", ok, msg);
}

void GmstStringTests::test_string_override_unknown_noop() {
    std::string msg;
    // EXE semantics: a GMST the binary never registered is ignored on load,
    // so publishing to an unknown key must not create an override.
    gmst::setStringOverride("sNoSuchSetting", "x");
    bool ok = !gmst::hasStringOverride("sNoSuchSetting");
    if (ok && gmst::getStringLive("sNoSuchSetting")[0] != '\0') {
        ok = false;
        msg = "getStringLive on unknown key is not empty";
    }
    gmst::clearStringOverride("sNoSuchSetting");
    record("string_override_unknown_noop", ok, msg);
}

void GmstStringTests::test_string_override_replacement() {
    std::string msg;
    gmst::setStringOverride("sSpecialization", "first");
    gmst::setStringOverride("sSpecialization", "second");
    const char* live = gmst::getStringLive("sSpecialization");
    bool ok = std::strcmp(live, "second") == 0;
    if (!ok) {
        msg = std::string("getStringLive returned \"") + live +
              "\" after replacement";
    }
    gmst::clearStringOverride("sSpecialization");
    if (ok && gmst::hasStringOverride("sSpecialization")) {
        ok = false;
        msg = "override still set after cleanup clear";
    }
    record("string_override_replacement", ok, msg);
}

void GmstStringTests::test_float_layer_defaults() {
    std::string msg;
    bool ok = gmst::hasKey("fFatigueBase");
    if (ok) {
        ok = gmst::get("fFatigueBase") == 1.25f;
        if (!ok) {
            msg = "get(fFatigueBase) is not 1.25";
        }
    } else {
        msg = "hasKey(fFatigueBase) is false";
    }
    record("float_layer_defaults", ok, msg);
}

void GmstStringTests::test_float_layer_override_roundtrip() {
    std::string msg;
    gmst::setOverride("fFatigueBase", 2.5f);
    bool ok = gmst::hasOverride("fFatigueBase");
    if (ok) {
        ok = gmst::getLive("fFatigueBase") == 2.5f;
        if (!ok) {
            msg = "getLive did not return the published 2.5";
        }
    } else {
        msg = "hasOverride is false after setOverride";
    }
    gmst::clearOverride("fFatigueBase");
    if (ok) {
        ok = !gmst::hasOverride("fFatigueBase") &&
             gmst::getLive("fFatigueBase") == 1.25f;
        if (!ok) {
            msg = "value did not fall back to the default after clear";
        }
    }
    record("float_layer_override_roundtrip", ok, msg);
}

void GmstStringTests::test_float_layer_unknown_sentinel() {
    std::string msg;
    bool ok = !gmst::hasKey("fNoSuchSetting");
    if (ok) {
        ok = gmst::getLive("fNoSuchSetting") == 0.0f;
        if (!ok) {
            msg = "getLive on unknown key is not the 0.0 sentinel";
        }
    } else {
        msg = "hasKey(fNoSuchSetting) is true";
    }
    if (ok) {
        // Unknown float keys must also be absent from the string table.
        ok = !gmst::hasStringKey("fNoSuchSetting");
        if (!ok) {
            msg = "hasStringKey(fNoSuchSetting) is true";
        }
    }
    record("float_layer_unknown_sentinel", ok, msg);
}

void GmstStringTests::record(const std::string& name, bool passed,
                             const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int GmstStringTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (r.passed) {
            ++n;
        }
    }
    return n;
}

int GmstStringTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (!r.passed) {
            ++n;
        }
    }
    return n;
}

std::string GmstStringTests::getSummary() const {
    return std::to_string(getPassCount()) + "/" +
           std::to_string(results.size()) + " passed";
}

}  // namespace gmst_string
}  // namespace oblivion
