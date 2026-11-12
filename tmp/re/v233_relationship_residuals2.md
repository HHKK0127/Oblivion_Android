# v233_relationship_residuals2.md — 設定・関係性残点第2束（7件）の照合結果

- 作成: 2026-11-01 / 担当: TODO serial 226（V226・中）「設定・関係性残点第2束」
- 出典本体: `tmp/re/v230_relationship_residuals.md` §5 項1〜6 ＋ `tmp/re/v232_color_residuals2.md` 新規残点 N-3
- 手法: 移植ソース読取のみ（`app/src/main/cpp/` 編集なし）＋ EXE 再逆アセンブル（capstone 5.0.7、PE 手動解析・IAT 解決つき）＋ UESP（cs.uesp.net CS Wiki）正式名リストの web 突合
- 結論早見: 項目1 **材料本体として解明（2,304 全数抽出・kEntries 40/41 同名同型一致）** / 項目2 **解明（41/41 fourcc 正式名一致・語義差3群を確定）** / 項目3 **解明（3 乖離で機構不整合を実証）** / 項目4 **解明（grep 全数で非接続実証）** / 項目5 **解明（コメント誤り・実装が正）** / 項目6 **解明（未使用実証）** / 項目7 **解明（型照合自体が存在しない生コピー往復）**
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**

---

## §1 項目1: kEntries 41 キーの拡張方針（[PORT] 判断材料本体）

**判定: 材料本体として解明。エンジン GameSetting map 2,304 件を static 登録ヘルパの全 call site から全数抽出し、kEntries 41 と同名・同型照合を全数完了。40/41 が float 登録と一致、不在は fGlobalTimeMultiplier 1 件のみ（= INI ノード）。拡張の可否・範囲判断は本タスクでは不下（[PORT]）。**

### §1-1 抽出手法と全数一致（EXE 実測）

`%TEMP%\opencode\v233_gmst_scan.py`（出力 `v233_gmst_scan_out.txt` / `v233_gmst_keys.txt`）:

- .text 全体の E8 rel32 で登録ヘルパを全走査: **FUN_00419A70（dword+string 版）= 1,312 call site / FUN_0041BAE0（float 版）= 992 call site**（v218 項1 の call 網羅数と一致）。
- 各 call site の直前 40 バイト窓から push imm32 / mov r32,imm32 のうち「識別子風 ASCII 文字列（先頭英字＋数字/underscore）」を **call に最も近い1件**を name として採用（MSVC の右から左 push により name が最後＝v218 L224 の `push default; push "sUActnSldleft"; call 0x419A70` 形と整合）。far 側に拾った既定値文字列64件（"Back"/"Bald"/"Block" 等）は破棄済み（監査ログ付き）。
- **結果: unique 名 2,304 件**（v208/v218 の map サイズと完全一致）。`:` を含む名 0 件・両ヘルパ重複 0 件（v218「':' 名 0 件」の再現）。

**エンジン型別内訳（先頭文字＝エンジン型判定、本タスク実測）**:

| 型 | 件数 | 登録ヘルパ | 移植 OverrideCell(atomic\<float\>) での吸収 |
|---|---|---|---|
| float ('f') | **992** | FUN_0041BAE0 のみ | **可**（現行機構そのまま） |
| int ('i') | **227** | FUN_00419A70 のみ | 不可（型付きセル新設が必要） |
| string ('s') | **1,085** | FUN_00419A70 のみ | 不可（同） |
| b/c/h/u/r/a | **0** | — | — |

### §1-2 kEntries 41 との同名・同型照合（全数）

| キー群 | 件数 | エンジン float 登録と一致 | 備考 |
|---|---|---|---|
| fatigue（gmst_defaults.h:42-57） | 16 | **16/16 一致** | 0xB37530 等の slot 注記と float 登録の存在が両立 |
| fGlobalTimeMultiplier（:60） | 1 | **不在（0/1）** | GameSetting map に登録なし＝INI ノード "fGlobalTimeMultiplier:General" 専用（v218 項1/§43 を本タスクで再確認。kEntries 混入は仕様逸脱、ESM 上書きが原理的に発生しない） |
| combat/block/knockback/perk/speak（:63-88） | 24 | **24/24 一致** | fKnockbackTime も engine 側は float 登録実在（:74 の "unwired" は移植側配線の話） |

→ **同名・同型一致 40 / 41。エンジン不在 1（fGlobalTimeMultiplier）。**

### §1-3 消費実在別・拡張優先度リスト（材料）

