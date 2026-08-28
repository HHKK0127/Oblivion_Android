// Phase 38: Script VM Unit Tests Implementation
// Tests the Oblivion Script VM bytecode interpreter and game functions

#include "script_vm_tests.h"

// Script system
#include "../script/script_vm.h"
#include "../script/script_context.h"
#include "../script/script_functions.h"
#include "../script/script_manager.h"
#include "../script/script_opcodes.h"

#include <chrono>
#include <cstring>
#include <cmath>

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "ScriptVMTest"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <cstdio>
#define TEST_LOGI(...) printf(__VA_ARGS__)
#define TEST_LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

using namespace oblivion::script;

// ============================================
// Helper: High-resolution timer
// ============================================
static float getTimeMs38() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(now - start).count();
}

// ============================================
// Constructor / Destructor
// ============================================
ScriptVMTests::ScriptVMTests() {}
ScriptVMTests::~ScriptVMTests() {}

// ============================================
// Record a test result
// ============================================
void ScriptVMTests::record(const std::string& name, bool passed,
                           const std::string& msg, float ms) {
    ScriptVMTestResult r;
    r.testName = name;
    r.passed = passed;
    r.message = msg;
    r.durationMs = ms;
    results.push_back(r);

    if (passed) {
        TEST_LOGI("[PASS] %s (%.2f ms) %s", name.c_str(), ms, msg.c_str());
    } else {
        TEST_LOGE("[FAIL] %s (%.2f ms) %s", name.c_str(), ms, msg.c_str());
    }
}

// ============================================
// Get pass/fail counts
// ============================================
int ScriptVMTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) if (r.passed) count++;
    return count;
}

int ScriptVMTests::getFailCount() const {
    int count = 0;
    for (const auto& r : results) if (!r.passed) count++;
    return count;
}

// ============================================
// Test ExecutionContext
// ============================================
void ScriptVMTests::testExecutionContext() {
    TEST_LOGI("--- Testing ExecutionContext ---");

    // Test 1: Stack operations
    {
        float start = getTimeMs38();
        ExecutionContext ctx;
        bool ok = true;

        // Push and pop
        ctx.pushStack(ScriptValue::makeInt(42));
        ctx.pushStack(ScriptValue::makeFloat(3.14f));

        ScriptValue top = ctx.popStack();
        ok = ok && (top.type == ScriptValue::Type::Float);
        ok = ok && (std::abs(top.floatVal - 3.14f) < 0.001f);

        ScriptValue second = ctx.popStack();
        ok = ok && (second.type == ScriptValue::Type::Integer);
        ok = ok && (second.intVal == 42);

        ok = ok && ctx.isStackEmpty();

        record("ExecutionContext: Stack push/pop", ok,
               "Push/pop int and float values", getTimeMs38() - start);
    }

    // Test 2: Stack overflow protection
    {
        float start = getTimeMs38();
        ExecutionContext ctx;
        bool ok = true;

        // Fill stack to max
        for (int i = 0; i < limits::MAX_STACK_SIZE; i++) {
            ctx.pushStack(ScriptValue::makeInt(i));
        }

        // Next push should fail
        bool overflow = !ctx.pushStack(ScriptValue::makeInt(999));
        ok = ok && overflow;
        ok = ok && (ctx.stackSize() == limits::MAX_STACK_SIZE);

        record("ExecutionContext: Stack overflow protection", ok,
               "Max stack size enforced", getTimeMs38() - start);
    }

    // Test 3: Local variables
    {
        float start = getTimeMs38();
        ExecutionContext ctx;
        bool ok = true;

        ctx.setLocal(0, ScriptValue::makeInt(100));
        ctx.setLocal(1, ScriptValue::makeFloat(2.5f));
        ctx.setLocal(2, ScriptValue::makeString("hello"));

        ScriptValue v0 = ctx.getLocal(0);
        ScriptValue v1 = ctx.getLocal(1);
        ScriptValue v2 = ctx.getLocal(2);

        ok = ok && (v0.intVal == 100);
        ok = ok && (std::abs(v1.floatVal - 2.5f) < 0.001f);
        ok = ok && (v2.strVal == "hello");

        record("ExecutionContext: Local variables", ok,
               "Set/get int, float, string locals", getTimeMs38() - start);
    }

    // Test 4: Reference table
    {
        float start = getTimeMs38();
        ExecutionContext ctx;
        bool ok = true;

        ctx.addReference(0, 0x12345678);
        ctx.addReference(1, 0xABCDEF00);

        ok = ok && (ctx.getReference(0) == 0x12345678);
        ok = ok && (ctx.getReference(1) == 0xABCDEF00);

        ctx.setSelfRef(0x11111111);
        ctx.setTargetRef(0x22222222);

        ok = ok && (ctx.getSelfRef() == 0x11111111);
        ok = ok && (ctx.getTargetRef() == 0x22222222);

        record("ExecutionContext: Reference table", ok,
               "Add/get references, self/target refs", getTimeMs38() - start);
    }

    // Test 5: Program counter
    {
        float start = getTimeMs38();
        ExecutionContext ctx;
        bool ok = true;

        ctx.setPC(0);
        ok = ok && (ctx.getPC() == 0);

        ctx.advancePC(10);
        ok = ok && (ctx.getPC() == 10);

        ctx.advancePC(5);
        ok = ok && (ctx.getPC() == 15);

        record("ExecutionContext: Program counter", ok,
               "Set/advance PC", getTimeMs38() - start);
    }
}

