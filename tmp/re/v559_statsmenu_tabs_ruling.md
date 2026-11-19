# v559 — U3 裁定書: StatsMenu(0x3EB) の 5 タブ表示仕様（ai-todo #157）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe v559_1〜v559_7）＋ BSA 実物 `menus\main\stats_menu.xml`（102,783 B / 3,478 行）。
判定: `[確定]` / `[確定(命令)]` / `[確定(XML+命令)]` / `[部分確定]` / `[推定]` / `[確定負]`
先行: `tmp/re/v364_stats_credits_supply_ruling.md`（本裁定は V-364-3 / V-364-4 をクローズ）

---

## 1. 結論（依頼の 4 点）

| # | 質問 | 判定 | 結論 |
|---|------|------|------|
| 1 | 5 タブの表示仕様 | **[確定(XML+命令)]** | 5 タブは **p1=General / p2=Attributes / p3=Skills / p4=Factions / p5=Misc**。タブ rect の `<id>` が 1..5 で、EXE の通知ハンドラ `0x5DCD70` の `lea eax,[ebx-1]; cmp eax,4; ja` と一致。タブ名は**文字列ではなくアイコン**（`Menus\Shared\all_tabs_icons.dds` のアトラス）。 |
| 2 | 表示行・ラベル文字列キー・値供給 getter の対応表 | **[確定(命令)]** | §2 の表。属性 8 行が `user1..user8`（現在）/`user9..user16`（base）、ラベルは `strings()` の `_strength` 等。 |
| 3 | 色分岐 `_red/_green/_blue_under/_even/_over` の比較定義 | **[確定(XML)]** | `under = 現在値 < base`／`even = 等しい`／`over = 現在値 > base`。色は root 定数で **under=(208,0,0) 赤 / even=(51,79,142) 青灰 / over=(0,128,0) 緑**。 |
| 4 | tab p1 の SetString 3 種が name/class/race のどれか | **[確定(命令)]（前提を訂正）** | **3 種ではない。文字列は 4 本 + 数値 1 本**：`0xFAF`=**名前**／`0xFB0`=**種族(race)**／`vfunc+0x268`=**星座(birthsign)**／`0xFB2`=**クラス(class)**／`0xFB3`=**レベル**(float)。 |

### 1.1 V-364-3（p4 の `[TESNPC+0x3C]` リスト）→ **[確定(命令)] 所属ファクション一覧**

`0x5DC950` の実体（§3.3）: `TESNPC+0x3C` は **所属ファクションの連結リスト**。要素は `{ +0 = TESFaction*, +4 = rank(byte) }` で、
* `[[faction]+0x18]+4` = ファクション名（TESFullName 契約）
* `0x51F350(faction, rank)` = **ランク名**
* ファクションのフラグ byte（`[[esi]+0x34]`）の **bit0 または bit3 が立っているものは除外**、ランク名が `"DUMMY"` のものも除外。

→ p4 は「所属ギルド／ランク」の一覧である。** faction でも relationship でも misc でもない**。

### 1.2 V-364-4（XML trait ↔ EXE trait）→ **[確定] 完全対応**

trait ID の実測（`0x5C3110` 系の名前→ID 表／v557 で 194 組を機械抽出）: `user0=0xFAE` から 1 ずつ増え `user16=0xFBE`。
`0x5DA1A0` は AV index 0..7 について **現在値 → `0xFAF+idx`、base → `0xFB7+idx`** を SetFloat する。

| XML 行 | XML 値 trait | XML base trait | EXE trait（現在 / base） | AV index | ラベル |
|---|---|---|---|---|---|
| strength | user1 | user9 | 0xFAF / 0xFB7 | 0 | `_strength` |
| intelligence | user2 | user10 | 0xFB0 / 0xFB8 | 1 | `_intelligence` |
| willpower | user3 | user11 | 0xFB1 / 0xFB9 | 2 | `_willpower` |
| agility | user4 | user12 | 0xFB2 / 0xFBA | 3 | `_agility` |
| speed | user5 | user13 | 0xFB3 / 0xFBB | 4 | `_speed` |
| endurance | user6 | user14 | 0xFB4 / 0xFBC | 5 | `_endurance` |
| personality | user7 | user15 | 0xFB5 / 0xFBD | 6 | `_personality` |
| luck | user8 | user16 | 0xFB6 / 0xFBE | 7 | `_luck` |

規則: **userN = AV(N−1) の現在値、user(N+8) = AV(N−1) の base**（N=1..8）。AV 順は STR, INT, WIL, AGI, SPD, END, PER, LUK。
値は `0x65D270(player, idx, 0)`（現在）と `0x5F1910(idx)`（base）から取得（v364 §3.3 の裏付けどおり）。

---

## 2. p1（General）の供給表

`0x5DBB00` の実読（[確定(命令)]）:

