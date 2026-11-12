# V59 — 登録キー件数の差分3組（Desc 45/33・AV名 36/40・Icon 17/33）の実名列挙と原因分類

- 作成日: 2026-10-24 / 作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
- 対象: `Oblivion.exe` (Steam v1.2.0416) — `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
- 解析: PE を `struct` 自前解析 + capstone 5.0.7（バイナリ実機）。Ghidraデコンパイルは本タスクでは不使用（バイナリ実読で全数検証）
- 調査対象: `tmp\re\v57_settings_unified_spec.md` §3 の矛盾 #1〜#3（§7-4「差分キーの所在・またはカウント法の相違は未検証」の解消）
- **既存ドキュメントは無編集。本ファイルが V59 の新規成果物。スクリプトは `%TEMP%\opencode\` にのみ配置。**

---

## 1. セルフテスト（4件 PASS）

| # | 検証 | 期待 | 実測 | 結果 |
|---|------|------|------|------|
| ① | `bytes@0x504050` (8B) | `518b4c24188b5424` | `518b4c24188b5424` | **PASS** |
| ② | `bytes@0x4FB580` (4B) | `578b7908` | `578b7908` | **PASS** |
| ③ | `string@0xA7035C` | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | **PASS** |
| ④ | オペコードテーブル opid `0x0015` impl==`0x504560` | impl一致 | idx21(VA `0xB0B290`) name=`OnLoad`、opid u32 @+0x08=`0x15`、impl @+0x18=`0x504560` | **PASS** |

実行: `python "%TEMP%\opencode\v59_st.py"` → `SELFTEST 4/4 PASS`

---

## 2. 抽出手順と全センサス

### 2.1 抽出対象の列挙（CRT 初期化テーブル起点）

| 項目 | 実測値 |
|---|---|
| `.rdata` 内の code-pointer 連続配列（CRT 初期化配列） | `0xA283D4` 〜 `0xA2D184`、**4,972 エントリ**（`0xA2885C` に 1 つの 0 で区切り、先頭 290 + 後続 4,682）。先頭セグメントは昇順でなく、先頭 7 件が `0xA1627A..0xA16310`、以降 `0x9D7E40..0x9DC350` |
| 付随する別の code-pointer 配列 | `0xA2D1E0` 〜 `0xA2F778`（2,407 エントリ、`0xA086C0..0xA163C0` を指す。用途は本タスク未特定）。**setter 呼出を含む関数は 0 件** |
| setter `0x419A70`（obj）への `E8 rel32` 呼出点 | **1,312** |
| float setter `0x41BAE0` への `E8 rel32` 呼出点 | **992** |
| 合計 | **2,304**（V49/V57 の 1,312 + 992 = 2,304 と一致） |
| 呼出点を内包する初期化関数 | **2,065 関数**（うち 9 関数が複数呼出＝計 248 呼出、最大 138 呼出 = `0x9F3400`）。残り 2,907 エントリは setter を含まない |
| 初期化配列エントリを先頭に持たない setter 呼出点 | **0 件**（2,304 件すべて「直前の初期化配列エントリ以降」に位置） |

**登録経路の網羅性（追加検証）**: 中央マップ `0xB35574` へ直接 `insert 0x412D30` を呼ぶ箇所は全 `.text` 中 **2 箇所のみ**（`0x419AF5` = obj setter `0x419A70` 内、`0x41BB65` = float setter `0x41BAE0` 内）。他の 45 呼出点は別マップ（`0xB06164` ほか）向け。→ **マップへの挿入は 2 種の setter に集約されており、本 2,304 件が登録の全数**。

### 2.2 抽出パターン（capstone 実機、2,304/2,304 件で成立）

| setter | パターン | key 位置 |
|---|---|---|
| obj `0x419A70` | `push <value>; push <key>; mov ecx,<obj>; call 0x419A70` | call 直前の `push imm32` |
| obj（malloc 系） | `push <value>; push <key>; mov ecx,eax; call 0x419A70`（後続 `mov [bss_slot],eax`） | 同上 |
| float `0x41BAE0` | `fld <src>; push ecx; fstp [esp]` **or** `mov ecx,<obj>` 並びの 2 形。末尾は `push <key>; call 0x41BAE0` | call 直前の `push imm32` |
| float の value ソース | `fld dword ptr [imm32]` = **730**、`fld1`(1.0f) = **113**、`fldz`(0.0f) = **149**（計 992） | — |

- 取得項目 = **(key 文字列, value, 設定オブジェクトVA)** の 3 点組。**2,304/2,304 件で key 文字列解決、obj VA も 2,304/2,304 件解決**（内 2,056 件は `mov ecx, imm32` 直値、248 件は malloc 系で後続の `mov [imm32],eax` から bss スロットを解決）
- value の内訳: obj setter 1,312 件 = 文字列 1,088 + 整数即値 224、float setter 992 件 = float 値 992（`fld` ソース `0xA468fc` 等から実読、`fld1`/`fldz` 含む）
- **key の重複 0 件 / null 0 件（2,304 件すべて一意）** → 「同名キーの二重登録」は構造的に不存在

### 2.3 分類別センサス

**(a) V49 パターンをそのまま適用した再カウント（V49 値の完全再現）**

| 分類ラベル | V49 パターン | setter | **再カウント** | **V49 記載** | 一致 |
|---|---|---|---|---|---|
| `f*` float 設定 | `^f` | float | **978** | 978 | ✓ |
| `s*` 文字列設定（その他） | `^s`（他に該当しない） | obj | **970** | 970 | ✓ |
| `i*` int 設定 | `^i` | obj | **227** | 227 | ✓ |
| `*Desc*`（説明文） | `'Desc' in key`（部分一致） | obj | **45** | 45 | ✓ |
| AV 表示名 | `sAttributeName`/`sSkillName`/`sTraitName`/`sVirtueName` prefix | obj | **36** | 36 | ✓ |
| Icon 系 | `^sAttributeIcon` / `^sIcon` / `Icon$` | obj | **17** | 17 | ✓ |
| `sMagicEffectItem*` | `^sMagicEffectItem` | obj | **17** | 17 | ✓ |
| 未分類 | key=null | — | **14** | 14 | ✓ |
| **合計** | | | **2,304** | 2,304 | ✓ |

**(b) V59 精密抽出での正しい帰属と静的テーブル件数**

| 分類 | **V59 抽出** | **静的テーブル** | 差の所在（詳細は §4） |
|---|---|---|---|
| `f*` float 設定 | **992** | — | V49「未分類14」の全件が `f*`（§4.4） |
| `s*` 文字列設定（その他） | **966** | — | V49 の 970 から −4 = AV 名の混入分（§4.2） |
| `i*` int 設定 | **227** | — | 一致 |
| `*Desc*` 説明文 | 表C由来 **33** ＋ 別系統 `s*Description` **12** = 45 | 表C **33** | 33 = 33 で一致。12 は別キー族（§4.1） |
| AV 表示名 | **40** | 表A **40** | 40 = 40 で一致。V49 の 36 は取りこぼし（§4.2） |
| Icon 系 | 表Icon由来 **8** ＋ 別系統 **9** | 表Icon **33**（うち 25 件が V49 パターン未捕捉） | 33 = 33 で一致（§4.3） |
| `sMagicEffectItem*` | **17**（内 prefix5） | — | 一致 |
| 未分類 | **0** | — | 全件解決 |
| **合計** | **2,304** | | 一意・重複 0 |

**V49 値の再現確認**: V49 の実カウントスクリプト `%TEMP%\opencode\v49_step4_census.py` / `v49_step20_census.py`（`classify_key()`）を**本抽出の 2,304 キーにそのまま適用**したところ、V49 記載値 **978 / 970 / 227 / 45 / 36 / 17 / 17 / 未分類14 を一桁のずれもなく完全再現**（`v59_repro_v49.py`）。
→ **V49 の数字は捏造・読み違いではなく、「V49 の分類関数そのもの」に由来する。** 差分の原因はすべて V49 `classify_key()` のパターン定義に閉じる。

### 2.4 V49 `classify_key()` と本分類の差（全18件）

| V49 分類 → V59 分類 | 件数 | 対象キー |
|---|---|---|
| `unknown` → `f*` | 14 | §4.4 の 14 件（push バイト列走査の取りこぼし） |
| `other s*` → `AVname` | 4 | `sDerivedAttributeName{Health,Magicka,Fatigue,Encumbrance}`（§4.2） |
| それ以外 | 0 | — |

V49 `classify_key()` の実コード（抜粋・要旨）:

```python
if k.startswith('sAttributeName') or k.startswith('sSkillName') \
   or k.startswith('sTraitName') or k.startswith('sVirtueName'):   # ← sDerivedAttributeName が欠落
    return 's*Name* (AV display)'
