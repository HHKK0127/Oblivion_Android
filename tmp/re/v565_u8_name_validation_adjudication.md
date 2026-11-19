# v565 — U8 裁定書: v370 と v561 の矛盾解消（`race_button_done`(id=90) の空名時挙動）（ai-todo #161）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v565_1 / v565_3 / v565_4**、全 EXIT=0）。
参照: `tmp/re/v370_name_validation_ruling.md`（#144）／`tmp/re/v561_u5_done_button_ruling.md`（#161）
判定: `[確定(命令)]` / `[確定負]`

---

## 1. 裁定: **v370 正 / v561 誤**（矛盾はフラグ極性の逆読みに起因）

| 問い | 判定 | 結論 |
|---|---|---|
| **(a)** 空/全空白名で Done → `0x5CCBB7` に到達するか、`'%s %s?'`（`sConfirmChoice1`）経路へ進むか | **[確定(命令)]** | **`0x5CCBB7` に到達する**（＝エラー）。`'%s %s?'` 確認文経路には**進まない**。 |
| **(b)** 両方が正しいなら分岐条件 | **[確定(命令)]** | **2 条件のみ**: ①`strcmp(name, [0xB39440] = "Enter character name.") == 0` ②**スペース以外を1文字も含まない**（flag=1 残存）。**どちらかが成立 → エラー `0x5CCBB7`**。ページ依存でも `[0xB39440]` の初期値依存でもない。 |
| **(c)** `sNoNameCharacter` はどの経路から出るか | **[確定(命令)]** | **`0x5CCBB7` の1箇所のみ**（`[0xB39438]` の .text 参照は登録 `0x9F7B1A`／atexit thunk `0xA22F91`／**呼出 `0x5CCBBE`** の 3 件）。 |
| **(d)** 移植 `hasConfirmableName()`（空/全空白 → false → `onNameRejected`）は原版準拠か | **[確定(命令)]** | **正しい**。移植は v370 を根拠に実装しており原版挙動と一致。**v561 の該当記述は [確定負] 訂正**。 |

---

## 2. 証拠（probe v565_1 / v565_3 / v565_4）

### 2.1 検証バンド `0x5CCA77`（Done=90 の Main ページ経路・head `0x5CC200`）

```
5CCA77  ecx=[esi+0x30] (race_name tile) ; push 0xFDE
5CCA7F  mov byte [esp+0x16], 1           ← ★flag 初期 = 1（push 中のため基準 esp+0x12）
5CCA84  xor edi, edi
5CCA86  call 0x588C10                    ← 文字列 trait 0xFDE（名前）取得
5CCA90..5CCA99  strlen(name)
5CCA9B  je 0x5CCADF                      ← ★長さ 0 → スキャンループを飛ばす
ループ 0x5CCAA0..0x5CCADD:
  5CCAAD  cmp byte [eax+edi], 0x20       ← ' ' か
  5CCAB1  je 0x5CCAB7                    ← スペース → 保存スキップ
  5CCAB3  mov byte [esp+0x12], bl        ← ★非スペース発見 → flag = 0（bl=0）
5CCADF  edi=[0xB39440] ; GetTrait(0xFDE) ; strcmp(name, edi)
5CCAFC  je 0x5CCBB7                      ← ★①プレースホルダ一致 → エラー
5CCB04  cmp byte [esp+0x12], bl(0)
5CCB08  jne 0x5CCBB7                     ← ★②flag=1（空/全空白） → エラー
5CCB0E..  成功: 母音判定 → [0xB38660]/[0xB38668]
5CCB76  push '%s %s?' (0xA6B4D8) ; call 0x402E30  (sprintf)
5CCB9C  call 0x579C10(text, callback=0x5C2BA0, 1, [0xB38D00], [0xB38CF8], 0)
5CCBAF  add esp, 0x2c
5CCBB2  jmp 0x5CD9C5                     ← ★成功 tail（生バイト e9 0e 0e 00 00 で実測）
0x5CCBB7  mov eax,[0xB38CF0] ; mov ecx,[0xB39438]
5CCBC8  call 0x579C10                    ← ★arg1 = [0xB39438] = sNoNameCharacter / arg2 = 0（callback 無し）
5CCBCD  add esp, 0x14 ; SEH 復元 ; ret 8  ← ★成功 tail 0x5CD9C5 を経由しない
```

**フラグのスロット同一性**: 初期化 `5CCA7F` は `push 0xFDE`（esp−4）直後の `[esp+0x16]`、ループ保存 `5CCAB3` は `call` 直後（esp 復元済み）の `[esp+0x12]` ＝ **同一スロット（基準 esp+0x12）**。`bl = 0` は Done パスの `0x5CC28F: xor ebx,ebx` より [確定(命令)]。
→ **flag = 1 は「スペース以外が1文字も無い（空 or 全空白）」**。`jne 0x5CCBB7` は flag≠0 ＝**空白のみ**を拾う。

