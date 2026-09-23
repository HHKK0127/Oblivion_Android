#include <cstdio>
#include <string>
#include "tests/script_vm_tests.h"

int main() {
    int failed = 0;
    {
        ScriptVMTests t;
        bool ok = t.runAllTests();
        std::string s = t.getSummary();
        std::printf("%s\n", s.c_str());
        if (!ok) failed++;
    }
    std::printf("\n=== HOST RUNNER: %s (%d suite(s) failed) ===\n", failed ? "FAILED" : "ALL GREEN", failed);
    return failed != 0 ? 1 : 0;
}
