# v585 — V-561-2 クローズ: RaceSexMenu の既定名 `[0xB39440]` と確認文 `[0xB39438]` の実体（ai-todo #161 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v561b_1〜v561b_3**・3本 全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v561_u5_done_button_ruling.md`（V-561-2 の起票元）／`tmp/re/v570_racenotify_dispatch_ruling.md`（name 経路の確定）
判定: `[確定(命令)]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| `[0xB39440]`（既定名）の実体 | **[確定(命令)]** | **`sEnterName` = `"Enter character name."`**（登録 `0x9F7B3B`・`push "Enter character name."; push "sEnterName"; mov ecx,0xB39440`）。 |
| `[0xB39438]`（確認文テンプレート）の実体 | **[確定(命令)]** | **`sNoNameCharacter` = `"You must first enter a valid name."`**（登録 `0x9F7B1B`）。 |
| 両者の使われ方 | **[確定(命令)]** | **`0x5CC200`（notify）内**: `0x5CCAE2: mov edi,[0xB39440]` → `push 0xFDE`（race_name の string trait）＋ `0x588C10`（GetString）→ **`0x98539F`（`__mbscmp`）で入力名と既定名 `sEnterName` を比較**（＝「既定のまま＝未入力」判定）。`0x5CCBBE` で **`[0xB39438]`＝`sNoNameCharacter`** を（確認/警告に）使用。 |
| 同族の同定（副次） | **[確定(命令)]** | `[0xB39448]`=`sResetFace`"Reset face?"／`[0xB39450]`=`sRandomizeFace`"Randomize face?"（v570 の Random/Reset ダイアログと一致）／`[0xB39458]`=`sJournalTitleActive`"Active Quest"（別系統 `0x5B8FC0`）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
静的登録バンド 0x9F7B1B: push "You must first enter a valid name." ; push "sNoNameCharacter" ; mov ecx,0xB39438
静的登録バンド 0x9F7B3B: push "Enter character name."             ; push "sEnterName"        ; mov ecx,0xB39440
静的登録バンド 0x9F7B5B: push "Reset face?"                       ; push "sResetFace"        ; mov ecx,0xB39448
静的登録バンド 0x9F7B7B: push "Randomize face?"                   ; push "sRandomizeFace"    ; mov ecx,0xB39450

0x5CC200 (notify) 内:
  5CCADF  mov ecx, [esi+0x30]        ; race_name tile
  5CCAE2  mov edi, [0xB39440]        ; = sEnterName（既定名）
  5CCAE8  push 0xFDE                 ; race_name の string trait
  5CCAED  call 0x588C10              ; GetString(tile, 0xFDE)（v556 の文字列系）
  5CCAF2  push edi
  5CCAF3  push eax
  5CCAF4  call 0x98539F              ; __mbscmp(入力名, sEnterName)  → 既定名と一致＝未入力
  5CCBBC  mov ecx, [0xB39438]        ; = sNoNameCharacter（警告文）
```

* `[0xB39440]` の `.text` 参照は2件（`0x5CC200` の `0x5CCAE4` と init `0x5CA010` の `0x5CBC62`）＝**RaceSexMenu 専用の既定名定数**。
* `[0xB39438]` の参照は1件（`0x5CC200` の `0x5CCBBE`）。
* → v561 §2.3 の「名前チェック → 確認」の**「名前チェック」の実体＝既定名 `sEnterName` との `__mbscmp`** であることを確定（v561 の「空名でも確認へ到達する」記述と整合＝**空判定ではなく既定名一致判定**）。

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-585b-1 | `0x5CCBBE` 以降（`sNoNameCharacter` 使用箇所）の分岐＝「無効名」時の動作（確認ダイアログを出すか拒否ループか） | 低 |
| V-585b-2 | `0x5CBC62`（init `0x5CA010` 内の `sEnterName` 参照）の用途（初期名の設定） | 低 |

---

## 4. probe 一覧

`v561b_1`（静的登録バンドの逐語で `sEnterName`/`sNoNameCharacter`/`sResetFace`/`sRandomizeFace` を同定＋各 `.text` 参照一覧）／`v561b_2`（文脈 back-decode＝land 失敗）／`v561b_3`（**head 線形 decode で `0x5CCAE2`/`0x5CCBBC` を命令境界つきで確定**＝`__mbscmp` 比較）。3本 EXIT=0。
