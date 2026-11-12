# v351 — #97 キャラクター作成構造調査 解析メモ（原本素材 / ESM / RE 正典の突合）

- 日付: 2026-11-10
- 依頼: ai-todo #97（解析依頼・高優先）／供与先: #101（キャラクター作成仕様の確定・人間裁定）
- 関連: #123（RE レーン依頼＝NAME 保存先の残件）、#122（同レーン既存依頼・v350 予約）
- 手法: 原本メニュー XML（BSA 抽出物）・ESM 読み取り実装・RE 正典（s58_chargen.md / s77_facegen_faction.md / v62_v59_residuals.md / RE_EXE_REPORT §46.1）との突合＋少量の新規 EXE プローブ（re_util.py）。
- 採番: v350 は #122 予約のため本メモは **v351**。

## 0. 結論サマリ

| # | 論点 | 判定 | 根拠の中心 |
|---|---|---|---|
| 1 | Appearance 画面の構造 | **部分確定**（UI 構造=確定・適用経路の一部が未確定） | race_sex_menu.xml（テンプレート/スライダー）＋ s77 §1.5 モーフ一覧 [確定] |
| 2 | 種族表示名 | **確定** | ESM RACE FULL → `RaceData.fullName`（実装済）＋ 原本ハンドラ FUN_005CDA30 の TESRace 走査 [確定] |
| 3 | 属性 / スキルの選択モデル | **確定** | s58 §1.2/§1.6/§1.7 [確定] ＋ class_menu.xml ＋ 正典 AV テーブル（21 スキル） |
| 4 | NAME 入力の保存先 | **UI 側=確定 / エンジン保存側=未確定**（→ #123 で RE 依頼） | race_sex_menu.xml `race_name` id=10 ／ EXE に文字列なし・プローブ失敗 |

---

## 1. Appearance 画面 [部分確定]

### 1.1 原本 UI = RaceSexMenu(0x40C) の中 [確定]

- 初期化 `FUN_005CA010` が `Data\Menus\CharGen\race_sex_menu.xml` を読込（v62_v59_residuals.md L101 [確定]）。
- 実物 XML（BSA 抽出物）: **`D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\chargen\race_sex_menu.xml`**（705 行）。
- 構成要素 [確定]:
  - `<text name="race_name"> <id>10</id> <target>true</target>` — 名前入力（§4 参照）
  - `race_button_done` `<id>90</id>`・`user5`: **1=Done / 2=Back**
  - `race_player_grab` — 3D プレビューのドラッグ回転領域
  - `user0` — **現在表示中の pane/page**（`race_template_pane` が `<eq src="RaceSexMenu" trait="user0"/>` でページ切替）
  - テンプレート: pane(100) / button(101) / toggle(102) / **slider(103)** / text(104)
- スライダーの trait 意味 [確定・XML コメント原文]:
  - `user0`=現在値、`user1`=開始値（変更検出用）、`user2`=ラベル、`user6`=マーカ ID、
    **`user7`=「スライド対象の editor id」**（＝ESM 側フォーム/配列への手がかり）、`user8`=geometry/texture フラグ、`user10`=変更フラグ、`user13`=有効/無効
  - bar: `user1`=最小 0、`user2`=最大 1、step=0.04(Xbox)/0.01、jump=0.1

### 1.2 編集可能モーフパラメータ一覧 [確定 — s77 §1.5（文字列 0xA62xxx-0xA63xxx）]

| グループ | 個数 | 例 |
|---|---|---|
| 頭部全体 | 5 | head small/large, tall/short, tilt fwd/back, sellion/nose ratio, nose/chin ratio |
| 目 | 12 | Eyesdown, Eyessmall, Eyestilt, Eyestogether, Eyeliner, Eyeshadow, Eyesdark, Eyeswhites … |
| 眉 | 7 | Eyebrowsdark/low/thick/very/lower/outer/uppert |
| 鼻 | 15 | Nosebridgeshallow/short, Nosedown, Nosenostriltilt/small/wide, Nosesellion* … |
| 口 | 10 | Mouthdrawn/happy/high/lips*/protruding/tilt/underbite/Chin |
| 髭 | 6 | Beardflushed/light/cheeks/circle/goatee/moustache |
| 髪色 | 16×2 系統 | HairColor0..15 / HairColor00..15（INI 名・0xA638E5-0xA63AAD） |