// ============================================
// Test ScriptVM
// ============================================
void ScriptVMTests::testScriptVM() {
    TEST_LOGI("--- Testing ScriptVM ---");

    // Test 1: Basic execution - STOP opcode
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Create minimal bytecode: just STOP
        ScriptData script;
        script.bytecode = {0x00, 0x00}; // STOP opcode (little-endian)
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);

        record("ScriptVM: STOP opcode", ok,
               "Execute STOP terminates script", getTimeMs38() - start);
    }

    // Test 2: PUSH_INT + STOP
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_INT 42, STOP
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, // PUSH_INT opcode
            0x04, 0x00, // arg length = 4
            0x2A, 0x00, 0x00, 0x00, // 42 in little-endian
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);
        ok = ok && !ctx.isStackEmpty();

        ScriptValue val = ctx.popStack();
        ok = ok && (val.type == ScriptValue::Type::Integer);
        ok = ok && (val.intVal == 42);

        record("ScriptVM: PUSH_INT + STOP", ok,
               "Push integer and verify stack", getTimeMs38() - start);
    }

    // Test 3: Arithmetic - ADD
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_INT 10, PUSH_INT 20, ADD, STOP
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 10
            0x0A, 0x00, 0x00, 0x00,
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 20
            0x14, 0x00, 0x00, 0x00,
            0x01, 0x00, // ADD
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);

        ScriptValue val = ctx.popStack();
        ok = ok && (val.intVal == 30);

        record("ScriptVM: ADD opcode", ok,
               "10 + 20 = 30", getTimeMs38() - start);
    }

    // Test 4: Comparison - CMP_LT
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_INT 5, PUSH_INT 10, CMP_LT, STOP
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 5
            0x05, 0x00, 0x00, 0x00,
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 10
            0x0A, 0x00, 0x00, 0x00,
            0x07, 0x00, // CMP_LT
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);

        ScriptValue val = ctx.popStack();
        ok = ok && (val.intVal == 1); // 5 < 10 = true

        record("ScriptVM: CMP_LT opcode", ok,
               "5 < 10 = true", getTimeMs38() - start);
    }

    // Test 5: Frame budget
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Create a loop that exceeds frame budget
        // PUSH_INT 0, PUSH_INT 1, ADD, POP, JUMP back to start
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 0 (counter)
            0x00, 0x00, 0x00, 0x00,
            // Loop start (PC = 8):
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 1
            0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, // ADD
            0x16, 0x00, // POP
            0x10, 0x00, // JUMP
            0x04, 0x00, // arg length = 4
            0x08, 0x00, 0x00, 0x00, // jump to PC = 8
            0x00, 0x00  // STOP (never reached)
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx, 100); // Small budget
        ok = ok && (result == VMResult::FrameBudget);

        record("ScriptVM: Frame budget", ok,
               "Exceeds instruction limit", getTimeMs38() - start);
    }
}

