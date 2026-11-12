# V206 — CI host tests 残障害調査（msvc_compat.h / OBLIVION_RUST_DIR）

- 作成: 2026-10-31 / opencode [RE] 調査専任セッション（TODO serial 193 / V193 承継）
- 性質: **調査のみ**。ソース・workflow・ツールの修正は未実施（L2 = 人間承認待ち）。
- 入力: `tools/host_tests/msvc_compat.h`、`tools/host_tests/stubs/dirent.h`、
  `tools/host_tests/run_host_tests.sh`、`tools/host_tests/run_host_tests_msvc.ps1`、
  `.github/workflows/android.yml`、`tmp/re/ci_fail_analysis_20261028.md`、`STATE.md`、
  `%TEMP%\opencode\host_tests_real_final3.log`、`app/src/main/cpp/script/script_opcodes.h`
- 参照 git 状態（read-only）: HEAD = `19fafb7f` "Save uncommitted changes"（調査時点）。

---

## サマリ

| # | 調査項目 | 判定 |
|---|---|---|
| 1 | msvc_compat.h の `<windows.h>` 無条件 include は**未修正**（修正ドラフト下記） | 修正未適用・ドラフト作成済み |
| 2 | CI に `oblivion-re-tools` の checkout なし・`OBLIVION_RUST_DIR` 未設定 | **ワークフロー追加が必須**（ただし前提条件あり） |
| 3 | 証跡 `host_tests_real_final3.log` | **実在**（140,276 bytes、ALL GREEN） |
| 4 | enum 12→9 値・Rust 移行4点の適用済み確認 | 適用済み（確認のみ・対象外） |

---

## 1. `tools/host_tests/msvc_compat.h` — 現況と修正ドラフト

### 1-1. 現況判定（未修正を確認）

`git status --porcelain` で以下が **HEAD と差異なし**を確認（= 修正ドラフトは適用されていない）:

- `tools/host_tests/msvc_compat.h` — 変更なし
- `tools/host_tests/stubs/dirent.h` — 変更なし
- `.github/workflows/android.yml` — 変更なし
- `app/src/main/cpp/script/script_opcodes.h` — 変更なし（9 値 enum は HEAD に存在）

現行 `msvc_compat.h` の問題箇所:

- L31: `#include <windows.h>` が **無条件**（`_WIN32_DIRENT_H` センチル内だが `_WIN32` ガードなし）。
- L20–89 の dirent シム（`struct DIR` / `opendir` / `readdir` / `closedir`）は Win32 API 依存で全体が Windows 固有。
- Linux CI での失敗経路（`tmp/re/ci_fail_analysis_20261028.md` #2 と一致）:
  `asset_manager.cpp` の `#include <dirent.h>` → `stubs/dirent.h`（CI が `-Itools/host_tests/stubs` を付与）→ `../msvc_compat.h` → `windows.h` が存在せずコンパイル失敗。
- 上部の `S_ISDIR`/`S_ISREG` ブロック（L10–18）は `#ifndef` ガード付きで、Linux の `<sys/stat.h>` では no-op になるため**無害**。ガード対象は dirent ブロック（L20–89）だけで十分。

### 1-2. 修正リスク検証（CMake / CI 設定の読取結果）

| 観点 | 検証結果 | リスク |
|---|---|---|
| MSVC ビルド（`run_host_tests_msvc.ps1`） | L236 が `/FI msvc_compat.h` で強制 include。cl.exe は常に `_WIN32` を定義 → ガード後も**ビルド挙動不変**。L216 で `/I stubs` を追加付与（`.sh` 由来の INCLUDES とは独立） | なし |
| MinGW ビルド（ローカル Git Bash の `run_host_tests.sh`） | MinGW g++ は `_WIN32` を定義 → ガード後も Win32 シム側（現行挙動）を踏む。現行 `.sh` ビルドは既にこの経路（MinGW の system dirent.h ではなくシム）で通っている | なし（挙動不変） |
| Linux CI ビルド（ubuntu-latest g++） | ガード後は `stubs/dirent.h` が `#include_next <dirent.h>` で `/usr/include/dirent.h` へパススルー。GCC は `-I` ディレクトリ以降の検索を続けるため解決可 | 低（`include_next` は GCC/Clang 拡張。MSVC は非対象だが `_WIN32` 分岐のため未コンパイル） |
| include path 設定 | `.sh` の `INCLUDES` 配列 L78 `-Itools/host_tests/stubs` は `android/log.h`・`jni.h`・`asset_manager.h` 用に**残す必要**がある（stubs ディレクトリごとの除去は不可）→ `stubs/dirent.h` 側のパススルーが必須 | 方針 B（stubs 除外）は不可と判定 |
| CMake（APK ビルド） | `CMakeLists.txt`（ルート）に `OBLIVION_RUST_DIR` なし。APK 側は `app/src/main/cpp/CMakeLists.txt` が cargo で直接ビルドする独立経路。host tests には CMake 非依存 | なし |

