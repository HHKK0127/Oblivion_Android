# v230_relationship_residuals.md — データ関係性マップ残点4件の照合結果

- 作成: 2026-11-01 / 担当: TODO serial 223（V223・低）「関係性マップ残点4件」
- 出典本体: `tmp/re/v229_data_relationship_map.md` §4 残点残点①②③ ＋ 項目4（V217(4) crosscheck2 接続）
- 手法: 移植ソース読取のみ（`app/src/main/cpp/` 編集なし）＋ EXE 再逆アセンブル（capstone 5.0.7、v218 §7 経路の踏襲確認）＋ RE 成果物突合
- 結論早見: 項目1 **解明（同型の後勝ち層・keyspace は部分集合）** / 項目2 **解明（対応表確定・ただし「9種」は誤りで8種）** / 項目3 **解明（gmst 外・ハードコード30.0×2＋コンソール）** / 項目4 **不一致確定（移植3件/エンジン4件・sBasePath 欠落）**

---

## §1 項目1: gmst_defaults 実行時 override とエンジン ESM 読込上書きの merge 語義照合

**判定: 解明。両者とも「lookup 命中時のみ後勝ち value 置換・insert/削除なし・キー集合不変」の同一構造だが、keyspace は異なる（エンジン = GMST map 2,304 件全体、移植 = kEntries 41 float サブセットの部分集合）。**

### §1-1 移植側の実装（読取のみ）

| 要素 | 実体 | 出典 |
|---|---|---|
| keyspace | `kEntries[]` = **41 float キー**（fatigue 16 + fGlobalTimeMultiplier 1 + combat 22 + speak 2） | `gmst_defaults.h:40-89`（kEntryCount = 配列長） |
| 既定値層 | `get()` = compile-time 既定（RE 測定値） | `gmst_defaults.h:112-117` |
| override 層 | `OverrideCell{atomic<float> value; atomic<bool> set}` ＋ `setOverride/clearOverride/clearOverrides/hasOverride` | `gmst_defaults.h:127-166` |
| **merge 語義** | `getLive()` = **override 優先 → 無ければ compile-time 既定**（＝後勝ち）。`setOverride` は `indexOf` 未登録キーを**黙って無視**（＝エンジンの「未命中 je スキップ」と同型） | `gmst_defaults.h:143-148, 170-177` |
| 公開経路 | `publishGmstOverrides()`: kEntries を全走査し `esm.findGameSetting(key)` 命中かつ `valueType=='f'` のみ `setOverride`。呼出しは `loadEsm` / `parseEsmFromMemory` / `loadPluginsFromList`（プラグイン列ロード後1回）の3系統 | `assets/asset_manager.cpp:134-146, 151, 207, 272` |
| 並行性 | value.store(relaxed) → set.store(release) / set.load(acquire) → value.load(relaxed)（ワーカースレッド parse 安全） | `gmst_defaults.h:119-125, 143-148` |

### §1-2 エンジン側の EXE 再確認（v218 §7 踏襲・本タスク実測）

`%TEMP%\opencode\v230_1.py`（出力 `v230_1_out.txt`）で以下を再逆アセンブルし v218 §7 と一致を確認:

1. **GMST レコードハンドラ `0x4A8040`**（.text file 0xA7440）:
   - `0x4A8067 call 0x55E000`（this+0x10C = GameSetting map 0xB35574 への hash lookup）
   - `0x4A8070 cmp eax,ebx(0) / 0x4A8072 je 0x4A8080`（**未命中なら vtable+0x10 を呼ばずスキップ**）
   - 命中時のみ間接 call `vtable+0x10`（`0x4A8077-0x4A807C`、実体 = `0x4A7D60` 値適用）
   - 末尾常に間接 call `vtable+0x18`（`0x4A8080-0x4A8087`、実体 = `0x4A7CB0` record ptr クリア）、`0x4A808D ret 8`
   - **関数本体（0x4A8040..0x4A808D）に insert `0x412D30` の呼出し不在**（直接 E8 は lookup の1本のみ）
