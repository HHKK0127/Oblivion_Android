# V137: v80/§70.15(6) 残不明点 4 件の解明（Oblivion.exe v1.2.0416 Steam）

- 作成日: 2026-10-26 / 分析専任サブエージェント（本ファイル新規作成のみ。docs・既存成果物は無変更）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B / ImageBase 0x400000 / capstone 5.0.7 skipdata）
- 出典: `tmp/re/v80_partB_globals.md` §5（残不明点 2/3/5/6）＋ `docs/RE_EXE_REPORT.md` §70.15 (6)
- 手法: ① 全ファイル raw バイト走査（imm32 / E8 / **E9** / jcc rel8 / rel32 一般式）② capstone 逆アセンブラ（宿主関数は Ghidra 台帳 `%TEMP%\opencode\func_list_new.txt` 21,597 件で帰属）③ `docs/Oblivion_decompiled.c` 部分走査（dec_index 20,465 ヘッダの二分探索）④ **MSVC RTTI 解像**（vtable-4 = COL → pTypeDescriptor → `.?AV...` 名。本セッションで新手法として確立）
- 中間成果物（すべて `%TEMP%\opencode` 配下・Git 管理外）: `v137_p1_ctx.py`〜`v137_p11.py` と `v137_p2_bin.txt` / `v137_p3_dec.txt` / `v137_p4_bin.txt` / `v137_p4_dec.txt` / `v137_p5_bin.txt` / `v137_p6_bin.txt` / `v137_p6_dec.txt` / `v137_p7_bin.txt` / `v137_p8_bin.txt` / `v137_p8_dec.txt` / `v137_p9_bin.txt` / `v137_p10.txt` / `v137_p11.txt`

---

## §0 判定サマリ

| # | 残不明点 | 判定 | 一句话 |
|---|---|---|---|
| 1 | FUN_004D5600（0xB35C24 setter）の到達経路 | **解明** | E8 は本当に 0。経路は **E9 tail-jump（0x482080 ミニスンク）** → そのスンクは **`.?AVGridCellArray@@` vtable の slot +0x4** に格納され、**TES 構築関数 FUN_00441970 が仮想呼び出しして到達** |
| 2 | 0xB4310C と ShowViewerStrings | **解明（v80 の前提は誤り＝帰属バイアスの産物）** | `FUN_0050FF70 → FUN_00441850` の経路は存在しない。実際の呼び出しは **`FUN_005101E0`（opcode 0x118 `SetLightingPasses`）** 内の 0x5103EB。この関数は **decompiled.c のヘッダ不在（dec_index 欠落）** のため v80 §1 記載の「直前ヘッダ誤帰属」で svs に着いた |
| 3 | 0xB39E04 の参照元クラス／フェード時間か | **解明** | B39E04 は **ポインタ（float ではない）**。pointee は vtable 無しの樹木キャノーシャドウ状態構造体（+0x24 = `LockFreeMap<TESObjectREFR*, BSTreeNode*>`）。**フェード時間は隣接グローバル** `0xB39E08=fTreeNearDistanceBase` / `0xB39E10=fTreeFarDistanceBase` など **8B Setting 対** |
| 4 | 0xB430DC と 0xB430F0 のペア意味（同一 puVar6 / alias か） | **解明: alias ではない（別値・別オブジェクト）** | 連続した独立ブロックで **別々のピクセル形式（RGB 3bpp vs RGBA 4bpp）→ 別々の `FUN_00701FC0` 呼出し（0x7B9637 / 0x7B971B）** を経て set。`puVar6` はデコンパイラの**変数名再利用**。読み手は `param_4[7] & 0x80` で選択（補足: 依頼文(4) の「B4310C と B430F0」は誤記。出典は DC/F0。B4310C と F0 に同時 set は**存在しない**） |

---

## §1 0x4D5600（FUN_004D5600）到達経路 [解明]

### 1-1 再確認した「無い」 things（v80 の記載は正しい）

| 検査 | 結果 |
|---|---|
| 全ファイル raw 走査で imm32 `00 56 4d 00`（= 0x4D5600） | **0 件**（PE ヘッダ〜overlay 含む 7,898,624 B 全域。.text/.rdata/.data に限らない） |
| E8 rel32 → 0x4D5600 | **0 件** |
| E9 / EB / jcc rel8・rel32 → 0x4D5600 | **E9 = 1 件（0x482085）**、rel8/jcc = 0 |
| 一般式 rel32（field+4+v / field+v / 0x400000+v が 0x4D5600） | 0 件（テーブル相対・imagebase 相対エンコードも無し） |
| Ghidra 台帳 | `004d5600,FUN_004d5600` **あり**（ラベルは存在する＝呼出し有無とは無関係） |

> **v80 の見落としは「E8 のみ走査した」こと**。間接経路（vtable）を疑っていなかったため、E9 を拾えなかった。

### 1-2 発見した経路（証拠つきチェーン）