// ============================================
// Test Opcodes
// ============================================
void ScriptVMTests::testOpcodes() {
    TEST_LOGI("--- Testing Opcodes ---");

    // Test 1: PUSH_FLOAT
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_FLOAT 3.14, STOP
        ScriptData script;
        // 3.14f in little-endian = 0x4048F5C3
        script.bytecode = {
            0x13, 0x00, // PUSH_FLOAT
            0x04, 0x00, // arg length = 4
            0xC3, 0xF5, 0x48, 0x40, // 3.14f
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);

        ScriptValue val = ctx.popStack();
        ok = ok && (val.type == ScriptValue::Type::Float);
        ok = ok && (std::abs(val.floatVal - 3.14f) < 0.01f);

        record("Opcode: PUSH_FLOAT", ok,
               "Push float 3.14", getTimeMs38() - start);
    }

    // Test 2: NEG
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_INT 42, NEG, STOP
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 42
            0x2A, 0x00, 0x00, 0x00,
            0x06, 0x00, // NEG
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);

        ScriptValue val = ctx.popStack();
        ok = ok && (val.intVal == -42);

        record("Opcode: NEG", ok,
               "-42", getTimeMs38() - start);
    }

    // Test 3: AND/OR/NOT
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_INT 1, PUSH_INT 0, AND, STOP
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 1
            0x01, 0x00, 0x00, 0x00,
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 0
            0x00, 0x00, 0x00, 0x00,
            0x0D, 0x00, // AND
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);

        ScriptValue val = ctx.popStack();
        ok = ok && (val.intVal == 0); // 1 AND 0 = 0

        record("Opcode: AND", ok,
               "1 AND 0 = 0", getTimeMs38() - start);
    }

    // Test 4: DUP
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_INT 99, DUP, STOP
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 99
            0x63, 0x00, 0x00, 0x00,
            0x17, 0x00, // DUP
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);
        ok = ok && (ctx.stackSize() == 2);

        ScriptValue v1 = ctx.popStack();
        ScriptValue v2 = ctx.popStack();
        ok = ok && (v1.intVal == 99);
        ok = ok && (v2.intVal == 99);

        record("Opcode: DUP", ok,
               "Duplicate stack top", getTimeMs38() - start);
    }

    // Test 5: JUMP_Z
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        bool ok = true;

        // Bytecode: PUSH_INT 0, JUMP_Z to STOP, PUSH_INT 99 (skipped), STOP
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 0
            0x00, 0x00, 0x00, 0x00,
            0x11, 0x00, // JUMP_Z
            0x04, 0x00, // arg length = 4
            0x12, 0x00, 0x00, 0x00, // jump to PC 18 (STOP)
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 99 (skipped)
            0x63, 0x00, 0x00, 0x00,
            0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);
        ok = ok && ctx.isStackEmpty(); // 99 was skipped

        record("Opcode: JUMP_Z", ok,
               "Jump when zero", getTimeMs38() - start);
    }
}

// ============================================
// Test ScriptFunctions
// ============================================
void ScriptVMTests::testScriptFunctions() {
    TEST_LOGI("--- Testing ScriptFunctions ---");

    // Test 1: Function registration
    {
        float start = getTimeMs38();
        ScriptFunctions funcs;
        bool ok = true;

        funcs.init(nullptr, nullptr, nullptr, nullptr);

        ok = ok && funcs.hasFunction(FunctionID::SetStage);
        ok = ok && funcs.hasFunction(FunctionID::GetStage);
        ok = ok && funcs.hasFunction(FunctionID::AddItem);
        ok = ok && funcs.hasFunction(FunctionID::Enable);
        ok = ok && funcs.hasFunction(FunctionID::Disable);
        ok = ok && funcs.hasFunction(FunctionID::GetPlayer);

        record("ScriptFunctions: Registration", ok,
               "Tier 1+2 functions registered", getTimeMs38() - start);
    }

    // Test 2: Function name lookup
    {
        float start = getTimeMs38();
        ScriptFunctions funcs;
        bool ok = true;

        ok = ok && (funcs.getFunctionName(FunctionID::SetStage) == "SetStage");
        ok = ok && (funcs.getFunctionName(FunctionID::GetPlayer) == "GetPlayer");
        ok = ok && (funcs.getFunctionName(FunctionID::AddItem) == "AddItem");
        ok = ok && (funcs.getFunctionName(FunctionID::IsDead) == "IsDead");

        record("ScriptFunctions: Name lookup", ok,
               "FunctionID to name conversion", getTimeMs38() - start);
    }

    // Test 3: Unknown function handling
    {
        float start = getTimeMs38();
        ScriptFunctions funcs;
        bool ok = true;

        ok = ok && !funcs.hasFunction(static_cast<FunctionID>(0xFFFF));

        record("ScriptFunctions: Unknown function", ok,
               "Unknown FunctionID returns false", getTimeMs38() - start);
    }
}