**P0 — 消費実在（移植がハードコードで消費中・GMST slot 注記あり・getLive 化で ESM override が即座に効く）: 6 件**（本タスクで全件 engine float 登録実在を確認）:

| キー | slot | 移植での現状 | 出典 |
|---|---|---|---|
| fDifficultyDamageMultiplier | 0xB37B88 | `DIFFICULTY_DAMAGE_MULTIPLIER = 5.0f` constexpr（EXE 10.0 → ESM 5.0 overlay 注記あり、override 経路なし） | settings_manager.h:31-34 |
| fAIMinGreetingDistance | 0xB36AD8 | `AI_MIN_GREETING_DISTANCE = 150.0f` constexpr | settings_manager.h:54-61 |
| fPerkLightArmorNoviceDamageMult | 0xB374F0 | `ARMOR_PERK_NOVICE_DAMAGE_MULT = 1.5f` → `armorPerkDamageMultiplier()` → **spell_manager.cpp:470 で実消費** | settings_manager.h:64-70 |
| fPerkHeavyArmorNoviceDamageMult | 0xB374F8 | 同上（heavy/light 同値のため1本に統合済み＝slot 2本ぶんの情報が失われている） | 同上 |
| fPerkLightArmorJourneymanDamageMult | 0xB37500 | `ARMOR_PERK_JOURNEYMAN_DAMAGE_MULT = 0.5f` 同上 | settings_manager.h:72-79 |
| fPerkHeavyArmorJourneymanDamageMult | 0xB37508 | 同上 | 同上 |

**P1 — 登録済みだが矛盾・未配線（整理対象）: 2 件**

| キー | 状態 | 出典 |
|---|---|---|
| fKnockbackTime | kEntries 登録済み・engine float 登録実在も確認するが、getLive 呼出し・static_assert ガードともに存在せず「登録だけ先に済んだ」状態（"impulse time, unwired"） | gmst_defaults.h:74 / combat_formulas.h grep で consumers 0 |
| fGlobalTimeMultiplier | kEntries に混入しているが engine GMST ではない（§1-2）。**kEntries からの削除または明示的例外化**が [PORT] 候補。消費側 jni_bridge.cpp:258 は項目3 の機構不整合と直結 | gmst_defaults.h:60 / jni_bridge.cpp:258 |

**P2 — 未消費 float 残り 952 件（992 − 40）**: 機能実装時に「kEntries 追加 ＋ 消費者 ＋ static_assert ガード」を同時に付ける運用が現行パターン（fatigue/combat の先例）。「網羅 getLive 化」は機械的忠実性のみで実利なし＝非推奨。

**型別の結論**: 拡張は float 992 件プール内でのみ即実行可能。int 227 / string 1,085 は型付き OverrideCell の新設が前提で、消費者実在がゼロの現状では拡張非推奨（string 系は localization の s* キー要否調査が別途必要）。

**網羅率の再計算**: 41/2,304 = **1.78%**（float 層では 40/992 = 4.0%）。v230 の「約1.8%」を実測で裏付け。

### §1-4 41 キーの消費実在マップ（grep 全数）

| 群 | getLive 消費 | 出典 |
|---|---|---|
| fatigue 16 | **16/16**（fatigue_system.h:69-130 に getLive 16 本） | grep 実測 |
| block 5 + armor 1 + knockback 5（Time 除く） + sneak 10 + speak 2 = **23** | **23/23**（combat_formulas.h:125-253） | grep 実測 |
| fKnockbackTime | **0**（unwired） | §1-3 P1 |
| fGlobalTimeMultiplier | 1（jni_bridge.cpp:258）だが機構不整合（項目3） | §3 |

→ 実効消費 39 キー（getLive 経由）＋ 機構不整合 1 ＋ 未配線 1 = 41。

---

## §2 項目2: mgefToEffectType fourcc の UESP 正式名厳密照合

**判定: 解明。移植 `mgefToEffectType()`（spell_manager.cpp:30-70）が参照する 41 fourcc（kDamageCodes 30 + HEAL 5 + RESTORE_MANA 3 + RESTORE_STAMINA 2 + PARA/INVI）はすべて UESP/CS Wiki の EffectSetting 正式 editorID と一致（41/41・不存在コードなし）。一方で「正式名の語義」と移植ラベルの適合には差異3群（近似3件・語義差3件）を確定。Ability/Disease/Power 型固有挙動欠落も下表で整理。**

### §2-1 出典（web・読取のみ）