- FaceGen 周辺 [確定]: ファイル形式（s77 §1.2）、INI 設定（§1.3）、NiNode パーツ階層（§1.4）、エラーデバッグ文字列（§1.6）。

### 1.3 未確定（本論点の残り）

1. `user0`（page）の値 → ページ構成の対応表（Race ページ / Face ページ群の内訳と並び順）。
2. レース / 性別ごとに表示されるスライダー集合の決定規則。
3. スライダーの INI 名 → FaceGen モーフ適用の対応の一部（s77 §1.3/§1.4 で骨格は確定、接続細部が残る）。

---

## 2. 種族表示名 [確定]

- **ESM 側（実装済み）**: `RACE` レコードの `FULL` → `RaceData.fullName`（`app/src/main/cpp/assets/esm_reader.h` L692-697）。DESC・スキルボーナス 8 件・属性ボーナス 7 件・ボイス/モデルパス・种族スペルも同構造体に在り。
- **原本のリスト構築 [確定]**: レース選択ハンドラ `FUN_005CDA30` が **TESRace リスト `DAT_00B33A98+0x44` を走査**（v62_v59_residuals.md L101）。選択レースの説明はメニュー変数 `0xFB4`（`sRaceDescription`）、レース固有 DESC は vtable `+0x10`（キー "DESC"）で取得。
- **ポート側**: `loadRacesFromESM()`（ui_character_creation.cpp L583-600）が `getAllRaces()` → `availableRaces_`、表示は `availableRaces_[i].fullName`（L364）。ESM 未読込時は "No races available" 表示。
- 残りの小点 [未確認]: 原本のリスト順（ESM 登録順か別順か）。

---

## 3. 属性 / スキルの選択モデル [確定]

### 3.1 フロー [確定 — s58 §1.1/§1.2]

```
Title MainMenu(0x414) New
  → FUN_005B5DF0 が player+0x7F8 = 1（新規フラグ）
  → ワールドロード FUN_0066EAF0 末尾で FUN_0057B7E0()
  → RaceSexMenu(0x40C) 生成（race_sex_menu.xml）
  → ClassMenu(0x406) ウィザード FUN_005976B0（this[0x16]=step で分岐）
      step1 → 0x5D6390(mode 2)  特化 + 得意属性のリスト
      step2 → 0x5D6390(mode 1)  得意属性（+0x44 = 2 件）
      step3 → 0x5D6390(mode 0)  メジャースキル（+0x44 = 7 件）
      step4 → 0x5DCF20  TextEditMenu(0x41B) でカスタムクラス名入力
      step5 → 0x579C10(cb=0x5974E0) 確認ダイアログ
```

- SkillsMenu(0x408) 表示関数 `0x5D6390` の mode 分岐 [確定]: 0=メジャースキル(7) / 1=得意属性(2) / 2=属性・特化リスト / 3=対話的配分画面（ShowBirthSignMenu 経路）。
- メニュー XML 実物: `menus\chargen\class_menu.xml`（specialization / favored attributes×2 / major skills バッファ32 を内訳表示）、`menus\chargen\skills_menu.xml`。

### 3.2 初期値の計算 [確定 — s58 §1.7、実行体 0x5222D0]

- **種族 / クラス変更のたびに全再計算**（呼出元 RaceSex 初期化 0x5C29E6/0x5C2AC0、ClassMenu 0x596AE5、SetClass 内 0x50C77C ほか）。
- 属性: 種族ベース（race+0x50.. の (base,bonus) ×7）＋ クラス一次属性ボーナス `fAttributeClassPrimaryBonus`=5.0 ＋ 二次 `fAttributeClassSecondaryBonus`=5.0 ＋ メジャースキル支配属性ボーナス → **clamp 100** → `player+0x88`。
- スキル: **21 回ループ** — メジャー `base + 25.0`、非メジャー `base × 0.1 + 5.0`、特化一致で加算 → clamp 100 → `byte[player + 0xEC + skillLocal]`。

### 3.3 正典スキル一覧（21 件） [確定 — RE_EXE_REPORT §46.1（0x00B0A1A8・73 要素）]