```
FUN_00441970  … RTTI 確定: .?AVTES@@ の構築/初期化関数（*param_1 = &PTR_LAB_00a37470、memset(param_1,0,0xAC)）
  │  decompiled L50835:  piVar3 = (int *)FUN_00483350();     ← GridCellArray 構築（E8 @ 0x441AAA）
  │  decompiled L50837:  param_1[2] = piVar3;                 ← TES+8 に格納
  │  decompiled L50839:  (**(code **)(*piVar3 + 4))();        ← ★ slot +0x04 の仮想呼び出し
  │  decompiled L50846:  piVar3 = FUN_004838f0();             ← GridDistantArray 構築（E8 @ 0x441ADD）
  │  decompiled L50850:  (**(code **)(*piVar3 + 4))();        ← GridDistantArray の slot +0x04 も即時呼び出し
  ▼
[0xA3D79C]  .?AVGridCellArray@@ の vtable（COL = 0xAB8D0C @ vt-4、TD = 0xB06A54）
   slot +0x00 = 0x483570  … 内部で GridCellArray 構築 B（FUN_00483460）を呼ぶ virtual Create 相当
   slot +0x04 = 0x482080  … ★ ミニスンク（Ghidra 台帳に無い = UNLABELED）
   slot +0x08 = 0x481E20  / +0x0C = 0x4821E0 / +0x10 = 0x482090 …（0x482090 も台帳外）
   …
  ▼
0x482080:  e8 7b fd ff ff   call  0x481E00     ; thiscall: [ecx+4]=[ecx+8]=0xFFFFD8F0（= -10000、座標セントリネル再初期化）
0x482085:  e9 76 35 05 00   jmp   0x4D5600     ; ★ tail-jump（E8 が無い理由＝ここ）
0x48208A:  cc …
  ▼
FUN_004D5600 @0x4D5600:
   local = (DAT_00b3f9a8, DAT_00b3f9ac, DAT_00b3f9b0)   ; float 三つ組をコピー
   _DAT_00b35c00 = 0;                                    ; フラグワード全消去
   DAT_00b35c24 = FUN_004d5000(&local)  + addref         ; ★ B35C24 SET
   _DAT_00b35c14 = min(_DAT_00b35c14, 2048.0f)           ; 0xA2FF44 = 2048.0f（= fTreeNearDistanceBase 既定値と共用定数）
```

### 1-3 証拠の詳細

1. **スンク 0x482080 は台帳に無い（UNLABELED）**: 0x482050 の `ret 8`（0x48207C）→ int3 → **0x482080** → int3 ×6 → **0x482090**（別プロローグ）→ 台帳次の開始点は 0x4820F0。§70.16 の「Ghidra が gap に飲み込んだ実関数」と同型（本件は gap に **2** つ並存）。
2. **スンクへの参照は vtable の 1 ダワードのみ**: imm32 `0x80 20 48 00` は全ファイル **1 件 = 0xA3D7A0**（= GridCellArray vtable slot +0x04）。直接分岐 0 件。→ **間接経路のみ**という v80 の推測（「未検出の間接経路」）を実証。
3. **RTTI（新手法）**: `vtable-4` の値を COL とみなして pTypeDescriptor を辿ると `.?AVGridCellArray@@` / `.?AVGridArray@@`（0xA3D718、COL 0xAB8C6C、TD 0xB06A10）/ `.?AVGridDistantArray@@`（0xA3D7E0、COL 0xAB8D58、TD 0xB06A70）が**一発で確定**。※ v80 時点で「RTTI は無い（.rdata に .?AV 無し）」と思われていたが、タイプデスクリプタは 0xB01A08〜（.rdata 末尾）に 1,529 件あり、COL 位置（vt-4）さえ合えば読める。
4. **兄弟 vtable との差分が役割を語る**: GridArray / GridDistantArray の slot+4 は素の `FUN_00481E00`（座標リセットのみ）、**GridCellArray のみ**が「座標リセット → jmp FUN_004D5600（チェーン再構築）」スンクを持つ。＝ B35C24 の再構築は **外部グリッドセル配列の初期化時**にだけ発生する。
5. **ライフサイクル整合**: GridCellArray 構築 B（FUN_00483460、vtable slot+0 から呼ばれる Create 系）は **E8 @ 0x4834AF で FUN_004CA390 を呼び B35C24 を 0 化**（decompiled L158507 `DAT_00b35c24 = 0`）。つまり「生成時 = 無効化（clear）／slot+4 = 再構築（set）」という一貫したペア。
6. **スンク以外の E8 呼出しは本当に無い**: `E8 → 0x482080` = 0 件、`E8 → 0x481E00` = **1 件（0x482080 のみ）**、imm32 参照 0x481E00 = 0xA3D71C / 0xA3D7E4（兄弟 vtable の slot+4）＝整合。

### 1-4 判定

