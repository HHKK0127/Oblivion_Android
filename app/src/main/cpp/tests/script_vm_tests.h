#pragma once

#include <string>
#include <vector>

// ============================================================================
// Phase 38: Script VM Unit Tests
// Tests the Oblivion Script VM bytecode interpreter and game functions
// ============================================================================

struct ScriptVMTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class ScriptVMTests {
public:
    ScriptVMTests();
    ~ScriptVMTests();

    // Run all tests
    bool runAllTests();

    // Get results
    const std::vector<ScriptVMTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;

private:
    std::vector<ScriptVMTestResult> results;

    // Test groups
    void testExecutionContext();
    void testScriptVM();
    void testOpcodes();
    void testScriptFunctions();
    void testScriptManager();

    // Helper
    void record(const std::string& name, bool passed,
                const std::string& msg = "", float ms = 0.0f);
};