if k.endswith('Desc') or 'Desc' in k:                              # ← 部分一致（Description にも当たる）
    return '*Desc* (table C desc)'
if k.startswith('sAttributeIcon') or k.startswith('sIcon') \
   or k.endswith('Icon'):                                          # ← sDerivedAttributeIcon / sSkillIcon が対象外
    return '*Icon* (table C icon)'
```

---

## 3. 静的テーブルの実読（比較基準）

getter 本体を capstone で実読し、テーブル基底・件数・エントリ実体を確定した。

| 表 | getter | テーブル基底 | 件数 | idx 条件 | null | エントリ解決 |
|---|---|---|---|---|---|---|
| 表A | `FUN_00565CC0` | `0xB12758` | **40** | `idx ≤ 0x27` | 0 | 40/40 が登録済 Setting* |
| 表C（Desc） | `FUN_00565CF0` | `0xB127F8` | **33** | `idx ≤ 0x20` | 0 | 33/33 が登録済 Setting* |
| 表Icon | `FUN_00565D10` | `0xB12880` | **33** | `idx ≤ 0x20` | 0 | 33/33 が登録済 Setting* |
| 表B | `FUN_00565CC0`（第2枝） | `0xB12868 + 4*idx` → 実体 `0xB12908`..`0xB12984` | **32** | `0x27 < idx < 0x48` | 0 | .rdata `char*` 直返（登録マップ不使用） |
| （小アイコン） | `FUN_00565D30` | ジャンプテーブル `0x565D78` | **8** | `idx ≤ 7` | — | bss `0xB3A2BC..0xB3A2F4` を直接参照 |

getter 実コード（要点）:

```
00565CC0: cmp eax,0x27 / ja 0x565cd7 ; mov eax,[eax*4+0xb12758] ; test ; mov eax,[eax] ; ret
00565CD7: cmp eax,0x48 / jge ret0    ; mov eax,[eax*4+0xb12868] ; ret     ← deref なし＝char* 直返
00565CF0: cmp eax,0x20 / ja ret0     ; mov eax,[eax*4+0xb127f8] ; mov eax,[eax] ; ret
00565D10: cmp eax,0x20 / ja ret0     ; mov eax,[eax*4+0xb12880] ; mov eax,[eax] ; ret
00565D30: cmp eax,0x20 / ja ret0 ; cmp eax,7 / ja ret0 ; jmp [eax*4+0x565d78]
           ... 0x565d45: mov eax,[0xb3a2bc] ...（8本）
