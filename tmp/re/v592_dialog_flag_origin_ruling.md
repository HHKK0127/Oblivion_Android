# v592 — V-588-1/2 + V-587b-1 クローズ: ダイアログ flag の保存元と `0x5BCCB0` の実体（v587 の `0x5BC6B0` 経路の訂正を含む）（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v592_1〜v592_3**・3本 全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v587_dialog_response_writer_ruling.md`／`tmp/re/v588_messagemenu_notify_ruling.md`（本裁定で一部訂正）
判定: `[確定(命令)]` / `[確定負（v587/v588 訂正）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| `[entry+0x60]`（flag）の保存元 | **[確定(命令)]** | **`0x5BC8B0`（ダイアログエントリ生成）の `0x5BCB3F: mov byte [ebp+0x60], al`** — `al` は **`0x5BC8B0` の第5引数（最後の push、`ret 0x14` の a5）**。 |
| `0x5BC8B0` の引数契約 | **[確定(命令)]** | **5引数**（`add esp,0x14`）。callers は3関数: **`0x579C10`（汎用ダイアログ登録・v570/v586 の6引数 API）／`0x579CF0`／`0x5BCC00`**。`0x579C10` の call site は `push <a5>; push <a4>; push <a3>; push ecx; push edx; call 0x5BC8B0` の5連 push で、**`0x579C10` の `flag`(arg3) が `0x5BC8B0` の a3 に対応**。 |
| `0x5BCDD0`（MessageMenu notify）の `id==2` 分岐先 | **[確定(命令)]** | `0x5BCCB0` を呼ぶ。 |
| **v587/v588 の「`0x5BCCB0`→`0x5BC6B0`（応答バイトへ flag コピー）」は正しいか** | **[確定負（訂正）]** | **誤り**。`0x5BCCB0` の call 一覧（`0x5BCCB0`〜`0x5BCDC5`）に **`0x5BC6B0` は無い**（`0x582160`／`0x588C10`／`0x4028D0`／`0x401F20`×5 のみ）。→ **`0x5BCCB0` は `0x5BC6B0` を呼ばない**。v587 の「応答バイトへ flag コピー」は **`0x5BC6B0` 単体では正しい**（`[+0xB0]=[entry+0x60]`）が、**その駆動元が `0x5BCDD0`/`0x5BCCB0` であるという接続は誤り**。 |
| 応答バイト `[+0xB0]` を実際に書く経路 | **[部分確定]** | `0x5BC6B0` の E8/E9 callers は **`0x5BCDD0` のみ**（v587 で確認・`0x5BCE27`）。ただし `0x5BCDD0` の `id==2` 分岐先 `0x5BCCB0` は `0x5BC6B0` を呼ばないため、**`0x5BCDD0` から `0x5BC6B0` への接続は `0x5BCCB0` の先（更に深い callee）にある** → **V-593-1 で追跡**（`0x5BCCB0` の全 call 先の先を辿る）。 |
| Random/Reset callback の応答判定 | **[確定(命令)]** | **`0x5C9FC0`（Random cb）/`0x5C9F70`（Reset cb）は `0x578D70()` の戻り `al==2` で実行本体へ jmp**（`0x5C9FA8: cmp al,2; jne; jmp 0x5C9C70`）＝**v570 の記述と一致**（v592 で再確認）。→ **`al==2` が「肯定（Yes）」**。 |
| したがって `al==1` の意味 | **[部分確定]** | `0x5BC6B0` は `[+0xB0]=[entry+0x60]=flag` を書く。Random cb が `al==2` で実行される一方、**`FUN_005B8DC0`（Fast Travel cb）は `al==1` で実行**（v583）→ **`al==1` と `al==2` は「どちらのボタンが押されたか」の2値**で、**`al==2`＝肯定（Yes）**、`al==1`＝もう一方。**どちらのボタンが `al==1` を立てるか（＝`[entry+0x60]` に 1 を書くボタン）の最終確定は V-593-2**。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
(a) ダイアログエントリ生成 0x5BC8B0（5引数・ret 0x14）:
      5BC8B0  prologue
      5BCB3F  mov byte ptr [ebp + 0x60], al    ; ★ a5(flag) を entry[+0x60] へ
    callers（E8/E9）: 0x579C10 @0x579CB7 / 0x579CF0 @0x579DAF / 0x5BCC00 @0x5BCC1B

(b) 0x579C10 の call site（5連 push）:
      579CA7  lea ecx, [esp]
      579CAA  push ecx
      579CAB  mov ecx, [esp+0x10]
      579CAF  push edx
      579CB0  mov edx, [esp+0x10]
      579CB4  push eax
      579CB5  push ecx
      579CB6  push edx
      579CB7  call 0x5BC8B0
    → 0x579C10 の (text, callback, flag, yes, no, ?) の flag が a3 として渡る。

(c) 0x5BCCB0（0x5BCDD0 の id==2 分岐先）の call 一覧:
      5BCCDC call 0x582160 / 5BCCF7 call 0x588C10 / 5BCD19 call 0x4028D0
      5BCD67/5BCD79/5BCD9E/5BCDA5/5BCDAB call 0x401F20
      ret 0x5BCDC5
    → ★ 0x5BC6B0 への call は無い（v587/v588 の接続は誤り）。

(d) Random callback 0x5C9FC0（v570 再確認）:
      5C9FA3  call 0x578D70
      5C9FA8  cmp al, 2
      5C9FAA  jne 0x5C9FB1
      5C9FAC  jmp 0x5C9C70          ; al==2 → 実行本体（肯定）
      5C9FB1  mov byte [0xB3B4C9], 0 ; 否 → flg クリア
```

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| **V-593-1** | **`0x5BCDD0`(id==2) → `0x5BCCB0` → ? → `0x5BC6B0` の完全な経路**（`0x5BCCB0` の全 call 先の先を辿り `0x5BC6B0` への到達を確定）＝v587 の接続訂正の完成 | **中** |
| V-593-2 | **`[entry+0x60]` に `1` を書くボタン**（`al==1` 側）と `0x5BC8B0` の a5 実値（flag=1/2 の Yes/No 最終対応） | 中 |
| V-593-3 | `0x579CF0`（`0x5BC8B0` の第2 caller）が渡す flag 値 | 低 |
| V-593-4 | `0x441920`（`0x5827D0` の callee）＝**vtable[3] 値を受け取る関数**（`[ebx+0x14]` の差し替え・`0x4028D0` 系でない）→ V-592-2 の解答 | 低 |

---

## 4. probe 一覧

`v592_1`（`0x5BCDD0` 全 decode／`0x5BCCB0` 頭／`0x5BC8B0` 頭／`[reg+0x60]` imm 書込み＝`0x5BC48A` のみ／`0x441920` head）／`v592_2`（`[reg+0x60]` reg 書込み7件＝`0x5BCB3F` が `0x5BC8B0`／Random cb `0x5C9F70`-`0x5C9FC0`＝`al==2`→jmp）／**`v592_3`（`0x5BC8B0` の arg 読取・callers 3関数と push 列・`0x5BCCB0` の全 call 一覧＝`0x5BC6B0` 不在を確定）**。3本 EXIT=0。
