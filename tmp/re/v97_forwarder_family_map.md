# V97: フォワーダ族の呼出先マップ

- 日付: 2026-10-25
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (Steam v1.2.0416 / ImageBase 0x400000)
- 前提資料: `tmp/re/v79_absorbed_tracking.md` §6 残課題 4（「フォワーダ族の呼出先マップ」）
- 自己検証: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時）／末尾 §7 に完了時再実行結果
- 成果物: 本ファイル 1 件のみ（新規）。既存 docs/・tmp/re ファイルは無変更。

## §0 手法と入力

### 0.1 入力ファイル

| 入力 | 用途 | 状態 |
|---|---|---|
| `tmp/re/v79_absorbed_tracking.md` §6-4 | 前提・課題定義 | あり（`## §6 残課題` の項目 4） |
| `%TEMP%\opencode\v79_classified.csv` | 357 VA のクラス / `skel_n` | あり（358 行） |
| `%TEMP%\opencode\v79_bodies.json` | 357 VA のデコンパイル本文（骨格計算の元） | あり（220,358 B） |
| `%TEMP%\opencode\v70_decomp_dump.txt` / `v70_force_create.csv` | V70 デコンパイル dump | あり |
| `tmp/re/v10_impl_decompile_join.csv` | 527 impl の `prologue16` / `cc_prefix` / `gap_kind` | あり（527 行） |
| `tmp/re/v5_ghidra_gaps_report.md` | gap 362 行（cc 前置・judge） | あり |
| `tmp/re/v69_show1stperson_link.md` | V69 正本 | あり（タスク記載の `v69_s1st_handler_map.md` は**非存在**） |
| `docs/Oblivion_decompiled.c` | 関数本文の部分検索 | あり（28,995,463 B） |
| `%TEMP%\opencode\func_list_new.txt` | Ghidra 台帳 21,597 件 | あり |
| `probe_harness.py` `Ctx.rd()` / capstone 5.0.7 (skipdata) | PE 読取・逆アセンブル | selftest 4/4 PASS |

> タスク記載の `tmp/re/v70_gap_decompile.csv` / `tmp/re/v70_impl_map.md` / `tmp/re/v69_s1st_handler_map.md` は
> このチェックアウトに存在しない。代替として v79 系の CSV/JSON（同一解析の産物）を使用した。

### 0.2 手順

1. **骨格族の抽出**: `v79_bodies.json` の本文を `re.sub(r"\s+","",…)` の後、`FUN_<hex>` / `func_0x<hex>` / `0x<hex>` をすべて `@` に置換（= `v79_classify2.py` の `skel()` と同一規則）して骨格キーとし、同一キーでグループ化。
2. **オペコード表の再読込**: `rd()`（セクション対応、.data const=0x401400）で `0xB0AF48` から 532 × 0x28 を読み、`+0x18 impl` / `+0x1C dispatcher` / `+0x20 cond` / `+0x12 argn` を採取。再読込結果は **impl≠0 = 527 / `+0x1C`==0x4FDE30 = 528 / `+0x20`≠0 = 169** で V77/V78 の GT と完全一致。
3. **呼出し先の採取**: 各メンバーを capstone 線形逆アシンブル（`ret` の直後が `0xCC`/`0x00`/`0x90` なら打ち切り）し、さらにデコンパイル本文の `FUN_x` / `func_0x` 参照を和集合化。自己参照（関数自身のシグネチャ）は除外。
4. **helper の解析**: 同様に逆アシンブルし、特徴量を採取 — `fst/fstp qword ptr […]`（結果格納）、`fldz`、絶対グローバル参照、push した .rdata 文字列、E8 呼出し先集合、`mov reg,[obj+0xNN]`（VT 参照）。
5. **E8 xref 計数**: `.text`（VA 0x401000 / raw 0x400 / 6,450,233 B）全域を走査して rel32 ターゲット別に呼出しサイトを集計（走査結果 14,185 ターゲット）。
6. **独立性判定**: 未登録 helper の直前 1 バイト（`0xCC` / `0xC3` / `0x00`）と、直前のラベル済み関数の終端位置を確認。

### 0.3 用語

- **メンバー** = 骨格族に属するオペコードハンドラ（`+0x18 impl` 列の値そのもの）。
- **helper** = メンバーが E8 で直接呼ぶ関数（0x004F4xxx〜0x004F9xxx 帯が中心）。
- **骨格族** = 骨格キーが同一のグループ。`prologue16`（v10 CSV）は `call` の rel32 下位バイトで細分化されるため、必ずしも一致しない（§1 注記）。

## §1 族一覧（13 族 / 115 メンバー）

`v79_summary.json` の `skel_dup_groups=13` / `skel_dup_members=115` と完全一致。

| 族 | 件数 | 骨格長 | 代表 prologue16 (先頭20B) | opid 帯 | opid 範囲 | +0x18 一致 | +0x1C | +0x20≠0 | 代表 VA | オペコード例 |
|---|---|---|---|---|---|---|---|---|---|---|
| FAM-01 | 47 | 130 | `8b44241c8b4c240c506a` | C×47 | 0x1018-0x116D | 47/47 | 0x4FDE30 47/47 | 47/47 | `0x005019e0` | GetFatiguePercentage 他46 |
| FAM-02 | 20 | 180 | `8b44240c85c074138b4c` | C×20 | 0x1065-0x111D | 20/20 | 0x4FDE30 20/20 | 20/20 | `0x00504890` | IsWeaponOut 他19 |
| FAM-03 | 8 | 137 | `8b44241c8b4c240c506a` | C×8 | 0x10BE-0x116A | 8/8 | 0x4FDE30 8/8 | 8/8 | `0x00505460` | GetAmountSoldStolen 他7 |
| FAM-04 | 6 | 180 | `8b44240c85c0750332c0` | C×6 | 0x1074-0x10B9 | 6/6 | 0x4FDE30 6/6 | 6/6 | `0x00504ce0` | GetCrimeGold 他5 |
| FAM-05 | 6 | 31 | `b900bdb300e8c6651700` | B×5 / C×1 | 0x0107-0x1145 | 6/6 | 0x4FDE30 6/6 | 0/6 | `0x005007c0` | PrintNPCDialog 他5 |
| FAM-06 | 6 | 384 | `518b4c24188b54241456` | A×6 | 0x0016-0x001B | 6/6 | 0x4FDE30 6/6 | 0/6 | `0x00503d90` | OnSell 他5 |
| FAM-07 | 5 | 75 | `8b44241c506a006a006a` | C×5 | 0x1012-0x10B0 | 5/5 | 0x4FDE30 5/5 | 5/5 | `0x00501c00` | GetCurrentTime 他4 |
| FAM-08 | 5 | 52 | `833dc433b300007405e8` | C×5 | 0x10D5-0x10EB | 5/5 | 0x4FDE30 5/5 | 0/5 | `0x005056f0` | AdvancePCLevel 他4 |
| FAM-09 | 3 | 170 | `8b44241c8b4c24188b54` | C×3 | 0x1005-0x1041 | 3/3 | 0x4FDE30 3/3 | 3/3 | `0x00502370` | GetDisabled 他2 |
| FAM-10 | 3 | 141 | `8b4c241ca1c433b3008b` | C×3 | 0x1085-0x1087 | 3/3 | 0x4FDE30 3/3 | 3/3 | `0x005027b0` | SameFactionAsPC 他2 |
| FAM-11 | 2 | 321 | `518b4c241c8b5424188d` | C×2 | 0x1036-0x1071 | 2/2 | 0x4FDE30 2/2 | 0/2 | `0x00502d30` | StartQuest 他1 |
| FAM-12 | 2 | 427 | `518b4c241c8b54241856` | C×2 | 0x10F1-0x10F3 | 2/2 | 0x4FDE30 2/2 | 0/2 | `0x00505ac0` | SetUnconscious 他1 |
| FAM-13 | 2 | 237 | `837c240c00d9ee568b74` | A×2 | 0x0015-0x001E | 2/2 | 0x4FDE30 2/2 | 0/2 | `0x00504560` | OnLoad 他1 |

**注記**