- **cs.uesp.net/wiki/Magic_Effects_List**（oldid=40309・2026-11-01 取得）= CS 公式 EffectSetting 一覧（Abbr 列 = EDID 4 文字）。
- **cs.uesp.net/wiki/Magic_Effects**（oldid=24965）: 「effect code = editorID 先頭 4 文字の ASCII」＝fourcc 語義の公式定義（"FIDG"=0x47444946 の例を明記）。
- v216 §3.1（MagicType 0..8 と SPIT 三重照合）・v216 §4（DISE/POSN メタエフェクト）と整合。

### §2-2 41 コードの UESP 正式名突合表

| 移植ラベル | fourcc（全件一致） | UESP 正式名 | 語義判定 |
|---|---|---|---|
| DAMAGE | FIDG / FRDG / SHDG | Fire Damage / Frost Damage / Shock Damage | **一致** |
| DAMAGE | DGHE / DGFA / DGSP / DGAT | Damage Health / Fatigue / Magicka / Attribute | **一致** |
| DAMAGE | DRHE / DRFA / DRSP / DRAT / DRSK | Drain Health / Fatigue / Magicka / Attribute / Skill | **一致** |
| DAMAGE | DIAR / DIWE | Disintegrate Armor / Weapon | **一致** |
| DAMAGE | SUDG | **Sun Damage**（日光ダメージ） | **一致**（v230 で未照合だった中核が確定） |
| DAMAGE | WKDI / WKFI / WKFR / WKMA / WKNW / WKPO / WKSH | Weakness to Disease / Fire / Frost / Magic / Normal Weapons / Poison / Shock | **一致**（弱体化も DAMAGE へ丸め＝v230 §2-4 の近似指摘を維持） |
| DAMAGE | ABHE / ABFA / ABSP / ABAT / ABSK | Absorb Health / Fatigue / Magicka / Attribute / Skill | **一致**（吸収も DAMAGE へ丸め） |
| DAMAGE | **POSN** | **Poison Info**（毒メタエフェクト） | **語義差（傍証獲得）**: 正式名は "Poison" ではなく "Poison **Info**"。v216 の「POSN/DISE は bit23 のみ set メタエフェクト」と CEF コメントが UESP 正式名と一致。移植→DAMAGE はエンジン毒代表選択（type5→POSN）と衝突せず実害小 |
| DAMAGE | **VAMP** | **Vampirism**（吸血鬼化能力） | **語義差（確定）**: 正式語義は Ability 型の常時能力。移植→DAMAGE は v230 §2-3 の「Ability 効果をダメージ扱い」を正式名で裏付け |
| DAMAGE | **STMA** | **Stunted Magicka**（マジッカ減衰・出生特性「Atronach」等） | **語義差（確定）**: 正式語義はステータス修正系（base cost 0）。移植→DAMAGE は過剰ラベル |
| HEAL | REHE | Restore Health | **一致** |
| HEAL | **FOHE** | **Fortify Health** | **近似**: Fortify（持続強化）を即時 HEAL に丸め |
| HEAL | CUDI / CUPO / CUPA | Cure Disease / Poison / Paralysis | **一致**（ただし移植 HEAL 実装は HP 回復のみ＝治癒 semantics なし） |
| RESTORE_MANA | RESP | Restore Magicka | **一致** |
| RESTORE_MANA | **FOSP** | **Fortify Magicka** | **近似**: Fortify を即時回復に丸め |
| RESTORE_MANA | **FOMM** | **Fortify Magicka Multiplier**（マジッカ回復倍率） | **近似**: 「回復率強化」を即時回復に丸め（v230 で未照合だった正式名が確定） |
| RESTORE_STAMINA | REFA | Restore Fatigue | **一致** |
| RESTORE_STAMINA | **FOFA** | **Fortify Fatigue** | **近似** |
| PARALYZE | PARA | Paralyze | **一致** |
| INVISIBILITY | INVI | Invisibility | **一致** |
| SUMMON | （fourcc 非依存: school==1 && linkedFormID≠0） | UESP 側は ZCLA/ZSCA/ZFIA…・BW*/BA*/MYHL/MYTH 等 50 コード超 | **合理的**: linkedFormID 判定なら召喚/バウンド全コードを fourcc 列挙なしで網羅可能 |

### §2-3 Ability / Disease / Power 型固有挙動欠落の整理（v230 §2-3 の追補）

