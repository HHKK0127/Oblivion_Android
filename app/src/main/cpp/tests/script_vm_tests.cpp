// Phase 38: Script VM Unit Tests Implementation
// Tests the Oblivion Script VM bytecode interpreter and game functions

#include "script_vm_tests.h"

// Script system
#include "../script/script_vm.h"
#include "../script/script_context.h"
#include "../script/script_disasm.h"
#include "../script/script_functions.h"
#include "../script/script_manager.h"
#include "../script/script_opcodes.h"
#include "../script/native_scda_decoder.h"
#include "../script/native_scda_vm.h"
#include "../script/native_scda_bridge.h"
#include "../quest/quest_flow_controller.h"
#include "../game/inventory_manager.h"
#include "../assets/esm_reader.h"

#include <chrono>
#include <cstring>
#include <cmath>
#include <sstream>

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
using namespace oblivion;

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

        // Locals use their serialized SLSD indices, not variable-table positions.
        ScriptData script;
        script.lastVarIndex = 6;
        for (uint32_t i : {0u, 2u, 5u, 6u}) {
            ScriptVariable var;
            var.index = i;
            var.type = i == 2 ? ScriptValue::Type::Float :
                       i == 5 ? ScriptValue::Type::String :
                       i == 6 ? ScriptValue::Type::Ref :
                                ScriptValue::Type::Integer;
            var.defaultValue = ScriptValue::makeInt(0);
            script.variables.push_back(var);
        }
        ctx.init(&script);

        ok = ok && ctx.getLocal(2).type == ScriptValue::Type::Float;
        ok = ok && ctx.getLocal(6).type == ScriptValue::Type::Ref;
        ctx.setLocal(0, ScriptValue::makeInt(100));
        ctx.setLocal(2, ScriptValue::makeFloat(2.5f));
        ctx.setLocal(5, ScriptValue::makeString("hello"));
        ctx.setLocal(6, ScriptValue::makeRef(0x01234567));

        ScriptValue v0 = ctx.getLocal(0);
        ScriptValue v2 = ctx.getLocal(2);
        ScriptValue v5 = ctx.getLocal(5);
        ScriptValue v6 = ctx.getLocal(6);

        ok = ok && (v0.intVal == 100);
        ok = ok && (std::abs(v2.floatVal - 2.5f) < 0.001f);
        ok = ok && (v5.strVal == "hello");
        ok = ok && (v6.refVal == 0x01234567);

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
        // Every instruction is opcode (2 bytes) + argument length (2 bytes)
        script.bytecode = {0x00, 0x00, 0x00, 0x00}; // STOP
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
            0x00, 0x00, 0x00, 0x00  // STOP
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
        // PUSH_INT 0, PUSH_INT 1, ADD, JUMP back to start
        // The counter must stay on the stack across iterations, so the loop
        // body must not pop it (ADD consumes two values and pushes one).
        ScriptData script;
        script.bytecode = {
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 0 (counter)
            0x00, 0x00, 0x00, 0x00,
            // Loop start (PC = 8):
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 1
            0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00, // ADD
            0x10, 0x00,             // JUMP
            0x04, 0x00,             // arg length = 4
            0x08, 0x00, 0x00, 0x00, // jump to PC = 8
            0x00, 0x00, 0x00, 0x00  // STOP (never reached)
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx, 100); // Small budget
        ok = ok && (result == VMResult::FrameBudget);

        record("ScriptVM: Frame budget", ok,
               "Exceeds instruction limit", getTimeMs38() - start);
    }

    // Test 6: Unknown opcode diagnostic
    {
        float start = getTimeMs38();
        ScriptVM vm;
        ExecutionContext ctx;
        ScriptData script;
        script.bytecode = {0x1D, 0x00, 0x00, 0x00};
        ctx.init(&script);
        ctx.setRunning(true);

        const VMResult result = vm.execute(ctx);
        const bool ok = result == VMResult::Error &&
                        vm.getLastError().find("Unknown opcode 0x1d") != std::string::npos;

        record("ScriptVM: Unknown opcode diagnostic", ok,
               "Unknown opcode values are reported in hexadecimal", getTimeMs38() - start);
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
            0x00, 0x00, 0x00, 0x00  // STOP
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
            0x11, 0x00,             // JUMP_Z
            0x04, 0x00,             // arg length = 4
            0x18, 0x00, 0x00, 0x00, // jump to PC 24 (STOP)
            0x12, 0x00, 0x04, 0x00, // PUSH_INT 99 (skipped)
            0x63, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00  // STOP
        };
        ctx.init(&script);
        ctx.setRunning(true);

        VMResult result = vm.execute(ctx);
        ok = ok && (result == VMResult::Success);
        ok = ok && ctx.isStackEmpty(); // 99 was skipped

        record("Opcode: JUMP_Z", ok,
               "Jump when zero", getTimeMs38() - start);
    }

    // Test 6: Raw SCDA marker instruction
    {
        float start = getTimeMs38();
        ScriptData script;
        script.bytecode = {
            0x1C, 0x00, 0xFF, 0xFF,
            0x11, 0x00, 0x00, 0x00
        };

        const std::string disassembly = ScriptDisasm::disassemble(script);
        const bool ok = disassembly.find("0000:") != std::string::npos &&
                        disassembly.find("0004:") != std::string::npos &&
                        disassembly.find("suspicious arg length") == std::string::npos;

        record("Opcode: SCDA marker length", ok,
               "Opcode 0x001C advances by its four-byte header", getTimeMs38() - start);
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
        ok = ok && funcs.hasFunction(FunctionID::StartQuest);
        ok = ok && funcs.hasFunction(FunctionID::CompleteQuest);
        ok = ok && funcs.hasFunction(FunctionID::SetObjectiveCompleted);
        ok = ok && funcs.hasFunction(FunctionID::GetObjectiveCompleted);
        ok = ok && funcs.hasFunction(FunctionID::IsQuestStageDone);
        ok = ok && funcs.hasFunction(FunctionID::GetQuestCompleted);
        ok = ok && funcs.hasFunction(FunctionID::GetQuestStarted);
        ok = ok && funcs.hasFunction(FunctionID::GetDead);
        ok = ok && funcs.hasFunction(FunctionID::GetStageDone);

        record("ScriptFunctions: Registration", ok,
               "Tier 1+2 functions registered", getTimeMs38() - start);
    }

    // Test 2: Function name lookup
    {
        float start = getTimeMs38();
        ScriptFunctions funcs;
        bool ok = true;

        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::SetStage), "SetStage") == 0);
        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::GetPlayer), "GetPlayer") == 0);
        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::AddItem), "AddItem") == 0);
        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::IsDead), "IsDead") == 0);
        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::StartQuest), "StartQuest") == 0);
        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::GetQuestStarted), "GetQuestStarted") == 0);
        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::GetDead), "GetDead") == 0);
        ok = ok && (std::strcmp(funcs.getFunctionName(FunctionID::GetStageDone), "GetStageDone") == 0);
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

    // Test 4: Quest flow functions
    {
        float start = getTimeMs38();
        QuestFlowController questFlowController;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        QuestRecord questRecord;
        questRecord.formID = 0x0100ABCE;
        questRecord.fullName = "Script VM Quest";
        questRecord.stages.push_back({10});
        questRecord.objectives.push_back({1, "Complete the test objective"});
        questFlowController.registerQuest(questRecord);
        funcs.init(nullptr, nullptr, nullptr, nullptr, &questFlowController);

        const std::vector<ScriptValue> questArgs = {ScriptValue::makeRef(questRecord.formID)};
        bool ok = true;

        FunctionResult startResult = funcs.execute(FunctionID::StartQuest, ctx, questArgs);
        FunctionResult startedResult = funcs.execute(FunctionID::GetQuestStarted, ctx, questArgs);
        FunctionResult stageResult = funcs.execute(
            FunctionID::IsQuestStageDone,
            ctx,
            {ScriptValue::makeRef(questRecord.formID), ScriptValue::makeInt(10)});
        FunctionResult getStageDoneResult = funcs.execute(
            FunctionID::GetStageDone,
            ctx,
            {ScriptValue::makeRef(questRecord.formID), ScriptValue::makeInt(10)});
        FunctionResult setObjectiveResult = funcs.execute(
            FunctionID::SetObjectiveCompleted,
            ctx,
            {ScriptValue::makeRef(questRecord.formID), ScriptValue::makeInt(1)});
        FunctionResult objectiveResult = funcs.execute(
            FunctionID::GetObjectiveCompleted,
            ctx,
            {ScriptValue::makeRef(questRecord.formID), ScriptValue::makeInt(1)});
        FunctionResult completeResult = funcs.execute(FunctionID::CompleteQuest, ctx, questArgs);
        FunctionResult completedResult = funcs.execute(FunctionID::GetQuestCompleted, ctx, questArgs);

        ok = ok && startResult.success && startResult.returnValue.intVal == 1;
        ok = ok && startedResult.success && startedResult.returnValue.intVal == 1;
        ok = ok && stageResult.success && stageResult.returnValue.intVal == 1;
        ok = ok && getStageDoneResult.success && getStageDoneResult.returnValue.intVal == 1;
        ok = ok && setObjectiveResult.success && setObjectiveResult.returnValue.intVal == 1;
        ok = ok && objectiveResult.success && objectiveResult.returnValue.intVal == 1;
        ok = ok && completeResult.success && completeResult.returnValue.intVal == 1;
        ok = ok && completedResult.success && completedResult.returnValue.intVal == 1;

        record("ScriptFunctions: Quest flow", ok,
               "Start, complete objective, and complete quest", getTimeMs38() - start);
    }

    // Test 5: Quest stage round trip
    {
        float start = getTimeMs38();
        QuestFlowController questFlowController;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        bool ok = true;

        funcs.init(nullptr, nullptr, nullptr, nullptr, &questFlowController);

        const uint32_t questFormID = 0x0100ABCD;
        FunctionResult setResult = funcs.execute(
            FunctionID::SetStage,
            ctx,
            {ScriptValue::makeRef(questFormID), ScriptValue::makeInt(20)});
        FunctionResult getResult = funcs.execute(
            FunctionID::GetStage,
            ctx,
            {ScriptValue::makeRef(questFormID)});
        FunctionResult repeatedSetResult = funcs.execute(
            FunctionID::SetStage,
            ctx,
            {ScriptValue::makeRef(questFormID), ScriptValue::makeInt(20)});

        ok = ok && setResult.success && setResult.returnValue.intVal == 1;
        ok = ok && getResult.success && getResult.returnValue.intVal == 20;
        ok = ok && !repeatedSetResult.success && repeatedSetResult.returnValue.intVal == 0;

        record("ScriptFunctions: SetStage/GetStage", ok,
               "Persist stages and reject duplicate transitions", getTimeMs38() - start);
    }

    // Test 6: Inventory function round trip and input validation
    {
        float start = getTimeMs38();
        InventoryManager inventory_manager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        bool ok = inventory_manager.initialize();

        funcs.init(nullptr, nullptr, nullptr, &inventory_manager);

        FunctionResult initial_count = funcs.execute(
            FunctionID::GetItemCount, ctx, {ScriptValue::makeRef(201)});
        FunctionResult add_result = funcs.execute(
            FunctionID::AddItem,
            ctx,
            {ScriptValue::makeRef(201), ScriptValue::makeInt(2)});
        FunctionResult added_count = funcs.execute(
            FunctionID::GetItemCount, ctx, {ScriptValue::makeRef(201)});
        FunctionResult remove_result = funcs.execute(
            FunctionID::RemoveItem,
            ctx,
            {ScriptValue::makeRef(201), ScriptValue::makeInt(2)});
        FunctionResult removed_count = funcs.execute(
            FunctionID::GetItemCount, ctx, {ScriptValue::makeRef(201)});
        FunctionResult invalid_count = funcs.execute(
            FunctionID::AddItem,
            ctx,
            {ScriptValue::makeRef(201), ScriptValue::makeInt(0)});
        FunctionResult missing_item = funcs.execute(
            FunctionID::AddItem,
            ctx,
            {ScriptValue::makeRef(0x0BADF00D), ScriptValue::makeInt(1)});

        ok = ok && initial_count.success && initial_count.returnValue.intVal == 5;
        ok = ok && add_result.success && add_result.returnValue.intVal == 1;
        ok = ok && added_count.success && added_count.returnValue.intVal == 7;
        ok = ok && remove_result.success && remove_result.returnValue.intVal == 1;
        ok = ok && removed_count.success && removed_count.returnValue.intVal == 5;
        ok = ok && !invalid_count.success && !missing_item.success;

        record("ScriptFunctions: Inventory", ok,
               "Add, remove, count, and reject invalid items", getTimeMs38() - start);
    }

    // Test 7: Actor functions reject unresolved references
    {
        float start = getTimeMs38();
        ScriptFunctions funcs;
        ExecutionContext ctx;
        NpcManager npc_manager;
        bool ok = true;

        auto npc = npc_manager.createNPC("Script Actor", glm::vec3(0.0f, 0.0f, 0.0f));
        auto target_npc = npc_manager.createNPC("Script Target", glm::vec3(2.0f, 3.0f, 4.0f));
        ok = ok && npc != nullptr;
        ok = ok && target_npc != nullptr;
        if (npc) {
            npc->status.currentHealth = 100.0f;
            npc->status.maxHealth = 100.0f;
            npc->inCombat = true;
            ctx.setSelfRef(npc->npcId);
        }
        funcs.init(nullptr, nullptr, &npc_manager, nullptr);

        FunctionResult health_result = funcs.execute(FunctionID::GetHealth, ctx, {});
        FunctionResult set_health_result = funcs.execute(
            FunctionID::SetHealth, ctx, {ScriptValue::makeFloat(50.0f)});
        FunctionResult dead_result = funcs.execute(FunctionID::IsDead, ctx, {});
        FunctionResult get_dead_result = funcs.execute(FunctionID::GetDead, ctx, {});
        FunctionResult combat_result = funcs.execute(FunctionID::IsInCombat, ctx, {});
        FunctionResult set_pos_result = funcs.execute(
            FunctionID::SetPos, ctx, {ScriptValue::makeInt(0), ScriptValue::makeFloat(2.0f)});
        FunctionResult get_pos_result = funcs.execute(
            FunctionID::GetPos, ctx, {ScriptValue::makeInt(0)});
        FunctionResult distance_result = funcs.execute(
            FunctionID::GetDistance, ctx, {ScriptValue::makeRef(target_npc->npcId)});
        FunctionResult move_result = funcs.execute(
            FunctionID::MoveTo, ctx, {ScriptValue::makeRef(target_npc->npcId)});

        ok = ok && health_result.success && health_result.returnValue.floatVal == 100.0f;
        ok = ok && set_health_result.success && npc->status.currentHealth == 50.0f;
        ok = ok && dead_result.success && dead_result.returnValue.intVal == 0;
        ok = ok && get_dead_result.success && get_dead_result.returnValue.intVal == 0;
        ok = ok && combat_result.success && combat_result.returnValue.intVal == 1;
        ok = ok && set_pos_result.success && get_pos_result.success;
        ok = ok && get_pos_result.returnValue.floatVal == 2.0f;
        ok = ok && distance_result.success && distance_result.returnValue.floatVal == 5.0f;
        ok = ok && move_result.success && npc->position.x == target_npc->position.x;
        ok = ok && npc->position.y == target_npc->position.y;
        ok = ok && npc->position.z == target_npc->position.z;

        ctx.setSelfRef(0x0BADF00D);
        FunctionResult unresolved_result = funcs.execute(FunctionID::GetHealth, ctx, {});
        ok = ok && !unresolved_result.success;

        record("ScriptFunctions: Actor resolution", ok,
               "Read and modify NPC state while rejecting unresolved references",
               getTimeMs38() - start);
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

    // Test 5: Inline script ownership and duplicate rejection
    {
        float start = getTimeMs38();
        ScriptManager mgr;
        mgr.init(nullptr, nullptr, nullptr, nullptr);

        ScriptData script;
        script.bytecode = {0x00, 0x00, 0x00, 0x00};
        const InlineScriptKey key{0x0100ABCD, 20, 0};

        const int firstStart = mgr.startInlineScript(script, key, 0x0100ABCD);
        const int duplicateStart = mgr.startInlineScript(script, key, 0x0100ABCD);
        const bool started = firstStart >= 0 && duplicateStart == -1 &&
                             mgr.getActiveScriptCount() == 1;

        mgr.update(0.0f);
        const bool ok = started && mgr.getActiveScriptCount() == 0;

        record("ScriptManager: Inline script", ok,
               "Own inline data and reject duplicate stage scripts", getTimeMs38() - start);
    }

    // Test 6: Quest stage indices use the on-disk u16 width.
    {
        float start = getTimeMs38();
        QuestStageEntry stage;
        const uint8_t rawStage[] = {0xC8, 0x00};
        const bool ok = QuestRecordParser::parseStageEntry(
                            rawStage, sizeof(rawStage), stage) &&
                        stage.stageIndex == 200;

        record("ScriptManager: Quest stage index", ok,
               "Parse u16 INDX stage indices", getTimeMs38() - start);
    }

    // Test 7: Run-once quest blocks do not run after returning to their stage.
    {
        float start = getTimeMs38();
        QuestManager questManager;
        ScriptManager scriptManager;
        QuestStageManager stageManager;
        scriptManager.init(&questManager, nullptr, nullptr, nullptr);
        stageManager.initialize(&questManager, &scriptManager, nullptr, nullptr);

        QuestRecord quest;
        quest.formID = 0x0100ABCD;
        QuestStageEntry stage;
        stage.stageIndex = 10;
        QuestStageBlock block;
        block.scriptIndex = 0;
        block.qsdtFlags = 0x01;
        block.script.bytecode = {0x00, 0x00, 0x00, 0x00};
        stage.blocks.push_back(block);
        QuestStageBlock repeatableBlock;
        repeatableBlock.scriptIndex = 1;
        repeatableBlock.script.bytecode = {0x00, 0x00, 0x00, 0x00};
        stage.blocks.push_back(repeatableBlock);
        quest.stages.push_back(stage);
        stageManager.registerQuest(quest);

        const bool firstTransition = stageManager.setStage(quest.formID, 10);
        const bool firstStarted = scriptManager.getActiveScriptCount() == 2;
        scriptManager.update(0.0f);
        const bool movedAway = stageManager.setStage(quest.formID, 20);
        const bool returned = stageManager.setStage(quest.formID, 10);

        const bool ok = firstTransition && firstStarted && movedAway && returned &&
                        scriptManager.getActiveScriptCount() == 1;
        record("ScriptManager: Run-once quest block", ok,
               "Suppress QSDT run-once blocks while repeating other blocks",
               getTimeMs38() - start);
    }

    // Test 8: QUST subrecords follow the on-disk layout, not the old
    // QSTN/QSTF/QSTR shape. INDX is a u16 stage, QSDT opens a block, CNAM is
    // block-level journal text, and CTDA belongs to the nearest preceding
    // owner (QSTA, QSDT or the quest-level run after DATA).
    {
        float start = getTimeMs38();

        auto appendSub = [](std::vector<uint8_t>& out, const char* type,
                            const std::vector<uint8_t>& body) {
            out.insert(out.end(), type, type + 4);
            const uint16_t size = static_cast<uint16_t>(body.size());
            out.push_back(static_cast<uint8_t>(size & 0xFF));
            out.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
            out.insert(out.end(), body.begin(), body.end());
        };
        auto ascii = [](const char* text) {
            return std::vector<uint8_t>(text, text + std::strlen(text));
        };

        std::vector<uint8_t> questBytes;
        appendSub(questBytes, "EDID", ascii("TestQuest"));
        appendSub(questBytes, "DATA", {0x01, 0x3C});
        // Quest-level condition: runs after DATA, before the first INDX.
        appendSub(questBytes, "CTDA", std::vector<uint8_t>(24, 0x00));
        // Stage 10 with one run-once block carrying bytecode and journal text.
        appendSub(questBytes, "INDX", {0x0A, 0x00});
        appendSub(questBytes, "QSDT", {0x01});
        appendSub(questBytes, "SCHR", std::vector<uint8_t>(20, 0x00));
        appendSub(questBytes, "SCDA", {0x00, 0x00, 0x00, 0x00});
        appendSub(questBytes, "CNAM", ascii("Stage ten journal"));
        appendSub(questBytes, "CTDA", std::vector<uint8_t>(24, 0x00));
        // Stage 20 with a repeatable block and no bytecode.
        appendSub(questBytes, "INDX", {0x14, 0x00});
        appendSub(questBytes, "QSDT", {0x00});
        appendSub(questBytes, "SCHR", std::vector<uint8_t>(20, 0x00));
        appendSub(questBytes, "CNAM", ascii("Stage twenty journal"));

        QuestRecord quest;
        const bool parsed =
            QuestRecordParser::parse(questBytes.data(), questBytes.size(), quest);

        const bool ok = parsed && quest.editorID == "TestQuest" &&
                        quest.questFlags == 0x01 && quest.priority == 0x3C &&
                        quest.conditions.size() == 1 && quest.stages.size() == 2 &&
                        quest.stages[0].stageIndex == 10 &&
                        quest.stages[1].stageIndex == 20 &&
                        quest.stages[0].blocks.size() == 1 &&
                        quest.stages[0].blocks[0].scriptIndex == 0 &&
                        quest.stages[0].blocks[0].runsOnce() &&
                        quest.stages[0].blocks[0].hasBytecode() &&
                        quest.stages[0].blocks[0].logText == "Stage ten journal" &&
                        quest.stages[0].blocks[0].conditions.size() == 1 &&
                        quest.stages[1].blocks.size() == 1 &&
                        !quest.stages[1].blocks[0].runsOnce() &&
                        !quest.stages[1].blocks[0].hasBytecode() &&
                        quest.stages[1].blocks[0].logText == "Stage twenty journal" &&
                        quest.stages[0].isCompletionStage() &&
                        !quest.stages[1].isCompletionStage();

        record("ScriptManager: Quest record layout", ok,
               "Parse INDX/QSDT/CNAM/CTDA owners from the on-disk QUST shape",
               getTimeMs38() - start);
    }

    // Test 9: INFO subrecords follow the measured layout. DATA is 3 bytes
    // (byte 0 = topic category, byte 1 = per-INFO flags), TRDT is 16 bytes
    // (byte 0 = emotion type, byte 4 = emotion value, byte 12 = 1-based
    // response ordinal) and carries no speaker FormID, NAM2 is an acting
    // direction rather than player-facing text, and faction/quest requirements
    // come from CTDA because ANAM/CNAM/QSTN never occur in INFO.
    {
        float start = getTimeMs38();

        auto appendSub = [](std::vector<uint8_t>& out, const char* type,
                            const std::vector<uint8_t>& body) {
            out.insert(out.end(), type, type + 4);
            const uint16_t size = static_cast<uint16_t>(body.size());
            out.push_back(static_cast<uint8_t>(size & 0xFF));
            out.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
            out.insert(out.end(), body.begin(), body.end());
        };
        auto ascii = [](const char* text) {
            return std::vector<uint8_t>(text, text + std::strlen(text));
        };
        auto u32le = [](uint32_t value) {
            return std::vector<uint8_t>{
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)};
        };

        ESMRecord infoRec;
        infoRec.recType[0] = 'I';
        infoRec.recType[1] = 'N';
        infoRec.recType[2] = 'F';
        infoRec.recType[3] = 'O';
        infoRec.formID = 0x00012345;

        auto addSub = [&](const char* type, const std::vector<uint8_t>& body) {
            SubRecord sub;
            std::memcpy(sub.tag, type, 4);
            sub.data = body;
            infoRec.subRecords.push_back(std::move(sub));
        };

        addSub("NAM1", ascii("Greetings, traveler."));
        addSub("NAM2", ascii("Lucien Lachance -- sinister"));
        addSub("DATA", {0x01, 0x0A, 0x00});
        // TRDT: emotion type 3, emotion value 50, ordinal 2, filler 0xCD.
        std::vector<uint8_t> trdt(16, 0x00);
        trdt[0] = 0x03;
        trdt[4] = 0x32;
        trdt[12] = 0x02;
        trdt[13] = 0xCD;
        trdt[14] = 0xCD;
        trdt[15] = 0xCD;
        addSub("TRDT", trdt);
        addSub("QSTI", u32le(0x0000ABCD));
        // CTDA: GetFactionRank (45) >= 3 against faction 0x0000BEEF.
        std::vector<uint8_t> ctda(24, 0x00);
        ctda[0] = 45;
        ctda[2] = 0x00;
        const float rank = 3.0f;
        std::memcpy(ctda.data() + 4, &rank, 4);
        const uint32_t factionFormID = 0x0000BEEF;
        std::memcpy(ctda.data() + 8, &factionFormID, 4);
        addSub("CTDA", ctda);

        InfoData info;
        decodeInfoRecord(infoRec, info);

        const bool ok = info.formID == 0x00012345 &&
                        info.responseText == "Greetings, traveler." &&
                        info.actingNotes == "Lucien Lachance -- sinister" &&
                        info.responseType == 0x01 && info.infoFlags == 0x0A &&
                        info.emotionType == 3 && info.emotionValue == 50 &&
                        info.responseNumber == 2 &&
                        info.questFormID == 0x0000ABCD &&
                        info.factionFormID == 0x0000BEEF &&
                        info.factionRank == 3;

        record("ScriptManager: INFO record layout", ok,
               "Read DATA/TRDT/CTDA from the on-disk INFO shape and keep NAM2 out of player text",
               getTimeMs38() - start);
    }
}

