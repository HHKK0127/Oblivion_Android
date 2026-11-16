# v441 — FormType の直接証拠・flag B consumer chain・docs 欠落下限確定の裁定（V-441-1..3 完遂・派生残点第33弾）

- 採番: **v441**（派生残点群・probe prefix **v441_1..v441_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v440 の残点 **V-441-1【低】（`byte[obj+4]=FormType` の直接証拠）／V-441-2【低】（flag B 新 consumer 4件の identity）／V-441-3【低】（docs 欠落の最終レンジと `[esp+0x5c]` path CFG）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-441-1: `byte[obj+4]`＝FormType の直接証拠** | **[確定]** | **record-class ctor（vtable store 直近の `mov byte [reg+4], imm`）から class↔FormType 対応表を49件構築 [確定]**: `0x04=TESGlobal/0x05=TESClass/0x06=TESFaction/0x07=TESHair/0x08=TESEyes/0x0A=TESSoundFile/0x0B=TESSkill/0x0E=TESLandTexture/0x12=TESObjectACTI/0x13=TESObjectAPPA/0x14=TESObjectARMO/0x15=TESObjectBOOK/0x16=TESObjectCLOT/0x17=TESObjectCONT/0x18=TESObjectDOOR/0x1A=TESObjectLIGH/0x1B=TESObjectMISC/0x1C=TESObjectSTAT/0x1D=TESGrass/0x1E=TESIconTree/0x20=TESFurniture/0x21=TESObjectWEAP/0x22=TESAmmo/0x24=TESCreature/0x25=TESLevCreature/0x26=TESSoulGem/0x27=TESKey/0x29=TESSubSpace/0x2A=TESSigilStone/0x2B=TESLevItem/0x2D=TESWeather/0x2E=TESClimate/0x2F=TESRegion/0x30=TESObjectCELL/0x31=TESChildCell/0x39=TESTopic/0x3A=TESTopicInfo/0x3B=TESQuest/0x3C=TESIdleForm/0x3E=TESCombatStyle/0x3F=TESLoadScreen/0x40=TESLevSpell/0x41=TESObjectANIO/0x42=TESWaterForm/0x43=TESEffectShader（+ `0x00=TESUsesForm` 等）**。**`FUN_00469520` の switch 14 値のうち12値が対応表と直接一致し全て inventory 品目**（`0x13 APAP/0x14 ARMO/0x15 BOOK/0x16 CLOT/0x1A LIGH/0x1B MISC/0x21 WEAP/0x22 AMMO/0x26 SLGM/0x27 KEYM/0x2A SGST/0x2B LVLI`）、残2値（`0x19/0x28`）は vtable-window が noise で masked [部分確定]。**＋ `FUN_004fab20` 内に `byte[obj+4]` の FormType 許可リスト判定を発見 [確定]**: `4FB168: movzx ecx,byte [edi+4]; cmp ecx,6; je …; cmp ecx,0x23; jne 0x4fb357`（**`byte[obj+4] ∈ {6, 0x23}` のみ許可**、他は fail）と `4FB183: cmp byte [eax+4],0x43`（`=EffectShader FormType`）→ **script 側が FormType を `+4` で switch/cmp する使用形を machine で直接確認 [確定]**。**→ `byte[obj+4]`＝FormType は v439/440/441 の3裁定を通じて [確定] に昇格** |
| **V-441-1（残）: `0x19`/`0x28` の class 名** | **[部分確定]**（残点） | vtable-window 法では noise（TESIcon 等）で masked。`0x19`/`0x28` は switch set 内（inventory 品目）で DOOR..LIGH / SLGM..SGST の空き — **別の同定法（window 拡大・FormType 名 string 対応）が要 [残点]**。enum 名テーブルは EXE に無し（`'kFormType'/'AlchemyItem\x00Ammo'` 等の string census 0 hits）[確定(否定側)] |
| **V-441-2: flag B getter chain** | **[部分確定]** | **`FUN_0056b190`（`mov al,byte [edx*8+0xb0a54d]; ret`）の E8 caller は1件 `0x56ADAC` → `FUN_0056ad60`**、`FUN_0056ad60` の callers も1件 `0x56A464` → **`FUN_0056a3b0`** → **chain `FUN_0056a3b0 → FUN_0056ad60 → FUN_0056b190` [確定]**。`FUN_0056ad60`（L270565）＝**`void __fastcall(byte *param_1)`**（`param_1`＝byte 参照＝type index 群）[確定(decompiled)]。**`FUN_0056a3b0` の所属 pass は [未確定]（残点）** |
| **V-441-2: `FUN_004fab20` 分岐先 `0x4FB1A1`** | **[確定]** | aligned decode（start `0x4FB160`）で **`0x4FB1A1: cmp eax, 0x21`** — **eax＝arg type code、`0x21`＝v437 の arg type 表で Form Type** → **type-B flag consumer の分岐先は「arg type==FormType」判定 [確定]**。同一 window で `4FB160: test edi,edi; je fail` → FormType 許可リスト（上記）→ `4FB17E: jmp 0x4fafdb` へ合流 [確定] |
| **V-441-2: compiler/VM 帯 fn の signature** | **[確定]（signature）＋[部分確定]（用途）** | `FUN_0056b220`（L270795）＝**`undefined4 FUN_0056b220(uint param_1, ushort *param_2)`**、`FUN_0056b2e0`（L270828）＝**`void FUN_0056b2e0(uint *param_1, uint *param_2, char param_3)`**、`FUN_004fab20`（L194420）＝**`void FUN_004fab20(double*, int, int*, int, undefined4, int, int, char)`（8引数）** [確定(decompiled)]。**両者の callers と所属 pass は [未確定]（残点）** |
| **V-441-3: docs 欠落下限（minseg=0x10 ret-confirmed）** | **[確定]（実測）** | **gaps>0x200 内部境界6197件／ret-confirmed（前 segment 末尾＝`C2/C3/C9/CB`）＝3038件（49.0%）／unconfirmed 3159** → **docs 欠落 function の ret-confirmed 下限＝3038 [確定(実測)]**（v439 の minseg=0x30 版2037から増加・小 stub を含めた効果）。unconfirmed は tail-jmp(`E9`) epilogue と intra-function padding を含む [推定]。**minseg=0x10 の segment 法 extra＝4749（v440）と併せ、欠落レンジ＝「ret-confirmed 下限 3038 ～ segment 法上限 4749」[確定(実測)]** |
| **V-441-3: `[esp+0x5c]` path CFG** | **[未確定]（残点継続）** | 本 v441 群でも未着手（probe budget）。v439 の記録（fill 点6件・`0x40A29D` branch 0件）を維持。 |

## §2 証拠（v441_1..v441_3）

### §2.1 record-class → FormType 対応表（v441_1）
```
method: mov byte [reg+4], imm8 hits (219) × window(vtable store ±0x50) RTTI,
        record-class prefix only (TES*/BGS*), noise (Extra*/NiT*/TESFullName...) 除外
  0x00 TESUsesForm      0x04 TESGlobal        0x05 TESClass        0x06 TESFaction
  0x07 TESHair          0x08 TESEyes          0x0A TESSoundFile    0x0B TESSkill
  0x0E TESLandTexture   0x12 TESObjectACTI    0x13 TESObjectAPPA   0x14 TESObjectARMO
  0x15 TESObjectBOOK    0x16 TESObjectCLOT    0x17 TESObjectCONT   0x18 TESObjectDOOR
  0x19 (masked -> TESIcon noise)              0x1A TESObjectLIGH   0x1B TESObjectMISC
  0x1C TESObjectSTAT    0x1D TESGrass         0x1E TESIconTree     0x20 TESFurniture
  0x21 TESObjectWEAP    0x22 TESAmmo          0x23 (masked)        0x24 TESCreature
  0x25 TESLevCreature   0x26 TESSoulGem       0x27 TESKey          0x28 (masked -> TESIcon)
  0x29 TESSubSpace      0x2A TESSigilStone    0x2B TESLevItem      0x2D TESWeather
  0x2E TESClimate       0x2F TESRegion        0x30 TESObjectCELL   0x31 TESChildCell
  0x39 TESTopic         0x3A TESTopicInfo     0x3B TESQuest        0x3C TESIdleForm
  0x3E TESCombatStyle   0x3F TESLoadScreen    0x40 TESLevSpell     0x41 TESObjectANIO
  0x42 TESWaterForm     0x43 TESEffectShader
