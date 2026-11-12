# v287_v76_data_map_residuals.md — v60v76-bundle 依頼5 [P5]: v76 データ関係マップ残件6点（2026-11-04）

- 依頼元: `tmp/re/handoff_20261105_v60v76_residuals_bundle.md` 依頼5 [優先度 P5]（出典 `tmp/re/v76_data_relationship_map.md` Part C）
- 対象: Oblivion.exe v1.2.0416 (Steam)
- ハーネス: `probe_harness.py selftest` → **4/4 PASS**（本セッション実行）
- **判定: 6点中 4点 [確定] / 2点 [部分確定]**

---

## 1. C-7: `0xB3CAC0` の用途 → **[部分確定] アドレス=キーの identity 照合で使われる番兵/単一例グローバル**

- **本体ヘルパ = `FUN_00560920`（annotate_db 名 `NiControllerManager_FindSeq`、168 callers、22行）**:
  ```c
  int* FUN_00560920(int key_addr, int *mgr) {
    if (!mgr) return 0;
    cur = vcall(mgr, vtable+4)();          // 先頭ノード取得
    while (cur) { if (cur == key_addr) return mgr; cur = *(cur+4); }  // アドレス一致で走査
    return 0;
  }
  ```
  → **「mgr のリンクリストが `key_addr` を含むか」の identity（ポインタ一致）照合**。
- **呼び出し形はほぼ `FUN_00560920(&DAT_00b3cac0, container)`**（decompiled L6756/L6772/L92107/L93740/L102975/L141774/L144696/L171770/L175016/L175099/L176318…）＝**グローバル `0xB3CAC0` のアドレスをキーとして照合**。加えて直接比較 `puVar == &DAT_00b3cac0`（L92244/L120525/L171824）と `FUN_00700010(&DAT_00b3cac0)`（L102912）も実在。
- **参照量**: `.text` abs32 = **53 サイト**（上位: `FUN_0069EB60`×4 / `FUN_004DA4F0`×3 / `FUN_004E2F70`×3 / `FUN_005107A0`×3 …。0x69xxxx 帯 = magic/MGEF 領域を含む）。
- 判定: **`0xB3CAC0` = identity 照合の番兵（単一例グローバル・先頭ノード）**。所属クラス（NiControllerManager 系のグローバル实例か、純番兵か）と初期化元は未特定 → **部分確定**（v76 の「MGEF/FormID マップ候補」は、照合方式（アドレス=キー）の面では支持されるが、キー領域の実体は未確定）。

---

## 2. C-8: `FUN_007b4010` 埋め込みマップの内容 → **[部分確定] obj+8 の map インスタンス・u16 キー・アドレス一致リスト**

`FUN_007b4010`（71行、caller 2 件）の本文:

- `DAT_00b42d60 += (param_7 & 0xFFFF)` — **u16 カウンタの集計グローバルに加算**。
- `FUN_0055e000(*(param_4 + 8), &key)` — **`param_4`（渡されたオブジェクト）の +8 に埋め込まれた map インスタンス**を `find`（`FUN_0055E000` = GMST/INI 系と**同一実装クラスの別インスタンス**。Hub2 の find 本体）。
- ヒットノードは `*(node+0x24)` のチェーンを辿り **`param_2 != node[2]`（アドレス一致）** で照合 → 登録/解除の membership 管理。未ヒット時は `FUN_005B1E20(&param_2)` へ。
- **caller 2 件**: `FUN_00437590`（dtor 系: vtable `0xA36BDC` 設定→`param_1[0xC]` を free、1 caller）＝**解除側**、`FUN_004BAA80`（8 引数・260B バッファ×2 のパーサ/ローダ、**E8 caller 0 件＝間接 dispatch**）＝**登録側**。
- 判定: **埋め込みマップは「u16 キー → アドレス一致ノードリスト」の登録/解除テーブル**。**GMST グローバル `0xB35574` とは無関係（N2 判定は維持）**。所有オブジェクトの正式クラス名（`param_4` の型）は未特定 → **部分確定**。

---

## 3. C-2: `b43070` の D3D caps セマンティクス → **[確定] 判定式・同時計測・入力・鍵帰属まで特定**

