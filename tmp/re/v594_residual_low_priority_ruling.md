# v594 — 低優先残点の一括クローズ: V-594-1（base とボタン id）・V-594-2（`0x5BCCB0`＝入力欄テキスト確定）・V-585-1（chargen 中 `[+8]`）・V-591-2（`0x5894F0`＝id trait 検索）（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v594_1**・1本 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v593_dialog_notify_earlyreturn_ruling.md`／`v590_id98_loop_survey_ruling.md`／`v584_chargen_gate_ruling.md`
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| **V-594-1** `[MessageMenu+0x60]`（base）を書くのは誰か | **[確定(命令)]** | `0x5B0000`〜`0x5CFFFF` で `[reg+0x60]` byte 書込みは **2件のみ**: **`0x5BC48A: mov byte [esi+0x60], 1`（head `0x5BC450`＝`MessageMenu` ctor/init）** と `0x5BCB3F: mov byte [ebp+0x60], al`（`0x5BC8B0` ダイアログエントリ生成＝**別オブジェクト**）。→ **MessageMenu の base は `0x5BC450` で `1` に初期化** → **`al = 1 + (id−4)`** → **id 4 → `al=1`、id 5 → `al=2`**。 |
| **V-594-1** `al==1`/`al==2` の Yes/No | **[確定(命令)]（条件付）** | **`al==2`（＝id 5 のボタン）が肯定（Yes）**（Random cb が `al==2` で実行＝v592）。→ **ボタン id 4＝否定（No 側）、id 5＝肯定（Yes 側）**。 |
| **V-594-2** `0x5BCCB0`（`id==2` 分岐）の役割 | **[確定(命令)]** | **`0xFAE`（`[+0x7e]` から取得した**現在のページ/フォーム値**）と `0xFDE`（`race_name` テキスト）を取得 → `0x4028D0`（文字列 Set）→ **文字列を1文字ずつ走査し `al==2` を検出すると `bl=1`** を立てる処理**。＝**「テキスト入力欄の内容から id2 相当の確定を行う」入力欄系ハンドラ**（MessageMenu の汎用確定の一形態）。 |
| **V-585-1** chargen 中に `[+8]` を書くサイトはあるか | **[確定(命令)（negative）]** | **`push 0x40C`（RaceSexMenu）を持つ 17 head のいずれにも InterfaceManager `[+8]`（byte）書込みは存在しない**。→ **chargen（RaceSexMenu）フローは `[+8]` を書かない**＝**v584 の「HUD 生成は `[+8]==1` のみ」と合わせ、chargen 中は `[+8]` が 1 にならないことが命令的に確認**（V-585-1 クローズ）。 |
| **V-591-2** `0x581390`／`0x5894F0` の identity | **[確定(命令)]** | **`0x5894F0`: `esi=tile`; `0x588BD0(esi, 0xFA8)`（id trait 取得）→ `fild` 引数と `fucompp` 比較 → 一致で `eax=esi`（tile を返す）**＝**「id trait が指定値と一致するタイルを探す」検索ヘルパ**（id98 の解決候補だが notify 送信ではない）。**`0x581390`** は `[ebp+0x58]` のフラグ操作＋`0x959BC0`（BSS 初期化）＝**別系統（描画/リスト系）**。 |
| **V-592-2** `0x441920` | **[確定(命令)]** | `[ebx+0x14]` を差し替える（旧値の解放→新値の AddRef）＝**スマートポインタ/参照カウント代入ヘルパ**（notify とは無関係・v591 の判定で既出）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
(a) MessageMenu base:
  5BC48A  mov byte ptr [esi + 0x60], 1      ; head 0x5BC450（MessageMenu init）
  → al = 1 + (id - 4): id4→1, id5→2

(b) 0x5BCCB0 (id==2 分岐):
  5BCCE1  movzx eax, word [eax+0x7e]        ; 現在値
  5BCCEE  push 0xFDE ; call 0x588C10        ; race_name テキスト取得
  5BCD19  call 0x4028D0                     ; 文字列 Set
  5BCD5C  mov al, [ecx+edi] ; cmp al, 2 ; jne ... ; mov bl, 1
  → 入力テキストを走査して確定（入力欄系）

(c) V-585-1: push 0x40C の17 head に [+8] byte 書込み = 0件

(d) 0x5894F0:
  5894F9  push 0xFA8 ; mov ecx,esi ; call 0x588BD0   ; id trait
  589505  fild [esp+0xc] ; fucompp ; test ah,0x44 ; jp
  589512  mov eax, esi ; ret                          ; 一致で tile 返却
  → 「id trait == 指定値」の tile 検索
```

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-595-1 | `0x5BC450`（MessageMenu init）の全体と `[+0x60]` 以外の初期値（base が常に 1 かの確認） | 低 |
| V-595-2 | `0x5894F0`（id trait 検索）の callers と id98 解決への寄与（notify ではないが id98 の解決候補） | 低 |

---

## 4. probe 一覧

`v594_1`（`[reg+0x60]` byte 書込みの 0x5B-0x5C 全域走査＝`0x5BC48A`(imm=1) と `0x5BCB3F` の2件／`0x5BCCB0` の `0xFAE`/`0xFDE`→`0x4028D0`→文字列走査／`push 0x40C` 17 head の `[+8]` 書込み 0件／`0x581390`・`0x5894F0` head＝id trait 検索ヘルパ）。1本 EXIT=0。
