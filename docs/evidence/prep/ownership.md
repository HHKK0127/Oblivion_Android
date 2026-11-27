# chargen 未達 3 項目のレーン所有判定

対象: **#181 名前入力 / V-631-1b Eyes トグル / V-664-2・V-664-3 ATTR・health**。
実装はしない。触るファイルを列挙し、Panel レーン所有（`race_sex_panel*` /
`ui_toggle*` / `chargen_commit*`）に該当するかを判定する。

## Panel レーン所有ファイル（実在確認済み・2026-11-27）

| ファイル | 所有 |
|---|---|
| `app/src/main/cpp/ui/race_sex_panel.h` | **Panel** |
| `app/src/main/cpp/ui/race_sex_panel.cpp` | **Panel** |
| `app/src/main/cpp/ui/race_sex_panel_instantiator.cpp` | **Panel** |
| `app/src/main/cpp/ui/ui_toggle.h` | **Panel** |
| `app/src/main/cpp/ui/ui_toggle.cpp` | **Panel** |
| `app/src/main/cpp/game/chargen_commit.h` | **Panel** |
| `app/src/main/cpp/game/chargen_commit.cpp` | **Panel** |
| `app/src/main/cpp/tests/race_sex_panel_tests.h` | **Panel** |
| `app/src/main/cpp/tests/race_sex_panel_tests.cpp` | **Panel** |

## 項目別に触るファイル

| ファイル | 所有 | #181 | Eyes | V-664-2/3 | 備考 |
|---|---|---|---|---|---|
| `ui/race_sex_panel.h` | Panel | ○ | ○ | | `setPlayerName`/`player_name_`、`handleClick`、トグル状態 |
| `ui/race_sex_panel.cpp` | Panel | ○ | ○ | | `handleClick` 204/207 は false、`validateName` |
| `ui/race_sex_panel_instantiator.cpp` | Panel | ○ | ○ | | `applyLayout`（名前 widget / トグル） |
| `ui/ui_toggle.h` / `.cpp` | Panel | | ○ | | `toggleChecked()` の呼び出し元が無い |
| `game/chargen_commit.h` / `.cpp` | Panel | ○ | | ○ | `CommitInput.name` / `raceFormID` |
| `engine/renderer.cpp` | **共用** | ○ | ○ | ○ | 下の「関数名」参照 |
| `engine/renderer.h` | **共用** | ○ | | | `isCharacterNameInputActive()` 宣言 |
| `jni_bridge.cpp` | 共用 | ○ | | | JNI は実装済み（変更不要の見込み） |
| `app/src/main/java/.../CharacterNameBridge.kt` | Java | ○ | | | **実装済み**（不可視 EditText 方式） |
| `app/src/main/java/.../MainActivity.kt` | Java | ○ | | | bridge の start/stop/refresh |
| `app/src/main/java/.../GameRenderer.kt` | Java | ○ | | | native 宣言（実装済み） |
| `assets/esm_reader.h` | 共用 | | | ○ | `struct RaceData` |
| `assets/esm_reader.cpp` | 共用 | | | ○ | `decodeRace`（ATTR 未パース） |
| `game/player.h` / `.cpp` | 共用 | | | ○ | `applyRace` / `RaceBonus` / `maxHealth` |
| `game/npc_manager.h` / `.cpp` | 共用 | | | ○ | `applyRaceToPlayer`（`RaceData`→`RaceBonus`） |
| `docs/evidence/golden/chargen-*` | **Panel と同一レーン候補** | ○ | ○ | | 本準備で新設。golden は chargen の計測器の基準 |
| `docs/smoke-tests/manual-device.md` | 共用 | ○ | ○ | ○ | 実機手順（chargen を通す） |
| `docs/smoke-tests/newgame.sh` | 共用 | ○ | ○ | ○ | 既定経路 smoke（chargen まで） |

## `renderer.cpp` の chargen 関数（共用ファイルなので関数名まで）

| 関数 | 行（2026-11-27 時点） | 触る項目 | 内容 |
|---|---|---|---|
| `Renderer::setChargenUi(ChargenUi)` | 9067 | #181 / Eyes / V-664 | Panel 生成・`seedChargenRaces`・名前の seed・`onCommit` |
| `Renderer::seedChargenRaces()` | 3433 | #181 / V-664 | 種族一覧の供給。空なら `gameDataMissing_` でダイアログ |
| `Renderer::onGameDataLoaded()` | 3467 | V-664 | ESM 由来システムの再読込 |
| `Renderer::isCharacterNameInputActive()` | 9062 | #181 | **`isLegacyChargenActive() && ...` で Panel を除外** |
| `Renderer::setCharacterName(const std::string&)` | 9164 | #181 | **Legacy の `characterCreationUI` にのみ書く** |
| `Renderer::onTouchEvent` の Panel 分岐 | 8368-8396 | Eyes | `hitTestScreen`→`handleClick`。トグル反転を呼ばない |
| `Renderer::startGame()` | 8988 | V-664 | `showTitleScreen=false`、装備・クエスト付与 |
| `Renderer::cmdChargenPanel`（console） | `ui/game_console.cpp` 経由 | #181 / Eyes | Legacy↔Panel 切替 |

## 判定

- **3 項目とも Panel 所有ファイルに触る必要がある**。
  - #181: `race_sex_panel.*`（名前の受け皿）＋ `renderer.cpp` の 2 関数
  - Eyes: `ui_toggle.*` ＋ `race_sex_panel.*`（`handleClick`）＋ `renderer.cpp` の Panel 分岐
  - V-664-2/3: `chargen_commit.*`（Panel 所有）＋ 共用の `player` / `npc_manager` / `esm_reader`
- **所有ファイルの変更が必要になった時点で宣言して止まる**（本準備では実装しない）。

## 競合（重要）

1. **`assets/esm_reader.cpp` は #186 の作業ファイル**（未コミットの変更あり）。
   V-664-2/3 が触るのは `decodeRace`（`esm_reader.cpp:1810`）で、#186 の変更は
   `open` / `parseFromMemory`（同ファイル 221-238 付近）と**別関数**。同一ファイル
   だが関数は重ならない。それでも「#186 のファイルに触らない」制約下では
   **V-664-2/3 の `decodeRace` 実装は #186 の landing 後**に行うのが安全。
2. `renderer.cpp` は #186 でも変更した共用ファイル。3 項目はいずれも
   `renderer.cpp` の chargen 関数（`setChargenUi` / `isCharacterNameInputActive` /
   `setCharacterName` / Panel タッチ分岐）に触る。**#186 の変更は `loadBSAArchives`
   系のみ**で関数は重ならない。
3. `docs/evidence/golden/chargen-*` は本準備で新設する。既存の
   `newgame-no-esm-prefix.json`（世界の golden）とは**別ファイル**にする（世界の
   ゴールデンと条件が違うため混ぜない）。

## 未確認

- Java 側（`CharacterNameBridge.kt` / `MainActivity.kt`）が Panel でもそのまま使えるか
  は、native の 2 関数（`isCharacterNameInputActive` / `setCharacterName`）を
  Panel に広げた後の実機確認が要る（`未確認`）。
- `docs/smoke-tests/newgame.sh` を chargen 固有の条件で使えるかは未確認。
