#pragma once
#include <cstdio>
#include <cstdarg>
#include <sys/stat.h>

static constexpr bool DEBUG_ENABLED = false;

inline void debugLog(const char* msg) {
    if constexpr (!DEBUG_ENABLED) return;
    static bool dirReady = false;
    if (!dirReady) { mkdir("/config/spotify-switch", 0777); dirReady = true; }
    FILE* f = fopen("/config/spotify-switch/debug.log", "a");
    if (f) { fputs(msg, f); fputc('\n', f); fclose(f); }
}

inline void debugLogf(const char* fmt, ...) {
    if constexpr (!DEBUG_ENABLED) return;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    debugLog(buf);
}
