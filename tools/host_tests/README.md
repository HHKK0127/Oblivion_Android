# Host Test Runner (Native C++ tests on desktop CI)

The Android build compiles the native C++ engine with the NDK. Some test suites
are pure logic (no GLES surface, no assets) and can be compiled and executed on
a plain desktop host. This directory contains everything needed to run them in
CI without an Android device or emulator.

## What is covered

Currently the **Phase 38 Script VM** suite (`tests/script_vm_tests.cpp`) is
wired up. It exercises the script bytecode interpreter
(`ExecutionContext`, `ScriptVM`, opcode execution) and the native game
function registry (`ScriptFunctions`) with zero hardware dependencies.

## Layout

- `stubs/` — minimal re-implementations of NDK/JNI headers needed to compile
  engine sources on a host: `<android/log.h>`, `<android/asset_manager.h>`,
  `<jni.h>`, and GLES stubs. They are compile-time only; symbols used at link
  time come from `host_stub_syms.cpp`.
- `host_stub_syms.cpp` — `extern "C"` stub definitions for the `AAsset*` API and
  `jni_audio_*` bridge functions.
- `run_host_tests.sh` — build + run on CI (ubuntu-latest). Fails the job on any
  suite failure or build error.

## Usage (local)

```sh
# From the repo root (needs a C++17 compiler):
bash tools/host_tests/run_host_tests.sh
```

Configured include flags are the same ones the Android build uses
(`-DAUDIO_SYSTEM_ENABLED -DJPH_PROFILE_ENABLED`), plus `-include` headers to
compensate for sources that rely on transitive includes.

## Expanding coverage

When a suite needs no hardware/asset access, add its `runAllTests()` to
`host_runner_main.cpp` (it returns exit code 1 on any suite failure) and list
its dependency `.cpp` files in `run_host_tests.sh`. If a suite needs symbols
you have not stubbed yet, follow the pattern of `host_stub_syms.cpp`.