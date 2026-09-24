#pragma once
// Host-side stub: emulates the NDK android/log.h API for desktop CI builds.
#include <cstdio>
#include <cstdarg>
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_WARN 5
#define ANDROID_LOG_ERROR 6
#define ANDROID_LOG_DEBUG 3
static inline int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
    (void)prio;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "[%s] ", tag);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
    return 0;
}
static inline int __android_log_write(int prio, const char* tag, const char* msg) {
    (void)prio;
    std::fprintf(stdout, "[%s] %s\n", tag, msg);
    return 0;
}
typedef enum { ANDROID_LOG_UNKNOWN=0 } android_LogPriority;
