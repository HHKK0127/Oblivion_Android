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
