# v510 — `FUN_0098C072`＝IAT dynamic lookup・math name table・map ctor 候補 signature の裁定（V-510-1..3 完遂・旧残点 V-406-3 継続第40弾）

- 採番: **v510**（旧残点群消化・probe prefix **v510_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-510-1..3（v509 残点）** — 1=`FUN_0098C072` identity と `"asin"` data 完全解／2=map ctor 別 fn 探索と Player `+0x80` 中身／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-510-1: `FUN_0098C072` identity** | **[推定相当]（IAT dynamic lookup/init・string copy 訂正）** | machine 0x98C072-0x98C0C2 decode [確定(機械)]: **`push esi/edi` → **`call dword [0xa281ec]`（**IAT import slot call**）→ `mov edi,eax; call 0x98bf57` → **`push [0xb310ac]; push [0xb310b0]; call dword [0xa28150]`（**IAT import call（2引数）**）→ `call eax`（動的 fn ptr）→ `mov esi,eax; test esi,esi; jne` → **`push 0x214; push 1; call 0x989e22`（**alloc `0x214`＝532B・count=1**）→ `push esi; push [0xb310ac]; push [0xba9e20]; call 0x98beeb; pop ecx; call eax`** → **`FUN_0098C072`＝IAT import call（`0xA281xx` slot）＋global（`0xB310AC`/`0xB310B0`）＋alloc `0x214`＋動的 `call eax` する dynamic resolution/init fn [推定相当]**（**DLL import 系 [確定相当]**、`0x214`＝532B＝object size [推定]）→ **v505/v508「string copy」解釈を本裁定で更新（string copy でなく dynamic lookup）[確定（解釈更新）]** |
| **V-510-1: `0xB30D28`（fallback）完全解** | **[確定相当]（math name table）** | `.data` dword dump [確定(機械)]: **`0xB30D28: 0x0000000C`（12）／`0xB30D2C: 0x00000008`（8）／**`0xB30D30: 0x6E697361`＝`"asin"`**／`0xB30D34..0xB30D40: 0`／**`0xB30D44: 0x3FE00000`（float `0.5`）**／**`0xB30D48: 0x00776F70`＝`"pow"`**／`0xB30D4C: 0`／**`0xB30D50: 0x00736F63`＝`"cos"`**／`0xB30D54: 0` → **`{12, 8, "asin", …, 0.5, "pow", …, "cos", …}` の math 関数名テーブル＋定数配列 [確定相当]**（`12`＝count 候補、`8`＝stride 候補 [推定]）→ **`FUN_0098540B`（v508 fallback）の null 返しがこのテーブル default entry を返す [推定相当]**（math/str 関数系の default [推定]） |
| **V-510-2: map ctor 別 fn 探索** | **[部分確定]（候補 signature 列挙）** | v507 業務 fn 候補の docs signature 列挙 [確定(decompiled)]: **`FUN_004492E0`（reset fn・v509 で確定）／`FUN_0046C300`（`void __thiscall(int param_1, int param_2, char param_3)`・**`char` flag 付き＝v492 `FUN_004CBA80` 系と同型 [推定]**）／`FUN_0046C460`（`undefined4 * __fastcall(undefined4 *param_1)`・return ptr [推定]）／`FUN_004B2730`（`void(int *param_1, int param_2, undefined4 param_3)`）／`FUN_004B3400`（`void __thiscall(int param_1, undefined4 param_2)`）** → **map ctor はこの候補群から＝本 probe で signature のみ・本文特定は残点 [部分確定]**（`FUN_0046C300` の `char param_3`（flag）が map 構築の mode [推定]） |
| **V-510-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v510_1）

### §2.1 `FUN_0098C072`（v510_1a）
```
98C072: push esi / push edi
98C074: call dword ptr [0xa281ec]        ; IAT import slot
98C07A: mov edi, eax / call 0x98bf57
98C081: push dword [0xb310ac] / push dword [0xb310b0]
98C08D: call dword ptr [0xa28150]        ; IAT import (2 args)
98C093: call eax / mov esi, eax / test esi,esi / jne
98C09B: push 0x214 / push 1 / call 0x989e22   ; alloc 532B, count 1
98C0AF: push esi / push [0xb310ac] / push [0xba9e20]
98C0BC: call 0x98beeb / pop ecx / call eax    ; dynamic call
=> IAT dynamic lookup/init fn  [推定相当]  (string-copy reading corrected)
```