### 3-1. 判定式（`FUN_00498340` decompiled L122060–L122098 の逐語確定）

```c
// D3D9 caps チェック（IDirect3D9 vtable slot10 = CheckDeviceFormat, +0x28）
hr1 = vcall(DAT_00b42154, +0x28)(d3d, Adapter=0, DeviceType=1(D3DDEVTYPE_HAL),
                                  AdapterFormat=0x16, Usage=0x80000,
                                  CheckType=3(D3DRTYPE_TEXTURE), CheckFormat=0x71);
DAT_00b350d6 = (hr1 >= 0);                     // L122071/72
hr2 = vcall(DAT_00b42154, +0x28)(…, Usage=0x20000, …);
DAT_00b43071 = (hr2 >= 0);                     // L122073/74（隣接の別クエリ）
DAT_00b43072 = (*(u32*)(caps+0x44) >> 10) & 1; // L122075

// b43070 の本体（L122080–L122085）
if (DAT_00b06de4 == 0 || DAT_00b06cc4 == 0 || DAT_00b350d6 == 0)
        DAT_00b43070 = 0;  // → DAT_00b43073 = 1（条件付き）
else    DAT_00b43070 = 1;  // → DAT_00b43073 = 0
```

- `Usage 0x80000` = **`D3DUSAGE_QUERY_SRGBREAD`**、`Usage 0x20000` = **`D3DUSAGE_QUERY_WRAPANDMIP`**（D3D9 のクエリフラグ）。
- **→ `b43070 = bDoHighDynamicRange:BlurShaderHDR ∧ bDoImageSpaceEffects:Display ∧ CheckDeviceFormat(SRGBREAD)成功`**（v81① の3項構成を確定）。

### 3-2. 鍵帰属の裏付け（v81① の根拠を今回確定）

`bDoHighDynamicRange:BlurShaderHDR`（.rdata `0xA3E810`）と `bDoImageSpaceEffects:Display`（`0xA3E434`）は **.text に abs32 参照0件**（= runtime push 登録は無い）だが、**`.data` に鍵ポインタが格納されている**:

| Setting オブジェクト | value（+0） | 鍵ポインタ（+4） | 鍵文字列 |
|---|---|---|---|
| **`0xB06DE4`** | 1 | `.data 0xB06DE8` → **`0xA3E810`** | `bDoHighDynamicRange:BlurShaderHDR` |
| **`0xB06CC4`** | 1 | `.data 0xB06CC8` → **`0xA3E434`** | `bDoImageSpaceEffects:Display` |

→ **静的初期化された GMST Setting オブジェクト（既定値ともに1）と確定**。CRT 帯の `mov ecx,obj; jmp 0x403BC0` スタブ（`0x9B0DC0` 等）は後始末系（`[obj+4]` の先頭が `0x53` のとき free）。

### 3-3. `FUN_007B45F0`（GPU 検出）の入力と呼び出し元

- **入力（実測）**: `FUN_007B45F0(DAT_00B06C44, DAT_00B06C48, DAT_00B06DB4, buf1, str, uVar5)` で **`0xB06C44 = 0x103` / `0xB06C48 = 0x101` / `0xB06DB4 = 0`**（.data の生 dword、Setting 対象外）。値の意味（caps/ID 候補）は**未特定=残点**。
- **呼び出し元 = 2 件のみ**: `FUN_00498340` @`0x49885B` / `FUN_004980D0` @`0x4982BF`（いずれも D3D 初期化帯 `0x498xxx`）。v76 の「GPU 検出呼び出し元」→ **確定**。

---

## 4. C-3: teardown パス詳細 → **[確定] CRT 初期化テーブル経由・起動時1回**

- **`FUN_007B7250` の E8 caller は 1 件のみ = `0xA10EB5`**（imm32 参照も 0 件 → テーブル/スタブ経由のみ）。
- スタブ全文（`[0xA10EB0, 0xA10EC6)`、無名 gap 関数）:
  ```
  00A10EB0  mov  ecx, 0xb430d8        ; シェーダ系グローバル（COM シングルトン）
  00A10EB5  call 0x7b7250             ; FUN_007B7250 = テーブル teardown/既定化リセット
  00A10EBA  push 0xa27150
  00A10EBF  call 0x981fb4             ; atexit(0xA27150)
  00A10EC4  pop  ecx / ret
  ```