**解明**。「デッドコードではない。E8/E9 直接到達は（E9 1 件を除き）無く、**`.?AVGridCellArray@@` vtable slot +0x04 → ミニスンク 0x482080 → E9 → FUN_004D5600** という仮想ディスパッチ経路のみで到達する、コンパイラが切り出した 2 段ジャンプ実装」。

---

## §2 0xB4310C と ShowViewerStrings [解明 — v80 の前提自体が帰属バイアス]

### 2-1 主張の検証結果

v80 §5-3 の主張「FUN_0050FF70（svs ハンドラ）が FUN_00441850 経由で B4310C を set」は **成立しない**。

| 検査 | 結果 |
|---|---|
| FUN_00441850 の E8 呼出し（全 .text） | **4 件のみ**: 0x482CBD・0x483194（両方 FUN_004826F0 内）／**0x5103EB**／0x5DF258 |
| FUN_00441850 の imm32 データ参照 | 0 件（vtable 登録なし＝純粋な内部ヘルパ） |
| FUN_0050FF70 の内部 call（capstone 全列挙） | `call 0x401F00`(alloc) / `call 0x579B60`(報告) ×3 / `call edx`（vtable +0x154, +8, +0x30, +4）**のみ。FUN_00441850 への経路なし** |
| opcode entry 0xB0FE58 | opid **0x1157** name=`ShowViewerStrings`/`svs`、+0x18 = **0x50FF70**、+0x1C = 0x4FDE30（v80 の同定は正しい） |
| **0x5103EB の真の宿主** | **FUN_005101E0** = opcode entry **0xB0B7E0 / opid 0x118 `SetLightingPasses`/`SLP`** のハンドラ（usage 文字列 `"4 (0/1) values. amb\|diff\|tex\|spec ex: 1010"`、+0x1C も 0x4FDE30＝svs と共有の解析ヘルパ） |
| dec_index（decompiled.c ヘッダ 20,465） | **`fun_00510150` = 不在、`fun_005101e0` = 不在**（本文中の出現も 0 件）。FUN_0050FF70 の次ヘッダは 0x510480 |

**→ 誤りの発生機構（v80 §1 に自ら記載した既知バイアスの実例）**:
「dec_index に無い関数のコードは**直前のヘッダ関数に誤帰属する**」。0x5103EB の直前の decompiled ヘッダは **FUN_0050FF70** なので、v80 の E8 辺スキャン＋ヘッダ帰属は「svs → FUN_00441850」と報告した。台帳（21,597）には `005101e0` が**存在する**のに decompiled.c（20,465 ヘッダ）には無い、という 1,132 関数ギャップに落ちた典型例。

### 2-2 実際の書き手（全数）

| site | 宿主 | 内容 |
|---|---|---|
| 0x482CBD | FUN_004826F0 | `iVar10 = FUN_004a41e0()`（64×64 ランダムノイズテクスチャ singleton `DAT_00b35414` を遅延生成）→ `**(iVar10+0x20)` の先頭要素の `+8` を **SET** |
| 0x483194 | FUN_004826F0 | `uVar11 = FUN_00405a30()`（`= *(this+0x20)` の汎用アクセサ、63 呼出し）を **SET** |
| 0x5103EB | **FUN_005101E0 = SetLightingPasses** | `push 0; mov byte [0xb06cbc], bl; call 0x441850` → **CLEAR(0)** |
| 0x5DF258 | **FUN_005DF160 = `.?AVVideoMenu@@` vtable slot +0x0C（COL 0xAC1A00）** | `switch(param_2)` case 1: `if (DAT_00b06cbc==0) { FUN_00441850(0); FUN_00482670(); B42F40 &= ~0x20; } else { FUN_004826f0(); B42F40 \|= 0x20; }` |

（FUN_004826F0 自身のもう 1 か所の set は 0x5103D2 経由: `mov eax,[0xb333a0]; mov byte [0xb06cbc],1; mov ecx,[eax+8]; call 0x4826f0`）

**インライン clear（set と同じ refcount 释放パターンを内包）**: FUN_004425D0 / FUN_00444340 / FUN_00444C70 / FUN_00482670 / FUN_004826F0 / FUN_0055F550（樹木側 teardown で 108 と 10C を連続 release）／release ヘルパ FUN_00A274C0。
**読手**: FUN_007E8E10（`FUN_0076c910(DAT_00b4310c)` = NiPointer 代入）と **0x83xxxx〜0x88xxxx のシェーダ/エフェクト群 約 50 関数**が B43108/B43110 とセットで比較。

### 2-3 「なぜ singleton 化したのか」

