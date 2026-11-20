# v588 — V-587-2 クローズ: `0x5BCDD0` は MessageMenu の通知ハンドラ（vtable slot3）で `id==2` のとき応答バイトを確定（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v588_1**・1本 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v587_dialog_response_writer_ruling.md`（V-587-2 の起票元＝`0x5BCDD0` の vtable 呼出元）
判定: `[確定(命令)]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| `0x5BCDD0` の呼び出し元（vtable 経由）は何か | **[確定(命令)]** | **`MessageMenu` の vtable `0xA6CE5C` の slot3（`+0x0C`）＝通知ハンドラそのもの**。`0x5BCDD0` の dword は `.rdata 0xA6CE68`（＝`0xA6CE5C + 0x0C`）に**1件のみ**存在。→ **`0x5BCDD0` はメニュー通知 `(id, ?)` を受けるハンドラ**で、E8/E9 callers が0件なのは vtable 経由だから（v583/v587 の教訓の再現）。 |
| `id==2` の意味 | **[確定(命令)]** | **`0x5BCDD0(id, ?)`**: `id==2` のときのみ **`0x5BCCB0` を呼ぶ**（→ 内部で `0x5BC6B0`＝応答バイトへ flag コピー）。`id!=2` は `0x5BCDE7` へ分岐（続きは §2）。→ **id 2 が「ダイアログ確定」通知**。 |
| MessageMenu の vtable 構成 | **[確定(命令)]** | `[0]=0x5BC690`／`[1]=0x5BC510`（head `0x5BC4A0`）／`[2]=0x60CF60`（no-op）／**`[3]=0x5BCDD0`（通知）**／`[4]=[5]=0x5BC770`／`[6]=[7]=0x890B90`（共通）／`[8]=0x60CF60`（no-op）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
MessageMenu vtable 0xA6CE5C:
  [3] +0x0C = 0x5BCDD0   ← dword 0x5BCDD0 の唯一の出現（.rdata 0xA6CE68）

通知ハンドラ 0x5BCDD0(id, ?):
  5BCDD1  mov ebx, [esp+8]          ; id（ret 8 の第1引数）
  5BCDD5  cmp ebx, 2
  5BCDD9  mov edi, ecx              ; this = MessageMenu
  5BCDDB  jne 0x5BCDE7              ; id != 2 は別処理
  5BCDDD  call 0x5BCCB0             ; → 0x5BC6B0（応答バイトへ flag コピー）
  5BCDE4  ret 8
```

* **`0x5BC6B0` への経路**: `MessageMenu` 通知 `id==2` → `0x5BCDD0` → `0x5BCCB0` → `0x5BC6B0`（`[singleton+0xB0] = [entry+0x60]`＝登録 flag）。＝**v587 で確定した「応答バイトの書き手」が MessageMenu の「確定」通知（id2）から駆動される**ことが確定。

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-588-1 | `0x5BCDD0` の `id!=2` 分岐（`0x5BCDE7` 以降）と、他の通知 id（例: ボタン押下が id 1 か）の対応＝**`1`/`2` の Yes/No 最終対応**（V-587b-1 の続き） | 低 |
| V-588-2 | `0x5BCCB0`（`0x5BCDD0` の callee）の本体と `0x5BC6B0` への到達条件 | 低 |

---

## 4. probe 一覧

`v588_1`（dword `0x5BCDD0` の唯一出現＝`.rdata 0xA6CE68`／MessageMenu vtable `0xA6CE5C` 全9 slot／通知ハンドラ `0x5BCDD0` decode＝`id==2`→`0x5BCCB0`）。1本 EXIT=0。
