# 手動チェックリスト（実機・chargen 既定 `Panel`）

既定の chargen UI（`RaceSexPanel` = `ChargenUi::Panel`）を実機で通す唯一の検証経路。
**#178（New Game の SIGABRT 修正）と既定 `Panel` の表示・タップ確認を兼ねる。**
emulator smoke は V688a park（hosted runner に hypervisor 無し）のため、このチェックが
既定経路の最後の砦。**リリース前に必須**（結果はリリースノートにも記録する）。

## 前提

- **ビルド**: debug APK（`./gradlew assembleDebug`）。`git rev-parse HEAD` の hash を記録する。
- **端末**: `adb devices` で 1 台だけ接続（複数なら `ANDROID_SERIAL=<serial>`）。
- **ログ採取**（アプリ起動**前**に開始。既定リングバッファは 256 KiB で溢れるため広げる）:
  ```
  adb logcat -G 16M
  adb logcat -c
  adb logcat -v time > smoke.log
  ```
  `smoke.log` は記録に添付する。
- **`Panel` に切替**: **既定は既に `Panel`**。`chargenpanel` は **トグル**（`Legacy`↔`Panel`）なので、
  1 回実行すると `Legacy` へ変わる。**既定のまま `Panel` であること**を画面とログ
  （`Chargen UI toggled (Legacy <-> RaceSexPanel)`）で確認する。
  - 開き方: 左上の「D」ボタン（`btn_debug_toggle`）でデバッグオーバーレイを開く →
    「Debug Console」（`btn_debug_console`）でコンソール表示 → `chargenpanel` を入力して Enter。
    （コンソールは Android ソフトキーボードで入力できる）
- **アプリ起動**: `adb shell monkey -p com.example.oblivion -c android.intent.category.LAUNCHER 1`

## 手順（各段の P/F を記入）

| # | 操作 | 期待結果 | P/F | メモ |
|---|---|---|---|---|
| 1 | タイトルで **New Game** | `Menu selection: New Game` → `Game started! Opening character creation`。**クラッシュしない**（#178） | | |
| 2 | chargen 表示 | `RaceSexPanel`（21 ページ）が描画される。名前欄に**現在名**（既定 `Player`＝`Player::name`） | | |
| 3 | Main の 3 ボタン | **Race / Face / Hair** へ遷移し、各ページが描画される | | |
| 4 | Race ページ | 種族トグルの左右タップで前後送り（両端で停止。V-654-3） | | |
| 5 | Face → Shape / Tone | Face の **Shape / Tone** ボタンで遷移 | | |
| 6 | Shape → 顔パーツ群 | Brow / Cheeks / Chin / Eyes / Forehead / Jaw / Mouth / Nose へ遷移 | | |
| 7 | Tone → 肌パーツ群 | General Skin / Beard / Eyes Tex / Eyebrows / Mouth Lips / Nose Tex へ遷移 | | |
| 8 | **Done/Return**（id 90） | 階層を 1 つ上へ戻る（Main まで遡れる） | | |
| 9 | scroll 窓 | 7 行以上のページ（例 Nose=14 項目）で **6 行の窓**＋スクロールバーが出る | | |
| 10 | **Random**（初回） | `Randomize face?`（Yes/No）。**No** は値不変・再クリックで再表示 | | |
| 11 | **Random**（Yes） | スライダーが変化する（実機乱数）。ページは Face のまま | | |
| 12 | **Random**（2 回目） | ダイアログ無しで即実行（armed） | | |
| 13 | **Reset** | `Reset face?`（Yes/No）→ Yes で全スライダーが既定へ | | |
| 14 | 名前を**空**にして Done | `You must enter a name.` の通知のみ（確定しない・パネル維持） | | |
| 15 | 名前を**空白入り**にして Done | `Names cannot contain spaces.` の通知のみ | | |
| 16 | 名前を**現名のまま** Done | `That is already your name.` の通知のみ | | |
| 17 | **有効名**で Done | `an/a <名前>?` の確認（Yes/No） | | |
| 18 | 確認で **Yes** | chargen が閉じ、ゲームへ（`setChargenUi(None)`）。名前が反映される | | |
| 19 | タイトルへ復帰 | Back などでタイトルへ戻れる | | |
| 20 | **Legacy 退避の実効性**: chargen 中に `chargenpanel` → **`Legacy`** chargen が落ちずに**名前欄が出る** → もう一度 `chargenpanel` で **`Panel`** へ戻る | `Legacy` がクラッシュせず名前欄を表示し、`Panel` に戻せる（**戻せないと「既定を `Legacy` に戻す」判定が空文化する**） | | |

## 記録（リリースごとに追記）

| 日付 | 端末 / Android | ビルド hash | 実施者 | 結果 | 既定を戻したか | smoke.log |
|---|---|---|---|---|---|---|
| | | | | | | |

- 端末機種 / Android バージョン:
- ビルド hash（`git rev-parse HEAD`）:
- APK サイズ:
- 実施者 / 日時:
- **既定を戻したか**: 未実施 / 戻した（`Legacy` へ戻したコミットの hash）。**判定で P が欠けた場合のみ**「戻した＋hash」を記入する。
- `smoke.log`（添付 or 保存先）:

## 既知の制約（失敗ではない）

- **morph/mesh 未適用（`V-654-4`）**: スライダー値は保持するが**顔メッシュに反映されない**。
  Random/Reset の値変化が**見た目に出なくても正常**（値は保持される）。
- **既定 `Panel` は実験的**: race 適用は `Player::raceFormID`（＋ skill bonus / spell）のみ。
  属性 / HP / mesh は RE 待ち（ai-todo **#179**）。
- **emulator smoke は park**: hosted runner に hypervisor が無く boot しない。このチェックが
  実機の唯一の経路。

## 判定

- 上表の **P が 1 つでも欠けたら、既定を `Legacy` に戻す**（`renderer.cpp` の New Game 起点を
  `setChargenUi(ChargenUi::Panel)` から `Legacy` へ。`chargenpanel` トグルの既定も含む）。
- すべて P なら `Panel` を既定のまま維持し、この表を記録として残す。

## 参照

- `docs/chargen_ui_spec.md` — 正典（§8 残点表、§10 chargen フロー）。
- `docs/smoke-tests/newgame.sh` — emulator 用の自動 smoke（座標はログから読む）。
- `docs/evidence/golden/README.md` — golden の 2 種と更新手順。
