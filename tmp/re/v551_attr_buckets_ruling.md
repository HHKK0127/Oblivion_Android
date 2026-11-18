# v551 — 属性表の実体＝**28 個の `NiTList<StringListElement@Tile*>` バケット配列（`0xB3B0B0` 起点・各 16 バイト）**と確定・**`0xB3B0CC`＝`0xB3B0B0+0x1C`（bucket[1] の count）＝BSS で読み 1・書き 0、`58DA55` は一度きり初期化ガード**・**`TileMenu` の他 slot（`+0x0C`/`+0x10`）は窓 ID `0x389`(905 "menu") と `0xA6AF30` を返す定数仮想**・**ハーネス `find_off` の BSS バグを修正**（V-551-1..3 完遂・旧残点 V-406-3 継続第81弾）

- 採番: **v551**（旧残点群消化・probe prefix **v551_1..4（4本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-551-1..3（v550 残点）** — 1=`0xB3B0CC` の実体＋`58DA55` ガード／2=`589EB0` の生成物（`StringListElement@Tile` 16B）の構造＋`6AA320` list の走査側／3=`TileMenu` の他 slot＋`589D60`。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-551-2: 属性表の実体＝28 `NiTList` バケット** | **[確定]（RTTI＋ctor 実読）** | `0xB3B0B0` は **`984BCF`（配列 ctor・要素 ctor `589860`）で構築される 28 要素 × 16 バイトの配列**（`push 0x1c(28); push 0x10(16); push 0xB3B0B0; call 984BCF`）。要素 ctor `589860` は `[eax]=0xA6A1BC`＝**`NiTList<StringListElement@Tile*>`**（vtable `0xA6A1BC`→TD `.?AV?$NiTList@PAVStringListElement@Tile@@@@`。`+4/+8/+0xC=0`＝head/tail/count）。→ **`0xB3B0B0 + index*16` が属性名の先頭文字インデックスに対応する `NiTList` バケット**（`58A189`: `shl ecx,4; add ecx,0xB3B0B0`）。`6AA320` は `[esi]`(vtable) の `+4`（alloc）を呼び node を先頭挿入（`[esi+0xC]`＝count++）。走査は `589890`（`[this+0x24]`→`[+0x10]` 配列→`NiTList` を辿り `0xB3FD44` を探す）。`589EB0` は `StringListElement@Tile`（16B: `[0]`=nameptr, `[4]`=?, `[8]`/`[0xC]`=0, `[0xE]`=word）を生成。 |
| **V-551-1: `0xB3B0CC` の実体** | **[確定]（セクション表＋全 disp32 走査）＝`.data` の BSS 内、属性バケット配列の bucket[1] `+0xC` フィールド** | `0xB3B0CC` の命令参照は **`58DA55` の `cmp dword [0xB3B0CC], ebx`（読み）1 箇所のみ**で**書きは 0 件**。`0xB3B0CC` は **`.data` の仮想範囲内だが raw 範囲外＝BSS（ファイル上ゼロ）**、かつ **`0xB3B0B0 + 0x1C` ＝ 属性バケット配列の bucket[1] の `+0xC` フィールド**（`6AA320` が `add [esi+0xC],1` で間接更新）。→ **v551_1/3 の「高エントロピー＝難読化」はハーネス `find_off` のバグ**（`max(vs,rsz)` で raw 外を次セクション `.bind` から誤読）で、実際はゼロ。`58DA55` の `cmp …; jne skip` は「バケットが空＝未登録なら `58A1C0` を呼ぶ」**一度きり初期化ガード** [確定]。 |
| **V-551-3: `TileMenu` の他 slot と `589D60`** | **[確定]（命令）** | `TileMenu`（`0xA6AF14`）: `+0x0C`=`5900D0`＝**`mov eax,0x389; ret`**（窓 ID 905="menu" を返す定数仮想）／`+0x10`=`5900E0`＝**`mov eax,0xA6AF30; ret`**（別 vtable/定数返し）／`+0x08`=`591C20`＝`591DC0` を呼び `707530`（`+0x50`=0.0f, `+0x54`++）＝**アニメ/フェード初期化**。`589D60` は `[this+0x24]`→要素列の先頭を取り、`vtable[+0x0C]() == 0x386`(902="image"?) のとき `707530` で処理＝**画像要素の更新**。dtor `5900F0`→`591B70`。 |

## §2 証拠

