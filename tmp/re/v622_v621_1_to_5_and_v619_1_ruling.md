# v622 — V-621-1〜5 と V-619-1 の決着: 12 サイトの (要素, ページ)・読み手 160 の層別・`0x589980` の owner・31 名の用途

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u68_1 / u69_1**（EXIT=0）。
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. V-621-1: 12 サイトの (要素名 strA, ページ名 strB) [確定(命令)]

1 サイトの全 dump で構造が確定:
```
0x5CDF29  push 0xFAE                      ; ★ trait（GetFloat の arg1）
0x5CDF2E  sub esp,8; ... build BSStringT#1 from [0xB38FC0]="Red"
0x5CDF4C  call 0x4028D0                   ; str#1
0x5CDF51  mov eax,[0xB38F88]="Hair"
0x5CDF56  sub esp,8; ... build BSStringT#2
0x5CDF6F  call 0x4028D0                   ; str#2
0x5CDF79  mov [esp+0x40],-1               ; flag
0x5CDF7D  call 0x5C3440                   ; GetTile(strA, strB, flag)
0x5CDF82  mov ecx,eax; call 0x588BD0      ; ★ GetFloat(tile, 0xFAE)
```
| site | 要素名 (strA) | ページ名 (strB) |
|---|---|---|
| `0x5CDF29` | **Red** | Hair |
| `0x5CDF92` | **Green** | Hair |
| `0x5CDFF3` | **Blue** | Hair |
| `0x5CE0F5` | **Length** | Hair |
| `0x5CE170` | **Hair Color** | Hair |
| `0x5CE1E3` | **Hair Color** | Hair |
| `0x5CE24B` | （未取得） | Race |
| `0x5CE2C4` | **Gender** | Race |
| `0x5CE345` | **Age** | Main |
| `0x5CE3A6` | **Complexion** | Main |
| `0x5CE45C` | **Eyes** | Main |
| `0x5CE4D5` | **Style** | Hair |

- → 12 サイトは **Hair ページの Red/Green/Blue/Length/HairColor/Style・Race ページの Gender・Main ページの Age/Complexion/Eyes** の user0 を読む。
- 0x5CE24B の要素名のみ未取得（残点 V-622-1）。

---

## 2. V-621-2: 12 名への user0 書き手 [確定(命令)（negative）]

- `0x5C3440` 呼出 **142** のうち、**直後 20 命令に `push 0xFAE`＋setter が続くものは 1 件のみ**（`0x5C6F6D` names=['Hair'] → `0x58CEB0`）。
- → **12 名のページ tile の user0 は `0x5C3440`＋setter の形では書かれない**。書き手は別経路（builder がローカルに保持する tile、または汎用の 155）→ 残点 **V-622-2**。

---

## 3. V-621-3: 読み手 160 の層別 [確定(命令)]

| tile 取得元 | 件数 |
|---|---|
| **menuRoot(`[+0x68]`)** | **58** |
| local/other | 73 |
| `GetTile(0x5C3440)` | 25 |
| `GetTileMenu(0x589B70)` | 3 |
| `GetContainingMenu(0x5898F0)` | 1 |
| 計 | **160** |

- → **読み手 160 の 36%（58 件）は menuRoot の user0**＝**汎用 UI フラグの読み**。**chargen のページ機構とは無関係**。
- ページ tile 由来は `GetTile(0x5C3440)` 25 件（うち 12 が 0x5CDEF0 のサイト）。
- → 式木と突き合わせる際は **menuRoot 58 を除外**すればノイズが減る。

---

## 4. V-621-4: `0x589980` の owner [確定(命令)]

- 5 サイトすべて同一形:
  ```
  push 0xFAE; call 0x588BD0        ; GetFloat([esi+0x58], 0xFAE)
  push ecx
  mov ecx, [esi+0x58]              ; ★ owner = [esi+0x58]
  fstp [esp]
  call 0x589980                    ; Delete([esi+0x58], 0xFAE, value)
  ```
- → **リストの owner は `[esi+0x58]`**（`esi` の +0x58 フィールドが指すオブジェクト）。5 サイトは **その user0 を読んでから削除**。
- `[esi+0x58]` の型（Tile か Menu か）は残点 V-622-3。

---

## 5. V-621-5: 未同定 2 件 [確定(命令)]

| site | 内容 |
|---|---|
| `0x58AC81` | `push 0xFAE; push 0xA6A3C0(name); mov ecx,esi; call 0x58A0A0` ＝ **trait 登録**（`0x58A0A0`＝v548 の属性登録。0xFAE は**登録する id**） |
| `0x5A2C73` | `push 0xFAE; push ecx; push 0xFA1` → 消費 call の trait は **0xFA1(visible)**。0xFAE は trait ではない |

- → **どちらも user0 の読み書きではない**。**155(setter)/160(reader)/5(deleter) の分類は変わらない**（322 = 155+160+5+2 の 2 は「非 user0 消費」）。

---

## 6. V-619-1: 31 名の用途 [確定(命令)]

- `0xB38F70..0xB39068` の **31 名**は、**ページ名（strB）と要素名（strA）の両方**に使われる**同一の名前プール**。
  - **ページ名として**: Hair・Race・Main（12 サイトの strB）＋ Face/Shape/Tone/General(Face)/Brow/…（21 ページの他）。
  - **要素名として**: Red/Green/Blue/Length/Hair Color/Style（Hair 配下）・Gender（Race 配下）・Age/Complexion/Eyes（Main 配下）。
- → **「ラベル文字列」と「ページ識別子」は文字列では区別できず、呼出の役割（strA/strB）で決まる**。v621 の「11/12/13」問題はこれで説明できる。
- `Race Description`(0xB38FB0) は **Text 行のラベル**（v610 §1.2 の Race ページ Text）、`Skin`/`Lips` 等は**要素名**。

---

## 7. 残点

- **V-622-1【低】**: `0x5CE24B` の要素名。
- **V-622-2【高】**: 12 名のページ tile の user0 **書き手**（`0x5C3440` 経由ではない経路＝builder のローカル保持）。
- **V-622-3【低】**: `[esi+0x58]` の型（Tile / Menu）。
- **V-622-4【中】**: local/other 73 の内訳（ページ button の可能性）。

---

## 8. 検証

- probe **u68_1**（1 サイト全 dump／12 サイトの push 列／未同定 2）。
- probe **u69_1**（12 サイトの (要素,ページ)＝V-621-1／0x5C3440+setter＝1 件／読み手 160 の層別／0x589980 の owner＝[esi+0x58]／31 名）。
- 155 の母数は不変（322 = 155 + 160 + 5 + 2）。
