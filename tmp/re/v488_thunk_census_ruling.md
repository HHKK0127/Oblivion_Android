# v488 — slot `+0x8C` 共通 thunk・GameSetting entry 122件 census・docs 文言 0件の裁定（V-488-1..3 完遂・旧残点 V-406-3 継続第18弾）

- 採番: **v488**（旧残点群消化・probe prefix **v488_1 の1本 EXIT=0**（初回構文エラー→修正再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-488-1..3（v487 残点）** — 1=`virtual+0x8C(0)` の機能（`0x4D65D0` impl 本文）と GameSettings usage／2=V-487-3 引継ぎ／3=GameSetting entry 群の全列挙を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-488-1: `virtual+0x8C(0)`（DATA 読み込み前の mode 呼び出し）impl** | **[確定]（thunk 同定）＋[未確定]（機能本文）** | **`FUN_004D65D0`（`[TESObjectREFR vt+0x8C]`）の machine＝`jmp 0x46a9e0`（5-byte thunk）[確定(機械)]** → **REFR と CELL の slot `+0x8C` は**共通 impl `FUN_0046A9E0` へ集約 [確定]**（v487 で「REFR＝`0x4D65D0`／CELL＝`0x46A9E0`」と別値に見えたのは**前者が thunk**）→ **`virtual+0x8C(0)` の実体＝`FUN_0046A9E0`（docs annotation 実在・owning 単独 fn）[確定]**、**mode-0 の機能（`0` が何を指定するか）は本文未読＝残点**。副次: `0x4D65E0`（int3 の次の連続 fn）＝`mov eax,[ecx]; mov edx,[eax+0x170]`（**slot `+0x170` 呼び出し wrapper**＝v480 getter 経由）[確定(機械)] |
| **V-488-3: GameSetting static entry 群の全列挙（台帳化 basis）** | **[確定相当]（census）＋[確定]（構造）** | **`mov ecx, 0xB37xxx`（imm32 `0xB37000..0xB37FFF`）sites＝1145件、うち**`call 0x419A70`（v484 key-insert）を含む **insert-style＝122件 [確定(機械)]**。**上位40件の key 文字列を一括取得 [確定(実文字列)]**: `iMarksmanFatigueBurnPerSecondSkill`（`0xB37018`）／`iAimingNumIterations`（`0xB370C0`）／`iArrowInventoryChance`（`0xB370C8`）／`iArrowMaxRefCount`（`0xB370D0`）／`sHitParticleStone..Wood/Chain/Snow`（`0xB37150..0xB371A8`）／`sBloodParticle*/sBloodTexture*`（`0xB371B0..0xB371D8`）／`iAINPCRacePowerChance`／`sNormalWeaponsResisted`／`iCombatHighPriorityModifier`／`iPerk*`（`0xB37228..0xB37260`）／`iCombatCastDrainMinimumValue`／`iAICombat*`／`iAICombatMinDetection`／`sActivateNPCCalmed`／`sActivateCreatureCalmed`（`0xB372F0..0xB37300`）…／v487 の `iClassCharactergenClass`（`0xB37D00`）／`iClassNightblade`（`0xB37CF8`）／`fEssentialDeathTime` も同系統** → **`DAT_00B3xxxx`＝GameSetting static entry 配列（**this が `0xB370C0/C8/D0…` の **8-byte stride＝`{value(4B), key ptr(4B)}` 構造 [確定相当]**、v487 `FUN_00419A70` の `*param_1=value; param_1[1]=key` と一致）→ **prefix `i`＝int／`f`＝float／`s`＝string の value 型体系 [確定相当]**。**全122件の key 台帳化は残り [低]＝次の手**（本裁定で40件分） |
| **V-488-1: GameSettings consumer（docs 側）** | **[確定]（否定側）** | **docs で `Charactergen`/`Nightblade`/`GameSetting` の3 keyword とも **0 hits [確定(否定側)]** → **GameSettings は docs 文言に無く EXE 文字列のみ [確定]**（consumer 調査は EXE 側（`FUN_005222D0(1)` trigger 経路）へ** |
| **V-488-2: V-487-3 引継ぎ** | **[未確定]（残点継続）** | predicate slot35 英名、`FUN_004CBA80` `param_3`、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v488_1）

### §2.1 slot `+0x8C` thunk（v488_1a）
```
FUN_004d65d0 (annotation absent):
  4D65D0: jmp 0x46a9e0        <- 5-byte thunk to shared impl
  4D65D5: int3 ...
  4D65E0: mov eax,[ecx] / mov edx,[eax+0x170]   (next fn: slot+0x170 wrapper)
=> REFR vt+0x8C and CELL vt+0x8C share FUN_0046A9E0
```

