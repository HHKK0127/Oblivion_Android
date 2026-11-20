# v580 — V-580-1: 第8系統 negative 完了（`0x591C50` 到達は vtable slot5 のみ）と、矛盾解消の本命＝「窓 id 生成 switch `0x587550`」の特定（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v580_1**・1本 EXIT=0・冒頭 Test-Path）＋ v574 の既出事実。
先行: `tmp/re/v579_587b20_floatid_ruling.md`／`tmp/re/v574_hud_open_path_ruling.md` §1.2
判定: `[確定(否定側)]` / `[部分確定]` / `[未確定]（本命を特定）**

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| (第8系統) `0x591C50` の到達形式は vtable slot5 の indirect のみか | **[確定(否定側)]** | **dword `0x591C50`（`50 1c 59 00`）のファイル全体出現＝1件のみ＝`.rdata 0xA6AF28`＝TileMenu vtable `0xA6AF14+0x14`（slot5 本体）**。context が slot4(`0x5900E0`)/slot5(`0x591C50`)/slot6(`0x591B40`) の並びで vtable と完全整合。**`.text` に `push 0x591C50`／`mov reg,0x591C50`／`ff 15` operand は一切なし**（**push+ret tail も不存在**＝`68 50 1c 59 00` も0件）。→ **open への静的到達は「vtable slot5 の 1段/2段 indirect」のみ**（=v578 の282件＋408対が全体）＝**静的経路の完全性を証明**。 |
| では矛盾はどこで解けるか | **[部分確定]（本命特定）** | **「生成＝`0x587D20` switch（open 内からのみ）」は1つの経路にすぎない** — **v574 §1.2 で発見済みの別 switch `0x587550`（`cmp edi,0x3EC → call 0x5A6B00`／`0x3ED→0x5A4840`／`0x3F2→0x5A8E30`／`0x3EF→0x5ADCF0`／`0x414→0x5B6040`）＝「窓 id → 生成ヘルパ」の第2 switch が本命**。E8/E9 と別 switch の**両系統で「open 不要の生成経路」が存在する可能性**が残っており、**`0x587550` の callers・name・`0x5A6B00` 等の identity が次の一手**。 |
| 矛盾5命題の現状 | **[部分確定]** | ①open 成功＝`a1==0xFA2` ②`0xFA2` 即値2件 ③`0x587D20`＝open 内1件 ④成功呼び出し＝全8系統（E8/E9/1段/2段/push-imm/mov-imm/data ptr(0x587D20)/data ptr+push(0x591C50)）で不存在 ⑤それでも生成される。→ **⑤を支える「第3経路＝別 switch」の存在確認が残るだけ**（§2）。 |

---

## 2. 矛盾解消の本命（次の一手＝V-581-1）

**`0x587550`（head）＝「窓 id → 生成ヘルパ」switch**（v574 §1.2 実測）:

```
0x58767F: cmp edi,0x3EC → call 0x5A6B00     ; HUDMainMenu 生成ヘルパ?
0x587693: cmp edi,0x3ED → call 0x5A4840     ; HUDInfoMenu
0x5876A7: cmp edi,0x3F2 → call 0x5A8E30     ; HUDSubtitleMenu
0x5876BB: cmp edi,0x3EF → call 0x5ADCF0     ; LoadingMenu
0x5876D4: cmp edi,0x414 → call 0x5B6040     ; MainMenu
（後続に id 判定列挙が続く）
```

* **`0x587D20` switch（1001..1051 の jmp table）とは独立**（E8 は open 内のみ・v576 確定）→ **この switch が第2の生成/取得経路**。
* **`edi` に窓 id を入れて戻り値（menu?）を返す関数**と読める → **callers を列挙すれば「open なしで menu を得る場所」＝矛盾解消の実行系**。
* v574 では `0x587550` の名前/ callers を未調査 → **V-581-1 で callers 列挙＋`0x58767F` ブロック全体 decode（id 判定列挙の続き）＋`0x5A6B00` 等の identity**。

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| **V-581-1** | **`0x587550`（窓 id 生成 switch）の callers・name・id 判定列挙の続き・`0x5A6B00` 等 identity**＝矛盾解消の第3経路 | **中**（V-580-1 継承） |
| V-581-2 | vtable slot5=0x591C50 の「open」という呼称の根拠再確認（v550 の記述と `cmp a1,0xFA2` type固定の関係） | 低 |
| V-581-3 | 2段形式408対の reg push の source 追跡（`mov reg,[..]` のソース集計＝entity/変数経由の id 確認） | 低 |

---

## 4. probe 一覧

`v580_1`（dword `0x591C50` 全ファイル走査＝**1件のみ `.rdata 0xA6AF28`＝vtable slot5**・`.text` 0件・ff15/push+ret 0件・ctx で slot4/5/6 並びを vtable 整合と確認）。1本 EXIT=0。
