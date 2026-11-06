# TODO

## Tasks

- [ ] **#66** docs 反映候補のゲート加算（v303 §確認 RE 証拠）: TES4SAVEGAME マジック＋version ゲート閾値列（0x1c/0x2e/0x34/0x37/0x38/0x51/0x52）と『ess_file_obj_load は完全 .ess パーサでない』注記、未解決域（圧縮ペイロード位置・change forms/globals/quests/cells/AI の完全境界）の docs/RE_EXE_REPORT.md 追記案は承認待ち — #47/#20 の一括適用時に合算（単独では編集しない）。移植側 OBLV フォーマット（save_format_version.h・タグ PLYR/INVT/SPLL/QSTS/WLDS/SCRP/CNTR/MERC・REF 変更表なし）との差分表も素材化 `#approval` `#oblivion` `#re`

- [ ] **#65** REF change state RE 静的解析束（v303 判断の次の一歩）: 実 .ess サンプルなしで EXE 側から change-form 境界表を確定できる範囲を調査 — docs/RE_EXE_REPORT.md:2748-2873 の既知経路（TES4SAVEGAME ヘッダ・version ゲート 0x1c/0x2e/0x34/0x37/0x38/0x51/0x52・ess_file_obj_load が復元パスであって完全パーサでない件）を踏まえ、REFR change form / cell / quest / global / AI の境界決定に使える命令証拠（write/read 窓・文字列・4cc 即値 sweep）を tmp/re/v304 として棚卸し。押収済み v281-286 の CHANGE_* 帯（CHANGE_REFR_DISABLE_STATE@0x455C72 等）と統合し、サンプル取得後にヘッダのみ→境界順で検証可能な手順書へ昇華させる（推測パーサ・移植コード追加は禁止） `#oblivion` `#re`

- [ ] **#64** 人間決定: Graphify の RE コーパス登録方針 — .gitignore:53 /tmp/ のまま受容か、!/tmp/re/ 追加で再スキャンか（現況: 旧 RE ノード 1,207 prune 実確認・v289-v303 未登録・tmp/re は RE ループの書き込み域のまま）。L1 では .gitignore 編集不可（根拠 tmp/re/v302 §件3） `#approval` `#graphify` `#oblivion`

- [ ] **#63** B13型実機ダンプ要求: table1/table2（[0xB33398]+0x20 経由・+0x1B9B/+0x1B7E の 29B カテゴリ→index 表）と 9/238 スロット配列の実データ値域取得 — slot0/index0 不変条件の最終裁定に必要（ユーザー実行待ち・根拠 tmp/re/v302 §件1/残点1） `#oblivion` `#re` `#user-execution`

- [ ] **#62** [PORT] GMST/Settings 検索コピー形の移植注意: エンジン 0x57C240/0x59BE50 の node null 経路は素の strlen(NULL) 形（je→xor ecx,ecx→mov al,[ecx]=[0x0] 読出し）＝移植では空文字短絡を明示的に持つべき（根拠: tmp/re/v302 §件1・docs候補1） `#low` `#oblivion` `#port` `#re`

- [ ] **#47** v283〜v288のRE正典・台帳追記（承認待ち） `#approval` `#oblivion` `#re`
  > `docs/RE_EXE_REPORT.md`、v5 gaps report、v10 join CSV等への追記候補。v284でCSV互換性を壊すコメント行追加は行わず、列追加等の形式を再確認したうえで、ユーザー承認後にのみ編集する。実行用TODO: `re-report-v283-v288-approval`。
  > 【2026-11-05 承認材料完成】tmp/re/v296_v283_v288_approval_materials.md（330行・SA-B作・適用なし）。docs候補4（§70.10注/§70.15済化/§70.8-2マーカー/新§74=12564行で§74空き実測）、v5/v62 3、v10 CSVは列追加v61_statusのみ可（#コメント行はDictReader互換破壊で不可・確定）、GT 0、保留9、変更不要6。適用手順雛形bak129〜（実測max128）。RE_EXE/STATE.mdはHEAD既存変更あり=適用時ライン再実測必須と明記。

- [ ] **#46** [PORT] L2承認待ち3件: V194 AddTopic・V197 TimeScale・V198 SetActorValue ブリッジ（起票のみ） `#gate` `#oblivion` `#port`
  > ホーム台帳 #9 より統合。承認ゲート: 人間承認あるまで適用禁止。

- [ ] **#45** Rust 移行の残件: N5b=animation keyframe/resolve+walker 系 C ABI 未実装（phase30 パリティ）、N6 docs パス（README/AGENTS ツリーに bsa-*/nif-*/cli 欠落）、native-lib リンク未実行 `#oblivion` `#re` `#rust`
  > ホーム台帳 #14 より統合。2026-11-05 再調査完了（ユーザー裁定: 調査のみで停止）。Rust側は`NiControllerManager`/`NiControllerSequence`とskin/collision readerまで実装済みだが、`NiKeyframeController`/`NiKeyframeData`/`NiTransformController`のreader、`resolveControllerReferences`、animation用C ABI、walker系C ABI、phase30 parity harnessは未実装。Android側の`parseNiKeyframeController`/`parseNiKeyframeData`/`resolveControllerReferences`はinert stubで、現時点で推測実装しない。`oblivion-re-tools`で`cargo test --offline --workspace`は176 passed（31 suites）、既存nif-ffi C ABI checkはgreen。N6 docs/native-lib linkは未実行のまま。

- [ ] **#44** V219 外部待ち項目の管理: GECK / registries資料 / [PORT]承認 / docs適用 `#oblivion` `#re`
  > ホーム台帳 #26 より統合。2026-11-01 起票 (bak88)。[PORT] Controls A/B 選択は承認ゲート性質。以降の serial ラウンドで解消済みの可能性が高い。台帳 row219 確認後に閉じる。
  > 【2026-11-05 照合実施→クローズ不可】成果物 tmp/re/v293_v219_residual_check.md。v223 §6 残点6件を後続 v265-v288 と照合: 完全解消0/部分解消1/未解消5。残点3(getter thunk 0xA1B480帯)=部分解消(機構=0xB07CFC INI登録/解除テーブル・0x65C620=リスト操作を v264/v275 で確定、対象範囲は未調査=残調査1件)。他5件は再調査不要な待ち条件付き=GECK入手(1)/registries.txt入手(2)/[PORT]承認(4)/docs適用(5,6)。推奨: 残3対象範囲を新束起票、4,5,6 は #20/#46 へ統合、1,2 は恒久待ち。selftest 4/4 PASS。判定により本タスクはクローズ不可。
  > 【2026-11-05 残点3解消】V280成果物 `tmp/re/v291_getter_thunk_scope.md` でgetter thunk 0xA1B480帯の対象範囲を確定。残りはGECK/registries資料待ち、[PORT]承認、docs適用待ちであり、実質的なRE調査は完了。#44は外部条件待ちとして維持する。

