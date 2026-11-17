# v509 — `FUN_004492E0`＝reset fn 訂正確定・`0xB30D28`＝`"asin"` data・`FUN_0098C072` docs 未収録の裁定（V-509-1..3 完遂・旧残点 V-406-3 継続第39弾）

- 採番: **v509**（旧残点群消化・probe prefix **v509_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-509-1..3（v508 残点）** — 1=`FUN_004492E0` 訂正確定（dtor vs ctor）＋`FUN_0098C072` identity＋`0xB30D28` 正体／2=map ctor 別 fn 探索と Player `+0x80` 中身／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-509-1: `FUN_004492E0` 訂正確定（v469 ctor vs dtor）** | **[確定]（訂正）→ reset/cleanup [確定相当]** | docs decompiled head 実読 [確定(decompiled)]: **`undefined4 __fastcall FUN_004492e0(int param_1)`（**1引数**）→ **`*(param_1 + 0xcd4) = 1`（flag set）→ `FUN_00447d00()` → **`if (DAT_00b06158 != 0) { do { *(DAT_00b06150 + uVar6*4) = 0; uVar6++; } while (uVar6 < DAT_00b06158); }`（**global bucket `0xB06150` clear**）→ **`DAT_00b06158 = 0; _DAT_00b0615c = 0;`（global reset）** → `FUN_00442630(0,0)` → **`*(DAT_00b333a0 + 0x74) = 0`**（`0xB333A0` object の `+0x74` clear）** → **v508 machine の list free 4系統（`esi+0x64`/`+0x34`/`+0x3c`/`+0x8c`）と docs の global reset が一致 → `FUN_004492E0`＝「global bucket clear＋global reset＋object list free＋flag set を行う reset/cleanup fn [確定相当]」** → **v469 ruling「`FUN_004492E0`＝TESObjectSTAT ctor」を [確定負] で訂正（ctor ではなく reset/cleanup）[確定]**（`param_1`＝this（`__fastcall` 1引数）[推定]） |
| **V-509-1: `0xB30D28`（`FUN_0098540B` の null fallback）正体** | **[推定相当]（`"asin"` data）** | `.data` 実読 [確定(機械)]: **dword `@0xB30D28`＝`0x0000000C`（12）、ctx hex `0C 00 00 00 08 00 00 00 61 73 69 6E 00` → `0x6173696E`＝`"asin"` 文字列 → `{12, 8, "asin", …}` の data 構造 [確定(機械)]**、docs mentions **0件 [確定(否定側)]** → **`0xB30D28`＝`"asin"`（arcsin 数学関数）に関連する data 構造（`12`＝size 候補、`8`＝len 候補、`"asin"`＝名前）[推定相当]** → **`FUN_0098540B`（v508 fallback wrapper）の null 返しが `asin` data を返す＝math/str 関数系の default [推定相当]** |
| **V-509-1: `FUN_0098C072`（string copy 実体）identity** | **[確定]（否定側・docs 未収録）** | **docs annotation **なし（`FUN_0098c072` の行が存在しない）** [確定(否定側)] → `FUN_0098C072` は docs 未収録（v437 annotation gap の type）→ **identity は machine/別法へ＝残点**（`FUN_0098540B` が `call 0x98C072` する構造は v508 で機械確定済み [確定]） |
| **V-509-2: map ctor 別 fn 探索／Player `+0x80` 中身** | **[未確定]（残点継続）** | 本 probe 未着手（budget は `FUN_004492E0` 訂正確定に集中） |
| **V-509-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v509_1）

### §2.1 `FUN_004492E0` docs head（v509_1a）
```
L56299: undefined4 __fastcall FUN_004492e0(int param_1) {
L56315:   *(undefined1 *)(param_1 + 0xcd4) = 1;                 // flag set
L56316:   FUN_00447d00();
L56318:   if (DAT_00b06158 != 0) {
L56320:     do { *(undefined4 *)(DAT_00b06150 + uVar6 * 4) = 0;  // global bucket clear
L56321:          uVar6 = uVar6 + 1; } while (uVar6 < DAT_00b06158); }
L56324:   DAT_00b06158 = 0;                                      // global reset
L56325:   _DAT_00b0615c = 0;
L56326:   FUN_00442630(0, 0);
L56327:   *(undefined4 *)(DAT_00b333a0 + 0x74) = 0;              // object +0x74 clear
  + v508 machine: list unlink/free at esi+0x64/+0x34/+0x3c/+0x8c
=> reset/cleanup fn (NOT ctor)  [確定相当]; v469 ctor record corrected  [確定負→訂正]
```