// ============================================
// Native SCDA decoder
// ============================================
namespace {

void appendU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void appendU32(std::vector<uint8_t>& out, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }
}

void appendInstruction(std::vector<uint8_t>& out, uint16_t opcode,
                       const std::vector<uint8_t>& payload) {
    appendU16(out, opcode);
    appendU16(out, static_cast<uint16_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
}

void appendMarker(std::vector<uint8_t>& out, uint16_t meta) {
    appendU16(out, NATIVE_SCDA_MARKER_OPCODE);
    appendU16(out, meta);
}

} // namespace

void ScriptVMTests::testNativeScdaDecoder() {
    // Test 1: the call reference selector is always 4 bytes
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> code;
        appendMarker(code, 37);
        appendInstruction(code, 0x0019, {});

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 2 &&
                        result.instructions[0].isMarker &&
                        result.instructions[0].encodedLength == 4 &&
                        result.instructions[0].referenceIndex == 37 &&
                        result.instructions[1].opcode == 0x0019;
        record("NativeScda: call ref selector", ok,
               "The selector index is read and its 4 bytes are always consumed",
               getTimeMs38() - start);
    }

    // Test 2: Begin body length lets the decoder skip the whole block
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendU16(body, 0x0000);  // gamemode
        appendU16(body, 0);       // body length patched below
        appendU32(body, 0);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0010, body);
        appendInstruction(code, 0x0011, {});

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 2 &&
                        result.instructions[0].isStructural &&
                        result.instructions[0].blockType == 0x0000 &&
                        result.instructions[1].opcode == 0x0011;
        record("NativeScda: Begin block", ok,
               "Begin payload exposes block type and body length",
               getTimeMs38() - start);
    }

    // Test 2b: A six byte Begin payload is the dominant retail shape. The
    // trailing word and the argument list are optional, so only blockType and
    // bodyLength are required. Rejecting these dropped 1,556 of 2,393 records.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 0x0000);  // gamemode
        appendU16(payload, 0x00db);  // body length
        appendU16(payload, 0);       // trailing word, no argument list

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0010, payload);
        appendInstruction(code, 0x0011, {});

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 2 &&
                        result.instructions[0].isStructural &&
                        result.instructions[0].blockType == 0x0000 &&
                        result.instructions[0].bodyLength == 0x00db &&
                        result.instructions[1].opcode == 0x0011;
        record("NativeScda: six byte Begin payload", ok,
               "Begin accepts the minimal blockType and bodyLength payload",
               getTimeMs38() - start);
    }

    // Test 3: If expression length must equal payload length minus 4
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 1);   // compiler metadata
        appendU16(payload, 3);   // expression length
        payload.push_back('n');
        payload.push_back(0x01);
        payload.push_back(0x00);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0016, payload);
        appendInstruction(code, 0x0019, {});

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 2 &&
                        result.instructions[0].meta == 1 &&
                        result.instructions[0].expression.size() == 3;
        record("NativeScda: If expression", ok,
               "If payload splits into metadata and expression bytes",
               getTimeMs38() - start);
    }

    // Test 4: Else is exactly 2 bytes
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 1);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0017, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].meta == 1;
        record("NativeScda: Else is 2 bytes", ok,
               "Else carries a single metadata word",
               getTimeMs38() - start);
    }

    // Test 5: SetStage SQ07 100 -> [2][<r3>][n 100]
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 2);
        payload.push_back('r');
        appendU16(payload, 3);
        payload.push_back('n');
        appendU32(payload, 100);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1039, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].tokens.size() == 2 &&
                        result.instructions[0].tokens[0].kind == NativeTokenKind::Variable &&
                        result.instructions[0].tokens[0].typeChar == 'r' &&
                        result.instructions[0].tokens[0].index == 3 &&
                        result.instructions[0].tokens[1].kind == NativeTokenKind::Integer &&
                        result.instructions[0].tokens[1].intValue == 100 &&
                        !result.instructions[0].hasImplicitSelf;
        record("NativeScda: SetStage tokens", ok,
               "SetStage decodes as a reference and an integer literal",
               getTimeMs38() - start);
    }

    // Test 6: AddItem with a reference property operand. The declared count
    // counts operands, so the reference and its member fold into one token and
    // the token count matches argc exactly.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 2);
        payload.push_back('r');
        appendU16(payload, 2);
        payload.push_back('r');
        appendU16(payload, 3);
        payload.push_back('s');
        appendU16(payload, 5);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1002, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].tokens.size() == 2 &&
                        !result.instructions[0].hasImplicitSelf &&
                        result.instructions[0].tokens[1].hasMember &&
                        result.instructions[0].tokens[1].memberIndex == 5 &&
                        result.instructions[0].declaredArgumentCount == 2;
        record("NativeScda: reference property operand", ok,
               "A reference and its member fold into one operand",
               getTimeMs38() - start);
    }

    // Test 7: bare u16 tokens are not limited to values below 0x20
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 2);
        appendU16(payload, 0x0021);  // Aggression
        payload.push_back('n');
        appendU32(payload, 5);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x100F, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].tokens.size() == 2 &&
                        result.instructions[0].tokens[0].kind == NativeTokenKind::BareU16 &&
                        result.instructions[0].tokens[0].intValue == 0x0021;
        record("NativeScda: bare u16 actor value", ok,
               "Actor value codes above 0x20 must decode as bare u16 tokens",
               getTimeMs38() - start);
    }

    // Test 8: MoveTo accepts only [1][<r>]
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 1);
        payload.push_back('r');
        appendU16(payload, 4);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x109E, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].tokens.size() == 1 &&
                        result.instructions[0].tokens[0].kind == NativeTokenKind::Variable;
        record("NativeScda: MoveTo single reference", ok,
               "MoveTo payload is a single reference token",
               getTimeMs38() - start);
    }

    // Test 8b: MoveTo accepts exactly the four shapes the compiler emits and
    // reads the offsets in source order x, y, z. The one-offset form lands in x
    // with y and z left at zero, and no shape carries a cell reference.
    {
        const float start = getTimeMs38();

        auto decodeMoveTo = [](const std::vector<uint8_t>& payload,
                               NativeMoveToArguments& args) {
            std::vector<uint8_t> code;
            appendInstruction(code, 0x109E, payload);
            const NativeDecodeResult result = decodeNativeScda(code);
            if (!result.success || result.instructions.size() != 1) return false;
            return decodeNativeMoveToArguments(result.instructions[0], args);
        };

        auto refToken = [](std::vector<uint8_t>& payload) {
            payload.push_back('r');
            appendU16(payload, 4);
        };
        auto doubleToken = [](std::vector<uint8_t>& payload, double value) {
            payload.push_back('z');
            uint64_t bits = 0;
            std::memcpy(&bits, &value, 8);
            for (int i = 0; i < 8; ++i) {
                payload.push_back(static_cast<uint8_t>((bits >> (8 * i)) & 0xFF));
            }
        };
        auto floatVarToken = [](std::vector<uint8_t>& payload, uint16_t index) {
            payload.push_back('f');
            appendU16(payload, index);
        };

        bool ok = true;

        // [1][<r>] ? no offsets
        {
            std::vector<uint8_t> payload;
            appendU16(payload, 1);
            refToken(payload);
            NativeMoveToArguments args;
            ok = ok && decodeMoveTo(payload, args) && args.valid &&
                 !args.hasOffsets && args.offsetCount == 0;
        }

        // [2][<r>][z] ? x only
        {
            std::vector<uint8_t> payload;
            appendU16(payload, 2);
            refToken(payload);
            doubleToken(payload, 20.0);
            NativeMoveToArguments args;
            ok = ok && decodeMoveTo(payload, args) && args.valid &&
                 args.hasOffsets && args.offsetCount == 1 &&
                 args.offsetIsLiteral[0] && args.offsetLiterals[0] == 20.0f &&
                 !args.offsetIsLiteral[1] && !args.offsetIsLiteral[2];
        }

        // [4][<r>][z][z][z] ? x y z
        {
            std::vector<uint8_t> payload;
            appendU16(payload, 4);
            refToken(payload);
            doubleToken(payload, 0.0);
            doubleToken(payload, 100.0);
            doubleToken(payload, 0.0);
            NativeMoveToArguments args;
            ok = ok && decodeMoveTo(payload, args) && args.valid &&
                 args.hasOffsets && args.offsetCount == 3 &&
                 args.offsetIsLiteral[0] && args.offsetLiterals[0] == 0.0f &&
                 args.offsetIsLiteral[1] && args.offsetLiterals[1] == 100.0f &&
                 args.offsetIsLiteral[2] && args.offsetLiterals[2] == 0.0f;
        }

        // [4][<r>][f][f][z] ? the shape behind
        // "SEHaskillRef.moveto player x y 0": x and y are float script
        // variables, so only z carries a literal.
        {
            std::vector<uint8_t> payload;
            appendU16(payload, 4);
            refToken(payload);
            floatVarToken(payload, 4);
            floatVarToken(payload, 5);
            doubleToken(payload, 0.0);
            NativeMoveToArguments args;
            ok = ok && decodeMoveTo(payload, args) && args.valid &&
                 args.hasOffsets && args.offsetCount == 3 &&
                 !args.offsetIsLiteral[0] && !args.offsetIsLiteral[1] &&
                 args.offsetIsLiteral[2] && args.offsetLiterals[2] == 0.0f &&
                 args.offsetTokens[0] != nullptr &&
                 args.offsetTokens[0]->kind == NativeTokenKind::Variable &&
                 args.offsetTokens[0]->index == 4 &&
                 args.offsetTokens[1] != nullptr &&
                 args.offsetTokens[1]->index == 5;
        }

        // Two offsets never occur, so they must not be accepted.
        {
            std::vector<uint8_t> payload;
            appendU16(payload, 3);
            refToken(payload);
            doubleToken(payload, 10.0);
            doubleToken(payload, 20.0);
            NativeMoveToArguments args;
            ok = ok && !decodeMoveTo(payload, args);
        }

        record("NativeScda: MoveTo offset shapes", ok,
               "MoveTo accepts the four emitted shapes and reads offsets as x, y, z",
               getTimeMs38() - start);
    }

    // Test 9: payload-free commands are bare
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> code;
        appendInstruction(code, 0x1021, {});
        appendInstruction(code, 0x1022, {});
        appendInstruction(code, 0x105E, {});

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 3 &&
                        result.instructions[0].isBare &&
                        result.instructions[1].isBare &&
                        result.instructions[2].isBare;
        record("NativeScda: bare commands", ok,
               "Enable, Disable and Evp carry no payload",
               getTimeMs38() - start);
    }

    // Test 10: MessageBox splits into text, format arguments and buttons
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 1);   // one argument
        appendU16(payload, 4);   // "Yes" plus NUL
        payload.push_back('Y');
        payload.push_back('e');
        payload.push_back('s');
        payload.push_back(0x00);
        appendU16(payload, 0);   // no format arguments
        appendU16(payload, 2);   // two buttons
        appendU16(payload, 1);   // button marker
        appendU16(payload, 4);
        payload.push_back('Y');
        payload.push_back('e');
        payload.push_back('s');
        payload.push_back(0x00);
        appendU16(payload, 1);   // button marker
        appendU16(payload, 3);
        payload.push_back('N');
        payload.push_back('o');
        payload.push_back(0x00);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1000, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].text == "Yes" &&
                        result.instructions[0].formatArgumentCount == 0 &&
                        result.instructions[0].buttonCount == 2 &&
                        result.instructions[0].buttonTexts.size() == 2 &&
                        result.instructions[0].buttonTexts[0] == "Yes" &&
                        result.instructions[0].buttonTexts[1] == "No";
        record("NativeScda: MessageBox framing", ok,
               "MessageBox splits into text, format arguments and button labels",
               getTimeMs38() - start);
    }

    // Test 10b: Message ends with a zero word
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 1);
        appendU16(payload, 5);
        payload.push_back('H');
        payload.push_back('e');
        payload.push_back('l');
        payload.push_back('l');
        payload.push_back('o');
        appendU16(payload, 0);
        appendU32(payload, 0);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1059, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].text == "Hello" &&
                        result.instructions[0].formatArgumentCount == 0;
        record("NativeScda: Message framing", ok,
               "Message splits into text, format arguments and a zero word",
               getTimeMs38() - start);
    }

    // Test 10e: a real message whose tail word is not zero
    {
        const float start = getTimeMs38();
        // Payload taken verbatim from Oblivion3.esm: "The lever is stuck and
        // will not budge." carries a nonzero tail word of 5.
        const char* hex =
            "01002600546865206c6576657220697320737475636b20616e642077696c6c20"
            "6e6f742062756467652e000005000000";
        std::vector<uint8_t> payload;
        for (size_t i = 0; hex[i] != '\0' && hex[i + 1] != '\0'; i += 2) {
            const auto nibble = [](char c) -> uint8_t {
                if (c >= '0' && c <= '9') {
                    return static_cast<uint8_t>(c - '0');
                }
                return static_cast<uint8_t>(c - 'a' + 10);
            };
            payload.push_back(static_cast<uint8_t>((nibble(hex[i]) << 4) |
                                                   nibble(hex[i + 1])));
        }

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1059, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].text ==
                            "The lever is stuck and will not budge." &&
                        result.instructions[0].formatArgumentCount == 0;
        record("NativeScda: Message nonzero tail", ok,
               "A message with a nonzero tail word still frames exactly",
               getTimeMs38() - start);
    }

    // Test 10f: a real message whose format count word undercounts
    {
        const float start = getTimeMs38();
        // Payload taken verbatim from Oblivion3.esm: the format count word is
        // 1 but two tokens follow, so the token list is walked greedily.
        const char* hex =
            "01002f0044454255473a2056616c656e204472657468206c61737420696e666f"
            "2c20636f6e7654696d657220746f20252e32660100720100660a0000000000";
        std::vector<uint8_t> payload;
        for (size_t i = 0; hex[i] != '\0' && hex[i + 1] != '\0'; i += 2) {
            const auto nibble = [](char c) -> uint8_t {
                if (c >= '0' && c <= '9') {
                    return static_cast<uint8_t>(c - '0');
                }
                return static_cast<uint8_t>(c - 'a' + 10);
            };
            payload.push_back(static_cast<uint8_t>((nibble(hex[i]) << 4) |
                                                   nibble(hex[i + 1])));
        }

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1059, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].formatArgumentCount == 2 &&
                        result.instructions[0].formatTokens.size() == 2;
        record("NativeScda: Message format count undercount", ok,
               "A message whose count word undercounts still frames exactly",
               getTimeMs38() - start);
    }

    // Test 10d: a real tombstone message with two buttons
    {
        const float start = getTimeMs38();
        // Payload taken verbatim from Oblivion3.esm: the Countess
        // Sheen-In-Glade tombstone offers "Pry Coffin Open" and
        // "Leave it Alone".
        const char* hex =
            "01005c0048657265206c69657320436f756e7465737320536865656e2d496e2d"
            "476c6164652c204d6174726f6e205363686f6c6172206f6620566974686172"
            "6e2c20616e6420416d6261737361646f72206f6620426c61636b204d617273"
            "682e0000020001000f0050727920436f6666696e204f70656e01000e004c65"
            "61766520697420416c6f6e65";
        std::vector<uint8_t> payload;
        for (size_t i = 0; hex[i] != '\0' && hex[i + 1] != '\0'; i += 2) {
            const auto nibble = [](char c) -> uint8_t {
                if (c >= '0' && c <= '9') {
                    return static_cast<uint8_t>(c - '0');
                }
                return static_cast<uint8_t>(c - 'a' + 10);
            };
            payload.push_back(static_cast<uint8_t>((nibble(hex[i]) << 4) |
                                                   nibble(hex[i + 1])));
        }

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1000, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].buttonCount == 2 &&
                        result.instructions[0].buttonTexts.size() == 2 &&
                        result.instructions[0].buttonTexts[0] ==
                            "Pry Coffin Open" &&
                        result.instructions[0].buttonTexts[1] ==
                            "Leave it Alone";
        record("NativeScda: MessageBox tombstone", ok,
               "A real two-button tombstone message frames exactly",
               getTimeMs38() - start);
    }

    // Test 10c: a rename command carries an explicit reference before the text
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 2);
        payload.push_back('r');
        appendU16(payload, 1);
        appendU16(payload, 7);
        payload.push_back('M');
        payload.push_back('y');
        payload.push_back(' ');
        payload.push_back('H');
        payload.push_back('o');
        payload.push_back('m');
        payload.push_back('e');

        std::vector<uint8_t> code;
        appendInstruction(code, 0x111B, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].text == "My Home" &&
                        result.instructions[0].hasImplicitSelf &&
                        result.instructions[0].tokens.size() == 1 &&
                        result.instructions[0].tokens[0].kind == NativeTokenKind::Variable;
        record("NativeScda: rename with reference", ok,
               "A rename command reads its reference before the new name",
               getTimeMs38() - start);
    }

    // Test 10d: a string payload that does not frame keeps its raw bytes
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 1);
        appendU16(payload, 4);
        payload.push_back('Y');
        payload.push_back('e');
        payload.push_back('s');
        payload.push_back(0x00);
        appendU16(payload, 0);
        appendU16(payload, 1);
        appendU16(payload, 4);
        payload.push_back('Y');
        payload.push_back('e');
        payload.push_back('s');
        payload.push_back(0x00);
        payload.push_back(0x00);  // one byte too many

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1000, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].framingFailed &&
                        result.instructions[0].text.empty() &&
                        result.instructions[0].payload.size() == payload.size();
        record("NativeScda: unframed string payload kept", ok,
               "A string payload that does not frame keeps its raw bytes",
               getTimeMs38() - start);
    }

    // Test 11: a truncated instruction reports its exact offset
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> code;
        appendInstruction(code, 0x0019, {});
        appendU16(code, 0x1039);
        appendU16(code, 10);
        code.push_back(0x00);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = !result.success && result.errorOffset == 4 &&
                        result.errorOpcode == 0x1039;
        record("NativeScda: truncated payload", ok,
               "Truncation reports the failing opcode and offset",
               getTimeMs38() - start);
    }

    // Test 12: the SCPT prologue is decoded but never required
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> prologuePayload;
        appendU16(prologuePayload, 0x0001);

        std::vector<uint8_t> code;
        appendInstruction(code, NATIVE_SCDA_PROLOGUE_OPCODE, prologuePayload);
        appendInstruction(code, 0x0019, {});

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 2 &&
                        result.instructions[0].isPrologue &&
                        result.instructions[1].opcode == 0x0019;
        record("NativeScda: prologue optional", ok,
               "Prologue decodes without being required by the decoder",
               getTimeMs38() - start);
    }

    // Test 13: an argument count mismatch is an explicit failure
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 3);
        payload.push_back('r');
        appendU16(payload, 1);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1039, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].tokens.empty() &&
                        !result.instructions[0].payload.empty();
        record("NativeScda: argc mismatch", ok,
               "An argument count mismatch falls back to the raw payload",
               getTimeMs38() - start);
    }

    // Test 14: opcode names resolve for the confirmed command set
    {
        const float start = getTimeMs38();
        const bool ok = getNativeOpcodeName(0x1039) == "SetStage" &&
                        getNativeOpcodeName(0x109E) == "MoveTo" &&
                        getNativeOpcodeName(0x001E) == "Return" &&
                        getNativeOpcodeName(0x1076) == "ModCrimeGold" &&
                        getNativeOpcodeName(0x1075) == "SetCrimeGold" &&
                        getNativeOpcodeName(0x111B) == "SetCellFullName" &&
                        getNativeOpcodeName(0x111C) == "SetActorFullName" &&
                        getNativeOpcodeName(0x1007) == "SetPos" &&
                        getNativeOpcodeName(0x10BF) == "ModAmountSoldStolen" &&
                        getNativeOpcodeName(0x10DE) == "CloseOblivionGate" &&
                        getNativeOpcodeName(0x10C0) == "CloseCurrentOblivionGate" &&
                        getNativeOpcodeName(0x1133) == "SetLevel" &&
                        getNativeOpcodeName(0x1114) == "PlayBink" &&
                        getNativeOpcodeName(0x9999).empty();
        record("NativeScda: opcode names", ok,
               "Confirmed opcodes resolve and unknown ones stay unnamed",
               getTimeMs38() - start);
    }

    // Test 14b: the expression side function names resolve
    {
        const float start = getTimeMs38();
        const bool ok = getNativeOpcodeName(0x103A) == "GetStage" &&
                        getNativeOpcodeName(0x102F) == "GetItemCount" &&
                        getNativeOpcodeName(0x103B) == "GetStageDone" &&
                        getNativeOpcodeName(0x102E) == "GetDead" &&
                        getNativeOpcodeName(0x1069) == "IsActionRef" &&
                        getNativeOpcodeName(0x10CE) == "GetSelf" &&
                        getNativeOpcodeName(0x1001) == "GetDistance" &&
                        getNativeOpcodeName(0x100E) == "GetAV" &&
                        getNativeOpcodeName(0x1048) == "GetIsID" &&
                        getNativeOpcodeName(0x1113) == "GetParentRef" &&
                        getNativeOpcodeName(0x1121) == "IsInCombat" &&
                        getNativeOpcodeName(0x1100) == "GetGameSetting";
        record("NativeScda: expression function names", ok,
               "The expression side function ids resolve to their command names",
               getTimeMs38() - start);
    }

    // Test 15: both spellings of an opcode are accepted
    {
        const float start = getTimeMs38();
        const auto evp = getNativeOpcodeAliases(0x105E);
        const auto moveTo = getNativeOpcodeAliases(0x109E);
        const auto setAv = getNativeOpcodeAliases(0x100F);
        const auto getGs = getNativeOpcodeAliases(0x1100);
        const auto scaOnActor = getNativeOpcodeAliases(0x1101);
        const bool ok = evp.size() == 2 && evp[0] == "evp" &&
                        evp[1] == "evaluatepackage" &&
                        moveTo.size() == 2 && moveTo[0] == "moveto" &&
                        moveTo[1] == "movetomarker" &&
                        setAv.size() == 2 && setAv[0] == "setav" &&
                        getGs.size() == 2 && getGs[0] == "getgs" &&
                        getGs[1] == "getgamesetting" &&
                        scaOnActor.size() == 2 && scaOnActor[0] == "scaonactor" &&
                        scaOnActor[1] == "stopcombalarmonactor" &&
                        getNativeOpcodeAliases(0x1039).empty();
        record("NativeScda: opcode aliases", ok,
               "Short and long spellings resolve to the same opcode",
               getTimeMs38() - start);
    }

    // Test 16: slots number the record's ref subrecords in file order
    {
        const float start = getTimeMs38();
        // XPXirethard01TrapButton01SCRIPT file order: SCRV(myParent), SCRO x3,
        // SCRV(mySelf). A fixed "SCRO first" rule would put myParent at slot 4.
        const std::vector<NativeReferenceEntry> entries = {
            {NativeReferenceKind::LocalRef, 0, 5},
            {NativeReferenceKind::Scro, 0x000446A1, 0},
            {NativeReferenceKind::Scro, 0x000446A7, 0},
            {NativeReferenceKind::Scro, 0x000446B9, 0},
            {NativeReferenceKind::LocalRef, 0, 4},
        };

        NativeReferenceSlot slot{};
        std::string error;
        const bool localFirst = resolveNativeReferenceSlot(1, entries, slot, error) &&
                                slot.kind == NativeReferenceKind::LocalRef &&
                                slot.localOrdinal == 5 && slot.slot == 1;

        const bool scroMiddle = resolveNativeReferenceSlot(2, entries, slot, error) &&
                                slot.kind == NativeReferenceKind::Scro &&
                                slot.formId == 0x000446A1;

        const bool localLast = resolveNativeReferenceSlot(5, entries, slot, error) &&
                               slot.kind == NativeReferenceKind::LocalRef &&
                               slot.localOrdinal == 4;

        const bool zeroRejected = !resolveNativeReferenceSlot(0, entries, slot, error) &&
                                  !error.empty();
        error.clear();
        const bool overRejected = !resolveNativeReferenceSlot(6, entries, slot, error) &&
                                  !error.empty();
        error.clear();
        const bool emptyTable = !resolveNativeReferenceSlot(1, {}, slot, error) &&
                                !error.empty();

        const bool ok = localFirst && scroMiddle && localLast && zeroRejected &&
                        overRejected && emptyTable;
        record("NativeScda: reference slot", ok,
               "Slots number SCRV/SCRO subrecords in file order; out-of-range slots are errors",
               getTimeMs38() - start);
    }

    // Test 16b: the call selector space is separate from the r/Z slot space.
    // XPXirethard01TrapButton01SCRIPT numbers myParent as r1, yet the selector
    // that precedes its Activate is also 1 only because selectors count call
    // targets in first-appearance order. The two tables are supplied separately
    // and must not be interchangeable.
    {
        const float start = getTimeMs38();
        const std::vector<NativeReferenceEntry> entries = {
            {NativeReferenceKind::LocalRef, 0, 5},
            {NativeReferenceKind::Scro, 0x000446A1, 0},
            {NativeReferenceKind::Scro, 0x000446A7, 0},
        };
        const std::vector<uint32_t> callTargets = {0x000446A1, 0x000446A7};

        NativeReferenceSlot slot{};
        std::string error;
        const bool slotIsLocal = resolveNativeReferenceSlot(1, entries, slot, error) &&
                                 slot.kind == NativeReferenceKind::LocalRef;

        uint32_t target = 0;
        error.clear();
        const bool selectorIsFirst = resolveNativeCallTarget(1, callTargets, target, error) &&
                                     target == 0x000446A1;
        error.clear();
        const bool selectorSecond = resolveNativeCallTarget(2, callTargets, target, error) &&
                                    target == 0x000446A7;
        error.clear();
        const bool zeroRejected = !resolveNativeCallTarget(0, callTargets, target, error) &&
                                  !error.empty();
        error.clear();
        const bool overRejected = !resolveNativeCallTarget(3, callTargets, target, error) &&
                                  !error.empty();

        const bool ok = slotIsLocal && selectorIsFirst && selectorSecond &&
                        zeroRejected && overRejected;
        record("NativeScda: call target space", ok,
               "Call selectors number first-appearance order, not the r/Z slot table",
               getTimeMs38() - start);
    }

    // Test 17: the later naming batch resolves
    {
        const float start = getTimeMs38();
        const bool ok = getNativeOpcodeName(0x1071) == "CompleteQuest" &&
                        getNativeOpcodeName(0x10A9) == "ModFactionReaction" &&
                        getNativeOpcodeName(0x10D9) == "ShowClassMenu" &&
                        getNativeOpcodeName(0x10E7) == "SetInChargen" &&
                        getNativeOpcodeName(0x10F0) == "SetClass" &&
                        getNativeOpcodeName(0x1111) == "EnableFastTravel" &&
                        getNativeOpcodeName(0x1127) == "ResetInterior" &&
                        getNativeOpcodeName(0x1141) == "SetNoRumors" &&
                        getNativeOpcodeName(0x1145) == "RefreshTopicList" &&
                        getNativeOpcodeName(0x115C) == "SendTrespassAlarm" &&
                        getNativeOpcodeName(0x1165) == "ForceCloseOblivionGate" &&
                        getNativeOpcodeName(0x116C) == "SetPlayerInSEWorld" &&
                        getNativeOpcodeName(0x1037) == "StopQuest";
        record("NativeScda: naming batch", ok,
               "CompleteQuest and the later command names resolve, StopQuest stays distinct",
               getTimeMs38() - start);
    }

    // Test 18: the remote Set form, taken from SEHaskillSummonQuestScript.
    // The payload is [0x72][u16 refSlot][type][u16 remoteVarIndex][u16 elen][expr]
    // and the remote variable index belongs to the target script's SLSD table.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        payload.push_back(NATIVE_SCDA_REMOTE_SET_CHAR);
        appendU16(payload, 1);   // reference slot 1
        payload.push_back('f');  // remote variable type
        appendU16(payload, 0x000B);
        appendU16(payload, 2);   // expression length
        payload.push_back(0x20);
        payload.push_back('1');

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0015, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].tokens.size() == 1 &&
                        result.instructions[0].tokens[0].kind == NativeTokenKind::Variable &&
                        result.instructions[0].tokens[0].typeChar == 'f' &&
                        result.instructions[0].tokens[0].index == 0x000B &&
                        result.instructions[0].expressionDecoded &&
                        result.instructions[0].expressionTokens.size() == 1 &&
                        result.instructions[0].expressionTokens[0].kind ==
                            NativeTokenKind::Text &&
                        result.instructions[0].expressionTokens[0].text == "1";
        record("NativeScda: remote Set target", ok,
               "Set resolves a remote variable target and its expression",
               getTimeMs38() - start);
    }

    // Test 18b: a local Set whose target is a reference member. The local form
    // starts with the target's type char, and the measured local type chars are
    // only f, s and G, so a reference member target is written as a reference
    // variable followed by the member variable.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        payload.push_back('f');
        appendU16(payload, 3);   // local reference variable
        payload.push_back('f');
        appendU16(payload, 8);   // member variable
        appendU16(payload, 2);   // expression length
        payload.push_back(0x20);
        payload.push_back('1');

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0015, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].tokens.size() == 2 &&
                        result.instructions[0].tokens[0].typeChar == 'f' &&
                        result.instructions[0].tokens[0].index == 3 &&
                        result.instructions[0].tokens[1].typeChar == 'f' &&
                        result.instructions[0].tokens[1].index == 8 &&
                        result.instructions[0].expressionDecoded;
        record("NativeScda: Set two token target", ok,
               "Set resolves a reference member target and its expression",
               getTimeMs38() - start);
    }

    // Test 19: the reference literal is a 1-based SCRO index
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        payload.push_back('f');
        appendU16(payload, 7);   // speaker
        appendU16(payload, 4);   // expression length
        payload.push_back(0x20);
        payload.push_back(NATIVE_SCDA_REFERENCE_LITERAL_CHAR);
        appendU16(payload, 2);   // SCRO[1]

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0015, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].expressionDecoded &&
                        result.instructions[0].expressionTokens.size() == 1 &&
                        result.instructions[0].expressionTokens[0].kind ==
                            NativeTokenKind::Variable &&
                        result.instructions[0].expressionTokens[0].typeChar ==
                            static_cast<char>(NATIVE_SCDA_REFERENCE_LITERAL_CHAR) &&
                        result.instructions[0].expressionTokens[0].index == 2;
        record("NativeScda: reference literal", ok,
               "The reference literal decodes as a 1-based SCRO index",
               getTimeMs38() - start);
    }

    // Test 20: expressions keep their postfix ASCII operators and function calls
    {
        const float start = getTimeMs38();
        // Real bytes from SE09RootGateMania02SCRIPT:
        //   " 5869 1005 0001 0072 0300 20 31 20 3d 3d"
        //   X 1069 argc=5 [u16 1][r3] " 1" " =="
        std::vector<uint8_t> expression;
        expression.push_back(0x20);
        expression.push_back('X');
        appendU16(expression, 0x1069);  // IsActionRef
        appendU16(expression, 5);
        appendU16(expression, 1);
        expression.push_back('r');
        appendU16(expression, 3);
        expression.push_back(0x20);
        expression.push_back('1');
        expression.push_back(0x20);
        expression.push_back('=');
        expression.push_back('=');

        std::vector<NativeToken> tokens;
        std::string error;
        const bool ok = decodeNativeExpression(expression.data(), expression.size(),
                                               tokens, error) &&
                        tokens.size() == 3 &&
                        tokens[0].kind == NativeTokenKind::Function &&
                        tokens[0].index == 0x1069 &&
                        tokens[0].arguments.size() == 1 &&
                        tokens[0].arguments[0].kind == NativeTokenKind::Variable &&
                        tokens[0].arguments[0].index == 3 &&
                        tokens[1].kind == NativeTokenKind::Text &&
                        tokens[1].text == "1" &&
                        tokens[2].kind == NativeTokenKind::Text &&
                        tokens[2].text == "==";
        record("NativeScda: expression tokenizer", ok,
               "Expressions keep postfix ASCII operators and nested call arguments",
               getTimeMs38() - start);
    }

    // Test 21: an axis selector is one byte and wins over a bare u16 reading
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        appendU16(payload, 2);
        payload.push_back('Z');
        payload.push_back('z');
        const uint8_t doubleBytes[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 0x40};
        payload.insert(payload.end(), doubleBytes, doubleBytes + 8);

        std::vector<uint8_t> code;
        appendInstruction(code, 0x1009, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        !result.instructions[0].framingFailed &&
                        result.instructions[0].tokens.size() == 2 &&
                        result.instructions[0].tokens[0].kind == NativeTokenKind::Axis &&
                        result.instructions[0].tokens[0].typeChar == 'Z' &&
                        result.instructions[0].tokens[1].kind == NativeTokenKind::Double &&
                        result.instructions[0].tokens[1].doubleValue == 200.0;
        record("NativeScda: axis selector", ok,
               "A one byte axis selector is preferred over a bare u16 reading",
               getTimeMs38() - start);
    }

    // Test 22: a Set payload that cannot be framed keeps its raw bytes
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> payload;
        payload.push_back('r');
        appendU16(payload, 1);
        appendU16(payload, 9);   // expression length does not match the payload
        payload.push_back(0x20);
        payload.push_back('1');

        std::vector<uint8_t> code;
        appendInstruction(code, 0x0015, payload);

        const NativeDecodeResult result = decodeNativeScda(code);
        const bool ok = result.success && result.instructions.size() == 1 &&
                        result.instructions[0].framingFailed &&
                        result.instructions[0].tokens.empty() &&
                        result.instructions[0].payload.size() == payload.size();
        record("NativeScda: unframed Set kept", ok,
               "A Set payload that does not frame keeps its raw bytes",
               getTimeMs38() - start);
    }

    // Test 23: the angle and reload names resolve
    {
        const float start = getTimeMs38();
        const bool ok = getNativeOpcodeName(0x1008) == "GetAngle" &&
                        getNativeOpcodeName(0x1009) == "SetAngle" &&
                        getNativeOpcodeName(0x114D) == "EssentialDeathReload" &&
                        getNativeOpcodeAliases(0x1009).size() == 1 &&
                        getNativeOpcodeAliases(0x1009)[0] == "setangle";
        record("NativeScda: angle and reload names", ok,
               "GetAngle, SetAngle and EssentialDeathReload resolve",
               getTimeMs38() - start);
    }
}