### §2.2 `0xB30D28` table（v510_1b）
```
0xB30D28: 0x0000000C        ; count 12 [推定]
0xB30D2C: 0x00000008        ; stride 8 [推定]
0xB30D30: 0x6E697361 = "asin"
0xB30D34..0xB30D40: 0
0xB30D44: 0x3FE00000 = float 0.5
0xB30D48: 0x00776F70 = "pow"
0xB30D4C: 0
0xB30D50: 0x00736F63 = "cos"
0xB30D54: 0
=> math function name table + constants  [確定相当]
   FUN_0098540B null fallback -> default entry  [推定相当]
```

### §2.3 map ctor candidates（v510_1c）
```
FUN_004492e0: undefined4 __fastcall (int param_1)          (reset fn, v509)
FUN_0046c300: void __thiscall (int, int, char)             (char flag -> map mode [推定])
FUN_0046c460: undefined4 * __fastcall (undefined4 *param_1)
FUN_004b2730: void (int *param_1, int, undefined4)
FUN_004b3400: void __thiscall (int, undefined4)
=> candidate list for the map ctor; bodies unread  [部分確定]
```

## §3 残点 **V-511-1..3**
1. 【中】**map ctor の本文特定**（`FUN_0046C300` から：`char param_3` mode と vtable store 有無）＋ **`FUN_0098C072` の IAT slot（`0xA281EC`/`0xA28150`）の import 先**（DLL/関数名）と `0xB310AC`/`0xB310B0` global identity。
2. 【低】**Player `+0x80`（ptr）の中身** ＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v510_1 の1本 EXIT=0**（V-510-1 `FUN_0098C072`＝IAT dynamic lookup を機械確定相当で解釈更新・`0xB30D28`＝math name table を dword dump で確定相当／V-510-2 map ctor 候補5fn signature 列挙で部分確定／V-510-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_0098C072` machine を int3 まで decode** → **IAT slot call（`call dword [0xa281xx]`）**を発見 → **DLL import 系と即決（v505/v508 string copy 訂正）** → (ii) **`0xB30D28` の dword dump（12個）**で `"asin"`/`"pow"`/`"cos"`＋`0.5` を獲得 → math name table と確定 → (iii) **map ctor 候補5fn の signature 列挙**で `char param_3`（flag）を発見。
- 妥当性チェック: ①`call dword [0xa281ec]`⇔IAT slot（`0xA281xx`＝import table range [推定]）— ②`push 0x214; push 1; call 0x989e22`⇔alloc 532B（v492 alloc `0x7C`／v493 alloc `8`／v505 alloc `len+1` に続く size alloc）— ③`0x6E697361`⇔`"asin"`（LE 読替）／`0x3FE00000`⇔float 0.5（IEEE754）— ④`FUN_0046C300` の `char param_3`⇔v492 `FUN_004CBA80(param_3)` 同型 — ⑤`FUN_004492E0`＝reset⇔v509 確定 — **五方向矛盾なし**。
- docs との不整合: **①v505/v508 ruling「`FUN_0098C072`＝string copy 実体 [推定]」→ 本裁定で IAT dynamic lookup/init [推定相当] へ解釈更新（machine の IAT slot call が決定打）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`call dword [0xA281xx]` が IAT import call の合図**（**`0xA281xx`＝import table slot range [推定]**）→ **DLL import 系と即決**（**string copy 推定を覆す machine 証拠**）。
  2. **math 関数名テーブル（`{"asin","pow","cos",…}`＋`0.5` 定数）が `.data` に並ぶ**（**fallback default entry が「何の table か」を語る**＝v509 `0xB30D28` の完全解）。
  3. **`{count, stride, "name"}` の data 構造（v509）が「テーブル entry」の型**（`12`/`8`＝count/stride [推定]）。
  4. **解釈の更新（string copy→dynamic lookup）も裁定に明記**（**machine 証拠が docs 推定を覆す例**＝v469/v500/v508 に続く訂正連鎖の4度目）。
  5. **map ctor 候補の `char param_3`（flag）が mode 指定の目印**（v492 `FUN_004CBA80` と同型＝**map 構築の mode [推定]**）。
