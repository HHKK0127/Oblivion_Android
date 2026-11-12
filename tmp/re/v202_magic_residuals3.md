# v202_magic_residuals3.md — magic 細部束 V189（毒0xE bit3 読手 / 0x69E410・0x6A0800 クラス名 / apparatus 倍率掛込み / SEFF bit11・bit1 書き手）

- 作成日: 2026-10-29 / 担当: TODO serial 189 (V189) — 分析専任サブエージェント
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / 既存 tmp/re / TODO_re49.md / GT / 台帳 / git / Supermemory / sync は一切変更していない（読取のみ）。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam, ImageBase 0x400000, 7,898,624 B)
- 出典（読取専用）: `tmp/re/v197_magic_residuals2.md` §2/§4 / `tmp/re/GT_CHEATSHEET.md` / `tmp/re/cache/*` / `docs/Oblivion_decompiled.c`
- 中間スクリプト: `%TEMP%\opencode\v202_01..20_*.py`（v197_*.py / v197b_*.py は読み取りのみ参照）
- 注意: 訂正は本ファイルにドラフト記載のみ（GT/docs/TODO 無編集）。
- 凡例: [確定] = 逆アセンブル/データ直接確認 / [部分] = 構造は判明・一部未 / [推定] = 論理推定 / [残] = 未解明。

---

## 1. 手法 / 入力

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl。着手時・成果物完成直前の 2 回）。
- 解析: capstone 5.0.7 detail 付き + `md.skipdata=True` の全 .text 線形走査（**2,258,215 命令** = GT §3 と一致）＋ `docs/Oblivion_decompiled.c` 関数ヘッダ部分読取（GT §2 規則）。
- 参照数は `tmp/re/cache/call_targets.json`、ラベル境界は `cache/named_funcs.txt`、vtable 索引は `cache/vtable_index.tsv`（再走査しない）。
- 中間スクリプトは `%TEMP%\opencode/v202_*.py`、出力は `v202_*_out.txt`。
- 出発点: `v197_magic_residuals2.md` §4 の残留 4 件（bit3 読手 / 0x69E410・0x6A0800 クラス名 / apparatus 倍率 / SEFF bit11・bit1 書手）。
- **走査の教訓（本タスクで踏んだ罠）**: 「`mov reg,[reg+0x14]` 直後の 9 命令窓」だけでは bit 読手を取りこぼす。今回は (a) decompile の式レベル grep、(b) `[reg+0x14]` を**メモリオペランドに持つ全 bit 操作命令**の全数走査、(c) マジック帯 (0x68E000..0x6A3000) のロード全列挙、の 3 系統で突き合わせた。

---

## 2. スコープ項目別解析

### 2-1 毒 flags 0xE の bit3 単独読手（スコープ1）

**結論: bit3 には単独読手が存在しない（write-only）。** [確定]

- **書手の全数（3 系統で一致）**:
  - 逆アス（`[reg+0x14]` への `or/and/xor` で即値に bit3 を含むもの、全 .text）→ **`or dword ptr [X+0x14], 0xE` のみ 3 件**:
    - `0x69B480` / `0x69B4C5` = `FUN_0069AF30`（both `or dword ptr [edi + 0x14], 0xe`）
    - `0x69BB6C` = `FUN_0069BA90`（`or dword ptr [esi + 0x14], 0xe`）
  - decompile の `puVar14[5] |= 0xe` / `puVar6[5] = puVar6[5] | 0xe` → **3 件**: L465968・L465987（`FUN_0069af30`）/ L466221（`FUN_0069ba90`）＝ **3/3 一致** [確定]。
  - 条件は v197 と同一（`if (iVar19 != iStack_48)` / `if (piVar7 != piVar5)` ＝「最高 magnitude の on-strike エントリ以外」に or）。
  - なお `or dword ptr [edi + 0x14], 0x14`（`0x69B2D3`, FUN_0069AF30）は **bit2|bit4** の別フラグで 0xE とは無関係。
