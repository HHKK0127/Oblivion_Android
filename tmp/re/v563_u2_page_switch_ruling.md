# v563 — V-558-1 クローズ: RaceSexMenu のページ切替は **Done ボタン(id=90) のみ**

対象: `Oblivion.exe` v1.2.0416 (Steam)。probe v563_1/v563_2 ＋ `0x5CC200` の実読。
判定: `[確定(命令)]` / `[確定負]`

---

## 1. 結論

| 問い | 判定 | 結論 |
|---|---|---|
| ページ切替の入力経路 | **[確定(命令)]** | **Done ボタン（tile id = 90）のクリックのみ。** タブ・左右キー・スクロールでのページ切替は存在しない。 |
| 実装 | **[確定(命令)]** | 通知ハンドラ `0x5CC200`（RaceSexMenu vtable `+0x0C`）内で、**現在ページに応じて** `SetTrait(root=[this+4], 0xFAE /*user0*/, GetTrait(GetPage("<target>"), 0xFAE))` を実行する。 |
| 切替先 | **[確定(命令)]** | target はページごとに固定で、実読できたのは **Main（`0x5CC857`）／Tone（`0x5CC9AD`）／Shape（`0x5CC9F2`）／Face（`0x5CCA42`）／Random（`0x5CC8BA`）**。 |
| Tab キー / 矢印キーでの切替 | **[確定負]** | `0x5C0000..0x5D0000` の `push 0xFAE` 80 箇所を全分類した結果、**メニュー根（`[this+4]`）への SetFloat は 4 箇所のみ**で、いずれも上記 Done 経路。キー入力からページを書く経路は無い。 |

---

## 2. 根拠

### 2.1 ページ書き込みの同定

`0x5CC200` 内の 4 箇所（`mov ecx, edi` は `push 0xfae` の直後にあるため、素朴な逆走査では取り逃す）:

```
5CC842  GetPage("[0xB38F70]"=Main)  → eax
5CC849  edi = [esi+4]                        ; メニュー根 tile
5CC84E  GetTrait(pageTile, 0xFAE)            ; そのページの index
5CC857  SetTrait(edi, 0xFAE, index)          ; → Main へ
```
同型が `0x5CC9AD`（`[0xB38FE8]`=Tone）／`0x5CC9F2`（`[0xB38FE0]`=Shape）／`0x5CCA42`（`[0xB38F80]`=Face）／`0x5CC8BA`（`[0xB38FF0]`=Random）。

### 2.2 分岐の入口

```
5CC277  cmp ebx, 0x5A          ; 90 = Done
5CC27A  jne 0x5CC8BA
5CC280  if (cur == Main)        → 0x5CCA77（名前チェック → 確認 → 0x5CC857 で Main へ戻す）
5CC2D6  if (cur == Shape)       → 0x5CCA09（→ Face）
5CC32A  if (cur == Tone)        → 0x5CCA09（→ Face）
5CC37E  if (cur == General(Face)) → 0x5CCA09（→ Face）
...     それ以外                → 0x5CC8BA ほか（→ Shape / Tone / Random）
```

### 2.3 他の入力 id

`0x5CC200` が扱う通知 id は **90（Done）／10（`race_name` クリック → `0x5C2730`）／98（`0x5CCBF4`）**。ページを書くのは 90 のみ。

---

## 3. 移植への示唆

1. 原版のページ送りは **Done ボタンの連打によるウィザード**であって、タブクリックでもキー操作でもない。
2. 移植がタブ UI を採るなら、**原版の「Done で次ページへ」を再現する手段が別途必要**（タブに置き換えるのは仕様変更）。
3. ページ index を書くのは `SetTrait(menuRoot, user0, index)` の 1 点なので、移植側も「現在ページ」を 1 個の状態値で持つのが原版準拠。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-563-1 | ページごとの target 全対応（Shape→Tone→…→Main の完全な順序） | 中 |
| V-563-2 | id=98 / id=10 の分岐内容（Back ボタン／タイトルクリック） | 低 |