- FUN_00441850 は **「1 グローバルスロットに 1 オブジェクトだけ載せる」refcounted assign ヘルパ**（既存の `InterlockedDecrement → vtable[0] release → =p → InterlockedIncrement` パターン）。B43104/108/10C/110 の **4 スロット族の 1 つ**であり（v80 §3 の構造論）、`DAT_00b4310c + 1` は「指し先オブジェクトの refcount」を叩く。
- singleton 化の理由は「ビューア文字列が特殊だから」ではなく、**B4310C に載る対象（lighting-pass 設定オブジェクト）が常時 1 つしか無く、下流のシェーダ群が 108/10C/110 の 3 点セット比較で「現行設定」を選び取る**ため。
- 切替トリガは **INI ノード化されたブール `DAT_00b06cbc`**（FUN_009DCFA0 が `FUN_004040E0(&DAT_00b06cbc)` で登録）と、コンソールコマンド **`SetLightingPasses`(opid 0x118, SLP)**、および VideoMenu の更新（FUN_005DF160）。連動ビットは `DAT_00b42f40 & 0x20`。

### 2-4 判定

**解明（前提の否定を含む）**。「svs → FUN_00441850 経路は無い。v80 の記載は dec_index 欠落関数（FUN_005101E0 = SetLightingPasses）への呼出しが『直前ヘッダ＝FUN_0050FF70』に誤帰属した結果」。

---

## §3 0xB39E04 の参照元クラス／フェード時間変数か [解明]

### 3-1 B39E04 は「ポインタ」である（float ではない）

| 証拠 | 内容 |
|---|---|
| 唯一の実体代入 | `DAT_00b39e04 = FUN_0055e850()`（lazy init: FUN_0055F750 内 0x55F7C3 / FUN_0055FCB0 内 0x55FD63 / FUN_0055FDF0 内）、破棄は FUN_0055F550（`DAT_00b39e04 = 0`） |
| imm32 参照 70 件の分類 | ほぼ全件が `mov/cmp [0xb39e04],…`（ポインタの読・比較）。**float 命令（fmul/fld）は 0 件** |
| pointee の形 | **vtable 無し**（ctor が `*param_1 = 0`）、0x28 B の素の構造体 |

**pointee のフィールド（ctor FUN_0055E850 + 各 writer で確定）**:

```
+0x00  キュー/リスト オブジェクト … FUN_0055E720(2,0x25,0xc) で生成、vtable = 0xA651C4
       └ RTTI: .?AV?$LockFreeMap@PAVTESObjectREFR@@PAVBSTreeNode@@@  ★樹木系決定的証拠
+0x04  refcounted テクスチャ（NiSourceTexture 系）
       └ FUN_0055FDF0: "Data\\Textures\\Trees\\CanopyShadow.dds"（DAT_00b33a04 != 0 時）
         または FUN_0070E560(0x10,0x10,&DAT_00b25e48,1,1) の 16×16 白 RGBA を FUN_00701FC0 でラップ
       └ FUN_0055E340(*(B39E04+4)) 経由で **0xB43108 へミラー**（B43108 の唯一 setter）
+0x0c  NiMaterialProperty 相当（0x5C B、FUN_007095A0。+0x1c/+0x20/+0x24 … を色として書換え、+0x54 カウンタ増加）
+0x1c  スカラー（FUN_0055FA50 が `*(B333A0+0x5c)+0xc0` から書込み／0 化）
+0x21..0x23  フラグ類（+0x20=1, +0x21=1, +0x22=0, +0x23=DAT_00b125f0）
+0x24  ★上記 LockFreeMap<TESObjectREFR*, BSTreeNode*>（FUN_0055F550 が vtable 復帰＋free）
```

**サブオブジェクトの vtable = RTTI で型確定（全部 Gamebryo プロパティ系）**:

| vtable | RTTI |
|---|---|
| 0xA2FDB4 | `.?AVNiZBufferProperty@@` |
| 0xA2FD04 | `.?AVNiVertexColorProperty@@` |
| 0xA3D604 | `.?AVNiAlphaProperty@@` |
| 0xA7E1F4 | `.?AVNiMaterialProperty@@`（FUN_007095A0 の 0x5C B オブジェクト） |
| 0xA7E624 | `.?AVNiPixelData@@`（FUN_0070E560 の 16×16 ピクセルデータ） |
| 0xA38FB4 | `.?AV?$NiTArray@PAD@@`（svs の文字列コレクタ 0x10 B オブジェクト。§2 の傍証） |

### 3-2 隣接グローバルこそ「変数」（しかも Setting 対）

0xB39E08 以降は **8 バイト間隔で並ぶ Setting オブジェクト `{ float value@+0 ; char* name@+4 }`**。登録スタブ（0x9F8EF0 以降、各 0x30 B の小関数）:

