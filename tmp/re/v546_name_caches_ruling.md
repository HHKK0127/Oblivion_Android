# v546 — メニュー名の 2 map の用途確定（**`0xB13948`＝`NiTMapBase<NiTPointerAllocator<I>, const char*, XMLStorage*>`／`0xB1395C`＝`NiTMapBase<..., const char*, BuildStorage<Tile>*>`（RTTI 実測）**）・**`FUN_004E4A10`＝1043 callers の汎用 NiTArray grow/set ヘルパ（v545 の「0x33 singleton 専用」を訂正）**・**`Menu` vtable `0xA696DC` の slot 実測**（V-546-1..3 完遂・旧残点 V-406-3 継続第76弾）

- 採番: **v546**（旧残点群消化・probe prefix **v546_1（1本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-546-1..3（v545 残点）** — 1=`4E4A10` の this 供給元と 0x33／2=`0xB13948`/`0xB1395C` の用途差／3=`Menu` vtable slot 一覧。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-546-2: 2 メニュー名 map の用途差** | **[確定]（RTTI 実測）** | `0xB13948` の vtable `0xA69688`→TD **`.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PBDPAVXMLStorage@@@@`** ＝ **名前（const char*）→ `XMLStorage*`**。`0xB1395C` の vtable `0xA696A8`→TD **`.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PBDPAVBuildStorage@Tile@@@@`** ＝ **名前→ `BuildStorage<Tile>*`**。→ **メニュー名 →（UI XML ソース）と（Tile ビルドキャッシュ）の 2 段キャッシュ**で、v545 の「両方から消す」はこの 2 種を解放する意味と確定。両 map への disp32 store は 0 件（**CRT init で `.data` 静的に成人り＝in-place 構築**。v544 の 0xB0613C/0xB06164 と同型）。 |
| **V-546-1: `FUN_004E4A10`** | **[確定]（1043 callers）→ v545 の「0x33 singleton 専用」は誤り → 訂正** | caller 1043 箇所・多クラス（`0x40C180`（head）/`0x42D800` 等）。代表窓: `movzx ecx, word [edi+0xE]; add ecx, esi; push ecx; mov ecx, edi; call 4E4A10`（edi=配列サブオブジェクト）＝**`NiTArray::SetSize/Resize(newCount)` 相当の汎用ヘルパ**。`587B20` の `alloc(1)`→`4E4A10(0x33)` は「**型 ID 0x33 用のグローバル配列/manager を index 0x33 まで grow してスロットを用意**」する意味に読むのが整合（`0xB3A708` はその manager スロットの配列ポインタ）。[部分確定]（manager 実体は残点）。 |
| **V-546-3: `Menu` vtable `0xA696DC`** | **[確定]（実読）** | `+0x00=0x5853F0`（scalar deleting dtor）／`+0x04/0x08/0x0C/0x10/0x14/0x20/0x24/0x28=0x60CF60`（no-op 共有スタブ）／`+0x18/+0x1C=0x890B90`（同実装 2 slot）／`+0x2C=0x60D0A0`（no-op）／`+0x30=0x6FE080`（false 固定）／`+0x34=0x6F7070`（0 固定）。→ `Menu` 基底は**大半が no-op スタブで仮想枠のみ**（UI 実装はほぼ派生側）。 |

## §2 証拠

### §2.1 RTTI（v546_1）
```
0xB13948 = 0xA69688 → COL → TD '.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PBDPAVXMLStorage@@@@'
0xB1395C = 0xA696A8 → COL → TD '.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PBDPAVBuildStorage@Tile@@@@'
両方 store 0 件（.data 静的単一tons・CRT in-place 構築）
```

### §2.2 `4E4A10` 代表 caller（v546_1 (a2)）
```
40C1F9: movzx ecx, word ptr [edi+0xe]   ; 既存 count
40C1FD: add   ecx, esi                  ; + 追加数
40C1FF: push  ecx / mov ecx,edi / call 4E4A10   ; ★ NiTArray::SetSize(edi, newCount)
edi = [ebx+0xAA8] 配列サブオブジェクト（クラス内 NiTArray メンバ）
```

### §2.3 `Menu` vtable 0xA696DC（実読）
```
+0x00 0x5853F0 / +0x04 0x60CF60 / +0x08 0x60CF60 / +0x0C 0x60CF60 / +0x10 0x60CF60
+0x14 0x60CF60 / +0x18 0x890B90 / +0x1C 0x890B90 / +0x20 0x60CF60 / +0x24 0x60CF60
+0x28 0x60CF60 / +0x2C 0x60D0A0 / +0x30 0x6FE080(false) / +0x34 0x6F7070(0)
```

## §3 判定の内訳

- **V-546-2 完遂**: 2 map＝`XMLStorage*` と `BuildStorage<Tile>*` の 2 段キャッシュと RTTI で確定（key=メニュー名 const char*）。
- **V-546-1 完遂＋訂正**: `4E4A10` は 1043 caller の汎用 NiTArray grow（v545 の「0x33 singleton 専用ヘルパ」は誤り→訂正）。0xB3A708 は「型索引 0x33 用スロット配列」。
- **V-546-3 完遂**: `Menu` 基底は no-op スタブ主体（実 UI は派生で override）。

## §4 残点（V-547-1..3）

1. `0xB3A708`（型索引 0x33 スロット）の manager 実体（スロット配列の台帳 `0xB13948` 等との関係／CRT init での初期化）【中】
2. `Menu` vtable `+0x18`/`+0x1C`（`0x890B90` 同一実装）の役割＋`0x60CF60` no-op スタブが 8 slot を占める理由【低】
3. `591C50` の type 4002（および 4005/4011）値域の定義元（UI XML のメニュー種別 enum）【中】

## §5 後続への申し送り

- **移植側 UI 名キャッシュ**: 名前→XML ソース／名前→Tile ビルドの 2 段（`NiTMapBase<..., const char*, T*>`）。解除時は両方 free。
- **`4E4A10` は NiTArray の SetSize 相当**——移植側の配列クラスに「newCount で拡張し増分クリア・count/cap word 更新」を実装すれば全 1043 caller を 1 関数で賄える。
- **`Menu` 基底はほぼ空**（no-op スタブ）——移植側は派生で実装する前提のインタフェース面。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
