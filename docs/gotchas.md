# docs/gotchas.md — ドメイン知識（症状 / 原因 / 正しい手順 / 確認方法）

AGENTS.md の Learnings（振る舞い）とは別。ここは**ドメイン知識**を固定書式で置く。
**確認方法が書けない項目は載せない**（載せた時点で腐る）。

---

## 1. NIF ヘッダで `unknown_field` と BzString×3 を飛ばさない

- **症状**: NIF のヘッダからブロック型表を読むと `num_block_types` が巨大（例 11）になり、
  `sized string too long` で落ちる。生 NIF コーパスで「型が 0 件」になる。
- **原因**: `Gamebryo File Format` 20.0.0.x のヘッダは version の後に
  `endian(u8) → user_version(u32) → num_objects(u32) → unknown_field(u32) → creator/process/export の BzString×3`
  が続く。`unknown_field` と 3 文字列を飛ばさずに型数を読むと位置がずれる。
- **正しい手順**: `rust/crates/nif-core/src/header.rs::read_header` に一致させる
  （`unknown_field` を 1 つ読み、BzString を 3 つ読んでから型表へ）。
- **確認方法**: `python scripts/nif_coverage.py --corpus meshes_extracted` が
  `files: 8005 (0 unreadable)` を出す（1 件でも `unreadable` ならずれている）。

## 2. NIF の `num_block_types` は u16

- **症状**: 型数を u32 で読むと、`num_objects` の次の値（例 11 = user version の残り）を
  型数と誤認する。
- **原因**: NIF の `num_block_types` は **u16**（`header.rs:165` の `reader.u16()`）。
- **正しい手順**: u16 で読む。
- **確認方法**: `python scripts/nif_coverage.py --corpus meshes_extracted` の
  `corpus_types` が妥当な値（118〜119）になる。

## 3. `-lsynchronization` は `*windows-gnu` の case のみ

- **症状**: Linux CI のホストテストのリンクが `-lsynchronization` で失敗する
  （そのライブラリが無い）。
- **原因**: Rust std が **MinGW ホストでのみ** winsock/ntdll/userenv/synchronization を
  引き込む。Linux では不要。
- **正しい手順**: `tools/host_tests/run_host_tests.sh` の `EXTRA_LIBS` へ
  `*windows-gnu` の case の中だけで追加する。
- **確認方法**: Windows(Git Bash/MinGW) 側で `-lsynchronization` を外すとリンクが
  308 undefined に戻る（＝Windows では必須）。Linux では外しても緑のまま
  （＝Linux では不要）。「Linux で緑」だけでは `*windows-gnu` 限定の証拠にならない。

## 4. 共有 `target/` に proc-macro の MSVC `.dll` を混ぜない

- **症状**: GNU ツールチェーンでコンパイルすると
  `error[E0786]: found invalid metadata files for crate thiserror_impl`（
  MSVC 製の proc-macro `.dll` を読む）。
- **原因**: 共有 `target/` に別ツールチェーンの proc-macro 成果物が混在。
  `--target` を付けても proc-macro は `target/<CARGO_TARGET_DIR>/release` 側に出るため、
  同じ dir を共有すると再発する。
- **正しい手順**: `scripts/build_host_rust.sh` で `CARGO_TARGET_DIR=target/gnu` に分離し、
  GNU の実体（`rustup which rustc/cargo --toolchain <gnu>`）を `RUSTC` に固定する。
- **確認方法**: `OBLIVION_RUST_FORCE_REBUILD=1 bash scripts/build_host_rust.sh` が exit 0 で
  `target/gnu/x86_64-pc-windows-gnu/release/lib*_ffi.a` を 6 本出す。

## 5. `nif_header_info.block_type_count` は型名テーブル長（`num_objects` ではない）

- **症状**: C++ shim の `NIFParser::getHeader().blockTypeNames` が実体より多く、後ろに空文字が
  並ぶ（例: 実 8 型のファイルで 15 件、`bt[8..14]=` が空）。オラクルで `dump_rust` が
  `blockTypes=15`、`dump_orig` が `blockTypes=8` と食い違う。
- **原因**: `nif-ffi/src/lib.rs` の `nif_get_header_info` が `block_type_count` に
  `header.block_type_indices.len()`（＝ブロック数＝`num_objects`）を入れていた。C++ shim はこの
  値を**型名テーブルのループ回数**に使うため、`num_objects != 型数` のファイルで後ろを読み過ぎる。
  Oblivion の実メッシュは 8032 中 6896 本（86%）で両者が異なる。
- **正しい手順**: `block_type_count` は `header.block_type_names.len()`（型名テーブル長）にする。
  ブロック数が要る箇所は `num_objects` を使う。`nif_ffi.h` のフィールドにもその旨を書く。