```
0x9F8EF0: fld [0xA2FF44](=2048.0f); push; push 0xA65228 "fTreeNearDistanceBase"; mov ecx, 0xB39E08; call FUN_0041BAE0; push 0xA23560; call 0x981FB4(atexit)
0x9F8F20: fld [0xA65258](=16384.0f); push 0xA65240 "fTreeFarDistanceBase" ; mov ecx, 0xB39E10; call FUN_0041BAE0; push 0xA23570; call atexit
0x9F8F50: fld [0xA31C80](=10.0f)   ; push 0xA6525C "fTreeSizeConversion"   ; mov ecx, 0xB39E18; call FUN_0041BAE0; push 0xA23580; call atexit
0x9F8F80: fld [0xA43328]           ; push 0xA65270 "fTreeWindVariance"     ; mov ecx, 0xB39E20; …
0x9F8FB0: fld [0xA379B4]           ; push 0xA65284 "fLeafRockTimeScale"    ; mov ecx, 0xB39E28; …
0x9F8FE0: fld [0xA3D65C]           ; push 0xA65298 "fLeafRustleTimeScale"  ; mov ecx, 0xB39E30; …
0x9F9010: fld1                     ; push 0xA652B0                          ; mov ecx, 0xB39E38; …
0x…       atexit スランク 0xA23560/0xA23570/…: `mov ecx, 0xB39E08/0xB39E10/…; jmp FUN_004055E0`
           0xA23550: `mov ecx, 0xB39E80; jmp FUN_00401010`(= DeleteCriticalSection) → 0xB39E80 = CRITICAL_SECTION
```

- `FUN_0041BAE0(this, name, default)` = **Setting 登録**: `*this = default; this[1] = name; FUN_00412D30(name, this)`（重複時は `"Setting key '%s' already used in map.\nSetting keys must be unique.\n"` を FUN_004a7a60 に出力）。
- `FUN_004055E0` = Setting 破棄（`*(this+4)` の文字列処理）。
- **裏付け**: `0xB39E0C` の imm32 参照は **0 件** → `+4`（name ポインタ）は base+disp でのみアクセスされる、という 8 B 構造と完全整合。
- 読み手の実例: `fmul dword ptr [0xb39e08]`（FUN_0055FCB0 @0x55FDB3 / FUN_00560600 @0x56083E）＝ **value を float として乗算**。

### 3-3 「フェード」に関わるのはどれか

```
FUN_005DF160(.?AVVideoMenu@@ slot+0x0C) case 1:
    _DAT_00b0760c = *time + (Δt × FUN_00588bd0(0xfb5) / _DAT_00a309f0)   … 時変積算値
    FUN_0055fcb0()                                                        … 樹木フェード更新へ
FUN_0055FCB0:
    local = _DAT_00b0760c * _DAT_00b39e08   (= fTreeNearDistanceBase)
    FUN_007871d0(local, _DAT_00b0760c * _DAT_00b39e10 (= fTreeFarDistanceBase))
        → mov ecx,[ecx]; call FUN_007A24D0 → *(obj+0x44)=f1; *(obj+0x40)=f2   … 2 float を書込み
FUN_00560600 (tree leaf maps 生成):
    FUN_0078b0e0(_DAT_00b39e18 * …, _DAT_00b39e18 * …)   … fTreeSizeConversion
    FUN_007871d0(_DAT_00b0760c * _DAT_00b39e08, _DAT_00b0760c * _DAT_00b39e10)
```

**結論**: 「フェード時間」に相当するのは **`_DAT_00b0760c`（時変積算）と `fTreeNearDistanceBase` / `fTreeFarDistanceBase`（距離ベースのフェード境界）** の掛け算であり、**B39E04 そのものはフェード時間変数ではない**（オブジェクトへのポインタ）。B39E04 の pointee はそれ自体が「キャノーシャドウの描画状態」を束ねる構造体。

### 3-4 参照元（どの関数群が触るか）

- 樹木系: FUN_0055E390（CRITICAL_SECTION 0xA2F830 下の待機/ドレイン）、FUN_0055F550（破棄）、FUN_0055F750（lazy init、29 呼出し）、FUN_0055F7E0（getter）、FUN_0055F800、FUN_0055FA50（色/スカラー書換え）、FUN_0055FCB0、FUN_0055FDF0（DDS 読み込み）、FUN_00560110、FUN_00560600（leaf maps）
- 汎用: FUN_004346A0（`mov ecx,[0xb39e04]` @0x4346AD → FUN_0055F800/FUN_0055DF70 呼出し）、**FUN_00439880（`.?AVQueuedTreeModel@@` / `.?AVQueuedFileEntry@@` の dtor 系 @0x4398E7 で pointee を this に載せる）** ＝ 非同期樹木モデル読込からの接続
- メインスレッド: **FUN_0043FC20**（`EnterCriticalSection(B35380)` 内で `FUN_0055e390(1)` ×2 と `FUN_007b84e0()`（シェーダ一括リセット）を実行）、FUN_004466F0（shutdown 系）
- 参考: 同一グループの `0x446864`（FUN_004466F0）は `"ould not match desired fullscreen mode"` リージョンに隣接。

### 3-5 判定

**解明**。「B39E04 = 樹木キャノーシャドウ状態へのポインタ（正式 RTTI は vtable 無しのため不可だが、+0x24 の `LockFreeMap<TESObjectREFR*, BSTreeNode*>` と Ni プロパティ群・CanopyShadow.dds・'Tree %s has too many leaf maps' 文字列で**木ロッド/キャノーシャドウ系と確定**）。フェード時間変数ではない。フェード距離は隣の `fTreeNearDistanceBase` / `fTreeFarDistanceBase`（0xB39E08 / 0xB39E10）」。