2. **値適用 `0x4A7D60`**: `[this+0x108]`(record) → `0x44FEA0`（次サブレコード前進）→ `0x4510B0`（chunk type）→ `0x4A7D8D cmp eax, 0x41544144('DATA')` / jne スキップ → `0x4A798D` 帯 `call 0x4A78D0`（名前先頭文字で型判別、戻り値 0..8 の型コード）→ int/float 系（型コード3/5）は **`0x4510E0`**、string（型コード6）は `0x450C20`+`0x4A7980`
3. **`0x4510E0`**: `push 4; call 0x450C20` = **DATA 4 バイトを Setting オブジェクト本体へ直接転送**
4. `0x55E000` = djb2 hash（`[esi]` vtable+4 呼び）→ バケット連鎖を `__stricmp` 相当で走査、不一致は `xor al,al; ret 8`

→ v218 §7-1 の「lookup 未命中→insert なしでスキップ／命中時のみ DATA 4B を同一 8B Setting へ置換／キー集合 2,304 不変・ESM 保有 378 キーのみ value 上書き（226 相違）」を**本タスクで再確認**。

### §1-3 merge 語義の照合表

| 軸 | エンジン（EXE 実証） | 移植（ソース実証） | 判定 |
|---|---|---|---|
| 基本構造 | static Setting 2,304 → ESM 読込で value のみ置換 | compile-time 既定 41 → ESM/PLUGIN で override 置換 | **同型** |
| 命中時 | DATA 4B → 同一 Setting へ直接転送（後勝ち） | setOverride → getLive が override 優先（後勝ち） | **同型（後勝ち）** |
| 未命中/未知キー | je スキップ・insert なし・map 不変 | setOverride が indexOf 未登録を黙殺・kEntries 不変 | **同型** |
| プラグイン順 | ロード順に ESM 側が後勝ち（値は累積保持） | loadPluginsFromList 後に findGameSetting の最終マージ値を再公開 → setOverride 上書き（累積保持） | **同型** |
| keyspace | GMST map **2,304 件全キー**（int/float/string 3型） | kEntries **41 float キーのみ**（`valueType=='f'` フィルタ） | **相違 = 部分集合（網羅率 1.8%）** |
| 型転送 | 名前先頭文字 switch（0x4A78D0）で int/float/string | float 固定（OverrideCell = atomic\<float\>） | **相違（float のみ）** |
| 例外キー | `fGlobalTimeMultiplier:General` は **INI ノードのみ**で GameSetting map に存在せず ESM 上書き対象外（v218 §7-1） | **kEntries に混入**（`gmst_defaults.h:60`）。`findGameSetting("fGlobalTimeMultiplier")` はエンジン由来 GMST が無いため恒常的に未命中＝実害ゼロだが、もし同名 GMST を持つプラグインを積むとエンジンより上書きされる | **keyspace 混入（軽微）** |

### §1-4 判定文

- 「移植 override が ESM 上書き層に相当」（v229 §3 表）は**成立**。後勝ちの順序・キー集合不変・未知キー黙殺が両者で一致する。
- ただし keyspace が異なるため、**移植 getLive が返し得る値はエンジンの runtime GMST 値の 41 キー射影**であり、ESM 独有 GMST（例 `fRegionGenTex*` 等4件）は両者とも取り込まない点も一致（エンジンは未命中スキップ、移植は走査対象外）。
- 残るのは「41 キーをどこまで拡張するか」という [PORT] 側の製品判断（§5 新規残点1）。

---

## §2 項目2: 移植 SpellEffectType ↔ エンジン MagicType の対応表

**判定: 解明（対応表を確定）。ただし前提の「移植 SpellEffectType 9種」は誤り — 実体は 8 列挙子（`spell.h:17-26`）。v229 §1-4 の「…」は8種の省略表記と読む。**

### §2-1 前提訂正と軸の分離

- 移植 `SpellEffectType`（`spell.h:17-26`）= **8種**: DAMAGE / HEAL / RESTORE_MANA / RESTORE_STAMINA / FORTIFY_ATTR / PARALYZE / INVISIBILITY / SUMMON
- エンジン `MagicType 0..8`（v216 §3.1: CEF `Magic::MagicTypes` ＝ UESP SPEL SPIT 仕様と三重照合済み）= **9種**: Spell=0 / Disease=1 / Power=2 / LesserPower=3 / Ability=4 / Poison=5 / Enchantment=6 / AlchemyItem=7 / Ingredient=8（SPIT 先頭 dword → SpellItem+0x34、`FUN_0041B40E` の 16B 一括読出、v216 項2）
- **両者は軸が異なる**: MagicType は SPEL（魔法アイテム）単位の型、SpellEffectType は MGEF（効果）単位の分類。変換橋は移植 `mgefToEffectType()`（`spell_manager.cpp:30-70`）で、**入力は MGEF editorID 4文字コード＋school、出力は SpellEffectType**。MagicType は移植 `SpellData.spellType`（`esm_reader.h:372`、0..5 注記）としてパースされるが `loadSpellsFromESM()` では**未使用**。