### §2.2 GameSetting entry census（v488_1c）
```
mov ecx, 0xB37xxx sites: 1145
insert-style (contains call 0x419A70): 122
sample keys (address = 'this' singleton):
  0xB37018 iMarksmanFatigueBurnPerSecondSkill
  0xB370C0 iAimingNumIterations   0xB370C8 iArrowInventoryChance   0xB370D0 iArrowMaxRefCount
  0xB37150 sHitParticleStone ... 0xB371A8 sHitParticleSnow (10 keys, stride 8)
  0xB371B0 sBloodParticleDefault .. 0xB371D8 sBloodTextureExtra2 (8 keys, stride 8)
  0xB37200 iAINPCRacePowerChance  0xB37208 sNormalWeaponsResisted
  0xB37228 iPerkAttackDisarmChance .. 0xB37260 iPerkLightArmorMasterMinSum
  0xB372F0 iAICombatMinDetection  0xB372F8 sActivateNPCCalmed  0xB37300 sActivateCreatureCalmed
  (+ v487: 0xB37CF8 iClassNightblade / 0xB37D00 iClassCharactergenClass / 0xB37D08 ctx fEssentialDeathTime)
entry stride 8 = {value(4), key-ptr(4)}  (matches FUN_00419A70 writes)
key prefixes: i=int / f=float / s=string   [推定相当]
```

### §2.3 docs keywords（v488_1b）
```
'Charactergen': 0 hits / 'Nightblade': 0 hits / 'GameSetting': 0 hits   [否定側]
```

## §3 残点 **V-489-1..3**
1. 【中】**`FUN_0046A9E0`（slot `+0x8C` shared impl）の本文 identity**（mode-0 の機能＝DATA 読み込み前の設定）＋ `iClassCharactergenClass`/`iClassNightblade` の consumer（`FUN_005222D0(1)` trigger 経路の EXE 側調査）。
2. 【低】**GameSetting entry 全122件の key 台帳化**（本裁定40件→残り82件）。
3. 【低】**V-488-2 引継ぎ** — predicate slot35 英名、`FUN_004CBA80` `param_3`、sink 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v488_1 の1本 EXIT=0**（初回構文エラー（余分な閉じ括弧）→修正）。V-488-1 slot `+0x8C` thunk を確定・機能本文は残点／V-488-3 census 122件・key 40件を確定相当／V-488-1 docs keyword 0件を否定側確定／V-488-2 引継ぎ。
- **決定打の作り**: (i) **vtable slot 値（`0x4D65D0`）を decompile すると `jmp 0x46a9e0`＝thunk** → v487「別値」を解消し**共通 impl に集約**（**slot 値は thunk のことがある**）→ (ii) **`mov ecx, 0xB37xxx` の imm 範囲 census＋`call 0x419A70` 併存判定**で entry 122件を機械抽出 → push arg の rdata 実読で key を一括 → (iii) **key 連続アドレス（`0xB370C0/C8/D0`）の stride 8**⇔`FUN_00419A70` 書き込み2 field で構造確定。
- 妥当性チェック: ①thunk `jmp 0x46a9e0`⇔CELL slot `+0x8C`＝`0x46A9E0`（v487）— ②entry stride 8⇔`*param_1`（4B）＋`param_1[1]`（4B）— ③`i/f/s` prefix⇔value 型体系 [推定]— ④122件⇔`0xB37xxx` 1145 sites のうち `call 0x419A70` 併存のみ — ⑤docs keyword 0⇔docs 未収録（v487 実文字列 docs 0件と整合）— **五方向矛盾なし**。
- docs との不整合: **docs に GameSetting keyword 0件（docs 未収録の systems）[確定(否定側)]**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **vtable slot 値は thunk（`jmp rel32`）のことがある** — slot を読んだら**その先が jmp なら飛び先の impl が本体**（v487「REFR/CELL 別値」を1本 thunk で解消）。
  2. **`mov ecx, imm32` の imm 範囲で singleton 群を census**（`0xB37xxx`＝1145→insert 122）— **imm 範囲＋特定 call の併存が機能で絞る**。
  3. **push arg（rdata ptr）の実文字列を一括取得**すると key 台帳が40件/本で立つ（**census→strings の順**）。
  4. **連続アドレスの stride（8B）⇔decompiled 書き込み field 数（2）で entry 構造を確定**（`{value,key}`）。
  5. **docs keyword 0件も確定**（systems が docs 未収録だと分かれば EXE 文字列側に経路を固定できる）。
