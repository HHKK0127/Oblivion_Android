# #181 名前入力の素材（Panel chargen）

結論を先に: **方式 (a) 不可視 EditText オーバーレイは既に実装されている**
（`CharacterNameBridge.kt`）。#181 の実体は「**そのブリッジを Panel に配線する**」
ことで、方式の選択は不要。実装はしない（素材のみ）。

## 1. 既存入力 UI の実測（2026-11-27・grep）

| 実装 | 場所 | 用途 |
|---|---|---|
| `EditText` sink | `app/src/main/res/layout/activity_main.xml:23`（`@+id/console_input_sink`） | コンソールと名前入力が**共有**する不可視入力 |
| `CharacterNameBridge` | `app/src/main/java/com/example/oblivion/CharacterNameBridge.kt` | **名前入力（実装済み）** |
| `ConsoleInputBridge` | `app/src/main/java/com/example/oblivion/ConsoleInputBridge.kt` | デバッグコンソール |
| `InputMethodManager` | 両ブリッジ（`showSoftInput` / `hideSoftInputFromWindow`） | IME の表示・非表示 |
| `InputConnection` | **リポジトリ内に 0 件** | 方式 (b) は未使用 |
| `dispatchKeyEvent` | `MainActivity.kt:178` / `GameSurfaceView.kt:127` | メニューキー転送の調停 |

sink の属性（`activity_main.xml:23-33`）: `1dp x 1dp`・`alpha=0`・
`inputType=textNoSuggestions|textVisiblePassword`・`imeOptions=actionDone|flagNoFullscreen`・
`privateImeOptions="nm"`。**`maxLength` は無い**。

## 2. 方式の比較表

| 観点 | (a) 不可視 EditText オーバーレイ | (b) `InputConnection` 直叩き |
|---|---|---|
| IME 表示 | **実装済み**: `showSoftInput(input, SHOW_IMPLICIT)`＋150/400ms のリトライ（window focus レース対策、`CharacterNameBridge.kt:43,166-167`） | 未検証（実装が無い）。`InputConnection` を返す `View` が要る |
| 文字確定 | **実装済み**: `TextWatcher.afterTextChanged` → `nativeOnCharacterNameChanged`（**1 文字ごと**に native へ。原作がリアルタイム書込のため） | 未検証。`commitText` を native から呼ぶ形になり、所有権の調停を自作する必要 |
| バックスペース | **実装済み**（EditText が処理し `afterTextChanged` が差分を送る） | 未検証。`deleteSurroundingText` を自作 |
| キャンセル / 確定 | **実装済み**: DONE/GO/SEARCH/UNSPECIFIED で IME を閉じる（`handleEditorAction`, task #143）。閉じないと下部の Create/Done がキーボードに隠れて押せない | 未検証 |
| GL スレッドとの関係 | **実装済み**: ブリッジは UI スレッド。native へは `nativeOnCharacterNameChanged`（JNI）で渡す。描画は従来どおり GL スレッド | 未検証（native から UI スレッドへ投げる経路を自作） |
| 既知の落とし穴 | ①sink はコンソールと**共有** → `consoleActive()` / `seeding` で書き込みを止めないとコンソール操作が名前欄に混入する（実装済みのガード: `CharacterNameBridge.kt:88`）②`GONE` の View には IME が付かない → `VISIBLE`＋`alpha=0`（`activity_main.xml:19` のコメント）③window はリサイズしないので IME が下部を覆う | 未検証（未知の落とし穴が残る） |

→ **(a) を選ぶ**。(b) は「未検証」だらけで、選ぶ理由が無い。

## 3. Panel との差分（#181 の実体）

`renderer.cpp` の 2 関数が **Legacy 専用**になっている:

- `Renderer::isCharacterNameInputActive()`（`renderer.cpp:9062`）
  `return isLegacyChargenActive() && characterCreationUI->isNameInputActive();`
  → Panel のとき false。ブリッジは名前入力を開かない。
- `Renderer::setCharacterName(const std::string&)`（`renderer.cpp:9164`）
  `characterCreationUI->setPlayerName(name)` にのみ書く。Panel の
  `raceSexPanel->setPlayerName(name)` には書かない。

Panel 側の受け皿は**既にある**: `RaceSexPanel::setPlayerName()`（`race_sex_panel.h:350`）＋
`applyLayout()` が `name_widget_` に `player_name_` を反映（`race_sex_panel_instantiator.cpp:179-181`）。
足りないのは「誰が `player_name_` を更新するか」だけ。

JNI は実装済み（`jni_bridge.cpp:424,432`）、Java 宣言も実装済み
（`GameRenderer.kt:589,592`）。**native の 2 関数を Panel に広げれば配線は閉じる**見込み。

## 4. 名前フィールドの制約（実測）

| 項目 | 値 | 出所 |
|---|---|---|
| 最大長 | **未確認**（XML にもコードにも制限が無い。原作の上限は未計測） | — |
| 使用可能文字 | 空白（`' '`）とタブ（`'\t'`）のみ拒否。他の制限なし | `race_sex_panel.cpp:716-723` |
| 空名 | 拒否（"You must enter a name."） | 同上 |
| 既定名 | `"Adventurer"`（`player->name` が空のとき。Panel の seed も Legacy も同じ） | `renderer.cpp:9144` / `ui_character_creation.cpp:110` |
| 確定キー | `imeOptions=actionDone`。DONE/GO/SEARCH/UNSPECIFIED で IME を閉じる | `activity_main.xml:30` / `CharacterNameBridge.kt:187-190` |
| 同名判定 | `validateName` の `Unchanged`。比較先 `current_name_` は Panel 経路で**空に設定**されるため、非空名では `Unchanged` が発火しない（V-653-1 の既知の逸脱） | `renderer.cpp:9146` / `race_sex_panel.cpp:721` |

## 5. 実機での IME 確認（Legacy 経路・2026-11-27 実施）

方式 (a) の実現可能性を **Legacy 経路の実機 1 run** で確認した。手順:
デバッグコンソール（`btn_debug_toggle` → `btn_debug_console`）から `chargenpanel` を実行して
Legacy へ切り替え、NAME タブの入力状態を見る。条件は 1280x960 / 240dpi / rotation 1、
no-ESM（Legacy の NAME タブは ESM の有無に関係なく開く: `ui_character_creation.cpp:116-117`）。

結果（**PASS**）:

```
I/ConsoleInputBridge: Console command: chargenpanel
I/JNI_Bridge:         nativeExecuteConsoleCommand: chargenpanel
I/CharacterNameBridge: Character name input opened        ← ブリッジが sink を取った
dumpsys input_method: mInputShown=true                    ← IME が出ている
```

スクリーンショットでもソフトキーボードの表示を確認（`artifacts/legacy-ime/legacy.png`）。

→ **方式 (a) は実機で動く**。Panel との差分は §3 の native 2 関数のゲートだけなので、
`isCharacterNameInputActive()` / `setCharacterName()` を Panel に広げれば同じ経路が使える
見込み。Panel 配線後の IME 表示・1 文字反映・Done での閉じは実装時の検証項目。

## 未確認

- 最大長（原作の上限）
- Panel 配線後の IME 表示・1 文字反映・Done での閉じ（実装時）
- Legacy の NAME タブで 1 文字入力が `nativeOnCharacterNameChanged` 経由で
  `playerName_` に届くこと（今回は「IME が出る」までを確認。文字入力そのものは未確認）