- `prologue16` は FAM-01 で 16 種類に細分される（`call` の rel32 下位バイト差のみ）。骨格正規化（アドレス→`@`）はこれを 1 つに束ねるため、**骨格一致の方が強い同型判定**である。逆に FAM-02 の 20 件は `prologue16` も 1 種類で一致する。
- 全 13 族・115/115 が `v10_impl_decompile_join.csv` の `impl` 列（= 表の `+0x18`）に一致。
- `+0x1C` は 115/115 が共有 dispatcher `0x4FDE30`（表全体では 528/532）。

### 1.1 代表的な骨格テンプレート（デコンパイル抜粋）

- **FAM-01** 例: `0x005019e0 GetFatiguePercentage` → `void F(void){ func_0x004f4640(param_3, 0, 0, in_stack_0000001c); return; }`
- **FAM-02** 例: `0x00504890 IsWeaponOut` → `if (param_3 != 0) { return func_0x004f50c0(param_3,0,0,in_stack_0000001c); } return 1;`
- **FAM-03** 例: `0x00505460 GetAmountSoldStolen` → `func_0x004f80a0(param_3,0,0,in_stack_0000001c); return 1;`
- **FAM-04** 例: `0x00504ce0 GetCrimeGold` → `if (param_3 == 0) { return 0; } return func_0x004f5230(param_3,0,0,in_stack_0000001c);`
- **FAM-05** 例: `0x005007c0 PrintNPCDialog` → `func_0x00676d90(); return 1;   // アクション直呼び（0x4F 帯を使わない）`
- **FAM-06** 例: `0x00503d90 OnSell` → `cVar1 = FUN_004fae80(...,&uStack_4); if (!cVar1) return 0; *param_7=0; if (param_3&&param_6) if (FUN_004fb5f0(uStack_4,0x4000)) *param_7=1.0; return 1;`
- **FAM-07** 例: `0x00501c00 GetCurrentTime` → `func_0x004f46e0(0, 0, 0, in_stack_0000001c); return;`
- **FAM-08** 例: `0x005056f0 AdvancePCLevel` → `if (DAT_00b333c4 != 0) { FUN_005ace20(); } return 1;`
- **FAM-09** 例: `0x00502370 GetDisabled` → `func_0x004f60e0(param_3, param_6, 0, param_7); return;`
- **FAM-10** 例: `0x005027b0 SameFactionAsPC` → `FUN_004f61a0(param_3, DAT_00b333c4, 0, in_stack_0000001c); return;`
- **FAM-11** 例: `0x00502d30 StartQuest` → `cVar1=FUN_004fae80(...,&iStack_4); if(!cVar1) return 0; if (iStack_4) FUN_00529820(1); return 1;`
- **FAM-12** 例: `0x00505ac0 SetUnconscious` → `cVar1=FUN_004fae80(...,&iStack_4); if(!cVar1) return 0; if (param_3) { iVar2=FUN_009832e6(...); if (iVar2) { if (iStack_4) { 0x5e8e30(1); FUN_005e02b0(); return 1; } 0x5e8e30(0); } } return 1;`
- **FAM-13** 例: `0x00504560 OnLoad` → `*param_7 = 0; if (param_3 && param_6) if (FUN_004fb5f0(0,0x1000)) { *param_7=1.0; return 1; } return 1;`

## §2 役割分類（helper 側）

### 2.1 役割コードの定義と判定根拠

| コード | 役割 | 判定根拠 | 判定 |
|---|---|---|---|
| R1 | 値取得（`+0x20` cond に直結） | メンバーの `+0x20` == 呼び出し先 helper。helper は `fst/fstp qword ptr [結果ポインタ]` で double を書き、`cmp byte [0xb361ac],0` でトレース出力（`call 0x579B60`）をゲート | **[確定]** |
| R2 | 値取得（`+0x20` がスタブ経由） | `+0x20` は別 VA だが、それは `mov [esp+0xc],0 / mov ecx,[0xb333c4] / mov [esp+8],ecx / jmp 実体` の尾ジャンプスタブ。実体は impl が呼ぶ helper と同一 | **[確定]**（3 件のみ） |
| R3 | 引数デコード | `FUN_004fae80`。デコンパイル本文（L194577-）で bytecode から u16 引数個数を読み、`+0x14` 記述子列に従って引数を展開。E8 xref = 278（表中最大級） | **[確定]** |
| R4 | スクリプト変数/フラグ照合 | `FUN_004fb5f0`。`(param_1+8)` の連結リストを走査して `*pi==id` を探しさらに `(pi[1] & mask)` を検査、未発見なら `FUN_004fb510`（未定義エラー）。E8 xref = 22 | **[確定]** |
| R5 | フォーム検索 / RTTI | `FUN_009832E6`。文字列 `"Access violation - no RTTI data!"` / `"Bad dynamic_cast!"` を保持。E8 xref = 1819（本調査の最大） | **[確定]** |
| R6 | アクション本体 | `store_f64` を持たず、かつ R3/R4/R5 でもない（= 値を返さず副作用を起こす関数）。FAM-05/08/11/12 の実装本体 | **[仮説]**（「アクション本体」までは確定、どのゲーム機構かは未判定 → §6） |

### 2.2 役割別の helper 件数（distinct 111 件）

| 役割 | distinct helper | 呼出し（member×helper 辺） |
|---|---|---|
| R1 値取得(+0x20 cond 直結) | 89 | 89 |
| R2 値取得(+0x20 cond スタブ経由) | 3 | 3 |
| R3 引数デコード(+0x14 記述子走査) | 1 | 10 |
| R4 スクリプト変数/フラグ照合 | 1 | 8 |
| R5 フォーム検索/RTTI | 1 | 2 |
| R6 アクション本体 | 16 | 17 |
| **合計** | **111** | **129** |

（`v97_data.json` 上の distinct helper = 111、メンバー→helper の辺 = 129。差 18 は R3/R4/R5 の共有 helper を複数メンバーが呼ぶ分。）

### 2.3 呼出しパターンの観察（役割を裏付ける構造）

- **R1/R2（値取得）の典型形**（FAM-01 代表 `0x004f4640`）

```
push esi / mov esi,[esp+8]        ; arg1 = 対象 ref（impl の param_3）
test esi,esi / je 末尾            ; NULL ガード
mov eax,[esi] / mov edx,[eax+0x190] / mov ecx,esi / call edx / test al,al  ; VT+0x190 有効性ガード
mov ecx,esi / call 0x5F4880       ; 値の実取得
mov eax,[esp+0x14] / fst qword ptr [eax]   ; arg4 = 結果 double* へ格納
cmp byte ptr [0xB361AC],0 / je 末尾 ; トレースゲート
fstp qword ptr [esp] / push 0xA487E0 / call 0x579B60  ; "GetFatiguePercentage >> %0.2f"
mov al,1 / pop esi / ret
```

- `VT+0x190` ガードを持つ helper = **68 / 111**。グローバル `0xB361AC`（トレースゲート）参照 = **89 / 111**、`0xB333C4`（コンテキスト/PC オブジェクト）参照 = **16 / 111**。
- **R6（アクション）の典型形**（FAM-05 `0x005007c0`）: 本文は `call <実体>; return 1;` の 1 呼出しのみ。0x4F 帯を使わない。
- **R3+R4/R5 併用形**（FAM-06/11/12/13）: `FUN_004FAE80`（引数デコード）で成否を確認してから、`FUN_004FB5F0`（フラグ）または実体（アクション）へ。

## §3 族別・メンバー別 helper 対応表（129 行）

列: 族 / メンバー VA / opid / オペコード名 / `+0x20` cond 有無 / helper VA / Ghidra ラベル（L=登録済, -=未登録）/ E8 xref 数 / 役割 / helper 内トレース文字列。