- **`u32(0xA2EFF0) = 0x00A10EB0` を実測確認** → v93 で特定した CRT 初期化テーブル（走査範囲 `[0xA283D0, 0xA2F77C)`、ウォーカ `0x981C6B`）のエントリ → **起動時に呼ばれる初期化スタブ**（＝v81②の「起動時の既定化リセット」を確定）。
- `atexit` 先 `0xA27150` の実体は **`ret`（空スタブ）**（`= 0` による終了時処理は行われない）。隣接 `0xA27160` は別シングルトン `0xB43104` の dtor スタブ。
- → **L671929 の `DAT_00b42f48 = 0` はこの起動時リセットの一部。実行中/終了時に `FUN_007B7250` が呼ばれる経路は存在しない（v81②維持）。**

---

## 5. `0xB42754` vtable の全スロット ID → **[確定] `NiDX9Renderer`・vtable `0xA88EA4`・98 スロット全列挙**

### 5-1. オブジェクトの同定（RTTI）

- 生成: `FUN_0076BD40` 内 `push 0xB00; call 0x401F00(new)` → `mov ecx,eax; call 0x769BE0`（ctor）→ **オブジェクト = `new(0xB00)`（2,816 B）**。ctor は `FUN_0076A1C0(this)` → caller `FUN_0076BD40`（`mov esi,eax` @0x76BD8C）を経由。
- ctor `FUN_00769BE0` の vptr 設定: `[obj] = 0xA88EA4`（主 vtable）/ `[obj+0x604] = 0xA88A3C`（埋め込み NiTMap の vtable）。
- **RTTI**: vtable `0xA88EA4` の COL=`0xACC9D0` → TypeDescriptor `0xB29428` → **`.?AVNiDX9Renderer@@`**。
  - 埋め込み vtable `0xA88A3C` の TD `0xB28E10` = **`.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PAVNiVBBlock@@PAVPrePackObj…@@`**（+0x604 の NiTMap メンバ）。