`0x0c..0x20` = **Armorer, Athletics, Blade, Block, Blunt, HandToHand, HeavyArmor, Alchemy, Alteration, Conjuration, Destruction, Illusion, Mysticism, Restoration, Acrobatics, LightArmor, Marksman, Mercantile, Security, Sneak, Speechcraft`（s58 の byte[21]・21 回ループと一致）。

### 3.4 ポートとの差分 [事実]

- ポートは**タブ型**（Name/Race/Attributes/Skills/Appearance の自由配分）であり、原本は **RaceSex → ClassMenu ウィザード → 配分 → 確認**のステップマシン。構造が異なる。
- **スキル一覧の不備（新規発見）**: `ui_character_creation.h` L89-95 の `SKILL_NAMES` は `array<const char*, 20>` で、
  1. 重複: `"Marksman"` が 2 本（index2 と 17）
  2. 正典に存在しない名: `"Unarmored"`（Morrowind 系）、`"Armor"`（"Armorer" の誤記と推定）
  3. **欠落 4 件: Armorer / Mercantile / Security / Speechcraft**
  4. 表示・処理ループが全て `i < 20`（L52/300/470/572）＝正典 21 件に対し 1 件不足
  - AV 定数コメント（npc.h "12-32 the skills"）は 21 件前提で、`SKILL_NAMES` と矛盾。

---

## 4. NAME 入力の保存先 [UI=確定 / エンジン保存=未確定 → #123]

### 4.1 UI 側 [確定]

- 入力欄は **RaceSexMenu 内の `<text name="race_name"> <id>10</id> <target>true</target>`**（race_sex_menu.xml L47-66）＝最初の chargen 画面で入力する。中央寄せ・wrapwidth 425・clicksound 付き。
- `TextEditMenu(0x41B)` は**カスタムクラス名専用**（s58 §1.2 step4 [確定]）で、プレイヤー名には使わない。
- EXE に文字列 `"race_name"` は**不在**（`race_sex_menu` / `race_template_{button,pane,slider,text,toggle}` のみ）→ C++ は**要素 id=10** で取得と推定 [推定]。

### 4.2 エンジン保存側 [未確定 — #123 で RE レーンへ依頼済]

- プローブ: `0x5C0000-0x5D0000` の `push imm8 (6A 0A)` 走査 → 偽陽性 2 件のみ（0x5CBB5C / 0x5CC575 とも無関係）＝コミット経路未特定。
- 保存側の手がかり: セーブはプレイヤー名を**フォーム名 getter `0x4DA2A0`**（汎用・244 呼び出し＝フォーム名系統と判明）経由で `0x464C04` 以降に書出（s58_save.md L107）。`PlayerCharacter` のフィールド直書きは未発見。
- 依頼内容（#123）: RaceSexMenu vtable（RTTI `.?AVRaceSexMenu@@`＝b07_vtable_index.md）→ id=10 ハンドラ特定 → 保存先裁定 → Done(id90) のコミット経路。成果物 v352+。

### 4.3 ポート側 [事実]

- `playerName_`（Name タブ）→ `createdCharacter_.name`（`CharacterStatus.name`・npc.h L25 "character creation identity … player-state handoff"）→ プレイヤー状態へ handoff。既定 "Adventurer"。

---

## 5. 資料の所在（次回以降の再利用）

| 資料 | パス |
|---|---|
| 原本メニュー XML（BSA 抽出・104 ファイル） | `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\`（`chargen\` = race_sex/class/skills/birthsign、`dialog\texteditmenu.xml`） |
| 対象 EXE | `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416） |
| ESM | リポジトリ `oblivion_esm_pull\Oblivion.esm`、`D:\Cargo\Other\Oblivion\Oblivion_Steam版\Data\Oblivion.esm` |
| RE 静的解析 | `tmp\re\s58_chargen.md`（40KB・本論点の骨格）、`s77_facegen_faction.md`（FaceGen/モーフ）、`v62_v59_residuals.md`（RaceSexMenu ハンドラ） |
| 再実行ツール | `tmp\re\re_util.py`（`disasm_text` / `callers_text` / `cstr` / `save_text` …） |

## 6. 残件

1. **#123**（NAME 保存先のエンジン側特定・RE レーン）— #101 の材料に到達次第、本メモ §4.2 を更新。
2. Appearance の page（user0）対応表とレース別スライダー集合（§1.3）— 必要なら #97 系の継続 or RE。
3. 生原本の `Data\Menus` ディレクトリはどのインストールにも不在（BSA 内に格納）→ 抽出物を正として扱う。
4. ポートの `SKILL_NAMES` 不備（§3.4）— 実装タスクへ分離推奨（#96/#99 ではなく仕様確定 #101 の材料としても提示）。
