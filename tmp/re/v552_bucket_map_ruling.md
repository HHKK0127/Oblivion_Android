# v552 — 属性バケット写像の全規則確定（**index 1..26=`a`–`z`（大小同一）／0=英字以外／27=`&`／`_` は別配列 `0xB13BC4`**）・**`6AA320` の alloc は要素 vtable `+4`（`401F00`＝operator new 経由）**・**`589D60`＝要素更新（`0x386`="image" のとき子 type 6 を `707530` で探し `4057B0`）**（V-552-1..3 完遂・旧残点 V-406-3 継続第82弾）

- 採番: **v552**（旧残点群消化・probe prefix **v552_1（1本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-552-1..3（v551 残点）** — 1=バケット index 0..27 と先頭文字の写像／2=衝突時の name 照合＋`6AA320` の allocator／3=`589D60` の `0x386` 判定と `707530`。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-552-1: バケット index 写像** | **[確定]（命令＋算術検証）** | `58A0A0`: 先頭文字 `al` が **`0x26`('&')** なら `esi=0x1B=27`（`589EB0` で `&entity;` 要素生成）／**`0x5F`('_')** なら**別配列 `0xB13BC4`（`NiTArray<StringListElement@Tile*>`）** へ `42D800`／それ以外は `movsx esi,al; sub esi,0x40; cmp esi,0x20; jle; sub esi,0x20` → `test esi,esi; jl → 0`、`cmp esi,0x1A; jle → 有効`、それ以外 `xor esi,esi`（0）。算術検証: **`a`–`z`（0x61–0x7A）→ 1–26、`A`–`Z`（0x41–0x5A）→ 1–26（大小同一）**、数字・記号（`1`=0x31 等）→ **0**、`&`→**27**、`_`→別配列。→ **`0xB3B0B0 + index*16` の 28 バケット（0–27）**。 |
| **V-552-2: `6AA320` の alloc と衝突照合** | **[確定]（命令）＝要素 vtable `+4` の allocator（node 12 バイト・`401F00`＝operator new）／照合なし（先頭挿入）** | `6AA320`: `mov eax,[esi]; mov edx,[eax+4]; call edx`＝**list の vtable `+4`（`0x46C0D0`: `push 0xC; call 401F00`＝node 12 バイト確保）で node 取得**、`node[8]=*arg`（値＝`StringListElement@Tile*`）、`node[4]=0`、`node[0]=[esi+4]`、`[esi+4]=node`、`[esi+0xC]++`（count）。→ **先頭挿入（LIFO）で、重複名の照合・置換は行わない**（`58A0A0` に name 比較ループは無い）。要素本体（`StringListElement@Tile` 16 バイト）は `58A0A0` が `push 0x10; call 401F00` で別途確保。→ **衝突時は同名エントリが複数積まれる**。[確定] |
| **V-552-3: `589D60` と `707530`** | **[確定]（命令）** | `589D60`: `[this+0x24]`→`[+0xB6]`（要素数）>0 なら `[+0xB0][0]`（先頭要素）を取り、`this->vtable[+0xC]() == 0x386`（**"image"**）のとき子要素 `707530(this, 6)`（**子 type 6 を探す**）→ 見つかれば `4057B0` を呼び `[this+0x2C] |= 0x20`（**フラグ 0x20 を立てる**）。続けて `[this+0x34]` のリストを辿り各要素の `vtable[+0x18]()` を呼ぶ（**子要素の更新**）。`707530(this,typeId)` は `[ecx+0x9C]` のリストを走査し `vtable[+0x4C]() == typeId` の要素を返す（**型 ID による子要素検索**。`typeId >= 0xA` で即 0）。 |

## §2 証拠

### §2.1 バケット写像（v552_1）
```
58A0D8: cmp al,0x26; jne 58A10E; mov esi,0x1B        ; '&' -> 27
58A10E: cmp al,0x5F; je 58A12D                       ; '_' -> 別配列 0xB13BC4
58A112: movsx esi,al; sub esi,0x40; cmp esi,0x20; jle 58A120; sub esi,0x20
58A120: test esi,esi; jl 58A129; cmp esi,0x1A; jle 58A0E1; 58A129: xor esi,esi
算術検証: a..z->1..26 / A..Z->1..26 / '1'(0x31)->0 / '&'->27 / '_'->別配列
```

### §2.2 `6AA320`（v552_1）
```
6AA320: mov eax,[esi]; mov edx,[eax+4]; call edx     ; vtable[+4] = alloc
  （list vtable +4 = 0x46C0D0: push 0xC; call 401F00 = node 12 バイト確保）
6AA330: mov [eax+8],[ecx]                            ; node.value = arg
6AA333: mov [eax+4],0
6AA33A: mov ecx,[esi+4]; mov [eax],ecx               ; node.next = head
6AA346: mov [ecx+4],eax / 6AA349: add [esi+0xC],1    ; head=node; count++
（要素本体 16 バイトは 58A0A0 が push 0x10; call 401F00 で確保）
```

### §2.3 `589D60`＋`707530`（v552_1）
```
589D63: mov eax,[esi+0x24]; test eax,eax; je end
589D6B: cmp word [eax+0xB6],0; ja → [eax+0xB0][0]（先頭要素）
589D81: mov edx,[esi]; mov eax,[edx+0xC]; call eax    ; this->vtable[+0xC]()
589D88: cmp eax,0x386                                 ; == "image" ?
589D97: call 707530(this, 6)                          ; 子 type 6 を検索
589DA4: call 4057B0 / 589DA9: or [esi+0x2C],0x20
589DAD: [esi+0x34] リスト走査 → 各 vtable[+0x18]()
707530: [ecx+0x9C] リスト走査; vtable[+0x4C]()==arg を返す（arg>=0xA は 0）
```

## §3 判定の内訳

- **V-552-1 完遂**: バケット index＝`a`–`z`(大小同一)=1–26、英字以外=0、`&`=27、`_`=別配列（`0xB13BC4`）。
- **V-552-2 完遂**: `6AA320` は vtable `+4`（`0x46C0D0`＝node 12 バイト alloc）で node 確保し先頭挿入（LIFO）。**重複照合なし**（同名は複数積まれる）。
- **V-552-3 完遂**: `589D60` は "image" 要素のとき子 type 6 を検索し `4057B0`＋フラグ `0x20` 設定。`707530`＝型 ID による子要素検索。

## §4 残点（V-553-1..3）

1. `58A0A0` の name 照合が無い件の帰結（同名属性の解決順＝先頭挿入 LIFO か、検索側 `588BD0` の実装で決まるか）【中】
2. `707530` の `[ecx+0x9C]` リストの所有クラス（`Tile`/`TileMenu` の子要素管理）と `vtable[+0x4C]`（型 ID getter）の実体【中】
3. `589D60` の `4057B0`（"image" 要素の初期化）と `[this+0x2C] |= 0x20` のフラグ意味【低】

## §5 後続への申し送り

- **移植側 UI 属性名表**: `0xB3B0B0 + index*16`（index: `a`–`z`=1–26、英字以外=0、`&`=27）、`_` 始まりは別配列 `0xB13BC4`。**重複名は先頭挿入で積まれる**（後勝ち検索にするなら検索側で先頭から）。
- **`Tile` 要素の型 ID**: `0x386`="image"／`0x389`="menu"（`TileMenu` の `+0x0C`）。子要素は `707530(typeId)` で型検索。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