- **`0xB42754 = NiDX9Renderer\*** （setter `FUN_00772060`（L618178 `DAT_00b42754 = param_1`、caller: 設定 `FUN_00778F60` / クリア `FUN_00779010(0)`）、getter `FUN_0077C0F0` = `return DAT_00B42898`）。派生グローバル: `0xB42750`（obj+0x280）、`0xB42758`（obj+0x8AC）、`0xB4203C`（`FUN_0075FB30` も同一 obj を格納）。

### 5-2. 主 vtable `0xA88EA4` 全 98 スロット（+0x000〜+0x184、+0x188 = 0 で終端）

| idx | +off | impl | idx | +off | impl | idx | +off | impl |
|---|---|---|---|---|---|---|---|---|
| 0 | +0x000 | 0x76BED0 | 33 | +0x084 | 0x769F10 | 66 | +0x108 | 0x768700 |
| 1 | +0x004 | 0x769F00 | 34 | +0x088 | 0x7625A0 | 67 | +0x10C | 0x7635C0 |
| 2 | +0x008 | 0x6F7070 | 35 | +0x08C | 0x7625B0 | 68 | +0x110 | 0x768740 |
| 3 | +0x00C | 0x6F7070 | 36 | +0x090 | 0x762550 | 69 | +0x114 | 0x768780 |
| 4 | +0x010 | 0x6F7070 | 37 | +0x094 | 0x764770 | 70 | +0x118 | 0x68F970 |
| 5 | +0x014 | 0x6F7070 | 38 | +0x098 | 0x701610 | 71 | +0x11C | 0x7687D0 |
| 6 | +0x018 | 0x700640 | 39 | +0x09C | 0x762570 | 72 | +0x120 | 0x763620 |
| 7 | +0x01C | 0x7008A0 | 40 | +0x0A0 | 0x762580 | 73 | +0x124 | 0x7636F0 |
| 8 | +0x020 | 0x68F970 | 41 | +0x0A4 | 0x7625C0 | 74 | +0x128 | 0x60D0A0 |
| 9 | +0x024 | 0x700650 | 42 | +0x0A8 | 0x7625E0 | 75 | +0x12C | 0x60D0A0 |
| 10 | +0x028 | 0x68F970 | 43 | +0x0AC | 0x7625F0 | 76 | +0x130 | 0x76BE00 |
| 11 | +0x02C | 0x700670 | 44 | +0x0B0 | 0x768E90 | 77 | +0x134 | 0x762600 |
| 12 | +0x030 | 0x701A30 | 45 | +0x0B4 | 0x767860 | 78 | +0x138 | 0x769340 |
| 13 | +0x034 | 0x68F970 | 46 | +0x0B8 | 0x763010 | 79 | +0x13C | 0x764C70 |
| 14 | +0x038 | 0x700750 | 47 | +0x0BC | 0x767810 | 80 | +0x140 | 0x762640 |
| 15 | +0x03C | 0x68F970 | 48 | +0x0C0 | 0x763020 | 81 | +0x144 | 0x762BE0 |
| 16 | +0x040 | 0x6F7070 | 49 | +0x0C4 | 0x767990 | 82 | +0x148 | 0x7647E0 |
| 17 | +0x044 | 0x6F7070 | 50 | +0x0C8 | 0x763050 | 83 | +0x14C | 0x768D60 |
| 18 | +0x048 | 0x68F970 | 51 | +0x0CC | 0x765290 | 84 | +0x150 | 0x762FB0 |
| 19 | +0x04C | 0x6FDFF0 | 52 | +0x0D0 | 0x7625D0 | 85 | +0x154 | 0x766F30 |
| 20 | +0x050 | 0x762480 | 53 | +0x0D4 | 0x764E50 | 86 | +0x158 | 0x76B310 |
| **21** | **+0x054** | **0x769F20** | 54 | +0x0D8 | 0x764FE0 | 87 | +0x15C | 0x76B310 |
| 22 | +0x058 | 0x769F70 | 55 | +0x0DC | 0x765120 | 88 | +0x160 | 0x7693E0 |
| 23 | +0x05C | 0x769F80 | 56 | +0x0E0 | 0x769F30 | 89 | +0x164 | 0x7693E0 |
| 24 | +0x060 | 0x76A040 | 57 | +0x0E4 | 0x769F40 | 90 | +0x168 | 0x7694F0 |
| 25 | +0x064 | 0x769F90 | 58 | +0x0E8 | 0x76A190 | 91 | +0x16C | 0x769700 |
| 26 | +0x068 | 0x76A110 | 59 | +0x0EC | 0x76A1A0 | 92 | +0x170 | 0x765990 |
| 27 | +0x06C | 0x76A170 | 60 | +0x0F0 | 0x725FA0 | 93 | +0x174 | 0x766880 |
| 28 | +0x070 | 0x76A180 | 61 | +0x0F4 | 0x60CF60 | 94 | +0x178 | 0x76B160 |
| 29 | +0x074 | 0x764A20 | 62 | +0x0F8 | 0x60CF60 | 95 | +0x17C | 0x768830 |
| 30 | +0x078 | 0x764BB0 | 63 | +0x0FC | 0x4348A0 | 96 | +0x180 | 0x768850 |
| 31 | +0x07C | 0x762590 | 64 | +0x100 | 0x6FDFF0 | 97 | +0x184 | 0x768870 |
| 32 | +0x080 | 0x769F60 | 65 | +0x104 | 0x763560 | 98 | +0x188 | 0（終端） |

- **意味づけが付いたスロット**: **`+0x054`（idx21, impl `0x769F20`）= フラグクエリ**（`FUN_007716E0` が2回呼び出し、bit3/bit2 を判定: `(f&8)==0 && (f&4)!=0` のとき描画ステート設定へ）。
- 注: 「name」列は台帳の包含関数（bisect）であり、ctor 帯の小関数（`0x769F00`〜`0x769F90` 等）は**台帳ギャップの独立メソッド**（impl アドレスが実体）。annotate_db には当該地址の名前なし。
- 埋め込み vtable `0xA88A3C`（+0x604 メンバ、NiTMap）: 7 スロット（`0x768AC0 / 0x6A9060 / 0x763E80 / 0x67F130 / 0x68F970 / 0x982D85 / 0x982D85`、+0x1C = RTTI `0xACC494`）。

---

## 6. C-10: ハーネス残 bug / entry162 マーカー → **[確定] (a)(b) は V77/V78 で解消済みを実装確認・(c) 区分確定**

| 項目 | 判定 | 根拠 |
|---|---|---|
| (a) `rd()` の .rdata/.data 0x600 シフト | **解消済み（V78）** | v76 追記のとおり .data 定数 = `0x401400`（file_off = VA − 0x401400）を確定済み。本セッションでも `.data` 参照を VMA = printed + 0x400000 で解決して全一致（P1/P4 でも同一換算を使用） |
| (b) `dec_index` ヘッダ正規表現 | **解消済み（V77）** | `probe_harness.Ctx.dec_index`（property）の実装を実読: `HDR = re.compile(r"^// (FUN_[0-9a-fA-F]+) @ 0x")` → **ヘッダコメント起点・lowercased・20,465 件**（docstring に明記）。旧「split-signature 型欠落872件」は解消 |
| (c) entry162 マーカー前後の区分 | **確定** | opcode テーブル（base `0xB0AF48`, stride 0x28, 532 エントリ）を opid 実測: **idx0..30 = opid `0x0000..0x001E`（イベント/ブロック型 31件）／ idx31..162 = opid `0x0100..0x0183`（コンソールコマンド 132件、末尾 idx162 = マーカー `ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!` @opid 0x0183）／ idx163..531 = opid `0x1000..0x1170`（スクリプト関数 369件）**。境界検証: idx0..161 の opid max = `0x0182`（≥0x1000 は **0 件**）、idx163..531 の min = `0x1000`（<0x1000 は **0 件**）→ **帯の分離は完全**（31+132+369 = 532 ✓）。マーカー名の指示「console commands をこの前に追加」は 0x01xx 帯=console と整合（v75a の「console 0x100-0x183」も一致）。 |

---

## 残点（記録）

1. `0xB3CAC0` の正式クラス/初期化元（identity 番兵の所有者）。
2. `FUN_007B45F0` 入力 `0x103` / `0x101` / `0` の意味（D3D caps/ID 候補）。
3. `FUN_007b4010` の `param_4`（埋め込みマップ所有オブジェクト）のクラス名。
4. `NiDX9Renderer` vtable の各スロットの**名称**（impl アドレスは全数確定済み。台帳ギャップの小関数が多数）。

## 証跡（すべて `%TEMP%\opencode\`）

| スクリプト / 出力 | 内容 |
|---|---|
| `v287_p5.py` + `_out.txt` | C-7（0xB3CAC0 53refs + FUN_00560920 本文）/ C-8（FUN_007b4010 71行+caller）/ C-2（L122055-122100 実読 + GPU detect callers）/ C-3（teardown caller1件）/ 0xB42754 refs・writer / C-10c（帯境界 opid 実測） |
| `v287_p5b.py` + `_out.txt` | 0xB3CAC0 の decompiled 文脈16件 / FUN_00772060 caller・本文 / FUN_007716E0・771830 本文 / C-8 supplier / CRT スタブ disasm（`mov ecx,0xB430D8; call 0x7B7250`）/ dec_index property 実読 |
| `v287_p5c.py` + `_out.txt` | GMST getter スタブ / `u32(0xA2EFF0)=0xA10EB0` / `0xA27150`=ret / setter caller 窓 / annotate_db 名（`NiControllerManager_FindSeq` のみ） |
| `v287_p5d.py` + `_out.txt` | FUN_0077C0F0=`DAT_00B42898` getter / FUN_00778f60 caller / bDo* 鍵の .text 参照0件 / 0xB06xxx 定数ダンプ |
| 追加 inline probe | `.data` 鍵ポインタ（0xB06DE8/0xB06CC8）/ FUN_00403BC0 後始末 / 生成経路（new(0xB00)→ctor `0x769BE0`）/ **RTTI `.?AVNiDX9Renderer@@`** / vtable 98スロット全ダンプ |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。内部整合: vtable 98スロット(+0x188=0 終端) / RTTI pTD 名解決 / opcode 帯 31+132+369=532 ✓ / `u32(0xA2EFF0)` = v93 記載一致 / bDo* 鍵 .data 归属 2/2。