| 族 | member VA | opid | オペコード | cond | helper | lbl | xref | 役割 | トレース文字列 |
|---|---|---|---|---|---|---|---|---|---|
| FAM-01 | `0x005019e0` | 0x1080 | GetFatiguePercentage | 0x004f4640 | `0x004f4640` | - | 1 | R1 | GetFatiguePercentage >> %0.2f |
| FAM-01 | `0x00501e20` | 0x1018 | GetScale | 0x004f4720 | `0x004f4720` | - | 1 | R1 | GetScale >> %0.2f |
| FAM-01 | `0x00502630` | 0x1027 | GetDisease | 0x004f4800 | `0x004f4800` | - | 1 | R1 | UNIMPLEMENTED: GetDisease >> %0.2f |
| FAM-01 | `0x00502650` | 0x1028 | GetVampire | 0x004f4830 | `0x004f4830` | - | 1 | R1 | GetVampire >> %0.2f |
| FAM-01 | `0x00502670` | 0x1029 | GetClothingValue | 0x004f6150 | `0x004f6150` | - | 1 | R1 | GetClothingValue >> %0.2f |
| FAM-01 | `0x00502870` | 0x102E | GetDead | 0x004f4890 | `0x004f4890` | - | 1 | R1 | GetDead >> %0.2f |
| FAM-01 | `0x005029c0` | 0x1030 | GetGold | 0x004f4a00 | `0x004f4a00` | - | 1 | R1 | GetGold >> %0.2f |
| FAM-01 | `0x005029e0` | 0x1031 | GetSleeping | 0x004f4a80 | `0x004f4a80` | - | 1 | R1 | GetSleeping >> %0.2f |
| FAM-01 | `0x00502a00` | 0x109F | GetSitting | 0x004f64e0 | `0x004f64e0` | - | 1 | R1 | reO |
| FAM-01 | `0x00502a20` | 0x10A0 | GetFurnitureMarkerID | 0x004f65b0 | `0x004f65b0` | - | 1 | R1 | GetFurnitureMarkerID >> %0.2f |
| FAM-01 | `0x00502b00` | 0x1032 | GetTalkedToPC | 0x004f4b10 | `0x004f4b10` | L | 2 | R1 | GetTalkedToPC >> %0.2f |
| FAM-01 | `0x00502e90` | 0x103D | GetAlarmed | 0x004f4c10 | `0x004f4c10` | - | 1 | R1 | GetAlarmed >> %0.2f |
| FAM-01 | `0x00502eb0` | 0x110A | IsPleasant | 0x004f68f0 | `0x004f68f0` | - | 1 | R1 | GetIsPleasant >> %0.2f |
| FAM-01 | `0x00502ed0` | 0x110B | IsCloudy | 0x004f6960 | `0x004f6960` | - | 1 | R1 | GetIsCloudy >> %0.2f |
| FAM-01 | `0x00502ef0` | 0x103E | IsRaining | 0x004f69d0 | `0x004f69d0` | - | 1 | R1 | GetIsRaining >> %0.2f |
| FAM-01 | `0x00502f10` | 0x104B | IsSnowing | 0x004f6a20 | `0x004f6a20` | - | 1 | R1 | GetIsSnowing >> %0.2f |
| FAM-01 | `0x00502f30` | 0x1093 | GetWindSpeed | 0x004f7f50 | `0x004f7f50` | - | 1 | R1 | GetWindSpeed >> %0.2f |
| FAM-01 | `0x00502f50` | 0x1094 | GetCurrentWeatherPercent | 0x004f6a70 | `0x004f6a70` | - | 1 | R1 | GetCurrentWeatherPercent >> %0.2f |
| FAM-01 | `0x00502fd0` | 0x103F | GetAttacked | 0x004f4c80 | `0x004f4c80` | - | 1 | R1 | GetAttacked >> %0.2f |
| FAM-01 | `0x00502ff0` | 0x1040 | GetIsCreature | 0x004f6b00 | `0x004f6b00` | - | 1 | R1 | GetIsCreature >> %0.2f |
| FAM-01 | `0x005033f0` | 0x10FE | GetIsPlayableRace | 0x004f7030 | `0x004f7030` | - | 1 | R1 | GetIsPlayableRace >> %0.2f |
| FAM-01 | `0x00503710` | 0x1102 | GetUsedItemLevel | 0x004f4da0 | `0x004f4da0` | - | 1 | R1 | GetIsUsedItemLevel >> %0.2f |
| FAM-01 | `0x00503730` | 0x1103 | GetUsedItemActivate | 0x004f4de0 | `0x004f4de0` | - | 1 | R1 | GetIsUsedItemActivate >> %0.2f |
| FAM-01 | `0x005039f0` | 0x104D | GetRandomPercent | 0x004f4ef0 | `0x004f4ef0` | - | 1 | R1 | GetRandomPercent >> %0.2f |
| FAM-01 | `0x00503a10` | 0x1050 | GetLevel | 0x004f4f50 | `0x004f4f50` | - | 1 | R1 | GetLevel >> %0.2f |
| FAM-01 | `0x00503a30` | 0x1051 | GetArmorRating | 0x004f4fb0 | `0x004f4fb0` | - | 1 | R1 | Armor Rating: %0.2f |
| FAM-01 | `0x00503c40` | 0x105B | GetIsAlerted | 0x004f5060 | `0x004f5060` | - | 1 | R1 | GetIsAlerted: %0.2f |
| FAM-01 | `0x00505930` | 0x10E1 | GetPersuasionNumber | 0x004f57d0 | `0x004f57d0` | - | 1 | R1 |  |
| FAM-01 | `0x00505950` | 0x113B | GetTotalPersuasionNumber | 0x004f57f0 | `0x004f57f0` | - | 1 | R1 |  |
| FAM-01 | `0x00505970` | 0x10E3 | HasVampireFed | 0x004f5810 | `0x004f5810` | - | 1 | R1 | Has Vampire Fed >> %0.2f |
| FAM-01 | `0x005059f0` | 0x10E5 | GetClassDefaultMatch | 0x004f5880 | `0x004f5880` | - | 1 | R1 | GetIsMyClassDefault >> %0.2f |
| FAM-01 | `0x00505a10` | 0x10ED | GetIsGhost | 0x004f5900 | `0x004f5900` | - | 1 | R1 | GetGhost >> %0.2f |
| FAM-01 | `0x00505b50` | 0x10F2 | GetUnconscious | 0x004f7370 | `0x004f7370` | - | 1 | R1 | %s is not unconscious |
| FAM-01 | `0x00505c00` | 0x10F4 | GetRestrained | 0x004f73f0 | `0x004f73f0` | - | 1 | R1 | %s is not restrained |
| FAM-01 | `0x00505c20` | 0x10F9 | GetPCFame | 0x004f5960 | `0x004f5960` | - | 1 | R1 | Player Fame is %0.2f |
| FAM-01 | `0x00505c40` | 0x10FB | GetPCInfamy | 0x004f59b0 | `0x004f59b0` | - | 1 | R1 | Player Infamy is %0.2f |
| FAM-01 | `0x00505c60` | 0x10FF | GetOffersServicesNow | 0x004f5a00 | `0x004f5a00` | - | 1 | R1 | %s is not offering services |
| FAM-01 | `0x00505d10` | 0x1108 | GetBarterGold | 0x004f5a80 | `0x004f5a80` | - | 1 | R1 | %s  has %d barter gold currently |
| FAM-01 | `0x00505d30` | 0x1135 | IsXBox | 0x004f5d80 | `0x004f5d80` | - | 1 | R1 | IsXBox >> %1.0f |
| FAM-01 | `0x00505d50` | 0x1131 | GetInvestmentGold | 0x004f8420 | `0x004f8420` | - | 1 | R1 | %s  has %0.2f investment  gold currently |
| FAM-01 | `0x00505d70` | 0x1132 | IsActorUsingATorch | 0x004f8480 | `0x004f8480` | - | 1 | R1 | %s  wearing a torch  |
| FAM-01 | `0x00505d90` | 0x1109 | IsTimePassing | 0x004f5ae0 | `0x004f5ae0` | - | 1 | R1 | Time is not passing |
| FAM-01 | `0x00505db0` | 0x1112 | GetArmorRatingUpperBody | 0x004f84f0 | `0x004f84f0` | L | 1 | R1 | Armor Rating upper body is %0.2f |
| FAM-01 | `0x00506370` | 0x1139 | IsActorEvil | 0x004f5b30 | `0x004f5b30` | - | 1 | R1 | %s  is not evil  |
| FAM-01 | `0x00506390` | 0x113A | IsActorAVictim | 0x004f5bb0 | `0x004f5bb0` | - | 1 | R1 | %s  is not a crime victim  |
| FAM-01 | `0x005063b0` | 0x113E | GetIdleDoneOnce | 0x004f85e0 | `0x004f85e0` | - | 1 | R1 |  |
| FAM-01 | `0x00506dc0` | 0x116D | GetPlayerInSEWorld | 0x004f87f0 | `0x004f87f0` | - | 1 | R1 | The player %s in the SE world |
| FAM-02 | `0x00504890` | 0x1065 | IsWeaponOut | 0x004f50c0 | `0x004f50c0` | - | 1 | R1 | Is Weapon Out >> %0.2f |
| FAM-02 | `0x005048b0` | 0x1066 | IsTorchOut | 0x004f75f0 | `0x004f75f0` | - | 1 | R1 | YxO |
| FAM-02 | `0x005048d0` | 0x1067 | IsShieldOut | 0x004f7650 | `0x004f7650` | - | 1 | R1 | YxO |
| FAM-02 | `0x005048f0` | 0x1068 | IsYielding | 0x004f5120 | `0x004f5120` | - | 1 | R1 | Is Yielding >> %0.2f |
| FAM-02 | `0x00504910` | 0x106A | IsFacingUp | 0x004f8970 | `0x004f8970` | - | 1 | R1 | Bip01 Spine |
| FAM-02 | `0x00504930` | 0x111D | IsLeftUp | 0x004f89e0 | `0x004f89e0` | L | 1 | R1 | Bip01 Spine |
| FAM-02 | `0x00504950` | 0x106B | GetKnockedState | 0x004f76b0 | `0x004f76b0` | - | 1 | R1 | YxO |
| FAM-02 | `0x00504970` | 0x106C | GetWeaponAnimType | 0x004f7750 | `0x004f7750` | - | 1 | R1 | YxO |
| FAM-02 | `0x00504990` | 0x106D | GetWeaponSkillType | 0x004f7800 | `0x004f7800` | - | 1 | R1 | YxO |
| FAM-02 | `0x005049b0` | 0x106E | GetCurrentAIPackage | 0x004f78b0 | `0x004f78b0` | - | 1 | R1 | Current Process >> %0.2f |
| FAM-02 | `0x005049d0` | 0x106F | IsWaiting | 0x004f5180 | `0x004f5180` | - | 1 | R1 | Is Waiting >> %0.2f |
| FAM-02 | `0x005049f0` | 0x1070 | IsIdlePlaying | 0x004f51e0 | `0x004f51e0` | - | 1 | R1 | Is Idle Playing >> %0.2f |
| FAM-02 | `0x00504e90` | 0x108F | GetCurrentAIProcedure | 0x004f7c00 | `0x004f7c00` | - | 1 | R1 | x\|O |
| FAM-02 | `0x00504eb0` | 0x1090 | GetTrespassWarningLevel | 0x004f7ef0 | `0x004f7ef0` | - | 1 | R1 | Procedure >> %0.2f |
| FAM-02 | `0x00504ed0` | 0x1091 | IsTrespassing | 0x004f5370 | `0x004f5370` | - | 1 | R1 | Is Trespassing Value %0.2f |
| FAM-02 | `0x00504ef0` | 0x1092 | IsInMyOwnedCell | 0x004f53d0 | `0x004f53d0` | - | 1 | R1 | Is in owned cell value %0.2f |
| FAM-02 | `0x00504f10` | 0x1096 | IsContinuingPackagePCNear | 0x004f5440 | `0x004f5440` | - | 1 | R1 | is continuing package near PC value %0.2f |
| FAM-02 | `0x00504f30` | 0x1099 | CanHaveFlames | 0x004f8a50 | `0x004f8a50` | - | 1 | R1 | CanHaveFlames >> %0.2f |
| FAM-02 | `0x00504f50` | 0x109A | HasFlames | 0x004f54a0 | `0x004f54a0` | L | 1 | R1 | HasFlames >> %0.2f |
| FAM-02 | `0x00505090` | 0x109D | GetOpenState | 0x004f5520 | `0x004f5520` | - | 1 | R1 | GetOpenState >> %0.2f |
| FAM-03 | `0x00505460` | 0x10BE | GetAmountSoldStolen | 0x004f80a0 | `0x004f80a0` | - | 1 | R1 | Amount stolen the player sold >> %0.2f |
| FAM-03 | `0x00505660` | 0x10CB | GetDestroyed | 0x004f82c0 | `0x004f82c0` | - | 1 | R1 | GetDestroyed >> %.0f |
| FAM-03 | `0x00505ee0` | 0x111A | IsHorseStolen | 0x004f8c20 | `0x004f8c20` | - | 1 | R1 | %s is not stolen |
| FAM-03 | `0x00505f00` | 0x116A | GetPlayerHasLastRiddenHorse | 0x004f8630 | `0x004f8630` | - | 1 | R1 | there is not a last ridden horse |
| FAM-03 | `0x00505f20` | 0x111E | IsSneaking | 0x004f8cf0 | `0x004f8cf0` | - | 1 | R1 | %s is not sneaking |
| FAM-03 | `0x00505f40` | 0x111F | IsRunning | 0x004f8db0 | `0x004f8db0` | - | 1 | R1 | %s is not running |
| FAM-03 | `0x00505fc0` | 0x1121 | IsInCombat | 0x004f8f30 | `0x004f8f30` | - | 1 | R1 | %s is not in combat |
| FAM-03 | `0x005062f0` | 0x112C | IsInInterior | 0x004f9010 | `0x004f9010` | - | 1 | R1 | %s is not an Interior |
| FAM-04 | `0x00504ce0` | 0x1074 | GetCrimeGold | 0x004f5230 | `0x004f5230` | - | 1 | R1 | Actor Crime Gold is %.02f  |
| FAM-04 | `0x00504e10` | 0x107D | IsGuard | 0x004f7b90 | `0x004f7b90` | - | 1 | R1 | x\|O |
| FAM-04 | `0x00504e30` | 0x107F | CanPayCrimeGold | 0x004f5290 | `0x004f5290` | - | 1 | R1 | Can Pay Crime Gold >> %0.2f |
| FAM-04 | `0x00504e50` | 0x108D | IsTalking | 0x004f5310 | `0x004f5310` | - | 1 | R1 | Talking >> %0.2f |
| FAM-04 | `0x00504e70` | 0x108E | GetWalkSpeed | 0x004f4690 | `0x004f4690` | - | 1 | R1 | GetWalkSpeed >> %0.2f |
| FAM-04 | `0x005053b0` | 0x10B9 | IsSwimming | 0x004f56d0 | `0x004f56d0` | - | 1 | R1 | Is Swimming >> %0.2f |
| FAM-05 | `0x005007c0` | 0x0150 | PrintNPCDialog | - | `0x00676d90` | - | 1 | R6 | %s %s% |
| FAM-05 | `0x00500e30` | 0x0107 | ToggleCollisionGeometry | - | `0x00440510` | - | 2 | R6 |  |
| FAM-05 | `0x005013e0` | 0x016E | ToggleFlyCam | - | `0x006669f0` | L | 1 | R6 |  |
| FAM-05 | `0x00501650` | 0x0115 | TogglePathGrid | - | `0x004e7d10` | - | 1 | R6 |  |
| FAM-05 | `0x00501660` | 0x0116 | TogglePathLine | - | `0x00683be0` | - | 1 | R6 |  |
| FAM-05 | `0x005064a0` | 0x1145 | RefreshTopicList | - | `0x0059f970` | - | 1 | R6 |  |
| FAM-06 | `0x00503d90` | 0x0017 | OnSell | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-06 | `0x00503d90` | 0x0017 | OnSell | - | `0x004fb5f0` | L | 22 | R4 |  |
| FAM-06 | `0x005041d0` | 0x0019 | OnStartCombat | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-06 | `0x005041d0` | 0x0019 | OnStartCombat | - | `0x004fb5f0` | L | 22 | R4 |  |
| FAM-06 | `0x005045e0` | 0x0016 | OnMagicEffectHit | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-06 | `0x005045e0` | 0x0016 | OnMagicEffectHit | - | `0x004fb5f0` | L | 22 | R4 |  |
| FAM-06 | `0x00504660` | 0x0018 | OnTrigger | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-06 | `0x00504660` | 0x0018 | OnTrigger | - | `0x004fb5f0` | L | 22 | R4 |  |
| FAM-06 | `0x005046e0` | 0x001A | OnTriggerActor | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-06 | `0x005046e0` | 0x001A | OnTriggerActor | - | `0x004fb5f0` | L | 22 | R4 |  |
| FAM-06 | `0x00504760` | 0x001B | OnTriggerMob | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-06 | `0x00504760` | 0x001B | OnTriggerMob | - | `0x004fb5f0` | L | 22 | R4 |  |
| FAM-07 | `0x00501c00` | 0x1012 | GetCurrentTime | 0x004f46e0 | `0x004f46e0` | - | 1 | R1 | GetCurrentTime >> %0.2f |
| FAM-07 | `0x005051d0` | 0x10AA | GetDayOfWeek | 0x004f5570 | `0x004f5570` | - | 1 | R1 | GetDayOfWeek >> %0.2f |
| FAM-07 | `0x005051f0` | 0x10AB | IsPlayerInJail | 0x004f55c0 | `0x004f55c0` | - | 1 | R1 | IsPlayerInJail >> %0.2f |
| FAM-07 | `0x00505210` | 0x10AF | IsPCSleeping | 0x004f8050 | `0x004f8050` | - | 1 | R1 | IsPlayerSleeping >> %0.2f |
| FAM-07 | `0x00505230` | 0x10B0 | IsPCAMurderer | 0x004f5600 | `0x004f5600` | - | 1 | R1 | IsPlayerAMurderer >> %0.2f |
| FAM-08 | `0x005056f0` | 0x10D5 | AdvancePCLevel | - | `0x005ace20` | L | 3 | R6 | %s %i |
| FAM-08 | `0x00505850` | 0x10DA | ShowRaceMenu | - | `0x0057b7e0` | L | 2 | R6 |  |
| FAM-08 | `0x00505870` | 0x10DB | ShowBirthSignMenu | - | `0x0057a9b0` | - | 1 | R6 |  |
| FAM-08 | `0x00505890` | 0x10EA | ShowSpellMaking | - | `0x005d86c0` | - | 1 | R6 | Data\Menus\dialog\Spellmaking.xml |
| FAM-08 | `0x005058b0` | 0x10EB | ShowEnchantment | - | `0x005a1ec0` | L | 1 | R6 | Enchantment Menu Creation Failed... Are your menu and art resources up to date? |
| FAM-09 | `0x00502370` | 0x1023 | GetDisabled | 0x004f60e0 | `0x004f60e0` | - | 1 | R1 | GetDisabled >> %0.f |
| FAM-09 | `0x00502390` | 0x1005 | GetLocked | 0x004f4760 | `0x004f4760` | - | 1 | R1 | GetLocked >> %0.f |
| FAM-09 | `0x005023b0` | 0x1041 | GetLockLevel | 0x004f47b0 | `0x004f47b0` | - | 1 | R1 | GetLockLevel >> %0.f |
| FAM-10 | `0x005027b0` | 0x1085 | SameFactionAsPC | 0x004f8910 | `0x004f61a0` | L | 2 | R2 | SameFaction >> %0.2f |
| FAM-10 | `0x005027d0` | 0x1086 | SameRaceAsPC | 0x004f8930 | `0x004f62a0` | L | 2 | R2 | SameRace >> %0.2f |
| FAM-10 | `0x005027f0` | 0x1087 | SameSexAsPC | 0x004f8950 | `0x004f6350` | L | 2 | R2 | SameSex >> %0.2f |
| FAM-11 | `0x00502d30` | 0x1036 | StartQuest | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-11 | `0x00502d30` | 0x1036 | StartQuest | - | `0x00529820` | L | 4 | R6 |  |
| FAM-11 | `0x00502dd0` | 0x1071 | CompleteQuest | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-11 | `0x00502dd0` | 0x1071 | CompleteQuest | - | `0x00529840` | L | 2 | R6 |  |
| FAM-12 | `0x00505ac0` | 0x10F1 | SetUnconscious | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-12 | `0x00505ac0` | 0x10F1 | SetUnconscious | - | `0x005e02b0` | L | 11 | R6 |  |
| FAM-12 | `0x00505ac0` | 0x10F1 | SetUnconscious | - | `0x005e8e30` | - | 2 | R6 |  |
| FAM-12 | `0x00505ac0` | 0x10F1 | SetUnconscious | - | `0x009832e6` | L | 1819 | R5 | Access violation - no RTTI data! |
| FAM-12 | `0x00505b70` | 0x10F3 | SetRestrained | - | `0x004fae80` | L | 278 | R3 |  |
| FAM-12 | `0x00505b70` | 0x10F3 | SetRestrained | - | `0x005e02b0` | L | 11 | R6 |  |
| FAM-12 | `0x00505b70` | 0x10F3 | SetRestrained | - | `0x005e8e60` | - | 2 | R6 |  |
| FAM-12 | `0x00505b70` | 0x10F3 | SetRestrained | - | `0x009832e6` | L | 1819 | R5 | Access violation - no RTTI data! |
| FAM-13 | `0x00504560` | 0x0015 | OnLoad | - | `0x004fb5f0` | L | 22 | R4 |  |
| FAM-13 | `0x005045a0` | 0x001E | OnReset | - | `0x004fb5f0` | L | 22 | R4 |  |

