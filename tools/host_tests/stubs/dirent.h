// MSVC dirent stub — redirects to the msvc_compat.h shim.
// asset_manager.cpp does #include <dirent.h>; MSVC has no such header.
// The MSVC runner adds tools/host_tests/stubs to the include path so this
// file is found before the (non-existent) system dirent.h.
#pragma once
#include "../msvc_compat.h"