- **読手の全否定（3 系統すべて 0 件）**:
  1. **decompile 式レベル**: `0x14)` の後続 40 文字以内に `>> 3` / `& 8` / `0x8` → **0 件**。`param_1[5] >> 3` / `[5] & 8` も **0 件**。(`>> 3 & 1` 全体は 85 件あるが、いずれも `+0x14`/`[5]` に無関係なフィールド)
  2. **逆アス（メモリオペランドが `[reg+0x14]` の全 bit 操作・225 件を全列挙）**: `test/cmp/and/or/xor` の即値に **8 / 0x8 は 0 件**、`bt ...,3` は 0 件、`cmp [X+0x14], 8` も 0 件。`and` の `0xfffffffe` 等は bit3 を**保存する**マスク（クリア/テストではない）。stack (`esp+0x14`) のものは除外して数えている。
  3. **ロード→シフト窓（全 .text + マジック帯）**: `mov eXx,[eXx+0x14]` の後 9 命令に `shr 3` → マジック帯 (0x68E000..0x6A3000, ロード 110 件) で **0 件**。帯内で実際に立っているのは **bit1 (`0x69DAAC: shr ecx,1; test cl,1`) / bit2 (`0x69DA17: shr eax,2; test al,1`) / bit4 (`0x6A1758: shr eax,4`) / bit5 (`0x68E8C9: shr eax,5`) / bit6 (`0x68E88F: shr ecx,6`) / bit0 (`0x68E6E3: test byte [esi+0x14],1`)** ＝ **bit3 のみ欠落** [確定]。
