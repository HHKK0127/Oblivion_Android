#include "topic_resolver.h"

namespace oblivion::dialogue {

bool TopicResolver::isRecursiveConversationType(int32_t conversationType) {
    return conversationType == 2 || conversationType == 3 ||
           conversationType == 4 || conversationType == 6;
}

TopicResolveResult TopicResolver::resolve(const TopicResolveInput& input) const {
    // Native prologue stores 0 to the out byte, then the [this+8] bit5
    // check exits immediately for disabled containers (ret 0x18 @0x52F035).
    if (input.disabled) {
        return TopicResolveResult{};
    }

    // Recursive pass (self-call @0x52F0EE): with the global guard set, a
    // conversation type in {2, 3, 4, 6} and a non-zero param5, the native
    // function clears DAT_00b333c4+0x116, calls itself, then restores the
    // guard before returning. The scan therefore runs with the guard
    // logically cleared; callers that wire Topic_CanUse parity can observe
    // the cleared guard during the pass. The nested call sees the guard
    // clear, so recursion is bounded to one extra level.
    if (input.recursionGuard && input.allowRecursivePass &&
        isRecursiveConversationType(input.conversationType)) {
        TopicResolveInput inner = input;
        inner.recursionGuard = false;
        TopicResolveResult result = resolvePass(inner);
        result.usedRecursion = true;
        return result;
    }

    return resolvePass(input);
}

TopicResolveResult TopicResolver::resolvePass(const TopicResolveInput& input) const {
    TopicResolveResult result;

    // Deferred list: native DAT_00b36510 {count, head} list appended via
    // the push helper (call 0x446CB0 @0x52F270).
    std::vector<int> deferred;

    // Native local_8: last candidate that passed Topic_CanUse.
    int lastEligible = -1;

    for (size_t i = 0; i < input.candidates.size(); ++i) {
        const TopicCandidate& candidate = input.candidates[i];

        // Topic_CanUse (FUN_00530830) is called per candidate @0x52F186.
        if (!candidate.canUse) {
            continue;
        }
        lastEligible = static_cast<int>(i);

        // ASSUMPTION: decompiled.c is not available in this workspace, so
        // the relative order of the bit5 priority-stop check and the bit1
        // alternate check inside the scan loop is not pinned (see
        // tmp/re/v75b_findings.md section 2). The implemented order treats
        // a priority-stop candidate as aborting the scan before it can be
        // deferred or selected, falling through to the end-of-scan logic.
        if (candidate.priorityStop) {
            result.priorityStopTriggered = true;
            break;
        }

        if (!candidate.alternate) {
            // First-eligible-wins: a bit1-clear candidate selects
            // immediately (one of the early ret 0x18 exits).
            result.selectedIndex = static_cast<int>(i);
            result.deferredCount = static_cast<int>(deferred.size());
            return result;
        }

        deferred.push_back(static_cast<int>(i));
    }

    result.deferredCount = static_cast<int>(deferred.size());

    if (!deferred.empty()) {
        // End-of-scan lottery (@0x52F2D9-0x52F32D): native returns the
        // (rand() % len + 1)-th deferred node, i.e. index rand() % len
        // zero-based. The raw modulo (not an unbiased distribution) is
        // kept deliberately for parity with the CRT rand() behaviour.
        const size_t draw = rng_() % static_cast<uint32_t>(deferred.size());
        result.selectedIndex = deferred[draw];
    } else {
        // Empty deferred list: native returns local_8, the last eligible
        // candidate seen during the scan (-1 when none passed).
        result.selectedIndex = lastEligible;
    }

    return result;
}

}  // namespace oblivion::dialogue