- **確認方法**: C ABI の `nobj != ntyp` ケース（3 blocks / 2 types で `num_objects==3` かつ
  `block_type_count==2`）と、marker 27 本のオラクル `27/27`。lodnif 100 本は `nobj==ntyp` なので
  このバグを見抜けない（100/100 のまま）。

## 6. `scripts/build_host_rust.sh` を WSL の bash で走らせない

- **症状**: `rust/target/` が丸ごと消える（`gnu/` と `cmake/` の `lib*_ffi.a` が消え、次回の
  install が古い/空の `.a` でリンクする）。
- **原因**: WSL の `bash`（`C:\WINDOWS\system32\bash.exe`）は `uname -s` が `Linux` を返すため、
  スクリプトが Linux 分岐に入り `cargo clean --target-dir target` を実行する。Windows 用の `.a` は
  作らないまま `target/` を消す。
- **正しい手順**: Git Bash（`C:\Program Files\Git\bin\bash.exe`）で実行する。スクリプトは
  `WSL_DISTRO_NAME` を見て WSL を拒否する（`OBLIVION_ALLOW_WSL=1` で上書き可）。`cargo clean` は
  `--clean`（または `OBLIVION_RUST_CLEAN=1`）のときだけ走る。CI は `run_host_tests.sh` が
  `CI=true` を見て `--clean` を付ける。
- **確認方法**: `bash --version` に `system32\bash.exe`、`uname -r` に `microsoft` が出れば WSL。
  Git Bash なら `uname -s` が `MINGW64_NT-...`。

## 7. Rust の `.a` を消しても Gradle の native タスクは UP-TO-DATE

- **症状**: `rust/target/cmake/**/lib*_ffi.a` を消して `./gradlew assembleDebug` しても、
  古い `libnative-lib.so`（前回ビルドの mtime）が APK に積まれる。実機検証が修正前のコードで
  空振りする。
- **原因**: Rust の `.a` は Gradle の追跡入力ではない。CMake のスタンプが UP-TO-DATE のままだと
  native タスクが再実行されず、`app/build/intermediates/{cmake,cxx}` の古い `.so` を拾い直す。
- **正しい手順**: `scripts/build_apk.sh` を使う（`app/.cxx` と
  `app/build/intermediates/{cmake,cxx}` を消してから `assembleDebug`）。
- **確認方法**: ビルド開始時刻より新しい `libnative-lib.so` が 3 本（ABI 数）以上あること。
  `scripts/build_apk.sh` がこれを検査し、3 未満なら exit 1。

## 8. ダンプの float は `%a` でなく生ビットで出す

- **症状**: legacy NetImmerse の空ジオメトリ（例 `marker_divine` / `marker_temple` /
  `marker_travel`）で、ダンプの float が Linux と Windows で食い違う（`-0x1.f8p-144` /
  `scale=nan`）。CI の `nif-oracle` が 27 中 3 本だけ赤になる。
- **原因**: `%a` の非正規化・NaN の表記が CRT 依存（glibc と MSVC ランタイム）。値は同一で、
  テキストだけ違う。
- **正しい手順**: float は IEEE-754 の生ビット（`%08x`）で出す（`tools/nif_oracle/dump.cpp`
  の `f32()`）。
- **確認方法**: Linux CI（`nif-oracle` ジョブ）で 27/27。

## 9. PowerShell の `Select-Object -First` は `$LASTEXITCODE` を壊す

- **症状**: `python script.py 2>&1 | Select-Object -First 3` の後で `$LASTEXITCODE` を
  見ると 0 になる。スクリプトが赤（exit 2）を出していても緑と誤読する。
- **原因**: `-First N` は N 個読んだ時点で上流パイプラインを早期終了させる。native プロセス
  が終了前に切られるため、`$LASTEXITCODE` が正しく設定されない（`-Last N` は最後まで
  読むので壊れない）。
- **正しい手順**: 終了コードを見るときは切り捨てない。`python script.py > out.txt 2>&1;
  $LASTEXITCODE` として出力はファイルから読む。どうしても切りたいなら `-Last N` を使う。
- **確認方法**: わざと落ちる入力（期待値ダンプを 1 バイト壊す）で `$LASTEXITCODE` が 2 に
  なること。0 なら計測器を疑う。

## 10. `git add` に存在しない pathspec を渡すと一部だけ staged される

- **症状**: `git add A B C` で C が存在しない（削除済み等）と、**A・B も登録されない**。
  前段の `git rm` で staged 済みの削除だけが残り、それを commit すると「データを消したのに
  代替が無い」壊れた commit ができる。2026-11-26 の `cefb7c52` がこれ（削除 2 ファイルのみ）。
- **原因**: `git add` は 1 つでも pathspec が一致しないと全体を中断する。`git rm` 済みのパスを
  もう一度 `git add` に渡すと、index から消えているので一致しない。
- **正しい手順**: `git add` の直後に **`git diff --cached --name-only` の件数を数え**、期待値と
  合わなければ commit しない。削除は `git add -A <path>` か、既に staged なら渡さない。
