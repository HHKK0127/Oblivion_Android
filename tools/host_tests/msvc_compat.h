// MSVC compatibility shims for the POSIX-isms the host suites use.
//
// run_host_tests.sh builds these same sources with g++, whose <sys/stat.h>
// provides S_ISDIR/S_ISREG. MSVC's <sys/stat.h> only offers the raw _S_IF*
// constants, so the macros are defined here and force-included (/FI) by
// run_host_tests_msvc.ps1. Keeping the shim in a header means the suite
// sources stay toolchain-neutral.
#pragma once

#include <sys/stat.h>

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#endif

// Windows dirent shim for asset_manager.cpp (which uses opendir/readdir).
// MSVC does not ship <dirent.h>; this minimal implementation wraps the
// Win32 FindFirstFile/FindNextFile API.
// We also redirect #include <dirent.h> to this header by defining a sentinel
// before the system include path can be searched.
#ifndef _WIN32_DIRENT_H
#define _WIN32_DIRENT_H

// Intercept #include <dirent.h> — MSVC has no such header, so we provide
// our own. The /FI force-include of this file ensures the sentinel is
// defined before any TU tries to include <dirent.h>.
#include <windows.h>
// Windows.h defines 'near' as an empty macro, which breaks identifiers used
// as function names (e.g. test helper `near()`). Undefine it here.
// Note: 'far' is deliberately NOT undefined because zlib's zconf.h maps
// FAR -> far on MSVC and relies on the empty-macro expansion.
#undef near

// WinBase.h defines IGNORE as 0 (the console ctrl handler signal), which
// rewrites `IGNORE = 0` in game/crime_reputation_system.h's GuardResponse
// enum into `0 = 0` and collapses the whole header. The host suites never
// use the Win32 signal constants, so drop the macro. Engine sources stay
// toolchain-neutral; the shim absorbs the collision.
#undef IGNORE
#include <cstring>
#include <string>

struct dirent {
    char d_name[260];
};

struct DIR {
    HANDLE handle;
    WIN32_FIND_DATAA findData;
    dirent entry;
    bool first;
};

static inline DIR* opendir(const char* path) {
    DIR* dir = new DIR();
    std::string pattern = std::string(path) + "\\*";
    dir->handle = FindFirstFileA(pattern.c_str(), &dir->findData);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        delete dir;
        return nullptr;
    }
    dir->first = true;
    return dir;
}

static inline dirent* readdir(DIR* dir) {
    if (dir->first) {
        dir->first = false;
    } else {
        if (!FindNextFileA(dir->handle, &dir->findData)) {
            return nullptr;
        }
    }
    std::strncpy(dir->entry.d_name, dir->findData.cFileName, sizeof(dir->entry.d_name) - 1);
    dir->entry.d_name[sizeof(dir->entry.d_name) - 1] = '\0';
    return &dir->entry;
}

static inline int closedir(DIR* dir) {
    FindClose(dir->handle);
    delete dir;
    return 0;
}

#endif // _WIN32_DIRENT_H