### 3.1 トレース文字列とオペコード名の突合

トレース文字列を持つ helper に対し、`オペコード名（小文字）が文字列に部分一致するか`で突合した結果:

- 完全部分一致: **35**、表記ゆれ（`GetIs…`/`Armor Rating:`/`%s is not …` 等の言い換え）: **60**、文字列なし: **34**
- **意味論的な不一致は 0 件**（例: `GetIsGhost` → `GetGhost >> %0.2f`、`GetRestrained` → `%s is not restrained`）。
- 表記ゆれの主な例: `GetUsedItemLevel`→`GetIsUsedItemLevel >> %0.2f`、`GetArmorRating`→`Armor Rating: %0.2f`、`IsWeaponOut`→`Is Weapon Out >> %0.2f`、`GetDisease`→`UNIMPLEMENTED: GetDisease >> %0.2f`。
- 文字列なし（役割は R1 だがトレースを持たない）: `0x004F57D0`(GetPersuasionNumber) / `0x004F57F0`(GetTotalPersuasionNumber) / `0x004F85E0`(GetIdleDoneOnce)、および R3/R4/R5/R6 の全 helper。

## §4 V69/V70 ハンドラ表との接続対応表

### 4.1 3 つの列と 3 つの消費経路（本調査の中心結論）

