# v487 — `DAT_00B37D00`＝GameSetting entry 確定・`FUN_004D9D00`＝REFR slot `+0x1C` method の裁定（V-487-1..3 完遂・旧残点 V-406-3/V-406-4 継続第17弾）

- 採番: **v487**（旧残点群消化・probe prefix **v487_1..v487_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-487-1..3（v486 残点）** — 1=旧残点 V-406-3 継続（`"DATA"` 24B が何の record の値か＝`FUN_004D9D00` 呼び出し元、`+0x8C(0)` の意味）／2=`DAT_00B37D00` 確定英名（`0x9EDEAA` 帯 owning 機械境界）／3=V-486-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-487-2: `DAT_00B37D00` の英名（key 文字列）** | **[確定相当]** | **init fn（`0x9EDEA0..0x9EDEBF`）の machine 実読 [確定(機械)]: `push 0x230e6`（143590・値/id）→ `push 0xa5b4a8`（**key 文字列 ptr**）→ `mov ecx,0xb37d00`（this）→ `call 0x419a70`（v484 key-insert）→ `push 0xa20120; call 0x981fb4`（registration）→ `ret`** → **`0xA5B4A8` の実文字列＝`"iClassCharactergenClass"` [確定(実文字列)]** → **`DAT_00B37D00`＝GameSetting entry「`iClassCharactergenClass`」（key=`i…`＝int 系・chargen class 設定値）singleton [確定相当]**。**隣接 singleton も同型で実文字列獲得 [確定]**: **`DAT_00B37CF8`＝`"iClassNightblade"`（`0xA5B494`）／`DAT_00B37D08` 帯 ctx＝`"fEssentialDeathTime"`（`0xA5B4C0`・`f…`＝float 系）→ 連続 init 帯（`0x9EDE80..`）＝**GameSetting entry 群の静的 singleton 列 [確定相当]**。docs mentions は3文字列とも **0件（docs 未収録の実文字列）[確定(否定側)]**。→ **v484「setting key 登録 map」推定を更新: 「登録 map singleton」ではなく「**特定 GameSetting entry の value object singleton**」[確定相当→訂正・追補]**（`FUN_00419A70` の key insert は entry の初期化として読む） |
| **V-487-1: `FUN_004D9D00` の owning class（呼び出し元）** | **[確定]（vtable 登録）** | **E8 callers＝0件（v487_1）→ vtable 登録を `.rdata` dword search（v479 法）→ raw hits 12件・全件 `slot=+0x1C` [確定(機械)]: `TESObjectREFR`（vt=0xA46C44）／`Actor`（0xA6E074）／`ArrowProjectile`（0xA6F08C）／`Character`（0xA6FC9C）／`Creature`（0xA710F4）／`MobileObject`（0xA732FC）…（12件＝REFR 派生系統）** → **`FUN_004D9D00`＝`TESObjectREFR` 系 vtable slot `+0x1C` の method [確定]** → **`param_1`＝`TESObjectREFR` 系 form（v480 と再一致）→ slot `+0x1C`＝subrecord 読み込み method（`Load/Read` [推定相当]）**。→ **6 float（`"DATA"` 24B）は REFR 系 form の DATA subrecord 値＝v480 で追った `+0x20..+0x34` の2×3float 群の出所が確定 [確定相当]** |
| **V-487-1: `+0x8C(0)`（DATA 読み込み前の呼び出し）** | **[部分確定]（slot 候補）＋[未確定]（機能）** | **`[TESObjectREFR vt=0xA46C44 + 0x8C] = 0x4D65D0`（owning `FUN_004D6450` 内）／`[TESObjectCELL vt+0x8C] = 0x46A9E0` [確定(機械)]** → **`param_1`（REFR 系）の `virtual+0x8C(0)` は `0x4D65D0` 系 impl [推定高]**、mode-0 呼び出し（`0`＝フラグ/セグメント番号 [推定]）の意味は本文未読＝残点 |
| **V-487-3: V-486-3 引継ぎ** | **[未確定]（残点継続）** | predicate slot35 英名（`CanResolveDependencies` [推定]）、`FUN_004CBA80` `param_3`、sink `DAT_00b34d90` 正体、新 FourCC（`XLOC/YLOC` 等）意味 |

## §2 証拠（v487_1/v487_2）

### §2.1 init fn boundary + key strings（v487_1b + v487_2a）
```
machine:
  9EDE80: push 0x23c08 / push 0xa5b494 / mov ecx,0xb37cf8 / call 0x419a70
          / push 0xa20110 / call 0x981fb4 / pop ecx / ret      (sibling init fn)
  9EDEA0: push 0x230e6 / push 0xa5b4a8 / mov ecx,0xb37d00 / call 0x419a70
          / push 0xa20120 / call 0x981fb4 / pop ecx / ret      (DAT_00B37D00 init, 0x9EDEA0..0x9EDEBF)
  9EDEC0: fld [0xa31c80] / push 0xa5b4c0 / mov ecx,0xb37d08 / call 0x41bae0 ...
strings (.rdata):
  0xA5B4A8 = 'iClassCharactergenClass'   (DAT_00B37D00 key)   docs mentions: 0
  0xA5B494 = 'iClassNightblade'          (DAT_00B37CF8 key)   docs mentions: 0
  0xA5B4C0 = 'fEssentialDeathTime'       (ctx of 0xB37D08)    docs mentions: 0
numeric pushes: 0x230e6 = 143590 / 0x23c08 = 146440
```