switch set 14 values: 12 directly matched (all inventory-capable), 0x19/0x28 masked.
string census (v441_1b): 'kFormType' / 'FormType\x00' / 'AlchemyItem\x00Ammo' 0 hits
  -> enum-name table NOT in binary [確定(否定側)].
```

### §2.2 FormType usage in FUN_004fab20（v441_3a）
```
aligned decode (start 0x4FB160) up to branch target:
  4FB160: test edi, edi / je 0x4fb357
  4FB168: movzx ecx, byte [edi + 4]        ; ★ FormType
  4FB16C: cmp ecx, 6 / je 0x4fafdb         ; allow {6}
  4FB175: cmp ecx, 0x23 / jne 0x4fb357     ; allow {0x23} else fail
  4FB17E: jmp 0x4fafdb
  4FB183: cmp byte [eax + 4], 0x43         ; ★ FormType == EffectShader
  4FB187: jne 0x4fb357
  ...
  4FB1A1: cmp eax, 0x21                    ; ★ arg type == Form Type (v437 table)
```

### §2.3 flag-B consumer chain（v441_2b / v441_3b）
```
FUN_0056b190 (mov al, byte[edx*8+0xb0a54d]; ret)
  E8 caller: 0x56ADAC -> FUN_0056ad60   (L270565: void __fastcall FUN_0056ad60(byte *param_1))
