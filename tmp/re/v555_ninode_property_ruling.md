# v555 — **`Value@Tile` の完全レイアウト（0x1C バイト・`+0`=所有 Tile／`+4`=float／`+8`=ポインタ／`+0x18`=word ID／`+0x1A`=byte）＋挿入は `58B220` GetOrCreate**／**`589D60` の `[this+0x24]` は所有メニューではなく `NiNode`（v554 を [確定負] に訂正）／`707530` の `[ecx+0x9C]` は NiAVObject のプロパティリスト（`+0x98`=`NiTPointerList<NiPointer<NiProperty>>`）**／**`0xB3B350/354/358` は id=1/2/3 の子へのキャッシュ（v554 の「singleton 3 種」を [部分確定] に訂正）**（V-555-1..3 完遂・旧残点 V-406-3 継続第85弾）

- 採番: **v555**（旧残点群消化・probe prefix **v555_1..16（16本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-555-1..3（v554 残点）** — 1=`Value@Tile` の完全レイアウトと挿入箇所／2=`TileMenu` の `+0xB0` 配列要素型と `[+0x9C]` との使い分け／3=`5A8260/5A8270/5A8280`（`0xB3B350/354/358`）の型と除外意味。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-555-1: `Value@Tile` の完全レイアウトと挿入箇所** | **[確定]（ctor 589DF0＋setter 58B220）** | payload は **0x1C バイト**: `+0x00`=所有 `Tile*`／`+0x04`=float 値／`+0x08`=ポインタ値（文字列 or オブジェクト）／`+0x0C`=word 0／`+0x0E`=word 0／`+0x18`=word 属性 ID／`+0x1A`=byte フラグ。**生成は `58B220(this,id)`（=GetOrCreate）**: `[this+0x18]` のソート済みリストを線形走査 → 見つからなければ `0x1C` を確保して ctor `589DF0(id)` → `*payload = this`（所有者）→ 初めて key が id を超えたノードの**直前**へ `589640` で挿入（無ければ `5B1E20` で**末尾 append**）。値の設定は `58CEB0`→`58B220`＋`58CA00`(float)、`58CED0`→`58B220`＋`58CA50`。 |
| **V-555-2: `TileMenu` の `+0xB0` 配列と `[+0x9C]` の使い分け** | **[確定]（ただし前提を訂正）＝`+0xB0` は `MapMenu` の埋め込み `NiTArray`（base/max/size = `+0xB0/+0xB4/+0xB6`）、`[+0x9C]` は `NiAVObject` のプロパティリスト** | `+0xB0/+0xB4/+0xB6` は **2 つのクラスが同型で持つ**: ①`MapMenu`（vtable `0xA6CC9C`・ctor `5B7360`・alloc `0x100`・dtor `5B7490` が `[+0xB0]` を解放）②`NiNode`（vtable `0xA7E38C`・ctor `70B780`・alloc `0xDC`・dtor `70B810` が `[+0xB0]` を解放）。**`589D60` が使うのは②**（後述の訂正）。`MapMenu` の要素は `5BB880`（MapMenu vtable `+0x0C`）が属性 **`0xFB2`="user4"** のポインタ値を `588C10(tile,0xFB2)` で取得し `488810`（→`4028D0`）で配列へ追加する。`5B7550`（MapMenu 更新）は `size` 回ループし各要素を `menu->vtable[+0x88](menu, element, out)` に渡す。**`[+0x9C]` は `NiAVObject` のプロパティリストの head**（`+0x98` が vtable `0xA7DF14`=`NiTPointerList<NiPointer<NiProperty>>`）。 |
| **V-555-3: `0xB3B350/354/358` の型と除外意味** | **[確定]＝id 属性（`0xFA8`="id"）1/2/3 の子へのキャッシュ（「singleton 型」ではない）** | `5A8000(this)` が **`5894F0(this,1/2/3)`** の結果を `0xB3B350`(id=1)／`0xB3B358`(id=2)／`0xB3B354`(id=3) に格納。`5894F0` は「自ノードの属性 `0xFA8` が引数と一致すれば自身を返し、さもなくば `[+0x34]` 子リストを再帰探索」＝**id による子の深さ優先探索**。取得後 `0xFB1`="user3" を id1 から、`0xFB7`="user9" を id2/id3 から読んで `0xB140BC/0xB140C0/0xB140C4` にキャッシュ。setter `5A8090(which,ptr)` が 1/2/3 を書き換える。`589D60` はこの 3 つを子更新ループから除外する（二重更新防止）。BSS 初期値 0。 |

