# v229_data_relationship_map.md — データ関係性マップ（graphify 実証 × RE 確定分の統合）

- 作成: 2026-11-01 / 担当: TODO serial 222（V222・ユーザー依頼「データ同士の関係性把握」）
- 手法: ①`graphify query` / inline NetworkX（`Oblivion_Android/graphify-out/graph.json` = 47,657 nodes・本日 16:01 再構築）②RE 成果物の確定分統合（出典は各件明記）
- 語彙展開（Step 0 規約）: `settings gmst register registry magic effect spell ini defaults map esm`（graph 語彙 5,623 tokens から選択）
- **制約**: グラフは FUN カバレッジ 46.73%・EXE⇔移植の構造エッジ 0 本（V202/v211 確定）＝本マップの「移植側」はグラフ実証、「エンジン側」は RE 成果物実証で、**両者の接続は §3 の対応表で明示的に結ぶ**（グラフが結んでいないことをグラフに帰属させない）。
- エッジ注意: AST ティアのエッジは relation/confidence が空（`?`/`[]`）＝**構造的な同居（フィールド/ヘッダ定義/呼び出し）**として読むこと。

## §1 移植（ポート）側の関係 — graphify グラフ実証

### §1-1 INI 系
```
IniDefaultEntry (oblivion_ini_defaults.h:38)
  ├─ section / key / value / type / provenance (同 :39-43)
  ├─ IniValueType (:25)  ← kInteger/kFloat/kString（HEAD で改名済み・V212 検証）
  └─ oblivion_ini.cpp: entry_less()(:168) / find()(:179) / find_by_key()(:206)

OblivionIni (oblivion_ini.h:38, degree=39)
  ├─ entries_ / by_key_ / keys_by_section_ / sections_ (:150-153) ＝4インデックス
  ├─ parsed_lines_ / skipped_lines_ (:155-156)
  └─ key_only_fallback() (:125) ＝「':' 無しキー」処理（エンジン側 MAIN 正規化と対応・V217(3)）
```
→ **既定表（ IniDefaultEntry ）→ パーサ（ OblivionIni 4 索引）→ key_only フォールバック**の3層。

### §1-2 GMST 系
```
gmst_defaults.h (L1)
  ├─ hasKey()(:102) / get()(:112) / indexOf()(:134)
  ├─ OverrideCell(:127) / setOverride()(:143) / clearOverride()(:150)
  │   / clearOverrides()(:156) / hasOverride()(:162)  ＝実行時上書き機構
  └─ 消費者（ファイル級エッジ）: combat_formulas.h / fatigue_system.h
      / asset_manager.cpp / jni_bridge.cpp
```
→ **gmst_defaults は「既定値＋実行時 override」の2層**で、combat/fatigue/asset/jni の4系統が消費。

### §1-3 ESM 系（ハブ）
```
ESMFile (esm_reader.h:1145, degree=228)
  ├─ m_recordIndex / m_formIDIndex (:1295/1297)
  ├─ m_weapons / m_npcs / m_quests / m_cells 等 (:1305-1309)
  ├─ GameSettingData (:863): formID/editorID/valueType/stringValue/numericValue
  │     └─ findGameSetting() (:1563)
  └─ （呼び出し側）SpellManager::loadSpellsFromESM() (spell_manager.h:42)
```
→ **ESMFile が最大ハブ（degree 228）**。GameSettingData と スペル読込がここに集合。

### §1-4 魔法系
```
SpellManager (spell_manager.h:22, degree=27)
  ├─ spells (unordered_map) / createSpell()(:38) / loadSpellsFromESM()(:42)
  └─ spell_manager.cpp:30 mgefToEffectType()  ＝★エンジン MGEF コード→移植 SpellEffectType 変換橋
SpellEffectType (spell.h:17): DAMAGE/HEAL/RESTORE_MANA/RESTORE_STAMINA/
  FORTIFY_ATTR/PARALYZE/INVISIBILITY/SUMMON …
  └─ SpellEffect (spell.h:28) → ActiveStatusEffect (npc_manager.h:36)
Enchantment (enchanting_system.h:166)
  ├─ id/name/type/effect/actorValue/magnitude/duration/charges/enchantCost (:167-179)
  └─ EnchantmentType(:27) / MagicEffect(:46) / EnchantingSystem(:213)
```
→ **MGEF コード（エンジン語彙）を `mgefToEffectType()` が移植 enum へ変換**する接点が存在。

## §2 エンジン（EXE）側の関係 — RE 確定分（出典付き）

### §2-1 設定データ3層（+GMST）
| 層 | 実体 | 出典 |
|---|---|---|
| ① INI レジストリノード | **637 ノード（635 静的 + 2 wrapper）**・'Key:Section' 混在名 604 件・fGlobalTimeMultiplier:General もこちら | v218 §1-2 / v208 |
| ② GameSetting map | **static 2,304 件**・':' を含むキーは登録 0 件（bare 名のみ） | v208 / v130 |
| ③ ESM GMST 上書き | ESM 読込後 lookup→**未命中は je でスキップ（insert なし）**、命中時のみ DATA 4B を Setting へ転送。値上書き 378 件中 226 相違 | v218 §7 |
| 経路 | 登録ヘルパ FUN_00419A70（static・call 1,312）/ FUN_0041BAE0（float・992）／wrapper 2 件は FUN_00404850/0x444060 の caller 各1 | v218 項1 |
| 表記差 | Controls INI キー正=**空白なし**（テーブル 0xB02BB8・29件）⇔ GMST `sUActnSld*` は表示名（空白付き）＝**別系統** | v218 項4 |