```

> 補記（V49 の記述訂正）: V49 §5.4 は「表Icon `[0]` = `sHandToHandIcon`」とするが、実測では `0xB12880[0] = 0xB3A27C = sAttributeIconStrength`。**`sHandToHandIcon` は表Icon にスロットを持たない**（§4.3）。

---

## 4. 差分3組の実名リストと原因分類

### 4.1 差① `*Desc*` キー 45 件 vs 表C 33 エントリ（差 12）

**表C 由来 33 件**（thunk `0x9F9B60+0x20i`, i=0..32、obj `0xB3A174`..`0xB3A274`、getter `FUN_00565CF0` の idx0..32 と 1:1）

| idx | key | thunk (call) |
|---|---|---|
| 0–7 | `sAttributeDesc{Strength,Intelligence,Willpower,Agility,Speed,Endurance,Personality,Luck}` | `0x9F9B6F`..`0x9F9C4F` |
| 8–11 | `sDerivedAttributeDesc{Health,Magicka,Fatigue,Encumbrance}` | `0x9F9C6F`..`0x9F9CCF` |
| 12–32 | `sSkillDesc{Armorer,Athletics,Blade,Block,Blunt,HandToHand,HeavyArmor,Alchemy,Alteration,Conjuration,Destruction,Illusion,Mysticism,Restoration,Acrobatics,LightArmor,Marksman,Mercantile,Security,Sneak,Speechcraft}` | `0x9F9CEF`..`0x9F9F6F` |

**表C に無くセンサス対象にのみ存在する 12 件（差の実名）** — すべて **`s<名>Description` 形式の別キー族**

| # | key | value | obj VA | 登録 thunk (call) |
|---|---|---|---|---|
| 1 | `sStrengthDescription` | `Need a gamesetting description` | `0xB38590` | `0x9F0A90` (`0x9F0A9F`) |
| 2 | `sIntellegenceDescription`（原文綴り） | 同上 | `0xB38598` | `0x9F0AB0` (`0x9F0ABF`) |
| 3 | `sWillPowerDescription` | 同上 | `0xB385A0` | `0x9F0AD0` |
| 4 | `sAgilityDescription` | 同上 | `0xB385A8` | `0x9F0AF0` |
| 5 | `sSpeedDescription` | 同上 | `0xB385B0` | `0x9F0B10` |
| 6 | `sEnduranceDescription` | 同上 | `0xB385B8` | `0x9F0B30` |
| 7 | `sPersonalityDescription` | 同上 | `0xB385C0` | `0x9F0B50` |
| 8 | `sLuckDescription` | 同上 | `0xB385C8` | `0x9F0B70` |
| 9 | `sCombatDescription` | 同上 | `0xB385F0` | `0x9F0C10` (`0x9F0C1F`) |
| 10 | `sMagicDescription` | 同上 | `0xB385F8` | `0x9F0C30` (`0x9F0C3F`) |
| 11 | `sStealthDescription` | 同上 | `0xB38600` | `0x9F0C50` (`0x9F0C5F`) |
| 12 | `sRaceDescription` | `Race Description` | `0xB38FB0` | `0x9F68F0` (`0x9F68FF`) |

**構造的証拠**
- 12 件の obj VA は **`.data` のどのテーブルにも 1 件も現れない**（`0xB127F8`/`0xB12880`/`0xB12758`/`0xB12908` を含む `.rdata`/`.data` 全走査で該当ポインタ 0 件）
- 登録 thunk は表C 群（`0x9F9B60+0x20i`）とは**別の初期化関数**（`0x9F0A90` 系 / `0x9F0C10` 系 / `0x9F68F0`）
- 読取元も別: `sCombatDescription`=`0x5D5CBA`、`sMagicDescription`=`0x5D5C57`、`sStealthDescription`=`0x5D5C8D`、`sRaceDescription`=6 箇所（`0x5C8205`/`0x5C8236`/`0x5CA5F2`/`0x5CDB72`/`0x5CDD87`/`0x5CDE19`）。属性8件（#1〜8）は `.text` 上の参照が**初期化 thunk と dtor thunk のみ**（`0xB38590..0xB385C8` を個別 xref 実測）＝**中央マップ lookup 経由**で読む GMST 系

**原因分類: (i) センサスのパターン誤分類（主原因）**
- 根拠: V49 の分類が `k.endswith('Desc') or 'Desc' in k` の**部分一致**。`Description` にも `"Desc"` が含まれるため、別キー族 12 件が「表C 説明文」に合流し 33 + 12 = **45** となった。表C 3系統の prefix（`sAttributeDesc`/`sDerivedAttributeDesc`/`sSkillDesc`）で数えると正確に **33**。
- **(ii) 不成立**: 全 2,304 キーが一意（重複 0）。
- **(iv) 不成立**: 表C 33 エントリは null 0 で全登録済 Setting* に解決。同一系統の取りこぼしが無い。
- **(iii) 併存（事実）**: 12 件は表C thunk 群の外に登録されている。ただし**別キー族であって「表C 系の欠落」ではない**ため、差を生んだ直接原因は (i)。

**訂正値**: 「表C 由来の `*Desc*` キーは **33 件**」で確定。V49 の 45 は「`Desc` を含む全キー」の数としては正しいが、**「表C 説明文」としては誤り（45 − 33 = 12 は別系統 `s*Description`）**。

---

### 4.2 差② AV 表示名キー 36 件 vs 表A 登録 thunk 40 個（差 4）

**表A 40 件の全実名**（thunk `0x9F9660+0x20i`, i=0..39 / getter `FUN_00565CC0` idx0..39 = `0xB12758` と 1:1、登録40 = テーブル40 = **集合完全一致**）

| 区分 | 件数 | key | thunk (call) |
|---|---|---|---|
| 属性 | 8 | `sAttributeName{Strength,Intelligence,Willpower,Agility,Speed,Endurance,Personality,Luck}` | `0x9F966F`..`0x9F974F` |
| 派生属性 | **4** | `sDerivedAttributeName{Health,Magicka,Fatigue,Encumbrance}` | `0x9F976F`..`0x9F97CF` |
| スキル | 21 | `sSkillName{Armorer,Athletics,Blade,Block,Blunt,HandToHand,HeavyArmor,Alchemy,Alteration,Conjuration,Destruction,Illusion,Mysticism,Restoration,Acrobatics,LightArmor,Marksman,Mercantile,Security,Sneak,Speechcraft}` | `0x9F97EF`..`0x9F9A6F` |
| トレイツ | 4 | `sTraitName{Aggression,Confidence,Energy,Responsibility}` | `0x9F9A8F`..`0x9F9AEF` |
| 徳目 | 3 | `sVirtueName{Bounty,Fame,Infamy}` | `0x9F9B0F`..`0x9F9B4F` |
| **計** | **40** | | |

**差 4 の実名（V49 が s* バケットに落とした 4 件）**

| # | key | value | obj VA | thunk (call) | V49 分類 | V59 分類 |
|---|---|---|---|---|---|---|
| 1 | `sDerivedAttributeNameHealth` | `Health`（表示名） | `0xB3A074` | `0x9F9760` (`0x9F976F`) | `other s*` | AV 表示名 |
| 2 | `sDerivedAttributeNameMagicka` | `Magicka` | `0xB3A07C` | `0x9F9780` | `other s*` | AV 表示名 |
| 3 | `sDerivedAttributeNameFatigue` | `Fatigue` | `0xB3A084` | `0x9F97A0` | `other s*` | AV 表示名 |
| 4 | `sDerivedAttributeNameEncumbrance` | `Encumbrance` | `0xB3A08C` | `0x9F97C0` | `other s*` | AV 表示名 |

**証拠（算術も一致）**
- V49 クラシファイアの AV 条件は `sAttributeName` / `sSkillName` / `sTraitName` / `sVirtueName` の 4 prefix のみで **`sDerivedAttributeName` が欠落** → 36。
- 「他方で 4 件が余っている」ことの行き先も一致: V49 の `other s*` = **970**、V59 = **966**（966 + 4 = 970）。差が AV −4 / s* +4 で**完全に相殺**（他の全カテゴリは差 0）。
- 表A getter は `idx ≤ 0x27` で **40 エントリ**、表A thunk は `0x9F9660+0x20i` で **40 個**、登録キーも **40 件**（3 者が集合として一致）。

**原因分類: (i) センサスのパターン誤分類（100% 確定・唯一の原因）**
- **(ii)/(iii)/(iv) はいずれも不成立**: 重複 0、40 件すべてが表A thunk 群内、表A 40 エントリは null 0 で全解決（テーブル側の欠落なし）。

**訂正値**: 「AV 表示名キーは **40 件**」（V49 の 36 は誤り）。getter 表A の idx0..39=40 と一致。

---

### 4.3 差③ Icon キー 17 件 vs 表Icon 33 エントリ（差 16）

**表Icon 33 件の全実名**（thunk `0x9F9F80`..`0x9FA480`、getter `FUN_00565D10` idx0..32 = `0xB12880` と 1:1、null 0）

| 区分 | 件数 | key 例（末尾） | thunk 範囲 (call) |
|---|---|---|---|
| 属性アイコン | 8 | `sAttributeIcon{Strength..Luck}` | `0x9F9F80`..`0x9FA060` (`0x9F9F8F`..`0x9FA06F`) |
| 派生属性アイコン | 4 | `sDerivedAttributeIcon{Health,Magicka,Fatigue,Encumbrance}` | `0x9FA180`..`0x9FA1E0` |
| スキルアイコン | 21 | `sSkillIcon{Armorer..Speechcraft}` | `0x9FA200`..`0x9FA480` |
| **計** | **33** | | |

**集合演算（差 16 の中身）**

| 集合 | 件数 | 内容 |
|---|---|---|
| 表Icon ∩ センサス17 | **8** | `sAttributeIcon{Strength..Luck}` |
| **表Icon のみ（センサス対象外）** | **25** | `sDerivedAttributeIcon*` **4** + `sSkillIcon*` **21** |
| **センサスのみ（表Icon 外）** | **9** | `sAttributeIconSmall*` **8** + `sHandToHandIcon` **1** |
| ネット差 | **16** | 25 − 9 = 16 |

**表Icon のみに存在する 25 件（実名）**: `sDerivedAttributeIcon{Health,Magicka,Fatigue,Encumbrance}`、`sSkillIcon{Armorer,Athletics,Blade,Block,Blunt,HandToHand,HeavyArmor,Alchemy,Alteration,Conjuration,Destruction,Illusion,Mysticism,Restoration,Acrobatics,LightArmor,Marksman,Mercantile,Security,Sneak,Speechcraft}`

**センサス 17 のみに存在する 9 件（実名・表Icon 外の別登録）**

| # | key | value | obj VA | thunk (call) | 読取経路 |
|---|---|---|---|---|---|
| 1–8 | `sAttributeIconSmall{Strength,Intelligence,Willpower,Agility,Speed,Endurance,Personality,Luck}` | `Menus\Level_up\attributes_icons\attributes_icon_<name>.dds` | `0xB3A2BC`..`0xB3A2F4` | `0x9FA080`..`0x9FA160` (`0x9FA08F`..`0x9FA16F`) | **別 getter `FUN_00565D30`**（ジャンプテーブル `0x565D78`、`mov eax,[0xb3a2bc]` 等 8 本） |
| 9 | `sHandToHandIcon` | `Icons\Weapons\HandToHand.dds` | `0xB33D84` | `0x9DC1C0` (`0x9DC1CF`) | **静的テーブルに載らない**（.data 参照 0 件）。コード直接参照 3 箇所: `0x478890` / `0x5A6A57` / `0x5A6D55` |

**原因分類: (i) センサスのパターン誤分類（主原因・両方向）**
- 根拠（表Icon られ 25 件）: V49 の Icon 条件は `startswith('sAttributeIcon') / startswith('sIcon') / endswith('Icon')`。`sDerivedAttributeIconHealth`・`sSkillIconArmorer` はいずれにも該当せず → `other s*` に落下（25 件）。
- 根拠（センサス過大 9 件）: 同条件は `sAttributeIconSmall*`（`startswith('sAttributeIcon')` に一致）と `sHandToHandIcon`（`endswith('Icon')` に一致）を拾うが、**どちらも表Icon 33 には無い**。
- **(iii) 併存（事実・根拠つき）**: 上記 9 件は表Icon thunk 群とは別経路（Small = `0x9FA080` 群だが getter が別 `0x565D30`、HandToHand = 遠隔 thunk `0x9DC1C0` でテーブル非掲載）。
- **(ii) 不成立**: 重複 0。**(iv) 不成立**: 表Icon 33 エントリは null 0 で全登録済（同一系統の欠落なし）。

**訂正値**: 「表Icon 系キーは **33 件**」で確定（V49 の 17 は誤り）。加えて「表Icon に載らない別系統 Icon キーが **9 件** 存在する」ことも確定（Small 8 + HandToHand 1）。「16」は 25 − 9 の**ネット差**であり、片方向の部分集合差ではない。

---

### 4.4 ついでに解消: V49「未分類 14」の実名（すべて `f*`）と原因

V49 の key 取得は「呼出点手前の 0x28 バイトを**生バイト走査**し、`68 xx xx xx xx`（`push imm32`）だけを候補にして、imm が `.rdata` 範囲 `0xA28000..0xB018D2` 内の最後の 1 件を key とする」方式だった。この貪欲走査（`68` を見つけたら **`j += 5`**）が、下記 14 件で**偽の `68` を消費して実の `push <key>` を読み飛ばす**ため key=null になっていた。

**原因（実測・全14件で集合一致）**: V49 の走査が失敗するのは **(A) 並びが `fld1`/`fldz` 形（＝ `mov ecx, imm32` が key push の手前に来る形。float 992 件中 262 件）** かつ **(B) その `mov ecx` の即値＝bss Setting アドレスに `0x68` バイトを含む** 場合に限られる。条件 (A)∩(B) の集合が**実測 14 件と完全一致**（`{ordering A} ∩ {obj VA に 0x68} = 14 = 未分類14`）。

理由: 窓 `call−0x28` 内で `mov ecx, <addr>` の即値が index 31〜34 にあり、そこに `68`（`0x00B35668` → LE `68 56 b3 00`、`0x00B36810` → LE `10 68 b3 00`）があると、V49 の貪欲走査がそれを `push` と誤認して `j += 5` し、index 35 に本物がある `push <key>` を飛ばして窓終端（`j < 36`）に到達して終了する。生成される偽 imm（例 `0x6800B356`）は `.rdata` 外なのでフィルタで脱落し key=null。並び B（`push key` が `mov ecx` の手前、730 件）では本物の push が先に来るため影響しない。

| # | key | site | bss obj VA | 既定値 | V49 走査が拾った偽 imm |
|---|---|---|---|---|---|
| 1 | `fAIDefaultMaximumRangeMult` | `0x9E0BA0` | `0xB35668` | 1.0f | `0x6800B356` |
| 2 | `fFurnitureMarker03DeltaY` | `0x9E13F0` | `0xB35868` | 0.0f | `0x6800B358` |
| 3 | `fFurnitureMarker14DeltaX` | `0x9E19F0` | `0xB35968` | 0.0f | `0x6800B359` |
| 4 | `fFurnitureMarker12HeadingDelta` | `0x9E1FF0` | `0xB35A68` | 0.0f | `0x6800B35A` |
| 5 | `fDispTargetRaceMult` | `0x9E6A00` | `0xB36810` | 1.0f | `0x546800B3` |
| 6 | `fDispTargetFactionMult` | `0x9E6A30` | `0xB36818` | 1.0f | `0x686800B3` |
| 7 | `fDispTargetFactionRankBase` | `0x9E6A60` | `0xB36820` | 1.0f | `0x806800B3` |
| 8 | `fFightAggrMult` | `0x9E6BB0` | `0xB36858` | 1.0f | `0x1C6800B3` |
| 9 | `fFightDistanceBase` | `0x9E6BE0` | `0xB36860` | 1.0f | `0x2C6800B3` |
| 10 | `fFightFriendDispMult` | `0x9E6C70` | `0xB36878` | 1.0f | `0x746800B3` |
| 11 | `fAiAquireStealBase` | `0x9E6DA0` | `0xB368B0` | 0.0f | `0x1C6800B3` |
| 12 | `fAiAquirePickBase` | `0x9E6E00` | `0xB368C0` | 0.0f | `0x446800B3` |
| 13 | `fAIYieldDurationBase` | `0x9E88A0` | `0xB36D68` | 1.0f | `0x6800B36D` |
| 14 | `fShockCoreColorB` | `0x9EF820` | `0xB38168` | 1.0f | `0x6800B381` |

（既定値 = 抽出した float value。偽 imm は V49 の `extract_pushes()` を再実行して実測。本物の key push は各 site の 5 バイト手前にあり、V59 の capstone 抽出では全件解決。）

**訂正値**: `f*` float 設定は **992 件**（978 + 14）。V49 の「未分類14」は**解消**（V57 §6-14・§7-6 の未検証点に対し、キー一覧を確定）。

---

## 5. 原因分類サマリ（(i)〜(iv) の判定）

| 差分 | 主原因 | 根拠 | (ii) 二重登録 | (iii) テーブル外別経路 | (iv) テーブル側が一部のみ |
|---|---|---|---|---|---|
| ① Desc 45 vs 33（差12） | **(i)** | V49 クラシファイアが `'Desc' in k` 部分一致 → `s*Description` 12件が合流。prefix一致なら 33 | 不成立（重複0） | **併存**（12件は `0x9F0A90`/`0x9F0C10`/`0x9F68F0` 群に登録・表C非掲載） | 不成立（表C 33/33 解決） |
| ② AV名 36 vs 40（差4） | **(i)** | V49 クラシファイアに `sDerivedAttributeName` prefix 欠落。s* の +4 と完全相殺 | 不成立 | 不成立（40件すべて表A thunk 群内） | 不成立（表A 40/40 解決） |
| ③ Icon 17 vs 33（差16） | **(i)**（両方向） | 同クラシファイアが `sDerivedAttributeIcon`(4)/`sSkillIcon`(21) を逃し、`sAttributeIconSmall`(8)/`sHandToHandIcon`(1) を過大包含 → 25 − 9 = 16 | 不成立 | **併存**（9件は別 getter/別 thunk・表Icon非掲載） | 不成立（表Icon 33/33 解決） |
| （付随）未分類 14 | **(i)**（抽出方式の癖） | V49 の生バイト `68` 走査が、並びA（`fld1`/`fldz` 形）× bss obj VA に `0x68` バイトの 14 件で偽 `68` を消費し実 key push をスキップ（条件 (A)∩(B) の集合が 14 件と一致）。全件 `f*` | — | — | — |

- **(ii) 同名キーの二重登録は 3 差分すべてで不成立**（2,304 キー全一意。setter の重複警告 `0xA32490` が発火する挙動も、静的登録では起きない設計と整合）。
- **(iv) テーブル側が一部のみは 3 差分すべてで不成立**（表A 40/40・表C 33/33・表Icon 33/33 が null 0 で全登録済 Setting* に解決）。
- (iii) は①③で**事実として併存**するが、それは「別キー族が別経路に登録されている」ことであって、差分の**発生原因**はいずれも (i) のセンサスパターン側に閉じる。

---

## 6. 訂正値サマリ

| 項目 | V49 値 | **訂正値** | 検証方法 | 状態 |
|---|---|---|---|---|
| 表C 由来 `*Desc*` キー | 45 | **33** | 表C `0xB127F8` 33エントリと登録キー集合の完全一致（33/33、null0）。別系統 12 件を実名特定 | **確定** |
| AV 表示名キー | 36 | **40** | 表A thunk 40 = 表A テーブル 40 = 登録キー 40 の 3 者一致。差 4 = `sDerivedAttributeName*` を実名特定 | **確定** |
| 表Icon 系キー | 17 | **33** | 表Icon `0xB12880` 33エントリと登録キー集合の完全一致（33/33、null0） | **確定** |
| （付随）表Icon 外の別系統 Icon キー | 記載なし | **9**（Small 8 + `sHandToHandIcon` 1） | 専用 getter `0x565D30` 8件 / .data 非掲載＋コード3参照 1件 | **確定** |
| （付随）`f*` float 設定 | 978 + 未分類14 | **992** | 14 件の実名を全取得 | **確定** |
| （付随）`i*` / `sMagicEffectItem*` | 227 / 17 | 227 / 17 | 再現一致 | **確定（変更なし）** |
| 「差16」の意味 | 未検証 | **25（表Iconのみ）− 9（センサスのみ）のネット差** | 集合演算 | **確定** |

---

## 7. 未解決点（推測で埋めなかった事項）

1. **`sHandToHandIcon` の読み元 3 箇所（`0x478890` / `0x5A6A57` / `0x5A6D55`）の UI 文脈** — 静的テーブル非掲載・コード直接参照であることは確定（`.data` に該当ポインタ 0 件）だが、各参照がどのメニュー/処理かは本タスクの範囲外で未調査。
2. **`s*Description` 属性8件（`sStrengthDescription` 等）の実行時読取経路** — `.text` 上のオブジェクト直接参照は初期化 thunk と dtor thunk のみで、中央マップ `0xB35574` の lookup（key 経由）で読まれるとの推定が最も整合するが、**lookup 呼び出し元を 1 件ずつ突き合わせた証拠は未取得**。値 `Need a gamesetting description` がプレースホルダである点も未確認（INI/ESM で上書きされる想定）。
3. **表B（`0xB12908`..`0xB12984`、32件）の .rdata 文字列実体と AV 名 40+32=72 の対応** — 「.rdata `char*` 直返・登録マップ不使用」であること（getter `0x565CD7` 枝に deref 無し）は確定。ただしその 32 文字列を 1 件ずつ列挙・AV idx 対応表化は本タスク対象外。
4. **`sAttributeIconSmall*` 8 件が「表Icon」と「別テーブル」どちらに数えるべきか** — 機構としては表Icon（`0x565D10`）と別 getter（`0x565D30`）であることが確定したが、仕様上のカテゴリ名（V49 表記の「表Icon」に含めるか否か）は文書側の方針次第。件数事実は 33 / 8 の分離で確定。
5. **V49 §5.4 の「表Icon `[0]` = `sHandToHandIcon`」記述** — 実測と矛盾（実際は `sAttributeIconStrength`）。本ファイル §3 で訂正したが、既存 `v49_registry_type.md` は本タスクの方針により**編集していない**（既存ドキュメント編集禁止のため）。

---

## 8. 使用スクリプト（すべて `%TEMP%\opencode\`）

| ファイル | 用途 |
|---|---|
| `v59_common.py` | PE 自前解析（section / va2off / rd / cstr / u32） |
| `v59_st.py` | セルフテスト 4 件 |
| `v59_scan1..5.py` | 呼出点走査・CRT 初期化配列の列挙・被覆確認 |
| `v59_dis.py` | capstone ディスアセンブラ補助 |
| `v59_extract.py` | **2,304 件の (key, value, obj VA) 機械抽出** → `v59_regs.json` |
| `v59_class.py` | 分類別件数・重複検査 |
| `v59_tables.py` | 表A / 表C / 表Icon / 表B の実読と obj→key 解決 |
| `v59_repro_v49.py` | **V49 `classify_key()` の再実行（V49 値の完全再現）と差分18件の特定** |
| `v59_final_data.py` | thunk 群ラン・集合演算・テーブル整合・CRT 統計 |
| `v59_xref.py` | 差分キー obj VA の全走査（.text/.rdata/.data 参照） |

*本ファイルは V59 の新規成果物。既存ドキュメントは無編集。*