## §2 証拠

### §2.1 `Value@Tile` ctor `589DF0`（v555_14）
```
589DF0: ...                        ; this = payload（0x1C バイト、58B220 が alloc）
589E1B: [esi+8]=0 / [esi+0xC]=0 / [esi+0xE]=0
589E26: fldz; [esi+4]=0.0f
589E28: ax = word [esp+0x20]       ; 引数 = 属性 ID
589E36: word [esi+0x18]=ax
589E3A: dword [esi]=0              ; 所有者 Tile*（58B220 が後で this を書く）
589E30: push 0x18; call 401F00     ; リストノード 0x18 バイト
589E48: node[0]=0 (next) / node[4]=0 (prev) / node[8]=payload
589E50: node[0xC]=0x65             ; リスト要素タグ
589E57: node[0x10]=0 / node[0x14]=0
```

### §2.2 GetOrCreate `58B220`（decompiled 295703＋命令）
```
puVar4 = [this+0x18]; puVar7 = [this+0x18];      ; head
while (node) { payload=node[8]; next=node[0];
               if (payload->key == id) goto hit;
               if (payload->key < id) break; }    ; ソート順
miss: param_2 = operator new(0x1c)               ; 295739: FUN_00401f00(0x1c)
      piVar6 = Value@Tile::ctor(id)              ; 295745: FUN_00589df0(uVar3)
      *piVar6 = this;                            ; 295749: payload+0 = 所有者
      if (found_pos) FUN_00589640(prev,&p)       ; 初めて key>id のノードの直前へ（昇順維持）
      else           FUN_005b1e20(&p)            ; 末尾へ append
```

挿入ヘルパの実体（v555_18）: リストオブジェクトは `+4`=head／`+8`=tail／`+0xC`=count、ノードは `+0`=next／`+4`=prev。
```
589640(list, at, &payload):  node[8]=*payload; node[0]=at; node[4]=at[4];
                             if (at[4]) at[4][0]=node; else list[4]=node;
                             at[4]=node; list[0xC]++      ← at の直前へ
5B1E20(list, &payload):      node[8]=*payload; node[0]=0; node[4]=list[8];
                             if (list[8]) list[8][0]=node; else list[4]=node;
                             list[8]=node; list[0xC]++    ← 末尾 append
6AA320(list, &payload):      node[0]=list[4]; node[4]=0;
                             if (list[4]) list[4][4]=node; list[4]=node  ← 先頭 prepend
```

### §2.3 getter ファミリ（v555_12）
```
588B50(this,id) -> Value@Tile*   （[this+0x18] を next のみ前進、key=[payload+0x18]）
588BD0(this,id) -> float         （hit: [payload+4]、miss: 0.0）
588B80(this,id) -> bool          （hit かつ !(byte[+0x1A]==0 && float[+4]==0.0)）
588C10(this,id) -> pointer       （hit: [payload+8]、miss: 0xA2F7EC=空文字列）
```
※ `588B50` は payload を返し、float/ptr は呼び出し側が `+4`/`+8` を読む（例 `5BB943: call 588b50; fld [eax+4]`）。

### §2.3b float setter `58CA00`（v555_18）
```
58CA00: byte [esi+0x1A] = 1        ; 「設定済み」フラグ
        if (新値 != 旧値[+4]) or (id == 0xFDE="string"):
            4028D0(payload+8, "", 0)   ; 文字列をクリア
        [payload+4] = 新値; 588930(payload); ...
```
→ `+0x1A` は**設定済みフラグ**（`588B80` は「フラグ 0 かつ float 0.0」を未設定と判定）。

