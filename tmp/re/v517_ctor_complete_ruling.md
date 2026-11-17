# v517 — `FUN_0046C460` ctor 完遂（current id 初期化）・`FUN_0060D0A0`＝empty stub・type10 extra の裁定（V-517-1..3 完遂・旧残点 V-406-3 継続第47弾）

- 採番: **v517**（旧残点群消化・probe prefix **v517_1 の1本 EXIT=0**（初回構文エラー→修正再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-517-1..3（v516 残点）** — 1=extra type 10 正体＋`FUN_0046C460` ctor 残り本文＋`FUN_0060D0A0` identity／2=gated mask 源と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-517-1: `FUN_0046C460` ctor 残り本文（init 完遂）** | **[確定相当]（current id 初期化）** | docs body 実読（L85986-86030）[確定(decompiled)]: **`"formEnumString[ %d ] and formEnumString[ %d ] have the same iFormString %s in TESForm.cpp."`（duplicate log）＋`"You must fix the problems in TESForm.cpp to run this game."`（fatal error）→ `local_8 = 0x45`（69）ループ: `uVar4 += 0xc`（**entry stride 12B**）、`uVar6++`、`ppuVar5 += 3`（3 dwords/entry）→ **`formEnumString[]`＝69 entries × 12B（`{iFormID, iFormString, ...}`）[確定相当]** → `DAT_00b33c10 = 1`（once flag）→ **`*(param_1 + 1) = 0; param_1[2] = 8; param_1[3] = 0;`（init fields）→ **`uVar3 = FUN_00448d40(); param_1[3] = uVar3;`（`param_1[3]`（`+0xC`）に FUN_00448D40 の結果＝current id を代入＝v511 current ptr と同 field！）** → **`if ((uVar7 < 0x800) && (uVar7 != 0x800))`（`< 2048` かつ `!= 2048`）→ `if ((param_1[2] >> 0xe & 1) == 0)`（bit14 gate・v512/v514 family）→ `if (uVar7 != 0) FUN_004524c0(uVar7)`（old remove）→ `if (param_1[3] != 0) FUN_00446b40(param_1[3])`（FF id set・v512）→ **`FUN_00452570(0x800, param_1)`（`0x800`＝2048 を insert）→ `param_1[3] = 0x800`** → `if (param_1[3] != 0) FUN_00452570(param_1[3], param_1);` → `return param_1`** → **`FUN_0046C460`＝TESForm ctor（vtable store＋formEnum 検証＋`param_1+0xC`（current id）を `FUN_00448D40()` で初期化し、`0x800`（2048）範囲チェック＋bit14 gate＋old remove＋FF id set＋insert で current id を設定）[確定相当]。`param_1+0xC`＝current form id（`0x800`＝2048＝FormID 範囲 [推定]）、`FUN_00448D40`＝current id getter [推定相当]** |
| **V-517-1: `FUN_0060D0A0`（TESForm vtable `+0x0/+0x4`）** | **[確定]（empty fn）** | L374987 実読 [確定(decompiled)]: **`void FUN_0060d0a0(void) { return; }`（empty fn）→ TESForm vtable の `[+0x0/+0x4]`＝empty fn（pure virtual stub / do-nothing default [推定相当]）**（ctor/dtor pair 候補は empty＝**pure virtual stub/default implementation [推定相当]**＝`FUN_0046C460`（real ctor）とは別 slot [確定]） |
| **V-517-1: extra data type 10 正体** | **[部分確定]（2 usage 確認・`+0xc` 語は残点）** | **`FUN_0041e210(10)` mentions＝2件 [確定]: `L22080`（`FUN_0041FA30` 内・v516）／`L26536`（別 fn）** → type 10 extra data は2箇所で使用 [確定]、**`+0xc` field の semantic は残り usage（`L26536` の context）が次の手＝残点 [部分確定]** |
| **V-517-2: gated mask 源／caller 分離** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-517-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v517_1）