// ============================================
// Native SCDA virtual machine
// ============================================
namespace {

// Builds a Begin block. bodyLength counts the bytes from the payload start to
// the last instruction inside the block, so it is the body size plus the 4 byte
// End that closes it.
void appendBeginBlock(std::vector<uint8_t>& out, uint16_t blockType,
                      const std::vector<uint8_t>& body) {
    std::vector<uint8_t> payload;
    appendU16(payload, blockType);
    appendU16(payload, static_cast<uint16_t>(body.size() + 4));
    appendU32(payload, 0);
    appendInstruction(out, 0x0010, payload);
    out.insert(out.end(), body.begin(), body.end());
    appendInstruction(out, 0x0011, {});
}

// An If/ElseIf/Else payload: compiler metadata, expression length, expression.
// Else carries only the metadata word; measured over the retail corpus, all 172
// Else instructions are exactly 2 bytes.
void appendConditional(std::vector<uint8_t>& out, uint16_t opcode,
                       const std::vector<uint8_t>& expression) {
    std::vector<uint8_t> payload;
    appendU16(payload, 1);
    if (opcode != 0x0017) {
        appendU16(payload, static_cast<uint16_t>(expression.size()));
        payload.insert(payload.end(), expression.begin(), expression.end());
    }
    appendInstruction(out, opcode, payload);
}

// An Else payload: the metadata word only. Measured over the retail corpus, all
// 172 Else instructions are exactly 2 bytes.
void appendElse(std::vector<uint8_t>& out) {
    std::vector<uint8_t> payload;
    appendU16(payload, 1);
    appendInstruction(out, 0x0017, payload);
}

// A Set payload: target tokens, then the expression length and expression.
void appendSet(std::vector<uint8_t>& out, const std::vector<uint8_t>& target,
               const std::vector<uint8_t>& expression) {
    std::vector<uint8_t> payload = target;
    appendU16(payload, static_cast<uint16_t>(expression.size()));
    payload.insert(payload.end(), expression.begin(), expression.end());
    appendInstruction(out, 0x0015, payload);
}

// A postfix expression made of ASCII text operands and operators.
std::vector<uint8_t> textExpression(const std::vector<std::string>& parts) {
    std::vector<uint8_t> out;
    for (const std::string& part : parts) {
        out.push_back(0x20);
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

} // namespace

void ScriptVMTests::testNativeScdaVm() {
    // Test 1: a block runs to its End and stops there
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendInstruction(body, 0x1021, {});  // Enable
        appendInstruction(body, 0x1022, {});  // Disable

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int calls = 0;
        vm.registerCommand(0x1021, [&calls](NativeCommandContext&, const std::vector<NativeToken>&,
                                            ScriptValue&, std::string&) {
            ++calls;
            return true;
        });
        vm.registerCommand(0x1022, [&calls](NativeCommandContext&, const std::vector<NativeToken>&,
                                            ScriptValue&, std::string&) {
            ++calls;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && calls == 2 &&
                        vm.getExecutedInstructionCount() == 2;
        record("NativeVm: block runs to End", ok,
               "A gamemode block executes its commands and stops at End",
               getTimeMs38() - start);
    }

    // Test 2: a block type that is absent reports NotRunning
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, {});

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        const NativeVmResult result = vm.run(program, 0x0003, 0x1234);
        const bool ok = result == NativeVmResult::NotRunning && !vm.getLastError().empty();
        record("NativeVm: missing block type", ok,
               "A block type the program does not contain reports NotRunning",
               getTimeMs38() - start);
    }

    // Test 3: If takes the true branch and skips the else
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendConditional(body, 0x0016, textExpression({"1"}));  // if 1
        appendInstruction(body, 0x1021, {});                     // Enable
        appendElse(body);                                        // else
        appendInstruction(body, 0x1022, {});                     // Disable
        appendInstruction(body, 0x0019, {});                     // endif

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        int disabled = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });
        vm.registerCommand(0x1022, [&disabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                               ScriptValue&, std::string&) {
            ++disabled;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 1 && disabled == 0;
        record("NativeVm: if true branch", ok,
               "A true condition runs the if body and skips the else body",
               getTimeMs38() - start);
    }

    // Test 4: a false condition falls through to the else body
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendConditional(body, 0x0016, textExpression({"0"}));  // if 0
        appendInstruction(body, 0x1021, {});                     // Enable
        appendElse(body);                     // else
        appendInstruction(body, 0x1022, {});                     // Disable
        appendInstruction(body, 0x0019, {});                     // endif

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        int disabled = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });
        vm.registerCommand(0x1022, [&disabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                               ScriptValue&, std::string&) {
            ++disabled;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 0 && disabled == 1;
        record("NativeVm: if false branch", ok,
               "A false condition skips the if body and runs the else body",
               getTimeMs38() - start);
    }

    // Test 5: ElseIf is only evaluated when no earlier branch was taken
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendConditional(body, 0x0016, textExpression({"0"}));  // if 0
        appendInstruction(body, 0x1021, {});                     // Enable
        appendConditional(body, 0x0018, textExpression({"1"}));  // elseif 1
        appendInstruction(body, 0x1022, {});                     // Disable
        appendElse(body);                     // else
        appendInstruction(body, 0x105E, {});                     // Evp
        appendInstruction(body, 0x0019, {});                     // endif

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        int disabled = 0;
        int evp = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });
        vm.registerCommand(0x1022, [&disabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                               ScriptValue&, std::string&) {
            ++disabled;
            return true;
        });
        vm.registerCommand(0x105E, [&evp](NativeCommandContext&, const std::vector<NativeToken>&,
                                          ScriptValue&, std::string&) {
            ++evp;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 0 &&
                        disabled == 1 && evp == 0;
        record("NativeVm: elseif chain", ok,
               "The first true branch wins and later branches are skipped",
               getTimeMs38() - start);
    }

    // Test 6: a stray EndIf at depth zero is a no-op, not an error. The retail
    // corpus ships 43 scripts with an unbalanced block, so rejecting them would
    // drop real content.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendInstruction(body, 0x0019, {});  // endif with no if
        appendInstruction(body, 0x1021, {});  // Enable
        appendInstruction(body, 0x0019, {});  // another stray endif

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 1;
        record("NativeVm: stray endif is a no-op", ok,
               "An EndIf with an empty block stack is ignored and execution continues",
               getTimeMs38() - start);
    }

    // Test 7: a stray Else at depth zero is a no-op too
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendElse(body);  // else with no if
        appendInstruction(body, 0x1021, {});  // Enable

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 1;
        record("NativeVm: stray else is a no-op", ok,
               "An Else with an empty block stack is ignored and execution continues",
               getTimeMs38() - start);
    }

    // Test 8: Set writes a local variable and the value survives the run
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> target;
        target.push_back('f');
        appendU16(target, 3);

        std::vector<uint8_t> body;
        appendSet(body, target, textExpression({"7"}));

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success &&
                        vm.variables().get(3).toInt() == 7;
        record("NativeVm: Set local", ok,
               "Set stores the expression result in the target local variable",
               getTimeMs38() - start);
    }

    // Test 9: the whole measured operator set evaluates. The set is closed: it
    // was measured over every expression in the vanilla corpus.
    {
        const float start = getTimeMs38();
        NativeScdaVm vm;
        std::string error;

        auto eval = [&vm, &error](const std::vector<std::string>& parts, ScriptValue& out) {
            error.clear();
            std::vector<uint8_t> bytes = textExpression(parts);
            std::vector<NativeToken> tokens;
            if (!decodeNativeExpression(bytes.data(), bytes.size(), tokens, error)) {
                return false;
            }
            return vm.evaluateExpression(tokens, out, error);
        };

        ScriptValue value;
        bool ok = true;

        ok = ok && eval({"1", "2", "+"}, value) && value.toInt() == 3;
        ok = ok && eval({"5", "3", "-"}, value) && value.toInt() == 2;
        ok = ok && eval({"4", "3", "*"}, value) && value.toInt() == 12;
        ok = ok && eval({"9", "3", "/"}, value) && value.toInt() == 3;
        ok = ok && eval({"1", "1", "=="}, value) && value.toInt() == 1;
        ok = ok && eval({"1", "2", "!="}, value) && value.toInt() == 1;
        ok = ok && eval({"1", "2", "<"}, value) && value.toInt() == 1;
        ok = ok && eval({"2", "2", "<="}, value) && value.toInt() == 1;
        ok = ok && eval({"3", "2", ">"}, value) && value.toInt() == 1;
        ok = ok && eval({"2", "2", ">="}, value) && value.toInt() == 1;
        ok = ok && eval({"1", "0", "&&"}, value) && value.toInt() == 0;
        ok = ok && eval({"1", "0", "||"}, value) && value.toInt() == 1;
        ok = ok && eval({"0", "~"}, value) && value.toInt() == 1;
        ok = ok && eval({"1", "~"}, value) && value.toInt() == 0;

        record("NativeVm: operator set", ok,
               "All thirteen measured operators evaluate in postfix order",
               getTimeMs38() - start);
    }

    // Test 10: decimal literals keep their fractional part
    {
        const float start = getTimeMs38();
        NativeScdaVm vm;
        std::string error;

        auto eval = [&vm, &error](const std::vector<std::string>& parts, ScriptValue& out) {
            error.clear();
            std::vector<uint8_t> bytes = textExpression(parts);
            std::vector<NativeToken> tokens;
            if (!decodeNativeExpression(bytes.data(), bytes.size(), tokens, error)) {
                return false;
            }
            return vm.evaluateExpression(tokens, out, error);
        };

        ScriptValue value;
        bool ok = true;

        // ".5" is a valid literal in this grammar, and a float operand keeps the
        // result a float. Integer operands stay integral: all seven divisions in
        // the retail corpus are "value / 2" or "value / 4", where truncation is
        // the intended behaviour.
        ok = ok && eval({".5"}, value) && value.type == ScriptValue::Type::Float &&
             std::fabs(value.toFloat() - 0.5f) < 0.0001f;
        ok = ok && eval({"1", "2", "/"}, value) &&
             value.type == ScriptValue::Type::Integer && value.toInt() == 0;
        ok = ok && eval({"1.5", "1.5", "+"}, value) &&
             std::fabs(value.toFloat() - 3.0f) < 0.0001f;

        record("NativeVm: decimal literals", ok,
               "Leading-dot and dotted literals parse and keep float precision",
               getTimeMs38() - start);
    }

    // Test 11: division by zero is an explicit error rather than a crash
    {
        const float start = getTimeMs38();
        NativeScdaVm vm;
        std::string error;
        std::vector<uint8_t> bytes = textExpression({"1", "0", "/"});
        std::vector<NativeToken> tokens;
        ScriptValue value;
        const bool decoded = decodeNativeExpression(bytes.data(), bytes.size(), tokens, error);
        const bool ok = decoded && !vm.evaluateExpression(tokens, value, error) &&
                        !error.empty();
        record("NativeVm: division by zero", ok,
               "Dividing by zero reports an error instead of trapping",
               getTimeMs38() - start);
    }

    // Test 12: an expression function is called with evaluated arguments
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> target;
        target.push_back('f');
        appendU16(target, 1);

        // set x to (GetStage 42) == 3
        std::vector<uint8_t> expression;
        expression.push_back(0x20);
        expression.push_back('X');
        appendU16(expression, 0x103A);  // GetStage
        // argumentBytes spans the whole argument list, including the 2 byte
        // count word: [argc=1] + 'n' + u32 = 2 + 5 = 7.
        appendU16(expression, 7);
        appendU16(expression, 1);
        expression.push_back('n');
        appendU32(expression, 42);
        expression.push_back(0x20);
        expression.push_back('3');
        expression.push_back(0x20);
        expression.push_back('=');
        expression.push_back('=');

        appendSet(body, target, expression);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        uint16_t seenOpcode = 0;
        int seenArg = -1;
        vm.registerExpressionFunction(
            0x103A, [&seenOpcode, &seenArg](NativeCommandContext& context,
                                            const std::vector<ScriptValue>& args,
                                            ScriptValue& returnValue, std::string&) {
                seenOpcode = context.opcode;
                if (!args.empty()) seenArg = args[0].toInt();
                returnValue = ScriptValue::makeInt(3);
                return true;
            });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && seenOpcode == 0x103A &&
                        seenArg == 42 && vm.variables().get(1).toInt() == 1;
        record("NativeVm: expression function", ok,
               "An expression function receives evaluated arguments and returns a value",
               getTimeMs38() - start);
    }

    // Test 13: an unregistered expression function is an explicit error
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> target;
        target.push_back('f');
        appendU16(target, 1);

        std::vector<uint8_t> expression;
        expression.push_back(0x20);
        expression.push_back('X');
        appendU16(expression, 0x103A);
        appendU16(expression, 5);
        appendU16(expression, 1);
        expression.push_back('n');
        appendU32(expression, 42);

        appendSet(body, target, expression);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Error && !vm.getLastError().empty();
        record("NativeVm: unknown expression function", ok,
               "An expression function with no handler reports an error",
               getTimeMs38() - start);
    }

    // Test 14: an unregistered command is counted and does not stop the run
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendInstruction(body, 0x10B8, {});  // unnamed opcode
        appendInstruction(body, 0x1021, {});  // Enable

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 1 &&
                        vm.getUnhandledCommandCount() == 1;
        record("NativeVm: unhandled command", ok,
               "A command with no handler is counted and the block keeps running",
               getTimeMs38() - start);
    }

    // Test 15: the selector marker names the reference a command acts on
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendMarker(body, 2);                // selector slot 2
        appendInstruction(body, 0x1021, {});  // Enable

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        vm.setCallTargets({0x000446A1, 0x000446A7, 0x000446B9});

        uint32_t seenRef = 0;
        uint16_t seenIndex = 0;
        vm.registerCommand(0x1021, [&seenRef, &seenIndex](NativeCommandContext& context,
                                                          const std::vector<NativeToken>&,
                                                          ScriptValue&, std::string&) {
            seenRef = context.referenceFormId;
            seenIndex = context.referenceIndex;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && seenIndex == 2 &&
                        seenRef == 0x000446A7;
        record("NativeVm: selector reference", ok,
               "The marker before a command resolves to the call target it names",
               getTimeMs38() - start);
    }

    // Test 16: a selector outside the call target table is an explicit error
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendMarker(body, 9);
        appendInstruction(body, 0x1021, {});

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        vm.setCallTargets({0x000446A1});
        vm.registerCommand(0x1021, [](NativeCommandContext&, const std::vector<NativeToken>&,
                                      ScriptValue&, std::string&) { return true; });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Error && !vm.getLastError().empty();
        record("NativeVm: selector out of range", ok,
               "A selector past the call target table reports an error",
               getTimeMs38() - start);
    }

    // Test 17: the instruction budget stops a run and it can be resumed
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        for (int i = 0; i < 10; ++i) {
            appendInstruction(body, 0x1021, {});
        }

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int calls = 0;
        vm.registerCommand(0x1021, [&calls](NativeCommandContext&, const std::vector<NativeToken>&,
                                            ScriptValue&, std::string&) {
            ++calls;
            return true;
        });

        const NativeVmResult first = vm.run(program, 0x0000, 0x1234, 0, 4);
        const int callsAfterBudget = calls;
        const NativeVmResult second = vm.resume(100);
        const bool ok = first == NativeVmResult::FrameBudget && callsAfterBudget == 4 &&
                        second == NativeVmResult::Success && calls == 10;
        record("NativeVm: instruction budget", ok,
               "A budgeted run stops and resumes where it left off",
               getTimeMs38() - start);
    }

    // Test 18: Return stops the block before its End
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendInstruction(body, 0x1021, {});  // Enable
        appendInstruction(body, 0x001E, {});  // Return
        appendInstruction(body, 0x1022, {});  // Disable, never reached

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        int disabled = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });
        vm.registerCommand(0x1022, [&disabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                               ScriptValue&, std::string&) {
            ++disabled;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 1 && disabled == 0;
        record("NativeVm: Return stops the block", ok,
               "Return ends the run before the remaining instructions execute",
               getTimeMs38() - start);
    }

    // Test 19: a handler failure surfaces the handler's own message
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendInstruction(body, 0x1021, {});

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        vm.registerCommand(0x1021, [](NativeCommandContext&, const std::vector<NativeToken>&,
                                      ScriptValue&, std::string& error) {
            error = "handler refused";
            return false;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Error &&
                        vm.getLastError() == "handler refused" &&
                        vm.getLastErrorOpcode() == 0x1021;
        record("NativeVm: handler failure", ok,
               "A failing handler reports its message and the failing opcode",
               getTimeMs38() - start);
    }

    // Test 20: a reference literal in an expression resolves through the table
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> target;
        target.push_back('f');
        appendU16(target, 1);

        // set x to (ref literal SCRO[1]) == 0x000446A7
        std::vector<uint8_t> expression;
        expression.push_back(0x20);
        expression.push_back(NATIVE_SCDA_REFERENCE_LITERAL_CHAR);
        appendU16(expression, 2);
        expression.push_back(0x20);
        expression.push_back('n');
        appendU32(expression, 0x000446A7);
        expression.push_back(0x20);
        expression.push_back('=');
        expression.push_back('=');

        appendSet(body, target, expression);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        vm.setReferences({{NativeReferenceKind::Scro, 0x000446A1, 0},
                          {NativeReferenceKind::Scro, 0x000446A7, 0}});

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success &&
                        vm.variables().get(1).toInt() == 1;
        record("NativeVm: reference literal", ok,
               "A reference literal resolves to the SCRO FormID it indexes",
               getTimeMs38() - start);
    }

    // Test 21: a global variable round-trips through the 'G' type char. The
    // global index is a u16, so the FormID must fit in 16 bits.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> target;
        target.push_back('G');
        appendU16(target, 0x2345);

        appendSet(body, target, textExpression({"11"}));

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success &&
                        vm.getGlobal(0x2345).toInt() == 11;
        record("NativeVm: global variable", ok,
               "Set writes a global variable addressed by its FormID",
               getTimeMs38() - start);
    }

    // Test 21b: the remote form of Set writes a variable of another script.
    // Its payload starts with 0x72 and places the expression length after the
    // remote variable index, so the local length equation must not be applied.
    // 0x72 is also the type char of a reference variable, so the local reading
    // is tried first and the remote reading is the fallback.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> payload;
        payload.push_back(NATIVE_SCDA_REMOTE_SET_CHAR);
        appendU16(payload, 1);          // reference slot 1
        payload.push_back('f');         // remote variable type
        appendU16(payload, 0x000B);     // remote variable index
        const std::vector<uint8_t> expression = textExpression({"0"});
        appendU16(payload, static_cast<uint16_t>(expression.size()));
        payload.insert(payload.end(), expression.begin(), expression.end());
        appendInstruction(body, 0x0015, payload);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        const bool framed = program.instructions.size() == 3 &&
                            !program.instructions[1].framingFailed &&
                            program.instructions[1].tokens.size() == 1 &&
                            program.instructions[1].tokens[0].index == 0x000B &&
                            program.instructions[1].expressionDecoded;
        record("NativeDecoder: remote Set", framed,
               "A 0x72 Set payload frames as [refSlot][type][varIndex][elen][expr]",
               getTimeMs38() - start);
    }

    // Test 21c: a payload that starts with 0x72 is always the remote form, even
    // when the local two token reading would also satisfy the length equation.
    // The remote variable index belongs to the target script's SLSD table, so
    // the local reading is never taken for a 0x72 payload.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> payload;
        payload.push_back(NATIVE_SCDA_REMOTE_SET_CHAR);
        appendU16(payload, 1);
        payload.push_back('f');
        appendU16(payload, 8);
        const std::vector<uint8_t> expression = textExpression({"1"});
        appendU16(payload, static_cast<uint16_t>(expression.size()));
        payload.insert(payload.end(), expression.begin(), expression.end());
        appendInstruction(body, 0x0015, payload);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        const bool framed = program.instructions.size() == 3 &&
                            !program.instructions[1].framingFailed &&
                            program.instructions[1].tokens.size() == 1 &&
                            program.instructions[1].tokens[0].typeChar == 'f' &&
                            program.instructions[1].tokens[0].index == 8 &&
                            program.instructions[1].expressionDecoded;
        record("NativeDecoder: remote Set wins", framed,
               "A 0x72 Set payload reads as the remote form, not a local target pair",
               getTimeMs38() - start);
    }

    // Test 21d: a reference operand followed by a local variable operand keeps
    // both tokens. The bytecode for a property access and for two adjacent
    // operands is identical, so the plain reading wins whenever it satisfies the
    // declared count. Folding unconditionally would collapse the pair and break
    // the many argument lists that pass a reference and a local variable.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> payload;
        appendU16(payload, 2);
        payload.push_back('r');
        appendU16(payload, 7);
        payload.push_back('s');
        appendU16(payload, 1);
        appendInstruction(body, 0x1052, payload);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        const bool framed = program.instructions.size() == 3 &&
                            !program.instructions[1].framingFailed &&
                            program.instructions[1].tokens.size() == 2 &&
                            program.instructions[1].tokens[0].typeChar == 'r' &&
                            program.instructions[1].tokens[0].index == 7 &&
                            !program.instructions[1].tokens[0].hasMember &&
                            program.instructions[1].tokens[1].typeChar == 's' &&
                            program.instructions[1].tokens[1].index == 1;
        record("NativeDecoder: adjacent operands", framed,
               "A reference and a local variable stay two operands when argc says two",
               getTimeMs38() - start);
    }

    // Test 21e: the same byte shape folds into one operand when the declared
    // count only fits the folded reading. This is the reference property form,
    // where the member index selects a variable on the referenced object.
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        std::vector<uint8_t> payload;
        appendU16(payload, 1);
        payload.push_back('r');
        appendU16(payload, 5);
        payload.push_back('s');
        appendU16(payload, 9);
        appendInstruction(body, 0x1076, payload);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        const bool framed = program.instructions.size() == 3 &&
                            !program.instructions[1].framingFailed &&
                            program.instructions[1].tokens.size() == 1 &&
                            program.instructions[1].tokens[0].typeChar == 'r' &&
                            program.instructions[1].tokens[0].index == 5 &&
                            program.instructions[1].tokens[0].hasMember &&
                            program.instructions[1].tokens[0].memberIndex == 9;
        record("NativeDecoder: reference property folds", framed,
               "A reference and member fold into one operand when argc says one",
               getTimeMs38() - start);
    }

    // Test 22: a nested if inside a taken branch is skipped as a whole
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendConditional(body, 0x0016, textExpression({"0"}));  // if 0
        appendConditional(body, 0x0016, textExpression({"1"}));  // nested if 1
        appendInstruction(body, 0x1021, {});                     // Enable
        appendInstruction(body, 0x0019, {});                     // nested endif
        appendElse(body);                     // else
        appendInstruction(body, 0x1022, {});                     // Disable
        appendInstruction(body, 0x0019, {});                     // endif

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        int enabled = 0;
        int disabled = 0;
        vm.registerCommand(0x1021, [&enabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                              ScriptValue&, std::string&) {
            ++enabled;
            return true;
        });
        vm.registerCommand(0x1022, [&disabled](NativeCommandContext&, const std::vector<NativeToken>&,
                                               ScriptValue&, std::string&) {
            ++disabled;
            return true;
        });

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success && enabled == 0 && disabled == 1;
        record("NativeVm: nested if skipped", ok,
               "A false outer condition skips a nested if without running its body",
               getTimeMs38() - start);
    }
}

