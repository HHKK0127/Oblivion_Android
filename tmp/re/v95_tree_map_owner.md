# V95: TESObjectTREE +0x0C のクラス/用途と slot#71 レシーバの同定（2026-10-25）

ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl`）。
対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（pefile なし・手動パース）、`docs/Oblivion_decompiled.c`（dec_index 20,465 件、`enclosing_fun()` で行→関数帰属）。
rd() セクション対応: .text const=0x400C00 / .rdata const=0x400E00 / .data const=0x401400、bss は None+警告。
前提資料: `tmp\re\v81_shader_ini_residual.md` §4 / §6 判定表④ / §7-5、`tmp\re\v76_data_relationship_map.md` N2。

**結論（3 行）**

1. **[訂正] `*(param_4+8)` は「マップの this」ではなく「キー（u32）」。マップの this はグローバル `0xB2C33C`**（`mov ecx,0xb2c33c; call 0x55e000` @0x7B40EC-F5）。Ghidra は global-this を省略するため decompiled では 2 引数に見える（v76 注記どおり）。
2. **TESObjectTREE +0x0C はポインタではなく u32 ID**（TESForm 構築経路 FUN_0046C460 が `param_1[3] = FUN_00448d40()` で書く）。マップ本体はグローバル 0xB2C33C = `NiTPointerMap<unsigned int, CachedDistantLOD*>`、bucket は +8、key = その u32、value = 0x30B の 2×NiTList レコード。
3. **slot#71 のディスパッチ形は `call [reg+0x11C]` ではなく `mov R,[vptr+0x11C]; call R`**（前者は 0 件）。実サイト 16/12 関数のうち **引数整合 (8) は `FUN_004bcef0` の 1 サイトのみ**。`0x004BAA80` はファイル内 1 回のみ → レシーバの動的型がちょうど TESObjectTREE のときだけ到達。

---

## §a. TESObjectTREE の型と +0x0C 先の「オブジェクト」の同定

### a-1. vtable 0xA44FB4 からの型確認 [確定]

| 項目 | 値 |
|---|---|
| vtable base | `0xA44FB4`（93 スロット、末尾 0xA45124） |
| vtable[-1] = COL | `0xABBB28` |
| TypeDescriptor | `0xB051B0` = `.?AVTESObjectTREE@@`（`.rdata` 内、他コピー `0xB04BB8` も同名） |
| slot#71 | `rd(0xA450D0) = 0x4BAA80` ✓（v81 と一致） |
| 既存成果物 | `tmp\re\rtti_vtables.txt:444` / `vtable_map.txt:444` = `0x00A44FB4 93 .?AVTESObjectTREE@@` |

**同一クラスの vtable は 3 本**（COL の offset フィールドで裏付け）:

| base | slots | COL offset | 備考 |
|---|---|---|---|
| `0xA44FB4` | 93 | 0 | 本体 |
| `0xA44F94` | 7 | **36 (0x24)** | +0x24 サブオブジェクト用 adjustor vtable |
| `0xA44F78` | 6 | **60 (0x3C)** | +0x3C サブオブジェクト用 adjustor vtable |

構築経路 `FUN_004b9da0`（TESObjectTREE ctor）:

```
004B9DCB  call 0x4b31f0                      ; base ctor
004B9DD0  lea edi,[esi+0x24] / mov ecx,edi
004B9DDB  call 0x46d7e0                      ; TESModelTree ベース構築
004B9DE0  mov dword [edi], 0xa44f10           ; TESModelTree vtable
004B9DE6  lea ebp,[esi+0x3c] / mov ecx,ebp
004B9DF0  call 0x46ffd0                      ; TESIconTree ベース構築
004B9DF5  mov dword [ebp], 0xa44f30           ; TESIconTree vtable
004B9E02  mov dword [esi], 0xa44fb4           ; ← 本体 vptr
004B9E08  mov dword [edi], 0xa44f94           ; ← ベースの vptr を adjustor に差し替え
004B9E0E  mov dword [ebp], 0xa44f78
004B9E15  mov dword [esi+0x48], 0xa44f6c      ; NiTArray<I> メンバ
004B9E3A  mov byte [esi+4], 0x1e              ; formType = 0x1E (TREE)
```

adjustor thunk `0x4B9E90: sub ecx,0x24 ; jmp 0x4ba3a0` → **非仮想多重継承（TESModelTree が +0x24、TESIconTree が +0x3C のベースサブオブジェクト）** [確定]。

**継承チェーン（vtable が裏付け）**: `FUN_004b9da0` → `FUN_004b31f0`（vptr `0xA43AAC` = `.?AVTESBoundTreeObject@@`, 73 slots）→ `FUN_0046c460`（vptr `0xA3BE3C` = `.?AVTESForm@@`, 55 slots）。
BTO(73) と TREE(93) のスロット前半比較: **66/73 一致 / 差分 7** = index `{4, 7, 9, 27, 45, 46, 71}` → **slot#71 は同一仮想関数のオーバーライド**（`0x4B3030` → `0x4BAA80`）[確定]。

`+0x04` byte = formType は他クラスでも確認: STAT=`0x1C`, MISC=`0x1B`, CONT=`0x17`, DOOR=`0x18`, TREE=`0x1E` → **+0x0C は TESForm レベルのフィールド** [確定]。

### a-2. `*(param_4+8)` の正体 = **u32 ID（ポインタではない）** [確定・v81 から訂正]

**find の `this` はグローバル `0xB2C33C`**（`FUN_007b4010` 末尾）:

```
007B40DF  mov edx, [esp+0x34]        ; edx = param_4
007B40E3  mov eax, [edx+8]           ; eax = *(param_4+8)  ← キー
007B40E6  lea ecx, [esp+0x40]
007B40EA  push ecx                   ; arg2 = &out
007B40EB  push eax                   ; arg1 = キー
007B40EC  mov ecx, 0xb2c33c          ; ★ this = グローバル
007B40F5  call 0x55e000              ; FUN_0055e000(this=0xB2C33C, key, out)
```

`FUN_0055e000` は `mov esi,ecx` で thiscall・`ret 8`（スタック 2 引数）→ decompiled の `FUN_0055e000(*(param_4+8), &param_7)` は **(key, out)**。v76 の「decompiled は global-this を省略する」注記がここでも機能しており、v81 §4.5 は arg1 を this と誤読していた。

**キーの供給元 = `*(param_1 + 0xC)` = TESObjectTREE の +0x0C**（L146273 `uStack_220 = *(param_1 + 0xc)`、param_4 = `&local_228` で +8 = `uStack_220`）。asm でも `004BAB16 mov eax,[esi+0xc]` / `004BAC21 mov ecx,[esi+0xc]`（esi = ecx = this）で一致。

**その +0x0C を書くのは TESForm 構築経路**（`FUN_0046c460` = TESForm ctor、decompiled L85949–86032）:

```c
*param_1 = &PTR_FUN_00a3be3c;      // vptr
*(undefined1 *)(param_1 + 1) = 0;   // +0x04 formType byte
param_1[2] = 8;                    // +0x08 = 8
param_1[3] = 0;                    // +0x0C = 0
if ((DAT_00b33a98 != 0) && (TLS[+0x184] == 0)) {
    uVar3 = FUN_00448d40();        // ★一意 ID の採番
    param_1[3] = uVar3;
    ...
    if ((uVar7 < 0x800) && (uVar7 != 0x800)) {   // ID が 0x7FF 以下
        ... FUN_004524c0(uVar7);                 // map 0xB0613C から erase
        ... FUN_00446b40(param_1[3]);            // アロケータへ返却
        FUN_00452570(0x800, param_1);            // insert(0x800 → this)
    }
    param_1[3] = 0x800;                          // センチネル
}
if (param_1[3] != 0) FUN_00452570(param_1[3], param_1);   // insert(id → this)
```

- `FUN_00448d40` = グローバル `DAT_00b33a98`（オブジェクト、+0x8C0 がカウンタ、+0x8C4 が別値）の一意 ID 採番器。low24 溢出時は `+0x800` で折り返す。
- 登録先は **map `0xB0613C`（id → TESForm\*）**、重複検査も同マップ（`FUN_00448d40` 内）。
- `FUN_007b2a00(id)` は `mov ecx,0xb2c34c; call 0x4a1ab0` = **map `0xB2C34C` で解決 → NiPointer をリリース**（L668869）→ **キーが「オブジェクトではなく u32」であることを独立経路で裏付け**。

**判定: `+0x0C` 先に「オブジェクト」は存在しない。`+0x0C` = TESForm レベルの u32 ID/ハンドル。[確定]**

### a-3. 「bucket +8」を持つマップ本体 = グローバル `0xB2C33C` [確定]

| 項目 | 値 |
|---|---|
| 静的 vptr | `0xA8D190` = `NiTMapBase<NiTPointerAllocator<unsigned int>, unsigned int, CachedDistantLOD@DistantLODShaderProperty>` |
| ランタイム vptr | `0xA8D2B0` = `NiTPointerMap<unsigned int, CachedDistantLOD@DistantLODShaderProperty>`（`FUN_00a0a290` @`0xA10DEB` が `mov dword [0xb2c33c],0xa8d2b0`） |
| レイアウト | `+0 vptr` / `+4 hashSize = DAT_00b2c340`（静的 `0x25` = 37）/ **`+8 buckets = DAT_00b2c344`（静的 0 = 遅延確保）** / `+0xC count = DAT_00b2c348` |
| キー型 | `unsigned int` = TESObjectTREE +0x0C の u32 |
| 値型 | `CachedDistantLOD*` = **0x30 バイトのレコード** |

メソッド実体（`ret 8` / `ret 0xc` で thiscall）:

| 役割 | VA | 呼出し元（0xB2C33C の imm32 参照は .text 内 8 件のみ） |
|---|---|---|
| find | `0x55E000` | `FUN_007b4010` @0x7B40F5、`FUN_007b3be0` @0x7B3C88 |
| insert | `0x452570` | `FUN_007b3be0` @0x7B3CD3 |
| erase | `0x4524C0` | （0xB0613C 側で使用） |
| iterator 1 ステップ | `0x452600` | `FUN_007b3a40` @0x7B3AA4、`FUN_007b3db0` @0x7B3E05 |
| begin（先頭非空バケット） | `0x6A9030` | （`FUN_004bcef0` 経由・後述 §b） |
| vptr 書き換え | `0xA10DEB` | `FUN_00a0a290` |
| 破棄 | `0xA27101` | `mov ecx,0xb2c33c; jmp 0x7b3860` |

**value（0x30B）の構築** = `FUN_007b3be0` 内 `iVar7 = FUN_00401f00(0x30); puVar10 = FUN_007b3b70();`:

```
007B3B70  mov eax,ecx
007B3B72  [eax+0..0x08] = 0
007B3B85  mov dword [eax+0x0c], 0xa8d268   ; ← 埋め込み vptr
007B3B8C  [eax+0x10] = 0                    ; list A head
007B3B98  mov dword [eax+0x20], 0xa770a4    ; ← 埋め込み vptr
007B3B92  [eax+0x24] = 0                    ; list B head
007B3B8F  [eax+0x2c] = 0
```

- `0xA8D268` = `.?AV?$NiTPointerList@PAVDistantLODGroup@@@@`（head = +0x10）
- `0xA770A4` = `.?AV?$NiTList@I@@`（head = +0x24、要素 = `unsigned int`）
- リストノード = `{+0 next, +8 data}`（走査は `piVar3[2]` = +8 を比較）

**用途（decompiled 確定）**:

- `FUN_007b4010`（L669961–670029）: key で find → 成功時 `value+0x24` の `NiTList<unsigned int>` を `param_2` と線形照合（一致で return、不一致で `FUN_005b1e20(&param_2)`）。ループ本体は `param_5`（12B レコード配列 + 8B ヘッダ）と `param_6`（f32 配列）を `FUN_00812510` へ流し、`DAT_00b42d60 += param_7 & 0xffff`。
- `FUN_007b3a40`（L669601–669667）: map を全走査し、value+0x24 に key 一致ノードがあれば、value+0x10（`NiTPointerList<DistantLODGroup*>`）を走って `FUN_00802a60(param_1)==0` の要素を **ノード除去 + `FUN_00401f20` (free) + `DAT_00b42d5c--`**。
- `FUN_007b3db0`（L669823–669858）: 全 value を `FUN_007b3940()` + `FUN_00401f20` で破棄し `DAT_00b42d5c = 0; DAT_00b42d60 = 0`（＝一括クリア）。
- **姉妹マップ `0xB2C34C`** = `NiTMapBase<..., unsigned int, NiPointer<CachedGeometry@DistantLODShaderProperty>>`（vptr 書手 `FUN_00a0a290` @0xA10E3D → `0xA8D2D0`）。`FUN_007b2a00` が `*(tree+0x0C)` をキーに解決 → **キー = u32 をもう 1 つの経路で確認**。

**判定: マップは「グローバル 0xB2C33C（DistantLOD シェーダ系の `map<unsigned, CachedDistantLOD*>`）」。key = TESObjectTREE+0x0C の u32、value = 2×NiTList を持つ 0x30B レコード（DistantLODGroup* リスト + unsigned リスト）。[確定]**

---

## §b. slot#71 (0xA450D0) の実行時レシーバ

### b-1. ディスパッチ形式 = `mov R,[vptr+0x11C]; call R`（`call [reg+0x11C]` は **0 件**）[確定]

| 手法 | 結果 |
|---|---|
| 明示バイト列 `FF 90/91/92/93/95/96/97 1C 01 00 00` + SIB 形 | **0 件** |
| modrm(mod=10) フィルタ経由の一般走査（.text 591 生ヒット → 375 候補 → 322 デコード）で `call/jmp [reg+0x11c]` | **0 件** |
| slot アドレス `0xA450D0` の .text 内 imm32 参照 | **0 件** |
| 参考: `FF /2 disp8`（`call [reg+disp8]`） | **6,584 件**（内 `[eax+0x1C]` = slot#7 が 140 件） |

→ **本コンパイラは disp32 の間接 call を `mov R,[base+disp32]; call R` に展開する**。v81 の示唆した `FF 90 1C 01 00 00` 形はこのバイナリには存在しない。

### b-2. 呼出し元の全列挙（2 手法の和 = **16 サイト / 12 関数**）[確定]

asm 手法（`.text` 内 `1C 01 00 00` → modrm(mod=10) → `mov R,[R2+0x11c]` → 14 命令以内に `call R`）と decompiled 手法（`+ 0x11c))` 検索、cap=200 → 25 ヒット）の和。

| # | 呼出し元 | サイト | スタック引数 | レシーバ（asm 実測） |
|---|---|---|---|---|
| 1 | `FUN_004aa1f0` | 0x4AA620 | **0** | グローバル **0xB35788**（`mov ecx,0xb35788`） |
| 2 | `FUN_004aa1f0` | 0x4AA630 | 1 | `esi` |
| 3 | `FUN_004abe50` | 0x4AC116 | 0 | `edi` |
| 4 | `FUN_004abe50` | 0x4AC124 | 0 | `esi` |
| 5 | **`FUN_004bcef0`** | **0x4BCFBC** | **8** | **`(arg1+0x0C)` 埋め込みマップのキー** |
| 6 | `FUN_00546930` | 0x54694F | 1 | 引数1（`esi`） |
| 7 | `FUN_005f5d10` | decomp L360488 / L360507 | 1 / 1 | `iVar5` = vptr（`[esi+0x58]` 系） |
| 8 | `FUN_005fc090` | decomp L364269 | 1 | `iVar3` |
| 9 | `FUN_00603500` | 0x603B1B / 0x603B33 | 1 / 3 | `esi` / `[ebp]` |
| 10 | `FUN_00613440` | 0x613458 | 0 | `FUN_005e0f50([this+0x3c])` 戻り値 |
| 11 | `FUN_006134c0` | 0x6134D9 | 0 | 同上 |
| 12 | `FUN_0061e8a0` | 0x61E8BA | 0 | 同上 |
| 13 | `FUN_00656270` | 0x6562C8 | 1 | `ebp`（this） |
| 14 | `FUN_00731f80` | 0x731FA0 | 2 | グローバルポインタ `DAT_00b3f928` |

**偽陽性 1 件（除外）**: `FUN_0051d0d0` @0x51D39C — `mov eax,[edi+0x11c]; mov eax,[eax+0xc]; lea ecx,[edi+0x11c]` = **オブジェクト+0x11C のネスト vptr から slot#3 を呼ぶ**（slot#71 ではない）。
**field アクセスの除外**: decompiled の `+ 0x11c)` 25 ヒット中、`fun_00479f80` / `fun_00540850` / `fun_005e10f0` / `fun_0070d300` / `fun_008dcdf0` / `fun_00939bb0` / `fun_0094fc90` は f32/i32 フィールド参照（計 9 件）→ 除外。

### b-3. どの kind のノードが slot#71 を通すか

**根拠 1（決定的）: `0x004BAA80` のファイル内出現は 1 件のみ = `0xA450D0`（`.rdata`）。**
他 vtable・ジャンプ表・immediates に複製なし → **TESObjectTREE を継承した別クラスは存在しない**（あれば vtable に 0x4BAa80 が複製される）。よって **FUN_004baa80 へ到達できるのは「レシーバの動的型がちょうど TESObjectTREE」のときだけ**。

**根拠 2: シグネチャの突合。**

- ベース実装 `FUN_004b3030`（`0xA43AAC` = TESBoundTreeObject の slot#71、および 72-slot 群 `TESObject*` の slot#71 で共有）は **8 スタック引数**（`push ebp; mov ebp,esp` のフレームで `[ebp+0x24]` = arg8 を読む。Ghidra シグネチャ = this + 8 = 9 パラメータ）。
- オーバーライド `FUN_004baa80` は arg1..arg7 のみ読む（フレーム追跡で確認: arg1 `004BAACB` / arg3 `004BAAC4` / arg4 `004BAAEE` / arg7 `004BAABB`。`0x4BABA9 [esp+0x264]` は push 1 活性時 = arg7）。**arg8 は未使用** → シグネチャ 8 と矛盾しない。
- したがって **「8 スタック引数で slot#71 を呼ぶサイト」= TESObject 系シグネチャと整合**。16 サイト中それは **`FUN_004bcef0` の 1 件のみ**（他は 0/1/2/3 引数 → Actor / Process / REFR / CombatStyle 等の**別階層**の slot#71）。

**レシーバの正体（`FUN_004bcef0`）**:

```
004BCEF5  mov edi, [esp+0x1c]          ; edi = arg1
004BCF1E  movzx eax, word [edi+4]
004BCF22  movzx ecx, word [edi]
004BCF29  call 0x7b2110                 ; ((x&~3)<<14)|(z>>2)  … DistantLOD グリッドハッシュ
004BCF3B  call 0x4ef1d0                 ; CONCAT22(word0, word4)
004BCF40  lea ebp, [edi + 0xc]          ; ★ 埋め込みマップのアドレス
004BCF46  mov ecx, ebp
004BCF4C  call 0x6a9030                 ; begin(): [ecx+4]=bucket数, [ecx+8]=buckets
004BCF6F  mov ecx, ebp
004BCF79  call 0x452600                 ; iterator → key=E+0x24, value=E+0x10
004BCF7E  mov ecx, [esp+0x24]           ; ecx = key
004BCF84  mov esi, [esp+0x10]           ; esi = value
004BCF98  mov edx, [ecx]                ; vptr
004BCF9A  mov edx, [edx + 0x11c]        ; slot#71
004BCFA0..FBB  push ×8                  ; arg8..arg1
004BCFBC  call edx                      ; ← 8 スタック引数
```

引数 8 件 = `local_10[5], local_10[1], local_10[9], local_10[0xc], local_4, local_8, piVar1[7], piVar1[8]`（decompiled L147909–147911 と完全一致）。
`arg1` の +0/+4 は u16 で `FUN_007b2110` が座標として量子化、`.\DistantLODShaderProperty.cpp` のヘルパ（`FUN_007b2130` は "Tried to enable/disable NULL LOD tree parent node." を出力）→ **arg1 = 座標系フィールド + `+0x0C` に NiTMap を埋め込む DistantLOD セル/グループ系オブジェクト [仮説]**。同関数は `FUN_00707370` / `FUN_00707610` / グローバル `DAT_00b34424` を無名関数 `0x437600`（v81 の第 2 呼出し）と共有し、DistantLOD/ツリー系パイプラインに同居する。

**判定: slot#71 の実行時レシーバは「`FUN_004bcef0` が `(arg1+0x0C)` の埋め込みマップからイテレータで取り出すキー（オブジェクト）」。そのうち動的型が TESObjectTREE のものだけが `FUN_004baa80` を通る。[仮説]**
（裏付け: 8/8 引数一致は 16 サイト中 1 件のみ + `0x4baa80` 単一出現。**反証不能な残点**: キーの動的型の直接証明が未了。）

**補助 [確定]**: サイト #1 のレシーバ = グローバル `0xB35788` は **TESCombatStyle / DefaultCombatStyle**（vptr 書手 `FUN_009e11d0` @0x9E1207 = `0xA407F4` TESCombatStyle、@0x9E1222 = `0xA4109C` DefaultCombatStyle）→ **TESObjectTREE ではない**。

---

## §c. v76 N2 との整合判定 → **[裏付け（結論）/ 訂正（根拠）]**

**裏付け（結論 NO は正しい）**

- .text 内 `imm32 = 0xB35574`（GMST グローバルマップ）は **19 件**だが、内訳は `FUN_007b4010` [0x7b4010,0x7b4160) / `FUN_007b4290` [0x7b4290,0x7b4520) / `FUN_007b3be0` / `FUN_007b3a40` / `FUN_007b3db0` / `FUN_007b2a00` の **全 6 関数で 0 件** → GMST マップと shader/LOD 系は非接続 ✓。
- `FUN_007b4010` の find の `this` は `0xB2C33C`（DistantLOD 系グローバル）であって `0xB35574` ではない ✓。

**訂正（根拠部分）**

| v76 / v81 の記述 | 本調査 |
|---|---|
| 「0x7b40f5 は FUN_007b4010 の**埋め込みマップ**」（v76 N2） | **グローバル 0xB2C33C**（`mov ecx,0xb2c33c`）。埋め込みではない |
| 「`*(param_4+8)` = マップの this」（v81 §4.5 / §6④） | **キー（u32）**。this はグローバルで decompiled では省略されている |
| 「+0x0C が指す同一クラスの map インスタンス（bucket +8）」（v81 §6④） | +0x0C は **u32 ID**。bucket +8 は正しいが、それを持つのはグローバル 0xB2C33C |
| 「+0x0C 先のクラス名は未特定」（v81 §7-5） | **オブジェクトは存在しない**（u32）→ 調査対象の立て替え |

---

## 判定サマリ

| # | 項目 | 判定 |
|---|---|---|
| a-1 | TESObjectTREE の型（vtable 0xA44FB4） | **[確定]** RTTI `.?AVTESObjectTREE@@` @0xB051B0、93 スロット、+0x24/+0x3C に adjustor vtable（TESModelTree/TESIconTree を基底に非仮想 MI）、継承 TESBoundTreeObject(73) → TESForm(55) |
| a-2 | `+0x0C` 先のオブジェクトのクラス | **[確定: 訂正]** オブジェクトは存在せず **u32 ID**（TESForm ctor FUN_0046C460 が採番、map 0xB0613C に id→this 登録） |
| a-3 | マップ本体・要素型・用途 | **[確定]** グローバル `0xB2C33C` = `NiTPointerMap<unsigned int, CachedDistantLOD*>`、bucket +8、value = 0x30B{`NiTPointerList<DistantLODGroup*>`@+0x0C/head+0x10, `NiTList<unsigned>`@+0x20/head+0x24}、find 0x55E000 / insert 0x452570 / iter 0x452600 / begin 0x6A9030 |
| b-1 | `call [reg+0x11C]` の有無 | **[確定: 0 件]** 実形式は `mov R,[vptr+0x11C]; call R`（disp8 形 `FF 5x` は 6,584 件） |
| b-2 | 呼出し元の列挙 | **[確定]** 16 サイト / 12 関数（上表）。偽陽性 1 件除外 |
| b-3 | slot#71 を通す kind | **[仮説]** `FUN_004bcef0` の `(arg1+0x0C)` 埋め込みマップのキーで、動的型が TESObjectTREE のもののみ（8 引数一致は 1 サイト、`0x4baa80` 単一出現） |
| c | v76 N2 | **[裏付け（結論）/ 訂正（根拠）]** GMST 非接続は正しいが、「埋め込みマップ」→グローバル 0xB2C33C、「this」→キー に訂正 |

## 残点

1. **`FUN_004bcef0` の arg1 クラス名**: 座標 u16×2 + `+0x0C` 埋め込み NiTMap + `+0x1c/+0x20/+0x28` の構造体。DistantLOD セル/グループ候補だが RTTI/構築経路未特定。**[未特定]**
2. **slot#71 レシーバ（マップのキー）の動的型の直接証明**: vptr が `0xA44FB4` である実行時証拠なし（シグネチャ整合と `0x4baa80` 単一出現のみ）。**[仮説]**
3. **`+0x0C` の u32 の意味論**: FormID か unique object handle か未確定（`FUN_00448d40` の初期カウンタ値・`DAT_00b33a98` の正体・FormID 書手の同定が未了）。採番結果は通常 ≥0x800、0x800 はセンチネル。**[未特定]**
4. **slot#71 の契約（ベース `FUN_004b3030` の意味論）**: 8 引数の内容（`[ebp+8..0x24]`）の解釈が未了。**[未特定]**
5. **走査の網羅性**: `mov R,[reg+0x11C]` から `call R` まで 14 命令超の間引を含むケースは未検証（decompiled `+ 0x11c))` 25 ヒットとの和で補完済みだが、Ghidra が別形に描画した site の残存可能性）。**[残]**

---
検証日: 2026-10-25。ハーネス: `probe_harness.py selftest` 4/4 PASS。本ファイルが V95 の正本（`docs/`・`TODO_re49.md`・`session-memory.json` は未変更）。
証跡スクリプト: `%TEMP%\opencode\v95_recon1.py` 〜 `v95_recon21.py`。