### §2-2 MGEF fourcc → 移植 SpellEffectType（`spell_manager.cpp:30-70` 実体）

| 移植 SpellEffectType | 判定条件（mgefToEffectType） | 該当 MGEF editorID fourcc（コード列挙） |
|---|---|---|
| DAMAGE（既定含む） | 4文字コードが以下30件に一致／nullptr / school==2 既定 | ダメージ系: `FIDG` `FRDG` `SHDG` `DGHE` `DGFA` `DGSP` `DGAT` / ドレイン系: `DRHE` `DRFA` `DRSP` `DRAT` `DRSK` / 破壊系: `DIAR` `DIWE` / その他: `SUDG` `POSN` `VAMP` `STMA` / 弱体化系: `WKDI` `WKFI` `WKFR` `WKMA` `WKNW` `WKPO` `WKSH` / 吸収系: `ABHE` `ABFA` `ABSP` `ABAT` `ABSK` |
| HEAL | コード一致5件 | `REHE` `FOHE` `CUDI` `CUPO` `CUPA` |
| RESTORE_MANA | コード一致3件 | `RESP` `FOSP` `FOMM` |
| RESTORE_STAMINA | コード一致2件 | `REFA` `FOFA` |
| PARALYZE | コード一致 | `PARA` |
| INVISIBILITY | コード一致 | `INVI` |
| SUMMON | `mgef->school==1`（Conjuration）かつ `linkedFormID != 0` | （fourcc 非依存・結合 formID 有無で判定） |
| FORTIFY_ATTR（フォールバック） | 上記以外のうち school!=2 | （例: `FOAT` 等はフォールバックで FORTIFY_ATTR） |
| DAMAGE（フォールバック） | 上記以外のうち school==2（Destruction） | — |

→ v229 §1-4 の「MGEF コード→移植 enum 変換橋」は**全分岐を確定**。

### §2-3 エンジン MagicType 0..8 ↔ 移植側の対応表

| MagicType | エンジン側意味（v216 §3.1 確定） | 移植側の実際の挙動 | 対応状態 |
|---|---|---|---|
| 0 Spell | 通常スペル | `getAllSpells()`（SPEL）→ 効果毎に `mgefToEffectType()` → 上表8種へ。代表選択 `FUN_00419B30` の type 依存分岐なし | **実効対応** |
| 1 Disease | 疾病（代表選択 FUN_00419B30 → `DISE` fourcc） | `SpellData.spellType=1` でパースされるが効果マッピングは Spell と同一、疾病固有（DISE 選択・感染経路）なし | **部分対応（固有挙動なし）** |
| 2 Power | 神殿の力（常時所持） | 同上（固有挙動なし） | **部分対応** |
| 3 LesserPower | 小さな力 | 同上（固有挙動なし） | **部分対応** |
| 4 Ability | 常時発動能力（vampirism = Ability 4 と v216 で確定） | `VAMP` fourcc が kDamageCodes に含まれ **DAMAGE へ**（Ability 効果をダメージ扱い） | **語義差（近似）** |
| 5 Poison | 毒（代表選択 → `POSN` fourcc、全 hostile 判定） | `POSN` ∈ kDamageCodes → **DAMAGE** | **一致** |
| 6 Enchantment | 巻物・杖等のエンチャント | ENCH は `EnchantmentData`（`esm_reader.h:399-`）別系統で `enchanting_system.h` が処理、`mgefToEffectType()` 非経路 | **別系統（SpellEffectType 非使用）** |
| 7 AlchemyItem | 錬金品（FUN_00414EB0 の毒判定対象） | 未実装（`getAllSpells` は SPEL のみ） | **エンジンのみ** |
| 8 Ingredient | 素材 | 未実装 | **エンジンのみ** |

### §2-4 判定文

- 「非同数＝要整理」（v229 §3 表）は**解消**: 8（MGEF 効果分類）× 9（SPEL 型）は軸が異なるため同数化は不要で、上表の多対多で表現できる。
- 実質的な相違は2点: ①Ability(4)/Disease(1)/Power(2,3) の**型固有挙動が移植に無い**、②移植の DAMAGE 集約が弱体化(WK*)・吸収(AB*)・Fortify(FOHE/FOSP等)まで1ラベルに丸める近似。
- fourcc の UESP 正式名照合（`STMA`/`SUDG` 等）は本タスクでは未実施＝新規残点（§5 項2）。