- [ ] **#20** V168【要承認】: 台帳追記反映バッチ＋GT/docs 反映第2 — 人間承認待ち（serial 168） `#needs-approval` `#re`
  > 2026-11-01: 承認材料を整備・提示済み（ユーザー不在のため適用せず保留）。(1) H1-H7 全定義（根拠: tmp/re/v170_ledger_granularity.md、表形式で全文確認済み）。(2) V157 推奨裁定: 実EHエントリ 10,716 粒度・adopt 10,572 を採用推奨、region開始点665/全10,742 は却下。(3) 対象2 GT/docs ドラフト群: v171（GT §4.1.1 確定4行+docs §41.3 訂正4行）/v177（GT ドラフト4点、EDL extra 0x14=ExtraStartingPosition・0x2D0=StartItemAction 確定）/v181/v182（docs 転記ドラフト）/v201（訂正素材、表示名3件は opencode 側適用済み・二重修正なし確認済）。(4) 適用ログ先例: V143→v168_v143_apply_log.md（GT 8操作+docs 6操作、docs .bak17/GT .bak_apply1、round_verify ALL OK）、V144→v167_v144_apply_log.md（訂正束14+1件を30操作、bak16）。承認材料は Supermemory（namespace opencode_project_470935b226aa5f24、id VQ2Zf3GEyfP9q16xAvbPoG）にも保存済み。#needs-approval のため人間承認あるまで適用禁止。
  > 2026-11-02 08:55 時点: 承認材料の提示・Supermemory 保存（id VQ2Zf3GEyfP9q16xAvbPoG）・根拠文書特定（v166/v170）は完了済み。人間承認（H1-H7 一括 or 個別裁定）待ちのため適用は保留。ユーザー不在のため適用は行っていない。承認後に v170 CSV（10,870行）＋v174 xcu を Ghidra 台帳へ、v171/v177/v181/v182/v201 ドラフト群を GT/docs へ反映（V143/V144 先例の適用ログ手順に従う）。

---

