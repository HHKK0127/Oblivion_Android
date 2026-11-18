# v550 — クロップ設定＝**`iSafeZoneX/Y/XWide/YWide:Interface`（既定 20）**と確定・**`591C50` は `.?AVTileMenu@@`（vtable `0xA6AF14`）の slot +0x14＝open 仮想**・**属性表の実体＝`0xB3B0B0` 起点の 16 バイト stride スロット配列＋`NiTArray<StringListElement@Tile*>`（`0xB13BC4`）**（V-550-1..3 完遂・旧残点 V-406-3 継続第80弾）

- 採番: **v550**（旧残点群消化・probe prefix **v550_1..4（4本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-550-1..3（v549 残点）** — 1=`57D330`/`57D390` が参照する `0xB135F8`/`0xB13600`/`0xB13608`/`0xB13610` の設定名＋クロップ計算式／2=`0x58A0A0` の登録先（`0xB3B0CC` が指すオブジェクトのクラス）＋`589EB0`／3=`591C50` の仮想 slot（vtable `0xA6AF28`）の帰属クラス＋姉妹ハンドラ。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-550-1: クロップ設定名** | **[確定]（メモリ実読＋getter）＝`iSafeZone*:Interface`（既定 20）** | `0xB135E0..0xB1361C` は **(値, nameptr) 対**（8 バイト stride・値が先）: **`0xB135F8`=20→`iSafeZoneX:Interface`／`0xB13600`=20→`iSafeZoneY:Interface`／`0xB13608`=20→`iSafeZoneXWide:Interface`／`0xB13610`=20→`iSafeZoneYWide:Interface`**。getter `0xA24820`/`0xA24880`（`push 0xB135F8/0xB13608; mov ecx,0xB07CFC; call 65C620`）が各 name の値域を照会。→ `57D330` は「ワイド画面か」を `[0xB06C50=480]/[0xB06C4C=640]=0.75 > [0xA68D08]=0.74` で判定し **`iSafeZoneXWide`(0xB13608) or `iSafeZoneX`(0xB135F8)** を返す（`57D390` は Y 側）。**クロップ計算式＝セーフゾーン設定そのもの**（v549 の「アスペクト比クロップ」は [部分確定]）。 |
| **V-550-3: `591C50` の仮想 slot と帰属クラス** | **[確定]（RTTI 実測）＝`.?AVTileMenu@@`（vtable `0xA6AF14`）の slot +0x14** | `0xA6AF14` COL `0xAC1D98`→TD **`.?AVTileMenu@@`**。slot: **`+0x00`=dtor `0x5900F0`／`+0x08`=`0x591C20`／`+0x14`=`0x591C50`（open）／`+0x18`=`0x591B40`**（他は vtable 末端の非ポインタ値）。`591B40` は `[this+0x44]`（menu）の窓 ID（`vtable[+0x34]`）を **`0x3EC`(1004 HUDMainMenu)/`0x3EF`(1007 LoadingMenu)** と比較し、一致しなければ `589D60` へ jmp＝**HUD/ローディング中は処理をスキップ**。callers は全て 0＝**vtable 経由のみ**。 |
| **V-550-2: 属性表の実体** | **[確定]（命令）＝`0xB3B0B0` 起点の 16 バイト stride スロット配列＋`NiTArray<StringListElement@Tile*>`（`0xB13BC4`）** | `0x58A0A0` 末尾: name 先頭 `0x26`('&') なら `589EB0` で `&entity;` 要素を生成（`alloc(0x10)`）。文字コード `al−0x40`（`0x1A` 超は `−0x20`、負は 0）→ `shl ecx,4; add ecx,0xB3B0B0`＝**スロット `0xB3B0B0 + index*16` へ `6AA320`（intrusive list insert）**。`0x5F`('_') 経路は `0xB13BC4`（**`NiTArray<StringListElement@Tile*>`** RTTI）へ `42D800`（append→`4E4A10` grow）で追加。→ 属性名の先頭文字が 16 バイトスロットの index（`'a'..'z'`→0..25、`'_'` は別リスト）。`0xB3B0CC` は**命令からの参照が `58DA55` の 1 箇所のみ**＝起動時 CRC/アンチタンパ系の値（属性表本体ではない）[部分確定]。 |

## §2 証拠

