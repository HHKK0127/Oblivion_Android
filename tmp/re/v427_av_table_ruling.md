# v427 — AV name table と actor value id/格納先の裁定（V-414-1 完遂・派生残点第19弾）

- 採番: **v427**（派生残点群・probe prefix **v427_1..v427_4 の4本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v414 の残点 **V-414-1【低】（AV index9/10/`[+0xC]` の語の裏取り＝TESActorBaseData `[+8]`/`[+0xA]` の正体）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **AV name table の所在と構造** | **[確定]** | **`.data 0xB0A1A8` ＝ char* テーブル（AV id→name）** — window 実読25件連続: **`[0]Strength/[1]Intelligence/[2]Willpower/[3]Agility/[4]Speed/[5]Endurance/[6]Personality/[7]Luck`（＝8属性）→ `[8]Health/[9]Magicka/[10]Fatigue/[11]Encumbrance` → `[12]Armorer/[13]Athletics/[14]Blade/[15]Block/[16]Blunt/[17]HandToHand/[18]HeavyArmor/[19]Alchemy/[20]Alteration/[21]Conjuration/[22]Destruction/[23]Illusion/[24]Mysticism`**（base refs: `.text 0x4F4610`/`0x4F60BC`/`0x4FEA26` の3件） |
| **table index ＝ AV id** | **[確定]** | **`FUN_004F45D0`（head 0x4F45D0）が AV 値取得 `call 0x5F1910`（FUN_005F1910＝AV id→値 getter・v410 既知）と同一 fn 内で `mov ecx, [edi*4 + 0xB0A1A8]`（＝`table[edi]`）を名前として debug ログ出力**（`cmp byte [0xB361AC],0` ガード→`push ecx; push 0xA487C0; call 0x579B60`）→ **table 空間 ＝ AV getter の id 空間**。＋dispatcher `FUN_00519F50` の case0..7＝8属性 setter と table[0..7]＝8属性の完全一致 |
| **AV id8/9/10/11 の語** | **[確定]** | **id8=Health・id9=Magicka・id10=Fatigue・id11=Encumbrance**（table 実読＋上記 index=id） |
| **格納先: AV#8（Health）** | **[確定]** | **`actor[+0x84]`** — dispatcher **case8 → `FUN_00519C50`**（L210155-210160: `*(param_1+0x84) = param_2; FUN_0046abf0();`（notify））。callers **5件**: dispatcher `FUN_00519F50`／**`FUN_005222D0`（attr/skill 再計算＝計算値投入）**／**`FUN_0066C200`（levelup 適用）**／`FUN_00511570`（未注記）／`0x523F40`（formType7 gate） |
| **格納先: AV#9（Magicka）/ AV#10（Fatigue）** | **[確定]** | **AV#9 → `TESActorBaseData[+8]`（ctor default 50）／AV#10 → `TESActorBaseData[+0xA]`（ctor default 50）** — dispatcher case9/case10 → `FUN_00467290`/`FUN_004672B0`（v414 setter family）＋ v414 ctor reset defaults（`mov edx,0x32`＝50/50）と接続 |
| **TESActorBaseData の getter（副次）** | **[確定]** | **slot19(+0x4C) `FUN_00467360` ＝ `mov ax,[ecx+0xa]; ret`（`[+0xA]` getter）**／slot20 `FUN_004672F0`・slot2 `FUN_004673F0` は stricmp ベースの name lookup 系 |
| **skills 区（id12..）の順序** | **[部分確定]**（残点 V-428-1） | table は `[18]HeavyArmor` の直後に **`[19]Alchemy`**（外部知識の skill id 順（LightArmor/Marksman/Mercantile 帯）と差異）→ **skills 区の table順＝id 順は [推定高]・EXE 表を正とする**（attrs/Health/Magicka/Fatigue 区の確定には影響なし） |

## §2 証拠（v427_1..v427_4）

