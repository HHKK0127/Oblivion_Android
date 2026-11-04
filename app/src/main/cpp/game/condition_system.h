// Condition (CTDA) system — faithful port of the shipping engine's condition evaluator.
//
// RE basis (docs/RE_EXE_REPORT.md section 58.1, evidence docs/Oblivion_decompiled.c):
//   FUN_0056a510 @ 0x56A510  condition-list evaluator (AND/OR grouping, skip semantics)
//   FUN_0056ac50 @ 0x56AC50  single-condition evaluator (swap, needs-ref, global compare)
//   FUN_0056a3b0 @ 0x56A3B0  float comparison switch
//   FUN_0056ad60 @ 0x56AD60  serialization (in-memory 0x18 item; on-disk CTDA record 0x14)
//   Function table base 0x00B0C8C0, stride 0x28; cond-capable rows extracted to
//   tmp/re/s58_condfunctions_compact.txt and embedded below as kCondFunctions.
//
// The same evaluator chain is shared by AI packages and DIAL/INFO topic selection.
#ifndef OBLIVION_GAME_CONDITION_SYSTEM_H
#define OBLIVION_GAME_CONDITION_SYSTEM_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace game {

// On-disk CTDA record layout (0x14 bytes of record data):
//   +0x00 flag byte   bit0 = OR with previous, bit1 = swap operands, bit2 = compare TESGlobal,
//                     bits 5-7 = comparison operation (0..5)
//   +0x01..03 padding
//   +0x04 float comparison value (raw IEEE-754 bits; formID when flag bit2 set)
//   +0x08 u16 condition function index (TESM table index; see kCondFunctions)
//   +0x0A padding
//   +0x0C param1 (formID / integer / float bits)
//   +0x10 param2
// There is no param3 on disk (the in-memory item carries one at +0x14, unserialized).
struct ConditionItem {
    uint8_t flag = 0;
    uint32_t comparison_value = 0;  // raw float bits, or TESGlobal formID when uses_global()
    uint16_t function_index = 0;
    uint32_t param1 = 0;
    uint32_t param2 = 0;

    bool is_or() const { return (flag & 1u) != 0; }
    bool is_swapped() const { return ((flag >> 1) & 1u) != 0; }
    bool uses_global() const { return ((flag >> 2) & 1u) != 0; }
    uint8_t comparison_op() const { return static_cast<uint8_t>(flag >> 5); }
    float compare_float() const {
        float f;
        std::memcpy(&f, &comparison_value, sizeof(f));
        return f;
    }

    // Parse one 0x14-byte CTDA record body. Returns false when data is too short.
    static bool parse(const uint8_t* data, size_t size, ConditionItem& out);
};

// Comparison operations (flag bits 5-7). Values 6..7 never appear in the toolset;
// the shipping switch has no default case so they always pass (preserved verbatim).
enum class ConditionOp : uint32_t {
    Equal = 0,
    NotEqual = 1,
    Greater = 2,
    GreaterEqual = 3,
    Less = 4,
    LessEqual = 5,
};

// Literal port of FUN_0056a3b0. left = condition function result, right = comparison value.
// Returns 1 when the comparison passes.
uint32_t compare_conditions(uint32_t op, float left, float right);

// Pluggable source of runtime facts. A game-side implementation answers the
// condition functions it supports; anything left unanswered is treated as
// UNKNOWN (skip) so callers can choose conservative behavior.
class ConditionRuntime {
public:
    virtual ~ConditionRuntime() = default;

    // Evaluate condition function `index` for `subject`. Return true when this
    // runtime implements the function and store its result in `result`; return
    // false for functions that are not supported (the evaluator reports skip).
    virtual bool query(uint16_t index, uint32_t subject, uint32_t param1,
                       uint32_t param2, float& result) = 0;

    // Value of a TESGlobal referenced by formID (flag bit2 comparisons).
    virtual float global_value(uint32_t global_form_id) = 0;

    // Hook for FUN_004d78e0: a non-zero return replaces the compare target
    // (global context object). Default: no override.
    virtual uint32_t context_override(uint32_t compare_target) const {
        (void)compare_target;
        return 0;
    }
};

struct EvalResult {
    bool passed = false;
    bool skip = false;  // condition unknown/skipped — callers must not treat as a hard failure
};

// Literal port of FUN_0056ac50 (single-condition evaluator).
EvalResult evaluate_single(const ConditionItem& item, uint32_t run_as,
                           uint32_t compare_target, ConditionRuntime& rt);

// Literal port of FUN_0056a510 (condition-list evaluator). Vector order matches
// the engine's linked-list node order; `secondary` continues evaluation once when
// the primary list ends (DIAL + INFO combined condition lists).
// Empty or null list passes (return 1 in the original).
EvalResult evaluate_list(const std::vector<ConditionItem>* list, uint32_t run_as,
                         uint32_t compare_target, ConditionRuntime& rt,
                         const std::vector<ConditionItem>* secondary = nullptr);

// Table entry for a condition-capable script function (shipping table rows with
// a non-NULL condfunc pointer). Index range is below 0x171.
struct CondFuncEntry {
    uint16_t index;
    const char* name;
    uint8_t param_count;
    uint8_t needs_ref;  // low byte of the table flags field (entry+0x10)
};

// All 169 condition-capable functions, sorted by index.
extern const CondFuncEntry kCondFunctions[];
extern const size_t kCondFunctionCount;

// Look up a table row; nullptr when the index is not condition-capable.
const CondFuncEntry* find_cond_function(uint16_t index);

}  // namespace game

#endif  // OBLIVION_GAME_CONDITION_SYSTEM_H