| 表の列 | 値 | 消費者（実際に読む関数） | 呼出し形式 | 本調査の族との対応 |
|---|---|---|---|---|
| `+0x18` 実装ハンドラ（527/532 非ゼロ） | フォワーダ本体 | **`FUN_004FAB20`**（bytecode `'X'` トークン経由） | `u16 opid` → `FUN_004FCA30` で entry 解決 → `mov eax,[eax+0x18]` (0x4FADBA) → `call eax` (0x4FADD5)、**8 引数** | 帯域 B/C の 107 件 |
| 同上 | 同上 | **`FUN_005169A0`**（`param_3==0x10` 経路） | `u16 opid`（`>0x1E` は拒否）→ `(&PTR_LAB_00b0af60)[uVar6*10]` = entry+0x18 → `call`、**8 引数** | 帯域 A の 8 件（FAM-06 / FAM-13） |
| `+0x1C` 共有 dispatcher（`0x4FDE30` が 528/532） | 引数コンパイラ | **`FUN_004FF710`**（ScriptCompiler） | `mov ebx,[eax+0x1c]` (0x4FF963) → `call ebx` (0x4FF9B9)、4 引数 `(argn, argdesc, &buf, ctx)` | **115/115 全員** |
| `+0x20` 条件関数（169/532 非ゼロ） | 値取得 helper | **`FUN_0056AC50` / `FUN_0056AE20`**（CTDA 条件評価） | `(**(code**)(&DAT_00b0C8E0 + opid*0x28))(ref, ref2, param+0x10, &double)` → `FUN_0056A3B0` で比較 | 115 中 **92 件** |

