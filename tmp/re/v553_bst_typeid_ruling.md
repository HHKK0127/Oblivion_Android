# v553 — `588BD0`/`588C10` は**属性バケットとは別の BST（`[ecx+0x18]`）の getter**・**`0x386`="image" は `5818D0`（`mov eax,0x386; ret`＝型 ID 定数仮想）が返す**・**`4057B0`＝`NiTexturingProperty::Map` 系の確保（`[esi+0x20]` リスト＋`704100` ctor＋`42BB90` SetAt）**（V-553-1..3 完遂・旧残点 V-406-3 継続第83弾）

- 採番: **v553**（旧残点群消化・probe prefix **v553_1..3（3本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-553-1..3（v552 残点）** — 1=同名属性の解決順／2=`707530` の `[ecx+0x9C]` リストの所有クラス＋`vtable[+0x4C]`／3=`4057B0` と `[this+0x2C]` のフラグ `0x20`。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-553-1: `588BD0`/`588C10` の構造** | **[確定]（命令）＝属性バケットとは別の BST（`[ecx+0x18]`）の getter。`58A0A0` のバケットとは別系統** | `588BD0(this,key)`: `[ecx+0x18]`（BST ルート）から `node=[eax+8]`、`movzx ecx,word[node+0x18]` と `key` を比較し `jg`（node key > key）で左、`je` で `fld [node+4]`（float）、miss は `fldz`=0.0。`588C10` は同型の **pointer getter**（hit は `[node+8]`、miss は `0xA2F7EC`=空文字列）。→ **key は 16 ビット（word `[node+0x18]`）で、`58A0A0` の `0xB3B0B0` バケット（name→`StringListElement`）とは別の BST 構造**。callers 1073/57。`5877D0` 等が `push 0xFA1/0xFAB/0xFA5`（属性 ID）で呼ぶ。 |
| **V-553-2: `0x386`="image" の供給元** | **[確定]（命令）＝`5818D0` の `mov eax,0x386; ret`（型 ID 定数仮想）** | `589D60` が `this->vtable[+0xC]() == 0x386` で判定する `0x386` の供給元は **`5818D0`（`mov eax,0x386; ret`）**＝**`Tile` 派生の型 ID を返す定数仮想**（`581810` ctor 直後）。`0x386` は属性表 `58A369` の `push 0x386`（`58A1C0`）でも登録＝**UI 要素型 "image"**。`707530(this,typeId)` は `[ecx+0x9C]` のリストを走査し `vtable[+0x4C]() == typeId` を返す（callers 220）＝**型 ID による子要素検索**（`TileMenu`/`Tile` 共通の子要素管理）。 |
| **V-553-3: `4057B0` とフラグ `0x20`** | **[確定]（命令）** | `4057B0(this)`: `[esi+0x20]`（リスト）の先頭を調べ、空なら `push 0x10; call 401F00`（16 バイト確保）→ `704100`（**`vtable 0xA7DBB4`＝`.?AVMap@NiTexturingProperty@@` 系の ctor**）→ `42BB90`（`NiTArray::SetAt`）。→ **`NiTexturingProperty::Map` の確保**。`589D60` は "image" 要素でこれを呼び `[this+0x2C] に 0x20 を OR`（**テクスチャ更新フラグ**）。続けて `[this+0x34]` のリストを辿り各要素の `vtable[+0x18]()`（**子要素更新**）を呼ぶ。 |

## §2 証拠

### §2.1 `588BD0`（BST getter・v553_1）
```
588BD0: mov eax,[ecx+0x18]        ; BST root
588BE0: mov edx,[eax+8]           ; node payload
588BE6: movzx ecx,word [edx+0x18] ; node key (word)
588BEA: cmp ecx,esi(key); je → fld [edx+4]（hit, float）
588BF0: jg → miss（左へ）/ 588BF2: test eax,eax; jne 588BE0（次）
588BF6: fldz; ret 4               ; miss = 0.0
（588C10 は同型で hit=[edx+8]、miss=0xA2F7EC=空文字列）
```

### §2.2 `0x386` の供給元（v553_3）
```
5818D0: mov eax,0x386; ret        ; ★ 型 ID 定数仮想（"image"）
589D88: cmp eax,0x386             ; this->vtable[+0xC]() == 0x386
58A369: push 0x386                ; 属性表でも "image" として登録
707530: [ecx+0x9C] リスト走査; vtable[+0x4C]() == typeId を返す
```

### §2.3 `4057B0`（v553_2）
```
4057B0: mov eax,[esi+0x20]; mov edi,[eax]; test edi,edi; jne → 既存
4057DF: push 0x10; call 401F00    ; 16 バイト確保
4057F7: call 704100               ; Map ctor（vtable 0xA7DBB4 = .?AVMap@NiTexturingProperty@@）
405818: call 42BB90               ; NiTArray::SetAt
589DA9: [esi+0x2C] に 0x20 を OR   ; テクスチャ更新フラグ
```

## §3 判定の内訳

- **V-553-1 完遂**: `588BD0`/`588C10` は `[ecx+0x18]` の **BST getter**（key=word）。`0xB3B0B0` バケットとは別系統。→ 同名属性の解決順は **BST 側（key 一意）で決まり、バケットの重複は別用途** [確定]。
- **V-553-2 完遂**: `0x386`="image" は `5818D0`（定数仮想）。`707530`＝型 ID による子要素検索（callers 220）。
- **V-553-3 完遂**: `4057B0`＝`NiTexturingProperty::Map` 確保（`704100` ctor＋`42BB90`）。フラグ `0x20`＝テクスチャ更新。

## §4 残点（V-554-1..3）

1. `588BD0` の BST を所有するクラス（`[ecx+0x18]` を持つオブジェクト）と、`58A0A0` の `0xB3B0B0` バケット（name→`StringListElement`）との役割分担（どちらが「属性値」でどちらが「名前→ID」か）【中】
2. `707530` の `[ecx+0x9C]` リストを持つクラス（`TileMenu` か `Tile` か）と `vtable[+0x4C]`（型 ID getter）の実装【中】
3. `589D60` の `[this+0x34]` リスト（子要素リスト）の要素型と `vtable[+0x18]`（更新仮想）【低】

## §5 後続への申し送り

- **UI 属性値の 2 系統**: `0xB3B0B0` バケット（name→`StringListElement@Tile`）と `[ecx+0x18]` BST（word key→float/pointer）。移植側は両方を再現する必要がある（前者は名前→定義、後者は ID→値の想定）。
- **型 ID 定数仮想**: `Tile` 派生は `vtable[+0xC]() → 0x386`("image")/`0x389`("menu") 等を返す。子要素は `707530(typeId)` で検索。
- **`NiTexturingProperty::Map`**（`vtable 0xA7DBB4`）は `4057B0` で確保し `[this+0x2C]` の `0x20` フラグで更新通知。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