| MagicType | エンジン固有挙動（RE 確定分＋本タスク正式名補強） | 移植側欠落内容 |
|---|---|---|
| 1 Disease | 代表選択 FUN_00419B30 が type1→**DISE**（"Disease Info" メタエフェクト、正式名は UESP 一覧で確認）。感染経路（スクリプト/接触）と治癒（CUDI）が型として機能 | spellType 未使用（項目6）＝疾病 SPEL が通常 SPEL と同一挙動。CUDI は HEAL ラベルで HP 回復に転用され治癒機能なし |
| 2 Power / 3 LesserPower | 常時所持・cast コスト無視・取得経路（种族/神殿）が型として機能 | manaCost 課金対象・自動付与なし・UI 分類なし（v230 の「固有挙動なし」を維持） |
| 4 Ability | 常時 ActiveEffect ストリーム（vampirism = Ability 4、FUN_005239c0 の型4→"VAMP" 照合） | **VAMP（正式名 Vampirism）/ STMA（Stunted Magicka）が DAMAGE に落ちる**＝Ability 常時効果がダメージ化。常時効果機構なし |
| 5 Poison | 代表選択 type5→**POSN**、hostile 効果のみ可（OBSE doc と v216 と整合） | POSN→DAMAGE で近似成立。hostile-only ルールと毒消費経路（alchemy）の接続は未実装 |
| 6 Enchantment / 7 AlchemyItem / 8 Ingredient | エンジンのみ（v230 維持） | 別系統（ENCH）／未実装（ALCH/INGR は getAllSpells 非対象） |

### §2-4 判定文

- v230 §5 項2「fourcc 正式名照合未実施」は**解消**: 41/41 一致・不存在 0。正式名確定により語義差は「POSN=Poison Info / VAMP=Vampirism / STMA=Stunted Magicka」の3件に特定。
- 近似群（FOHE/FOSP/FOMM/FOFA の Fortify→即時回復丸め）は v230 §2-4 の「1ラベル丸め」指摘に具体的コードを追加したにとどまり、挙動差は [PORT] 側の効果系設計課題。

---

## §3 項目3: jni_bridge の fGlobalTimeMultiplier frame-dt 乗算とエンジン RE54.3 の機構不整合

**判定: 解明（不整合を 3 乖離で実証）。現在は「恒常 1.0 沈静化」で実害ゼロだが、沈静化の条件（GMST 恒常未命中）と破壊条件（同名 GMST を持つプラグイン）を確定。**

### §3-1 乖離の実態

| 軸 | エンジン（RE 確定） | 移植（ソース実証） | 乖離 |
|---|---|---|---|
| ① 乗算対象 | ゲーム時計へ折り込む**経過秒のみ**（FUN_0047D170、`0x47D249 fmul [0xB06704]`。settings_manager.h:39-44 の注記どおり frame-dt 倍率ではない） | **frame deltaTime 全体**（jni_bridge.cpp:258 `deltaTime *= gmst::getLive("fGlobalTimeMultiplier")`） | renderer / sky / AI scheduler / throttle（SceneTimeThrottle::frame_update, jni_bridge.cpp:264）へ波及する**エンジンに存在しない全域倍率** |
| ② 供給源 | **INI レジストリノード** "fGlobalTimeMultiplier:General"（0xB06704、実読 1.0f。v218 項1/§43。GameSetting map 2,304 登録点に ':' 名 0 件＝GMST ではない。本タスクの全数抽出でも本キー不在を再確認） | **GMST 経路**: kEntries:60 に混入 → しかし publishGmstOverrides は `esm.findGameSetting(key)` 必経（asset_manager.cpp:139-141）＝エンジンに GMST が無いため**恒常未命中 → setOverride 不発 → getLive は恒常 compile-time 1.0** | 供給系統そのものが別物（INI node ⇔ GMST lookup） |
| ③ INI ファイル経路 | Oblivion.ini [General] fGlobalTimeMultiplier を読めばゲーム時計に効く（ノード実体） | 移植は oblivion_ini_defaults.cpp:444 に {"General","fGlobalTimeMultiplier","1.0000"} を**登録済みだが消費者ゼロ**（grep 全 5 件がコメント/宣言/登録/kEntries のみ。OblivionIni::resolve 経由の読出なし）＝**ユーザー設定が死ぬ** | 移植 INI 既定表の同名エントリは死にデータ |

### §3-2 沈静化と破壊条件