### §2.4 `[this+0x24]` = NiNode（v555_9/10/11/16）
```
TileImage vtable 0xA69110: +0x08 = 591600 / +0x18 = 589D60   (v555_11 (a))
Tile3D   vtable 0xA6AEEC: +0x08 = 590C20 / +0x18 = 589D60
Tile     vtable 0xA690E8: +0x18 = 589D60

591600 (TileImage+0x08):
  591920: push 0xDC; call 401F00        ; NiNode 0xDC バイト
  59193E: call 70B780                   ; NiNode ctor（vtable 0xA7E38C）
  591947: [ebp+0x24] = eax               ; ← this+0x24 = NiNode*
590C20 (Tile3D+0x08):
  590C61: push 0xDC; call 401F00 / 590C82: call 70B780
  590C96: [esi+0x24] = eax
```
`NiNode` ctor `70B780`: `call 708450`（NiAVObject ctor）→ `[esi]=0xA7E38C`、`lea ecx,[esi+0xAC]; call 4B2D30`（埋め込み配列初期化）。`NiNode` dtor `70B810`: `[ebp+0xB0]` を `-4` ヘッダ付きで解放、`word [ebp+0xB6]` を子数として `vtable[0](child,1)` をループ。

### §2.5 `NiAVObject` の `+0x98..+0xA8`（v555_12）
```
708450: [esi+0x98] = 0xA7DF14   ; NiTPointerList<NiPointer<NiProperty>> vtable
708491: [esi+0x9C] = 0          ; ← プロパティリスト head
70848B/497: [esi+0xA4]=0 / [esi+0xA0]=0
7084B5: [esi+0x1C]=0 / word [esi+0x18]=0（フラグ）
```

### §2.6 `707530(obj,type)`（v555_9）
```
707530: if (type >= 0xA) return 0
        node = [obj+0x9C]
        while (node) { p = node[8]; node = node[0];
                       if (!p) continue;
                       if (p->vtable[+0x4C]() == type) return p; }
        return 0
NiTexturingProperty vtable 0xA7DB5C: +0x4C = 0x7AFE90 = mov eax,6; ret   (v555_15)
NiMaterialProperty  vtable 0xA7E1F4: +0x4C = 0x8CF6B0 = mov eax,2; ret
```
→ `707530` ＝ **型インデックスによるプロパティ検索**。`589D60` は `707530(firstChild, 6)`＝テクスチャプロパティを取り、`4057B0(tex,0)` の後 `[this+0x2C] |= 0x20`（テクスチャ更新フラグ。`4057B0`＝`Map@NiTexturingProperty` 系 ctor 経由の Map スロット確保＝v553）。

### §2.7 `MapMenu` の `+0xB0` 配列（v555_6/8/16）
```
5B7360 (MapMenu ctor, alloc 0x100 は factory 587D20 の "push 0x100"):
  5B739A: [esi+0xB0]=0 / 5B73A0: word[esi+0xB4]=0 / 5B73A7: word[esi+0xB6]=0
  5B7390: [esi]=0xA6CC9C  (MapMenu)
5B7490 (dtor): 5B74F6 [esi+0xB0] → 401F20(free) → 5B7507/511/51A クリア
5B7550 (update): 5B75E2 movzx eax,word[ebp+0xB6]        ; size
                 5B7602 mov eax,[ecx+esi*4]             ; element = base[i]
                 5B7609 vtable[+0x88](menu, element, out)
5BB880 (MapMenu vtable +0x0C):
  5BB922: push 0xFB2; mov ecx,edi(tile); call 588C10    ; "user4" pointer
  5BB929: lea ebx,[esi+0xB0]; 5BB937: call 488810       ; NiTArray add(item,0)
5B7550 (nested): 5B78AB/78B9/78C6/78D0 は element の +0xB6/+0xB0 を同型に辿る
               5B78E0: test byte [element+0x18],1        ; NiAVObject フラグ
```
RTTI: `0xA6CC9C`→`.?AVMapMenu@@`。MapMenu type-id は vtable `+0x34` = `5B7480: mov eax,0x3FF; ret`＝1023 で、名前表 `58A1C0` の `FUN_0058a0a0("&MapMenu;",0x3ff)`（decompiled 295429）と一致。

### §2.8 id 探索とキャッシュ（v555_13＋decompiled 313923）
```
5894F0(this,id):  if (this && 588BD0(0xFA8)==id) return this;
                  for (child = [this+0x34]; child; child=child[0])
                      r = 5894F0(child[8], id); if (r) return r;
                  return 0
5A8000(this):     0xB3B350 = 5894F0(this,1)
                  0xB3B358 = 5894F0(this,2)
                  0xB3B354 = 5894F0(this,3)
                  0xB140BC = 588BD0(id1, 0xFB1)   ; "user3"
                  0xB140C0 = 588BD0(id2, 0xFB7)   ; "user9"
                  0xB140C4 = 588BD0(id3, 0xFB7)
5A8090(which,ptr): which==1 -> 0xB3B350 / 3 -> 0xB3B354 / 2 -> 0xB3B358
5A8260/70/80:     mov eax,[0xB3B350]/[0xB3B354]/[0xB3B358]; ret
```