---

## §4 0xB430DC と 0xB430F0（＋ B4310C との関係） [解明]

> **注記**: 依頼文 (4) は「0xB4310C と 0xB430F0」だが、出典（v80 §5-6 / RE_EXE_REPORT §70.15(6)-6）は **0xB430DC と 0xB430F0**（同一 puVar6）。本項は両方について回答する（末尾に B4310C 側）。

### 4-1 結論: alias ではない（別値・別オブジェクト）

FUN_007B8990（`.\BSShaderManager.cpp` アサート付き、shader init 本体）内に **連続した独立ブロック**が並ぶ（EH スコープ番号 `local_44 = 5 → 6 → 7 → 8` が順次進行 ＝ いずれも同一パスで順に実行）:

| # | 生成 | ピクセル形式 | 塗り | wrap | set 先（store VA） |
|---|---|---|---|---|---|
| 5 | `FUN_0070e560(0x80,0x80,local_1d8,…)` 球面手続きテクスチャ | — | 法線風（sqrt 正規化） | `FUN_00720a30(…,DAT_00b43104)` | **0xB430EC**（0x7B95A2 近傍） |
| 6 | `FUN_0070e560(0x10,0x10,&DAT_00b25e48,1,1)` **16×16 RGB 3bpp** | `puVar10 += 3` | **(0x80,0x80,0xFF)** | **`FUN_00701fc0` @ 0x7B9637** | **0xB430DC**（store @ **0x7B966E**） |
| 7 | `FUN_0070e560(0x10,0x10,&DAT_00b25e00,1,1)` **16×16 RGBA 4bpp** | `puVar10 += 4` | **(0x80,0x80,0xFF,0x40)** | **`FUN_00701fc0` @ 0x7B971B** | **0xB430F0**（store @ **0x7B9752**） |
| 8 | `FUN_0070e560(0x10,0x10,&DAT_00b25e48,…)` | 3bpp | 同上 | `FUN_00701fc0`（以降 0x7B97F7/9A4B/9B2B/9FE9 も同族） | 次スロット |

**alias でない 4 証拠**:
1. **別々の `FUN_00701FC0` 呼出しサイト**（0x7B9637 = DC 用 / 0x7B971B = F0 用。FUN 内で計 6 箇所）＝返り値は別ヒープオブジェクト。
2. **別々のピクセル形式記述子**（`&DAT_00b25e48` = RGB 版 vs `&DAT_00b25e00` = RGBA 版）と**別々の塗りループ**（3 B/pixel vs 4 B/pixel、alpha 0x40）。
3. 両 set は**同一パスの連続ブロック**（`local_44` 6→7）であり、片方だけが実行される分岐ではない → 「同じ値が 2 回 set される」状況は構造的に起きない。
4. v80 の「同一 puVar6」は **Ghidra デコンパイラが別ブロックのローカルに同じ名前を付けた**もの（各ブロック先頭で `puVar6 = FUN_00701fc0(iVar13)` と**再代入**されている）。

### 4-2 読み手での使い分け（＝2 スロットの意味）

FUN_0084ABC0 系（BSShaderManager レンダリング）:

```c
iVar6 = (**(code **)(*param_4 + 0x8c))(i);      // シェーダ側が自分のテクスチャを持っていればそれ
if (iVar6 == 0) {
    puVar7 = DAT_00b430f0;                       // 既定(F0, RGBA 版)
    if ((param_4[7] & 0x80U) == 0)
        puVar7 = DAT_00b430dc;                   // 既定(DC, RGB 版)
} else puVar7 = iVar6;
```

→ **DC/F0 = 「シェーダがテクスチャを持たない場合のフォールバック既定テクスチャ対**、`param_4[7]` の bit0x80 で RGB 版 / RGBA 版（透過あり）を選択」。両者は同じ初期化パスで別々に生成される **対（pair）** であって同一値ではない。

### 4-3 B4310C と B430F0 の関係（依頼文の表記について）

- **同一 puVar6 による両方への set は存在しない**: B4310C の書き手は FUN_00441850 とインライン clear 群のみ（§2-2）で、**FUN_007B8990 は B4310C を一切触らない**（decompiled 全走査で B4310C 出現 200 回に FUN_007b8990 は 0）。
- B430F0（0xB430F0）と B4310C（0xB4310C）は BSS 上 0x14 離れた**独立 refcounted singleton スロット**であり、メモリ alias はしない（v80 §3-4 の「base+disp 相互参照 0 件」とも整合）。
- したがって依頼文 (4) の「B4310C と B430F0」は **B430DC/F0 の誤記**と判定。本質の問い（同一 puVar6 / alias か）に対しては §4-1 の通り **alias ではない**。

### 4-4 判定