### 1-3. 修正ドラフト A: `tools/host_tests/msvc_compat.h`（before→after 全文）

**Before（現行全文 89 行）** — 調査時点の原文（冒頭 L1–18 は after と同一、L20–89 が差分対象）。

**After（全文・修正ドラフト）:**

```c
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
// tools/host_tests/stubs/dirent.h (put on the include path by both runners)
// redirects #include <dirent.h> to this header when _WIN32 is defined.
//
// The whole block is guarded with _WIN32: it needs <windows.h>, which does
// not exist on the Linux CI runner, and every type/function here is Win32-
// specific. On a non-Windows build this header compiles to just the S_IS*
// fallbacks above, and stubs/dirent.h falls through to the system <dirent.h>.
#ifndef _WIN32_DIRENT_H
#define _WIN32_DIRENT_H

#ifdef _WIN32

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

#endif // _WIN32

#endif // _WIN32_DIRENT_H
```

差分要点: L20–30 のコメントをガード意図で書き換え、L31 以前に `#ifdef _WIN32` を追加、シム実体（L31–87）は一字一句変えず、末尾に `#endif // _WIN32` を追加。`_WIN32_DIRENT_H` センチルは既存の include ガードとして残置。

### 1-4. 修正ドラフト B: `tools/host_tests/stubs/dirent.h`（before→after 全文）

**Before（現行全文 6 行）:**

```c
// MSVC dirent stub — redirects to the msvc_compat.h shim.
// asset_manager.cpp does #include <dirent.h>; MSVC has no such header.
// The MSVC runner adds tools/host_tests/stubs to the include path so this
// file is found before the (non-existent) system dirent.h.
#pragma once
#include "../msvc_compat.h"
```

**After（全文・修正ドラフト）:**

```c
// MSVC dirent stub — redirects to the msvc_compat.h shim on Windows.
// asset_manager.cpp does #include <dirent.h>; MSVC has no such header.
// The MSVC runner adds tools/host_tests/stubs to the include path so this
// file is found before the (non-existent) system dirent.h.
//
// On non-Windows builds (Linux CI) this stub must not drag in the Win32
// shim: #include_next continues the angle-bracket search after this
// directory, so the system <dirent.h> (/usr/include) is found even though
// this stub shadows it. GCC and Clang support #include_next; MSVC does not,
// but MSVC defines _WIN32 and therefore never compiles that line.
#pragma once
#ifdef _WIN32
#include "../msvc_compat.h"   // Win32 FindFirstFile-based shim (MSVC + MinGW)
#else
#include_next <dirent.h>      // GCC/Clang: pass through to the system header
#endif
```

### 1-5. 検証手順（承認後の実施項目・実行はしていない）

1. Linux 相当: `docker run ubuntu:24.04`（または WSL）で `bash tools/host_tests/run_host_tests.sh`。
   ※現行 `.sh` は Rust FFI も必須（後述 #2）。msvc_compat 単体検証には Rust checkout が前提。
2. MSVC: `powershell -File tools/host_tests/run_host_tests_msvc.ps1`（ガード後も `_WIN32` 分岐＝現行同一）。
3. MinGW: Git Bash で `bash tools/host_tests/run_host_tests.sh`（挙動不変のはず）。

---

## 2. CI の checkout 構成と `OBLIVION_RUST_DIR` — 現況と要否裁定

### 2-1. 現況（`.github/workflows/` 全件走査）

`.github/workflows/` 配下の YAML は **`android.yml` の 1 件のみ**（`github-app.yml` は workflow ではない）。

`android.yml` の `host-tests` job（L107–129）の構成:

| 観点 | 現況 |
|---|---|
| (a) checkout | `actions/checkout@v5` は **Oblivion_Android 本体のみ**。兄弟リポジトリ `oblivion-re-tools` の checkout は**一切ない**（2つ目の checkout step も submodule もなし） |
| (b) `OBLIVION_RUST_DIR` | job 内に env / `GITHUB_ENV` 書き込み**なし**。Rust ツールチェーン setup（rustup / cargo build）の step も**なし** |
| (c) include path / 環境変数 | step は checkout → `zlib1g-dev` → `bash tools/host_tests/run_host_tests.sh` の 3 つだけ。include path はスクリプト内 `INCLUDES` 配列が担う（`$RUST_DIR/crates/*/include` を含む） |

### 2-2. 重要な前提事実（本追加調査で判明）

