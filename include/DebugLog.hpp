#pragma once
#include <cstdio>
#include <sys/stat.h>

inline void debugLog(const char* msg) {
    static bool dirReady = false;
    if (!dirReady) { mkdir("/config/spotify-switch", 0777); dirReady = true; }
    FILE* f = fopen("/config/spotify-switch/debug.log", "a");
    if (f) { fputs(msg, f); fputc('\n', f); fclose(f); }
}