## Archived Tasks
- [x] **#61** V285 実行: 配列 index 域と 0xB39000 所有名束（slot0 センチネル突合・0xB39000 書手/帰属・Graphify doc 登録試行）→ v302 `#oblivion` `#re` (2026-11-06)
  > 【2026-11-06 V285完走】成果物 tmp/re/v302_array_index_domain.md（3probe+graphify skill参照）。件1部分確定: 29カテゴリ→隣接2表(0x1B9B/0x1B7E)→9-slot(0xB39554..74・主配列完全隣接)/238-slot(0xB39578)の供給経路閉鎖。index0両ゲート通過可・9-slot slot0書手あり/238-slot slot0書手なし・読手null経路=素のstrlen(NULL)形([0x0]読出し・ガードなし)＝実機挙動依存→B13型実機ゲート待ち。件2確定: 0xB39000=GMSTスカラーnode(sGeneralFace/General(Face))・706本atexit thunk(0xA20720..0xA235F0→0x4055E0)・node帯0xB38000..0xB39E50 stride8・ギャップ2本=fast-index配列と交互配置整合・graphify既存GMSTノードの実測拡張。件3確定(ブロック記録): .gitignore:53 /tmp/(8e4c1b88)でtmp/re scan対象外→v289-v301登録不可能=人間決定・既存REノード1,207prune実確認・本日CLI update=2/2 cached topology無変化・backlog=images1667(キー)/RE_EXE_REPORT(12,564行)/meta群。台帳row285[x] bak139-140・round_verify 171/166/5 ALL OK。docs候補+3。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#60** V284 実行: 残留微細束（[0xB39578] 登録書手・0x4A7A60 フォーマット実文言語・0x463913 到達元）→ v301 `#oblivion` `#re` (2026-11-06)
  > 【2026-11-05 V284完走】成果物 tmp/re/v301_micro_residuals_bundle.md（8 probe）。件1確定: [0xB39578]=238スロット・バイト索引ポインタ配列（三数一致 0x3B8/4=0xEE）、書手=CRT静的初期化帯 0x9F34xx-0x9F77xx（new(8)→0x419A70→mov [slot],eax 逐次・stride0x38）、姉妹 base 0xB39554 同閉包。slot0=imm書手不存在[確定負]＝.bss ゼロ予約スロット。v300「リスト」呼称→配列へ訂正。件2確定: 0xA2FAB0/0xA32490=Setting重複警告実文言語、sink=0x4A7A60→0x4941A0→[0xB34D90]→'Warnings.txt'@0xA3D9BC 接続確定。件3確定（v297訂正）: 0x463912=33cc=xor ecx,esp でfstart誤検出、実エントリ=0x463700（E8 2件）、0x463922 は je 0x46388e 到達。v297 host表記訂正（結論不変）。台帳row284[x] bak137-138・round_verify 170/165/5 ALL OK。docs候補+3・残点2（低）。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#59** V283 実行: INI 3層構造確定束（0xB07BF0→0xB07CFC→0xB35574 突合・0x40C3FB 所有クラス）→ v300 `#oblivion` `#re` (2026-11-06)
  > 【2026-11-05 V283完走】成果物 tmp/re/v300_ini_three_layer_bundle.md。件1確定: 0xB07BF0+0x10C=0xB07CFC 算術一致＝3層構造確定（①SettingsManager 0x4040E0 stricmp重複検査+警告0xA2FAB0+0x446CB0 Push実呼び出し初確認 ②名リスト ③0xB35574=vtbl付きNiTMap(0x55E000/0x412D30/0x4524C0)。参照574/634件）。件2確定: [0xB39578] companion 発見（teardown drain=登録順リスト・map/list二重構造・登録順保証）、0x4055E0=jmp thunk専用refs=0。件3恒久保留踏襲。台帳row283[x] bak136・round_verify 169/164/5 ALL OK。残点2（[0xB39578]登録書手・0x4A7A60フォーマット）。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#58** V282 実行: pending マスク最終決着束（0x595190 vtable帰属・0x5C071E/0x5D335A object同定・bit1 store網羅・0xB35574単例名）→ v299 `#oblivion` `#re` (2026-11-06)
  > 【2026-11-05 V282完走】成果物 tmp/re/v299_pending_mask_setter.md。件1確定（否定+再解訳）: [ctx+0x50]非ゼロ書手不存在=0x595190→.?AVAudioMenu@@(vft A6B29C)/0x5D335A→.?AVSaveMenu@@dtor/0x5C071E=GUIメニューnew(0x3F8)で全て[確定負]、dispatch 0x4662C9('Save Game Header'・E8ゼロ=jump table)は読取+ゼロ化のみ。非ゼロ値=Saveレコードflagsのparse-time mirrorのみ。呼称をLoadGame parse-time record-flags mirror fieldへ訂正（v297 §件1 [推定]差替え）。件2部分確定（bit2=v286再接続・bit1=生成窓0x452000-465000の83/1(imm=2)3点は全て+0x18 form級で負方向確定=恒久保留）。件3部分確定（0xB35574=INI値ストア・0x419A70登録/0x4055E0破棄の往復19参照全帰属・0xB07BF0→0xB07CFC+0xB35574の3層構造[推定強化]）。件4=v298件(5)参照。台帳row282[x] bak135・round_verify 168/163/5 ALL OK。残点3→次束候補。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#57** V281 実行: INI登録/パサ低位ビット束（[ctx+0x50] 書手・pass2 bit1/2・0x4055E0・CRT表スロット・TOFT訂正ドラフト）→ v298 `#oblivion` `#re` (2026-11-06)
  > 【2026-11-05 V281完走】成果物 tmp/re/v298_ini_registration_bundle.md。(3)確定=0x4055E0は破棄関数(lazy-init getter説確定負・0xB35574 unlink+delete)、0x9E21F0/210=スカラーINI登録0x419A70+atexit pairs。(4)確定=16+スタブの.initterm連続slot 0xA28E38..0xA28E7C＝登録昇順/解除LIFO二重保証。(1)部分確定=pending書手候補3件構造的否定(memset0x68/list head/型別binder case26)、残=0x595190 propertyセッタ+0x5C07xx帯に絞り込み。(2)bit2=v286古典形式と接続・bit1は禁止域[未確定]。(5)TOFT訂正ドラフト確定(0xA3BA94→0x00A3BC94)。台帳row281[x] bak134・round_verify 167/162/5 ALL OK。残点3件=次束候補(低)。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#56** V280 実行: getter thunk 0xA1B480 帯対象範囲束（V219残点3・SA-A upstream error のため inline）→ v291 `#oblivion` `#re` `#v219` (2026-11-06)
  > 【2026-11-05 V280完走】成果物 tmp/re/v291_getter_thunk_scope.md。正体=bLightAttenuation INI 12キーの atexit 登録解除スタブ帯（0xA1B3C0..0xA1B5D0・stride 0x30・12/12、v223呼称「getter」は誤り・真のgetter形は0xA1B3A0/3B0=jmp 0x4055E0群）。二次登録=0x9E22xx..0x9E26xx 12スタブ（0x4040E0(ecx=0xB07BF0 SettingsManager)登録+0x981FB4 atexit・CRT静的初期化帯=起動1回）。群外=確定負。v223各6件参照と完全突合(消費3+SEH1+登1+thunk1)。消費者0x4B1160=減衰合成[0xB35AC0]1/2/4+clamp。残点2（CRT表スロット/0x4055E0）→V281集約。台帳row280[x] bak131・round_verify 166/160/6 ALL OK。V219残点3解消により #44 の実質RE残=恒久待ちのみ。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#55** V279 実行: ChangeFlags 台帳後片付け束（[ctx+0x50] 書手広域探索・ExtraHavok ID 帰属・pass2 種別8語義・0x99D9xx 帰属）→ v297 `#oblivion` `#re` (2026-11-06)
  > 【2026-11-05 V279完走】成果物 tmp/re/v297_changeflags_ledger_bundle.md。件3確定（種別8=flags bl=0x08: bit3単独→force除去/bit3+他→bit11 disable標へ翻訳再発行、bit31符号で0x2c/0x1c分岐・bit23+4）。件2確定負（0x99D9xx帯は非実在=i1 ident汚染・ExtraHavok型ID空間不参加旁証確定・0x0E説棄却）。件1部分確定（pending=[ctx+0x50]丸値movのみ・接触3点・台帳API host 16関数・ハブ0x4662C9、書手は次束①）。v289 bit3/31命名=役割[確定]へ昇格。台帳row279[x] bak130・round_verify ALL OK 166/159/7。残点3（書手/bit1-2/0x463913到達元）。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#54** `blend_settings.cpp`の不要な`kOneBits`警告を除去 `#build` `#cleanup` `#oblivion` (2026-11-06)
  > #50の正規化変更に関連して残っていた未使用定数を削除。Android native debug build（3 ABI）成功、MSVC NoAsanホストテスト全体`ALL GREEN`、`BlendSettingsTests` PASS。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#53** Android native debug buildの回帰確認 `#build` `#oblivion` `#verification` (2026-11-06)
  > `.\gradlew :app:externalNativeBuildDebug`を実行し、arm64-v8a / armeabi-v7a / x86_64の3 ABIで`BUILD SUCCESSFUL`。既存のSDK XML version 4警告と`blend_settings.cpp`の`kOneBits`未使用警告は残存するが、今回の変更によるビルドエラーなし。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#52** メニュー中の `ACTION_MOVE` によるゲームプレイ入力更新をホストテストで固定 `#input` `#oblivion` `#tests` (2026-11-06)
  > `InputRouterTests`を追加し、GAMEPLAYではジョイスティックが更新され、PAUSED/INVENTORY/SAVE_MENUでは同じ`ACTION_MOVE`がゲームプレイ用ジョイスティックを更新せず、各UIゾーンへ配送されることを検証。MSVC NoAsanホストテストで4/4 PASS、全スイート`ALL GREEN`。本番コードの挙動変更はなし。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#51** v283〜v288 RE成果物の移植側照合 `#oblivion` `#re` `#re-impl` (2026-11-06)
  > 照合完了。Topic_Resolve、PlayerCharacterState の主要オフセットと再帰ガードは既存実装で対応済み。未反映は sRHand:DEFAULT の vanilla typo 正規化（#50）、369件の opcode flagA/argn メタデータ（#48）。v286 annotation 4関数は #49、RE正典追記は承認待ち #47 へ分離。ScriptEffectUpdate は移植側対応なし、NiDX9Renderer はDX9固有のためAndroid GLESへ直接移植しない。推測段階の配列名・未同定BSS・個別vtable名・実行時計測不能値は実装対象外。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#50** `sRHand:DEFAULT` の既定値を移植側で正規化 `#oblivion` `#port` (2026-11-06)
  > `blend_settings.cpp` のvanilla typo `1.0f, 1.0` を `1.0, 1.0` に正規化し、ヘッダ・ホストテストの説明と期待値を更新。Windows NoAsanホストテストで `BlendSettingsTests` PASS、全スイート ALL GREEN。実行用TODO: `normalize-srhand-blend-default`。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#49** v286確定関数を annotation 台帳へ反映 `#oblivion` `#re` `#tools` (2026-11-06)
  > `tmp/re/annotate_db.json` に `0x4526E0 List_Clear`、`0x46CB60 List_IsEmpty`、`0x530020 Topic_Resolve_Wrapper`、`0x50A120 ScriptEffectUpdate` を追加。既に正典で同定済みの `0x446CB0 Push` は重複登録していない。JSON妥当性と関数数372件を確認。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#48** opcode `flagA` / 引数数メタデータの移植側照合 `#oblivion` `#re` `#script` (2026-11-06)
  > 照合完了。RE側は369件（flagA=1:235/0:134、argn=0:166/1:139/2:53/3:5/4:3/5:3）。移植側はnative bridgeの37マッピングとdecoderの一部`declaredArgumentCount`/`argLength`のみで、flagA保持フィールドと369件対応表は存在しない。`NativeScdaOpcode`は観測候補6件、`FunctionID`は別の合成ID空間。推測で追加せず、メタデータ構造の設計・導入は別タスクへ保留。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#43** V220 残点確認: v221 残点8件＋GTドラフト6件（反映判断は V168 ゲートへ一元的に正本#20 で管理中）→ 解消確認できたらクローズ `#oblivion` `#re` (2026-11-06)
  > ホーム台帳 #28 より統合。V-221-1..8 は旧台帳 #29.1 で tmp/re/v227_opcode_gt_names9.md として解明済のため、実質残りは GT ドラフト反映判断のみ（#20 と重複）。台帳 row220 確認後に閉じる。
  > 【2026-11-05 照合実施→クローズ可】成果物 tmp/re/v292_v220_residual_check.md。v221 §7 残点8件を後続と照合: 解消7/大部分解消1(V-221-6=細目は v237/v238 tail POD 解明に包含)。各根拠: V-221-1=v242 §4[解明]、V-221-2=v227 §2 g_effectTargetMode/g_feetLevelCandidateRefs、V-221-3=v242 §5 0xB3BD00シングルトン、V-221-4=v227 §4 EffectSetting RTTI、V-221-5=v256 §5.1、V-221-7=v227 §7 161件完全列、V-221-8=v231 AlarmPackage。GTドラフト6件=整合5/上書き訂正1(formType 0x20=TESFurniture/0x0C=EffectSetting が v227 確定)。GT本体反映は #20 ゲートへ引継。selftest 4/4 PASS。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#42** （提案）Copilot バンドル v2 起案候補: 会話テーマ→商人判定の具象表（v261 TOPC/INFO 経路）と GetBarterGold の fame/infamy+週日リセット詳細 `#copilot` `#oblivion` `#re` (2026-11-06)
  > ホーム台帳 #86 より統合。2026-11-04 起票（未投入）。受領ラウンドで serial269+ として台帳起票し、優先度を実装側と確認して投入予定。
  > 【2026-11-05 完走】成果物 tmp/re/v290_copilot_bundle_v2_proposal.md。依頼2件を起案: [P1] 会話テーマ(TOPC/INFO)→商人判定の具象表（TOPC構造/メニュー起動対応表/最小判別述語）、[P2] GetBarterGold の fame/infamy+週日リセット詳細（0x5F1910 の引数矛盾=v261 0x1D vs v279/v280 Mercantile の解消を最優先、opcode 0x10F8-0x10FD 5件、週リセット機構、0x56A300()==4）。提案成果物番号 v294/v295。既確定分(CLAS不存在/価格式/骨格)は参考として除外。訂正候補2件(v261 の opid 0x10F8→正 0x1108、fame/infamy 引数)を併記、反映は #20 ゲート。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#41** round87: 台帳 row268（v275・3/3確定・残点X-1..3）反映＋残点新束起票→推奨順再投入 `#oblivion` `#re` (2026-11-06)
  > 資料照合により完了扱い。`tmp/re/TODO_re49.md` row268 にv275（3/3確定）が記録済みで、残点X-1〜X-3はrow269として新束起票・完了済み。Copilotの書き込み対象外である`.github/memory/session-memory.json`は変更していない。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#40** V278 進行中: extra 型空間後始末束（bit3/31 命名・統合表・12B param 網羅）→ v287 `#oblivion` `#re` (2026-11-06)
  > ホーム台帳 #95 より統合（2026-11-05）。2026-11-05 起票・バックグラウンド投入済（ses_e5718c49…）。受領後 round99 で台帳反映。
  > 【番号重複の注意 2026-11-05】本タスクの成果物予定 `tmp/re/v287_extra_space_bundle.md` は、Copilot バンドル（v60v76）側の本セッションが既に `tmp/re/v287_v76_data_map_residuals.md` として v287 番を使用済み。同様に v280/v281/v282/v283/v286 でも2系統のファイルが併存（例: v280_slg_attr_mask_bundle.md と v280_sSpecialization_gmst_count.md）。ファイル名が異なるため上書き・欠落は無いが、連番は二重稼働中。**V278 の成果物は v289 以降（v289 が空き）への振り分けを推奨**。出典の v286_tail_payload_bundle.md は他セッション製で既存。
  > 【2026-11-05 V278完走】成果物 tmp/re/v289_extra_space_bundle.md（予定v287→先占のためv289振替）。件1=命名[確定負]+機構[確定]（entry=8B{+0 changeFlags,+4 refcount}・キー=[form+0xC]=FormID・452C90無条件flags上書き・452DF0 force除去全caller push1・452D60 static 0x80000008/dynamic [ctx+0x50]）。件2=ID空間71値権威再構築（v286訂正: 付録未載は0x5Aのみ、差分+1）。件3=12B=3dword固定全網羅（書手452CF0caller唯一・advance全経路0xC・≥0x5B実在は0x5B/0x5Cのみ）。残点4→row279(V279)、V219残点3→row280(V280)起票。台帳 total166/done158/open8・round_verify ALL OK・bak129。subagent投入1回upstream error→v277/v286先例でinline。round_verify.py恒久再構築済。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#39** v60v76-bundle 依頼6 [P6]: 小物残件まとめ（v60/v63/v64/v69: 0x584D10 / objects 3件 / [0xB35580] / sRHand:DEFAULT / thunk 文字列 / 0x66C580 callers / opcode entry+0x10）— 成果物 tmp/re/v288_*.md `#re` `#v60v76-bundle` (2026-11-06)
  > 2026-11-04 完了: tmp/re/v288_misc_residuals.md。9点中7確定+1部分確定+1統合: (v60a)[確定] 初期化配列の走査コード=0x981C6Bのインラインループ（abs32(0xA283D0)=0x981CB5/abs32(0xA2F77C)=0x981CBCを本日再検証、v60の探査アドレス0xA28860/0xA2D184が誤り）+0x981FB4=atexitラッパで解消 (v60c)[確定] first-matchバグを完全再現（file first find=0xA5D55D=sClassSpecialization内部・abs32=0件→push未検出再現、standalone 0xA5E194→0x9F0D56）(v63-1)[確定] 0x584D10=文字列compare(stricmp)ラッパ・vtable 0xA40454の+8・CRT 0x98262D→0x982525・decompiled無定義のため命名案MapCompare_StrICase (v63-3)[確定] 3値はvtableアドレス: 0xA3BF1C=グローバルmap 0xB06164、0xA53920=埋め込みメンバ(ctor FUN_00521450他14callers)、0xA64800=別系統(FUN_00551370等) (v63-4)[部分確定] 0xB35580=map header+0x0C count・実数は静的不可能(重複スキップ/条件分岐)・静的近似=登録試行2,304/キー2,175(2,176候補) (v64-3)[確定] タイポ=値文字列1.0f,1.0、パーサFUN_0053A720のsscanf("%f, %f")で第1値はf無視OK・第2値は書式不一致で既定1.0にフォールバック→結果的に正しい、移植は1.0,1.0へ正規化推奨 (v64-4)[確定] string型runtime thunkなし（EH 0x9AB578ファミリ5件=float thunk/dword thunk/GMST registrar2/eraser）(v69-2)[確定] v283依頼1-4へ統合 (v69-3)[確定] +0x10=flagA(u8)+unused+argn(u16)、V99でflagA=参照必須、sub-table 0xB0C8C0 369件の内訳flagA 235/134・argn 0:166/1:139/2:53/3:5/4:3/5:3。selftest 4/4 PASS。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#38** v60v76-bundle 依頼5 [P5]: v76 データ関係マップ残件6点（0xB3CAC0 / FUN_007b4010 map / b43070 caps / teardown / 0xB42754 vtable / ハーネス残bug）— 成果物 tmp/re/v287_*.md `#re` `#v60v76-bundle` (2026-11-06)
  > 2026-11-04 完了: tmp/re/v287_v76_data_map_residuals.md。6点中4確定+2部分確定: (C-7)[部分確定] 0xB3CAC0=identity照合の番兵（ヘルパFUN_00560920=annotate名NiControllerManager_FindSeq・168callers・53 abs32 refs、&DAT形式で照合）(C-8)[部分確定] 埋め込みマップ=obj+8のmapインスタンス（find=FUN_0055e000=GMST/INI同一クラス）、u16キー、+0x24チェーンのアドレス一致、caller=FUN_00437590(dtor解除)/FUN_004BAA80(登録・間接)、N2維持 (C-2)[確定] b43070=(0xB06DE4!=0)&&(0xB06CC4!=0)&&CheckDeviceFormat(HAL/X8R8G8B8/Usage0x80000=SRGBREAD/TEXTURE/0x71)>=0、鍵帰属は.data 0xB06DE8/CC8の鍵ポインタで裏付け（bDoHighDynamicRange:BlurShaderHDR/bDoImageSpaceEffects:Display=既定1）、GPU detect入力=0x103/0x101/0、caller=FUN_00498340/004980D0 (C-3)[確定] teardown=CRT初期化テーブル0xA2EFF0→スタブ0xA10EB0→FUN_007B7250(0xB430D8)+atexit(0xA27150=ret空)、起動時1回のみ (5)[確定] 0xB42754=NiDX9Renderer（RTTI .?AVNiDX9Renderer@@、new(0xB00)→ctor FUN_00769BE0、vtable 0xA88EA4=98スロット全列挙、+0x54=flag query、埋め込みNiTMap vtable 0xA88A3C）(C-10)[確定] (a)rd()=V78解消済/(b)dec_index=V77でヘッダ正規表現実装確認/(c)entry162区分=events31/console132(末尾マーカーopid0x0183)/script functions369=532。selftest 4/4 PASS。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#37** v60v76-bundle 依頼4 [P4]: v75a/v75b アノテーション・来歴残件（132/39・band-6/total-7・QUST388/INFO1700・名前欠落5件 等）— 成果物 tmp/re/v286_*.md `#re` `#v60v76-bundle` (2026-11-06)
  > 2026-11-04 完了: tmp/re/v286_v75_annotation_residuals.md。7点中5確定+2部分確定: (1)[部分確定] 132/39=Supermemory旧エントリbUP3gq7DeSyw53Mdw49kmYの計測値（forget済みで手順は喪失・引用のみ残存）、124/37=一次出典がリポジトリ/Supermemory共に不存在で特定不能（v75aのA=344/126が正）(2)[確定] band-6 vs total-7=RE_EXE_REPORT L11602の原典で「新規opcode 7件/新規FunctionID 6件(0x9A-0x9F)、7つ目StopQuestは既存FunctionID」→両方正しい (3)[確定] QUST388/INFO1700=ESM全体GRUP-walk+SCDA命令walkで9/9全一致（records1,167,017・groups85,079・blocks 2393/5552/1701・insns 65628/10326/10962=86916・SetStage 1391/388/1700=3479・INFO比率15.51%・opcode141）、新規artifact v286_p4c (4)[確定] 50=SetStage最終命令SCPT数・987=原文センテンス数・220=opcode表エントリ通し番号((opid&0xFF)+163で5行一致) (5)[確定] 名前欠落5件=0x4526E0 List_Clear(240callers)/0x446CB0 Push(正典L11840で同定済)/0x46CB60 IsEmpty(96callers)/0x530020 Topic_Resolveラッパ(ret8・outフラグマスク・caller FUN_006B8E00)/0x50A120 ScriptEffectUpdate実装(.data VMA 0xB0B240テーブルが正式名保持) (6)[部分確定] fun_00530020=2引数をTopic_Resolveへ転送+フラグマスク (7)[確定] DAT_00b36510 users=FUN_0052F010(6)+FUN_00530050(3、保持子解放者・caller FUN_00530340)。selftest 4/4 PASS。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#36** v60v76-bundle 依頼3 [P3]: v72 グローバル 0xB333C4 マップ残件5点（decompile欠落195 / +0x200..+0x56C / this固定DAT / bssラベル / tail +0x7A4..+0x803）— 成果物 tmp/re/v285_*.md `#re` `#v60v76-bundle` (2026-11-06)
  > 2026-11-04 完了: tmp/re/v285_v72_b333c4_residuals.md。5点中3確定+2部分確定: (#2)[確定] decompiled欠落195=195/195がthis引数エリジョン（定義あり・本文にDATなし、未出力0件）(#3)[部分確定] +0x204..0x56C=float配列72×3+特殊float3（ctor 3ループ+3個別dwordで全覆蓋、getter FUN_0065D270/setter FUN_0065D310、caller20/20がグローバル渡し、72=AV数と一致でAV系[推定]）(#4)[確定] FUN_00660110=79/79・FUN_006600D0=38/38が常にDAT（2例外を分岐解析で解消: 0x6A0E0Aはecx=global生存パス、0x67052BはFUN_006701B0のthis=global（caller2/2））(#5)[確定] 正方向bss 230本→ラベル177/未同定53（未同定のowner集中=FUN_00445A10(38)/FUN_009D9FA0(34)、完全リスト記載）(#6)[部分確定] +0x7A4..0x7F4=float21要素のformType(0x0C..0x20)係数表（+0x130配列と正規化比算）、+0x7F8=write-only byteフラグ（書手0x5B5EF1唯一・decompiled未出力）、+0x7FC/800=float、0x7FA/7FB=ctor未初期化=残点。selftest 4/4 PASS。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#35** v60v76-bundle 依頼2 [P2]: v61 headless relabel 残件6点（prev_exists / 21240内訳 / gap 362vs357 / isExecute / cc2-cc3 59件 / §5 relabels適用判断）— 成果物 tmp/re/v284_*.md `#re` `#v60v76-bundle` (2026-11-06)
  > 2026-11-04 完了: tmp/re/v284_v61_headless_relabel.md。6点中5確定+1部分確定: (1) prev_exists=前関数の新解析実在を表す列、実測362/362 true（pre/post両方）→ V61§4.3-4弱点解消・吸収説の機構主張は決定的否定 (2) 21,240内訳=FUN_* 20,473 / __* CRT 465 / その他特殊152 / thunk_FUN_* 100 / scalar_deleting_destructor 50（合計一致、V73会計20,465+775=21,240と8件差を完全整合）(3) 362vs357差5=エイリアスopcode（OnEquip/OnActorEquip・AdvancePCSkill/ModPCSkill 等の2opid共通実装）で確定 (4) isExecute=用途はseed不在vs data誤分割の切り分け軸、実測362/362が.text(0x60000020 EXECUTE) → コードブロックに在るがseed不在が確定 (5) cc2-3 59件=57U中class1 51/class2 5/class3 1 → V5「欠落」が正・V10 absorbed_suspectは再分類すべき (6) 適用判断=部分確定: RE_EXE分は既適用(L11530/11528)、v5案A/B・v10案Bは適用可（V70/V73/V79統合と行番号再特定が条件）、v10案A（#行追記）はCSV互換性破壊で不可。docs/tmp/re正本編集は承認待ち。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#34** v60v76-bundle 依頼1 [P1]: v62 残件4点（s*Description ESM置換 / FUN_00478780 this型 / 0xB0A1A8 消費者一覧 / 0x66C580 callers分類）— 成果物 tmp/re/v283_*.md `#re` `#v60v76-bundle` (2026-11-06)
  > 2026-11-04 完了: tmp/re/v283_v62_residuals4.md。4点すべて[確定]: (1) s*Description ESM置換=12件中10件（sSpeed/sRaceはESM無し・綴り違い2件は両側一致でヒット・文字列型は FUN_004A7D60 の type6 ブランチで転送実証）(2) FUN_00478780 this=HighProcess/REFR本体ではなく ref+0x104 に保持される非多形0x154B装備アタッチ/ボーン管理オブジェクト（Bip01 9ボーン+スロット15個、vtable +0x168/16C で遅延生成）(3) 0xB0A1A8 消費者=直接3件（GetBaseActorValue/GetActorValue デバッグログ＋FUN_004FDE30=スクリプトコンパイラのAV名前解決、v62の「全AVダンプ」は誤りと訂正、opcode table +0x1C=528エントリがこの共通パーサを参照）(4) 0x66C580 caller=22件全数分類（A群12=ecxに[0xB333C4]明示、B群10=同クラスメソッドthis、本体=+0x588反転setter+時刻系グローバル副作用）。selftest 4/4 PASS。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#33** v93-bundle 依頼3: CRT 初期化テーブル 0xA2ED04..0xA2F1B0（299エントリ）の走査側コード特定（v81 §7-1）— 成果物 tmp/re/v267_*.md `#re` `#v93-bundle` (2026-11-06)
  > 2026-11-04 [確定] 走査側 = 無名 gap 関数 0x981C6B (146B) のインラインループ 0x981CC5..0x981CD4、begin/end は固定 imm32 直接ロード (0xA283D0@0x981CB5 / 0xA2F77C@0x981CBC) = MSVC _initterm 相当のインライン展開。走査範囲は v81 run の外周 7,403 スロット（299 はうち 0xA10xxx シンクロン分）。実行 = WinMainCRTStartup が起動時に 1 回 (E8 0x9877E1)。0xA10EB0 は呼び出しでなく atexit 登録スタブ (call 0x7B7250)。既に V93 §c で確定済み → 再検証のみ。成果物 tmp/re/v282_crt_init_table_walker.md (7,317B)。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#32** v93-bundle 依頼2: FUN_0040c350 唯一呼出し元 0x40F580（無名 gap [0x40DF74,0x40F640)）の関数特定・caller・到達時期（v81 §7-4）— 成果物 tmp/re/v266_*.md `#re` `#v93-bundle` (2026-11-06)
  > 2026-11-04 [確定] gap 関数 = WinMain (0x40DF80..0x40F640, 5,816B, ret 0x10@0x40F637)、直接 caller = WinMainCRTStartup@0x98769C (E8 0x98780E のみ)、到達時期 = プロセス終了時（メッセージループ退出後の shutdown 経路、call 0x40C350 @0x40F580）。起動時/ESMリロード時ではない。既に V93 §b で確定済み → 再検証のみ (v93_final_verify + v280_spot 全一致)。成果物 tmp/re/v281_gap_caller_winmain.md (5,505B)。残点: 早期 return 3本の成立条件（依頼範囲外）。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#31** v93-bundle 依頼1: sSpecialization 間接登録経路の有無 → GMST 静的登録件数 377 vs 378 の確定（出典 v81 §7-3）— 成果物 tmp/re/v265_*.md `#re` `#v93-bundle` (2026-11-06)
  > 2026-11-04 [確定] 378が正・377は反証。本件は既に V93 (v93_gmst_gt_residuals.md §a, 2026-10-25) で確定済みで、本バンドルは v81 §7-3 の旧残点を転記したもの。再解析せず結論再提示＋再検証のみ: v93_final_verify.py 再実行で ESM 382×EXE 両基準 378/差0、abs32(0xA5E194)=0x9F0D56 の push を完全一致確認。成果物 tmp/re/v280_sSpecialization_gmst_count.md (4,830B)。v81 の誤 .rdata 定数 0x4009D0（正 0x400E00）が残点の原因。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#30** Phase45 ホストテストの msvc ASan 実ハング（PID 39080・2400秒）原因特定を完了。procdump フルダンプ + PDB + PE .pdata + capstone で待機チェーンを全追跡し、決定的証拠を確定: (1) main (tid 29704) は AsyncTaskManager::cleanup() の worker.join() 内で _Thrd_join → WaitForSingleObjectEx → ntdll 待機syscall (ntdll+0x161DD4) のままブロック、最上位 EXE フレーム = cleanup@AsyncTaskManager +0x177 (0x593327)、(2) ワーカー5本は ntdll ローダーロック待機syscall (ntdll+0x1659C4) + clang_rt.asan_dynamic+0x484BC (ASan スレッド起動フック) のローダー連鎖のみで EXE フレーム0件 = workerThread() に到達不能、(3) minidump モジュール全26件列挙で Trend Micro AV のインプロセスフック tmmon64.dll (base=0x56530000, C:\Windows\System32\tmumh\20019\TmMon\2.9.0.1243\) がロード済みを確認 → tools/host_tests/README.md L220-250 の既知制約「ASan + AV フックの DLL_THREAD_ATTACH ローダーロックデッドロック = 環境制約・スイート欠陥ではない」と完全一致。コード修正不要と確定。回避検証: run_host_tests_msvc.ps1 -NoAsan で全89 TU 再ビルド → HOST RUNNER: ALL GREEN (0 suite failed) / Host tests passed (exit 0)。AsyncTaskManager 実装は健全と確認。 #phase45 #hang-analysis #asan #tmmon64 #environment-constraint (2026-11-06)
  > 2026-11-01/02 完了。Phase 45 ハング死因は Trend Micro tmmon64.dll によるデッドロックと確定（エンジン側不具合ではない）。問題点2件＋改善提案4件をユーザーへ報告済み。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#29** [v230 §5-3] jni_bridge.cpp:255-258 の fGlobalTimeMultiplier が frame-dt に乗算する仕組みはエンジン機構と不整合（エンジン側に同機構なし）。現在は恒常 1.0 で沈静化しているが、外部プラグイン等が変数を書き換えるとフレームレート依存でゲーム速度が暴れる。恒常1.0前提なら乗算自体を削除するか、エンジン風の伝播機構に合わせるかの [PORT] 判断。優先度低。根拠: tmp/re/v230_relationship_residuals.md §5-3。 #porting #re (2026-11-06)
  > Implemented and verified (host tests ALL GREEN via run_host_tests.sh, 2026-10-30). Decision: removed the frame-dt multiply (constant 1.0 assumption; no engine-side equivalent). jni_bridge.cpp: multiply block replaced by an evidence comment citing RE 54.3 (v230 3-2); the gmst include was dropped. GLOBAL_TIME_MULTIPLIER removed from settings_manager.h; the fGlobalTimeMultiplier entry removed from gmst_defaults.h time/timing group (kEntryCount 41 -> 40) and combat_formulas_tests.cpp updated. The INI default table entry (oblivion_ini_defaults.cpp:444) was intentionally kept - it is an INI-derived default, not a live GMST.
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#28** [v230 §5-2] Ability/Disease/Power 固有挙動の実装判断と VAMP/POSN の DAMAGE 丸め注記。移植 SpellEffectType は8種（9種は誤り・v229 訂正済み）、MGEF→対応表は四字正式名で一致確認済み。差分は Ability(4)/Disease(1)/Power(2,3) の固有挙動不在と VAMP/POSN→DAMAGE 近似。fourcc 正式名照合（208/409/600）は未実施。実装するか [PORT] 判断し plan.md に反映。根拠: tmp/re/v230_relationship_residuals.md §2、§5-2。 #porting #re #needs-approval (2026-11-06)
  > [2026-11-02 Copilot] 判断確定・plan.md 反映済み（plan.md L1790 "Decided 2026-11-01 (#28)"）。2部構成で判断: (1) fourcc 正式名照合完了 — 全45 mapped codes（DAMAGE 30 / HEAL 5 / RESTORE_MANA 3 / RESTORE_STAMINA 2 / PARA / INVI）を CS Wiki 公式 Magic_Effects_List と照合し全件一致。VAMP→DAMAGE は vampirism の combat manifestation が health drain なので accepted approximation、POSN→DAMAGE は poison が damage-based で consistent。(2) Ability(4)/Disease(1)/Power(2,3) の固有挙動は accepted approximation として記録 — magnitude/duration は正しく適用済みで gameplay 体感は成立。magic-type lifecycle（disease spread、once-per-day cooldown、permanent passive）は magic system 完全移植フェーズへ繰延べ。SpellData::spellType は #26 の connection point として保持。Evidence: v230 §2-2/§2-3、spell_manager.cpp:27-95、spell.h SpellEffectType 8種、cs.uesp.net/wiki/Magic_Effects_List
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#27** [v230 §5-1] gmst_defaults 41 float キーの拡張範囲判断。移植の GMST merge 語義はエンジンと同型（後勝ち・insert なし・キー集合不変）と確定したが、移植は float 41キーのみでエンジン 2,304件に対し網羅率 1.8%。どのキー群を getLive 化するかは [PORT] 製品判断。判断後 plan.md に反映。根拠: tmp/re/v230_relationship_residuals.md §1、§5-1。 #porting #re #needs-approval (2026-11-06)
  > [2026-11-02 Copilot] 判断確定・plan.md 反映済み（plan.md L1789 "Decided 2026-11-01 (#27)"）。判断内容: coverage scope は consumer-driven 40 float keys のまま維持、2304 GMST への一括拡張なし。キーは getLive コンシューマ登場と同時に kEntries へ逐次追加（現キーは全て actively consumed、fKnockbackTime のみ reserved）。カウント問題も決着: kEntries 実件数 40（kEntryCount は sizeof 由来）、fGlobalTimeMultiplier は INI General キー（oblivion_ini_defaults.cpp:444, default 1.0000, node 0xB06704）で GMST ではなく frame-dt 乗算は #29 で削除済み — v230 §1/§3 の記載は stale。Evidence: gmst_defaults.h kEntries 40件、getLive 41 matches（combat_formulas.h 22 / fatigue_system.h 16 / コメント3 / テスト1）、jni_bridge.cpp:245-275、oblivion_ini_defaults.cpp:444
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#26** [v230 §5-5/§5-6] 低優先の軽微整理2件: ①key_only_fallback 相関の find_by_key() がヘッダコメント「フォールバックしない」と書くが実装はフォールバックする（4件中3件一致、sBasePath 欠落は Reg 死にコード追随で実害なし）— コメント実態に合わせ訂正。②SpellData::spellType が未使用 — 型別挙動（Ability/Disease/Power）の接続点候補として整理方針を決める。根拠: tmp/re/v230_relationship_residuals.md §4、§5-5、§5-6。 #porting #re (2026-11-06)
  > Implemented and verified (host tests ALL GREEN via run_host_tests.sh, 2026-10-30). ① find_by_key(): header comment corrected to match the implementation (linear scan, first match wins, falls back to defaults; the "no fallback" wording was wrong). ② SpellData::spellType: kept as the connection point for #28 (Ability/Disease/Power behaviour); a comment in esm_reader.h marks it explicitly. Files: app/src/main/cpp/system/oblivion_ini_defaults.h, app/src/main/cpp/assets/esm_reader.h. Note: run_host_tests_msvc.ps1 fails with C1083 on Rust FFI headers - pre-existing runner gap (ps1 never updated for the Rust migration; $RUST_DIR tokens unexpanded, no archive linking). The supported path is Git Bash run_host_tests.sh which is fully Rust-aware.
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#25** [v230 §5-4] AI時計と setTimeScale の非接続是正。エンジンは TESGlobal TimeScale 1本が sky_weather_system / ai_scheduler / コンソールを束ねるが、移植はコンソール setter が SkyWeatherSystem にのみ効き AI 時計（sleep/wait）に伝播しない。対応: ai_scheduler へ setTimeScale 接続を実装し、ai_scheduler.h:114 の未使用 timeScale メンバを整理（使用 or削除）。根拠: tmp/re/v230_relationship_residuals.md §5-4、§3。 #porting #re (2026-11-06)
  > 2026-11-01/02 完了。setTimeScale 接続を3ファイル（ai_scheduler / jni_bridge / 予備）で実装し、ホストテスト ALL GREEN。AI時計と実時間の乖離是正済み。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#24** esm_reader.cpp (app/src/main/cpp/assets/, 約5,088行) のRust移行。2026-11-01方針「Rustで記載できる所はRustで記載」の初号案件。plan.md Rust migration節で次ターゲットと明記済み。技術方針: zlib→flate2 (bsa-coreで実証済み)、std::ifstreamはC++シム側に残す、レコード型はsyncCache()方式(NIF/BSAのopaque handle方式に準拠)、新クレート esm-core / esm-ffi。オラクル検証はhost dump differ方式(tmp/bsa_oracle, tmp/nif_oracleと同じ)。TODO #21 (V31 MEDTデッドコード) はこの移行に吸収する方針(ai-todo #21に注記済み)。参考: docs/RE_EXE_REPORT.md, tmp/re/v31_medt_deadcode.md `#completed` (2026-11-06)
  > 進捗（2026-11-02）: shim 置換完了（5188→4676行）+ CMake IMPORTED ターゲット修正 + 3 ABI シンボル確認済み。docs 反映完了（plan.md / CHANGELOG.md / ARCHITECTURE.md 各数箇所）。オラクルハーネス構築中: tmp/esm_oracle に dump.cpp（公開ESMFile API全カバレッジ）/ build_orig.ps1 / build_rust.ps1 / compare.ps1。build_orig.ps1 に2つのバグ修正済み（git show パスを repo 相対に、$cppRoot を ..\..\ に）。dump_orig.exe 再ビルド中。既知ギャップ: mem パスの type-1 GRUP worldspaceFormID（orig は未処理 / Rust は正しく付与 → 意図的改善として document 予定）。
  > 完了（2026-11-02 オラクル green）: compare.ps1 exit=0。集計 20 pairs / 9 identical / 11 different。全 11 件が orig 側 — mem は 8 件全て orig の parseFromMemory が空マスタ返し（Rust は file≡mem 一貫）、file は 2 件（DLCMehrunesRazor / Knights）で orig がレコード population 欠落（Knights は orig が CELL 0 / Rust は CELL 132・NPC_ 76・REFR 7058・LAND 61）。残り 8 file ペア（277MB Oblivion.esm: CELL 35494・REFR 1029280）はバイト一致。Rust が正しい・orig の既存バグを修正。docs（plan.md / CHANGELOG.md）にオラクル結果反映済み。commit / push はユーザー明示指示待ち。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#23** coord-watch: 他セッション（ReverseEngineering A）との二重作業監視 — TODO/docs 編集は連絡済みの連託分のみ `#coord` (2026-11-06)
  > [2026-11-02 Copilot] 監視結果: #40（V231 起票の v237 残点3件等）は RE ループ側（opencode ReverseEngineering セッション）の作業分であり Copilot 側は着手しない。二重作業なしを確認。TODO/docs 編集は連託分のみという運用ルールも維持され、本セグメントの plan.md 追記は #27/#28 の連託判断の記録（append-only Decided 段落）に該当し問題なし。coord-watch 任務は継続中だが現時点で対応が必要な二重作業は検出せず。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#22** CI commit: oblivion_ini_defaults.cpp の kFloat 修正 — 再確認完了（2026-11-02）。catch-all コミット `19fafb7f` "Save uncommitted changes" でコミット済み。HEAD の `app/src/main/cpp/system/oblivion_ini_defaults.cpp` に kFloat エントリ存在（L50/54/55 等）、未コミット差分にも含まれない。追加作業不要 `#ci` `#completed` (2026-11-06)
  > Rust方針転換(2026-11-01)の影響確認: このファイルはINI静的デフォルトテーブルでRust移行対象の即時影響はなし。ただし現況調査で、このファイルは catch-all コミット 19fafb7f "Save uncommitted changes"（87 files, +6389/−164976）に含まれて1314行変更としてコミット済みの可能性が高い。HEADのファイルには kFloat エントリが存在する。未コミットという記述が既に古い可能性があるため、コミット要否の再確認を推奨。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#21** V31: esm_reader MEDT デッドコード修正 — #24（esm_reader Rust 移行）に吸収済みで完了（esm-core は MEDT 探索デッドコードを持ち込まない方針、挙動変化ゼロを維持） `#completed` `#porting` `#re` (2026-11-06)
  > Rust方針転換(2026-11-01)による影響確認済み: plan.mdでesm_reader.cppは次Rust移行ターゲットと明記済み。推奨方針は「C++側に先行パッチせずRust移行時に吸収」— オラクル検証はgit HEADのC++と比較するため、C++側を先に変えるとベースラインがずれる。Rust実装(esm-core)ではMEDT探索デッドコード自体を持ち込まない形で対応すれば、挙動変化ゼロというV31の約束も維持される。C++側に先に適用する場合はオラクル再ベース化が必要。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#19** V192: havok/INI 命名束（O 正式クラス名・[0xB3BCF0] INI 名・23エントリ表 等）— serial 192。成果物 tmp/re/v205_havok_ini_naming.md `#completed` `#re` (2026-11-06)
  > 同期（2026-11-02）: RE 正本 tmp/re/TODO_re49.md で serial 192 が [x] 完了 (2026-10-31) を確認。成果物 v205_havok_ini_naming.md 実在（9,579 bytes, 10/31 21:46）。ローカル側が同期漏れだったため [x] 化。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#18** V191: process 細部束（KnockedState 値1 算術・[+0xB0] 名・UI キークラス名 等7件）— serial 191。成果物 tmp/re/v204_process_residuals2.md `#completed` `#re` (2026-11-06)
  > 同期（2026-11-02）: RE 正本 tmp/re/TODO_re49.md で serial 191 が [x] 完了 (2026-10-31) を確認。成果物 v204_process_residuals2.md 実在（23,969 bytes, 10/31 21:51）。ローカル側が同期漏れだったため [x] 化。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理
- [x] **#17** V188: opcode/GT 名前 第6束（+0x10 登録帯書手・base+0x58 bit7/bit22 名・slot[6] 値4）— 正本 tmp/re/TODO_re49.md serial 188。親セッションが inline 進行（証拠 v201_* 41点） `#completed` (2026-11-06)
  > 同期（2026-11-02）: RE 正本 tmp/re/TODO_re49.md で serial 188 が [x] 完了 (2026-10-31) を確認（後続 V207 が同 serial の完了行から起票されている）。ローカル側が同期漏れだったため [x] 化。
  > 2026-10-30: 子エージェント中断続きのため親セッションが inline 継続。証拠 %TEMP%/opencode/v201_*（41点）を走査済み。判明: ①+0x10 初回書手=FUN_004FF710@0x4FFB0C（[parser+0x410] が 0x14/0x1F → 0='f'、それ以外 → 1='s'）②bit22 読手13件=エフェクトリストチェッカ群（0x414EB0 系）+FUN_00419B30、直接クリアは 0x416854（FUN_004167E0）③slot[6]: SpellItem は [+0x1C] 動的返し（Enchant=6/Alchemy=7/Ingredient=8 済み）。残: v201 成果物への統合執筆。
  > Reason: 2026-11-06 RE シリアルループ完了分を含む既存完了タスクを一括整理

---

<!-- TASK RELATIONSHIPS
21:related-to:24
-->

---
**ai-todo** | Last Updated: 2026-11-06 14:33:43