- **沈静化**: ②の恒常未命中により deltaTime は恒等（×1.0）。「見かけ上エンジンと一致」はこの偶然による。
- **破壊条件**: 同名 GMST（"fGlobalTimeMultiplier"）を持つ ESP/ESM プラグインを積むと findGameSetting 命中 → setOverride 発火 → frame dt が倍率化。TimeScale 30 のプレイヤーが 2.0 を入れると**全サブシステムが 2.0 倍速**（エンジンではゲーム時計だけが速くなる）になり、エンジンで再現不能な挙動になる。
- settings_manager.h:46-51 の NOTE 自身が「EXE path が正しければ jni_bridge を恒等 1.0f へ戻すべき」と記録済み。**恒等化（jni_bridge.cpp:258 の削除）がエンジン整合の正解**で、[PORT] 判断待ち（残点優度は v230 の Low を維持。ただし破壊条件の明確化により「プラグイン同梱時のリスクあり」に格上げの根拠を持つ）。

---

## §4 項目4: ai_scheduler 第二時計が setTimeScale 非接続

**判定: 解明（移植ソース grep 全数で経路分岐を実証）。コンソール settimescale は SkyWeatherSystem だけに到達し、AI スケジューラの gameHour 推進は固定 30.0 のまま。エンジンの「TESGlobal 1 本で両時計を駆動」との構造差を実証。**

### §4-1 2 経路の分岐実証

| 経路 | 実体 | 出典 |
|---|---|---|
| **コンソール経路**（sky 時計のみ） | `cmdSetTimeScale`（Usage 表示含む）→ `gameRefs.setTimeScale(scale)` → renderer のラムダ `if (skyWeatherSystem) skyWeatherSystem->setTimeScale(s)` → `timeScale_ = max(0.0f, scale)` | ui/game_console.cpp:2476-2494 / ui/game_console.h:145 / engine/renderer.cpp:1262-1266 / engine/sky_weather_system.h:271-273, 442 |
| **AI 時計経路**（独立固定） | `AIScheduler::update()` 内 `constexpr float DEFAULT_TIME_SCALE = 30.0f;` → `gameHour += (deltaTime * DEFAULT_TIME_SCALE) / 3600.0f`。**setTimeScale への接続文は関数内に存在しない** | game/ai_scheduler.cpp:147-156 |
| 接続の欠落証拠 | `grep setTimeScale\|timeScale` 全数: renderer ラムダ・sky 定義・tests・game_console の 2 箇所以外に**消費者なし**。ai_scheduler は grep ヒットなし。ai_scheduler.h:114 には未使用メンバ `float timeScale = 1.0f` が残存（update は constexpr を使いメンバを参照しない＝二重ハードコード） | grep 実測 / game/ai_scheduler.h:114 |

### §4-2 影響とエンジン対比

- settimescale 実行後: sky 時計（時間帯・天候スケジュール）だけが new rate で進み、**AI の package 時刻条件（`stack.evaluate(gameHour, ...)`、ai_scheduler.cpp:182）は 30x 固定のまま**＝2 時計が散開する。
- エンジン: TESGlobal `TimeScale`（0xB332E0+0x14、既定 30.0）1 本が「増分パス1（3600/T·x）・待機 fast-forward・コンソール書込」を束ねる（v222 項3-4 / v230 §3-2）。
- 訂正候補（[PORT]）: AIScheduler に setTimeScale setter を設け renderer ラムダから二重配送する、または両時計を SkyWeatherSystem の gameHour に集約。あわせて ai_scheduler.h:114 の未使用メンバ削除。優先度は v230 の「中」を維持。

---

## §5 項目5: find_by_key() のヘッダコメントと実装の不一致

**判定: 解明（コメント誤り・実装が正）。実装の「任意セクション線形走査」は resolve() と type_of_file_value() の両呼び出し路が前提とする広義挙動であり、エンジン ReadOne の ':' 無し→MAIN 正規化に合致する。誤りはヘッダコメント1箇所のみ。訂正ドラフトを提示。**

### §5-1 3 対 1 の突合

| 側 | 内容 | 出典 |
|---|---|---|
| ヘッダコメント | "Key-only fallback for the **three** compiled-in nodes whose name literal carries no section … **Returns null when the key is not a section-less entry.**" | system/oblivion_ini_defaults.h:54-57 |
| 実装 | `kDefaultCount` 全線形走査・`to_lower(key)` のみ比較・**section チェックなし**・最初の一致を返す | system/oblivion_ini.cpp:206-217 |
| 呼出し 1 | `resolve()` の last-resort（exact → find → by_key_（file）→ **find_by_key**（default）の第4段） | system/oblivion_ini.cpp:415-420 |
| 呼出し 2 | `type_of_file_value()`（find 未命中時に key-only で型を解決。resolve():411 から **section=nullptr** で呼ばれ得る＝広義走査が前提） | system/oblivion_ini.cpp:345-354, 411 |
| テスト | 3 件の bare 名＋大小無視＋未知キー→null のみ検証。section 限定挙動のアサートなし | tests/oblivion_ini_tests.cpp:270-282 |

