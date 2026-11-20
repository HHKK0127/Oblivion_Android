# v593 — V-593-1 クローズ: `0x5BCDD0` も early-return 型で本体を完全同定（`id==2`→`0x5BCCB0`／`id>=4`→応答バイト算出→`0x5BC6B0`）＝v587/v588 の接続を回復（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v593_1・v593_2**・2本 全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v592_dialog_flag_origin_ruling.md`（V-593-1 の起票＝`0x5BCCB0` の先に `0x5BC6B0` 到達）
判定: `[確定(命令)]` / `[確定負（v592 訂正）]` / `[部分確定]`

---

## 1. 結論（矛盾の解消）

| 問い | 判定 | 結果 |
|---|---|---|
| `0x5BCDD0`（MessageMenu notify）の実体 | **[確定(命令)]** | **early-return 型の多分岐ハンドラ**（v588/v592 の「短い関数」は**最初の `ret 8` で打ち切ったアーティファクト**＝v586 の `0x587550` と同じ罠）。真の本体（`0x5BCDE7` 以降）を含む。 |
| `0x5BCDD0` の全分岐 | **[確定(命令)]** | ①**`id==2` → `0x5BCCB0`**（`ret 8`）②**`id>=4` → 応答バイト算出 → `0x5BC6B0`**（§2）③`id<4` かつ `!=2` は `0x5BCE2D` へ（実質何もしない）。 |
| **v587/v588 の「`0x5BCDD0`→`0x5BCCB0`→`0x5BC6B0`」接続は正しいか** | **[確定負（v592 訂正の取消）]** | **v592 の訂正が誤り。接続は正しい。** `0x5BC6B0` の呼び出しは **`0x5BCDD0` の本体 `0x5BCE27`**（`id>=4` 分岐の末尾）にあり、**`0x5BCCB0` 経由ではない**。v587 の「`0x5BC6B0` の唯一 caller＝`0x5BCDD0`」は正しかった。 |
| 応答バイト `[+0xB0]` の値の決まり方 | **[確定(命令)]** | **`0x5BCDD0` の `id>=4` 分岐**: `cl = byte [this(MessageMenu)+0x60] + id - 4` → **`[singleton+0xB0] = cl`**（`0x5BCE16`-`0x5BCE21`）→ `0x5BC6B0`（最終処理）。**＝通知 id（ボタン番号）から応答値が算術で決まる**。 |
| `al==1`/`al==2` の Yes/No 最終対応 | **[部分確定]** | `al = base + (id − 4)`（`base=[this+0x60]`）。**2ボタンダイアログなら id 4/5 → al = base/base+1**。Random cb は `al==2` で実行（v592）＝**al==2 が肯定（Yes）** → **id 5 側（base=1 のとき）が Yes**。`base` は実行時値のため**厳密な 1/2 対応は [部分確定]**（残点 V-594-1）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
MessageMenu notify 0x5BCDD0（真の末尾 0x5BCE2F ret 8）:
  5BCDD1  mov ebx, [esp+8]            ; id（通知＝ボタン番号）
  5BCDD5  cmp ebx, 2
  5BCDDB  jne 0x5BCDE7                ; id != 2
  5BCDDD  call 0x5BCCB0               ; ① id==2 の処理
  5BCDE4  ret 8
  5BCDE7  cmp ebx, 4                  ; ② id >= 4
  5BCDEA  jl  0x5BCE2D                ; id < 4 かつ != 2 は何もしない
  5BCDED  push 1 ; call 0x57DE50
  5BCDF9  call 0x582160               ; singleton
  5BCDFE  mov [eax+0x98], esi / +0x9c / +0xa0 / +0xa4 = 0   ; フィールドクリア
  5BCE16  mov cl, [edi+0x60]          ; edi = MessageMenu this
  5BCE19  add cl, bl                  ; bl = id
  5BCE1B  sub cl, 4
  5BCE21  mov [eax+0xB0], cl          ; ★ 応答バイト＝[this+0x60] + id - 4
  5BCE27  call 0x5BC6B0               ; ★ 最終処理（[+0xB0] は既に非0xFFなので 0x5BC6B0 内の 0xFF 判定は skip）
  5BCE2F  ret 8

0x5BC6B0（応答セッター・v587）: cmp byte [eax+0xB0],0xFF ; jne skip ; cl=[esi+0x60]; [eax+0xB0]=cl
  → 0x5BCDD0 が先に [+0xB0] を非0xFF に設定するため、0x5BC6B0 のコピーは skip され
    「ダイアログ最終処理」として機能する。
```

* `0x5BC6B0` の E8 caller は **`0x5BCE27`（`0x5BCDD0` 本体）の1件のみ**（v593_1 で再確認）。
* `0x5BCCB0`（`id==2` 分岐先）は `0x5BC6B0` を呼ばない（v592 で確定）＝**別経路**。

---

## 3. 手法メモ（再発型・重要）

**early-return 型の関数は「最初の `ret` で打ち切る線形 decode」で誤って短く見積もる** — 本件では `0x587550`（v586）・`0x5BCDD0`（v588/v592）の**2回**踏んだ。正しい手順:
1. **関数末尾は `ret` の直後が `int3`（0xCC）連続かどうかで判定**（`ret` + `CC` = 真の末尾）。
2. `jne`/`je`/`jmp` の分岐先も**本体**として decode に含める（`jne` の直後の `ret` で止めない）。
3. `find_head` は「そのアドレス以下の最大 head」なので、**同一関数内の複数 `ret` はすべて同じ head を返す**（＝関数長は `find_head` では分からない）。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-594-1 | **`[MessageMenu+0x60]`（`base`）の値**とボタン id の実値（2ボタンダイアログの Yes/No が id 4/5 か）＝`al==1`/`al==2` の厳密対応 | 低 |
| V-594-2 | `0x5BCCB0`（`id==2` 分岐）の役割（`0xFAE`/`0xFDE` テキスト取得＝入力欄系か） | 低 |

---

## 5. probe 一覧

`v593_1`（`0x5BC6B0` callers＝`0x5BCDD0` のみ再確認／`0x5BCCB0` の全 call 一覧／`0x5BCE27` 文脈／`0x5BCCB0`・`0x5BCC00` head）／**`v593_2`（`0x5BCDD0` の全 linear decode＝`ret`+`CC` 判定で真の末尾 `0x5BCE2F` を確定・`id>=4` 分岐の応答バイト算出 `cl=[edi+0x60]+bl-4`→`[eax+0xB0]`→`call 0x5BC6B0` を実測）**。2本 EXIT=0。
