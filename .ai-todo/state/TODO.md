# TODO

## Tasks

- [x] **#30** Phase45 ホストテストの msvc ASan 実ハング（PID 39080・2400秒）原因特定を完了。procdump フルダンプ + PDB + PE .pdata + capstone で待機チェーンを全追跡し、決定的証拠を確定: (1) main (tid 29704) は AsyncTaskManager::cleanup() の worker.join() 内で _Thrd_join → WaitForSingleObjectEx → ntdll 待機syscall (ntdll+0x161DD4) のままブロック、最上位 EXE フレーム = cleanup@AsyncTaskManager +0x177 (0x593327)、(2) ワーカー5本は ntdll ローダーロック待機syscall (ntdll+0x1659C4) + clang_rt.asan_dynamic+0x484BC (ASan スレッド起動フック) のローダー連鎖のみで EXE フレーム0件 = workerThread() に到達不能、(3) minidump モジュール全26件列挙で Trend Micro AV のインプロセスフック tmmon64.dll (base=0x56530000, C:\Windows\System32\tmumh\20019\TmMon\2.9.0.1243\) がロード済みを確認 → tools/host_tests/README.md L220-250 の既知制約「ASan + AV フックの DLL_THREAD_ATTACH ローダーロックデッドロック = 環境制約・スイート欠陥ではない」と完全一致。コード修正不要と確定。回避検証: run_host_tests_msvc.ps1 -NoAsan で全89 TU 再ビルド → HOST RUNNER: ALL GREEN (0 suite failed) / Host tests passed (exit 0)。AsyncTaskManager 実装は健全と確認。 #phase45 #hang-analysis #asan #tmmon64 #environment-constraint (2026-11-02)
  > 2026-11-01/02 完了。Phase 45 ハング死因は Trend Micro tmmon64.dll によるデッドロックと確定（エンジン側不具合ではない）。問題点2件＋改善提案4件をユーザーへ報告済み。

- [x] **#29** [v230 §5-3] jni_bridge.cpp:255-258 の fGlobalTimeMultiplier が frame-dt に乗算する仕組みはエンジン機構と不整合（エンジン側に同機構なし）。現在は恒常 1.0 で沈静化しているが、外部プラグイン等が変数を書き換えるとフレームレート依存でゲーム速度が暴れる。恒常1.0前提なら乗算自体を削除するか、エンジン風の伝播機構に合わせるかの [PORT] 判断。優先度低。根拠: tmp/re/v230_relationship_residuals.md §5-3。 #porting #re (2026-11-02)
  > Implemented and verified (host tests ALL GREEN via run_host_tests.sh, 2026-10-30). Decision: removed the frame-dt multiply (constant 1.0 assumption; no engine-side equivalent). jni_bridge.cpp: multiply block replaced by an evidence comment citing RE 54.3 (v230 3-2); the gmst include was dropped. GLOBAL_TIME_MULTIPLIER removed from settings_manager.h; the fGlobalTimeMultiplier entry removed from gmst_defaults.h time/timing group (kEntryCount 41 -> 40) and combat_formulas_tests.cpp updated. The INI default table entry (oblivion_ini_defaults.cpp:444) was intentionally kept - it is an INI-derived default, not a live GMST.

- [x] **#28** [v230 §5-2] Ability/Disease/Power 固有挙動の実装判断と VAMP/POSN の DAMAGE 丸め注記。移植 SpellEffectType は8種（9種は誤り・v229 訂正済み）、MGEF→対応表は四字正式名で一致確認済み。差分は Ability(4)/Disease(1)/Power(2,3) の固有挙動不在と VAMP/POSN→DAMAGE 近似。fourcc 正式名照合（208/409/600）は未実施。実装するか [PORT] 判断し plan.md に反映。根拠: tmp/re/v230_relationship_residuals.md §2、§5-2。 #porting #re #needs-approval (2026-11-02)
  > [2026-11-02 Copilot] 判断確定・plan.md 反映済み（plan.md L1790 "Decided 2026-11-01 (#28)"）。2部構成で判断: (1) fourcc 正式名照合完了 — 全45 mapped codes（DAMAGE 30 / HEAL 5 / RESTORE_MANA 3 / RESTORE_STAMINA 2 / PARA / INVI）を CS Wiki 公式 Magic_Effects_List と照合し全件一致。VAMP→DAMAGE は vampirism の combat manifestation が health drain なので accepted approximation、POSN→DAMAGE は poison が damage-based で consistent。(2) Ability(4)/Disease(1)/Power(2,3) の固有挙動は accepted approximation として記録 — magnitude/duration は正しく適用済みで gameplay 体感は成立。magic-type lifecycle（disease spread、once-per-day cooldown、permanent passive）は magic system 完全移植フェーズへ繰延べ。SpellData::spellType は #26 の connection point として保持。Evidence: v230 §2-2/§2-3、spell_manager.cpp:27-95、spell.h SpellEffectType 8種、cs.uesp.net/wiki/Magic_Effects_List