**要点**: `+0x18` と `+0x1C` は排他ではなく**同時成立**する別列である。115 件はすべて「`+0x18` に自身が載る」「`+0x1C` が 0x4FDE30」「うち 92 件は `+0x20` が自分の helper を指す」という 3 条件を同時に満たす。

### 4.2 帯域別・実行経路の対応表

| 族 | 件数 | opid 帯 | `+0x18` 経由の実行者 | `+0x1C` | `+0x20` 非ゼロ | helper 役割 |
|---|---|---|---|---|---|---|
| FAM-01 | 47 | C(47) | FUN_004FAB20 | 0x4FDE30 ×47 | 47/47 | R1×47 |
| FAM-02 | 20 | C(20) | FUN_004FAB20 | 0x4FDE30 ×20 | 20/20 | R1×20 |
| FAM-03 | 8 | C(8) | FUN_004FAB20 | 0x4FDE30 ×8 | 8/8 | R1×8 |
| FAM-04 | 6 | C(6) | FUN_004FAB20 | 0x4FDE30 ×6 | 6/6 | R1×6 |
| FAM-05 | 6 | B(5) + C(1) | FUN_004FAB20 | 0x4FDE30 ×6 | 0/6 | R6×6 |
| FAM-06 | 6 | A(6) | FUN_005169A0 | 0x4FDE30 ×6 | 0/6 | R3×6 + R4×6 |
| FAM-07 | 5 | C(5) | FUN_004FAB20 | 0x4FDE30 ×5 | 5/5 | R1×5 |
| FAM-08 | 5 | C(5) | FUN_004FAB20 | 0x4FDE30 ×5 | 0/5 | R6×5 |
| FAM-09 | 3 | C(3) | FUN_004FAB20 | 0x4FDE30 ×3 | 3/3 | R1×3 |
| FAM-10 | 3 | C(3) | FUN_004FAB20 | 0x4FDE30 ×3 | 3/3 | R2×3 |
| FAM-11 | 2 | C(2) | FUN_004FAB20 | 0x4FDE30 ×2 | 0/2 | R3×2 + R6×2 |
| FAM-12 | 2 | C(2) | FUN_004FAB20 | 0x4FDE30 ×2 | 0/2 | R3×2 + R5×2 + R6×4 |
| FAM-13 | 2 | A(2) | FUN_005169A0 | 0x4FDE30 ×2 | 0/2 | R4×2 |

### 4.3 `+0x18` impl バンド（527 件）との対応

- 族メンバー 115/115 が `+0x18` 列に出現（テーブル 527 impl 中の **21.8%**）。
- 逆に、族メンバーが `+0x1C` / `+0x20` 列に *impl として* 出现することは **0 件**（= 族は impl 専用）。
- helper 111 件のうち `+0x18` 列に出現するのは **0 件**（helper は impl 列には載らない）。
- **E8 直接呼出しは 115 件すべて 0 件**。impl は必ず間接呼び出し（テーブル経由）である。これは「実行経路が表参照のみ」であることの裏付けでもある。
- `.text` 内のテーブル基底即値 `0x00B0AF48` の出現は **1 箇所（0x4FF906, `FUN_004FF710` 内）** のみで V77 の記載と一致。すなわち *基底 + インデックス* 形式で表を読むのはコンパイラだけで、実行側は `FUN_004FCA30`（バンド別基址 `0xB0B420` / `0xB0C8C0` + `lea` スケール）と帯域 A のインライン基址 `0xB0AF60` を使う。

### 4.4 `+0x20` 条件関数との対応（新規確定）

| 区分 | 件数 | 内容 |
|---|---|---|
| `+0x20` == helper と完全一致 | **89** | impl が直接呼ぶ helper がそのまま `+0x20` |
| `+0x20` ≠ helper（尾ジャンプスタブ） | **3** | FAM-10 の 3 件。`0x004F8910/8930/8950` が `mov [esp+0xc],0; mov ecx,[0xB333C4]; mov [esp+8],ecx; jmp 実体` で `0x004F61A0/62A0/6350` へ到達。実体は一致 |
| `+0x20` == 0 | **23** | FAM-05/06/08/11/12/13（アクション・イベント系。値を返さないため条件評価対象外） |
| 合計 | 115 | |

→ つまり **「値を返すオペコード」は `+0x18` と `+0x20` に同一の helper を置く**構造であり、
オペコード実行（`FUN_004FAB20` / `FUN_005169A0` 経由）と CTDA 条件評価（`FUN_0056AC50` 経由）が
**同じ helper を共有する**。これが V76 の「`+0x20` 条件関数 169 件」と V79 の「フォワーダ族」を繋ぐ関係である。

### 4.5 helper 側で見る実行フロー（オペコード fetch / 引数 / スタック）

| 段階 | 動作 | 場所 | 判定 |
|---|---|---|---|
| ① fetch/decode | bytecode トークン 1B と `u16 opid` を読み、バンド判定 | `FUN_004FAB20` L194519-194523（`'X'`）／`FUN_005169A0` L207947-207953 | [確定] |
| ② entry 解決 | `u16 opid` → entry ポインタ | `FUN_004FCA30`（`0x100-0x182`→`0xB0B420+40*（op-0x100)`、`0x1000-0x1170`→`0xB0C8C0+40*（op-0x1000)`、それ以外→0） | [確定] |
| ③ 引数コンパイル | `+0x14` 記述子列から引数を展開 | `FUN_004FAE80`（`+0x12` argn / `+0x14` argdesc を消費） | [確定] |
| ④ impl 呼出し | entry+0x18 を **8 引数**で indirect call（3 番目 = 対象 ref、7 番目 = 結果 `double*`） | `FUN_004FAB20` L194544-194546 / `FUN_005169A0` L207961-207963 | [確定] |
| ⑤ VM スタック/結果格納 | フォワーダは impl a7（`[esp+0x1c]`）を helper a4 へ渡し、helper が `fst qword ptr [a4]` で double を書く | フォワーダ本文 + helper 本文 | [確定] |
| ⑥ dispatcher 接続 | コンパイル時に `+0x1C` = `0x4FDE30` を `call` | `FUN_004FF710` L198218-198229 | [確定] |
| ⑦ コンテキスト取得 | helper が `0xB333C4`（コンテキスト/PC オブジェクト）や VT+0x190 ガードを参照 | 本文 §2.3 参照 | [確定] |

## §5 helper 汎用一覧（distinct 111 件）

