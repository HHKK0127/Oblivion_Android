# v561 — U5 裁定書: `race_button_done`(id=90) の未入力・未選択時の原版挙動（ai-todo #161）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe v561_1 ほか）＋ BSA 実物 `menus\chargen\race_sex_menu.xml`（21,451 B）。
判定: `[確定]` / `[確定(命令)]` / `[確定(XML+命令)]` / `[確定負]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結論 |
|---|---|---|
| ボタン無効化（disabled）か | **[確定負]** | **無効化しない。** `race_sex_menu.xml` に `<disabled>` は **0 個**（`<onlyif>` は 4 個だが id=90 には付いていない）。id=90 は常に押せる。 |
| エラー文字列キーか | **[確定負]** | **エラー表示しない。** 未入力時に出す専用文字列キーは無い（`_done`/`_return` のボタン表記のみ）。 |
| そのまま確定か | **[確定(命令)]** | **「そのまま確定」でも「拒否」でもなく、確認ダイアログを出す。** 名前が空・空白のみ・既定名のいずれでも `FUN_00579C10`（確認メッセージ）へ進む。 |
| 種族未選択の扱い | **[確定(命令)]** | **「未選択」状態が存在しない。** 種族は常に 1 つ選ばれた状態で初期化されるため、Done 側に種族の未選択ガードは無い。 |

### 1.1 要点

* id=90 の挙動は**現在ページに依存する**（§2.1）。Main ページでは「名前の妥当性を見て確認ダイアログ」、Shape/Tone/General(Face) では「Face ページへ戻る」、その他は「特定ページへ移動」。
* 空名は**拒否されない**。空名用の分岐は「確認メッセージの文面を組み立てる」ために使われる（母音で a/an を切り替える処理がある）。

---

## 2. 根拠

### 2.1 id=90 の通知は `0x5CC200`（RaceSexMenu のスロット 3）で処理される

`RaceSexMenu` vtable `0xA6D38C` の `[+0x0C]` = `0x5CC200`（v364 が「[3] init」と呼んだ関数）。この関数の先頭で id を分岐する:

```
5CC25A  ebx = arg /*通知 id*/
5CC263..5CC272  0x57D2F0 / 0x57DD90 / 0x5C30C0（共通前処理）
5CC277  cmp ebx, 0x5A        ; 90
5CC27A  jne 0x5CC8BA         ; 90 以外 → 別処理
```

### 2.2 90 の分岐（ページ判定）

```
5CC280  pane = GetPage("Main")          ; 0x5C3110(this, "Main")
5CC2B0  cur  = GetTrait([esi+4], 0xFAE) ; 現在ページ（メニュー根 tile）
5CC2C2  if (GetTrait(pane,0xFAE) == cur) → 0x5CCA77
5CC2D6  pane = GetPage("Shape")  → 一致なら 0x5CCA09
5CC32A  pane = GetPage("Tone")   → 一致なら 0x5CCA09
5CC37E  pane = GetPage("General(Face)") → 一致なら 0x5CCA09
```

* **0x5CCA09**（Shape / Tone / General(Face) のとき）: `SetTrait([esi+4], 0xFAE, GetTrait(GetPage("Face"), 0xFAE))` → **Face ページへ戻す**。
* **0x5CCA77**（Main ページのとき）: 名前チェック → 確認（§2.3）。
* **0x5CC8BA**（それ以外）: `GetPage("Random")` 系の後処理 → 共通テール `0x5CD9C5`。

### 2.3 Main ページの Done = 名前チェック＋確認ダイアログ

```
5CCA77  name = GetTrait([esi+0x30] /* race_name tile */, 0xFDE /*string*/)
5CCA8B  長さ 0 なら 0x5CCADF へ
5CCAA0  各文字が ' '(0x20) 以外なら「空白のみではない」フラグを落とす
5CCADF  strcmp(name, [0xB39440])        ; 既定名（"Bendu Olo" 系の既定値）
5CCAFC  if (== 0) → 0x5CCBB7            ; 既定名なら確認へ
5CCB04  if (!空白のみ) → 0x5CCBB7       ; 実名が入っていれば確認へ
5CCB0E  それ以外（空 or 空白のみ）: 0x5E32F0(player) の名前を取り、
        先頭文字が a/e/i/o/u/A/E/I/O/U かで冠詞を選び、確認文を組み立てる
5CCBB7  FUN_00579C10(0xB38CF0, 0xB39438, …, 1, …)   ; 確認メッセージ表示
5CCBE2  ret 8
```

→ **空名でも 0x5CCBB7（確認）に到達する**。つまり拒否ではなく「確認してから進む」。

### 2.4 XML 側

* `<image name="race_button_done">`（`:211-237`）: `<id> 90 </id>`、`<user5>` で Done/Back の表記切替（1=false=Done／2=true=Return）、`<_text_1>=_done`、`<_text_2>=_return`、`<clicksound>2</clicksound>`。
* `<disabled>` は **ファイル全体で 0 個**。`<onlyif>` は 4 個（いずれも表示条件で id=90 には無関係）。

### 2.5 「種族未選択」が起こらない理由

種族は `0x5CC200` の初期化（同一関数の先頭側）で必ず 1 件選択された状態になり、`0x5CCA77` の Done 経路に**種族の有無を見る分岐が無い**（名前の分岐のみ）。したがって原版には「種族未選択で Done を拒む」状態が存在しない。

---

## 3. 移植への示唆

1. 移植の「races が空なら Done を拒否」は**原版に無い防御**。原版は「空でも確認ダイアログを出して進む」なので、移植側のガードは**データ欠損時の独自安全策**として残すなら、その旨をコメントで明示するのが正確。
2. 原版の Done は**ページ送りボタン**でもある（Main→確認 / Shape・Tone・General(Face)→Face へ戻る）。移植でタブ UI にする場合、この「ページ依存の Done」を再現するかは設計判断。
3. 名前の空チェックは「拒否」ではなく「確認文の冠詞（a/an）選択」に使われている。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-561-1 | `0x5CC8BA` 以降（90 以外の通知）の全分岐と `GetPage("Random")`/`GetPage("Reset")` の役割 | 中 |
| V-561-2 | `[0xB39440]`（既定名）と `[0xB39438]`（確認文テンプレート）の実体 | 低 |
| V-561-3 | `0x5CCA09` に落ちる 3 ページ（Shape/Tone/General(Face)）以外で Done が「Face へ戻す」か「次ページへ進む」かの切り分け | 中 |
