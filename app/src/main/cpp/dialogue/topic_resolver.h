#pragma once

// Topic_Resolve parity resolver.
//
// Models FUN_0052F010 (Topic_Resolve) from the Windows executable as
// reverse-engineered in tmp/re/v75b_findings.md section 2. Given the
// ordered candidate list for a Topic (the native this+0x28 Conversation
// list), it selects the candidate the game would present, following the
// native can-use scan, alternate deferral, priority-stop and end-of-scan
// lottery semantics, plus the guard-cleared recursive pass.
//
// The component is deliberately standalone: it includes no game or
// dialogue_tree.h headers so the selection logic stays host-testable.
// Candidate evaluation (Topic_CanUse parity) is performed by the caller
// and injected through TopicCandidate::canUse.

#include <cstdint>
#include <random>
#include <vector>

namespace oblivion::dialogue {

// One topic candidate with the caller-evaluated flags the native scan
// reads from [cand+0x25].
struct TopicCandidate {
    uint32_t formID = 0;
    // Topic_CanUse (FUN_00530830) result for this candidate.
    bool canUse = false;
    // [cand+0x25] bit1: deferred candidate. Eligible alternates are
    // appended to the deferred list instead of being selected at once.
    bool alternate = false;
    // [cand+0x25] bit5: priority stop. An eligible priority-stop
    // candidate aborts the scan.
    bool priorityStop = false;
};

// Inputs for one Topic_Resolve invocation.
struct TopicResolveInput {
    // [this+8] bit5: disabled container; resolve() then selects nothing
    // (native early exit ret 0x18 @0x52F035 with the out byte left 0).
    bool disabled = false;
    // [this+0x24]: conversation type. The recursive pass runs only for
    // types {2, 3, 4, 6}.
    int32_t conversationType = 0;
    // DAT_00b333c4+0x116: global recursion guard.
    bool recursionGuard = false;
    // Native param5: the recursive pass requires it non-zero.
    bool allowRecursivePass = false;
    // Candidate list in native table order (this+0x28 walk order).
    std::vector<TopicCandidate> candidates;
};

// Outcome of one Topic_Resolve pass.
struct TopicResolveResult {
    // Index into TopicResolveInput::candidates of the selected candidate,
    // or -1 when nothing was selected.
    int selectedIndex = -1;
    // True when the selection came from the guard-cleared recursive pass.
    bool usedRecursion = false;
    // True when the scan was aborted by a priority-stop candidate.
    bool priorityStopTriggered = false;
    // Number of alternate candidates deferred during the scan.
    int deferredCount = 0;
};

class TopicResolver {
public:
    TopicResolver() : rng_(std::random_device{}()) {}

    // Makes the end-of-scan lottery deterministic (tests, replays).
    void setSeed(uint32_t seed) { rng_.seed(seed); }

    // Runs the native Topic_Resolve algorithm (FUN_0052F010).
    TopicResolveResult resolve(const TopicResolveInput& input) const;

private:
    static bool isRecursiveConversationType(int32_t conversationType);
    TopicResolveResult resolvePass(const TopicResolveInput& input) const;

    mutable std::mt19937 rng_;
};

}  // namespace oblivion::dialogue