// ============================================
// Test ScriptManager
// ============================================
void ScriptVMTests::testScriptManager() {
    TEST_LOGI("--- Testing ScriptManager ---");

    // Test 1: Manager initialization
    {
        float start = getTimeMs38();
        ScriptManager mgr;
        bool ok = true;

        mgr.init(nullptr, nullptr, nullptr, nullptr);

        // Should not crash
        mgr.update(0.016f);

        record("ScriptManager: Initialization", ok,
               "Init and update without crash", getTimeMs38() - start);
    }

    // Test 2: Script loading
    {
        float start = getTimeMs38();
        ScriptManager mgr;
        bool ok = true;

        mgr.init(nullptr, nullptr, nullptr, nullptr);

        ScriptData script;
        script.formID = 0x12345678;
        script.editorID = "TestScript";
        script.scriptType = ScriptType::Object;
        script.bytecode = {0x00, 0x00}; // STOP

        mgr.addScript(script);

        // Script should be loaded but not running
        ok = ok && !mgr.isScriptRunning(0x12345678, 0);

        record("ScriptManager: Script loading", ok,
               "Load script by FormID", getTimeMs38() - start);
    }

    // Test 3: Global variables
    {
        float start = getTimeMs38();
        ScriptManager mgr;
        bool ok = true;

        mgr.init(nullptr, nullptr, nullptr, nullptr);

        mgr.setGlobalVariable(0x100, ScriptValue::makeInt(42));
        mgr.setGlobalVariable(0x101, ScriptValue::makeFloat(3.14f));

        ScriptValue v0 = mgr.getGlobalVariable(0x100);
        ScriptValue v1 = mgr.getGlobalVariable(0x101);

        ok = ok && (v0.intVal == 42);
        ok = ok && (std::abs(v1.floatVal - 3.14f) < 0.001f);

        record("ScriptManager: Global variables", ok,
               "Set/get global variables", getTimeMs38() - start);
    }

    // Test 4: Script stop
    {
        float start = getTimeMs38();
        ScriptManager mgr;
        bool ok = true;

        mgr.init(nullptr, nullptr, nullptr, nullptr);

        ScriptData script;
        script.formID = 0xAAAAAAAA;
        script.editorID = "StopTest";
        script.scriptType = ScriptType::Object;
        script.bytecode = {0x00, 0x00};

        mgr.addScript(script);
        mgr.stopScript(0xAAAAAAAA, 0);

        ok = ok && !mgr.isScriptRunning(0xAAAAAAAA, 0);

        record("ScriptManager: Script stop", ok,
               "Stop script by FormID", getTimeMs38() - start);
    }
}

// ============================================
// Run all tests
// ============================================
bool ScriptVMTests::runAllTests() {
    results.clear();

    TEST_LOGI("========================================");
    TEST_LOGI("Phase 38: Script VM Unit Tests");
    TEST_LOGI("========================================");

    testExecutionContext();
    testScriptVM();
    testOpcodes();
    testScriptFunctions();
    testScriptManager();

    TEST_LOGI("========================================");
    TEST_LOGI("Results: %d passed, %d failed, %zu total",
              getPassCount(), getFailCount(), results.size());
    TEST_LOGI("========================================");

    return getFailCount() == 0;
}
