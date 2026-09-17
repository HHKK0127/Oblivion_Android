#include "script_context.h"
#include <algorithm>
#include <cstring>
#include <android/log.h>

#define SCRIPT_LOG_TAG "ScriptVM"
#define SCRIPT_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SCRIPT_LOG_TAG, __VA_ARGS__)

// ============================================================================
// Oblivion Script VM - Execution Context Implementation
// ============================================================================

namespace oblivion {
namespace script {

void ExecutionContext::init(const ScriptData* script) {
    script_ = script;
    if (script) {
        bytecode_ = script->bytecode.data();
        bytecodeSize_ = script->bytecode.size();

        // Initialize local variables from script definition
        locals_.clear();
        locals_.resize(script->variables.size());
        for (size_t i = 0; i < script->variables.size(); ++i) {
            locals_[i] = script->variables[i].defaultValue;
        }

        // Initialize references from script definition
        references_.clear();
        for (size_t i = 0; i < script->references.size(); ++i) {
            references_[static_cast<uint32_t>(i)] = script->references[i];
        }
    } else {
        bytecode_ = nullptr;
        bytecodeSize_ = 0;
        locals_.clear();
        references_.clear();
    }

    // Reset execution state
    stack_.clear();
    pc_ = 0;
    running_ = true;
    instructionCount_ = 0;
    selfRef_ = 0;
    targetRef_ = 0;
}

void ExecutionContext::reset() {
    stack_.clear();
    pc_ = 0;
    running_ = false;
    instructionCount_ = 0;
    selfRef_ = 0;
    targetRef_ = 0;

    // Re-initialize locals from script definition
    if (script_) {
        locals_.clear();
        locals_.resize(script_->variables.size());
        for (size_t i = 0; i < script_->variables.size(); ++i) {
            locals_[i] = script_->variables[i].defaultValue;
        }
    }
}

// ============================================================================
// Stack operations
// ============================================================================

bool ExecutionContext::pushStack(const ScriptValue& value) {
    if (stack_.size() >= limits::MAX_STACK_SIZE) {
        SCRIPT_LOGE("Stack overflow at PC=%u, stack depth=%zu (max=%d)",
                     pc_, stack_.size(), limits::MAX_STACK_SIZE);
        running_ = false;
        return false;
    }
    stack_.push_back(value);
    return true;
}

ScriptValue ExecutionContext::popStack() {
    if (stack_.empty()) {
        SCRIPT_LOGE("Stack underflow at PC=%u, stack is empty", pc_);
        running_ = false;
        return ScriptValue::makeInt(0);
    }
    ScriptValue val = stack_.back();
    stack_.pop_back();
    return val;
}

ScriptValue ExecutionContext::peekStack() const {
    if (stack_.empty()) {
        return ScriptValue::makeInt(0);
    }
    return stack_.back();
}

// ============================================================================
// Local variable access
// ============================================================================

void ExecutionContext::setLocal(uint16_t index, const ScriptValue& value) {
    if (index < locals_.size()) {
        locals_[index] = value;
    }
}

ScriptValue ExecutionContext::getLocal(uint16_t index) const {
    if (index < locals_.size()) {
        return locals_[index];
    }
    return ScriptValue::makeInt(0);
}

// ============================================================================
// Reference table
// ============================================================================

void ExecutionContext::addReference(uint32_t index, uint32_t formID) {
    references_[index] = formID;
}

uint32_t ExecutionContext::getReference(uint32_t index) const {
    auto it = references_.find(index);
    if (it != references_.end()) {
        return it->second;
    }
    return 0;
}

} // namespace script
} // namespace oblivion