### §2.1 セーフゾーン設定（v550_2/3 メモリ実読）
```
0xB135E0 = 3F4CCCCD(0.8f) 0xB135E4 -> 'fMenuPlayerLightDiffuseRed:Interface'
0xB135F8 = 20             0xB135FC -> 'iSafeZoneX:Interface'
0xB13600 = 20             0xB13604 -> 'iSafeZoneY:Interface'
0xB13608 = 20             0xB1360C -> 'iSafeZoneXWide:Interface'
0xB13610 = 20             0xB13614 -> 'iSafeZoneYWide:Interface'
0xB13618 = 40A00000(5.0f) 0xB1361C -> 'fXenonMenuStickSpeedMaxMod:Controls'
getter A24820: push 0xB135F8; mov ecx,0xB07CFC; call 65C620; mov eax,[0xB135FC] …
```

### §2.2 TileMenu vtable（v550_1/4）
```
0xA6AF14 COL=0xAC1D98 TD=0xB13A74 class='.?AVTileMenu@@'
+0x00 0x5900F0(dtor) / +0x04 0x590110 / +0x08 0x591C20 / +0x0C 0x5900D0 / +0x10 0x5900E0
+0x14 0x591C50(open) / +0x18 0x591B40（HUD/ローディング ガード）/ +0x1C 非ポインタ
591B40: [this+0x44]->vtable[+0x34]() == 0x3EC or 0x3EF ? ret : jmp 589D60
```

### §2.3 属性登録の行き先（v550_3/4）
```
58A112: movsx esi, al; sub esi, 0x40; cmp esi,0x20; jle; sub esi,0x20   ; 文字コード算術
58A189: mov ecx, esi; shl ecx, 4; add ecx, 0xB3B0B0                    ; ★ 16B stride slot
58A1A5: call 0x6AA320                                                  ; intrusive list insert
58A15D: mov ecx, 0xB13BC4; call 0x42D800                               ; NiTArray<StringListElement@Tile*>
0xB13BC4 vtable=0xA6A0F0 class='.?AV?$NiTArray@PAVStringListElement@Tile@@@@'
```

## §3 判定の内訳

- **V-550-1 完遂**: クロップ元値は `iSafeZoneX/Y/XWide/YWide:Interface`（既定 20）。`57D330`/`57D390` は画面 480/640 比でワイド判定し XWide/X（Y 側は YWide/Y）を返す。v549 の「アスペクト比クロップ」は [部分確定]（実体はセーフゾーン）。
- **V-550-2 完遂**: 属性表は `0xB3B0B0 + index*16` のスロット配列（`6AA320`）と `NiTArray<StringListElement@Tile*>`（`0xB13BC4`）。`0xB3B0CC` は 1 箇所参照の別用途 [部分確定]。
- **V-550-3 完遂**: `591C50` は `TileMenu`（`.?AVTileMenu@@`）の slot +0x14＝open 仮想。`591B40`（+0x18）が HUD/ローディング ガード。callers 0＝vtable 経由のみ。

## §4 残点（V-551-1..3）

1. `0xB3B0CC` の実体（1 箇所参照の値 0x6F4320B3 の生成元＝アンチタンパ/CRC のどれか）と `58DA55` ガードの意味【中】
2. `589EB0` の生成物（`StringListElement@Tile` 16 バイト）の構造と `6AA320` list の走査側【中】
3. `TileMenu` の他 slot（`0x591C20`/`0x5900D0`/`0x5900E0`）と `589D60`（`591B40` の分岐先）の役割【低】

## §5 後続への申し送り

- **移植側セーフゾーン**: `iSafeZoneX/Y/XWide/YWide:Interface`（既定 20）を読み、画面比 > 0.74 なら Wide 値、そうでなければ通常値を採る（`57D330`/`57D390`）。`0xB3A700`/`0xB3A704` はこれとコンソール設定の合成。
- **`TileMenu`（`.?AVTileMenu@@`）の open は slot +0x14**。`+0x18`（`591B40`）は HUDMainMenu(1004)/LoadingMenu(1007) のとき何もしないガード。
- **UI 属性名 → slot**: 先頭文字で `0xB3B0B0 + (code)*16` のスロット（`a`..`z`）、`_` 始まりは `0xB13BC4`（`StringListElement@Tile` 配列）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