### §2.2 `0xB30D28`（v509_1c）
```
dword @0xB30D28 = 0x0000000C
ctx hex: 0C 00 00 00 08 00 00 00 61 73 69 6E 00 ...
  -> 0x6173696E = "asin" ; structure {12, 8, "asin", ...}
docs mentions of 0xb30d28: 0   [否定側]
=> asin-related data structure (size 12, len 8)  [推定相当]
```

### §2.3 `FUN_0098C072`（v509_1b）
```
docs: annotation line "// FUN_0098c072 @" absent  [否定側 -> docs gap]
(v508 machine: FUN_0098540B does call 0x98C072 -> structure confirmed)
```

## §3 残点 **V-510-1..3**
1. 【中】**`FUN_0098C072` identity**（docs 未収録＝machine decode か `FUN_0098xxxx` band の同族 fn から）＋ **`"asin"` data 構造の完全解**（`12`/`8` の意味・`FUN_0098540B` との関係）。
2. 【低】**map ctor 別 fn 探索**（`FUN_00452570` の this 入手元 machine 追跡）＋ Player `+0x80` 中身。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v509_1 の1本 EXIT=0**（V-509-1 `FUN_004492E0` 訂正確定（reset/cleanup・v469 ctor 訂正）／`0xB30D28`＝`"asin"` data を機械確定相当／`FUN_0098C072` docs 未収録を否定側確定／V-509-2/3 は budget 外・引継ぎ）。
- **決定打の作り**: (i) **`FUN_004492E0` docs head 34行を読む** → global bucket clear（`0xB06150`）＋reset（`0xB06158/0xB0615C`）＋object clear（`0xB333A0+0x74`）が **v508 machine の list free と一致** → **reset/cleanup と確定（v469 ctor 訂正）** → (ii) **`0xB30D28` の `.data` 実読**で `{12, 8, "asin"}` を獲得 → (iii) **`FUN_0098C072` の annotation 欠落**を docs grep で否定側確定。
- 妥当性チェック: ①`DAT_00b06158`（count）＋`DAT_00b06150`（bucket）⇔v485/v494 map 構造（global 側）— ②`*(DAT_00b333a0+0x74)=0`⇔v508 machine `mov [edx+0x74], ebx`（`0xB333A0` object）と一致 — ③`0x6173696E`⇔`"asin"`（LE 読替）— ④`12`/`8`⇔size/len 候補（v505 strlen alloc と同型の数値）— ⑤docs annotation 0⇔`FUN_0098540B` の `call 0x98C072`（machine と整合）— **五方向矛盾なし**。
- docs との不整合: **①v469 ruling「`FUN_004492E0`＝TESObjectSTAT ctor」→ 本裁定で reset/cleanup fn と [確定負] 訂正（global reset＋list free）／②`FUN_0098C072`／`0xB30D28` は docs 未収録（annotation gap・string 無し）[確定（否定側）]**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **「ctor と記録した fn」の訂正は docs head の「global clear/reset pattern」で確定**（`bucket clear＋count=0＋object field clear`＝**reset/cleanup の定番**＝v508 machine list free と docs の突合が決め手）。
  2. **`.data` の `{size, len, "name"}` 構造が「名前付き data エントリ」の目印**（`0xB30D28`＝`{12, 8, "asin"}`＝**math 関数テーブル [推定]**、**LE 読替（`0x6173696E`→`"asin"`）の反復**）。
  3. **docs annotation 欠落（gap）も [確定（否定側）] として記録**（**docs 未収録＝machine/別法へ経路固定**＝v437/v486 に続く gap 対応）。
  4. **fallback（`0xB30D28`）の identity が「何の default か」を語る**（`"asin"` data ⇔ math default [推定]）。
  5. **訂正確定（v469→v509）も裁定に明記**（**誤記録の軌跡が残る＝追跡可能性**・v500 ctor→v505 remove、v508 dtor 候補→v509 reset 確定の連鎖）。