| helper | Ghidra | E8 xref | 役割 | 族 | オペコード例 | 主な特徴 |
|---|---|---|---|---|---|---|
| `0x00440510` | **-** | 2 | R6 | FAM-05 | ToggleCollisionGeometry | - |
| `0x004e7d10` | **-** | 1 | R6 | FAM-05 | TogglePathGrid | - |
| `0x004f4640` | **-** | 1 | R1 | FAM-01 | GetFatiguePercentage | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4690` | **-** | 1 | R1 | FAM-04 | GetWalkSpeed | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f46e0` | **-** | 1 | R1 | FAM-07 | GetCurrentTime | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4720` | **-** | 1 | R1 | FAM-01 | GetScale | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4760` | **-** | 1 | R1 | FAM-09 | GetLocked | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f47b0` | **-** | 1 | R1 | FAM-09 | GetLockLevel | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4800` | **-** | 1 | R1 | FAM-01 | GetDisease | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4830` | **-** | 1 | R1 | FAM-01 | GetVampire | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4890` | **-** | 1 | R1 | FAM-01 | GetDead | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4a00` | **-** | 1 | R1 | FAM-01 | GetGold | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4a80` | **-** | 1 | R1 | FAM-01 | GetSleeping | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4b10` | L | 2 | R1 | FAM-01 | GetTalkedToPC | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4c10` | **-** | 1 | R1 | FAM-01 | GetAlarmed | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4c80` | **-** | 1 | R1 | FAM-01 | GetAttacked | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4da0` | **-** | 1 | R1 | FAM-01 | GetUsedItemLevel | 結果f64格納, 0xB361AC トレース |
| `0x004f4de0` | **-** | 1 | R1 | FAM-01 | GetUsedItemActivate | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f4ef0` | **-** | 1 | R1 | FAM-01 | GetRandomPercent | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4f50` | **-** | 1 | R1 | FAM-01 | GetLevel | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f4fb0` | **-** | 1 | R1 | FAM-01 | GetArmorRating | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5060` | **-** | 1 | R1 | FAM-01 | GetIsAlerted | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f50c0` | **-** | 1 | R1 | FAM-02 | IsWeaponOut | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5120` | **-** | 1 | R1 | FAM-02 | IsYielding | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5180` | **-** | 1 | R1 | FAM-02 | IsWaiting | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f51e0` | **-** | 1 | R1 | FAM-02 | IsIdlePlaying | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5230` | **-** | 1 | R1 | FAM-04 | GetCrimeGold | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5290` | **-** | 1 | R1 | FAM-04 | CanPayCrimeGold | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5310` | **-** | 1 | R1 | FAM-04 | IsTalking | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5370` | **-** | 1 | R1 | FAM-02 | IsTrespassing | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f53d0` | **-** | 1 | R1 | FAM-02 | IsInMyOwnedCell | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5440` | **-** | 1 | R1 | FAM-02 | IsContinuingPackagePCNear | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f54a0` | L | 1 | R1 | FAM-02 | HasFlames | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f5520` | **-** | 1 | R1 | FAM-02 | GetOpenState | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f5570` | **-** | 1 | R1 | FAM-07 | GetDayOfWeek | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f55c0` | **-** | 1 | R1 | FAM-07 | IsPlayerInJail | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照 |
| `0x004f5600` | **-** | 1 | R1 | FAM-07 | IsPCAMurderer | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照 |
| `0x004f56d0` | **-** | 1 | R1 | FAM-04 | IsSwimming | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f57d0` | **-** | 1 | R1 | FAM-01 | GetPersuasionNumber | 結果f64格納, fldz既定0, VT+0x190 ガード |
| `0x004f57f0` | **-** | 1 | R1 | FAM-01 | GetTotalPersuasionNumber | 結果f64格納, fldz既定0, VT+0x190 ガード |
| `0x004f5810` | **-** | 1 | R1 | FAM-01 | HasVampireFed | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5880` | **-** | 1 | R1 | FAM-01 | GetClassDefaultMatch | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照, VT+0x190 ガード |
| `0x004f5900` | **-** | 1 | R1 | FAM-01 | GetIsGhost | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5960` | **-** | 1 | R1 | FAM-01 | GetPCFame | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照, VT+0x190 ガード |
| `0x004f59b0` | **-** | 1 | R1 | FAM-01 | GetPCInfamy | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照, VT+0x190 ガード |
| `0x004f5a00` | **-** | 1 | R1 | FAM-01 | GetOffersServicesNow | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5a80` | **-** | 1 | R1 | FAM-01 | GetBarterGold | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5ae0` | **-** | 1 | R1 | FAM-01 | IsTimePassing | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照, VT+0x190 ガード |
| `0x004f5b30` | **-** | 1 | R1 | FAM-01 | IsActorEvil | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5bb0` | **-** | 1 | R1 | FAM-01 | IsActorAVictim | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f5d80` | **-** | 1 | R1 | FAM-01 | IsXBox | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f60e0` | **-** | 1 | R1 | FAM-09 | GetDisabled | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f6150` | **-** | 1 | R1 | FAM-01 | GetClothingValue | 結果f64格納, 0xB361AC トレース |
| `0x004f61a0` | L | 2 | R2 | FAM-10 | SameFactionAsPC | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照, VT+0x190 ガード |
| `0x004f62a0` | L | 2 | R2 | FAM-10 | SameRaceAsPC | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f6350` | L | 2 | R2 | FAM-10 | SameSexAsPC | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f64e0` | **-** | 1 | R1 | FAM-01 | GetSitting | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f65b0` | **-** | 1 | R1 | FAM-01 | GetFurnitureMarkerID | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f68f0` | **-** | 1 | R1 | FAM-01 | IsPleasant | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f6960` | **-** | 1 | R1 | FAM-01 | IsCloudy | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f69d0` | **-** | 1 | R1 | FAM-01 | IsRaining | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f6a20` | **-** | 1 | R1 | FAM-01 | IsSnowing | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f6a70` | **-** | 1 | R1 | FAM-01 | GetCurrentWeatherPercent | 結果f64格納, 0xB361AC トレース |
| `0x004f6b00` | **-** | 1 | R1 | FAM-01 | GetIsCreature | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f7030` | **-** | 1 | R1 | FAM-01 | GetIsPlayableRace | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f7370` | **-** | 1 | R1 | FAM-01 | GetUnconscious | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f73f0` | **-** | 1 | R1 | FAM-01 | GetRestrained | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f75f0` | **-** | 1 | R1 | FAM-02 | IsTorchOut | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f7650` | **-** | 1 | R1 | FAM-02 | IsShieldOut | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f76b0` | **-** | 1 | R1 | FAM-02 | GetKnockedState | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f7750` | **-** | 1 | R1 | FAM-02 | GetWeaponAnimType | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f7800` | **-** | 1 | R1 | FAM-02 | GetWeaponSkillType | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f78b0` | **-** | 1 | R1 | FAM-02 | GetCurrentAIPackage | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f7b90` | **-** | 1 | R1 | FAM-04 | IsGuard | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f7c00` | **-** | 1 | R1 | FAM-02 | GetCurrentAIProcedure | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f7ef0` | **-** | 1 | R1 | FAM-02 | GetTrespassWarningLevel | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f7f50` | **-** | 1 | R1 | FAM-01 | GetWindSpeed | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f8050` | **-** | 1 | R1 | FAM-07 | IsPCSleeping | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照 |
| `0x004f80a0` | **-** | 1 | R1 | FAM-03 | GetAmountSoldStolen | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照 |
| `0x004f82c0` | **-** | 1 | R1 | FAM-03 | GetDestroyed | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f8420` | **-** | 1 | R1 | FAM-01 | GetInvestmentGold | 結果f64格納, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f8480` | **-** | 1 | R1 | FAM-01 | IsActorUsingATorch | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f84f0` | L | 1 | R1 | FAM-01 | GetArmorRatingUpperBody | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f85e0` | **-** | 1 | R1 | FAM-01 | GetIdleDoneOnce | 結果f64格納, fldz既定0, VT+0x190 ガード |
| `0x004f8630` | **-** | 1 | R1 | FAM-03 | GetPlayerHasLastRiddenHorse | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照, VT+0x190 ガード |
| `0x004f87f0` | **-** | 1 | R1 | FAM-01 | GetPlayerInSEWorld | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照 |
| `0x004f8970` | **-** | 1 | R1 | FAM-02 | IsFacingUp | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f89e0` | L | 1 | R1 | FAM-02 | IsLeftUp | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f8a50` | **-** | 1 | R1 | FAM-02 | CanHaveFlames | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004f8c20` | **-** | 1 | R1 | FAM-03 | IsHorseStolen | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f8cf0` | **-** | 1 | R1 | FAM-03 | IsSneaking | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f8db0` | **-** | 1 | R1 | FAM-03 | IsRunning | 結果f64格納, fldz既定0, 0xB361AC トレース, VT+0x190 ガード |
| `0x004f8f30` | **-** | 1 | R1 | FAM-03 | IsInCombat | 結果f64格納, fldz既定0, 0xB361AC トレース, 0xB333C4 参照, VT+0x190 ガード |
| `0x004f9010` | **-** | 1 | R1 | FAM-03 | IsInInterior | 結果f64格納, fldz既定0, 0xB361AC トレース |
| `0x004fae80` | L | 278 | R3 | FAM-06,FAM-11,FAM-12 | OnSell 他 | 結果f64格納, fldz既定0 |
| `0x004fb5f0` | L | 22 | R4 | FAM-06,FAM-13 | OnSell 他 | - |
| `0x00529820` | L | 4 | R6 | FAM-11 | StartQuest | - |
| `0x00529840` | L | 2 | R6 | FAM-11 | CompleteQuest | - |
| `0x0057a9b0` | **-** | 1 | R6 | FAM-08 | ShowBirthSignMenu | - |
| `0x0057b7e0` | L | 2 | R6 | FAM-08 | ShowRaceMenu | fldz既定0, 0xB333C4 参照 |
| `0x0059f970` | **-** | 1 | R6 | FAM-05 | RefreshTopicList | - |
| `0x005a1ec0` | L | 1 | R6 | FAM-08 | ShowEnchantment | 0xB333C4 参照 |
| `0x005ace20` | L | 3 | R6 | FAM-08 | AdvancePCLevel | 0xB333C4 参照 |
| `0x005d86c0` | **-** | 1 | R6 | FAM-08 | ShowSpellMaking | 0xB333C4 参照 |
| `0x005e02b0` | L | 11 | R6 | FAM-12 | SetUnconscious 他 | - |
| `0x005e8e30` | **-** | 2 | R6 | FAM-12 | SetUnconscious | - |
| `0x005e8e60` | **-** | 2 | R6 | FAM-12 | SetRestrained | - |
| `0x006669f0` | L | 1 | R6 | FAM-05 | ToggleFlyCam | - |
| `0x00676d90` | **-** | 1 | R6 | FAM-05 | PrintNPCDialog | 0xB361AC トレース, VT+0x190 ガード |
| `0x00683be0` | **-** | 1 | R6 | FAM-05 | TogglePathLine | - |
| `0x009832e6` | L | 1819 | R5 | FAM-12 | SetUnconscious 他 | - |

