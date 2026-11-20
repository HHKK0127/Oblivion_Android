# v604 — 低優先残点クローズ（バンドル2）: `0x582160` 戻り値・Done 非帰還ページ・`0x579C10` modality・母音文字列・コールバック `0x5C2BA0`・`0x983754`・visible 1.0/2.0（ai-todo #96/#161 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u17_1**・EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v557_title_video_ruling.md`・`v560_u1_residuals_ruling.md`・`v561_u5_done_button_ruling.md`・`v565_u8_name_validation_adjudication.md`・`v568_bink_residuals_ruling.md`
判定: `[確定(命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 残点 | 判定 | 結果 |
|---|---|---|
| **V-560-2** `FUN_00582160(0,1)` の戻り値 | **[確定(命令)]** | `0x582160(create, keep)` は **`InterfaceManager` singleton `[0xB3A6E0]` の getter**。`arg1`（`[esp+0x14]`）が 0 でなく NULL なら**生成**（`0x401F00(0x134)`→`0x5802B0` ctor→`0x581CC0` init）。戻り値は **singleton ポインタ**（`eax=[0xB3A6E0]`）。→ `(0,1)` の第2引数（keep）は生成条件の制御で、**戻り値は常に singleton**。前景判定の前段は `[singleton+0x1c]`（メニュースタック）。 |
| **V-561-3** Done の非 Shape/Tone/General(Face) ページ | **[確定(命令)]** | `0x5CC8BA`（全ブロック不一致フォールスルー）に落ちるのは **Race／Hair／Face／Hair Color** ページ等。ここでは `0xFA8` 比較で一致要素がなければ **`0x5CCBF4`（id98 判定）→ act なし epilogue**。→ **これらページの Done は「次ページへ進む」ではなく「フォールスルー（実質何もしない）」**。Shape/Tone/General(Face) のみ明示的に Face/Shape へ戻す（v567 の表と整合）。 |
| **V-565-1** `0x579C10` の modality | **[確定(命令)]** | `0x579C10` は**ダイアログ登録関数**（`0x5BC8B0` を呼ぶ汎用登録・v592 で 5 引数と確定）。**成功 tail は jmp（無条件）で、Yes/No 待ちではなくコールバック登録型**。確認文は登録後に非同期で表示される（`0x579C10` 自体はブロックしない）。 |
| **V-565-2** 母音別文字列 | **[確定(命令)]** | `0xB38660`/`0xB38668`（`0x5CCB64`/`0x5CCB6B` で選択される a/an 冠詞テンプレート）および `0xB38CF0`/`0xB38CF8`/`0xB38D00`（確認文テンプレート）は **`.data` の登録スロット（`{value,name}` 対の value 側ポインタ）**であり、実文字列は **静的登録ブロック（`0x9F7xxx` 帯）の `push value; push name; mov ecx,<global>` 逐語**から取得する。本 probe では `.data` のポインタフィールドを読んだため文字列本体は別アドレス（登録ブロック）に在る。 |
| **V-565-3** コールバック `0x5C2BA0` | **[確定(命令)]** | `0x5C2BA0`＝**確認ダイアログの Yes コールバック**。`FindMenu(0x40C)`→`0x5898F0`→cast→**`0x578D70()`（応答バイト読取＋クリア）が `al==2`（Yes）なら `0x5C28F0` へ jmp**、否なら ret。→ **成功時の遷移先は `0x5C28F0`**（名前確定後の処理）。 |
| **V-568-2** `0x983754` の返り値 | **[部分確定]** | `0x983754` は **CRT の `_stat64i32` 系**（`___doserrno`/`__errno` を呼ぶ）＝**ファイル stat**。返り値 `eax==-1` で失敗。`0x4101F9` の AND 条件は「サイズ閾値超過＋フラグ」の判定に使われる（`0xB030BC`＝`iPreloadSizeLimit`）。**厳密な AND 条件の全形は [部分確定]**（stat 成功可否）。 |
| **V-557-5** `visible`(0xFA1) を 1.0/2.0 で書き分ける理由 | **[部分確定]** | `0x5B5CE4`-`0x5B5D52`（メインメニュー表示）で背景ムービー有無に応じ `0xFA1` を **1.0（前面にメニュー）/2.0（背景扱い）** に設定。**`0xFA1`(visible) の値域 0/1/2 は「不可視/可視（前面）/可視（背景）」の 3 状態**（`0x410425` の `[movie+0x20]==2` 前景判定と対応）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-560-2 singleton getter

```
0x582160(create,keep):  ecx-tier arg1=[esp+0x14]
  582185 mov eax,[0xB3A6E0]        ; ★ singleton
  582180 cmp byte [esp+0x14],0 ; 58218A je 0x5821D6
  58218C test eax,eax ; 58218E jne 0x5821D6
  582190 push 0x134 ; call 0x401F00 ; ... 0x5821AF call 0x5802B0 ; 0x5821CC call 0x581CC0
  5821D6 ret                        ; ★ 戻り値=[0xB3A6E0]
```

### 2.2 V-565-3 コールバック

```
0x5C2BA0: push 0x40C ; call 0x589B70 ; cast(0xB13348/B0A018 via 0x9832E6)
  5C2BD3 call 0x578D70    ; 応答バイト読取＋クリア
  5C2BD8 cmp al,2 ; 5C2BDA jne 0x5C2BE1
  5C2BDC jmp 0x5C28F0     ; ★ Yes → 名前確定処理
  5C2BE1 ret
```

### 2.3 V-561-3 非帰還ページ

```
0x5CC8BA: id と 0xFA8 比較 → 不一致で 0x5CCBF4
  5CCBF4 cmp ebx,0x62 ; jne 0x5CCC1C   ; id98 でなければ epilogue（act なし）
```

### 2.4 V-557-5 visible 3 状態

```
0x5B5CE4 mov eax,[0xB33428] ; test eax,eax ; je 0x5B5D31
0x5B5CF5 mov byte [0xB3B408],al     ; 背景ムービー有無フラグ
0x5B5D18 fld1 ; ... SetFloat(0xFA1, 1.0)   ; 前面
0x5B5D2B ...（背景時は 2.0）
0x410425 cmp dword [eax+0x20],2     ; 前景判定（2=背景）
```

---

## 3. 残点（更新後）

- **V-565-2【低】**: 母音文字列の実文字列本体（静的登録ブロックの逐語読取が必要）。
- **V-568-1【低】**: `BinkOpen` フラグ `0x2000`/`0x08000000` の SDK 名（bink.h 依存）。
- **V-568-2【低】**: `0x983754` の AND 条件全形。
- **V-562-1/2・V-566-2/3・V-569-5・V-572-1・V-573-2【低】**。
- **V-599-2・V-598-4・V-601-1【低】**: 動的領域/実用価値低（打切り継続）。

---

## 4. 検証

- probe: **u17_1（1本）EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x582160`（`[0xB3A6E0]` getter）／`0x5C2BA0`（`al==2`→`0x5C28F0`）／`0x5CC8BA`（`0x5CCBF4` id98）／`0x5B5CE4`（`0xFA1` 1.0/2.0）／`0x983754`（CRT stat）／`0xB38660`/`0xB38668`/`0xB38CF0`/`0xB38CF8`/`0xB38D00`（`.data` ポインタスロット）。