1. **HEAD committed の `.sh` は Rust 非対応**: `git show HEAD:tools/host_tests/run_host_tests.sh` に `OBLIVION_RUST`/`RUST_`/`assets_ffi` の文字列は**0件**。現行 working tree の Rust 対応（`OBLIVION_RUST_DIR` フォールバック、`crates/*/include`、`libassets_ffi.a` リンク等）は **+85 行の未コミット差分**（`git diff --stat HEAD` = 85 insertions）。
2. **HEAD committed の C++ は既に Rust FFI ヘッダを include 済み**: `asset_path_resolver.cpp`/`mesh_builder.cpp`（`assets_ffi.h`）、`bsa_reader.cpp`（`bsa_ffi.h`）、`formid_index.cpp`/`plugin_list.cpp`（`plugin_ffi.h`）が HEAD に含まれる。→ **msvc_compat だけ直しても、現行 HEAD では次に `assets_ffi.h` not found で host tests は失敗する**（`tmp/re/ci_fail_analysis_20261028.md` の "died on assets_ffi.h and then on the FFI symbols" 記述と整合）。
3. **`oblivion-re-tools` は GitHub に存在しない**: ローカル `C:\Users\hiroki.kogarumai\oblivion-re-tools` は git リポジトリだが **remote 設定なし**（`.git/config` に `[remote]` なし）、STATE.md 記載のとおり **zero commits**。`https://github.com/HHKK0127/oblivion-re-tools` は **404**。→ CI（Actions runner）から**取得する術がない**。これが workflow 追加の**最大のブロッカー**。

### 2-3. 裁定: workflow 追加は **必須（ただし前提条件あり）**

- run 40164723857（10-25, `85a057c7`）時点の CI 失敗は msvc_compat が先に出たが、**C++ 側は既に Rust ヘッダ依存**。したがって CI の host tests job を緑にするには **(1) msvc_compat ガード修正 ＋ (2) Rust 対応 `.sh` のコミット ＋ (3) workflow に oblivion-re-tools checkout ＋ Rust ビルド ＋ `OBLIVION_RUST_DIR` export の 3 点が揃う必要**がある。どれか 1 つでも欠ければ再び失敗する。
- **前提条件（人間判断が必要）**: `oblivion-re-tools` を GitHub へ公開（または fine-grained PAT で private アクセス許可）し、zero commits の状態を解消すること。現状のままだと workflow 追加は実行時 `checkout` で必ず失敗する。
- 個人リポジトリの扱い（public 化するか PAT 秘匿情報で private checkout するか）は L1 では決められない。下記ドラフトは private 前提の PAT 方式＋ public なら token 行削除で動く両対応とする。

### 2-4. 追加ドラフト（編集しない）: `android.yml` `host-tests` job への step 追加

`host-tests` job の `steps:` 内、`Ensure zlib development headers` と `Run native host tests` の**間**に挿入:

```yaml
      # The host runner links the Rust C-ABI archives and needs the FFI
      # headers on the include path (see run_host_tests.sh). The runner has no
      # sibling checkout of oblivion-re-tools, so fetch it here and point
      # OBLIVION_RUST_DIR at it.
      #
      # Prerequisite: oblivion-re-tools must be pushed to GitHub. For a
      # private repo store a fine-grained PAT (Contents: read) as the
      # OBIVION_RE_TOOLS_TOKEN secret; for a public repo delete the token
      # line and the secret reference.
      - name: Checkout oblivion-re-tools (Rust FFI)
        uses: actions/checkout@v5
        with:
          repository: HHKK0127/oblivion-re-tools
          path: oblivion-re-tools
          token: ${{ secrets.OBLIVION_RE_TOOLS_TOKEN }}

      - name: Install Rust toolchain
        run: rustup toolchain install stable --profile minimal && rustup default stable

      # The three archives the host link needs (assets/bsa/plugin) plus the
      # optional esm/nif ones; --release matches the paths run_host_tests.sh
      # probes (target/<triple>/release, target/release).
      - name: Build Rust FFI archives (host triple)
        working-directory: oblivion-re-tools
        run: cargo build --release -p assets-ffi -p bsa-ffi -p plugin-ffi -p esm-ffi -p nif-ffi

      - name: Export OBLIVION_RUST_DIR
        run: echo "OBLIVION_RUST_DIR=$GITHUB_WORKSPACE/oblivion-re-tools" >> "$GITHUB_ENV"
```

補足（採用判断の材料）:

- `run_host_tests.sh` は `RUST_DIR="${OBLIVION_RUST_DIR:-$(cd .. && pwd)/oblivion-re-tools}"` を既に持ち、`crates/{assets,bsa,plugin,esm,nif}-ffi/include` と `target/*/{triple,}release` を探査するため、上記 export だけで追加のパラメータは不要。
- `cargo build -p …` の `-p` パッケージ名は実リポジトリ（`crates/*/Cargo.toml` の `name = "assets-ffi"` 等）で確認済み。ワークスペース全体 `cargo build --release` でも可（`cli` 等不要 crate もビルドされる分だけ遅い）。
- ubuntu-latest は rustup 同梱だが、明示 install で image 依存をなくせる。`dtolnay/rust-toolchain@stable` action 方式でも同等。
- 現行 `.sh`（未コミット +85 行版）は `.a` を直接リンクするため Rust リンカー（`x86_64-unknown-linux-gnu` staticlib）で問題なし。静的アーカイブはホスト triple で build すれば `target/release` に落ち、スクリプトの第3候補パスで検出される。