---

## §3 項目3: TimeScale の移植側供給経路同定

**判定: 解明。TimeScale は gmst_defaults 消費者（combat/fatigue/asset/jni 4系統）とは無関係の別経路で、移植は「ハードコード 30.0 ×2 箇所 ＋ コンソール setter」の3系統。ESM/GMST 由来の供給経路は存在しない。**

### §3-1 移植側の供給経路（全数・grep 実証）

| # | 経路 | 実体 | 出典 |
|---|---|---|---|
| 1 | **SkyWeatherSystem 時計** | `timeScale_` 初期値 **30.0f ハードコード**（メンバ初期化 + ctor 再代入）。時間進行 `hours = dt * timeScale_ / 3600.0f` | `engine/sky_weather_system.h:146, 168-173, 271-273, 442` |
| 2 | **コンソール `settimescale`** | `GameConsole::cmdSetTimeScale` → `gameRefs.setTimeScale` → `renderer.cpp` のラムダが `skyWeatherSystem->setTimeScale(s)`（0.0 以上に clamp） | `ui/game_console.cpp:702, 2476-2485` / `engine/renderer.cpp:1262-1265` / `sky_weather_system.h:271-273` |
| 3 | **AI スケジューラ時計（第二時計）** | `constexpr DEFAULT_TIME_SCALE = 30.0f` ハードコードで `gameHour += dt*30/3600`。**経路2 の setter とは非接続**（コンソール変更は反映されない）。なお `ai_scheduler.h:114` に未使用の `float timeScale = 1.0f` メンバが残存 | `game/ai_scheduler.cpp:150-156` / `game/ai_scheduler.h:114` |

- **gmst_defaults 経由の TimeScale 供給は不在**: `kEntries` に `fTimeScale`/`TimeScale` キー無し（`gmst_defaults.h:40-89`）。gmst_defaults の消費者は v229 §1-2 の4系統どおりで、いずれも TimeScale を読まない（grep 確認）。
- **`fGlobalTimeMultiplier` は別機構**: `settings_manager.h:36-51` が「EXE ではゲーム時計 `0x47D170`（`0x47D249 fmul [0xB06704]`）でのみ乗算され、frame-delta 倍率ではなく TimeScale（index 0x3A・既定30.0）とは別機構」と明記。にもかかわらず移植 `jni_bridge.cpp:255-258` は **frame deltaTime に `gmst::getLive("fGlobalTimeMultiplier")` を乗算**している（ヘッダ自体が「恒常1.0で恒等に戻すべき」と注記。keyspace の詳細は §1-3 例外キー行）。

### §3-2 エンジン側 consumers（v222 項3/項4 の確定分）との対応

| エンジン（RE 確定） | 対応する移植実体 | 状態 |
|---|---|---|
| TESGlobal `TimeScale`（`0xB332E0+0x14`、既定 **30.0**（.rdata `0xA37CC8`）、DataHandler が ESM 未定義時に formID 0x3A でランタイム生成） | `sky_weather_system.h` の `timeScale_`（30.0f 初期値）。**ESM GLOB 由来の値供給なし** | 既定値一致・供給源相違 |
| 増分パス1: `hours_delta = T·x/3600`（T = `[[0xB332E0+0x14]+0x24]`、x = `[0xB33E9C]` 実秒 delta、3600 は double `0xA2F938`） | `sky_weather_system.h:173` `hours = dt * timeScale_ / 3600.0f` | **式同型** |
| 待機1時間ステップ: `FUN_004029E0(d)` with d = 3600/T（待機 fast-forward、`FUN_0065F770`） | 移植に待機系 TimeScale 乗算は未確認（wait/rest 実装が別系統） | 未照合（残点外・参考） |
| コンソール "Set TimeScale to"（TESGlobal 書き込み経路） | 経路2（settimescale → setTimeScale） | **対応** |
| fGlobalTimeMultiplier（INI node `0xB06704` のみ・GMST ではない、ゲーム時計限定） | `jni_bridge.cpp:258` が frame dt に乗算 ＝ **エンジン機構と不整合**（見かけ上恒等1.0で沈静化） | **機構相違（記録）** |

### §3-3 判定文