```
5DBB0A  eax = 0x4DA2A0(player)      → SetString(elem, 0xFAF /*user1*/, eax)   ; 名前（TESFullName）
5DBB23  eax = 0x5EA720(player)      → SetString(elem, 0xFB0 /*user2*/, eax)   ; 種族
5DBB3E  vfunc+0x268(player)         ; 戻り 0 なら "-" (0xA3A0E4)
5DBB5A  [[vfunc+0x268]+0x18]+4      ; 文字列（空なら 0xA2F7EC）
5DBB74                              → SetString(elem, 0xFB1 /*user3*/, eax)   ; 星座
5DBB84  eax = 0x5EA6B0(player)      → SetString(elem, 0xFB2 /*user4*/, eax)   ; クラス
5DBB9D  ax  = 0x5E1FD0(player)      → SetFloat (elem, 0xFB3 /*user5*/, ax)    ; レベル
```

XML 側（`:154-364`）: `stat_p1.race=user2`／`birthsign=user3`／`class=user4`／`level=user5`／`name=parent().user1`、ラベルは `_race`/`_birthsign`/`_class`/`_level`。

health/magicka/fatigue は `stat_p1.user6/user7`（health current/max）、`user8/user9`（magicka）、`user10/user11`（fatigue）。
供給は AV 8/9/10 系で、v364 §3.3 の **AV 9 → 0xFB6(current)+0xFB7/0xFBB、AV 10 → 0xFB8(current)+0xFB9/0xFBC** が magicka/fatigue の current/max と一致する **[部分確定]**（AV 8 = health は同 switch の列挙外）。

---

## 3. 各タブの実装

### 3.1 タブ切替 `0x5DCD70`（slot3）
`lea eax,[ebx-1]; cmp eax,4; ja` → **通知 id 1..5 = `stat_tabs_p1..p5` の `<id>1..5`**。XML root の `<user0> 1 </user0> <!-- current visible tab page -->` が現在タブ。

### 3.2 p2 = Attributes
8 行（`stat_p2_strength_layout` … `stat_p2_luck_layout`）。各行は `y = me().user0*67 + 87`、値 `<copy src="stat_p2" trait="userN">`、色は `userN` と `user(N+8)` の比較。行 rect の `<id>` は全て 24。

### 3.3 p4 = Factions `0x5DC950`（[確定(命令)]）

```
5DC990  vfunc+0x170(player) → 0x9832E6(TD 0xB03248=TESBoundObject, 0xB02FB4=TESNPC) → TESNPC*
5DC9A5  0x5893F0(elem 0x44)                  ; 一覧クリア
5DC9AD  node = TESNPC + 0x3C                 ; 所属ファクションリスト head
5DC9C4  item = [node]; next = [node+4]
5DC9D7  flags = [[item]+0x34]（byte）
5DC9DD  if (flags >> 3) & 1 → skip
5DC9E9  if flags & 1        → skip
5DC9F7  rankName = 0x51F350([item], (signed char)[item+4])
5DCA09  if rankName == "DUMMY"(0xA534F0) → skip
5DCA2C  row = 0x585410(elem, "stat_faction_template")
5DCA45  SetFloat(row, 0xFAA /*listindex*/, 行番号)
5DCAA9  SetString(row, 0xFAF /*user1*/, ファクション名)   ; 空白→'_' に置換してから文字列化
5DCACE  SetString(row, 0xFB0 /*user2*/, rankName)
```

### 3.4 p3 / p5
* `stat_skill_template` を instantiate するのは **`0x5DC630`** → p3 = Skills。
* `stat_misc_template` を instantiate するのは **`0x5DA930`**（31 箇所の参照）→ p5 = Misc。

（テンプレート名の実体: `stat_skill_template`=`0xA6DB58`、`stat_faction_template`=`0xA6DB6C`、`stat_misc_template`=`0xA6DA18`。）

---

## 4. 移植への示唆

1. 5 タブの意味は **General / Attributes / Skills / Factions / Misc**。移植のタブ名・順序を合わせるならこの 5 つ。
2. 属性の色は「base との比較」で決まる（赤=下回る / 青灰=同じ / 緑=上回る）。**現在値と base の 2 系統を必ず持つ**必要がある。
3. タブ見出しはアイコン（`all_tabs_icons.dds` のアトラス）で、文字列キーは無い。移植で文字ラベルを出すなら原版に無い独自要素。
4. p4 は「所属ファクション＋ランク名」。非表示フラグ（bit0/bit3）と `"DUMMY"` ランクの除外を再現する必要がある。

---

## 5. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-559-1 | health（AV 8）の p1 供給箇所（AV 9/10 と同型と推定） | 低 |
| V-559-2 | p3 / p5 の行内容の完全対応（`0x5DC630` / `0x5DA930` の行生成の全分岐） | 中 |
| V-559-3 | タブアイコンアトラスの各タブの UV（`stat_tabs_pX_icon` の crop 指定） | 低 |
| V-559-4 | `[item]+0x34` フラグ bit0/bit3 の意味（faction の hidden/特殊フラグ名） | 低 |

---

## 6. probe 一覧

`v559_1`〜`v559_7`（EXIT=0）: XML タブ/色/行抽出 → `0x5DBB00`（p1）／`0x5DC950`（p4）の実読 → テンプレート名の参照元特定（skill=0x5DC630 / faction=0x5DC950 / misc=0x5DA930）。
