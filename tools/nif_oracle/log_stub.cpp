// Host stub: forwards __android_log_print to stderr so oracle stdout stays clean.
#include <android/log.h>

#include <cstdarg>
#include <cstdio>

extern "C" int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
    (void)prio;
    std::fprintf(stderr, "[%s] ", tag ? tag : "?");
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fputc('\n', stderr);
    return 0;
}