### §5-2 実勢（kDefaults 657 行の重複実態）

`%TEMP%\opencode\v233_ini_keys.py`（出力 `v233_ini_keys_out.txt`）:

- section=""（bare 名）は **3 件のみ**（L50-52: fLowPerfCombatantVoiceDistance / iDetectionHighNumPicks / iLastHDRSetting）。3 件とも同名のセクション付き登録は存在せず単一登録。
- **同名キーのセクション跨ぎ重複が 15 キー**（例: fBlurRadius = BlurShader/BlurShaderHDR/BlurShaderHDRInterior、inumblurpasses = 3 セクション）。find_by_key は **(section,key) ソート順で最小セクションのもの**を返す（fQuestScriptDelayTime: General@L445 を MAIN@L633 より先に返す）。
- 重複15キーが実在する以上、「section-less entry のみを返す」コメントは**実装とも実勢とも不一致**。

### §5-3 裁定とコメント訂正ドラフト（コード変更なし）

- **実装が正**: last-resort および type_of_file_value の両路が「セクションを問わずキー名で既定値の型/値を引く」ことを要求しており、エンジン ReadOne の ':' 無しキー→MAIN 既定化（v218 項5）の広義対応にあたる。コメントの「section-less entry のみ」は狭義誤記。
- 訂正ドラフト（oblivion_ini_defaults.h:54-57 を以下に置換）:

```cpp
/// Key-only fallback: returns the first kDefaults entry whose key matches
/// (case-insensitive; any section; ties resolve to the lowest section in
/// table order), else null. Used by OblivionIni::type_of_file_value() and
/// as resolve()'s last resort after the exact (section, key) lookup and
/// the file by_key_ lookup - the engine-side counterpart of the ReadOne
/// MAIN normalization for ':'-less keys.
```

---

## §6 項目6: SpellData.spellType（0..5）が loadSpellsFromESM で未使用

**判定: 解明（未使用を実証）。読み込み経路での spellType 設定は decodeSpell 内 1 箇所のみで、以降移植 C++ 全体で参照ゼロ。loadSpellsFromESM は spellType を一切消費しない。**

### §6-1 証拠（grep 全数）

| 側 | 内容 | 出典 |
|---|---|---|
| 宣言 | `uint8_t spellType = 0; // 0=spell, 1=disease, 2=power, 3=lesser, 4=ability, 5=poison` | assets/esm_reader.h:372 |
| 代入（1 箇所のみ） | `spell.spellType = static_cast<uint8_t>(spit->data[0]);`（SPIT 16B 一括読出の先頭 dword 下位 byte） | assets/esm_reader.cpp:1157（decodeSpell 内） |
| 参照 | **移植 C++ 全体 grep（`spellType`）＝上記 2 件のみ**。それ以外の read/call なし | grep 実測 |
| 消費側 | `loadSpellsFromESM()`（spell_manager.cpp:120-171）が読むのは formID / fullName / editorID / school / cost / effectFormIDs / effectMagnitudes / effectDurations のみ。**s.spellType 参照なし** | 読取実証 |

### §6-2 併記事実

- `SpellData.effectType`（esm_reader.h:381 "legacy: derived effect category"）も**代入・参照とも 0 件**（同じく死にフィールド）。
- 対照的に `MagicEffectData.effectType`（esm_reader.h:428）は **alchemy_system.cpp:96 で参照されている**が、こちらは**代入が 0 件**（＝常に 0）— 新規残点 N-1（§8）。
- 接続点（型別挙動実装時）: spell_manager.cpp:129-136 の Spell 生成部で `s.spellType` を Spell 側へ引き継ぐのが最小変更。SPEL の SPIT は 0..5 が公式仕様（v216 三重照合、UESP SPEL ページ）。

---

## §7 項目7: type8 読み戻し型不一致（v232 N-3）— FUN_004a8d85 帯の capstone 復元