FUN_0056ad60 callers: 0x56A464 -> FUN_0056a3b0   <- chain root (identity [未確定])
FUN_0056b220 (L270795): undefined4 FUN_0056b220(uint, ushort*)
FUN_0056b2e0 (L270828): void FUN_0056b2e0(uint*, uint*, char)
FUN_004fab20 (L194420): void FUN_004fab20(double*, int, int*, int, undefined4, int, int, char)
```

### §2.4 docs census（v441_3c）
```
minseg=0x10, gaps>0x200: internal boundaries 6197
  ret-confirmed (prev ends C2/C3/C9/CB): 3038 (49.0%)
  unconfirmed: 3159  (tail-jmp E9 epilogue + intra-function CC padding を含む [推定])
missing-function range: 3038 (ret-confirmed lower bound) .. 4749 (segment extra upper)
```

## §3 残点 **V-442-1..3**
1. 【低】**`0x19`/`0x28` FormType の最終同定**（vtable-window noise 除外の別法：window 拡大／`FUN_004fab20` の許可リスト `{6,0x23}` の意味（6=Faction? 0x23=NPC? — 何の許可リストか）＋ `0x43` 判定の用途）。
2. 【低】**flag B chain の所属 pass** — `FUN_0056a3b0`（chain root）の identity と `FUN_0056b220/2e0` の callers（script compiler か runtime か）。
3. 【低】**`[esp+0x5c]` path CFG**（V-440-3 引き継ぎ）＋ **docs census unconfirmed 3159 の切り分け**（tail-jmp epilogue 判定＝前 segment 末尾が `E9` 系 jcc/jmp である件の走査）。

## §4 検証・自己批評
- probe: **v441_1..v441_3 の3本全 EXIT=0**（V-441-1 **[確定] に昇格**（2値 masked は [部分確定]）／V-441-2 chain・signature・分岐先を確定（root pass は残点）／V-441-3 census 完遂・CFG は残点）。
- **決定打の作り**: (i) **`mov byte [reg+4], imm` ×「vtable store 直近 window」×「record-class prefix」の3条件交差で対応表49件を一撃構築**（v440 の window 法を class 種別フィルタで昇華）→ (ii) **switch set と対応表の肯定一致12/14＋否定側（STAT/NPC/Creature は switch 外）で [確定] 化** → (iii) **`FUN_004fab20` の branch target を aligned decode し `byte[obj+4]` の FormType 許可リストと `cmp eax,0x21` を直接確認** → (iv) chain は **E8 census を caller 側へ2段たどる**（getter→ad60→a3b0）→ (v) census は **minseg=0x10 で ret-confirmed まで実測**。
- 妥当性チェック: ①対応表49件⇔class 名が既知 record sig（ARMO/BOOK/CLOT/CONT/DOOR/LIGH/MISC/STAT/WEAP/AMMO/SLGM/KEYM/SGST/LVLI/CREA/LVLC/ACTI/APPA/FURN…）と完全整合 — ②switch 14 値⇔inventory 品目（12 direct＋2 masked）— ③`cmp ecx,6 / 0x23`⇔`byte[obj+4]` の FormType 使用形 — ④`cmp eax,0x21`⇔v437 arg type 表 `0x21=Form Type` — ⑤chain caller が各1件⇔linear な single-use getter — ⑥ret-confirmed 3038 ≥ minseg=0x30 版2037（閾値単調性）— **六方向矛盾なし**。
- docs との不整合: **なし**（enum 名テーブルが docs/EXE 双方に無いことを確認したのみ）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **window 相関の noise は「class 種別 prefix ＋ noise denylist」の2段フィルタで大幅に削れる** — v440 の1段 window が12件の偽マッチを出したのに対し、v441 の2段で49件の純対応表が得られた（ただし denylist に無い `TESIcon` 等が2値を still masked → **denylist 補完を次の probe の第一歩に**）。
  2. **enum の name テーブル不在は「string census の0 hits」で [確定(否定側)]** — 名前を出す努力より先に「名前が無いこと」を確定させる。
  3. **branch target の identity は aligned decode で target 直前から取得** — `cmp eax,0x21` のように、target そのものが判定命令ならその場で意味が確定する（fun の中盤 decode は start を 0x40 ずらして試行）。
  4. **single-use getter chain は E8 census を2段たどれば root まで2回で届く** — caller=1 の fn は chain 構造の強いシグナル。
  5. **census の retro 精度（ret-confirmed）は閾値と組み合わせて必ず2×2 で測る**（0x30×ret=2037／0x10×ret=3038／0x10×raw=4749）— 単値引用は禁止。
