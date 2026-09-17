#pragma once

// ============================================================================
// Crash Handler
// Catches SIGSEGV/SIGABRT and writes crash dump to app storage
// ============================================================================

#include <string>

namespace oblivion {
namespace debug {

class CrashHandler {
public:
    // Install signal handlers (call once at startup)
    static void install();

    // Check if a crash dump exists from previous run
    static bool hasCrashDump();

    // Get path to crash dump
    static std::string getCrashDumpPath();

    // Read crash dump content
    static std::string readCrashDump();

    // Delete crash dump after reporting
    static void clearCrashDump();

private:
    static void handler(int sig, void* addr, void* uc);
    static void writeDump(int sig, void* addr);
};

} // namespace debug
} // namespace oblivion