- [x] **#27** [v230 §5-1] gmst_defaults 41 float キーの拡張範囲判断。移植の GMST merge 語義はエンジンと同型（後勝ち・insert なし・キー集合不変）と確定したが、移植は float 41キーのみでエンジン 2,304件に対し網羅率 1.8%。どのキー群を getLive 化するかは [PORT] 製品判断。判断後 plan.md に反映。根拠: tmp/re/v230_relationship_residuals.md §1、§5-1。 #porting #re #needs-approval (2026-11-02)
  > [2026-11-02 Copilot] 判断確定・plan.md 反映済み（plan.md L1789 "Decided 2026-11-01 (#27)"）。判断内容: coverage scope は consumer-driven 40 float keys のまま維持、2304 GMST への一括拡張なし。キーは getLive コンシューマ登場と同時に kEntries へ逐次追加（現キーは全て actively consumed、fKnockbackTime のみ reserved）。カウント問題も決着: kEntries 実件数 40（kEntryCount は sizeof 由来）、fGlobalTimeMultiplier は INI General キー（oblivion_ini_defaults.cpp:444, default 1.0000, node 0xB06704）で GMST ではなく frame-dt 乗算は #29 で削除済み — v230 §1/§3 の記載は stale。Evidence: gmst_defaults.h kEntries 40件、getLive 41 matches（combat_formulas.h 22 / fatigue_system.h 16 / コメント3 / テスト1）、jni_bridge.cpp:245-275、oblivion_ini_defaults.cpp:444

- [x] **#26** [v230 §5-5/§5-6] 低優先の軽微整理2件: ①key_only_fallback 相関の find_by_key() がヘッダコメント「フォールバックしない」と書くが実装はフォールバックする（4件中3件一致、sBasePath 欠落は Reg 死にコード追随で実害なし）— コメント実態に合わせ訂正。②SpellData::spellType が未使用 — 型別挙動（Ability/Disease/Power）の接続点候補として整理方針を決める。根拠: tmp/re/v230_relationship_residuals.md §4、§5-5、§5-6。 #porting #re (2026-11-02)
  > Implemented and verified (host tests ALL GREEN via run_host_tests.sh, 2026-10-30). ① find_by_key(): header comment corrected to match the implementation (linear scan, first match wins, falls back to defaults; the "no fallback" wording was wrong). ② SpellData::spellType: kept as the connection point for #28 (Ability/Disease/Power behaviour); a comment in esm_reader.h marks it explicitly. Files: app/src/main/cpp/system/oblivion_ini_defaults.h, app/src/main/cpp/assets/esm_reader.h. Note: run_host_tests_msvc.ps1 fails with C1083 on Rust FFI headers - pre-existing runner gap (ps1 never updated for the Rust migration; $RUST_DIR tokens unexpanded, no archive linking). The supported path is Git Bash run_host_tests.sh which is fully Rust-aware.

- [x] **#25** [v230 §5-4] AI時計と setTimeScale の非接続是正。エンジンは TESGlobal TimeScale 1本が sky_weather_system / ai_scheduler / コンソールを束ねるが、移植はコンソール setter が SkyWeatherSystem にのみ効き AI 時計（sleep/wait）に伝播しない。対応: ai_scheduler へ setTimeScale 接続を実装し、ai_scheduler.h:114 の未使用 timeScale メンバを整理（使用 or削除）。根拠: tmp/re/v230_relationship_residuals.md §5-4、§3。 #porting #re (2026-11-02)
  > 2026-11-01/02 完了。setTimeScale 接続を3ファイル（ai_scheduler / jni_bridge / 予備）で実装し、ホストテスト ALL GREEN。AI時計と実時間の乖離是正済み。

