# V31: esm_reader.cpp MEDT フィルタ重複読取（デッドコード）解析

日付: 2026-10-24
対象: `app/src/main/cpp/assets/esm_reader.cpp`（HEAD `17c3de23` 時点、5,088 行）
ステータス: **調査完了・パッチ未適用**（本ファイルは仕様書のみ。リポジトリ内コードは無変更）

---

## 1. 結論（TL;DR）

`decodeMagicEffect()`（esm_reader.cpp:1583-1610）が `findSubRecord("MEDT")` を
先に試し、無ければ `findSubRecord("DATA")` にフォールバックする。
**実 ESM（Oblivion.esm + 全 DLC .esp）には MEDT サブレコードが1件も存在しない**こと
を全件走査で実測した。よって `findSubRecord("MEDT")`（1593 行目）は
**全実データで常に `nullptr` を返すデッドコード**であり、
`if (!medt)`（1594 行目）は**常に真**。修正は「MEDT 探索行の純削除」で
挙動変化ゼロ。MGEF のデータサブレコードの正体は **DATA（36B or 64B）**。

---

## 2. MEDT の全出現箇所（HEAD 実測）

`grep -i medt` で esm_reader.cpp 内の MEDT リテラルは **1 箇所のみ**。
（TODO 背景の「2 箇所」は 1593/1594 行の
**同一データに対する2段 findSubRecord = 重複読取**を指す。詳細は §6。）

`esm_reader.cpp:1583-1610`（引用・行番号つき）:

```cpp
1583: void ESMFile::decodeMagicEffect(const ESMRecord& rec) {
1584:     MagicEffectData effect;
1585:     effect.formID = rec.formID;
1586:     effect.editorID = rec.getString("EDID");
1587:     effect.fullName = rec.getString("FULL");
1588:     effect.description = rec.getString("DESC");
1589:
1590:     // DATA subrecord: magic effect data (36 or 64 bytes)
1591:     // struct { uint32_t unknown0; float baseCost; uint32_t actorValue/link;
1592:     //          uint32_t school; uint32_t flags; ... }
1593:     auto* medt = rec.findSubRecord("MEDT");          // <-- デッドコード
1594:     if (!medt) medt = rec.findSubRecord("DATA");     // <-- 常に実行される
1595:     if (medt && medt->size() >= 20) {
1596:         effect.baseCost = readFloatOrInt(medt->data.data() + 4);
1597:         uint32_t av = readU32(medt->data.data() + 8);
1598:         // Conjuration/restoration effects store a formID (summoned creature,
1599:         // bound item) in this slot instead of an actor value.
1600:         if (av <= 200) {
1601:             effect.actorValue = av;
1602:         } else {
1603:             effect.linkedFormID = av;
1604:         }
1605:         effect.school = readU32(medt->data.data() + 12);
1606:         effect.flags = readU32(medt->data.data() + 16);
1607:     }
1608:
1609:     m_magicEffects.push_back(std::move(effect));
1610: }
```

呼び出し元（デコード分岐）: `decodeRecord()` の `esm_reader.cpp:766-767`:

```cpp
766:         } else if (std::memcmp(rec.recType, "MGEF", 4) == 0) {
767:             decodeMagicEffect(rec);
```

`findSubRecord` 本体（esm_reader.cpp:90-97）は単純な線形走査:

```cpp
90: const SubRecord* ESMRecord::findSubRecord(const char* tag) const {
91:     for (const auto& sub : subRecords) {
92:         if (std::memcmp(sub.tag, tag, 4) == 0) {
93:             return &sub;
94:         }
95:     }
96:     return nullptr;
97: }
```

ファイル読み込みパス（`ESMFile::open` → 索引 → `decodeRecord`）と
メモリ読み込みパス（`parseFromMemory` esm_reader.cpp:3263）の両方が
`decodeRecord` を経由するので、MGEF はどちらの経路でも必ず
1593-1594 行を通る。

---

## 3. デッドコード判定の根拠（制御フロー）

1. **`findSubRecord("MEDT")` は実データで常に nullptr**（§4 の実測:
   Oblivion.esm 全 1,167,016 レコード・63 種・サブレコード全走査で
   MEDT は 0 件、MGEF 145 件すべてが DATA を持ち MEDT を持たない）。
2. したがって 1593 行の呼び出し結果は常に棄却され、
   1594 行の `findSubRecord("DATA")` が**常に**実行される。
   コンパイラ到達性の議論: 両行とも到達可能（呼び出しは副作用
   `subRecords` 走査を持つ）だが、**1593 行の戦果は決して使われない**
   — 「結果が常に棄却される読み取り」= クリティカルでないデッド。
3. 1595 行以降の変数 `medt` が受け取るのは 1594 行の DATA 結果のみ。
   後続処理（baseCost/actorValue/school/flags の読み取り）は
   DATA レイアウト（§4）と整合する。
4. 副作用コスト: MGEF ごとにサブレコード線形走査が2回（MEDT 用1回は
   必ず全走査して miss）。145 レコードのみで僅少だが、完全に無駄。

### 履歴（デッドコードの由来）