**判定: 解明（N-3 の問い「型不一致時のフォールバック」は実在しない）。RegQueryValueExA 読出系は型照合を一切行わない生コピー往復であり、type8 blob は 4 バイトのままノードへ復元される（往復一致）。期待型計算は API が上書きする lpType スロットへの事前格納（デッドストア）だった。**

### §7-1 Ghidra 抽出失敗帯の復元と vtable 位置付け

`%TEMP%\opencode\v233_type8_readback.py`（出力 `v233_type8_readback_out.txt`）＋ `v233_vtable.py` / `v233_vtable2.py`（PE 手動解析・IAT 解決: RegQueryValueExA = IAT 0xA28010 / RegSetValueExA = 0xA28000 / RegCloseKey = 0xA2800C）:

- **RegSettings バックエンド vtable = 0xA4059C**（.rdata 実読）: `+00=0x4A8E50`（"RegSettings" dump ヘルパ）/ `+04=0x4A8BC0`（RegSetValueExA writer＝v232 §2-1 の書出関数）/ **`+08=0x4A8D20`（RegQueryValueExA reader＝本項の読出関数）** / `+0C=0x4A8C90` / `+10=0x4A8CF0`（RegCloseKey）。
- `FUN_004a8cf0`（0x4A8CF0-0x4A8D16）= RegCloseKey ラッパ（`[this+0x108]` の hKey を閉じて 0 化）。decompiled.c L134677 と capstone が逐語一致＝v232 の「RegCloseKey ラッパのみ抽出」を再確認。
- **読出関数 = 0x4A8D20..0x4A8E40**（sub esp,0x158 / security cookie / `ret 4`）。decompiled.c には不在（Ghidra 抽出失敗域）＝capstone 実読で全補完。

### §7-2 読出関数の実挙動（逐行復元）

```
004A8D36  mov ebp,[esp+0x164]        ; ebp = ノード（[ebp+4]=キー名）
004A8D41  xor bl,bl                  ; bl = 成功フラグ初期 false
004A8D4E  cmp [edi+0x108],0 / sete   ; hKey 未開なら vtable+0x14 で開く
004A8D76  mov ecx,ebp
004A8D78  call 0x4a7a80              ; cbData = 先頭文字別サイズ（'a'→4）
004A8D7D  mov [esp+0x18],eax         ;   → lpcbData 初期値
004A8D81  mov eax,[ebp+4] / push eax
004A8D85  call 0x4a78d0              ; type = 先頭文字型判定
004A8D8D  test eax,eax / je 0x4A8DA0 ; t=0(b) → 3 (REG_BINARY)
004A8D91  sub eax,6 / neg eax / sbb eax,eax / and eax,3 / add eax,1
                                     ; t=6(s) → 1 (REG_SZ)、
                                     ; **その他（t=8 'a' 含む）→ 4 (REG_DWORD)**
004A8DA5  cmp eax,1 / mov [esp+0x14],eax / jne 0x4A8DDD
                                     ; 期待型（≠REG_SZ）→ ノード直書きパスへ
                                     ; （REG_SZ のみ名前を 's' 書換＋一時バッファ [esp+0x5c] 使用）
004A8DDF  mov eax,[edi+0x108]        ; eax = hKey
004A8DE5  lea edx,[esp+0x18]         ; arg6 lpcbData = &cbData（'a'→4）
004A8DE9  push edx / push ecx        ; arg5 lpData = ノード本体（非文字列パス）
004A8DEB  lea ecx,[esp+0x1c]         ; = original[esp+0x14]（**期待型の格納先＝lpType 出力スロット**）
004A8DEF  push ecx / push 0 / push esi / push eax
004A8DF4  call [IAT RegQueryValueExA]
004A8DFA  test eax,eax / sete bl     ; ERROR_SUCCESS → bl=1
004A8E03  cmp dword[esp+0x14],1      ; ★lpType に **API が書いた実型**（期待型は上書き済み）
004A8E08  jne 0x4A8E16               ; 非 REG_SZ → 後処理なしで成功終了
004A8E0A  ... call 0x4a7980          ; REG_SZ のみ一時バッファをノードへ格納
004A8E30  mov al,bl / ret 4          ; 戻り値 = 成功フラグのみ
```

### §7-3 N-3 への回答