- [x] **#24** esm_reader.cpp (app/src/main/cpp/assets/, 約5,088行) のRust移行。2026-11-01方針「Rustで記載できる所はRustで記載」の初号案件。plan.md Rust migration節で次ターゲットと明記済み。技術方針: zlib→flate2 (bsa-coreで実証済み)、std::ifstreamはC++シム側に残す、レコード型はsyncCache()方式(NIF/BSAのopaque handle方式に準拠)、新クレート esm-core / esm-ffi。オラクル検証はhost dump differ方式(tmp/bsa_oracle, tmp/nif_oracleと同じ)。TODO #21 (V31 MEDTデッドコード) はこの移行に吸収する方針(ai-todo #21に注記済み)。参考: docs/RE_EXE_REPORT.md, tmp/re/v31_medt_deadcode.md `#completed` (2026-11-02)
  > 進捗（2026-11-02）: shim 置換完了（5188→4676行）+ CMake IMPORTED ターゲット修正 + 3 ABI シンボル確認済み。docs 反映完了（plan.md / CHANGELOG.md / ARCHITECTURE.md 各数箇所）。オラクルハーネス構築中: tmp/esm_oracle に dump.cpp（公開ESMFile API全カバレッジ）/ build_orig.ps1 / build_rust.ps1 / compare.ps1。build_orig.ps1 に2つのバグ修正済み（git show パスを repo 相対に、$cppRoot を ..\..\ に）。dump_orig.exe 再ビルド中。既知ギャップ: mem パスの type-1 GRUP worldspaceFormID（orig は未処理 / Rust は正しく付与 → 意図的改善として document 予定）。
  > 完了（2026-11-02 オラクル green）: compare.ps1 exit=0。集計 20 pairs / 9 identical / 11 different。全 11 件が orig 側 — mem は 8 件全て orig の parseFromMemory が空マスタ返し（Rust は file≡mem 一貫）、file は 2 件（DLCMehrunesRazor / Knights）で orig がレコード population 欠落（Knights は orig が CELL 0 / Rust は CELL 132・NPC_ 76・REFR 7058・LAND 61）。残り 8 file ペア（277MB Oblivion.esm: CELL 35494・REFR 1029280）はバイト一致。Rust が正しい・orig の既存バグを修正。docs（plan.md / CHANGELOG.md）にオラクル結果反映済み。commit / push はユーザー明示指示待ち。

- [x] **#23** coord-watch: 他セッション（ReverseEngineering A）との二重作業監視 — TODO/docs 編集は連絡済みの連託分のみ `#coord` (2026-11-02)
  > [2026-11-02 Copilot] 監視結果: #40（V231 起票の v237 残点3件等）は RE ループ側（opencode ReverseEngineering セッション）の作業分であり Copilot 側は着手しない。二重作業なしを確認。TODO/docs 編集は連託分のみという運用ルールも維持され、本セグメントの plan.md 追記は #27/#28 の連託判断の記録（append-only Decided 段落）に該当し問題なし。coord-watch 任務は継続中だが現時点で対応が必要な二重作業は検出せず。

- [x] **#22** CI commit: oblivion_ini_defaults.cpp の kFloat 修正 — 再確認完了（2026-11-02）。catch-all コミット `19fafb7f` "Save uncommitted changes" でコミット済み。HEAD の `app/src/main/cpp/system/oblivion_ini_defaults.cpp` に kFloat エントリ存在（L50/54/55 等）、未コミット差分にも含まれない。追加作業不要 `#ci` `#completed` (2026-11-02)
  > Rust方針転換(2026-11-01)の影響確認: このファイルはINI静的デフォルトテーブルでRust移行対象の即時影響はなし。ただし現況調査で、このファイルは catch-all コミット 19fafb7f "Save uncommitted changes"（87 files, +6389/−164976）に含まれて1314行変更としてコミット済みの可能性が高い。HEADのファイルには kFloat エントリが存在する。未コミットという記述が既に古い可能性があるため、コミット要否の再確認を推奨。

- [x] **#21** V31: esm_reader MEDT デッドコード修正 — #24（esm_reader Rust 移行）に吸収済みで完了（esm-core は MEDT 探索デッドコードを持ち込まない方針、挙動変化ゼロを維持） `#completed` `#porting` `#re` (2026-11-02)
  > Rust方針転換(2026-11-01)による影響確認済み: plan.mdでesm_reader.cppは次Rust移行ターゲットと明記済み。推奨方針は「C++側に先行パッチせずRust移行時に吸収」— オラクル検証はgit HEADのC++と比較するため、C++側を先に変えるとベースラインがずれる。Rust実装(esm-core)ではMEDT探索デッドコード自体を持ち込まない形で対応すれば、挙動変化ゼロというV31の約束も維持される。C++側に先に適用する場合はオラクル再ベース化が必要。