- `7ba33dd8`（初版）: `findSubRecord("MEDT")` **のみ**で、
  DATA フォールバックなし。当時のオフセットも別物
  （school@0, baseCost@4, flags@8, magnitude@12, duration@16,
  range@20, actorValue@24）。**MEDT が常に nullptr なのでこの版では
  MGEF データが一切デコードされていなかった**。
- `012a5bb4`: 実データに合わせ「DATA フォールバック追加＋
  オフセット修正（baseCost@4, actorValue@8, school@12, flags@16）」。
  ただし MEDT 探索行は「念のため」残置 → これが現在のデッドコード。
- `012a5bb4` 以降 HEAD まで未修正。

---

## 4. 実 ESM 測定結果（実測・再現可）

### 4.1 構造化全件走査

スクリプト: `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\v31_medt_scan.py`
対象: `C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Data\Oblivion.esm`（277,504,985 B）
方式: レコードヘッダ 20B / GRUP ヘッダ 20B / サブレコード 6B（tag4+u16）。
圧縮レコード（flag `0x40000`）は 4B 伸長サイズ + zlib 展開後にサブレコード走査。

結果（実出力）:

```
file size: 277504985
after TES4: 764
records: 1167016   compressed: 41789   errors: 0   distinct record types: 63
(＋TES4 ヘッダ1件 = 1,167,017 レコード / 64 種。既知 ground truth と一致)

=== MGEF ===
MGEF records: 145   with MEDT: 0
MGEF DATA size distribution: {64: 144, 36: 1}
MGEF subrecord tag census:
  EDID 145, FULL 145, DESC 145, DATA 145, MODL 141, MODB 141, ICON 137, ESCE 41

=== MEDT occurrences by record type ===
  NONE - no MEDT subrecord anywhere in Oblivion.esm

compressed by type (top): LAND 31083, PGRD 8224, NPC_ 2482
```

### 4.2 生バイトスキャン（全プラグイン）