- **確認方法**: staged 件数が期待どおり。`git show --stat HEAD` に削除と追加が**両方**あること。
- **CI は緑だったから大丈夫、が通じない**: CI は fixture を再生成しないので、壊れた中間状態
  （削除のみ・追加なし）でも旧ゲート×旧 fixture で緑になる。緑は「ツリーが正しい」証拠ではない。

## 11. 連続 screencap は「たいてい同一・ときどき数百画素動く」（安定待ちをバイト一致で組むと flake になる）

- **症状**: `docs/smoke-tests/newgame.sh` の画面の安定待ちを「PNG のバイト一致（`cmp -s`）」で
  組むと、**収束する run と 20s 上限に張り付く run が混ざる**（＝ flake）。「たまに落ちる」
  ゲートになり、原因が見えにくい。
- **原因**: 静止して見える画面でも、連続する `screencap` が**ときどき**数百画素だけ変わる
  （ステータスバーの時計の刻み・アニメーションの端）。実測:
  - 同一画面の連続 4 枚（ランチャー）: **0 / 2304000（完全一致、PNG もバイト同一）**
  - 時計が刻んだ瞬間の 1 組: **333 / 2304000 = 0.0145%**（`|Δ|>8` で数えた変化画素）
  - つまり**毎回ではない**。だからバイト一致は「ほとんど通る」ので、失敗が再現しにくい。
- **正しい手順**: 同一判定は**画素差の割合**で行い、閾値は観測した最悪値の約 7 倍の
  **0.1%（`SMOKE_SETTLE_DIFF_MAX=0.001`）**。一過性フレーム（タイトル分岐
  `renderer.cpp:5151` が message box を chargen より先に描く 1 フレーム）は **70% 級**に
  動くので、0.1% で明確に分かれる（ノイズ床の約 700 倍下）。**収束しなかったら `exit 2`**
  （撮って進めると、収束しなかった run が緑になり flake が戻る）。
- **#194 で settle が不要になるわけではない**: #194（タイトル分岐の描画順）が消すのは
  **70% 級の一過性**だけで、**ステータスバーの時計は動き続ける**ので安定待ちは残る。
  狙いは「settle の廃止」ではなく「0.1% 閾値の余裕を 7 倍から数百倍に広げる」こと。
- **確認方法**: 実測 2026-11-27 —
  - 正常: `SMOKE_SETTLE_DIFF_MAX=0.001` で `settled: 3 連続同一（elapsed=2.10s）`、
    golden `mean=0.02 / changed=0/576` で一致。
  - 閾値の位置: `0.0001`（0.01%）でも chargen 画面では収束した（＝ 0.1% は縁ではない。
    chargen 画面のノイズは 0 のこともある）。
  - バイト一致（`cmp`）では収束せず 20s 上限に張り付いた（実測）。
  - 上限の陰性対照: `SMOKE_SETTLE_MAX=1` にすると収束前に **exit 2** で止まる。

## 12. 手元 Windows の host テストは `OBLIVION_RUST_HOST_TRIPLE` を付けないと Linux 書庫を掴む

- **症状**: `tools/host_tests/run_host_tests.sh` がリンクで落ちる。
  `relocation truncated to fit: R_X86_64_DTPOFF32`、
  `undefined reference to core::slice::index::slice_fail`、
  `std::sys::io::error::unix::errno` などが出る。リポジトリの記録も
  「手元 Windows では host 書庫が MSVC 成果物になり手元実行は不可」としている。
- **原因**: `_cand_triples=("$RUST_HOST_TRIPLE")` の先頭が空だと、探索が
  `target/gnu//release` → `target//release` の順に落ち、**Linux 産の ELF 書庫
  `rust/target/release/`** を先に掴む（MINGW 分岐で gnu triple が足されるのは
  その後）。`target/release/` は Linux 産（`std::sys::…::unix` を含む）なので
  MinGW ではリンクできない。**MSVC の問題ではなく、Linux 書庫を掴んでいた**。
- **正しい手順**: `OBLIVION_RUST_HOST_TRIPLE=x86_64-pc-windows-gnu` を付けて実行する。
  これで `rust/target/gnu/x86_64-pc-windows-gnu/release/` が選ばれる。環境変数名は
  `RUST_HOST_TRIPLE` ではなく **`OBLIVION_RUST_HOST_TRIPLE`**（`run_host_tests.sh:28`）。
- **確認方法**: `OBLIVION_RUST_HOST_TRIPLE=x86_64-pc-windows-gnu bash
  tools/host_tests/run_host_tests.sh` → `HOST RUNNER: ALL GREEN (0 suite(s) failed)`
  （2026-11-27 実測。`rust/target/gnu/x86_64-pc-windows-gnu/release/lib*_ffi.a` は
  6 本そろっている）。C++ のホストテストは手元で回せる。