- [ ] **#20** V168【要承認】: 台帳追記反映バッチ＋GT/docs 反映第2 — 人間承認待ち（serial 168） `#needs-approval` `#re`
  > 2026-11-01: 承認材料を整備・提示済み（ユーザー不在のため適用せず保留）。(1) H1-H7 全定義（根拠: tmp/re/v170_ledger_granularity.md、表形式で全文確認済み）。(2) V157 推奨裁定: 実EHエントリ 10,716 粒度・adopt 10,572 を採用推奨、region開始点665/全10,742 は却下。(3) 対象2 GT/docs ドラフト群: v171（GT §4.1.1 確定4行+docs §41.3 訂正4行）/v177（GT ドラフト4点、EDL extra 0x14=ExtraStartingPosition・0x2D0=StartItemAction 確定）/v181/v182（docs 転記ドラフト）/v201（訂正素材、表示名3件は opencode 側適用済み・二重修正なし確認済）。(4) 適用ログ先例: V143→v168_v143_apply_log.md（GT 8操作+docs 6操作、docs .bak17/GT .bak_apply1、round_verify ALL OK）、V144→v167_v144_apply_log.md（訂正束14+1件を30操作、bak16）。承認材料は Supermemory（namespace opencode_project_470935b226aa5f24、id VQ2Zf3GEyfP9q16xAvbPoG）にも保存済み。#needs-approval のため人間承認あるまで適用禁止。
  > 2026-11-02 08:55 時点: 承認材料の提示・Supermemory 保存（id VQ2Zf3GEyfP9q16xAvbPoG）・根拠文書特定（v166/v170）は完了済み。人間承認（H1-H7 一括 or 個別裁定）待ちのため適用は保留。ユーザー不在のため適用は行っていない。承認後に v170 CSV（10,870行）＋v174 xcu を Ghidra 台帳へ、v171/v177/v181/v182/v201 ドラフト群を GT/docs へ反映（V143/V144 先例の適用ログ手順に従う）。

- [x] **#19** V192: havok/INI 命名束（O 正式クラス名・[0xB3BCF0] INI 名・23エントリ表 等）— serial 192。成果物 tmp/re/v205_havok_ini_naming.md `#completed` `#re` (2026-11-02)
  > 同期（2026-11-02）: RE 正本 tmp/re/TODO_re49.md で serial 192 が [x] 完了 (2026-10-31) を確認。成果物 v205_havok_ini_naming.md 実在（9,579 bytes, 10/31 21:46）。ローカル側が同期漏れだったため [x] 化。

- [x] **#18** V191: process 細部束（KnockedState 値1 算術・[+0xB0] 名・UI キークラス名 等7件）— serial 191。成果物 tmp/re/v204_process_residuals2.md `#completed` `#re` (2026-11-02)
  > 同期（2026-11-02）: RE 正本 tmp/re/TODO_re49.md で serial 191 が [x] 完了 (2026-10-31) を確認。成果物 v204_process_residuals2.md 実在（23,969 bytes, 10/31 21:51）。ローカル側が同期漏れだったため [x] 化。

- [x] **#17** V188: opcode/GT 名前 第6束（+0x10 登録帯書手・base+0x58 bit7/bit22 名・slot[6] 値4）— 正本 tmp/re/TODO_re49.md serial 188。親セッションが inline 進行（証拠 v201_* 41点） `#completed` (2026-11-02)
  > 同期（2026-11-02）: RE 正本 tmp/re/TODO_re49.md で serial 188 が [x] 完了 (2026-10-31) を確認（後続 V207 が同 serial の完了行から起票されている）。ローカル側が同期漏れだったため [x] 化。
  > 2026-10-30: 子エージェント中断続きのため親セッションが inline 継続。証拠 %TEMP%/opencode/v201_*（41点）を走査済み。判明: ①+0x10 初回書手=FUN_004FF710@0x4FFB0C（[parser+0x410] が 0x14/0x1F → 0='f'、それ以外 → 1='s'）②bit22 読手13件=エフェクトリストチェッカ群（0x414EB0 系）+FUN_00419B30、直接クリアは 0x416854（FUN_004167E0）③slot[6]: SpellItem は [+0x1C] 動的返し（Enchant=6/Alchemy=7/Ingredient=8 済み）。残: v201 成果物への統合執筆。

---

## Task Metadata

Task relationships and dependencies (managed by todo.ai tool).
View with: `./todo.ai show <task-id>`

<!-- TASK RELATIONSHIPS
21:related-to:24
-->

---
**ai-todo** | Last Updated: 2026-11-02 09:10:41