スクリプト: `...\Temp\opencode\v31_medt_rawscan.py`
`Data\` 直下の全11プラグイン（Oblivion.esm + DLC/Knights .esp）に
`b"MEDT"` を生検索:

- 全 .esp: **0 ヒット**
- Oblivion.esm: **1 ヒット** @`0xE2CD56A`

1ヒットの分類（`...\Temp\opencode\v31_medt_hit_classify.py`）:
所有レコードは **LAND**（formID `0x3A505`、レコードヘッダ @237,819,203、
data 237,819,223–237,828,197、**圧縮**）。ヒットは data+19 = zlib
ストリーム内部。周辺バイト `...78 9C 9D 99 07 58 14 57 FB F6 9F D9 5D 76
E9 4D 45 44 54 50 EC BD 83...`（`78 9C` は zlib ヘッダ直後）。
**サブレコード境界ではなく圧縮負荷内の偶然の ASCII 並び**。

### 4.3 MEDT の正体（実測確定・憶測なし）

- **Oblivion 形式（TES4）に MEDT サブレコードは存在しない。**
  MEDT を持つレコードはどの型（MGEF 含む）にも0件。
- MGEF の Magic Effect Default Data は **DATA**（36B または 64B）。
  レイアウト（移植側 decodeMagicEffect と一致、実測 145 件全適合）:
  - `+0`  unknown0 (u32)
  - `+4`  baseCost (f32)
  - `+8`  actorValue / linkedFormID (u32) — `av <= 200` 判定で分岐
  - `+12` school (u32, 0-5)
  - `+16` flags (u32)
- MEDT タグの出自はコード履歴（7ba33dd8）で**推測導入**されたもの。
  実在フォーマットのどこにも無いタグであり、他ゲーム由来の混同か
  純粋な誤りかは本調査では（意図的に）断定しない。

---

## 5. 修正パッチ仕様（未適用）

### 変更対象: `app/src/main/cpp/assets/esm_reader.cpp`

**Before**（1590-1595 行）:

```cpp
    // DATA subrecord: magic effect data (36 or 64 bytes)
    // struct { uint32_t unknown0; float baseCost; uint32_t actorValue/link;
    //          uint32_t school; uint32_t flags; ... }
    auto* medt = rec.findSubRecord("MEDT");
    if (!medt) medt = rec.findSubRecord("DATA");
    if (medt && medt->size() >= 20) {
```

**After**（1590-1594 行へ1行削減）:

```cpp
    // DATA subrecord: magic effect data (36 or 64 bytes)
    // struct { uint32_t unknown0; float baseCost; uint32_t actorValue/link;
    //          uint32_t school; uint32_t flags; ... }
    // MEDT does not exist in TES4 records (measured 0/1,167,017 in
    // Oblivion.esm); the magic effect data subrecord is DATA.
    auto* medt = rec.findSubRecord("DATA");
    if (medt && medt->size() >= 20) {
```

- 削除行: 1593（MEDT 探索）+ 1594（フォールバック分岐）→ 1 行に集約。
- 変数名 `medt` は本質 `data` だが、呼び出し範囲が 1595-1606 行の
  局所7箇所のみ。**最小差分のため `medt` のまま残す**（推奨）。
  リネーム（`data`）まで行う場合も同じブロック内置換のみで安全。
- 1590-1592 の既存コメントは DATA 正しいまま維持。新コメントは
  「MEDT が存在しない」実測根拠を1-2行で残す（上記 After の案）。
- `esm_reader.h` は無変更（`MagicEffectData` 構造・宣言に MEDT への
  依存なし。`baseMagnitude`/`baseDuration`/`range` フィールドは
  7ba33dd8 期の MEDT レイアウト用で現行コードでは未使用だが、
  本 V31 のスコープ外 = 別タスクで判断）。

### 削除で安全か

**はい（純削除＋分岐簡素化）。** 理由:
- 1593 行の結果は全実データで棄却される（§3）。
- 1594 行以降の DATA 読取結果が唯一の使用経路であり、変化しない。
- 読取結果の使用先: `m_magicEffects` → `resolveMagicReferences`
  （esm_reader.cpp:3429、EFID コード解決と school 付与）、
  `ESMManager` の `m_magicEffectByEditorID`（3942/4095/4247）、
  `game/spell_manager.cpp:62`（`mgef->school == 1 && linkedFormID`）。
  どれも DATA 由来の値のみ参照しており MEDT 廃止の影響なし。

---

## 6. TODO 背景の「2 箇所」について

HEAD の MEDT リテラルは **1 箇所（1593 行）**。
「読み取りが2箇所」の正体は、同一 MGEF レコードの同一論理データに対する
**2段階 findSubRecord（1593: MEDT → 1594: DATA）**であり、
先の段（MEDT）が常に失敗する「重複読取」。過去の解析が見た「2箇所」は
これと解するのが実コードと整合する。別の2箇所目に相当する
`findSubRecord("MEDT")` 呼び出しは esm_reader.cpp にも
`app/src/main/cpp/` 全体にも存在しない（`dialogue_record.cpp` の
DATA 読取等は別文脈）。

---

## 7. 検証コマンド案

リポジトリ実態確認済み: ホストテストは `esm_reader.cpp` を
ビルド対象に含む（`tools/host_tests/run_host_tests.sh:116`）。
MGEF 特化テストは現存しないが、`watr_decode_tests` / `xtel_decode_tests` が
`parseFromMemory` + `decodeRecord` の分岐全体（MGEF 分岐を含む
if-else 鎖）をコンパイル・実行し、リグレッションを保証する。

適用後（ユーザー/別セッション）に実行:

```powershell
# ホストテスト（MSVC・ASan なし。2026-10-23 に exit-code 修正済み・green 実績あり）
powershell -File tools\host_tests\run_host_tests_msvc.ps1 -NoAsan
# 期待: "Host tests passed." / exit 0（全スイート含む WatrDecodeTests /
#        XtelDecodeTests が合成 ESM で parseFromMemory を通す）
```

```sh
# CI/g++ パス（同等）
bash tools/host_tests/run_host_tests.sh
```

```powershell
# MEDT 非存在の実測再確認（本調査のスクリプト・変更不要）
python "$env:TEMP\opencode\v31_medt_scan.py"
# 期待: "MEDT occurrences by record type: NONE" / MGEF with MEDT: 0 /
#        MGEF DATA size distribution: {64: 144, 36: 1}
```

追加検証（任意・将来）: `watr_decode_tests.cpp` の `buildEsm()` パターンを
踏襲し、MGEF レコード1件（EDID/FULL/DESC + DATA 20B+）を含む合成 ESM で
`getMagicEffects()[0].baseCost/school/flags/actorValue` を
アサートするテストを `app/src/main/cpp/tests/` に追加すると
本修正の回帰を直接検出できる（現時点では未着手）。

---

## 8. リスク評価

| 観点 | 評価 |
|------|------|
| 挙動変化 | **なし（純デッドコード削除）**。MEDT 探索は常に nullptr を返し、DATA 読取結果が常に使われていた。 |
| 機能追加 | なし。新規データ解釈・フィールド追加を含まない。 |
| 性能 | MGEF あたりサブレコード線形走査が1回減る（145件のみ・僅少）。劣化なし。 |
| 互換性 | TES4 仕様外の MEDT を含む仮想的な非標準プラグインがあった場合、旧コードは MEDT 側を優先して読むが、それは仕様外データでむしろ危険。実測で市販全プラグインに該当なし。 |
| テスト影響 | 既存ホストテストに MGEF ケースなし。ビルド・分岐整合は Watr/Xtel スイートで担保（§7）。 |
| ファイル範囲 | esm_reader.cpp の 2→1 行変更のみ。esm_reader.h 無変更。 |

**総合: 低リスク。適用推奨。**

---

## 9. 成果物・再現リソース

- 本ファイル: `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v31_medt_deadcode.md`
- スクリプト（%TEMP%、リポジトリ外）:
  - `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\v31_medt_scan.py` — 構造化全件走査
  - `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\v31_medt_rawscan.py` — 全プラグイン生バイト検索
  - `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\v31_medt_hit_classify.py` — 1ヒットの所有レコード分類
- パッチは**未適用**（本ドキュメント §5 が適用仕様）。