---

## 3. 証跡確認: `%TEMP%\opencode\host_tests_real_final3.log`

**実在を確認**（サイズ 140,276 bytes、mtime 2026-10-30 15:18:04）。末尾 20 行のサマリ:

```
Phase 30 Integration Test Results
================================
[PASS] SKIP_Assets_Unavailable - SKIPPED: assets not available

1/1 passed (0 failed)

--- SUITE RESULT: Phase30IntegrationTest PASSED ---
[LodBlockTests] PASS BlockNameGrammar - 15 names classified as expected (0 ms)
[LodBlockTests] PASS BlockVertexPacking (0 ms)
[LodBlockTests] PASS BlockPackingRejections (8 ms)
[LodBlockTests] 0 landscape LOD files found, 0 sampled (limit 100)
[LodBlockTests] PASS SKIP_Assets_Unavailable - SKIPPED: assets not available - extracted meshes\landscape\lod NIF files are required (set OBLIVION_LOD_NIF_DIR or OBLIVION_ASSET_BASE) (0 ms)

--- SUITE: LodBlockTests ---
LOD Block Test Results
Total: 4 | Pass: 4 | Fail: 0
--- SUITE RESULT: LodBlockTests PASSED ---

=== HOST RUNNER: ALL GREEN (0 suite(s) failed) ===
::endgroup::
```

判定: ローカル（Windows/MinGW）で **ALL GREEN**。ローカル成立 ≠ CI 成立（CI は Linux + oblivion-re-tools checkout なし = 2-2 の壁）。

---

## 4. 適用済み・対象外項目の確認

| 項目 | 確認結果 |
|---|---|
| enum 12→9 値（`NativeScdaOpcode`） | `app/src/main/cpp/script/script_opcodes.h` L153–166: `Stop/Marker/Prologue/Activate/SetActorValue/PlayGroup/Cast/SetStage/MoveTo` の **9 値**。12 値時代の値は不在。**適用済み・対象外** |
| Rust 移行4点（`.sh` の Rust 対応） | working tree の `tools/host_tests/run_host_tests.sh` に `OBLIVION_RUST_DIR` フォールバック（L25）、rustc triple 探査（L26–39）、`have_rust_archives` 検証（L42–47）、`crates/*/include` 5本（L83–87）、`lib*_ffi.a` リンク＋esm/nif optional（L246–257）を確認。**適用済み（ただし未コミット）・対象外** |
| 補足 | `tools/run_host_tests.sh` は存在せず、実体は `tools/host_tests/run_host_tests.sh`（`.github/workflows/android.yml` L129 も同パスを起動）。タスク記載の `tools/run_host_tests.sh` はこれの意図 |

---

## 5. 人間承認チェックリスト（L2 前提）

- [ ] **A. msvc_compat ガード修正の承認**: 1-3 / 1-4 の 2 ファイル before→after を適用してよいか（Linux CI 復旧の最小修正）
- [ ] **B. Rust 対応 `.sh`（+85行）のコミット判断**: working tree の未コミット `run_host_tests.sh` をコミットするか（A 単独では `assets_ffi.h` not found で CI は再失敗する）
- [ ] **C. `oblivion-re-tools` の公開方針**: GitHub に公開（public）するか / private + fine-grained PAT（secret `OBLIVION_RE_TOOLS_TOKEN`）にするか / CI から Rust を外す方向（`-DRUST_DISABLED` 等の別設計）を選ぶか
- [ ] **D. workflow 追加の承認**: 2-4 の step 追加を `android.yml` に入れてよいか（C の方針確定後）
- [ ] **E. 検証**: A+B+D 適用後、Linux 相当（CI run または WSL/コンタナ）＋ MSVC（`run_host_tests_msvc.ps1`）の両ビルド通過を確認
- [ ] **F. Watch List との兼ね合え**: 作業ツリーに 40+ ファイルの未コミット変更あり（STATE.md Watch List）。B のコミットが他の未コミット作業と混ざらないか事前確認
- [ ] **G. `oblivion-re-tools` の zero commits 解消**: push 前に main へ initial commit を作る必要あり（STATE.md 2026-10-29/30 記載の worktree 不可問題と同根）

---
*本ファイルは V206 の調査成果物（新規・UTF-8）。ソース/CI/ツール類の編集は一切行っていない。*
