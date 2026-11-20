# v587 — V-587-1 クローズ: ダイアログ応答 `[+0xB0]` の書き手＝`0x5BC6B0`（登録 flag `[entry+0x60]` をコピー）＝`al` の 1/2 は登録 flag 由来（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v587_1・v587_2**・2本 全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v586_laneC_reconcile_ruling.md`（V-587-1 の起票元＝`0x578D70` の al=1/2 の肯定否定対応と `[+0xB0]` writer）
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| singleton `[+0xB0]`（ダイアログ応答バイト）に `1`/`2` を書くのは誰か | **[確定(命令)]** | **`0x5BC6B0`（応答セッター）**: `[+0xB0]==0xFF`（未応答）のときだけ **`cl = [esi+0x60]` → `[singleton+0xB0] = cl`**（`0x5BC6F6`-`0x5BC705`）。`esi`＝ダイアログエントリ（`0x589B70(0x3E9)`＝MessageMenu → `0x5898F0` 根探索）。 |
| `[entry+0x60]` は何か | **[確定(命令)]** | **登録 API `0x579C10` の `flag` 引数（arg3）**。`FUN_005BC8B0`（ダイアログエントリ生成）が `0x5BCB3F: mov byte [ebp+0x60], al` で保存する値。→ **`[+0xB0]` に書かれる値＝登録時の flag**。 |
| したがって `0x578D70` の戻り値 `al` の 1/2 は何を意味するか | **[部分確定]** | **`al` ＝登録 flag の値**（`1` か `2`）。**flag は「どちらの応答でコールバックを起動するか」のセレクタ**と整合（`0x579C10` は 6 引数で `(text, callback, flag, yes, no, ?)`＝v586 で実測）。**`1`/`2` のどちらが Yes/No かは、ボタン押下側（`0x5BCDD0` の `arg`）と flag の対応が未読のため [部分確定]**。 |
| 応答の書き込み経路 | **[確定(命令)]** | **`0x5BCDD0(arg, ?)`**: `arg==2` のとき `0x5BCCB0` → **`0x5BC6B0`**（`[+0xB0]` へ flag コピー）。`0x5BCDD0` は E8/E9 callers 0件＝**vtable/登録経由**（v583 の「callers 0件＝登録経由」教訓の再現）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
応答セッター 0x5BC6B0（head）:
  5BC6B1  push 0x3E9 ; call 0x589B70      ; MessageMenu(0x3E9) 取得
  5BC6CB  call 0x5898F0                   ; 根タイル探索
  5BC6DE  mov byte [0xB143AE], 1          ; 「応答待ち」グローバル
  5BC6E5  call 0x582160 (1,0)             ; singleton 取得
  5BC6ED  cmp byte [eax+0xB0], 0xFF       ; ★ 未応答のときだけ
  5BC6F4  jne 0x5BC70B                    ;   既応答ならスキップ
  5BC6F6  call 0x582160 (1,0)
  5BC6FF  mov cl, byte [esi+0x60]         ; ★ ダイアログエントリの flag
  5BC705  mov byte [eax+0xB0], cl         ; ★ 応答バイトへコピー
  5BC70B  fld [0xA379B4](=2.0) ; push 0x1772 ; 0x58CEB0  ; 後処理

応答読取 0x578D70（v586 で確定）:
  578D7A  mov bl, byte [eax+0xB0]         ; 旧値
  578D8C  mov byte [eax+0xB0], 0xFF       ; 0xFF へクリア
  578D93  mov al, bl ; ret                ; 旧値を返す
  （初期値は ctor 0x5802B0 の 0x58042B: mov byte [esi+0xB0], 0xFF）

応答経路 0x5BCDD0:
  5BCDD5  cmp ebx, 2 ; jne 0x5BCDE7
  5BCDDD  call 0x5BCCB0  →（0x5BC6B0 を call @0x5BCE27）
  （0x5BCDD0 の E8/E9 callers = 0 → vtable/登録経由）
```

* `[+0xB0]` の書き手は `.text` 全体で 7 件（imm 形式）＋4 件（reg 形式）のみ。**singleton を `0x582160` で取って `[+0xB0]` に書くのは `0x578D70`/`0x578DA0`（=0xFF クリア）／`0x5802B0`（ctor 初期化 0xFF）／`0x5BC8B0`（0xFF）／`0x5BC6B0`（flag コピー）／`0x5BCDD0`（同経路）** の5関数に限定。
* → **`1`/`2` を書く唯一の実体＝`0x5BC6B0`**（`[entry+0x60]` の値）。

---

## 3. 副次（[確定(命令)]）

* `0x5A3D00`（`0x3F3`＝MessageMenu 系で `[reg+0xB0]` に reg=3 を書く）は**別グローバルの別クラス**（`[0xB143AE]` 系の応答待ちではなく、`0x3F3` の別メニュー）。`0x8DF420`（imm=1）は**レンダラ系構造体の初期化**（`movaps` 連続＋`0x3F800000`）でダイアログとは無関係（Lane C が別クラスと推定した `0x8DF4A2` を [確定(命令)] で否定）。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-587b-1 | `1`/`2` の Yes/No 対応: `0x5BCDD0` の `arg`（押下ボタン番号）と `[entry+0x60]`（flag）の対応、および `0x579C10` の `yes`/`no` 引数（arg4/arg5）が `[entry+?]` のどこへ保存されるか | 低 |
| V-587b-2 | `0x5BCDD0` の vtable 経由呼び出し元（MessageMenu のボタン通知ハンドラ） | 低 |

---

## 5. probe 一覧

`v587_1`（`[reg+0xB0]` 書込み全走査＝imm 7件/reg 4件・singleton-head 分類・`0x5BC8B0` 内の `[+0x60]` 読取走査）／**`v587_2`（`0x5BC6B0`＝flag コピー実測・`0x5BCDD0`・`0x8DF420`＝レンダラ系と否定・callers）**。2本 EXIT=0。
