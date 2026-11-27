# V-664-2 / V-664-3 の素材（RACE ATTR と health）

実装はしない。実測（実データ）と外部資料を突き合わせ、確定/未確定を分ける。

## 1. RACE ATTR のレイアウト（実測で確定）

**測定**: `D:\re\Oblivion.esm`（277504985 B）を python で走査（`GROUP_HEADER_SIZE=20`）。
RACE レコード **15 件**、`ATTR` は **全件 16 バイト**（UESP の RACE 統計表とも一致:
`ATTR | 15 | … | 16`）。

**レイアウト（確定）**: `16 B = 男性 8 u8 + 女性 8 u8`、順序は
`Strength, Intelligence, Willpower, Agility, Speed, Endurance, Personality, Luck`
（AV 0..7）。

実測値と UESP の突き合わせ（**完全一致**）:

| race | ATTR hex | 男性 8 | 女性 8 | UESP 男性 | UESP 女性 |
|---|---|---|---|---|---|
| Argonian `0x00023FE9` | `28 28 1e 32 32 1e 1e 32 28 32 28 28 28 1e 1e 32` | 40,40,30,50,50,30,30,50 | 40,50,40,40,40,30,30,50 | 40,40,30,50,50,30,30,50 | 40,50,40,40,40,30,30,50 |
| Imperial `0x00000907` | `28 28 1e 1e 28 28 32 32 28 28 28 1e 1e 28 32 32` | 40,40,30,30,40,40,50,50 | 40,40,40,30,30,40,50,50 | 40,40,30,30,40,40,50,50 | 40,40,40,30,30,40,50,50 |

値域は {10, 25, 30, 40, 50}（非 playable の種族で 10/25 が出る）。

**現行コードとの不一致**: `assets/esm_reader.h:702-709` の `RaceData` は
`attrStrength..attrPersonality` の **7 個の `uint8_t`**（Luck が無い・性別が無い）。
実測の 16 バイト（8 × 男女）と**形が合わない**。V-664-2 は
`RaceData` の属性表現を変える必要がある（`esm_reader.h` は #186 の作業ファイル
ではないが、`esm_reader.cpp:decodeRace` は #186 と**同一ファイル**。§5 参照）。

`decodeRace`（`esm_reader.cpp:1876-1878`）は ATTR を読まず、
「layout と health 源が未確定」というコメントで既定値 0 のまま。

## 2. 属性 → health の式

**外部資料（UESP Oblivion:Health）**: Original Oblivion の health は**レベル項を持つ**。

> "the base of your health is **Endurance × 2**"

レベルアップ時に Endurance の 1/10（切り捨て）が加算される。したがって式は

```
maxHealth(level) = 2 × Endurance + Σ_{l=2..level} floor(Endurance_at_l / 10)
```

**chargen はレベル 1**（レベルアップ加算はまだ 1 回も無い）なので、初期値は

```
maxHealth = 2 × Endurance        ← レベル 1 限定。レベル項は 0
```

UESP の表でも Lv1 は Endurance 30 → Health 60、60 → 120（= 2×Endurance）で一致。

**注意（レベルを書かないと壊れる）**: この式はレベル項を持つので、「レベル 1 で
`2 × Endurance`」と明示しないと、後で「レベルが上がっても `2 × Endurance` のまま
（または 100 のまま）」という実装を作る。レベルアップ加算は V-664-3 の範囲外だが、
式の形はここに残す。

| race | Endurance（男女同値） | 期待 maxHealth |
|---|---|---|
| Imperial | 40 | **80** |
| Argonian | 30 | **60** |
| Nord | 50 | **100** |

**現行コードとの不一致**: `player.h:54` の `BASE_MAX_HEALTH = 100` と
`Player::applyRace`（`player.cpp:190`）の `maxHealth = BASE_MAX_HEALTH` は
**100 固定**。race 適用後の期待値 80/60/100 にならない。

**未確定（Ghidra 側）**: exe の係数（`fHealth…` 系の定数）と、レベル 1 の
初期 health を「2 × Endurance」で計算していることの命令レベル確認は**未実施**
（2026-11-27 の `ghidra-mcp list_instances` は `{"instances": []}`）。
外部資料と port の一致は §2 の表までで、exe との突き合わせは残る。

## 3. 適用順序の設計（提案・実装はしない）

現行: `chargen::commit`（Panel 所有）→ `NpcManager::applyRaceToPlayer`
（`npc_manager.cpp:493`）→ `Player::applyRace`（`player.cpp:186`）。
`applyRace` は「①base へ revert ②skill bonus ③spell」。

提案する順序:

1. **race 選択**（Panel）→ `raceFormID` を決める（実装済み）
2. **性別**（Panel の Gender 210）→ 男性/女性のどちらの ATTR を使うか決める
   → **未実装（V-631-1b）**。ATTR は男女で値が違うため、**V-664-2 は Gender に依存**
3. **ATTR 設定**: `RaceBonus` に 8 属性（選択性別の値）を載せ、
   `Player::applyRace` が `attributes` を base ではなく**race 値で設定**
4. **health 再計算**: `maxHealth = 2 × attributes.Endurance`、`health = maxHealth`
   （`BASE_MAX_HEALTH` 固定をやめる）

`RaceBonus`（`player.h:178`）は今 `skillBonuses` と `spells` だけ。
`attributes`（8）を足すのが最小の変更点。

## 4. 期待値（1 種族・数値で）

**Imperial 男性**: 属性 = Str 40, Int 40, Wil 30, Agi 30, Spd 40, End 40, Per 50, Luck 50
（§1 実測）→ **maxHealth = 80**。
skill bonus は別系統（`DATA` 実測済み）で、health には効かない。

## 5. 競合

**`RaceData` は `assets/esm_reader.h`** にあり、**ヘッダ変更**になる（コンパイル面が広い）。
V-664-2/3 の `decodeRace`（`esm_reader.cpp:1810`）は **#186 の作業ファイル
`esm_reader.cpp`** の中にある。#186 の変更は `open`/`parseFromMemory`（同ファイル
221-238 付近）と**別関数**で、`.cpp` の関数衝突は無い。

- **#186 landing 後に着手**する。
- **着手時に `git status` で `esm_reader.h` / `esm_reader.cpp` が触られていないか
  再確認**する（ヘッダは広く効くので、取りこぼすと全ビルドに響く）。
- `player.h` / `npc_manager.h` もヘッダだが、#186 は触っていない。

## 6. RE の位置づけ（ブロッカーではない）

RE に投げるのは ①トグル 4 つの ON/OFF の意味 ②health の定数確認 の 2 つ。
**Eyes の反転は①の意味が分からなくても書ける**（反転の有無だけなら実装可能）。
**②の定数だけは、実装前に確認する**（`2 × Endurance` の係数を推測で焼かない）。

## 未確認

- exe 側の health 式と定数（Ghidra 未実施）
- 男女の選択が `raceFormID` とどう結びつくか（Gender 210 の ON/OFF の意味）
- `RaceData` の属性表現を男女ペアにするか、`applyRaceToPlayer` で性別を引数に
  取るか（設計判断は実装時）
