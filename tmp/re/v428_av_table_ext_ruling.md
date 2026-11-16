# v428 — AV name table の拡張実読と skills 区 id 順の裁定（V-428-1 完遂・派生残点第20弾）

- 採番: **v428**（派生残点群・probe prefix **v428_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416（読み取りのみ）
- 依頼経緯: v427 の残点 **V-428-1【低】（skills 区（id12..）の table 順序と LightArmor/Marksman/Mercantile の所在＝25件超の拡張実読）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 内文字列・data refs のみ（推測禁止・外部知識との差異は EXE 表を正とする）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **skills 区（id12..）の table 順序** | **[確定]（表実読）** | **EXE 表の id 順（`0xB0A1A8` 基点）**: `[12]Armorer/[13]Athletics/[14]Blade/[15]Block/[16]Blunt/[17]HandToHand/[18]HeavyArmor` → **`[19]Alchemy/[20]Alteration/[21]Conjuration/[22]Destruction/[23]Illusion/[24]Mysticism/[25]Restoration`** → **`[26]Acrobatics/[27]LightArmor/[28]Marksman/[29]Mercantile/[30]Security/[31]Sneak/[32]Speechcraft`** — **LightArmor/Marksman/Mercantile は `[27]/[28]/[29]` に所在 [確定]**（v427 の「[19]Alchemy で外部知識と差異」は**表の skill 区が physical(12-18)→magic(19-25)→stealth(26-32) の順**であることが拡張実読で判明し解消） |
| **skills 区の id 対応** | **[確定]（表実読）** | table index＝AV id（v427 の getter-fn 証明を継続適用）→ **EXE の skill id 順 ＝ 上記表順**（外部知識との差異は本裁定で EXE 表を正とする） |
| **behavior/reputation/effect AVs** | **[確定]（表実読・拡張）** | `[33]Aggression/[34]Confidence/[35]Energy/[36]Responsibility`（行為特性）→ `[37]Bounty/[38]Fame/[39]Infamy`（評判）→ **`[40]MagickaMultiplier/[41]NightEyeBonus/[42]AttackBonus/[43]DefendBonus/[44]CastingPenalty/[45]Blindness/[46]Chameleon/[47]Invisibility/[48]Paralysis/[49]Silence/[50]Confusion/[51]DetectItemRange/[52]SpellAbsorbChance/[53]SpellReflectChance/[54]SwimSpeedMultiplier/[55]WaterBreathing/[56]WaterWalking/[57]StuntedMagicka/[58]DetectLifeRange/[59]ReflectDamage/[60]Telekinesis/[61]ResistFire/[62]ResistFrost/[63]ResistDisease/[64]ResistMagic/[65]ResistNormalWeapons/[66]ResistParalysis/[67]ResistPoison/[68]ResistShock/[69]Vampirism/[70]Darkness`（効果系 AV・**window 砂切りで終端未確認＝残点 V-429-1**） |
| **table の使用形式** | **[確定(否定側)]** | 個別 cell（+19/+24/+25/+26/+27）への dword xref＝**0件** → **使用は base（`0xB0A1A8`）+offset の index access のみ**（v427 の `[edi*4+base]` 形と整合・`0x4F4610`/`0x4F60BC`/`0x4FEA26` の3 base refs） |

## §2 証拠（v428_1）

### §2.1 拡張 window 実読（`0xB0A1F0..0xB0A2C0`）
```
[18]HeavyArmor [19]Alchemy [20]Alteration [21]Conjuration [22]Destruction
[23]Illusion [24]Mysticism [25]Restoration [26]Acrobatics [27]LightArmor
[28]Marksman [29]Mercantile [30]Security [31]Sneak [32]Speechcraft
[33]Aggression [34]Confidence [35]Energy [36]Responsibility [37]Bounty
[38]Fame [39]Infamy [40]MagickaMultiplier ... [69]Vampirism [70]Darkness
（+70 まで string-ptr run が連続＝終端は window 外）
```

### §2.2 refs（v428_1(b)）
- `cell[+19]/[+24]/[+25]/[+26]/[+27]` への個別 dword xref＝**全て0件** → index access のみ（base refs 3件＝v427）。

## §3 残点 **V-429-1..2**
1. 【低】**table 終端（`[70]Darkness` 以降）の特定** — window を延長して run の終わり（非 string ptr）を確認＋最大 AV id（dispatcher case の上限 `0x1A+` との対応）。
2. 【低】V-428-2 引き継ぎ: `FUN_004F45D0`/`FUN_00501A00` 文脈と dispatcher direct caller `0x523358` の opcode identity。

## §4 検証・自己批評
- probe: **v428_1 全 EXIT=0**（1本で V-428-1 完遂）。
- **決定打の作り**: (i) **dword window の延長実読（+18..+70）で run が切れるまで列挙** → (ii) **個別 cell xref＝0 件で「index access のみ」を確認**（v427 の `[edi*4+base]` と接続）。
- 妥当性チェック: ①表順（physical→magic→stealth→behavior→reputation→effect）⇔index=id 証明（v427 getter-fn）— ②個別 refs 0⇔base refs 3件 — **二方向矛盾なし**。
- docs との不整合: **なし**（v427 の [部分確定] を [確定] へ昇格・外部知識差異は EXE 表優先で解消）。docs/GT 反映は #47/#20 一括。
- **教訓**: **「外部知識との差異」は自分の記憶側の疑いより先に表を延長実読する** — 差異の正体が「順序の外部記憶の誤り」か「表の subset」かを**実読が一撃で決める**（本件は前者＝表が完全で外部記憶が誤り）。