### 2.2 文字列の正体（静的登録で実測）

```
0x9F7B10  push 0xA63AD0  ; 'You must first enter a valid name.'
0x9F7B15  push 0xA63ABC  ; 'sNoNameCharacter'
0x9F7B1A  mov ecx, 0xB39438 ; call 0x419A70

0x9F7B30  push 0xA63B00  ; 'Enter character name.'
0x9F7B35  push 0xA63AF4  ; 'sEnterName'
0x9F7B3A  mov ecx, 0xB39440 ; call 0x419A70
```

| グローバル | GMST | **value（`[g]+0`）** |
|---|---|---|
| `[0xB39438]` | `sNoNameCharacter` | **"You must first enter a valid name."** |
| `[0xB39440]` | `sEnterName` | **"Enter character name."**（初期表示＝プレースホルダ） |

`0x419A70` の格納は `[esi] = arg2(value)` / `[esi+4] = arg1(name)`（v370_10 の実測どおり）。
→ **`mov edi,[0xB39440]` は value = "Enter character name." を指す**。v352 の「既存名照合」解釈は引き続き訂正（v370 が正）。

### 2.3 成功 / 失敗の帰結の違い

| | 成功（実名あり） | 失敗（空・全空白・プレースホルダ） |
|---|---|---|
| メッセージ | `'%s %s?'` を sprintf → `0x579C10`（**callback `0x5C2BA0` 付き**） | `0x579C10(sNoNameCharacter, **callback=0**)` |
| 戻り | `add esp,0x2c` → **`jmp 0x5CD9C5`**（成功 tail） | `add esp,0x14` → SEH 復元 → **`ret 8`（tail 不経由）** |
| 帰結 | 画面遷移（`vfunc+0x154`）へ | **Done ブロック（画面残留）** |

**生バイト実測**: `bytes_at(0x5CCBA8,16)` = `ff ff e8 71 53 e3 ff 83 c4 2c e9 0e 0e 00 00 a1`
→ `83 c4 2c`（`add esp,0x2c` @0x5CCBAF）＋ **`e9 0e 0e 00 00`（`jmp 0x5CD9C5` @0x5CCBB2）** ＋ `a1`（エラーブロック開始 @0x5CCBB7）。

---

## 3. v561 が誤った箇所の特定

| v561 の記述 | 実命令 | 裁定 |
|---|---|---|
| 「`cmp [esp+0x12], bl; jne 0x5CCBB7` ＝ **実名が入っていれば確認へ**」 | flag は初期1／非スペースで0 → **flag=1 は空白のみ** | **[確定負] 極性を逆読み**（`jne` は空白のみ→エラー） |
| 「**エラー文字列キーも無い**」 | `sNoNameCharacter` が `0x5CCBB7` から呼ばれる | **[確定負] エラー文字列は存在する** |
| 「空名でも確認ダイアログへ到達する」 | 空名は `je`/`jne` の2本で `0x5CCBB7` へ | **[確定負]** |
| 「id=90 は確認ダイアログを出す」 | **有効名の場合のみ** `'%s %s?'` を出す | **[部分確定] 条件付きで正しい** |
| 「`<disabled>` は無い」「種族未選択ガードは無い」「確認表示は `FUN_00579C10`」 | いずれも実命令と一致 | **v561 が正しい（維持）** |

---

## 4. (d) 移植実装への判定

移植 `renderer.cpp:986-998` の `sNoNameCharacter` ダイアログと `ui_character_creation.cpp:694 hasConfirmableName()`:

* **空 or 全空白 → `false` → `onNameRejected`（`sNoNameCharacter` 表示・遷移しない）＝原版と一致 [確定(命令)]**
* **移植が足すべき第3条件**: 名前 == プレースホルダ `"Enter character name."` も拒否する（`strcmp` 分岐 `0x5CCAFC`）。初期値をプレースホルダ文字列にしているなら必須。
* **v561 の「エラー文字列なし」「空名でも確認ダイアログ」は移植に使わないこと。**

---

## 5. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-565-1 | `0x579C10` の modality（成功側が Yes/No 待ちか即時か）＝v370 V-370-1 継続 | 中 |
| V-565-2 | `[0xB38660]`/`[0xB38668]` の実文字列（母音別テンプレート）と `[0xB38CF0]`/`[0xB38CF8]`/`[0xB38D00]` の正体 | 低 |
| V-565-3 | コールバック `0x5C2BA0` の中身（成功時の遷移先） | 中 |