**解明**。「DC と F0 は同一パスの連続ブロックで **別形式・別オブジェクト**として生成・set される。`puVar6` はデコンパイラの名前再利用。読手は bit0x80 で使い分ける対」。

---

## §5 手法メモ（再現用手順）

1. **E8 のみでは到達不能な関数を倒す**: 全ファイル raw 走査で `E8/E9/EB/0F8x` の rel32/rel8 を解釈（VA 変換はセクション const、.text = off+0x400C00）。→ 本件は **E9 1 件が全てを決した**。
2. **vtable 発見**: imm32 参照を拾い、連続コードポインタ列の **vtable-4 = COL** とみなして `pTypeDescriptor`（imagebase RVA）→ `.?AV...` を読む。`.?AV` は 0xB01A08〜 に 1,529 件（.rdata 末尾寄り）＝「RTTI なし」は誤り。
3. **dec_index 欠落の罠の再確認**: `docs/Oblivion_decompiled.c` のヘッダ（20,465）と台帳（21,597）の差 1,132 関数は、E8 site の宿主帰属を狂わせる。本件では `fun_00510150`/`fun_005101e0` が不在であることを直接確認して誤帰属を証明した。
4. **設定オブジェクトの見つけ方**: `mov ecx, <global>; call <fn>` + 直前の `push <name str>` + `push <atexit thunk>` の 3 連続パターン（本件 0x9F8Exx 群）。

---

## §6 新規残点（V143+ 起票粒度）

| 優先 | 内容 | 起票粒度 |
|---|---|---|
| 中 | **GridCellArray::slot+0x04 の全呼出し元**の特定（本件で特定できたのは TES 構築 FUN_00441970 の 2 箇所のみ。`call [reg+4]` 形式は .text に 196 件あり、GridCellArray 限定の絞り込みは未了）＋ FUN_00441970 を呼ぶ上位（TES の生成時期・他プレース） | 1 タスク（E8+仮想呼び出しセット走査） |
| 中 | **B4310C に格納される実体クラス**の同定（FUN_004826F0 が `FUN_00719cb0(4, …)` で作る 4 要素オブジェクト＝amb/diff/tex/spec の 4 pass、および FUN_004A41E0 のノイズテクスチャ +8 由来） | 1 タスク（ctor→field フォロー） |
| 中 | **`0xB430DC/F0/EC` の残りスロット**（0x7B97F7 以降で set される 3 つ）の形式・役割一覧と、`param_4[7]` bit0x80 の設定元 | 1 タスク |
| 低 | **decompiled.c 欠落 1,132 関数の列挙**（本件の `FUN_00510150`/`FUN_005101E0` を含む。台帳差分で機械抽出可）→ §70.16 の UNLABELED 列挙と接続 | 1 タスク（スクリプト 1 本） |
| 低 | `_DAT_00b0760c` の厳密意味（LOD フェード進行度か経過時間か）と `FUN_007871D0` の `this` 対象（`+0x40/+0x44` フィールドの所有クラス） | 1 タスク |
| 低 | 0x9F8E00〜0x9F9050 の Setting 登録スタブ全数（B39E08..B39E38 の 7 件＋前後）と 0xA235xx atexit スランクの対応表 → Android 移植の INI/GMST 既定値テーブルに接続 | 1 タスク（§70.2 と接続） |

---

## §7 使用証拠一覧

