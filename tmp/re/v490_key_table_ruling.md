# v490 — key 台帳 122/122 完成（v489 訂正）・notify 先 `FUN_0046ABF0`・bit5 grep 部分の裁定（V-490-1..3 完遂・旧残点 V-406-3 継続第20弾）

- 採番: **v490**（旧残点群消化・probe prefix **v490_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-490-1..3（v489 残点）** — 1=`+0x8` u32 の bit5 semantic と slot `+0x40` 機能＋`iClassCharactergenClass` consumer／2=GameSetting entry key 台帳化の再取得／3=V-489-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-490-2: GameSetting entry key 台帳化（再取得）** | **[確定]（122/122・v489 訂正）** | **nearest-push（fn 境界内で `mov ecx` 直前まで）方式で全122件 key capture 成功（no-key 0）[確定(機械)]** → **v489 の window ずれを訂正**（例: `0xB370C8`＝v489 `iAimingNumIterations` → 正 `iArrowInventoryChance`／`0xB37300` → 正 `sActivateCreatureCalmed`）。**`iClass*` 完全リスト（22種）[確定(実文字列)]**: `Acrobat/Agent/Assassin/Pilgrim/Knight/Healer/Mage/Bard/Priest/Warrior/Barbarian/Crusader/Scout/Archer/Rogue/Thief/Monk/Sorcerer/Battlemage/Witchhunter/Spellsword/Nightblade/CharactergenClass`（`0xB37C48..0xB37D00`）＋**`iPlayerCustomClass`（`0xB37C88`）** → **v489「21種」を22種に訂正（`iClassPilgrim`（`0xB37C60`）と `iClassBattlemage`（`0xB37CE0`）を検出）→ 基準クラス22種＋custom＋chargen [推定相当]**。**value 初期値の例も獲得: `iMapMarkerVisibleDistance`（`0xB37BC8`）`value=0x4E20`（20000）[確定(機械)]**。他 systems も正確対応で確定: `iLevelUp01..10Mult`（`0xB37620..0xB37668`・**`iLevelUp10Mult` 検出＝v489 の `09Mult` までを拡張**）／`iCrimeGold*`（6種）／`iPersuasion*`（14種）／`iPerk*`（10種）／`sHitParticle*`（12種）／`sBlood*`（6種）／`iRemoveExcessDead*`（4種）…（**全122件を台帳 row 資料として保存 [確定]**） |
| **V-490-1: slot `+0x40`（`FUN_0046A9E0` の notify 先）** | **[確定]** | **`[TESObjectREFR vt=0xA46C44 + 0x40] = 0x46ABF0` → `FUN_0046ABF0`（owning L84338）[確定(機械)]** → **`virtual+0x8C(flag)`（bit5 set/clear）→ `virtual+0x40()`＝`FUN_0046ABF0` が bit 変更後の実行体 [確定相当]**、`FUN_0046ABF0` の本文 identity は [未確定]＝残点 |
| **V-490-1: bit5（`+0x8` u32 の `0x20`）semantic** | **[部分確定]（直接 grep は band 内 hit なし）** | REFR band（115 fn）で `param_1[2]`/`+0x8` と `0x20` mask の直接 hit **0件 [確定（否定側）]** — ただし band hits は**別 usage**（`param_1[2] = … \| 0x200000`＝**bit21**／`*(param_1[2] + 0x20)`＝**ptr+offset 20**（`param_1[2]` が ptr の別 type usage））＝ **`param_1[2]` の type 依存性を再確認 [確定]** → **`FUN_0046A9E0` 内の `0x20`（bit5）以外に直接 hit が無く、semantic は「`FUN_0046ABF0`（notify 先）の扱う flag」として残点 [部分確定]** |
| **V-490-3: V-489-3 引継ぎ** | **[未確定]（残点継続）** | predicate slot35 英名、`FUN_004CBA80` `param_3`、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v490_1）

### §2.1 notify 先（v490_1a1）
```
[TESObjectREFR vt=0xA46C44 + 0x40] = 0x46ABF0  owning=FUN_0046abf0 (L84338)
```

