# V18: 台帳480 opid 間接呼出抽出と vtable スロット解決

対象: Oblivion.exe v1.2.0416 (Steam) / 台帳: tmp\re\svm_495_miss_addrs.md (480 opid) / vtable: tmp\re\vtable_map.txt (2,172クラス)

## 1. セルフテスト4件の実測値

| # | 検査 | 実測 | 期望 | 結果 |
|---|------|------|------|------|
| ① bytes@0x504050 | - | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| ② bytes@0x4FB580 | - | `578b7908` | `578b7908` | PASS |
| ③ string@0xA7035C | - | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| ④ opid 0x0015 impl | - | `0x00504560` | `0x00504560` | PASS |

注: VA→file_off は PE セクションテーブルによる汎用マッピング（.text=RawPtr 0x400, .rdata=RawPtr 0x627200, .data=RawPtr 0x700C00）。

## 2. 間接呼出を持つ opid 件数と総呼出数

- 間接呼出（call/jmp の間接形式）を持つ opid: **119 / 480**
- 総間接呼出数（CSV データ行数）: **406**
- impl デコード失敗: 0 件

## 3. 形式別内訳

| 形式 | 件数 |
|------|------|
| call <reg> | 375 |
| call [reg+disp] / [reg] | 0 |
| call [abs32] | 26 |
| jmp <reg> | 0 |
| jmp_idx（補助） | 4 |
| jmp_mem（補助） | 1 |

所見: `call [reg+disp]` 形式は 480 opid 中 **0件**（vtable 経由呼出は全て `call <reg>` + 直前の `mov reg,[eax+off]` パターン）。`jmp <reg>` も 0件。`call [abs32]` 26件は全て IAT（.rdata 0xA280xx 帯）経由の外部 DLL 呼出。`jmp [reg*4+abs]` 4件は switch ジャンプテーブル、`jmp [reg+disp]` 1件は vtable スロット経由ジャンプ（0x514501, [eax+0x48]）。

## 4. 固有 disp トップ20 と vtable スロット代表クラス

集計対象: vtable スロット候補となり得る off 値（call <reg> のトレース元 mem disp / call [reg+disp] / jmp [reg+disp]）。abs32 呼出やジャンプテーブル地址は除外。

| disp | slot | 件数 | 代表クラス（先頭3） |
|------|------|------|--------------------|
| 0x40 | 16 | 33 | NiVertexColorProperty, NiWireframeProperty, NiZBufferProperty 他522件（計525件） |
| 0xD4 | 53 | 27 | AlchemyItem, MagicItemObject, EffectSetting 他95件（計98件） |
| 0x154 | 85 | 26 | TESCombatStyle, DefaultCombatStyle, TESGrass 他20件（計23件） |
| 0x170 | 92 | 26 | TESGrass, TESObjectTREE, TESObjectREFR 他18件（計21件） |
| 0x174 | 93 | 23 | TESGrass, TESObjectREFR, Actor 他17件（計20件） |
| 0x0 | - | 19 | 範囲外 |
| 0x50 | 20 | 14 | NiVertexColorProperty, NiWireframeProperty, NiZBufferProperty 他471件（計474件） |
| 0x164 | 89 | 12 | TESCombatStyle, DefaultCombatStyle, TESGrass 他20件（計23件） |
| 0x330 | 204 | 12 | Actor, BaseProcess, Character 他6件（計9件） |
| 0x190 | 100 | 10 | TESObjectREFR, Actor, ArrowProjectile 他14件（計17件） |
| 0x8 | - | 10 | 範囲外 |
| 0x14 | 5 | 8 | MemoryHeap, NiVertexColorProperty, NiWireframeProperty 他1061件（計1064件） |
| 0x18 | 6 | 8 | MemoryHeap, NiVertexColorProperty, NiWireframeProperty 他1013件（計1016件） |
| 0x208 | 130 | 5 | Actor, BaseProcess, Character 他11件（計14件） |
| 0x100 | 64 | 5 | AlchemyItem, MagicItemObject, IngredientItem 他54件（計57件） |
| 0x88 | 34 | 4 | SceneGraph, AlchemyItem, MagicItemObject 他284件（計287件） |
| 0x204 | 129 | 4 | Actor, BaseProcess, Character 他11件（計14件） |
| 0x4 | - | 4 | 範囲外 |
| 0x198 | 102 | 4 | TESObjectREFR, Actor, ArrowProjectile 他14件（計17件） |
| 0x10 | 4 | 4 | MemoryHeap, hkCollisionListener, NiVertexColorProperty 他1115件（計1118件） |

## 5. トレース結果

- call <reg> / jmp <reg> 合計: 375 件
- トレース成功: **365 件**
- トレース不能: **10 件**

| 成功ソース形式 | 件数 |
|----------------|------|
| mov reg, [base+off] | 359 |
| lea reg, [base+off] | 0 |
| mov reg, [abs32] | 6 |

| 不能理由 | 件数 |
|----------|------|
| no write in 20-insn window | 10 |

## 6. 自己整合チェック: opid 0x0165 SaveIniFiles

impl 0x00513B60 からの抽出結果（3件）:

| site_va | insn | disp | trace_src |
|---------|------|------|-----------|
| 0x00513B7F | call edx | 0x14 | mov edx, dword ptr [eax + 0x14] |
| 0x00513B92 | call edx | 0x1C | mov edx, dword ptr [eax + 0x1c] |
| 0x00513BA1 | call edx | 0x18 | mov edx, dword ptr [eax + 0x18] |

判定: **一致** — 既知情報（call edx ×3, off +0x14/+0x18/+0x1C, trace mov edx,[eax+off]）と照合。

注: 台帳の名称列は `Op0165`（RE_EXE_REPORT 系の呼称では SaveIniFiles）。件数・レジスタ・オフセットは完全一致。

## 7. 判定できなかった点

- call <reg> のトレース元が `lea` の場合、off はオフセット計算値であり vtable スロットである保証はない（候補提示のみ）。
- 候補が10件超のスロットはクラス特定不可（件数のみ記録）。同一スロットを参照する多数クラスが存在する場合、呼び出し元のthis型が未特定のため一意決定できない。
- `call [reg+disp]` の base レジスタ由来（vtable ポインタ解決元）は本タスク範囲外のため未トレース（disp のグローバルクラス照合のみ実施）。
- disp 範囲外（<0x10, >0x400, 4の倍数でない）は vtable スロット未解決。
- リニアデコードは int3 / ret+0xCC 終端・上限0x800で区切った近似。カウンドジャンプ等で実際の関数境界とずれる可能性は排除できない（外部呼出版台帳と同一手法）。