**バイナリ（本セッションで実測）**
- 全ファイル imm32 `00 56 4d 00` = 0 件 / E8→0x4D5600 = 0 / **E9→0x4D5600 = 1（site 0x482085、host 上は台帳 0x482050 だが実体は gap 内スンク 0x482080）**
- imm32 参照: `0x482080` → **0xA3D7A0 のみ**、`0x482090` → 0xA3D7AC、`0x481E00` → 0xA3D71C・0xA3D7E4、`0x4D5600` → 0
- E8 callers: FUN_00441850 = 4（0x482CBD / 0x483194 / 0x5103EB / 0x5DF258）、FUN_004826F0 = 3（0x40D650・0x5103D2・0x5DF248）、FUN_00483350 = 1（0x441AAA）、FUN_00483460 = 1（0x483573）、FUN_004838F0 = 1（0x441ADD）、FUN_004CA390 = 1（0x4834AF）、FUN_00481E00 = 1（0x482080）、FUN_0055E390 = 4、FUN_0055E850 = 5、FUN_0055F750 = 29、FUN_00701FC0 = 15（うち FUN_007b8990 内 6: 0x7B9637 / 0x7B971B / 0x7B97F7 / 0x7B9A4B / 0x7B9B2B / 0x7B9FE9）
- 逆アセンブル: 0x482040-0x4820B0（スンクと int3 境界）、0x481E00（座標リセット）、0x5103C0-0x510410（SLP の set/clear）、0x9F8EF0-0x9F9010（Setting 登録）、0xA23540-0xA2357F（atexit スランク）、0x7871D0、0x55FDB3/0x56083E（fmul）
- opcode テーブル: entry 0xB0FE58（opid 0x1157 `ShowViewerStrings`/`svs`, +0x18=0x50FF70, +0x1C=0x4FDE30）、entry 0xB0B7E0（opid 0x118 `SetLightingPasses`/`SLP`, +0x18=0x5101E0, +0x1C=0x4FDE30）、neighbors 0x116 `TogglePathLine` / 0x117 `PickRefByID` / 0x119 `ToggleLODLand` / 0x11A `ToggleGrassUpdate`
- **RTTI（vtable-4 = COL 方式）**: `GridCellArray`(0xA3D79C/COL 0xAB8D0C/TD 0xB06A54) / `GridArray`(0xA3D718/0xAB8C6C/0xB06A10) / `GridDistantArray`(0xA3D7E0/0xAB8D58/0xB06A70) / `VideoMenu`(0xA6A09C/0xAC1A00/0xB13BA8) / `TES`(0xA37470) / `NiZBufferProperty`(0xA2FDB4) / `NiVertexColorProperty`(0xA2FD04) / `NiAlphaProperty`(0xA3D604) / `NiMaterialProperty`(0xA7E1F4) / `NiPixelData`(0xA7E624) / `NiTArray<char*>`(0xA38FB4) / `LockFreeMap<TESObjectREFR*,BSTreeNode*>`(0xA651C4) / `NiTMap<uint,VertexDist>`(0xA3D6C8) / `QueuedTreeModel`(0xA36C14) / `QueuedFileEntry`(0xA36B04) / `TESWindListener`(0xA4699C)
- 文字列: `Data\Textures\Trees\CanopyShadow.dds`(0xA65200) / `Trees`(0xA366C4) / `Tree %s has too many leaf maps (greater than six)…`(0xA65478) / `Textures\Trees\Branches\`(0xA65504) / `Textures\Trees\Leaves\`(0xA654EC) / `.\BSShaderManager.cpp`(FUN_006F7070 アサート) / `fTreeNearDistanceBase`(0xA65228)・`fTreeFarDistanceBase`(0xA65240)・`fTreeSizeConversion`(0xA6525C)・`fTreeWindVariance`(0xA65270)・`fLeafRockTimeScale`(0xA65284)・`fLeafRustleTimeScale`(0xA65298) / `4 (0/1) values. amb|diff|tex|spec ex: 1010`
- 浮動小数点定数: 0xA2FF44 = **2048.0f**、0xA65258 = 16384.0f、0xA31C80 = 10.0f

**decompiled.c（`docs/Oblivion_decompiled.c` 部分走査）**
- L167027-167063 FUN_004d5600 本体 / L166737-166884 FUN_004d5000 / L167067+ FUN_004d56a0
- L204225-204350 FUN_0050ff70（svs 実装、call なし確認）／L204354 が次のヘッダ = FUN_00510480（**0x510150・0x5101E0 のヘッダ欠落**）
- L50686-50707 FUN_00441850 / L50775+ FUN_00441970（**L50835/50839/50846/50850**）/ L104581-105066 FUN_004826F0（L104775・L105014 の 2 set）/ L104548+ FUN_00482670
- L344557+ FUN_005df160（switch case 1）/ L344598 `FUN_00441850(0)`
- L260078+ FUN_0055e390 / L260257+ FUN_0055e720 / L260334+ FUN_0055e850 / L261038+ FUN_0055f550 / L261137+ FUN_0055f750 / L261286+ FUN_0055fa50 / L261386+ FUN_0055fcb0 / L261457+ FUN_0055fdf0 / L261853+ FUN_00560600
- L672854+ FUN_007b8990（**L673203-673245 = DC ブロック、L673246-673288 = F0 ブロック**、L673182-673201 = EC ブロック）
- L757098+ FUN_0084abc0（`puVar7 = F0; if (!(param_4[7]&0x80)) puVar7 = DC`）
- L18310+ FUN_0041bae0（Setting 登録）/ L4267+ FUN_004055e0（Setting 破棄）/ L1034218+ FUN_009f8d70 / L4581+ FUN_00405a30 / L130451+ FUN_004a41e0 / L636050+ FUN_007871d0 / L656221+ FUN_007a24d0
- B4310C 全出現 200 行（書手 7 関数＋読手約 50 関数）を L50694〜L1064638 で列挙

**既存成果物（参照のみ）**
- `tmp/re/v80_partB_globals.md` §1（帰属バイアスの自認）/ §2 判定表 / §3 COM 単例族 / §5 残不明点 2/3/5/6
- `docs/RE_EXE_REPORT.md` §70.15 (1)-(7)、§70.16（UNLABELED 列挙手法）
- `%TEMP%\opencode\probe_harness.py`（Ctx: セクション VA 変換・dec_index・rd/cstr）、`%TEMP%\opencode\func_list_new.txt`（Ghidra 台帳 21,597 件）

**本ファイル**: `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v137_v80_global_residuals.md`（UTF-8・新規 1 件のみ）