### §2.1 28 バケット配列（v551_1/2/3）
```
9FBCA0: push 0x589880; push 0x589860; push 0x1c(28); push 0x10(16); push 0xB3B0B0; call 984BCF
A24A60: push 0x589880; push 0x1c; push 0x10; push 0xB3B0B0; call 98124F   ; dtor
589860: mov [eax+0xC]=0; [eax+4]=0; [eax+8]=0; mov [eax],0xA6A1BC; ret   ; 要素(NiTList) ctor
0xA6A1BC COL=0xAC1C10 TD=0xB13D58 = '.?AV?$NiTList@PAVStringListElement@Tile@@@@'
58A189: shl ecx,4; add ecx,0xB3B0B0; call 6AA320                          ; バケットへ挿入
6AA320: [esi]->vtable[+4](alloc); node[8]=*arg; node[4]=0; node[0]=[esi+4]; … [esi+0xC]++
589890: [this+0x24]->[+0x10] 配列 → NiTList 走査（0xB3FD44 探索）
```

### §2.2 `0xB3B0CC` は BSS 内の bucket[1].count（v551_1/3/4＋ハーネス修正）
```
.data  VA 0xB02000..0xBABC1C (vsize 0xA9C1C) / raw 0x700C00..0x731800 (rsize 0x30C00)
  → raw が覆う VA は 0xB02000..0xB32C00 のみ。0xB3B0B0/0xB3B0CC は raw 外＝BSS（ファイル上ゼロ）
0xB3B0CC = 0xB3B0B0 + 0x1C = bucket[1] + 0xC（bucket[1] = 先頭文字 'a' のバケット）
  （'a'=0x61 → 0x61-0x40=0x21 > 0x20 → 0x21-0x20=1。バケット 1..26 = a-z、27 = '&' エンティティ、0 = 未使用）
58DA53: cmp dword [0xB3B0CC], ebx; jne 0x58DA62; call 58A1C0
  → 58A1C0 が 'abs'/'add'/'alpha'… を bucket[1] へ登録すると 6AA320 の `add [esi+0xC],1` で
    0xB3B0CC が非 0 になり、2 回目以降はスキップ＝一度きり初期化ガード
全 disp32 走査: 0xB3B0CC は読み 1 件（58DA55）・書き 0 件（間接更新のみ）
★ ハーネス find_off のバグ: `sva <= rva < sva + max(vs, rsz)` だと raw 外を
  次セクション（.bind raw 0x733200..）から読み、0xB3B0B0 に高エントロピー値が出ていた。
  修正: 仮想範囲は vs、実データは raw 範囲のみ（raw 外は None＝ゼロ）。
```

### §2.3 `TileMenu` 他 slot（v551_1）
```
0xA6AF14: +0x08 0x591C20（591DC0→707530: [+0x50]=0.0f, [+0x54]++）
          +0x0C 0x5900D0（mov eax,0x389; ret ＝窓 ID 905="menu"）
          +0x10 0x5900E0（mov eax,0xA6AF30; ret）
          +0x00 0x5900F0（dtor → 591B70）
589D60: [this+0x24]->要素先頭; vtable[+0x0C]()==0x386 ? 707530 : skip
```

## §3 判定の内訳

- **V-551-2 完遂**: 属性表＝`0xB3B0B0` 起点の **28 `NiTList<StringListElement@Tile*>` バケット**（要素 ctor `589860`・配列 ctor `984BCF`）。`6AA320` は先頭挿入、`589890` が走査。
- **V-551-1 完遂**: `0xB3B0CC`＝`0xB3B0B0+0x1C`＝bucket[1]（'a'）の count フィールド（書き 0 件・`6AA320` が間接更新）。`58DA55` は一度きり初期化ガード。**「難読化」はハーネス `find_off` のバグ（raw 外を次セクションから誤読）だった**→修正済み。

## §4 残点（V-552-1..3）

1. バケット index の完全な対応（`0..27` と先頭文字の写像。`&` エンティティ＝27 で確定か、0 と 2..26 の境界）の実測【中】
2. `NiTList<StringListElement@Tile*>` の衝突時の照合（`58A0A0` の name 比較）と `6AA320` の alloc が使う allocator【中】
3. `589D60` の `0x386` 判定（要素型 902）と `707530`（アニメ値更新）の引数意味【低】

## §5 後続への申し送り

- **移植側 UI 属性名表**: `0xB3B0B0` の 28 バケット（各 `NiTList<StringListElement@Tile*>`）に属性名を先頭文字インデックスで格納。要素は `StringListElement@Tile`（nameptr＋属性値）。
- **`TileMenu`（`.?AVTileMenu@@`）**: `+0x0C` は窓 ID 905（"menu"）を返す定数仮想、`+0x14` が open（v550）、`+0x18` が HUD/ローディング ガード。
- **`.data` には BSS 領域がある**——raw（ファイル）範囲外の VA は起動時にゼロから書かれる。静的な値の読出時は「raw 範囲内か」を必ず確認（`0xB3B0B0`/`0xB3B0CC` は BSS）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
