# v489 — `+0x8C` impl＝bit5 flag setter・`iClass*` 21種リスト・key capture ずれの裁定（V-489-1..3 完遂・旧残点 V-406-3 継続第19弾）

- 採番: **v489**（旧残点群消化・probe prefix **v489_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-489-1..3（v488 残点）** — 1=`FUN_0046A9E0`（slot `+0x8C` shared impl）本文 identity と `iClass` キーの consumer／2=GameSetting entry 全122件 key 台帳化／3=V-488-2 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-489-1: `FUN_0046A9E0`（slot `+0x8C` impl）本文** | **[確定相当]** | L84201 実読 [確定(decompiled)]: **`void __thiscall FUN_0046a9e0(int *param_1, char param_2)`: `if (param_2 == 0) param_1[2] = param_1[2] & 0xffffffdf; else param_1[2] = param_1[2] \| 0x20;` → **`virtual[*param_1 + 0x40]()`** [確定(構造)] → **`virtual+0x8C(flag)`＝`self+0x8`（`param_1[2]`）u32 の **bit5（`0x20`）を flag==0 で clear／else set** し、その後 `virtual+0x40()` を呼ぶ flag-setter+notify fn [確定相当]**。→ **v488/v487 の「DATA 読み込み前 `virtual+0x8C(0)`」＝**bit5 clear（flag 下ろし）→ slot `+0x40` 実行**を意味 [確定相当]**、bit5（`0x20`）の semantic（writing/loaded/dirty 等 [推定]）と slot `+0x40` の機能は [未確定]＝残点 |
| **V-489-1: `iClass*` consumer（EXE 側）** | **[確定]（新規 consumer なし）** | `.text` refs of `0xB37D00`＝**8件すべて v479/v481 で既知の系統 [確定]**: `FUN_0051C090`（`cmp edx,[0xb37d00]; ret 4`）／`FUN_005222D0`（mid-decode `or eax,0xb37d00`＋`cmp ecx,[eax+0xc]`＝docs L216783 と同一）／`FUN_005C28F0`・`FUN_00662750`・`FUN_00663C50`（docs 比較行＝v479）／init（`0x9EDEAA`）／thunk（`0xA20120` `mov ecx,0xb37d00; jmp 0x4055E0`）→ **新規 consumer は0件**、`FUN_005222D0(1)` trigger 経路は v479 構造のまま [確定] |
| **V-489-2: GameSetting entry 全 key 台帳化** | **[部分確定]（121/122 capture・ずれ注意）** | **insert-style 122件中121件で key 文字列 capture（`0xB37BC0` のみ no-key）[確定(機械)]**。**`iClass*` ファミリ21種＋`iPlayerCustomClass` を一覧獲得 [確定(実文字列)]**: `iClassAcrobat/Agent/Assassin/Knight/Healer/Mage/Bard/Priest/Warrior/Barbarian/Crusader/Scout/Archer/Rogue/Thief/Monk/Sorcerer/Witchhunter/Spellsword/Nightblade/CharactergenClass`（`0xB37C48..0xB37D00`）＋**`iPlayerCustomClass`（`0xB37C88`）** → **chargen クラス選択 set（基準クラス群＋カスタム＋chargen）[推定相当]**。他系統も獲得: `iLevelUp01..09Mult`（`0xB37628..0xB37668`）／`iPersuasion*`（11種）／`iCrimeGold*`（5種）／`iPerk*`（7種）／`Effects\*.NIF/.dds` パス群（`0xB37150..`）…。**ただし window decode（40-byte back）方式のため address→key 対応に 1件ずれの可能性（前方 entry の key を拾う）[確定（制約）] → 台帳化は「各 addr 直近の push 固定」で再取得が必要＝残点の形を明記 [部分確定]** |
| **V-489-3: V-488-2 引継ぎ** | **[未確定]（残点継続）** | predicate slot35 英名、`FUN_004CBA80` `param_3`、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v489_1）

### §2.1 `FUN_0046A9E0` body（v489_1a）
```
L84201: void __thiscall FUN_0046a9e0(int *param_1, char param_2) {
L84204:   if (param_2 == '\0') { param_1[2] = param_1[2] & 0xffffffdf; }   // clear bit5 (0x20)
L84207:   else { param_1[2] = param_1[2] | 0x20; }                          // set bit5
L84212:   (**(code **)(*param_1 + 0x40))();                                 // notify slot +0x40
L84214: }
WARNING: Could not recover jumptable at 0x0046a9fe / Treating indirect jump as call
```