| 問い | 結論 |
|---|---|
| 期待型 4 と書出型 8 の「食い違い」は読み戻しでどうなる？ | **比較されない**。計算値は lpType（出力）スロットへの事前格納で、RegQueryValueExA が実型で上書きする（デッドストア）。戻り値は成功フラグ bl のみ |
| type8 値のフォールバック（値破棄か生読みか） | ** neither — 型不一致の概念が存在しない**。RegQueryValueExA は型を検証せず lpData へ生コピーするため、dwType=8・cbData=4 で書いた pack DWORD は**ノードへそのまま復元（生往復成功）**。lpType==1(REG_SZ) チェックは文字列後処理の分岐専用 |
| 実在する失敗経路 | (a) レジストリ値が cbData(4) を超過 → ERROR_MORE_DATA → bl=0・ノード不変、(b) キー名先頭 s/S 経路で実型が非 REG_SZ → 一時バッファ内容がノードに到達せず値無視（この経路だけ型の偶発的な守りがある） |
| v232 の記述の扱い | 「将来 'a' 登録追加時は読み戻し系で型不一致となり得る」は**訂正**: 'a' キーは正常往復する。REG_RESOURCE_LIST(8) として外部ツールから見えてもエンジンは型を問わない |

---

## §8 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-1 | **MagicEffectData.effectType（esm_reader.h:428）が未代入のまま alchemy_system.cpp:96 で消費**: `mgef->effectType == 4/1/2/3` の damage 分岐が死にコード（全コードで代入 0 件＝常に 0）。fire/frost/shock/drain 系素材のポーションが else（healAmount）に落ちる。[PORT] 側修正候補（editorID fourcc からの分類充填。照合済み fourcc 辞書 §2-2 を再利用可） | 中 |
| N-2 | **kEntries の fGlobalTimeMultiplier 混入**（gmst_defaults.h:60）: engine GMST 非在の確定（§1-2）＋項目3 の破壊条件と直結。削除 or 明示的例外化の [PORT] 判断 | 低 |
| N-3 | **kDefaults の fQuestScriptDelayTime 二重登録**（General@L445 / MAIN@L633）: find_by_key はソート順で General を返す。':' 無し→MAIN 正規化（v217(3)/v230 §4-2）との意図的二重か要照合（[PORT] 前提の表設計確認） | 低（参考） |

---

## §9 検証情報

- EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B、v218/v230 と同一バイナリ）。VA↔file: .text +0x400C00 / .rdata +0x400E00 / .data +0x401400（本タスクで PE セクション表を手動解析し image_base=0x400000 を確認のうえ使用）。
- capstone 5.0.7（skipdata）。本タスクの再逆アセンブル対象: 0x4A8CF0（RegCloseKey ラッパ）/ **0x4A8D20-0x4A8E40（RegSettings reader＝N-3 解明の核心）** / 0x4A8E50（dump ヘルパ）/ .text 全体 E8 call スキャン（0x419A70: 1,312 site / 0x41BAE0: 992 site）。IAT 実解決（RegQueryValueExA=0xA28010 等）。vtable 実読: 0xA404F8（INI 系）/ **0xA4059C（RegSettings 系）**。
- selftest: 着手時に `probe_harness.py selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）。
- スクリプト（新規のみ・%TEMP%opencode/）: `v233_type8_readback.py`（＋ `_out.txt`）/ `v233_vtable.py` / `v233_vtable2.py` / `v233_gmst_scan.py`（＋ `v233_gmst_scan_out.txt` / `v233_gmst_keys.txt` 全 2,304 件）/ `v233_ini_keys.py`（＋ `v233_ini_keys_out.txt`）。
- ESM 本体: **未アクセス**（再スキャン禁止を遵守）。
- 移植ソース: 読取のみ（gmst_defaults.h / spell.h / spell_manager.cpp / spell_manager.h 周辺 / esm_reader.h / esm_reader.cpp / jni_bridge.cpp / ai_scheduler.cpp / ai_scheduler.h / oblivion_ini.cpp / oblivion_ini_defaults.h / oblivion_ini_defaults.cpp / asset_manager.cpp / settings_manager.h / renderer.cpp / game_console.cpp / game_console.h / sky_weather_system.h / alchemy_system.cpp / fatigue_system.h / combat_formulas.h / tests（参照のみ）、いずれも編集なし）。
- web 資料（読取のみ）: cs.uesp.net/wiki/Magic_Effects_List（oldid=40309）/ cs.uesp.net/wiki/Magic_Effects（oldid=24965）。
- RE 成果物（読取のみ）: v230 本体 / v232 本体（N-3）/ v229 本体 / v218_ini_docs_bundle3.md / v216_opcode_gt_names7.md / tmp/re/cache/named_funcs.txt。