### §2.1 table 実読（v427_2(a)・dword window `0xB0A148..0xB0A208`）
```
[0]0xA51EF4 Strength   [1]Intelligence [2]Willpower [3]Agility [4]Speed
[5]Endurance [6]Personality [7]Luck
[8]0xA51EA0 Health  [9]0xA51E98 Magicka  [10]0xA51E90 Fatigue  [11]Encumbrance
[12]Armorer [13]Athletics [14]Blade [15]Block [16]Blunt [17]HandToHand
[18]HeavyArmor [19]Alchemy [20]Alteration [21]Conjuration [22]Destruction
[23]Illusion [24]Mysticism
（[-24..-1] は id/code ペアの別構造＝テーブルは 0xB0A1A8 から開始）
```
- `Strength` の xref＝`.text 0x751BE1`/`0x9F9661`＋`.data 0xB0A1A8`（table 本体）。

### §2.2 index=id の証明（v427_4(a)）
```
FUN_004F45D0（head 0x4F45D0）:
  4F45F1: call 0x5f1910            ; AV 値取得（v410 で FUN_005F1910(player, id) 確定済）
  4F4604: cmp byte [0xb361ac], 0   ; debug フラグ
  4F460D: mov ecx, [edi*4 + 0xb0a1a8]  ; ★ table[edi] = AV name
  4F461B: push 0xa487c0 ; call 0x579b60 ; ログ出力（name 付き）
```
- `FUN_004F45D0` callers＝`FUN_00501A00` の1件（v427_4(b)）。

### §2.3 case8 handler と callers（v427_3）
- `FUN_00519C50`（L210155-210160）: **`*(param_1 + 0x84) = param_2; FUN_0046abf0();`**（notify＝v411 の dirty 更新系と同 fn）。
- E8=5: **`FUN_00519F50`（dispatcher case8）**／`FUN_005222D0`／`FUN_0066C200`／`FUN_00511570`／`0x523F40`。
- dispatcher `FUN_00519F50` の direct callers＝`0x523358`（unannotated・script opcode 帯）1件（v427_4(c)）。

### §2.4 vtable 補足（v427_1(b)/v427_2(c)）
- TESActorBaseData vtable `0xA3B654`: slot0=`0x467240`（v414 reset）／slot14=`0x4677D0`・slot16=`0x4677E0`（global getter `mov eax,[0xB371C8/0xB371B0]; ret`）／**slot19=`0x467360`＝`mov ax,[ecx+0xa]; ret`**／slot24 帯は vtable 外の文字列（`0x62616E55`="Una"）。

## §3 残点 **V-428-1..2**
1. 【低】**skills 区（id12..）の table 順序と id の対応**（`[19]Alchemy` の外部知識差異）— table を 25 件超えて拡張実読し、LightArmor/Marksman/Mercantile の所在を確認（EXE 表を正とする）。
2. 【低】`FUN_004F45D0`/`FUN_00501A00` の文脈（どの id でログされるか＝id 空間の最終応用例）と dispatcher direct caller `0x523358`（opcode identity）。

## §4 検証・自己批評
- probe: **v427_1..v427_4 の4本全 EXIT=0**（V-414-1 完遂）。
- **決定打の作り**: (i) **`Strength` の xref から `.data` の table cell を発見** → (ii) **dword window 実読で table 全体（25連続 string ptr）と開始位置を確定** → (iii) **table 使用 fn を aligned decode で `table[edi]` 形に特定し、同 fn 内の AV getter（`FUN_005F1910`）接続で index=id を証明** → (iv) **case8 handler（`[param_1+0x84]` setter）と5 callers で格納先を確定** → (v) v414 の setter family/defaults と接続。
- 妥当性チェック: ①table[0..7]＝8属性⇔dispatcher case0..7＝属性 setter — ②`table[edi]`⇔AV getter 同一 fn — ③case8→`[actor+0x84]`⇔callers＝recalc/levelup（計算値系）— ④case9/10→setter⇔ctor default 50/50 — **四方向矛盾なし**。
- docs との不整合: **なし**（v414 の [推定高]（Health/Magicka 系）を語確定へ）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **名前テーブルは「xref が無い文字列」からではなく「xref がある隣接文字列」を足がかりに**（`Strength` の `.data` cell xref が入口、`Health`/`Magicka` 本体には dword xref が無い）。
  2. **`[edi*4 + table]` 形の index access を見つけたら、同 fn 内の「値取得 call」との接続で id 空間を証明できる**。
  3. **格納先は「case handler fn の1行 setter（`*(this+disp)=value`）」で決まる** — handler の callers（recalc/levelup）が値の性格（計算値/基礎値）を語る。
