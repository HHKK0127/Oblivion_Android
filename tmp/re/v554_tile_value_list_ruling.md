# v554 — `588BD0` は **BST ではなくソート済み `NiTList<Value@Tile*>` の線形走査**（v553 の「BST」を [確定負] に訂正）・**`TileImage`（vtable `0xA69110`）の `+0x14`＝`NiTList<Value@Tile*>`（プロパティ値・key=word）／`+0x30`＝`NiTList<Tile*>`（子要素）**・**`707530(this,typeId)` の `[ecx+0x9C]` は所有メニューの子要素リスト**（V-554-1..3 完遂・旧残点 V-406-3 継続第84弾）

- 採番: **v554**（旧残点群消化・probe prefix **v554_1..2（2本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-554-1..3（v553 残点）** — 1=`588BD0` の BST 所有クラスと `0xB3B0B0` バケットとの役割分担／2=`707530` の `[ecx+0x9C]` 所有クラス＋`vtable[+0x4C]`／3=`589D60` の `[this+0x34]` 子要素リスト。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-554-1: `588BD0` の所有構造と役割分担** | **[確定]（命令）＝ソート済み `NiTList<Value@Tile*>` の線形走査（v553 の「BST」は [確定負]）** | `588BD0`: `mov eax,[ecx+0x18]`（**リスト head**）→ `mov edx,[eax+8]`（payload）→ `movzx ecx,word[edx+0x18]`（key）→ `cmp ecx,esi` → **`mov eax,[eax]`（next へ前進）**、`jg` で miss。**`node[0]`=next の単方向線形走査**（`6AA320` の挿入形式 `node[0]=head` と一致）＝**ソート済み `NiTList` 検索であり BST ではない**。payload は `Value@Tile`（`+4`=float／`+8`=pointer／`+0x18`=word key）。**役割分担**: `0xB3B0B0` バケット＝**属性名→`StringListElement@Tile`（グローバル名前表）**、`Tile` の `+0x14 NiTList<Value@Tile*>`＝**インスタンスの属性値（属性 ID→値）**。 |
| **V-554-2: `707530` の `[ecx+0x9C]`** | **[確定]（命令）＝所有メニュー（`TileMenu`）の子要素リスト `NiTList<Tile*>`** | `589D60` は `[this+0x24]`（**所有メニュー**）を取り、`[menu+0xB6]`（配列要素数）>0 なら `[menu+0xB0][0]`（先頭要素）を `edi` に。`707530(edi, 6)` は `[edi+0x9C]` を `NiTList<Tile*>` として走査し `payload->vtable[+0x4C]() == 6` を返す。→ **`[ecx+0x9C]`＝メニューの子要素リスト**（`Tile` の `+0x34` とは別に、メニューが全子を保持）。`[ecx+0x9C]` は head（`node[0]`=next／`node[8]`=payload）。 |
| **V-554-3: `[this+0x34]` 子要素リスト** | **[確定]（命令＋ctor）＝`NiTList<Tile*>`（vtable `0xA690AC`）** | `TileImage` ctor `581810`: `[esi+0x14]=0xA6909C`（**`NiTList<Value@Tile*>`**・head=`+0x18`）／`[esi+0x30]=0xA690AC`（**`NiTList<Tile*>`**・head=`+0x34`）。`589D60` は `[this+0x34]`（`NiTList<Tile*>` head）を辿り、`5A8260`/`5A8270`/`5A8280`（**singleton 3 種**）と比較しつつ各要素の `vtable[+0x18]()`（更新）を呼ぶ。→ **子要素は `Tile`（`NiTList<Tile*>`）**。 |

## §2 証拠

### §2.1 `588BD0`＝ソート済み NiTList 走査（v554_1）
```
588BD0: mov eax,[ecx+0x18]         ; head
588BE0: mov edx,[eax+8]            ; payload (Value@Tile)
588BE6: movzx ecx,word [edx+0x18]  ; payload.key (word)
588BEA: cmp ecx,esi; je hit        ; hit → fld [edx+4]
588BEC: mov eax,[eax]              ; next = node[0]  ← 線形前進（BST の左右分岐ではない）
588BF0: jg miss; test eax,eax; jne 588BE0
588BF6: fldz; ret 4                ; miss = 0.0
```

### §2.2 TileImage ctor（v554_2）
```
581810: ... TileImage ctor
581847: [esi+0x20]=0 / 58184A: [esi+0x18]=0 / 58184D: [esi+0x1C]=0
581850: [esi+0x14]=0xA6909C        ; NiTList<Value@Tile*> vtable（head=+0x18）
581857: [esi+0x3C]=0 / 58185A: [esi+0x34]=0 / 58185D: [esi+0x38]=0
581860: [esi+0x30]=0xA690AC        ; NiTList<Tile*> vtable（head=+0x34）
581873: [esi]=0xA69110              ; TileImage vtable
5818D0: mov eax,0x386; ret         ; 型 ID "image"
0xA69110 → .?AVTileImage@@ / 0xA6909C → NiTList<Value@Tile*> / 0xA690AC → NiTList<Tile*>
```

### §2.3 `589D60`（v554_1）
```
589D63: mov eax,[esi+0x24]         ; 所有メニュー
589D6B: cmp word [eax+0xB6],0      ; メニュー配列の要素数
589D79: mov eax,[eax+0xB0]; mov edi,[eax]  ; 先頭要素
589D97: call 707530(edi, 6)        ; [edi+0x9C] を走査し型 6 を検索
589DAD: [this+0x34]（NiTList<Tile*> head）を辿る
589DBC/DC5/DCE: 5A8260/5A8270/5A8280（singleton と比較）
589DD9: payload->vtable[+0x18]()   ; 子要素更新
```

## §3 判定の内訳

- **V-554-1 完遂＋訂正**: `588BD0` は **ソート済み `NiTList<Value@Tile*>` の線形走査**（v553 の「BST」は [確定負]）。役割分担＝グローバル名前表（`0xB3B0B0`）とインスタンス属性値（`+0x14 NiTList<Value@Tile*>`）。
- **V-554-2 完遂**: `707530` の `[ecx+0x9C]`＝所有メニュー（`TileMenu`）の子要素リスト（`NiTList<Tile*>`）。
- **V-554-3 完遂**: `[this+0x34]`＝`NiTList<Tile*>`（vtable `0xA690AC`）。`TileImage`（`0xA69110`）の子要素。

## §4 残点（V-555-1..3）

1. `Value@Tile` の完全レイアウト（`+4`=float／`+8`=pointer／`+0x18`=word key 以外のフィールド）と `0x14 NiTList<Value@Tile*>` への挿入箇所（属性値の設定側）【中】
2. `TileMenu` の `+0xB0` 配列（`[+0xB6]`=要素数）の要素型と、`[+0x9C]` 子要素リストとの使い分け【中】
3. `5A8260`/`5A8270`/`5A8280`（singleton 3 種、`0xB3B350`/`0xB3B354`/`0xB3B358`）の型と「子要素更新から除外する要素」の意味【低】

## §5 後続への申し送り

- **`Tile` の 2 リスト**: `+0x14 NiTList<Value@Tile*>`（属性値・key=word）と `+0x30 NiTList<Tile*>`（子要素）。移植側は両方を持つこと。
- **`588BD0` はソート済み NiTList の線形検索**（BST ではない）——key は payload `+0x18` の word、値は `+4`(float)/`+8`(pointer)。
- **`707530(menu, typeId)`** はメニューの `+0x9C` 子要素リストを型 ID（`vtable[+0x4C]`）で検索。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
