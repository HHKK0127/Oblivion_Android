#include "script_vm.h"
#include <cstring>
#include <cmath>

// ============================================================================
// Oblivion Script VM - Bytecode Execution Engine Implementation
// ============================================================================

namespace oblivion {
namespace script {

ScriptVM::ScriptVM() = default;

// ============================================================================
// Main execution loop
// ============================================================================

VMResult ScriptVM::execute(ExecutionContext& ctx, int maxInstructions) {
    if (!ctx.isRunning()) {
        return VMResult::NotRunning;
    }

    ctx.resetInstructionCount();

    while (ctx.isRunning() && ctx.getInstructionCount() < maxInstructions) {
        VMResult result = step(ctx);
        if (result != VMResult::Success) {
            return result;
        }
    }

    if (ctx.getInstructionCount() >= maxInstructions) {
        return VMResult::FrameBudget;
    }

    return VMResult::Success;
}

// ============================================================================
// Single instruction step
// ============================================================================

VMResult ScriptVM::step(ExecutionContext& ctx) {
    if (!ctx.isRunning()) {
        return VMResult::NotRunning;
    }

    // Bounds check
    if (ctx.getPC() >= ctx.getBytecodeSize()) {
        ctx.stop();
        return VMResult::Success;
    }

    ctx.incrementInstructionCount();

    // Decode instruction
    Instruction inst;
    if (!decodeInstruction(ctx, inst)) {
        setError("Failed to decode instruction at PC=" + std::to_string(ctx.getPC()));
        ctx.stop();
        return VMResult::Error;
    }

    // Advance PC past instruction header (4 bytes: opcode + argLength)
    uint32_t instSize = 4 + inst.argLength;

    // Execute based on opcode category
    Opcode op = inst.opcode;
    VMResult result = VMResult::Success;

    if (op == Opcode::STOP) {
        ctx.stop();
        return VMResult::Success;
    } else if (op >= Opcode::ADD && op <= Opcode::NEG) {
        result = executeArithmetic(ctx, op);
    } else if (op >= Opcode::CMP_LT && op <= Opcode::CMP_NE) {
        result = executeComparison(ctx, op);
    } else if (op >= Opcode::AND && op <= Opcode::NOT) {
        result = executeLogical(ctx, op);
    } else if (op == Opcode::JUMP || op == Opcode::JUMP_Z) {
        // Control flow handles its own PC advancement
        result = executeControlFlow(ctx, op, inst);
        return result;  // Don't advance PC again
    } else if (op >= Opcode::PUSH_INT && op <= Opcode::DUP) {
        result = executeStackOp(ctx, op, inst);
    } else if (op >= Opcode::GET_LOCAL && op <= Opcode::SET_GLOBAL) {
        result = executeVariableOp(ctx, op, inst);
    } else if (op >= Opcode::STR_CAT && op <= Opcode::STR_SUB) {
        result = executeStringOp(ctx, op);
    } else if (op >= Opcode::GET_SELF && op <= Opcode::GET_TARGET) {
        result = executeRefOp(ctx, op);
    } else if (op == Opcode::CALL) {
        result = executeCall(ctx, inst);
    } else {
        setError("Unknown opcode 0x" + std::to_string(static_cast<uint16_t>(op)) +
                 " at PC=" + std::to_string(ctx.getPC()));
        ctx.stop();
        return VMResult::Error;
    }

    // Advance PC past this instruction
    ctx.advancePC(instSize);

    return result;
}

// ============================================================================
// Instruction decoding
// ============================================================================

bool ScriptVM::decodeInstruction(const ExecutionContext& ctx, Instruction& inst) {
    const uint8_t* pc = ctx.getBytecode() + ctx.getPC();
    size_t remaining = ctx.getBytecodeSize() - ctx.getPC();

    if (remaining < 4) {
        return false;
    }

    // Read opcode (2 bytes, little-endian)
    uint16_t rawOpcode;
    std::memcpy(&rawOpcode, pc, 2);
    inst.opcode = static_cast<Opcode>(rawOpcode);

    // Read argument length (2 bytes, little-endian)
    std::memcpy(&inst.argLength, pc + 2, 2);

    // Validate argument data is within bounds
    if (remaining < 4 + inst.argLength) {
        return false;
    }

    inst.argData = (inst.argLength > 0) ? pc + 4 : nullptr;

    return true;
}

// ============================================================================
// Arithmetic operations
// ============================================================================

VMResult ScriptVM::executeArithmetic(ExecutionContext& ctx, Opcode op) {
    if (op == Opcode::NEG) {
        if (ctx.isStackEmpty()) {
            setError("NEG: stack underflow");
            ctx.stop();
            return VMResult::Error;
        }
        ScriptValue a = ctx.popStack();
        if (a.type == ScriptValue::Type::Float) {
            ctx.pushStack(ScriptValue::makeFloat(-a.floatVal));
        } else {
            ctx.pushStack(ScriptValue::makeInt(-a.intVal));
        }
        return VMResult::Success;
    }

    if (ctx.stackSize() < 2) {
        setError("Arithmetic: stack underflow");
        ctx.stop();
        return VMResult::Error;
    }

    ScriptValue b = ctx.popStack();
    ScriptValue a = ctx.popStack();

    // Promote to float if either operand is float
    bool useFloat = (a.type == ScriptValue::Type::Float || b.type == ScriptValue::Type::Float);

    switch (op) {
        case Opcode::ADD:
            if (useFloat) {
                ctx.pushStack(ScriptValue::makeFloat(a.toFloat() + b.toFloat()));
            } else {
                ctx.pushStack(ScriptValue::makeInt(a.intVal + b.intVal));
            }
            break;
        case Opcode::SUB:
            if (useFloat) {
                ctx.pushStack(ScriptValue::makeFloat(a.toFloat() - b.toFloat()));
            } else {
                ctx.pushStack(ScriptValue::makeInt(a.intVal - b.intVal));
            }
            break;
        case Opcode::MUL:
            if (useFloat) {
                ctx.pushStack(ScriptValue::makeFloat(a.toFloat() * b.toFloat()));
            } else {
                ctx.pushStack(ScriptValue::makeInt(a.intVal * b.intVal));
            }
            break;
        case Opcode::DIV: {
            float divisor = b.toFloat();
            if (divisor == 0.0f) {
                setError("Division by zero");
                ctx.stop();
                return VMResult::Error;
            }
            if (useFloat) {
                ctx.pushStack(ScriptValue::makeFloat(a.toFloat() / divisor));
            } else {
                if (b.intVal == 0) {
                    setError("Integer division by zero");
                    ctx.stop();
                    return VMResult::Error;
                }
                ctx.pushStack(ScriptValue::makeInt(a.intVal / b.intVal));
            }
            break;
        }
        case Opcode::MOD: {
            if (b.intVal == 0) {
                setError("Modulo by zero");
                ctx.stop();
                return VMResult::Error;
            }
            ctx.pushStack(ScriptValue::makeInt(a.intVal % b.intVal));
            break;
        }
        default:
            setError("Unknown arithmetic opcode");
            ctx.stop();
            return VMResult::Error;
    }

    return VMResult::Success;
}

// ============================================================================
// Comparison operations
// ============================================================================

VMResult ScriptVM::executeComparison(ExecutionContext& ctx, Opcode op) {
    if (ctx.stackSize() < 2) {
        setError("Comparison: stack underflow");
        ctx.stop();
        return VMResult::Error;
    }

    ScriptValue b = ctx.popStack();
    ScriptValue a = ctx.popStack();

    bool useFloat = (a.type == ScriptValue::Type::Float || b.type == ScriptValue::Type::Float);
    bool result = false;

    if (useFloat) {
        float fa = a.toFloat();
        float fb = b.toFloat();
        switch (op) {
            case Opcode::CMP_LT: result = fa < fb; break;
            case Opcode::CMP_LE: result = fa <= fb; break;
            case Opcode::CMP_GT: result = fa > fb; break;
            case Opcode::CMP_GE: result = fa >= fb; break;
            case Opcode::CMP_EQ: result = fa == fb; break;
            case Opcode::CMP_NE: result = fa != fb; break;
            default: break;
        }
    } else {
        int32_t ia = a.toInt();
        int32_t ib = b.toInt();
        switch (op) {
            case Opcode::CMP_LT: result = ia < ib; break;
            case Opcode::CMP_LE: result = ia <= ib; break;
            case Opcode::CMP_GT: result = ia > ib; break;
            case Opcode::CMP_GE: result = ia >= ib; break;
            case Opcode::CMP_EQ: result = ia == ib; break;
            case Opcode::CMP_NE: result = ia != ib; break;
            default: break;
        }
    }

    ctx.pushStack(ScriptValue::makeInt(result ? 1 : 0));
    return VMResult::Success;
}

// ============================================================================
// Logical operations
// ============================================================================

VMResult ScriptVM::executeLogical(ExecutionContext& ctx, Opcode op) {
    if (op == Opcode::NOT) {
        if (ctx.isStackEmpty()) {
            setError("NOT: stack underflow");
            ctx.stop();
            return VMResult::Error;
        }
        ScriptValue a = ctx.popStack();
        ctx.pushStack(ScriptValue::makeInt(a.isTruthy() ? 0 : 1));
        return VMResult::Success;
    }

    if (ctx.stackSize() < 2) {
        setError("Logical: stack underflow");
        ctx.stop();
        return VMResult::Error;
    }

    ScriptValue b = ctx.popStack();
    ScriptValue a = ctx.popStack();

    switch (op) {
        case Opcode::AND:
            ctx.pushStack(ScriptValue::makeInt((a.isTruthy() && b.isTruthy()) ? 1 : 0));
            break;
        case Opcode::OR:
            ctx.pushStack(ScriptValue::makeInt((a.isTruthy() || b.isTruthy()) ? 1 : 0));
            break;
        default:
            setError("Unknown logical opcode");
            ctx.stop();
            return VMResult::Error;
    }

    return VMResult::Success;
}

// ============================================================================
// Control flow
// ============================================================================

VMResult ScriptVM::executeControlFlow(ExecutionContext& ctx, Opcode op, const Instruction& inst) {
    if (inst.argLength < 4) {
        setError("JUMP: insufficient argument data");
        ctx.stop();
        return VMResult::Error;
    }

    uint32_t target = readArg<uint32_t>(inst.argData);

    switch (op) {
        case Opcode::JUMP:
            ctx.setPC(target);
            break;
        case Opcode::JUMP_Z: {
            if (ctx.isStackEmpty()) {
                setError("JUMP_Z: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            ScriptValue cond = ctx.popStack();
            if (!cond.isTruthy()) {
                ctx.setPC(target);
            } else {
                // Advance past this instruction normally
                ctx.advancePC(4 + inst.argLength);
            }
            break;
        }
        default:
            setError("Unknown control flow opcode");
            ctx.stop();
            return VMResult::Error;
    }

    return VMResult::Success;
}

// ============================================================================
// Stack operations
// ============================================================================

VMResult ScriptVM::executeStackOp(ExecutionContext& ctx, Opcode op, const Instruction& inst) {
    switch (op) {
        case Opcode::PUSH_INT: {
            if (inst.argLength < 4) {
                setError("PUSH_INT: insufficient argument data");
                ctx.stop();
                return VMResult::Error;
            }
            int32_t val = readArg<int32_t>(inst.argData);
            ctx.pushStack(ScriptValue::makeInt(val));
            break;
        }
        case Opcode::PUSH_FLOAT: {
            if (inst.argLength < 4) {
                setError("PUSH_FLOAT: insufficient argument data");
                ctx.stop();
                return VMResult::Error;
            }
            float val = readArg<float>(inst.argData);
            ctx.pushStack(ScriptValue::makeFloat(val));
            break;
        }
        case Opcode::PUSH_STRING: {
            if (!inst.argData || inst.argLength == 0) {
                setError("PUSH_STRING: no string data");
                ctx.stop();
                return VMResult::Error;
            }
            // Null-terminated string in argument data
            std::string val(reinterpret_cast<const char*>(inst.argData), inst.argLength);
            // Remove trailing null if present
            if (!val.empty() && val.back() == '\0') {
                val.pop_back();
            }
            ctx.pushStack(ScriptValue::makeString(val));
            break;
        }
        case Opcode::PUSH_REF: {
            if (inst.argLength < 4) {
                setError("PUSH_REF: insufficient argument data");
                ctx.stop();
                return VMResult::Error;
            }
            uint32_t formID = readArg<uint32_t>(inst.argData);
            ctx.pushStack(ScriptValue::makeRef(formID));
            break;
        }
        case Opcode::POP: {
            if (ctx.isStackEmpty()) {
                setError("POP: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            ctx.popStack();
            break;
        }
        case Opcode::DUP: {
            if (ctx.isStackEmpty()) {
                setError("DUP: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            ScriptValue top = ctx.peekStack();
            ctx.pushStack(top);
            break;
        }
        default:
            setError("Unknown stack opcode");
            ctx.stop();
            return VMResult::Error;
    }

    return VMResult::Success;
}

// ============================================================================
// Variable operations
// ============================================================================

VMResult ScriptVM::executeVariableOp(ExecutionContext& ctx, Opcode op, const Instruction& inst) {
    switch (op) {
        case Opcode::GET_LOCAL: {
            if (inst.argLength < 2) {
                setError("GET_LOCAL: insufficient argument data");
                ctx.stop();
                return VMResult::Error;
            }
            uint16_t index = readArg<uint16_t>(inst.argData);
            ScriptValue val = ctx.getLocal(index);
            ctx.pushStack(val);
            break;
        }
        case Opcode::SET_LOCAL: {
            if (inst.argLength < 2) {
                setError("SET_LOCAL: insufficient argument data");
                ctx.stop();
                return VMResult::Error;
            }
            if (ctx.isStackEmpty()) {
                setError("SET_LOCAL: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            uint16_t index = readArg<uint16_t>(inst.argData);
            ScriptValue val = ctx.popStack();
            ctx.setLocal(index, val);
            break;
        }
        case Opcode::GET_GLOBAL: {
            if (inst.argLength < 4) {
                setError("GET_GLOBAL: insufficient argument data");
                ctx.stop();
                return VMResult::Error;
            }
            uint32_t formID = readArg<uint32_t>(inst.argData);
            // Global variables are stored as references
            uint32_t val = ctx.getReference(formID);
            ctx.pushStack(ScriptValue::makeInt(static_cast<int32_t>(val)));
            break;
        }
        case Opcode::SET_GLOBAL: {
            if (inst.argLength < 4) {
                setError("SET_GLOBAL: insufficient argument data");
                ctx.stop();
                return VMResult::Error;
            }
            if (ctx.isStackEmpty()) {
                setError("SET_GLOBAL: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            uint32_t formID = readArg<uint32_t>(inst.argData);
            ScriptValue val = ctx.popStack();
            ctx.addReference(formID, static_cast<uint32_t>(val.toInt()));
            break;
        }
        default:
            setError("Unknown variable opcode");
            ctx.stop();
            return VMResult::Error;
    }

    return VMResult::Success;
}

// ============================================================================
// String operations
// ============================================================================

VMResult ScriptVM::executeStringOp(ExecutionContext& ctx, Opcode op) {
    switch (op) {
        case Opcode::STR_CAT: {
            if (ctx.stackSize() < 2) {
                setError("STR_CAT: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            ScriptValue b = ctx.popStack();
            ScriptValue a = ctx.popStack();
            std::string result;
            if (a.type == ScriptValue::Type::String) result += a.strVal;
            else result += std::to_string(a.intVal);
            if (b.type == ScriptValue::Type::String) result += b.strVal;
            else result += std::to_string(b.intVal);
            ctx.pushStack(ScriptValue::makeString(result));
            break;
        }
        case Opcode::STR_LEN: {
            if (ctx.isStackEmpty()) {
                setError("STR_LEN: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            ScriptValue a = ctx.popStack();
            int32_t len = 0;
            if (a.type == ScriptValue::Type::String) {
                len = static_cast<int32_t>(a.strVal.length());
            }
            ctx.pushStack(ScriptValue::makeInt(len));
            break;
        }
        case Opcode::STR_SUB: {
            if (ctx.stackSize() < 3) {
                setError("STR_SUB: stack underflow");
                ctx.stop();
                return VMResult::Error;
            }
            ScriptValue len = ctx.popStack();
            ScriptValue pos = ctx.popStack();
            ScriptValue str = ctx.popStack();
            if (str.type == ScriptValue::Type::String) {
                int32_t p = pos.toInt();
                int32_t l = len.toInt();
                if (p >= 0 && p < static_cast<int32_t>(str.strVal.length())) {
                    ctx.pushStack(ScriptValue::makeString(str.strVal.substr(p, l)));
                } else {
                    ctx.pushStack(ScriptValue::makeString(""));
                }
            } else {
                ctx.pushStack(ScriptValue::makeString(""));
            }
            break;
        }
        default:
            setError("Unknown string opcode");
            ctx.stop();
            return VMResult::Error;
    }

    return VMResult::Success;
}

// ============================================================================
// Reference operations
// ============================================================================

VMResult ScriptVM::executeRefOp(ExecutionContext& ctx, Opcode op) {
    switch (op) {
        case Opcode::GET_SELF:
            ctx.pushStack(ScriptValue::makeRef(ctx.getSelfRef()));
            break;
        case Opcode::GET_TARGET:
            ctx.pushStack(ScriptValue::makeRef(ctx.getTargetRef()));
            break;
        default:
            setError("Unknown reference opcode");
            ctx.stop();
            return VMResult::Error;
    }

    return VMResult::Success;
}

// ============================================================================
// Function call
// ============================================================================

VMResult ScriptVM::executeCall(ExecutionContext& ctx, const Instruction& inst) {
    if (inst.argLength < 4) {
        setError("CALL: insufficient argument data (need funcID + argCount)");
        ctx.stop();
        return VMResult::Error;
    }

    // Read function ID (2 bytes) and argument count (2 bytes)
    uint16_t rawFuncID = readArg<uint16_t>(inst.argData);
    uint16_t argCount = readArg<uint16_t>(inst.argData + 2);

    FunctionID funcID = static_cast<FunctionID>(rawFuncID);

    // Pop arguments from stack
    std::vector<ScriptValue> args;
    args.reserve(argCount);
    for (uint16_t i = 0; i < argCount; ++i) {
        if (ctx.isStackEmpty()) {
            setError("CALL: stack underflow while popping arguments for function 0x" +
                     std::to_string(rawFuncID));
            ctx.stop();
            return VMResult::Error;
        }
        args.push_back(ctx.popStack());
    }
    // Arguments are popped in reverse order; reverse to get correct order
    std::reverse(args.begin(), args.end());

    // Execute function
    if (!functions_) {
        setError("CALL: no function registry set");
        ctx.stop();
        return VMResult::Error;
    }

    FunctionResult result = functions_->execute(funcID, ctx, args);

    if (!result.success) {
        SCRIPT_LOGW("Function 0x%04X failed: %s", rawFuncID, result.errorMessage.c_str());
        // Push zero as default return value on failure
        ctx.pushStack(ScriptValue::makeInt(0));
    } else {
        // Push return value if function produced one
        // Functions that return void still push a value (typically 0)
        ctx.pushStack(result.returnValue);
    }

    return VMResult::Success;
}

} // namespace script
} // namespace oblivion
