#pragma once
#include <cstdio>
#include <sys/stat.h>

static constexpr bool DEBUG_ENABLED = false;

inline void debugLog(const char* msg) {
    if constexpr (!DEBUG_ENABLED) return;
    static bool dirReady = false;
    if (!dirReady) { mkdir("/config/spotify-switch", 0777); dirReady = true; }
    FILE* f = fopen("/config/spotify-switch/debug.log", "a");
    if (f) { fputs(msg, f); fputc('\n', f); fclose(f); }
}
