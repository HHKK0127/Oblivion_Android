# v614 — v613 検収とレンジ再計数: slot 11 の同定・レンジを vtable union に張替え（結果 0 件）・`0x58CEB0` ヘッド確定・trait id⇔文字列・Value 完全レイアウト

対象: `Oblivion.exe` v1.2.0416 (Steam, md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u51_1 / u52_1 / u53_1**（EXIT=0）＋ v613 の一次資料。
判定: `[確定(命令)]` / `[確定(命令)（negative）]`

---

## 1. 結論

| # | 問い | 判定 | 答え |
|---|---|---|---|
| 1 | slot 11 は Menu と異なるか | **[確定(命令)] 異なる** | RaceSexMenu[11]=**`0x5CE570`**（head `0x5CDEF0` へ誤帰属）／Menu[11]=`0x60D0A0`。→ **Update を上書き**＝chargen の毎フレーム処理。 |
| 2 | レンジを vtable union に張替え、139 を再計数 | **[確定(命令)] 0 件** | union の generator 呼出は **0**。139 は**非仮想ビルダー `0x5CA010` に集中**（1 関数・extent `0x5CA010..0x5CC200`）。**vtable スロットには元々無い**（査読の「落ちている」は成立しない）。 |
| 2' | builder の呼出元 | **[確定(命令)]** | `0x5CA010` の caller は **1 箇所 `0x57B8D9`（head `0x57B7E0`）**。 |
| 3 | `0x58CEB0` の prologue | **[確定(命令)] ヘッド** | 直前 `0x58CEA0`=`ret 4`＋int3 → **正当な関数ヘッド**（leaf・`ret 8`）。 |
| 4 | trait id ⇔ 文字列 | **[確定(命令)]** | `0x589080 TileStrIDToStr(id)`＝id→文字列／`0x588EF0 TileStrToStrID(str)`＝逆。 |
| 5 | Value `+0x0C..+0x14` | **[確定(命令)]** | `+0x0C`=len(u16)／`+0x0E`=cap(u16)／`+0x10`=ptr／`+0x14`=ptr（いずれも ctor が確保する 0x18 バイト node）。 |

---

## 2. slot 11 = Update の同定 [確定(命令)]

- RaceSexMenu[11] = **`0x005CE570`**、Menu[11] = `0x0060D0A0` → **異なる（上書き）**。
- **`0x5CE570` は正当な関数ヘッド**：直前 `0x5CE561`=`ret`、`0x5CE562..0x5CE56F`=int3。`named_funcs.txt` にヘッドが無いため `find_head(0x5CE570)` が `0x5CDEF0` へ**誤帰属**（→ V-614-4 で是正）。
- 本体（先頭）:
  ```
  0x5CE570 push ecx; push esi; push 0; push 0x3E9; mov esi,ecx; call 0x5790E0; add esp,8; test al,al; jne ...
  0x5CE587 mov ecx,[esi+4]; push 0xFA1; call 0x588BD0   ; trait 0xFA1 = visible を読む
  ```
  → `visible`(0xFA1) を扱う Update 系。chargen の毎フレーム処理（V-614-1 で decode）。
- なお `0x5CDEF0`（別関数・extent `0x5CDEF0..0x5CE561`）は `0x5CDF19 mov ecx,[0xB333C4]`（PlayerCharacter 系）から `vfunc+0x170` を呼び `push 0xFAE`（user0）を扱う。

---

## 3. レンジ: vtable union で再計数 → 0 件 [確定(命令)]

RaceSexMenu vtable 15 slot の関数範囲（`find_head`＋次ヘッド）:

| slot | 値 | 関数範囲 | gen |
|---|---|---|---|
| 0 | `0x5C4E20` | `0x5C4E20..0x5C4E40` | 0 |
| 1 | `0x5C26D0` | `0x5C25C0..0x5C2730` | 0 |
| 2 | `0x5C2790` | `0x5C2790..0x5C28F0` | 0 |
| 3 | `0x5CC200` | `0x5CC200..0x5CDA30` | 0 |
| 4/5/9 | `0x60CF60` | `0x60CF60..0x60CF80` | 0 |
| 6/7 | `0x890B90` | `0x890B90..0x890BA0` | 0 |
| 8 | `0x5C3BE0` | `0x5C3BE0..0x5C3DD0` | 0 |
| 10 | `0x5C2870` | `0x5C2790..0x5C28F0` | 0 |
| 11 | `0x5CE570` | `0x5CDEF0..0x5CE680`（誤帰属で拡大） | 0 |
| 12 | `0x5C3DD0` | `0x5C3DD0..0x5C3E10` | 0 |
| 13 | `0x5C4E10` | `0x5C4BF0..0x5C4E20` | 0 |
| 14 | `0x725DA0` | `0x725D80..0x725DE0` | 0 |

- **union 合計 generator 呼出 = 0 件**。
- `[0x5CA010, 0x5CBC00)` のヘッドは **1 個（`0x5CA010`）**。その関数 extent は `0x5CA010..0x5CC200` で **generator 139 件**（Pane 21・Button 22・Toggle 5・Slider 90・Text 1）。旧レンジの尾 `0x5CBC00..0x5CC200` に generator は無い（139 のまま）。
- **帰結**：139 は**非仮想のビルダー `0x5CA010`** に集中し、**vtable スロットには 1 件も無い**。→ 査読の「一次探索が ctor 側と Done 側の generator を落とした」は**成立しない**（vtable 側に元々無い）。ただし**レンジ表現は関数 extent `[0x5CA010, 0x5CC200)` とすべき**（旧端 `0x5CBC00` は関数の途中）。
- **新事実**：builder `0x5CA010` の呼出元は **1 箇所 `0x57B8D9`（head `0x57B7E0`）**。→ メニュー構築の起点（V-614-2）。

---

## 4. `0x58CEB0` の prologue [確定(命令)]

```
0x58CEA0  c2 04 00        ret 4
0x58CEA3  cc cc ...        int3 詰め
0x58CEB0  ...              関数ヘッド（leaf: mov eax,[esp+4]; push eax; call 0x58B220; ...; ret 8）
```
→ **正当な関数ヘッド**。xOBSE の `::callAddr`（＝callee のアドレス）と整合。**v613 の「関数ヘッド」判定で確定**、査読の残疑義は解消。`0x58CED0`（`UpdateString`）も同様に独立ヘッド。

---

## 5. trait id ⇔ 文字列 [確定(命令)]

- **`0x589080 TileStrIDToStr(id)`**：`id >= 0x2710` は `[0xB13BC8]`（ハッシュ表）を、`id < 0x2710` は `[0xB3B0B4]`（リスト）を引き、`[entry+8]` の char* を返す。
- **`0x588EF0 TileStrToStrID(str)`**：逆方向。`str[0]=='&'`(0x26) の entity 分岐（`0xB3B0B4` のリスト照合）と、通常は文字から id を合成（`movsx eax, al; sub eax, 0x40` …）。
- → **観測した trait id を `TileStrIDToStr` に通せば XML 属性名に直結**。手作業の対応表が不要。probe は既知分のみ表引きし、未知 id は数値で出す（§7）。

---

## 6. Value の完全レイアウト [確定(命令)]

ctor **`0x589DF0 Value::Value(trait)`**（`alloc(0x1C)`）＋ `0x58CA00 SetNumber`／`0x58CA50 SetString` から:

| Offset | 型 | 内容 | 根拠 |
|---|---|---|---|
| +0x00 | ptr | owner（Tile*） | `[esi]=0`（ctor）／`[value]=ebp`（0x58B2AE） |
| +0x04 | f32 | number | `fstp [esi+4]`（ctor/SetNumber） |
| +0x08 | char* | string（BSStringT の data） | `[esi+8]=0`（ctor）／`FUN_004028d0(esi+8,...)` |
| +0x0C | u16 | string length | `word[esi+0xC]=0`（ctor） |
| +0x0E | u16 | string capacity | `word[esi+0xE]=0`（ctor） |
| +0x10 | ptr | 0x18 バイト node（ctor が確保、`[node+8]=Value`・`[node+0xC]=0x65`） | `[esi+0x10]=eax` |
| +0x14 | ptr | 0x18 バイト node（同上・2 個目） | `[esi+0x14]=eax` |
| +0x18 | u16 | **id（trait id）** | `word[esi+0x18]=ax`（ctor）／`cmp word[esi+0x18],0xFDE` |
| +0x1A | u8 | **isNumber**（ctor 既定 1） | `[esi+0x1A]=1`（ctor/SetNumber）／`=0`（SetString） |
| — | — | size **0x1C** | `push 0x1C; call 0x401F00` |

- 文字列値の読み出しは **`+0x08`（char*）＋ `+0x0C`（len）** で足りる（BSStringT）。
- `+0x10`/`+0x14` の 0x18 バイト node の役割は残点（V-614-3）。

---

## 7. probe 更新（`u47_1.py`・v614 仕様）

- **全 trait を dump**（既知 id に限定しない。未知 id は `0xXXXX` で出力）。
- **user\*（`0xFAE..0xFC2`）を `USER{...}` で明示フラグ**。
- **`[v+0x1A]`(isNumber) で分岐**：num は `[v+0x04]`、str は `[v+0x08]`（BSStringT）。
- **子リスト差分 STOP を維持**＋**trait 数 0 の tile を警告**（レイアウト誤りか本当に空かの切り分け）。
- 静的実行：`vtable=0x00A69680`・`m_pBase=0`・`capacity/firstFree/numObjs=0`（runtime 構築）。

---

## 8. 順序 [決定]

- **slot 11（意味）を先**に静的に読み切る（§2）。RegisterTile（構造）は **slot 11 が Menu と同一だった場合の fallback** に回す。
- 実機が要るのは**木の走査（V-613-1）とブレークポイント（V-613-3）だけ**。slot 11 は静的で進められる。

---

## 9. 残点

- **V-614-1【高】**: slot 11 `0x5CE570`（RaceSexMenu::Update）の完全 decode。
- **V-614-2【高】**: builder 呼出元 `0x57B7E0`（`0x57B8D9`）の decode＝メニュー構築の起点。
- **V-614-3【低】**: Value `+0x10`/`+0x14` の 0x18 バイト node ×2 の役割。
- **V-614-4【低】**: `named_funcs.txt` に `0x5CE570` のヘッドが無い（`find_head` 誤帰属の是正）。
- （継続）V-613-1/2/3/4。

---

## 10. 検証

- probe **u51_1 / u52_1 / u53_1**（EXIT=0・冒頭 Test-Path）。
- slot 11: `0xA6D38C+0x2C`=`0x5CE570`／`0xA696DC+0x2C`=`0x60D0A0`。直前 `0x5CE561`=`ret`＋int3。
- レンジ: `[0x5CA010,0x5CBC00)` ヘッド 1・gen 139／union 9 区間・gen 0。caller `0x57B8D9`(head `0x57B7E0`)。
- `0x58CEA0`=`ret 4`＋int3 → `0x58CEB0` ヘッド。
- `0x589080`/`0x588EF0` 実読。
- Value ctor `0x589DF0`：`+0x04/+0x08/+0x0C/+0x0E/+0x10/+0x14/+0x18/+0x1A` の全書込。
- probe `u47_1.py` v614 仕様に更新。
