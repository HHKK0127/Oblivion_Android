// ============================================================================
// Crash Handler Implementation
// Phase 56: Signal-based crash detection and state dump
// ============================================================================

#include "crash_handler.h"
#include <android/log.h>
#include <csignal>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

#define LOG_TAG_CRASH "CrashHandler"

namespace oblivion {
namespace debug {

static const char* CRASH_DIR = "/data/data/com.example.oblivion/files/";
static const char* CRASH_FILE = "crash_dump.log";

static struct sigaction s_oldSigsegv;
static struct sigaction s_oldSigabrt;
static volatile sig_atomic_t s_fault = 0;
static void* s_faultAddr = nullptr;

void CrashHandler::handler(int sig, void* addr, void* uc) {
    s_fault = sig;
    s_faultAddr = addr;
    writeDump(sig, addr);
    _exit(1);
}

void CrashHandler::writeDump(int sig, void* addr) {
    char path[512];
    snprintf(path, sizeof(path), "%s%s", CRASH_DIR, CRASH_FILE);

    FILE* f = fopen(path, "w");
    if (!f) return;

    time_t now = time(nullptr);
    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(f, "=== CRASH DUMP ===\n");
    fprintf(f, "Time: %s\n", timeBuf);
    fprintf(f, "Signal: %d (%s)\n", sig,
            sig == SIGSEGV ? "SIGSEGV" :
            sig == SIGABRT ? "SIGABRT" :
            sig == SIGFPE  ? "SIGFPE"  :
            sig == SIGBUS  ? "SIGBUS"  :
            "UNKNOWN");
    fprintf(f, "Fault address: %p\n", addr);
    fprintf(f, "\n");
    fprintf(f, "=== SYSTEM INFO ===\n");
    fprintf(f, "PID: %d\n", getpid());
    fprintf(f, "UID: %d\n", getuid());

    // Attempt to get backtrace (limited on Android without libunwind)
    fprintf(f, "\n=== BACKTRACE ===\n");
    fprintf(f, "(Use ndk-stack with the APK's unstripped .so for symbolic trace)\n");

    fclose(f);

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_CRASH,
        "CRASH: signal %d addr=%p - dump written to %s", sig, addr, path);
}

void CrashHandler::install() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = (void (*)(int, siginfo_t*, void*))handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, &s_oldSigsegv);
    sigaction(SIGABRT, &sa, &s_oldSigabrt);

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG_CRASH,
        "Crash handler installed (SIGSEGV, SIGABRT)");
}

bool CrashHandler::hasCrashDump() {
    char path[512];
    snprintf(path, sizeof(path), "%s%s", CRASH_DIR, CRASH_FILE);
    struct stat st;
    return stat(path, &st) == 0 && st.st_size > 0;
}

std::string CrashHandler::getCrashDumpPath() {
    return std::string(CRASH_DIR) + CRASH_FILE;
}

std::string CrashHandler::readCrashDump() {
    char path[512];
    snprintf(path, sizeof(path), "%s%s", CRASH_DIR, CRASH_FILE);
    FILE* f = fopen(path, "r");
    if (!f) return "";

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string content(size, '\0');
    fread(&content[0], 1, size, f);
    fclose(f);
    return content;
}

void CrashHandler::clearCrashDump() {
    char path[512];
    snprintf(path, sizeof(path), "%s%s", CRASH_DIR, CRASH_FILE);
    unlink(path);
}

} // namespace debug
} // namespace oblivion