### §2.2 `0xB37D00` refs（v489_1b）＝ v479 system reconfirmed
```
51C095 (FUN_0051c090): cmp edx,[0xb37d00] / jne / ret 4
522320 (FUN_005222d0): or eax,0xb37d00 / cmp ecx,[eax+0xc]  (mid-decode = docs L216783)
5C297C / 66275A / 663CB4: docs-side compares (FUN_005c28f0 / FUN_00662750 / FUN_00663c50)  [known]
9EDEAA init / A20120 thunk  [known]
-> new consumers: 0  [確定]
```

### §2.3 full key capture（v489_1c）
```
insert-style: 122 / captured with key: 121 / without: 1 (0xB37BC0)
iClass family (0xB37C48..0xB37D00): Acrobat Agent Assassin Knight Healer Mage Bard Priest
  Warrior Barbarian Crusader Scout Archer Rogue Thief Monk Sorcerer Witchhunter
  Spellsword Nightblade CharactergenClass  (+ iPlayerCustomClass @0xB37C88)
others: iLevelUp01..09Mult / iPersuasion* (11) / iCrimeGold* (5) / iPerk* (7) /
        Effects\*.NIF|dds paths / iHorse* / iBarter* / iMapMarkerVisibleDistance / ...
CAVEAT: window-based capture may shift address->key by one (frontier entry's key)
        -> re-capture with nearest-push fix before ledgering  [部分確定]
```

## §3 残点 **V-490-1..3**
1. 【中】**`+0x8` u32 の bit5（`0x20`）semantic**（writing/loaded/dirty 等 [推定]）と **slot `+0x40` 機能**（bit 変更後の notify 先）＋ **`iClassCharactergenClass`/`iClassNightblade` consumer**（v479 `FUN_005222D0(1)` trigger 経路の EXE 側調査＝`FUN_0051C090` と `+0xc` 比較の意味）。
2. 【中】**GameSetting entry key 台帳化の再取得**（各 addr 直近 push 固定で122件の正確対応→台帳 row）。
3. 【低】**V-489-3 引継ぎ** — predicate slot35 英名、`FUN_004CBA80` `param_3`、sink 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v489_1 の1本 EXIT=0**（V-489-1 `FUN_0046A9E0` 本文で bit5 set/clear＋`+0x40` notify を確定相当／V-489-1 consumer 新規0件を確定／V-489-2 key 121/122 capture と `iClass*` 21種獲得・**ずれ制約を明記**／V-489-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_0046A9E0` decompiled が3行**で `|= 0x20 / &= ~0x20`＋`virtual+0x40()` → **flag-setter+notify と即決**（v488 thunk 追跡の完結）→ (ii) **`0xB37D00` refs を全列挙**し8件すべて既知系統と確認（新規 consumer 0＝v479 構造維持）→ (iii) **key capture を122件へ拡張**し `iClass*` ファミリを一覧獲得、**window 方式のずれ制約を自己指摘**。
- 妥当性チェック: ①`param_1[2]`⇔byte `+0x8` u32（`int*` index2×4）⇔flag field — ②`0x20`⇔bit5 — ③`virtual+0x40()`⇔notify（flag 変更後の再計算 [推定]）— ④`iClass*` 21種⇔Oblivion 基準クラス数 [推定]— ⑤121/122⇔1145 sites のうち insert 122（v488）と整合 — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_0046A9E0` annotation 実在）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **flag setter fn の `|= mask / &= ~mask` の2分岐＋直後の virtual notify が定番**（v472 bit11 と同じ型・mask `0x20`＝bit5）。
  2. **mid-decode（`or eax,addr`）でも docs 行と対応すれば既知系統と判別できる**（本裁定で8件を v479 既知に同定）。
  3. **window-based string capture は「前方 entry の key を拾う」ずれを生む** — **台帳化前に「各 addr 直近の push」固定で再取得**（**capture 方式の精度を ruling に明記**する習慣が残点の形を守る）。
  4. **`iClass*` ファミリ一覧が「chargen クラス set」としての裏付け**（21種＋`iPlayerCustomClass`＋`iClassCharactergenClass`）＝**v462 chargen 文脈と接続**。
  5. **capture 完了率（121/122）と no-key 件数（1件）を明記**（欠損の所在＝`0xB37BC0`）。