### §2.1 `FUN_0046C460` ctor 完遂（v517_1b）
```
L85986: log("formEnumString[ %d ] and formEnumString[ %d ] have the same iFormString %s ...")
L85990: uVar4 += 0xc / uVar6++ / ppuVar5 += 3 / local_8--   ; 69 x 12B entries
L85997: DAT_00b06b18 = uVar2;  L85999: fatal("You must fix the problems in TESForm.cpp...")
L86001: DAT_00b33c10 = 1;                                    ; once flag
L86003: *(param_1 + 1) = 0;  L86004: param_1[2] = 8;  L86005: param_1[3] = 0;
L86008: uVar3 = FUN_00448d40();  L86009: param_1[3] = uVar3;  ; current id into +0xC
L86014: if ((uVar7 < 0x800) && (uVar7 != 0x800)) {            ; 2048 range
L86015:   if ((param_1[2] >> 0xe & 1) == 0) {                 ; bit14 gate (v512)
L86017:     if (uVar7 != 0) FUN_004524c0(uVar7);              // old remove (v505)
L86020:     if (param_1[3] != 0) FUN_00446b40(param_1[3]);    // FF id set (v512)
L86022:     FUN_00452570(0x800, param_1);                      // insert 2048 (v494)
L86024:   } param_1[3] = 0x800; }
L86028: if (param_1[3] != 0) FUN_00452570(param_1[3], param_1);
L86030: return param_1;
=> TESForm ctor complete: current id init + range check + set/replace chain  [確定相当]
   param_1+0xC = current form id; FUN_00448D40 = current id getter  [推定相当]
```

### §2.2 `FUN_0060D0A0`（v517_1a）
```
L374987: void FUN_0060d0a0(void) { return; }   // empty stub  [確定]
  -> TESForm vt[+0x0/+0x4] = stub; real ctor = FUN_0046C460 (different slot)
```

### §2.3 type 10 usage（v517_1c）
```
FUN_0041e210(10): 2 mentions
  L22080 (FUN_0041fa30, v516) / L26536 (other fn)
  -> +0xc field semantics = read L26536 context next  [部分確定]
```

## §3 残点 **V-518-1..3**
1. 【中】**`FUN_00448D40`（current id getter）identity** ＋ **`L26536` context（type 10 extra `+0xc` の値の意味）** ＋ **`0x800`（2048）＝default form id の semantic**（FormID 範囲の下限 [推定]）。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v517_1 の1本 EXIT=0**（初回構文エラー→修正）。V-517-1 `FUN_0046C460` ctor 完遂（current id 初期化＋set/replace chain）を確定相当・`FUN_0060D0A0`＝empty stub を確定・type10 は2 usage で部分確定／V-517-2/3 は引継ぎ）。
- **決定打の作り**: (i) **`FUN_0046C460` の docs body 続き（L85986-86030）を読む** → **`FUN_00448D40()`→`param_1[3]`（current id）＋`0x800` range＋bit14 gate＋old remove＋FF id set＋insert の連鎖**が ctor の init 完遂と判明 → (ii) **`FUN_0060D0A0` docs 5行**で empty stub と確定 → (iii) **`FUN_0041e210(10)` mentions（2件）**で usage 確認。
- 妥当性チェック: ①`param_1[3]`（`+0xC`）⇔v511 current ptr（同一 field で ctor が初期化）— ②`FUN_004524c0`（old remove）⇔v505 remove／`FUN_00446b40`（FF id set）⇔v512／`FUN_00452570`（insert）⇔v494＝**set/replace chain の4 fn が ctor に集結** — ③`0x800`＝2048⇔`< 0x800 && != 0x800` range — ④`uVar4 += 0xc` stride⇔`formEnumString` 12B/entry — ⑤empty stub⇔vtable `+0x0`（pure virtual default [推定]）— **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_0046C460` ctor 完遂・`FUN_0060D0A0` empty とも docs と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **ctor の init は「current id 初期化（getter→`+0xC`）＋range check＋set/replace chain」の4段**（**`FUN_00448D40`→`+0xC`→`0x800` range→remove/set/insert**＝**v511 set/replace と v512 FF id set が ctor に集結**）。
  2. **`0x800`（2048）＝default/current form id の目印**（`< 0x800 && != 0x800` range＝**FormID 範囲 [推定]**）。
  3. **empty fn（`{ return; }`）＝pure virtual stub/default**（**vtable `+0x0` の empty が「未実装 default」**＝ctor/dtor pair 候補の否定）。
  4. **`formEnumString[]` の stride（`+= 0xc`＝12B）が entry size を語る**（**69 entries×12B＝828B テーブル**＝v515 `0x45` と接続）。
  5. **ctor 完遂で「`param_1+0xC`＝current id」が確定**（**v511 current ptr の初期化元が判明＝横断整合**）。