### §2.2 `FUN_004D9D00` vtable registration（v487_1a + v487_2b）
```
E8 callers: 0                      -> vtable-registered (v479 method)
rdata dword search 0x4D9D00: 12 hits, ALL at vtable slot +0x1C:
  vt=0xA46C44 .?AVTESObjectREFR@@   slot=+0x1C
  vt=0xA6E074 .?AVActor@@           slot=+0x1C
  vt=0xA6F08C .?AVArrowProjectile@@ slot=+0x1C
  vt=0xA6FC9C .?AVCharacter@@       slot=+0x1C
  vt=0xA710F4 .?AVCreature@@        slot=+0x1C
  vt=0xA732FC .?AVMobileObject@@    slot=+0x1C  (+6 more, same family)
```

### §2.3 `+0x8C` slot candidates（v487_1a）
```
[TESObjectREFR vt=0xA46C44 + 0x8C] = 0x4D65D0 (in FUN_004d6450)
[TESObjectCELL vt=0xA469D4 + 0x8C] = 0x46A9E0 (FUN_0046a9e0)
```

## §3 残点 **V-488-1..3**
1. 【中】**`virtual+0x8C(0)` の機能**（`0x4D65D0` impl 本文＝DATA 読み込み前の mode/segment 設定の意味）＋ **`iClassCharactergenClass`/`iClassNightblade` の GameSettings usage**（`FUN_005222D0(1)` trigger と v462 chargen の接続）。
2. 【低】**V-487-3 引継ぎ** — predicate slot35 英名裏取り、`FUN_004CBA80` `param_3`、sink `DAT_00b34d90` 正体、新 FourCC（`XLOC/YLOC` 等）意味。
3. 【低】**GameSetting entry 群の全列挙**（`0x9EDE00..0x9DF000` 帯の init fn 連鎖から key 文字列を一括取得＝`DAT_00B3xxxx` 系の台帳化）。

## §4 検証・自己批評
- probe: **v487_1..v487_2 の2本全 EXIT=0**（V-487-1 `FUN_004D9D00`＝REFR slot `+0x1C` method を vtable 逆引きで確定・6 float 出所を確定相当／V-487-2 `DAT_00B37D00`＝`"iClassCharactergenClass"` entry を実文字列で確定相当・owning 境界 `0x9EDEA0..0x9EDEBF` を機械確定／V-487-3 引継ぎ）。
- **決定打の作り**: (i) **init fn の `push 0xa5b4a8`（key ptr）を .rdata で実読** → `"iClassCharactergenClass"` が identity を一撃（**v484 error 文言法に続く「key 文字列法」**）→ (ii) **E8 0件→vtable 逆引き（v479 法）で12件・全 `slot=+0x1C`** → class と slot を同時確定 → (iii) 隣接 singleton の key も同時に取得（`iClassNightblade`／`fEssentialDeathTime`）。
- 妥当性チェック: ①`push 0xa5b4a8`⇔`FUN_00419A70` の `param_2`（key・v484 で `%s` に使われると一致）— ②`i…`/`f…` prefix⇔GameSettings int/float key naming [推定] — ③`slot=+0x1C` 12件⇔REFR 派生系統（v480 `+0x170` getter の同一 family）— ④`FUN_004D9D00` の `param_1[7]`（`+0x1C`）⇔reference 解決結果＝**vtable slot `+0x1C` と `param_1[7]`＝`+0x1C` field の同 offset 一致（slot と field の対応を注意）** — ⑤`"iClassCharactergenClass"`⇔chargen class（v462 `FUN_005222D0` 文脈）— **五方向矛盾なし**。
- docs との不整合: **①v484 ruling「`DAT_00B37D00`＝setting key 登録 map singleton」→ 本裁定で「特定 GameSetting entry（`iClassCharactergenClass`）の value object singleton」へ訂正・追補（`FUN_00419A70` は entry 初期化として読む）／②3文字列とも docs mentions 0件＝docs 未収録の実文字列 [確定(否定側)]**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **init fn の `push <ptr>` 引数を .rdata で実読**（key 文字列 ptr→実文字列）が singleton identity の決定打（**v484 error 文言法→本裁定 key 文字列法**）。
  2. **E8＝0 は vtable 登録 fn の合図（v479 教訓の再利用）→ raw hit を slot index まで遡る**（12件が全 `+0x1C` で class family と slot が同時に確定）。
  3. **隣接 init fn（同型・int3 無し連鎖）の key 文字列を一括取得**すると singleton 群の台帳が一気に立つ（本裁定で3件）。
  4. **key の prefix（`i`/`f`）が value 型（int/float）を語る**（GameSettings naming [推定]）。
  5. **slot index（`+0x1C`）と同 offset の field（`param_1[7]`＝`+0x1C`）は別物**（slot は vtable 位置・field は object 位置）—— **同一 hex での混同を避けるため「slot `+0x1C`」と「field `+0x1C`」を明記**。
