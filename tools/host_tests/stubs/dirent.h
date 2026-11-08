// MSVC dirent stub — redirects to the msvc_compat.h shim.
// asset_manager.cpp does #include <dirent.h>; MSVC has no such header.
// The host runners add tools/host_tests/stubs to the include path, so keep
// this wrapper MSVC-only and defer to the platform header elsewhere.
#pragma once

#if defined(_MSC_VER)
#include "../msvc_compat.h"
#else
#include_next <dirent.h>
#endif