### §2.9 名前表の実読（decompiled 295493..295550）
```
"class"=0xFA2 / "id"=0xFA8 / "stackingtype"=0xFA5 / "user3"=0xFB1 / "user4"=0xFB2 / "user9"=0xFB7
0xA6A49C = "id"   0xA6B178 = "ERROR: Cant find menu class!"
```
（`591C50`: 属性 `0xFA2`="class" の文字列→`587B20`/`587D20`（1001–1051 のメニュー生成 switch）→成功時 `[this+0x44]=生成物`、失敗時 `"ERROR: Cant find menu class!"` を `585C10`/`585F40` で通知。`0x44` の意味も同時に確定。）

## §3 判定の内訳・訂正

- **V-555-1 完遂**: `Value@Tile` 0x1C バイトの全フィールド＋`58B220` の生成/挿入＋setter/getter の系列を確定。**v554 の「単方向リスト」は [部分確定]→「ノードは next `+0`/prev `+4` の双方向、走査は next のみ」に精緻化**（`589DF0` が node `+4` を 0 初期化し、挿入 `589640` が prev を繋ぐ）。
- **V-555-2 完遂＋前提訂正**: `+0xB0/+0xB4/+0xB6` は `NiTArray`（base/max/size）で、**`MapMenu` と `NiNode` が同型に持つ**。`MapMenu` 側の要素は `user4`(0xFB2) のポインタ。**`[+0x9C]` は MapMenu/TileMenu の子要素リストではない**。
- **v554 §1 V-554-2 を [確定負]**: `707530` の `[ecx+0x9C]` は「所有メニューの子要素リスト」ではなく **`NiAVObject` のプロパティリスト**（`+0x98`=`NiTPointerList<NiPointer<NiProperty>>`、`+0x9C`=head）。`589D60` の `[this+0x24]` も「所有メニュー」ではなく **`NiNode*`**（`591600`/`590C20` が `70B780` で生成して格納）。根拠は §2.4〜§2.6 の命令と RTTI（`0xA7DF14`/`0xA7DB5C`）。
- **V-555-3 完遂＋訂正**: 3 グローバルは「singleton 3 種」ではなく **id=1/2/3 の子ノードのキャッシュ**。`5A8260/70/80` は単なる getter（`mov eax,[global]; ret`）で、型は探索結果（`Tile*`）。除外の意味は**子更新ループでの二重更新防止**。

## §4 残点（V-556-1..3）

1. `589640`／`5B1E20`（`Value@Tile` リストの挿入ヘルパ）と `488810`→`4028D0`（`NiTArray` 追加）の実装差（prev 連結・grow 条件）【中】
2. `MapMenu` 要素（`user4` のポインタ）の実型と `vtable[+0x88]`（`5B7550` が呼ぶ更新仮想）の実装、および `+0xB4`(max) の grow 契機【中】
3. `4057B0(tex,0)` の完全な意味（`Map@NiTexturingProperty` のスロット 0 確保か取得か）と `[this+0x2C] |= 0x20` の消費側【低】

## §5 後続への申し送り

- **`Value@Tile`**: payload 0x1C バイト＝`+0` 所有者 `Tile*`／`+4` float／`+8` ポインタ／`+0x18` word 属性 ID／`+0x1A` byte。ノードは 0x18 バイト（next/prev/payload/tag `0x65`）。
- **属性の設定は必ず `58B220` 経由**（GetOrCreate）。`58CEB0`（float）／`58CED0`（その他）が入口。
- **`589D60`（Tile 更新仮想 `+0x18`）の順序**: ①`[this+0x24]`=NiNode の先頭子 → ②その `NiTexturingProperty`(6) を `707530` で取得 → ③`4057B0` → ④`[this+0x2C] |= 0x20` → ⑤`[this+0x34]` の子 Tile を 3 つの id キャッシュ（1/2/3）を除いて `vtable[+0x18]()`。
- **`TileMenu+0x44`＝属性 `class`(0xFA2) から生成したメニュー要素**（`587D20` の ID 1001–1051 switch）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