- **bit3 が観測される唯一の経路**: byte 単位の `FUN_00413470`（`[entry+0x18]`→`byte[[ptr]+0x14]` を無マスク返す）。**callers 15 件を逆アス全開示**（0x414344/0x41434D/0x414BA3/0x414BE9/0x414E76/0x414EDC/0x415553/**0x594974**/0x5EE617/0x613CFD/0x616BB1/0x6A1F08/0x6A2A4A/0x6A8417/0x6A853A）。**全 15 件が `test al,al`（非0判定）か `cmp dl,al`（比較）**で、`test al,8` 型の bit3 マスクは **0 件** [確定]。
- **訂正（v197 §4-1 への回答）**: 「bit3 は byte 単位（0x413470）経由でのみ観測」は**正しい**。加えて本タスクで「**bit3 単独の読手は .text 全域に存在しない**」を全数走査で確定した。逆に v202_06 の一次走査で `or [X+0x14], 0xE = 0 件」と出たのは lhs 判定のバグ（`lhs.startswith("[")` が常に偽）による**誤検出**で、v202_20 で 3 件に訂正した（本節の数値は訂正版）。

### 2-2 FUN_0069E410 / FUN_006A0800 のクラス名（スコープ2）

**結論: `MagicModelHitEffect` / `MagicShaderHitEffect`（魔法の命中視覚エフェクト）** [確定]（v197 §4-2 の「sub-effect / DoT インスタンス」推定を**訂正**）

- **境界（`cache/named_funcs.txt` の次エントリ）**: `FUN_0069E410` = 0x69E410..0x69E4F0（size **0xE0**）、`FUN_006A0800` = 0x6A0800..0x6A0980（size **0x180**）[確定]。
- **vptr ストア（逆アス直接確認、v202_01）**:
  - `0x69E44C: mov dword ptr [esi], 0xA761C4` → `cache/vtable_index.tsv` 一致 → **`MagicModelHitEffect`**（mangled `.?AVMagicModelHitEffect@@`、COL `0x00AC5094`）[確定]
  - `0x6A083C: mov dword ptr [esi], 0xA764B4` → 同索引一致 → **`MagicShaderHitEffect`**（mangled `.?AVMagicShaderHitEffect@@`、COL `0x00AC5114`）[確定]
- **ctor 構造**: 両関数とも `push 0x4C(?)系 → call 0x98D8C0`（アロケータ）→ vptr 設置 → `call 0x989B30`（初期化）→ 以降に RTTI（0x6A0800 は `call 0x9832E6` = `__RTDynamicCast`）＝ **ctor 相当** [確定]。`FUN_0069D9F0` は未立ビットごとに 0x38B / 0x4CB バイトを確保してこれらを生成し、`entry+0x14` の bit1/bit2 を立てる（v197 §2-1 と接続）。
- **意味づけ**: 0xE は「サブ効果生成済み」の重複ガードである点は v197 のまま。ただし生成物の正体は **DoT ではなく、命中時に付随するモデル系 / シェーダ系の視覚ヒットエフェクト**（＝毒の残滓表現）[確定]。

### 2-3 apparatus 品質の倍率掛込み位置（スコープ3）

**結論: `FUN_00593CD0`（アルケミ帯）で 4 スロットの品質を float 化し、`FUN_00548420` / `FUN_005486A0` / `FUN_005487F0` の**倍率引数**として渡し、そこで効果値（magnitude 系）に乗算される** [確定]

- **品質の取得（decompile L302817–302842 / 逆アス 0x594869・0x5948B5・0x5948DA・0x5948FF）**: 4 スロットは menu の **`+0x78` / `+0x7C` / `+0x80` / `+0x84`**（各 4 ワード → それぞれ `+8` が form）で、`FUN_0046E3F0(form)` の byte を float 化:
  - `*(menu+0x78)+8` → `fStack_74` → さらに `fStack_78 = FUN_00548400(fStack_74, fStack_88)`
  - `*(menu+0x7C)+8` → `fStack_54`
  - `*(menu+0x84)+8` → `fStack_58`
  - `*(menu+0x80)+8` → `fStack_84`
  - `fStack_88 = FUN_005E0430(0x13)`（`ret 4`、`vt+0x170`=GetBaseForm → `vt+0x190`=IsActor → `vt+0x128(av)` ×2 → `FUN_00547B90` で合成）＝ **アクター値 0x13(=19) の取得**（調合関数内使用＋順序から **Alchemy スキル** [推定]）。`0x1f(=31)` 版も L400877 ほかで実在。
  - `FUN_00548400(q, av)` = **`_DAT_00b379d8 * q + av`**（線形。`_DAT_00b379d8` は .data 末尾 bss 相当＝実行時セット float）[確定]
- **倍率関数 3 種（MGEF `+0x58` の bit7/bit8 で選択、decompile L302884–302903 / 逆アス 0x5949D1–0x594AC9）**:
  | 選択条件 (MGEF+0x58) | 関数 | 逆アス call |
  |---|---|---|
  | bit7==0 かつ bit8==0 | `FUN_00548420` | `0x594A23` |
  | bit7==0 かつ bit8==1 | `FUN_005486A0` | `0x594A7A` |
  | bit7==1 かつ bit8==0 | `FUN_005487F0` | `0x594AC9` |
- **引数対応（`FUN_00548420`、スタック逆算で確定）**: `param_7`=fStack_58(`menu+0x84` 品質) / `param_8`=fStack_54(`menu+0x7C`) / `param_9`=fStack_84(`menu+0x80`) / `param_4`=fStack_78(=`C*q(menu+0x78)+av`) / `param_3`=fStack_88(av) / `param_5`=スロット選択 1..4 / `param_6`=毒 byte(`FUN_00413470` の結果) / `param_10`=毒全体 byte(`FUN_00414EB0`) [確定]。
- **掛込み式（`FUN_00548420` 逆アス 0x548420..0x5484DC + decompile L244330–244411）** [確定]:
  - 先頭: `*param_1 = FUN_00985B70( 1.0 / (_DAT_00b38270 + 1.0) )`、`*param_2 = (*param_1 / _DAT_00b379e0) / _DAT_00b37dd8`（0..1 系の基準値。ただし先に `fmul/fdiv [param_4]/[param_3]` で `K*q0+av` 系の計算も行われる＝** incoming param_4/param_3 は実際に消費される**）
  - `case 1`: `*param_1 += *pfVar3 * param_7 * *param_1`（`pfVar3 = FUN_00403C00(0xb379e8/0xb379f0)` が返す float 定数のアドレス）＝ **`mag *= (1 + K * quality)` 型の乗算** [確定]
  - `case 4`: `*param_1 = *param_2*param_9*rand + *param_1*param_7*rand + *param_1`（param_7 と param_9 の 2 品質を合成）
  - `case 5`: param_8 × `_DAT_00b379f8` / fallback `*param_1 += *param_1 * param_8 * _DAT_00b37a08`（L244404-07）
  - `param_5 == 6` → `ja default` で**何もしない**（下記 0x69AF30 がこれ）
- **出力の行き先（逆アス 0x594AD1–0x594B86）**: `FUN_00548420` の 2 出力 float を **round（`call 0x9828c0` + 端数処理）→ `max(1, …)` でクランプ** →
  - `FUN_004133E0(this=effectItem, v)` → **`this+0xC = v`**（MGEF `+0x58` bit7==0 のときのみ、`this+0x20 = DAT_00a30634` を更新）
  - `FUN_00413340(this, v)` → **`this+4 = v`**（bit8==0 のときのみ）
  - `FUN_00413390(this, 0)` → **`this+8 = 0`**（bit9==0 かつ `this+0x10 != 0` のときのみ）
  - その後 `FUN_00593B20(effectItem, idx)` [確定]
- **接続の補強**: `FUN_0046E3F0` の callers は **9 件**（`0x486330`×4 / `0x593CD0`×4 / `0x4D6600`×1、E8 全走査と cache が 9/9 一致）。`FUN_00486330` は「**最高品質の apparatus を 1 つ選ぶ**」（`local_c < (float)bVar3` で更新、`FUN_0046E3F0` の -1(0xFF) は下回る）、`FUN_004D6600` は REFR→base form の quality を返すラッパ [確定]。
- **補足**: 同じ倍率関数はマジック帯 `FUN_0069AF30` からも呼ばれるが、**全品質引数 0 / `param_5=6`**（decompile L465936-49）＝ **ActiveEffect 側では品質倍率が効かない**（`ja default` で無操作）[確定]。＝倍率掛込みは**調合（crafting）経路のみ**。

### 2-4 SEFF Finish 条件グローバルの bit11 / bit1 書き手（スコープ4）

**前提の再確認**: 実アドレスは **`0x00B33B00`**（v197 の地址訂正どおり）、アクセスは **`[[0x00B33B00] + 0x18]`**（間接）。本タスクの全走査はこの正規化地址で行った。

- **bit11 (0x800) の書き手 = `FUN_00465860` のみ** [確定]
  - **set**: decompile **L80019** `*(uint *)(param_1 + 0x18) |= 0x800`（直前でスレッド判定により `|= 1`（メインスレッド）or `|= 0x40000`（別スレッド））→ 逆アス `0x465AF3: or dword ptr [ebp + 0x18], 0x800`
  - **clear**: 同関数 **L80497** / **L80540** `&= 0xfffff7ff`（逆アス `0x465B9E` / `0x46694F: and dword ptr [ebp + 0x18], 0xfffff7ff`）。直前には `&= 0xfffffffe`(bit0 clear) / `&= 0xfffbffff`(bit18 clear)。
  - **`FUN_00465860` の正体**: `__thiscall FUN_00465860(int param_1, undefined4 param_2, char *param_3, char param_4)`、size **0x1270**（0x465860..0x466AD0）。内部文字列 = `"LoadGame() call did not properly empty buffer"`(0xA3AFA0) / `"LoadGame took %.2f seconds"`(0xA3B1C0) / `"Save Game Header"`(0xA3B1A0) / `"quicksave"`(0xA3B144) / 読込エラー群 → **LoadGame/クイックセーブ系のロード処理本体** [確定]。
  - **`param_1` = シングルトンの裏取り**: callers **cache=5 / 実測=5**（0x466B31, 0x466D5F, 0x5025F7, 0x5AE1E6, 0x5B5DA8）。うち **`0x5025F7` と `0x5AE1E6` は `mov ecx, dword ptr [0xb33b00]`** → この 2 経路でのみシングルトンの bit11 が set/clear される [確定]。
  - = **bit11 = ロード（LoadGame）中立つ「処理中/inhibit」フラグ**。SEFF Finish 条件 `bit11==0 || bit1==1` は「ロード中は ScriptEffect の Finish を止める」[確定]。
- **bit1 (0x2) の書き手 = 3 関数の set + 3 経路の clear** [確定]
  - **set（`param_1 + 0x18 |= 2`）は全 .text で 3 件**:
    | 関数 | decompile | 逆アス | シングルトン caller |
    |---|---|---|---|
    | `FUN_00452A70`（`__thiscall(int param_1, char param_2)`; `param_2!=0` で set / `==0` で clear） | L63481 / L63484 | — | **cache=1 / 実測=1**、`0x5E87CF: mov ecx,[0xb33b00]` → `0x5E87D6 call 0x452a70`（**唯一の caller**） |
    | `FUN_0045C020`（`__thiscall(int param_1, undefined4, byte, char)`） | L72763（`param_4==0` 等の条件下） | — | cache=3 / 実測=3、うち `0x4CBFD8: mov ecx,[0xb33b00]` → `0x4CBFE3 call` の 1 件 |
    | `FUN_0045FDA0`（`__thiscall(int param_1, int, int, char)`） | L75775（`(this+0x18 の bit0 or bit18) != 0` の条件下）/ clear L75987 | — | cache=3 / 実測=3、うち `0x44458B: mov ecx,[0xb33b00]` → `0x444595 call` の 1 件 |
  - **clear**: 上記 3 関数の `&= 0xfffffffd`（L63484 / L72770 / L75987）＋ **`FUN_005E83A0` の L351753 直接 clear**（逆アス `0x5E87B6: and dword ptr [eax + 0x18], 0xfffffffd`、`eax=[0xb33b00]`）。
  - **意味づけ**: `FUN_0045FDA0` は set 前に `*(byte*)(this+0x18)`（メインスレッド）または `>>0x12`（bit18）を見て、**メインスレッド側フラグが立っているときだけ bit1 を立てる**（L75765-75775）。bit1 は「ロード中でも Finish を通す」赦免ビットと読むのが整合的 [推定]。
- **書手の網羅性チェック**: `DAT_00b33b00+0x18` を直接書き換える decompile 行は 79 件、うちビット演算書手は bit0/bit1(only clear)/bit7/bit8/bit10/bit13/bit15/bit16 で、**`|= 0x800`（bit11 set）は L80019 の 1 件のみ、bit1 の set は `param_1` 別名経由の 3 件のみ**（`DAT_00b33b00` 直書きの set は 0 件）[確定]。instruction 級でも `[reg+0x18]` への `or ...,0x800` は `0x465AF3` の 1 件、`or ...,2` は 0 件（すべてレジスタ経由の `or [eax+0x18], esi/edi` は `esi/edi=0x80` 即ち **bit7** と即値追跡で確定: `0x444583 mov esi,0x80` / `0x4CDA41 mov edi,0x80`）。
- **v197 §4-4 の回答**: 「decompile grep に未出現（レジスタ経由の可能性）」は**部分的に正しい**（bit1 set は `param_1` 別名経由）が、**bit11 は `DAT_00b33b00` 直書きでも L80019 に実在した**（grep パターンが `|=` の位置と `0x800` の間に制約を置いたため取りこぼした）。

---

## 3. 結論（3行）

1. **毒 flags 0xE の bit3 には単独読手が存在しない（write-only）**: 書手は `or [X+0x14],0xE` の 3 箇所（`FUN_0069AF30`×2 / `FUN_0069BA90`×1）のみで、読手は decompile（`+0x14)>>3`・`&8`=0 件）、逆アス（`[reg+0x14]` の `test/cmp/and` に 8 なし・`bt 3` なし）、マジック帯のロード 110 件（bit0/1/2/4/5/6 は実在・**bit3 のみ欠落**）の 3 系統すべて 0 件。観測は byte 単位 `FUN_00413470`（callers 15 件すべて `test al,al`）のみ [確定]。
2. **`FUN_0069E410` = `MagicModelHitEffect`（vptr `0xA761C4`）／`FUN_006A0800` = `MagicShaderHitEffect`（vptr `0xA764B4`）**（vtable_index 2/2 一致）。＝毒の bit1/bit2 が生成をゲートするのは **DoT ではなく命中時の視覚ヒットエフェクト**（v197 §4-2 を訂正）。apparatus 品質は `FUN_00593CD0` で menu `+0x78/+0x7C/+0x80/+0x84` の 4 スロットから `FUN_0046E3F0` で取得し、`FUN_00548420/005486A0/005487F0`（MGEF bit7/bit8 で選択）の **param_7/8/9（および `C*q0+av` の param_4）として効果値に乗算**、結果を round→clamp≥1 して effectItem の `+0xC`（`FUN_004133E0`）/ `+4`（`FUN_00413340`）/ `+8=0`（`FUN_00413390`）へ書き込む [確定]。
3. **SEFF Finish のグローバル `[[0x00B33B00]+0x18]` の bit11 は `FUN_00465860`（LoadGame 本体・size 0x1270）が L80019 で set / L80497・L80540 で clear**（シングルトン経由 caller は 0x5025F7・0x5AE1E6 の 2 件）＝「ロード中 inhibit」。**bit1 の set は `FUN_00452A70`（唯一 caller がシングルトン）/ `FUN_0045C020` / `FUN_0045FDA0` の 3 関数**、clear は同 3 関数＋`FUN_005E83A0`(L351753) [確定]。

---

## 4. 残留不明点

1. **bit3 の意味論**（何を表すフラグか）は未解明。書手は「選抜エントリ以外」一括 or のみで、読手が無い以上実行時挙動への寄与は byte 単位の非0判定以外に確認できない [残]。
2. **`FUN_005E0430(0x13)` のアクター値 19 = Alchemy は [推定]**（`vt+0x128(av)` の引数として間接的にしか確認できず、enum の正本は本 EXE 内に無い）[推定]。
3. **`FUN_00548420` 先頭の FPU スタック**（`K*param_4/param_3` を算出してから `1/(0xb38270+1)` で上書きする経路）で **残り 1 スロットが case 切替後も保持される**ように読めるが、case1 の return 直前 (`0x5484D9`) に `ffree/fstp` が無い。デコンパイラの引数再利用（param_4 スロットの書き換え）による表示上の問題の可能性が高く、実挙動は未確定 [残]。
4. **effectItem の `+4 / +8 / +0xC / +0x10` のフィールド名**（magnitude / duration / area のどれかは [推定]）は、`FUN_004133E0/40/90` が MGEF bit7/8/9 で独立ゲートされる点のみ [確定]、命名は未同定 [残]。
5. **`FUN_00465860` の bit11 set がシングルトン以外のオブジェクトでも立つ**（callers 5 件中 3 件は別オブジェクト）ため、「シングルトンの bit11」は LoadGame 経由の 2 caller に限定される。他のオブジェクトの bit11 が SEFF に影響するかは [残]。
6. `_DAT_00b379d8 / 00b379e0 / 00b379e8 / 00b379f0 / 00b379f8 / 00b37a08 / 00b37a10 / 00b37a28 / 00b37a40 / 00b37dd8 / 00b38270` はいずれも **.data の raw 末尾（0xB32C00）より先＝ファイル無バックの bss 相当**で、起動時にセットされる float。セット元（GMST / ini）は未特定 [残]。

---

## 5. 自己検証

- `probe_harness.py selftest` = **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl。着手時・完成直前の 2 回）。
- **内部整合（一致数）**:
  1. 毒 `|= 0xE` 書手: decompile **3** = 逆アス **3**（0x69B480 / 0x69B4C5 / 0x69BB6C）→ **3/3 一致**。
  2. bit3 読手: decompile 式 **0** = 逆アス `[reg+0x14]` に即値 8 の bit 操作 **0** = マジック帯 `shr 3` **0** → **0/0/0 一致**（bit1/2/4/5/6 は帯内に実在＝対照確認）。
  3. `FUN_00413470` callers: cache **15** = 実測 E8 **15**、うち bit マスク **0** → **15/15 + 0**。
  4. クラス名: vptr `0xA761C4` / `0xA764B4` → `vtable_index.tsv` **2/2 一致**（MagicModelHitEffect / MagicShaderHitEffect）。
  5. `call 0x46E3F0`: 実測 **9** = v197/cache の **9**（0x486330×4 / 0x593CD0×4 / 0x4D6600×1）→ **9/9 一致**。
  6. 倍率関数の call site: `FUN_00548420` 逆アス **2**（0x594A23 / 0x69B374）= decompile **2**（L302885 / L465936）、`FUN_005486A0` **2/2**、`FUN_005487F0` **2/2** → **6/6 一致**。
  7. bit1/bit11 書き手関数の caller 数: `FUN_00452A70` cache1=実測1 / `FUN_0045C020` 3=3 / `FUN_0045FDA0` 3=3 / `FUN_00465860` 5=5 → **4/4 関数で cache 一致**。
  8. bit11 set/clear の decompile 行 = 逆アス 命令: set **1/1**（L80019 ↔ 0x465AF3）、clear **2/2**（L80497↔0x465B9E、L80540↔0x46694F）。
  9. 全 .text 命令数 **2,258,215** = GT §3 の値（走査網羅の裏付け）。
- 数値・地址はすべて probe（capstone 逆アス / decompile 行番号 / cache/call_targets.json・named_funcs.txt・vtable_index.tsv）で裏取り。旧メモ・要約の地址は不採用（GT §4 規則）。本タスクで 1 件、自前の走査バグ（v202_06 の lhs 判定）を検出して v202_20 で訂正済み。

---

## 6. Android 移植への示唆

移植では 4 案件をそれぞれ独立モジュールに切り、（1）毒 flags 0xE の **bit3 を実装しなくてよい**ことを明示するのが要点である（write-only なので、byte 単位の「非0=毒」述語と bit1/bit2 の生成ガードだけを実装すれば元 exe と観測上等価。bit3 は `|= 0xE` の一括セットとして保持するだけで挙動差は生じない）。（2）命中視覚エフェクトは `MagicModelHitEffect` / `MagicShaderHitEffect` の 2 クラスとして立て、ActiveEffect の bit1/bit2 に「生成済み」を atomic に立てる冪等ガードをかませば double-spawn 回避の要点を再現できる。（3）apparatus 品質は「`TESQualityForm+4` の float を round→byte 化し、4 スロットぶんを float で保持 → `qualityMultiplier(slotIdx, poisonFlag, slotMask)` という純関数（= FUN_00548420 相当、内部で `(1 + K*quality)` 型の乗算）に通し、結果を round & clamp≥1 して効果アイテムへ書き込む」というパイプラインに丸ごと写すのが安全。倍率定数群は .bss 由来の実行時セット float なので、ハードコードせず設定/GMST 読み込み側に置く。（4）SEFF Finish の gate は `bit11==0 || bit1==1` の 1 式に集約し、bit11 を「ロード中 inhibit」（LoadGame 入口で set・完了で clear）、bit1 を「赦免」（メインスレッドフラグが立っているときだけ set）として実装すれば元の挙動に一致する。絶対アドレス `0x00B33B00` は依存を排し、シングルトン参照に置き換える。実装は本タスクの範囲外（分析専任）。