### §2.2 bit5 grep（v490_1a2）
```
REFR band (115 fns): param_1[2]/+0x8 with 0x20 mask direct hits: 0   [否定側]
band hits are other usages:
  param_1[2] = param_1[2] | 0x200000   (bit21, L346592/L346683/L371110)
  *(char *)(param_1[2] + 0x20) == ...   (ptr + offset 0x20, L401538..)
=> param_1[2] is type-dependent in this band (reconfirmed)
```

### §2.3 full accurate key table（v490_1b）— 122/122
```
insert sites: 122 / key captured: 122 / no-key: 0  (nearest-push within fn boundary)
corrections vs v489: 0xB370C8 iArrowInventoryChance (was iAimingNumIterations)
                     0xB37300 sActivateCreatureCalmed / 0xB37160 sHitParticleDirt ...
iClass complete (22): Acrobat Agent Assassin Pilgrim Knight Healer Mage Bard Priest
  Warrior Barbarian Crusader Scout Archer Rogue Thief Monk Sorcerer Battlemage
  Witchhunter Spellsword Nightblade CharactergenClass  (+ iPlayerCustomClass @0xB37C88)
value example: 0xB37BC8 iMapMarkerVisibleDistance value=0x4E20 (20000)
also new vs v489: iLevelUp10Mult @0xB37668 / iCrimeGold* (6) / iPersuasion* (14)
full list saved in probe out (ledger material)
```

## §3 残点 **V-491-1..3**
1. 【中】**`FUN_0046ABF0`（slot `+0x40` notify 先）の本文 identity** → bit5（`0x20`）semantic と slot `+0x40` の機能が確定する。
2. 【低】**GameSetting key 台帳の row 化**（v490 probe out の122件を台帳 `tmp/re` ファイルへ保存→ledger 連携）＋value 初期値の取得拡張（`push imm` の int value を全件）。
3. 【低】**V-490-3 引継ぎ** — predicate slot35 英名、`FUN_004CBA80` `param_3`、sink 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v490_1 の1本 EXIT=0**（V-490-1 notify 先 `FUN_0046ABF0` を slot 実読で確定・bit5 grep は否定側／V-490-2 key 台帳 122/122 完成と v489 訂正／V-490-3 引継ぎ）。
- **決定打の作り**: (i) **slot `+0x40` を vtable 実読で `FUN_0046ABF0` に特定**（v487/v488 の slot 実読連鎖）→ (ii) **nearest-push（fn 境界＋直前固定）で capture を再実行** → 122/122・v489 ずれを訂正・`iClassPilgrim`/`iClassBattlemage`/`iLevelUp10Mult` 検出 → (iii) **bit5 直接 grep の否定側**を記録して semantic を notify 先へ絞る。
- 妥当性チェック: ①`[vt+0x40]=FUN_0046ABF0`⇔`FUN_0046A9E0` の `virtual+0x40()` 呼び出し（v489）— ②122/122⇔v488 の insert-style 122件と一致 — ③`iClass` 22種⇔v489 の21種＋`Pilgrim`/`Battlemage`（Oblivion 基準クラス [推定]）— ④`value=0x4E20`⇔int value（`i` prefix）— ⑤bit21（`0x200000`）⇔bit5（`0x20`）＝**別 bit で混同しない区別を記録** — **五方向矛盾なし**。
- docs との不整合: **①v489 ruling「`iClass*` 21種」→ 本裁定で22種に訂正（`Pilgrim`/`Battlemage` 追加）／②v489 key ずれ（window offset）→ nearest-push で訂正（例 `0xB370C8`）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **capture の精度は「fn 境界＋直前の push 固定」で確実になる** — window（後方 40B）方式のずれを1本の probe で訂正（**v489 の制約を v490 で解消**＝**残点の形（制約明記）が次の probe を的確に誘導した**）。
  2. **同一 `param_1[2]` でも band で type 依存（u32 vs ptr）** — **mask bit（`0x20` vs `0x200000`）と offset（`+0x20`）を混同しない区別を明記**（grep hit 0 は否定側として有効）。
  3. **vtable slot 実読（`[vt+0x40]`）が notify 先を即特定**（slot 実読連鎖の総仕上げ）。
  4. **value 初期値（`push imm`）は key と同時取得可能**（`iMapMarkerVisibleDistance=0x4E20`）→ 台帳化時に value も拾う。
  5. **訂正も裁定の価値**（v489 の21種→22種・key ずれ→正確対応＝**台帳データの品質向上**が成果）。