### 5.1 Ghidra 未登録 helper の独立性

- distinct helper 111 件のうち **Ghidra 台帳（21,597 件）に未登録 = 94 件**、登録済 = 17 件。
- 未登録 94 件の直前 1 バイト: `0xCC` ×85 / `0xC3` ×8 / `0x00` ×1。**独立した関数エントリであることが 93/94 で機械的に裏付けられる**（`0xCC` パディング直後または直前関数の `ret` 直後）。
- 例外 1 件: `0x004F65B0`（FAM-01 / GetFurnitureMarkerID `0x00502a20` の helper）は直前が `0x00`。
- **V96 とのクロスチェック**: V96 の「パディングラン内 = やや弱」6 件（`0x004672D0` / `0x004F65B0` / `0x004F7750` / `0x004F7800` / `0x004F78B0` / `0x004F7EF0`）のうち **5 件が本調査の helper と一致**（`0x004F65B0`=FAM-01、残り 4 件=FAM-02）。V96 が「宿主の外から呼ばれる未ラベル関数」として拾った候補のほとんどは、実は本調査のフォワーダの呼び先だったことになる。
- これらは `v79_classified.csv` の gap357 には **1 件も含まれない**（= V70 が force-create した対象外）。V85 系の境界候補にも上がらなかった、**別系統の Ghidra 欠落**である。

## §6 残点（役割未判定・未検証）

1. **R6 アクション本体 16 件の「中身」は未判定 [仮説]**: `0x00676D90`(PrintNPCDialog) / `0x00440510`(ToggleCollisionGeometry) / `0x006669F0`(ToggleFlyCam) / `0x004E7D10`(TogglePathGrid) / `0x00683BE0`(TogglePathLine) / `0x0059F970`(RefreshTopicList) / `0x005ACE20`(AdvancePCLevel) / `0x0057B7E0`(ShowRaceMenu) / `0x0057A9B0`(ShowBirthSignMenu) / `0x005D86C0`(ShowSpellMaking) / `0x005A1EC0`(ShowEnchantment) / `0x00529820`,`0x00529840`(StartQuest/CompleteQuest) / `0x005E02B0`,`0x005E8E30`,`0x005E8E60`(SetUnconscious/SetRestrained)。「アクション本体」という分類は確定だが、どのゲーム機構を叩くかは本文精査が未実施。
2. **R1/R2 helper の引数 `a2`/`a3` の意味論が未判定**: オペコード実行経路では 0 が渡されるのに対し、CTDA 条件評価経路（`FUN_0056AC50`）では実値（他 ref / `param_1+0x10` の条件パラメータ）が渡る。今回の 111 件のうち `a2`/`a3` を実際に読む helper は特定していない。
3. **帯域 A/B における `+0x20` の消費者が未確認**: `FUN_0056AC50`/`FUN_0056AE20` はバンド C の基址 `0xB0C8E0` を即値で持つ。帯域 A（0x00-0x1E）・帯域 B（0x100-0x183）で `+0x20` を読むコードは本調査で未発見。今回の 115 件では帯域 A の FAM-06/FAM-13 が `+0x20==0` のため直接の問題は生じないが、表全体の 169 件には影響しうる。
4. **`+0x1C` の例外 dispatcher は本調査の族に非該当**: `0x500130`×2 / `0x500150`×1 / `0`×1（計 4 件）は 115 件に含まれない。例外 4 件の切り分けは未実施（V77 以来保留）。
5. **R1/R2 でトレース文字列を持たない 3 件**（`0x004F57D0` / `0x004F57F0` / `0x004F85E0`）: 役割判定は `store_f64` と `+0x20` 一致で行えたが、命名の裏付け（文字列）がない。
6. **V79 §6-4 の要件のうち「47件骨格族・20件族」以外の主要族の網羅性**: 本調査は `v79_bodies.json`（gap357）内での骨格重複に限定した。ギャップ外の 21,240 関数との骨格照合（V79 §6-1）は未実施のため、同一骨格がギャップ外にも存在する可能性は残る。
7. **`prologue16` による再現**: FAM-01 は `prologue16` では 16 群に分裂する。`cc_prefix` 列（yes/no の 1 列）は骨格族の指標として機能しない（`yes`=400 / `no`=127 のみ）。「骨格族」は本調査の骨格正規化でのみ復元できる。

## §7 自己検証

```
$ python "%TEMP%\opencode\probe_harness.py" selftest
   bytes@0x504050       518b4c24188b5424 PASS
   bytes@0x4FB580       578b7908 PASS
   string@0xA7035C      .\AI\CombatController.cpp PASS
   opid0x0015 impl      0x504560 PASS
   => ALL PASS
== [selftest] セルフテスト4件（ground truth） -> OK
```

**4/4 PASS**（着手時・完了時とも実行）。

### 7.1 検証サマリ

| 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| selftest | 4/4 PASS | 4/4 PASS | OK |
| オペコード表フィールド | impl=527 / disp(0x4FDE30)=528 / cond=169 | 527 / 528 / 169 | OK（V77/V78 GT 一致） |
| 骨格族数 / メンバー数 | 13 / 115（v79_summary） | 13 / 115 | OK |
| メンバーの `+0x18` 一致 | 115/115 | 115/115 | OK |
| メンバーの `+0x1C` == 0x4FDE30 | 115/115 | 115/115 | OK |
| `+0x20` == helper | 一致 + スタブ + 0 で 115 | 89 + 3 + 23 | OK |
| メンバーの E8 直接呼出し | 0（間接のみ） | 0/115 | OK |
| distinct helper | - | 111（辺 129） | 記録 |
| helper の Ghidra 未登録 | - | 94/111（うち独立性裏付け 93） | 記録 |

### 7.2 生成物

- 本ファイル: `tmp/re/v97_forwarder_family_map.md`（新規 1 ファイルのみ）
- 中間データ（リポジトリ外）: `%TEMP%\opencode\v97_forwarder_family.py` / `v97_data.json` / `v97_rows.json` / `v97_helper_rows.json` / `v97_vtloads.json` / `v97_opcode_table.json` / `v97_families.json` / `v97_member_table.md` / `v97_skel_examples.txt` / `v97_mk_report.py`
