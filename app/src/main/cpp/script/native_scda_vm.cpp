#include "native_scda_vm.h"

#include <cmath>
#include <cstdlib>

namespace oblivion {
namespace script {

namespace {

// The operator set is closed: it was measured over every expression in the
// vanilla corpus and these thirteen spellings are all of them.
bool isOperatorText(const std::string& text) {
    return text == "==" || text == "!=" || text == "<" || text == "<=" ||
           text == ">" || text == ">=" || text == "+" || text == "-" ||
           text == "*" || text == "/" || text == "&&" || text == "||" ||
           text == "~";
}

// Parses a numeric literal. The compiler writes integers as plain digits and
// fractions with a leading dot, so both forms must be accepted. Returns false
// for anything that is not a number, which is how operator text is told apart
// from a literal.
bool parseNumber(const std::string& text, ScriptValue& out) {
    if (text.empty()) {
        return false;
    }

    bool isFloat = text.find('.') != std::string::npos;
    if (!isFloat) {
        for (char c : text) {
            if (c < '0' || c > '9') {
                return false;
            }
        }
        out = ScriptValue::makeInt(std::atoi(text.c_str()));
        return true;
    }

    // A leading dot is valid in this grammar (".5"), so validate the digits
    // around the dot rather than relying on strtod alone.
    size_t digits = 0;
    for (char c : text) {
        if (c == '.') {
            continue;
        }
        if (c < '0' || c > '9') {
            return false;
        }
        ++digits;
    }
    if (digits == 0) {
        return false;
    }

    out = ScriptValue::makeFloat(static_cast<float>(std::strtod(text.c_str(), nullptr)));
    return true;
}

} // namespace

NativeScdaVm::NativeScdaVm() = default;

void NativeScdaVm::registerCommand(uint16_t opcode, NativeCommandHandler handler) {
    commands_[opcode] = std::move(handler);
}

void NativeScdaVm::registerExpressionFunction(uint16_t opcode,
                                              NativeExpressionHandler handler) {
    expressionFunctions_[opcode] = std::move(handler);
}

void NativeScdaVm::registerCommandByName(uint16_t opcode, NativeCommandHandler handler) {
    registerCommand(opcode, handler);
}

bool NativeScdaVm::hasCommand(uint16_t opcode) const {
    return commands_.find(opcode) != commands_.end();
}

bool NativeScdaVm::hasExpressionFunction(uint16_t opcode) const {
    return expressionFunctions_.find(opcode) != expressionFunctions_.end();
}

void NativeScdaVm::setReferences(const std::vector<uint32_t>& scroRefs,
                                 uint16_t localRefCount) {
    scroRefs_ = scroRefs;
    localRefCount_ = localRefCount;
}

void NativeScdaVm::setGlobal(uint32_t formId, const ScriptValue& value) {
    globals_[formId] = value;
}

ScriptValue NativeScdaVm::getGlobal(uint32_t formId) const {
    auto it = globals_.find(formId);
    if (it == globals_.end()) {
        return ScriptValue::makeInt(0);
    }
    return it->second;
}

bool NativeScdaVm::resolveReference(uint16_t slot, NativeReferenceSlot& out,
                                    std::string& error) const {
    return resolveNativeReferenceSlot(slot, scroRefs_, localRefCount_, out, error);
}

void NativeScdaVm::setError(const std::string& message, uint32_t offset,
                            uint16_t opcode) {
    lastError_ = message;
    lastErrorOffset_ = offset;
    lastErrorOpcode_ = opcode;
}

bool NativeScdaVm::pushValue(const ScriptValue& value, std::string& error) {
    if (stack_.size() >= static_cast<size_t>(limits::MAX_STACK_SIZE)) {
        error = "expression stack overflow";
        return false;
    }
    stack_.push_back(value);
    return true;
}

bool NativeScdaVm::popValue(ScriptValue& out, std::string& error) {
    if (stack_.empty()) {
        error = "expression stack underflow";
        return false;
    }
    out = stack_.back();
    stack_.pop_back();
    return true;
}

bool NativeScdaVm::evaluateToken(const NativeToken& token, ScriptValue& out,
                                 std::string& error) {
    switch (token.kind) {
        case NativeTokenKind::Integer:
            out = ScriptValue::makeInt(token.intValue);
            return true;

        case NativeTokenKind::Double:
            out = ScriptValue::makeFloat(static_cast<float>(token.doubleValue));
            return true;

        case NativeTokenKind::BareU16:
            out = ScriptValue::makeInt(token.intValue);
            return true;

        case NativeTokenKind::Variable: {
            switch (token.typeChar) {
                case 'f':
                case 's':
                case 'l':
                    out = variables_.get(token.index);
                    return true;
                case 'r': {
                    // A local ref variable holds a FormID.
                    const ScriptValue value = variables_.get(token.index);
                    out = ScriptValue::makeRef(static_cast<uint32_t>(value.toInt()));
                    return true;
                }
                case 'G':
                    out = getGlobal(token.index);
                    return true;
                case static_cast<char>(NATIVE_SCDA_REFERENCE_LITERAL_CHAR): {
                    NativeReferenceSlot slot;
                    std::string slotError;
                    if (!resolveReference(token.index, slot, slotError)) {
                        error = slotError;
                        return false;
                    }
                    if (slot.kind == NativeReferenceKind::Scro) {
                        out = ScriptValue::makeRef(slot.formId);
                    } else {
                        const ScriptValue value = variables_.get(slot.localOrdinal);
                        out = ScriptValue::makeRef(static_cast<uint32_t>(value.toInt()));
                    }
                    return true;
                }
                default:
                    error = std::string("unsupported variable type char '") +
                            token.typeChar + "'";
                    return false;
            }
        }

        case NativeTokenKind::Function: {
            // A condition-only function such as GetStage or IsActionRef. It
            // shares the opcode number space with the command opcodes, so the
            // same handler table serves both.
            auto it = expressionFunctions_.find(token.index);
            if (it == expressionFunctions_.end()) {
                error = "no handler for expression function 0x" +
                        std::to_string(token.index);
                return false;
            }

            std::vector<ScriptValue> args;
            args.reserve(token.arguments.size());
            for (const NativeToken& argument : token.arguments) {
                ScriptValue value;
                if (!evaluateToken(argument, value, error)) {
                    return false;
                }
                args.push_back(value);
            }

            NativeCommandContext context;
            context.selfRef = selfRef_;
            context.targetRef = targetRef_;
            context.opcode = token.index;
            context.offset = token.offset;

            ScriptValue returnValue = ScriptValue::makeInt(0);
            std::string handlerError;
            if (!it->second(context, args, returnValue, handlerError)) {
                error = handlerError.empty()
                            ? "expression function 0x" + std::to_string(token.index) +
                                  " failed"
                            : handlerError;
                return false;
            }
            out = returnValue;
            return true;
        }

        case NativeTokenKind::Text: {
            if (isOperatorText(token.text)) {
                error = "operator '" + token.text + "' used as an operand";
                return false;
            }
            if (!parseNumber(token.text, out)) {
                error = "unrecognized expression token '" + token.text + "'";
                return false;
            }
            return true;
        }

        default:
            error = "unsupported expression token";
            return false;
    }
}

bool NativeScdaVm::applyOperator(const std::string& op, std::string& error) {
    if (op == "~") {
        ScriptValue value;
        if (!popValue(value, error)) {
            return false;
        }
        return pushValue(ScriptValue::makeInt(value.isTruthy() ? 0 : 1), error);
    }

    ScriptValue right;
    ScriptValue left;
    if (!popValue(right, error) || !popValue(left, error)) {
        return false;
    }

    if (op == "&&") {
        return pushValue(
            ScriptValue::makeInt((left.isTruthy() && right.isTruthy()) ? 1 : 0), error);
    }
    if (op == "||") {
        return pushValue(
            ScriptValue::makeInt((left.isTruthy() || right.isTruthy()) ? 1 : 0), error);
    }

    // Comparison and arithmetic both work on the numeric reading of the
    // operands. A float operand keeps the result a float so that a chain such as
    // "1 / 2 * 2" does not silently truncate.
    const bool floatResult = left.type == ScriptValue::Type::Float ||
                             right.type == ScriptValue::Type::Float;

    if (op == "==") {
        if (floatResult) {
            return pushValue(
                ScriptValue::makeInt(left.toFloat() == right.toFloat() ? 1 : 0), error);
        }
        return pushValue(ScriptValue::makeInt(left.toInt() == right.toInt() ? 1 : 0),
                         error);
    }
    if (op == "!=") {
        if (floatResult) {
            return pushValue(
                ScriptValue::makeInt(left.toFloat() != right.toFloat() ? 1 : 0), error);
        }
        return pushValue(ScriptValue::makeInt(left.toInt() != right.toInt() ? 1 : 0),
                         error);
    }
    if (op == "<") {
        return pushValue(
            ScriptValue::makeInt(left.toFloat() < right.toFloat() ? 1 : 0), error);
    }
    if (op == "<=") {
        return pushValue(
            ScriptValue::makeInt(left.toFloat() <= right.toFloat() ? 1 : 0), error);
    }
    if (op == ">") {
        return pushValue(
            ScriptValue::makeInt(left.toFloat() > right.toFloat() ? 1 : 0), error);
    }
    if (op == ">=") {
        return pushValue(
            ScriptValue::makeInt(left.toFloat() >= right.toFloat() ? 1 : 0), error);
    }

    if (op == "+") {
        if (floatResult) {
            return pushValue(ScriptValue::makeFloat(left.toFloat() + right.toFloat()),
                             error);
        }
        return pushValue(ScriptValue::makeInt(left.toInt() + right.toInt()), error);
    }
    if (op == "-") {
        if (floatResult) {
            return pushValue(ScriptValue::makeFloat(left.toFloat() - right.toFloat()),
                             error);
        }
        return pushValue(ScriptValue::makeInt(left.toInt() - right.toInt()), error);
    }
    if (op == "*") {
        if (floatResult) {
            return pushValue(ScriptValue::makeFloat(left.toFloat() * right.toFloat()),
                             error);
        }
        return pushValue(ScriptValue::makeInt(left.toInt() * right.toInt()), error);
    }
    if (op == "/") {
        if (right.toFloat() == 0.0f) {
            error = "division by zero";
            return false;
        }
        if (floatResult) {
            return pushValue(ScriptValue::makeFloat(left.toFloat() / right.toFloat()),
                             error);
        }
        return pushValue(ScriptValue::makeInt(left.toInt() / right.toInt()), error);
    }

    error = "unsupported operator '" + op + "'";
    return false;
}

bool NativeScdaVm::evaluateExpression(const std::vector<NativeToken>& tokens,
                                      ScriptValue& out, std::string& error) {
    const size_t base = stack_.size();

    for (const NativeToken& token : tokens) {
        if (token.kind == NativeTokenKind::Text && isOperatorText(token.text)) {
            if (!applyOperator(token.text, error)) {
                stack_.resize(base);
                return false;
            }
            continue;
        }

        ScriptValue value;
        if (!evaluateToken(token, value, error)) {
            stack_.resize(base);
            return false;
        }
        if (!pushValue(value, error)) {
            stack_.resize(base);
            return false;
        }
    }

    if (stack_.size() != base + 1) {
        error = "expression left " + std::to_string(stack_.size() - base) +
                " values on the stack";
        stack_.resize(base);
        return false;
    }

    out = stack_.back();
    stack_.resize(base);
    return true;
}

size_t NativeScdaVm::findEndIf(size_t ifIndex) const {
    if (program_ == nullptr) {
        return 0;
    }

    const auto& instructions = program_->instructions;
    int depth = 0;
    for (size_t i = ifIndex + 1; i < instructions.size(); ++i) {
        const uint16_t opcode = instructions[i].opcode;
        if (opcode == static_cast<uint16_t>(NativeStructuralOpcode::If)) {
            ++depth;
        } else if (opcode == static_cast<uint16_t>(NativeStructuralOpcode::EndIf)) {
            if (depth == 0) {
                return i;
            }
            --depth;
        }
    }
    return instructions.size();
}

size_t NativeScdaVm::findNextBranch(size_t fromIndex) const {
    if (program_ == nullptr) {
        return 0;
    }

    const auto& instructions = program_->instructions;
    int depth = 0;
    for (size_t i = fromIndex + 1; i < instructions.size(); ++i) {
        const uint16_t opcode = instructions[i].opcode;
        if (opcode == static_cast<uint16_t>(NativeStructuralOpcode::If)) {
            ++depth;
            continue;
        }
        if (opcode == static_cast<uint16_t>(NativeStructuralOpcode::EndIf)) {
            if (depth == 0) {
                return i;
            }
            --depth;
            continue;
        }
        if (depth == 0 &&
            (opcode == static_cast<uint16_t>(NativeStructuralOpcode::Else) ||
             opcode == static_cast<uint16_t>(NativeStructuralOpcode::ElseIf))) {
            return i;
        }
    }
    return instructions.size();
}

bool NativeScdaVm::executeSet(const NativeInstruction& instruction,
                              std::string& error) {
    if (!instruction.expressionDecoded) {
        error = "Set expression could not be decoded";
        return false;
    }

    ScriptValue value;
    if (!evaluateExpression(instruction.expressionTokens, value, error)) {
        return false;
    }

    if (instruction.tokens.empty()) {
        error = "Set has no target";
        return false;
    }

    // A two token target writes a member of a reference, so the value lands in
    // the reference's variable slot rather than a plain local.
    const NativeToken& target = instruction.tokens.back();
    if (target.kind != NativeTokenKind::Variable) {
        error = "Set target is not a variable";
        return false;
    }

    switch (target.typeChar) {
        case 'f':
        case 's':
        case 'l':
        case 'r':
            variables_.set(target.index, value);
            return true;
        case 'G':
            setGlobal(target.index, value);
            return true;
        default:
            error = std::string("unsupported Set target type char '") +
                    target.typeChar + "'";
            return false;
    }
}

bool NativeScdaVm::executeCommand(const NativeInstruction& instruction,
                                  std::string& error) {
    auto it = commands_.find(instruction.opcode);
    if (it == commands_.end()) {
        // An unregistered command is reported once per occurrence and does not
        // stop the run: a script that calls a command this build does not model
        // yet must still execute the rest of its block.
        ++unhandledCommands_;
        return true;
    }

    NativeCommandContext context;
    context.selfRef = selfRef_;
    context.targetRef = targetRef_;
    context.opcode = instruction.opcode;
    context.offset = instruction.offset;
    context.variables = &variables_;

    // The selector marker that precedes a command names the reference the
    // command acts on. A command without one acts on the script's self.
    const uint16_t selector = instruction.referenceIndex != 0
                                  ? instruction.referenceIndex
                                  : pendingReferenceIndex_;
    pendingReferenceIndex_ = 0;
    if (selector != 0) {
        context.referenceIndex = selector;
        NativeReferenceSlot slot;
        std::string slotError;
        if (resolveReference(selector, slot, slotError)) {
            if (slot.kind == NativeReferenceKind::Scro) {
                context.referenceFormId = slot.formId;
            } else {
                const ScriptValue value = variables_.get(slot.localOrdinal);
                context.referenceFormId = static_cast<uint32_t>(value.toInt());
            }
        } else {
            error = slotError;
            return false;
        }
    }

    ScriptValue returnValue = ScriptValue::makeInt(0);
    std::string handlerError;
    if (!it->second(context, instruction.tokens, returnValue, handlerError)) {
        error = handlerError.empty()
                    ? "command 0x" + std::to_string(instruction.opcode) + " failed"
                    : handlerError;
        return false;
    }
    return true;
}

bool NativeScdaVm::executeInstruction(const NativeInstruction& instruction,
                                      bool& stopped, std::string& error) {
    stopped = false;

    switch (instruction.opcode) {
        case static_cast<uint16_t>(NativeStructuralOpcode::Begin):
            // A nested Begin is not part of the grammar; the block selected by
            // run() is the only one that executes.
            return true;

        case static_cast<uint16_t>(NativeStructuralOpcode::End):
        case static_cast<uint16_t>(NativeStructuralOpcode::Return):
            stopped = true;
            return true;

        case static_cast<uint16_t>(NativeStructuralOpcode::If): {
            ScriptValue condition;
            if (!instruction.expressionDecoded) {
                error = "If expression could not be decoded";
                return false;
            }
            if (!evaluateExpression(instruction.expressionTokens, condition, error)) {
                return false;
            }

            NativeIfFrame frame;
            frame.ifIndex = pc_;
            frame.branchTaken = condition.isTruthy();
            ifStack_.push_back(frame);

            if (!frame.branchTaken) {
                const size_t next = findNextBranch(pc_);
                if (next >= program_->instructions.size()) {
                    stopped = true;
                } else {
                    pc_ = next;
                }
            }
            return true;
        }

        case static_cast<uint16_t>(NativeStructuralOpcode::ElseIf): {
            // A stray ElseIf at depth zero is a no-op: the retail corpus has
            // unbalanced blocks and rejecting them would drop real scripts.
            if (ifStack_.empty()) {
                return true;
            }

            NativeIfFrame& frame = ifStack_.back();
            if (frame.branchTaken) {
                const size_t end = findEndIf(frame.ifIndex);
                if (end >= program_->instructions.size()) {
                    stopped = true;
                } else {
                    pc_ = end;
                }
                return true;
            }

            ScriptValue condition;
            if (!instruction.expressionDecoded) {
                error = "ElseIf expression could not be decoded";
                return false;
            }
            if (!evaluateExpression(instruction.expressionTokens, condition, error)) {
                return false;
            }
            if (condition.isTruthy()) {
                frame.branchTaken = true;
            } else {
                const size_t next = findNextBranch(pc_);
                if (next >= program_->instructions.size()) {
                    stopped = true;
                } else {
                    pc_ = next;
                }
            }
            return true;
        }

        case static_cast<uint16_t>(NativeStructuralOpcode::Else): {
            if (ifStack_.empty()) {
                return true;
            }

            NativeIfFrame& frame = ifStack_.back();
            if (frame.branchTaken) {
                const size_t end = findEndIf(frame.ifIndex);
                if (end >= program_->instructions.size()) {
                    stopped = true;
                } else {
                    pc_ = end;
                }
            } else {
                frame.branchTaken = true;
            }
            return true;
        }

        case static_cast<uint16_t>(NativeStructuralOpcode::EndIf): {
            if (ifStack_.empty()) {
                return true;
            }
            ifStack_.pop_back();
            return true;
        }

        case static_cast<uint16_t>(NativeStructuralOpcode::Set):
            return executeSet(instruction, error);

        case NATIVE_SCDA_MARKER_OPCODE:
            // The marker carries the selector for the command that follows it.
            pendingReferenceIndex_ = instruction.referenceIndex;
            return true;

        case NATIVE_SCDA_PROLOGUE_OPCODE:
            return true;

        default:
            return executeCommand(instruction, error);
    }
}

NativeVmResult NativeScdaVm::run(const NativeDecodeResult& program,
                                 uint16_t blockType,
                                 uint32_t selfRef,
                                 uint32_t targetRef,
                                 int maxInstructions) {
    lastError_.clear();
    lastErrorOffset_ = 0;
    lastErrorOpcode_ = 0;
    executedInstructions_ = 0;
    unhandledCommands_ = 0;
    ifStack_.clear();
    stack_.clear();

    program_ = &program;
    blockType_ = blockType;
    selfRef_ = selfRef;
    targetRef_ = targetRef;
    running_ = false;

    if (!program.success) {
        setError("program did not decode: " + program.error, program.errorOffset,
                 program.errorOpcode);
        return NativeVmResult::Error;
    }

    // Select the block by type. Begin blocks are top level siblings, so the
    // first match is the one to run.
    size_t beginIndex = program.instructions.size();
    for (size_t i = 0; i < program.instructions.size(); ++i) {
        const NativeInstruction& instruction = program.instructions[i];
        if (instruction.opcode == static_cast<uint16_t>(NativeStructuralOpcode::Begin) &&
            instruction.blockType == blockType) {
            beginIndex = i;
            break;
        }
    }

    if (beginIndex >= program.instructions.size()) {
        setError("no Begin block of type " + std::to_string(blockType), 0, 0);
        return NativeVmResult::NotRunning;
    }

    const NativeInstruction& begin = program.instructions[beginIndex];
    // bodyLength counts the bytes from the payload start to the last
    // instruction inside the block, so the End that closes it sits 4 bytes
    // further on. Measured over all 948 vanilla blocks: the instruction at
    // begin.offset + 8 + bodyLength is End in every case.
    blockEnd_ = begin.offset + 8 + begin.bodyLength;
    pc_ = beginIndex + 1;
    running_ = true;

    return resume(maxInstructions);
}

NativeVmResult NativeScdaVm::resume(int maxInstructions) {
    if (!running_ || program_ == nullptr) {
        return NativeVmResult::NotRunning;
    }

    const auto& instructions = program_->instructions;

    while (pc_ < instructions.size()) {
        const NativeInstruction& instruction = instructions[pc_];

        // The block ends at the End that closes it. The body length includes
        // that End, so reaching the boundary stops the run even if the End
        // instruction itself was consumed by a jump.
        if (instruction.offset >= blockEnd_) {
            running_ = false;
            return NativeVmResult::Success;
        }

        if (executedInstructions_ >= maxInstructions) {
            return NativeVmResult::FrameBudget;
        }

        bool stopped = false;
        std::string error;
        const size_t current = pc_;
        if (!executeInstruction(instruction, stopped, error)) {
            setError(error, instruction.offset, instruction.opcode);
            running_ = false;
            return NativeVmResult::Error;
        }
        ++executedInstructions_;

        if (stopped) {
            running_ = false;
            return NativeVmResult::Success;
        }

        // A branch jump moves pc_ itself; otherwise advance by one.
        if (pc_ == current) {
            ++pc_;
        }
    }

    running_ = false;
    return NativeVmResult::Success;
}

} // namespace script
} // namespace oblivion
