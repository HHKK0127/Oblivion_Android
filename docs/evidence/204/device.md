# #204 実機確認 — chargen commit の RACE ATTR 行が gender に追従する

## 結論

**初回 commit の ATTR 行は gender に追従する（合格）**。`end` が 50 → 40 → 50（Nord の
ESM 実値どおり）。ただし **gender 単独変更の再適用（`raceChanged || genderApplied` の
バグ修正）は実機では踏めない**（下記「限界」）。その経路はホストテストのみが押さえる。

## 条件

- 端末: emulator-5554（`wm size 1280x960` / density 240 / rotation 1）
- **アプリ surface: 1280x870**（nav bar で 90px 減。`s=min(1280/1280, 870/960)=0.90625`、
  Panel origin = (150.6, 176.7)。座標は `origin + canvas*s`）
- APK: `app/build/outputs/apk/debug/app-debug.apk`（コード commit `f049ceee` 系）
- ゲームデータ: 実 `Oblivion.esm`（277504985B）
- race: **Nord**（`0x000224FD`）。実測 ESM: 男性 str=50 end=50 / 女性 str=50 end=40

## 手順

**1 プロセス 1 セッション**で 3 run（開き直し不可。下記「限界」）。各 run:
タイトル New Game → chargen → Race ページ → Race トグル右（Nord, index 1）→
（**run 2 のみ**）Gender トグル → Return → Done → ダイアログ **Yes をタップ (640,456)**
→ commit。

- ダイアログの Yes は **タップ**（`input keyevent 66` では確定しなかった）。
- 座標は実測（app surface 1280x870）。

## 結果（`adb logcat`、生ログ `artifacts/204-device/logcat.txt`）

```
11-27 20:25:15 I/NpcManager: chargen commit applied: race=0x000224FD hp=100/100 str=50 end=50 gender=0
11-27 20:26:48 I/NpcManager: chargen commit applied: race=0x000224FD hp=100/100 str=50 end=40 gender=1
11-27 20:28:14 I/NpcManager: chargen commit applied: race=0x000224FD hp=100/100 str=50 end=50 gender=0
11-27 20:26:48 I/NpcManager: chargen commit: gender 0 -> 1
11-27 20:25:15 I/NpcManager: Race applied to player: Nord (skillBonuses=6, spells=3, attrs=ATTR)
11-27 20:26:48 I/NpcManager: Race applied to player: Nord (skillBonuses=6, spells=3, attrs=ATTR)
11-27 20:28:14 I/NpcManager: Race applied to player: Nord (skillBonuses=6, spells=3, attrs=ATTR)
```

- **`end` が 50（Male）→ 40（Female）→ 50（Male）**。Nord の ESM 実値（M 50 / F 40）と一致。
- `attrs=ATTR` は `hasAttr` 経路（`decodeRace` が ATTR >= 16 で true）。
- run 1/3 は gender タップ無し（新規プロセスで既定 Male）。

## 限界（重要・混同しない）

- **gender 単独変更の再適用（`raceChanged || genderApplied`）は実機で踏めない。** commit 後に
  chargen は閉じ（`setChargenUi(None)`）、開き直しは console `chargenpanel` だが、**console は
  文字入力の JNI が無い**（`GameConsole::onKeyPress → handleConsoleKey` は特殊キー
  Enter/Backspace/Escape/矢印のみ。文字は `onTextInput` だが**それを呼ぶ JNI が存在しない**）。
  よって adb から `chargenpanel` を打てず、各 run は新規プロセスで `raceChanged=true`。
- この経路は**ホストテスト** `chargen::commit: gender-only change re-applies the ATTR row`
  （`app/src/main/cpp/tests/race_sex_panel_tests.cpp`）が押さえる。
- スクショ・生 logcat は `artifacts/204-device/`（`/artifacts/` は gitignore）。生スクショは
  Bethesda アセットを含むためコミットしない（`docs/evidence/README.md`）。
  証拠スクショ: `race-page-m1/f1/m2.png`（Nord 説明）・`dialog-*.png`（Yes/No）・
  `after-*.png`（commit 後）。