### §2-2 魔法データ系（MGEF ↔ EffectItem ↔ SPIT）
```
MGEF レコード mgefFlags (+0x58, EffectSetting)
  bit0 Hostile / bit7 NoDuration / bit21 PCHasEffect / bit22 Disabled
  bit23 UnknownO(POSN/DISE のみ) / bit30 MagnitudeIsLevel / bit31 MagnitudeIsFeet
    ⇅ EffectItem (+0x00 mgefCode, +0x04 magnitude, +0x08 area, +0x0C duration,
                  +0x10 range, +0x14 actorValue, +0x18 ScriptEffectInfo*,
                  +0x1C EffectSetting*, +0x20 cost)
    ⇅ MagicItem 埋め込み sub: SPIT 16B 一括読出 → +0x34(=sub+0x1C)
        SPIT = type(0..8: Ability=4) / SpellCost / SpellLevel / Flags
    ⇅ EffectSettingCollection [0xB33508]（fourcc キーの MGEF 専用マップ）
代表選択 FUN_00419B30: type5/全hostile→POSN, type1→DISE, 他→Disabled でない最大 effect
```
出典: v216（bit 定数・SPIT 契約）/ v221（bit23 多経路 reader・POSN/DISE 0x800000 登録）/ v201 / v215。
→ **v221 で「エンジン MGEF コード」＝移植 `mgefToEffectType()` の入力語彙**が確定し、§1-4 の橋渡しが両側から閉じる。

### §2-3 時間・時計系
```
TESGlobal 6本 [0xB332E0]: GameYear(+0)/GameMonth(+4)/GameDay(+8)/GameHour(+xC)
  /GameDaysPassed(+10)/TimeScale(+14, 既定30.0)   ← DataHandler formID 0x35..0x3A
  ⇅ 増分パス1: 3600/T·x（T=TimeScale・x=[0xB33E9C] 実秒 delta）
  ⇅ 累積時計 [0xB3BCF0]（実秒累積・セーブ4B永続）⇄ [0xB3BCF4] フェーズマーカ
```
出典: v222 項3-4 / v205（INI 名なし・経過秒累積クロック）。

### §2-4 process / 状態系
```
TESObjectREFR+0xB0 LifeState（7値: 0Alive..6Normal・名前テーブル 0xB09EF8）
  ← SetUnconscious(opid 0x10F1) tail-jmp / SetLifeState FUN_005E6680 ← FSM FUN_006545E0
KnockedState {1,3,5,6} ← FSM 内部のみ（外部 setter 0x629440 は在らず）
装備: active process(0x18C)+0xE4=WEAP 装備12B ⇔ 補助(0x154)+0xE4=3Dアドオン第3スロット
1st/3rd: +0x104/+0x5C8 ← byte [Player+0x588]=Show1stPerson
```
出典: v222 項1/7/8 / v217 項1 / v204（LifeState）。

## §3 レイヤ跨越対応表（エンジン ↔ 移植 ↔ 台帳）
| データ群 | エンジン（EXE 実証） | 移植（グラフ実証） | 接続状態 |
|---|---|---|---|
| INI 既定値 | 637 ノード・INI_REG_ADD(0x4040E0) | IniDefaultEntry→OblivionIni 4索引 | **kFromBoth provenance で対応**（V200 で 84キー=同梱ini由来確定） |
| GMST | GameSetting map 2,304＋ESM 上書き | gmst_defaults.h（get+Override 2層） | 移植 override が「ESM 上書き層」に相当・**merge 語義は要照合（残点）** |
| 魔法効果 | MGEF/EffectItem/SPIT/四cc map | SpellManager+mgefToEffectType+Enchantment | **変換橋 `mgefToEffectType()` を両側で特定**・語義対応表は移植 enum 9種 vs エンジン type 0..8＝**非同数＝要整理（残点）** |
| 時間 | TESGlobal 6本+TimeScale | （graph で未同定 — gmst_defaults 消費者は combat/fatigue/asset/jni の4系統のみ） | 移植側 TimeScale 供給経路は graph で未同定（残点） |
| Controls | 0xB02BB8 空白なし29件 | key_only_fallback | 表記差 A/B 判定待ち（[PORT] ゲート・V219-4） |

## §4 未接続・限界（正直な記録）
1. グラフの **EXE⇔移植構造エッジ 0 本**（V202/v211 確定）＝§3 は本マップの明示対応であり機械エッジではない
2. グラフ AST エッジの relation/confidence 空（`?`）＝§1 は同居関係の実証、呼び出し方向は decompiled/RE 側で補完
3. FUN カバレッジ 46.73%・後半欠落（tree-sitter 解析失敗・v211）＝EXE 側は §2 の RE 成果物が正
4. 残点（次回起票候補）: ①移植 gmst_defaults override と ESM 上書きの merge 語義照合 ②移植 SpellEffectType 9種 ↔ 引擎 MagicType 0..8 の対応表整備 ③TimeScale の移植側供給経路同定