- v229 §3「移植側 TimeScale 供給経路は graph で未同定（残点）」は**解消**: sky_weather_system（ハードコード30.0＋setter）、ai_scheduler（ハードコード30.0・独立）、コンソールの3系統。いずれも gmst_defaults と ESM は非依存。
- エンジンは「TESGlobal 1 本が時計・待機・コンソールを束ねる」（v222 項4: DataHandler が ESM GLOB を解決する形で runtime 値を供給）のに対し、移植は**ハードコード値が2系統に分岐**しており、コンソール変更が AI 時計に伝播しない（§5 新規残点4）。

---

## §4 項目4: 移植 key_only_fallback のキー集合とエンジン bare 名4件の照合

**判定: 不一致（3/4件）。移植既定表は ':' 無し（section=""）エントリを3件のみ登録し、`sBasePath` は不在。ただし sBasePath はエンジンでも Reg バックエンド起動読出1回のみの死にコード（v51）であるため機能差は実害なしレベル。**

### §4-1 エンジン側 bare 名4件（RE 確定分）

| キー | エンジン値 | node / site | バックエンド | 出典 |
|---|---|---|---|---|
| `fLowPerfCombatantVoiceDistance` | 1000.0 (f32) | `0x00B14BAC` / `0x009FD542` | INI（0xB07BF0 系） | v91_ini_defaults.csv / crosscheck2:308（UNRESOLVED: no colon） |
| `iDetectionHighNumPicks` | 40 (hex) | `0x00B14904` / `0x009FD272` | INI | crosscheck2:309 |
| `iLastHDRSetting` | 4294967295 = -1 (hex) | `0x00B06F54` / `0x009DEEF2` | INI | crosscheck2:310 |
| `sBasePath` | `.\`（.data 文字列 `0x00B04628`） | `0x00B0472C` / `0x009DAC82` | **Reg（0xB07D28）唯一ノード** | v91_ini_defaults.md:61,137,177 / v51 §3.3 |
- v91_ini_defaults.md:137「`:` 無し（セクション MAIN 扱い）4件」＝エンジン bare 名集合の確定値。':' 無しは ReadOne 分割ヘルパ `0x4A8430` の既定セクション MAIN(0xA404C4) 扱い（v218 項5）。

### §4-2 移植側の登録集合（読取実証）

- `oblivion_ini_defaults.cpp`（657エントリ、grep `^    {` 実測657＝ヘッダ「657 keys in total」と一致）のうち **section="" は3件のみ**（:50-52）:

| 移植登録 | section | value | type | provenance |
|---|---|---|---|---|
| `fLowPerfCombatantVoiceDistance` | `""` | "1000" | kFloat | 0x02(kFromEngine) |
| `iDetectionHighNumPicks` | `""` | "40" | kInteger | 0x02 |
| `iLastHDRSetting` | `""` | "-1" | kInteger | 0x02 |
| `sBasePath` | — | **不在（grep 全ソース0件）** | — | — |

- ヘッダ自体も「Key-only fallback for the **three** compiled-in nodes…(iLastHDRSetting, iDetectionHighNumPicks, fLowPerfCombatantVoiceDistance)」と3件明記（`oblivion_ini_defaults.h:54-57`）＝sBasePath 欠落は**意図的選択**（Reg バックエンド起動読出のみ・以後死にコードという v51 判定に追随）。
- 参考: `fQuestScriptDelayTime` は移植では `{"MAIN", ...}` として ':' 正規化後に登録（:633）＝crosscheck2 の「MAIN 正規化」と接続済み。

### §4-3 key_only_fallback 経路と crosscheck2 正規化の接続

`OblivionIni::resolve()`（`oblivion_ini.cpp:383-422`）の優先順位:
1. `entries_` exact (section, key) → kFile
2. `ini_defaults::find(section, key)`（バイナリサーチ、:179-204）→ kDefault
3. `key_only_fallback_ 有効時のみ`: `by_key_` の**任意セクション**キー一致 → kFile（`':' 無しキーでもファイル値を解決` ＝ engine ReadOne の MAIN 既定化に対応）
4. `ini_defaults::find_by_key(key)` → kDefault

- これにより、Oblivion.ini に `[Display]` 等へ置かれた `iLastHDRSetting` 等が section 指定なしでも解決でき、**v218 項5 の crosscheck2 正規化（':' 無し→MAIN）と同一の結合規則**を移植はフォールバック層で実装している。
- **コメント/実装の不一致を1件検出**: `find_by_key()` のヘッダコメントは「Returns null when the key is not a **section-less** entry」（`oblivion_ini_defaults.h:54-57`）だが、実装（`oblivion_ini.cpp:206-217`）は全657エントリをキー名で線形走査し**任意セクションの最初の一致を返す**（section=="" チェックなし）。挙動としては v218 項5 が求める正規化に適うが、コメントは実装を正しく記述していない（§5 新規残点5）。

### §4-4 判定文

- 「移植 key_only（':' 無し）のキー集合がエンジン bare 名4件と一致するか」→ **不一致: 移植3件 / エンジン4件**。欠落1件は `sBasePath`（Reg バックエンド由来）。
- 実害評価: `sBasePath` はエンジンでも起動 Triplet の1回読出のみで以後読取ゼロ（v51 §397-398・imm32 参照は初期化3箇所のみ）＝移植が Reg バックエンドを廃止方針（v57 §6: no-op スタブ推奨）である以上、**機能差はなし**。ただし「657エントリで bare 名4件を完全網羅」という記述を作る場合は3件と訂正すること。

---

## §5 新規残点（本タスクで新たに発生）

1. **kEntries 41 キーの拡張方針**（§1-4）: エンジン GMST map 2,304 に対し移植 getLive 層は float 41 キーのみ（網羅率約1.8%）。どこまで getLive 化するかは [PORT] 製品判断。優先度 中。
2. **mgefToEffectType の fourcc 正式名照合**（§2-4）: kDamageCodes の30件中 `STMA`/`SUDG`/`VAMP` 等の UESP 正式名（EffectSetting editorID）との厳密照合が未実施。Ability/Disease/Power 型の固有挙動欠落と併せて魔法系の次タスク候補。優先度 中。
3. **jni_bridge の fGlobalTimeMultiplier frame-dt 乗算**（§3-2）: エンジン RE §54.3（ゲーム時計 `0x47D170` 限定・`settings_manager.h:36-51` 自身の注記）と機構不整合。現在は恒常1.0で沈静化しているが、同名 GMST を持つプラグインで沈静化が破れる。[PORT] 対応判断待ち。優先度 低。
4. **ai_scheduler 第二時計の setTimeScale 非接続**（§3-1 経路3）: コンソール `settimescale` が SkyWeatherSystem だけに効き、AI スケジューラの gameHour 推進は固定30のまま。エンジンは TESGlobal 1 本で両方を駆動。優先度 中。
5. **`find_by_key()` のヘッダコメントと実装の不一致**（§4-3）: コメントは「セクション無し entry のみ」、実装は全エントリ線形走査。挙動は妥当、コメント修正候補。優先度 低。
6. **`SpellData.spellType` の未使用**（§2-1）: 移植は SPIT type 0..5 をパースするが `loadSpellsFromESM()` で未使用（effectType も legacy 注記のみ）。型別挙動実装時の接続点。優先度 低。

---

## §6 検証情報

- EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B、v218 §9 と同一バイナリ）。VA→file: .text +0x400C00 / .rdata +0x400E00 / .data +0x401400。
- capstone 5.0.7（skipdata）。再逆アセンブル対象: `0x4A8040`（GMST ESM ハンドラ）/ `0x4A7D60`（値適用）/ `0x4A78D0`（先頭文字型判別）/ `0x4510E0`（DATA 4B 転送）/ `0x55E000`（hash lookup）。
- selftest: 着手時および本タスク実施中に `probe_harness.py selftest` **4/4 PASS**（bytes×2 / string / opid → ALL PASS、変更なし）。
- スクリプト（新規のみ）: `%TEMP%\opencode\v230_1.py`（＋ `v230_1_out.txt`）。
- ESM 本体: **未アクセス**（禁止遵守）。
- 移植ソース: 読取のみ（gmst_defaults.h / spell.h / spell_manager.cpp / oblivion_ini.cpp / oblivion_ini_defaults.h / oblivion_ini_defaults.cpp / asset_manager.cpp / jni_bridge.cpp / ai_scheduler.cpp/.h / sky_weather_system.h / renderer.cpp / game_console.cpp / settings_manager.h / esm_reader.h、いずれも編集なし）。
- RE 成果物（読取のみ）: v229 本体 / v218 §7・項5 / v216 §3.1（MagicType）/ v222 §3・§4（TimeScale）/ v91_ini_defaults.md / v51_inisettings_spec.md / copilot_ini_crosscheck2.md。