void ScriptVMTests::testNativeScdaBridge() {
    // Test 1: every bridged opcode maps to a FunctionID and is registered
    {
        const float start = getTimeMs38();
        NativeScdaVm vm;
        ScriptFunctions functions;
        const size_t registered = registerNativeFunctionBridge(vm, functions);

        // The bridge only forwards to FunctionIDs that actually have a
        // handler, so the expected count is the number of mapped entries
        // whose FunctionID is implemented.
        size_t expected = 0;
        bool allMapped = true;
        for (uint16_t opcode = 0x1000; opcode <= 0x1170; ++opcode) {
            FunctionID id;
            if (!mapNativeOpcodeToFunctionId(opcode, id)) {
                continue;
            }
            if (!functions.hasFunction(id)) {
                continue;
            }
            ++expected;
            if (!vm.hasCommand(opcode)) {
                allMapped = false;
            }
        }

        const bool ok = registered > 0 && registered == expected && allMapped;
        record("NativeBridge: opcodes registered", ok,
               "Every mapped native opcode with a handler has a forwarding handler",
               getTimeMs38() - start);
    }

    // Test 2: an opcode with no counterpart stays unregistered
    {
        const float start = getTimeMs38();
        FunctionID id;
        const bool mapped = mapNativeOpcodeToFunctionId(0x1004, id);  // Rotate
        const bool ok = !mapped;
        record("NativeBridge: unmapped opcode", ok,
               "An opcode without a FunctionID counterpart is not bridged",
               getTimeMs38() - start);
    }

    // Test 3: a bridged command forwards its selector and arguments
    {
        const float start = getTimeMs38();
        std::vector<uint8_t> body;
        appendMarker(body, 1);  // selector slot 1

        // SetStage <ref> 10 -> [2][<r1>][n 10]
        std::vector<uint8_t> payload;
        appendU16(payload, 2);
        payload.push_back('r');
        appendU16(payload, 1);
        payload.push_back('n');
        appendU32(payload, 10);
        appendInstruction(body, 0x1039, payload);

        std::vector<uint8_t> code;
        appendBeginBlock(code, 0x0000, body);

        const NativeDecodeResult program = decodeNativeScda(code);
        NativeScdaVm vm;
        ScriptFunctions functions;

        // SetStage needs a quest flow controller, so give the bridge a real
        // one with the quest the block targets.
        QuestFlowController questFlowController;
        QuestRecord questRecord;
        questRecord.formID = 0x1234;
        questRecord.fullName = "Bridge Quest";
        questRecord.stages.push_back({10});
        questFlowController.registerQuest(questRecord);
        functions.init(nullptr, nullptr, nullptr, nullptr, &questFlowController);

        registerNativeFunctionBridge(vm, functions);
        vm.setCallTargets({0x1234});
        vm.variables().set(1, ScriptValue::makeRef(0x1234));

        const NativeVmResult result = vm.run(program, 0x0000, 0x1234);
        const bool ok = result == NativeVmResult::Success;
        record("NativeBridge: command forwarded", ok,
               "A bridged command runs through the native VM without error",
               getTimeMs38() - start);
    }

    // Test 4: token conversion preserves literal kinds
    {
        const float start = getTimeMs38();
        NativeCommandContext context;
        context.referenceFormId = 0xABCD;

        std::vector<NativeToken> tokens;
        NativeToken integer;
        integer.kind = NativeTokenKind::Integer;
        integer.intValue = 42;
        tokens.push_back(integer);

        NativeToken text;
        text.kind = NativeTokenKind::String;
        text.text = "hello";
        tokens.push_back(text);

        const std::vector<ScriptValue> args = nativeTokensToArguments(context, tokens);
        const bool ok = args.size() == 3 &&
                        args[0].type == ScriptValue::Type::Ref &&
                        args[0].refVal == 0xABCD &&
                        args[1].toInt() == 42 &&
                        args[2].type == ScriptValue::Type::String &&
                        args[2].strVal == "hello";
        record("NativeBridge: token conversion", ok,
               "The selector becomes a Ref argument and literals keep their kind",
               getTimeMs38() - start);
    }

    // Test 5: a variable token reads its slot from the VM store instead of
    // passing the slot index through as a value.
    {
        const float start = getTimeMs38();
        NativeVariableStore store;
        store.set(3, ScriptValue::makeRef(0x5678));

        NativeCommandContext context;
        context.variables = &store;

        std::vector<NativeToken> tokens;
        NativeToken variable;
        variable.kind = NativeTokenKind::Variable;
        variable.index = 3;
        variable.intValue = 3;
        tokens.push_back(variable);

        const std::vector<ScriptValue> args = nativeTokensToArguments(context, tokens);
        const bool ok = args.size() == 1 &&
                        args[0].type == ScriptValue::Type::Ref &&
                        args[0].refVal == 0x5678;
        record("NativeBridge: variable token", ok,
               "A variable token resolves to its stored value, not its slot index",
               getTimeMs38() - start);
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
    testNativeScdaDecoder();
    testNativeScdaVm();
    testNativeScdaBridge();

    TEST_LOGI("========================================");
    TEST_LOGI("Results: %d passed, %d failed, %zu total",
              getPassCount(), getFailCount(), results.size());
    TEST_LOGI("========================================");

    return getFailCount() == 0;
}

std::string ScriptVMTests::getSummary() const {
    std::ostringstream ss;
    ss << "=== Phase 38 Script VM Test Results ===\n";
    ss << "Total: " << results.size()
       << " | Pass: " << getPassCount()
       << " | Fail: " << getFailCount() << "\n\n";

    for (const auto& r : results) {
        ss << (r.passed ? "[PASS]" : "[FAIL]") << " " << r.testName;
        if (r.durationMs > 0.0f) {
            ss << " (" << r.durationMs << " ms)";
        }
        if (!r.message.empty()) {
            ss << " - " << r.message;
        }
        ss << "\n";
    }

    return ss.str();
}
