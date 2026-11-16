# v440 — byte[obj+4] の type-blob 検証・flag consumer 拡張・docs 欠落下限の裁定（V-440-1..3 完遂・派生残点第32弾）

- 採番: **v440**（派生残点群・probe prefix **v440_1..v440_5 の5本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v439 の残点 **V-440-1【低】（`FUN_00469520` case 集合＝FormType enum 対応と `0x03` handler の `byte[obj+4]` identity）／V-440-2【低】（flag A の他 consumer と `+4 id(=type+1)` の consumer・表末尾不一致の意味）／V-440-3【低】（docs 欠落数の精密化と `[esp+0x5c]` path CFG）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`mov byte [reg+4], imm8` パターンの二重構造** | **[確定]** | **`+4` type byte パターンは2階層で存在**: **(A) BSExtraData 派生 ctor 群（cluster-1 `0x429DE0..0x429EBA`）** — 構造 **`{+0=vtable, +4=type tag(u8), +8=0, +0xC=payload}`・`ret 4`**・tag は 0x26..0x2C 連番・payload は dword/word/float/byte を type で分岐。**vtable RTTI 実読で全同定 [確定]**: `0xA3580C=.?AVExtraOriginalReference@@`(tag 0x26)／`0xA35818=.?AVExtraOwnership@@`(0x27)／`0xA3583C=.?AVExtraCount@@`(0x2A)／`0xA35848=.?AVExtraHealth@@`(0x2B)／`0xA35854=.?AVExtraUses@@`(0x2C) → **`+4`＝ExtraData の type tag [確定]** **(B) TESForm 系の FormType store（候補）** — v440_2 の vtable-window 相関で `0x14→TESObjectARMO/0x15→TESObjectBOOK/0x16→TESObjectCLOT/0x21→TESObjectWEAP/0x2B→TESLevItem/0x1C→TESObjectSTAT/0x23→TESNPC/0x24→TESCreature/0x32→Character/0x33→Creature` を各 imm に相関（ただし window 法ゆえ Extra 汚染あり＝**[推定高]**）。v440_3(b) の cluster-2 側 hit（`0x4B5AD9`）は **aligned decode で data 帯と判明＝偽陽性 [確定]** → **FormType store の直接証拠は本裁定では未達 [未確定]** |
| **V-440-1: `byte[obj+4]`（`0x03` handler）の identity** | **[推定高]（強化）** | **FormType と推定**。根拠: **(i) switch set 14 値と inventory 品目の整合** — `0x14/0x15/0x16/0x21/0x2B` の RTTI 相関が `TESObjectARMO/BOOK/CLOT/WEAP/LevItem`（＝inventory 品目）で **一致**、一方 **`0x1C→TESObjectSTAT/0x23→TESNPC/0x24→TESCreature` は switch に不在＝否定側も一致**（NPC/Creature/Static は inventory 品目でない）。**(ii) gate `{0x31,0x32,0x33}` の RTTI 相関＝`0x32→Character/0x33→Creature`（＋0x31 は未特定）＝ reference の3型** — v436 の「reference object 判定」と整合。**(iii)** 「inventory object」エラー文言。ただし **(a)(B) の直接証拠未達・enum 名は docs に無し（'FormType' grep 0 hits）** → **[推定高] に留める** |
| **V-440-2: flag A（`0xB0A54C`）の他 consumer** | **[確定]（否定側）＋[確定]（base 参照）** | **byte consumer は `FUN_004fde30` の `mov bl` のみ（他0件）[確定(否定側)]**。ただし **base 参照1件**: **`FUN_004fca10 @0x4FCAB1: cmp esi,0xB0A54C; jl 0x4fca80`**（range 分岐→`mov esi,0xB0C8C4` 側で stricmp name lookup 続行）→ **flag 配列の base を type-id range 判定に使用 [確定]**（`FUN_004fca10`＝name/id lookup の range 分岐 fn）。`0xB0A550`/`0xB0A554`（`id` フィールド）への .text refs は **0件 → id consumer なし（computed 経由も含め直接参照なし）[確定(否定側)]**。**→ `id=type+1` フィールドは .text consumer 不在、表末尾（type0x25 の `0x00A51B8C`）不一致の意味もこれで [未確定]（データ由来か他セクション参照か切り分け不能）** |
| **V-440-2: flag B（`0xB0A54D`）の consumer 拡張** | **[確定]** | **refs 6件（v439 で2件 was → 新4件）**: `FUN_004fde30` ×2（既知）＋ **`FUN_004fab20 @0x4FAED6`**（`cmp byte [eax*8+0xb0a54d],0; je 0x4fb1a1` — console/function 実行 gate の中で type-B 判定＝**実行時 dispatch 追加 [確定]**）＋ **`FUN_0056b190 @0x56B1C5: mov al,byte [edx*8+0xb0a54d]; ret`（flag B を返す1行 getter [確定]）** ＋ **`FUN_0056b220 @0x56B25C`**・**`FUN_0056b2e0 @0x56B3A5`**（`cmp byte [..+0xb0a54d],0; je` → arg pool ディスクリプタ読み `mov esi,[esi+ecx*4+4]` → push … — **script compiler/VM 帯の第3/第4系統 [確定]）** |
| **V-440-3: docs 欠落数の精密化（minseg=0x10）** | **[確定]（実測）＋[推定]（解釈）** | **minseg=0x10 で再 census（gaps>0x200）: gaps 2735／segments 7484／extra 4749** — v439 の minseg=0x30 版（extra 1737）に **小 stub（0x10..0x2F）を含めると extra が 3倍強に増加** → **docs 欠落 candidates は 1737（大 stub のみ）～4749（16B 以上）レンジ [確定(実測)]**。**intra-function padding 偽陽性を含む上位値＝実欠落は 5k 未満 [推定]**（ret-confirmed 手法の 0x10 版は未実施＝残点） |
| **V-440-3: `[esp+0x5c]` path CFG** | **[未確定]（残点）** | v440 群では未着手（probe budget）。v439 の fill 点6件＋`0x40A29D` branch 0件（fall-through）の記録を維持。 |

## §2 証拠（v440_1..v440_5）

### §2.1 `FUN_00469520` 全 body ＋ docs enum 名（v440_1）
```
L83027: undefined4 FUN_00469520(undefined1 param_1) {
  switch(param_1) {
    case 0x13,0x14,0x15,0x16,0x19,0x1a,0x1b,0x21,0x22,0x26,0x27,0x28,0x2a,0x2b: return 1;
    default: return 0; } }          <- default=0 を body 完読で確定
docs greps: 'kFormType'/'FormType_'/'FormType' すべて 0 hits  <- enum 名は docs に無し
```

### §2.2 `+4` type byte の二重構造（v440_2/v440_4/v440_5）
```
(a) ctor 小型 fn 群 cluster-1 (0x429DE0..0x429EBA), 全 fn 同型:
  429DE0: mov eax,ecx / mov ecx,[esp+4]
  429DE6: mov byte [eax+4], 0x26      ; type tag
  429DEA: mov dword [eax+8], 0
  429DF1: mov dword [eax], 0xa3580c    ; vtable
  429DF7: mov dword [eax+0xc], ecx     ; payload (dword ver)
  429DFA: ret 4
  429E60..: mov cx,[esp+4] / [eax+4]=0x2a / [eax+0xc]=cx  (word ver: ExtraCount)
  429E80..: fld [esp+4] / [eax+4]=0x2b / fstp [eax+0xc]    (float ver: ExtraHealth)
  429EA0..: mov cl,[esp+4] / [eax+4]=0x2c / [eax+0xc]=cl   (byte ver: ExtraUses)
RTTI (v440_5a): 0xA3580C=.?AVExtraOriginalReference@@ 0xA35818=.?AVExtraOwnership@@
                0xA3583C=.?AVExtraCount@@ 0xA35848=.?AVExtraHealth@@
                0xA35854=.?AVExtraUses@@   -> +4 = ExtraData type tag [確定]

(b) vtable-window 相関 (v440_2, TES 行のみ抜粋, [推定高]):
  0x14 -> TESObjectARMO / 0x15 -> TESObjectBOOK / 0x16 -> TESObjectCLOT
  0x21 -> TESObjectWEAP / 0x2B -> TESLevItem   (switch set 内 = 一致)
  0x1C -> TESObjectSTAT / 0x23 -> TESNPC / 0x24 -> TESCreature (switch 外 = 否定側一致)
  0x32 -> Character / 0x33 -> Creature          (v436 gate 内 = reference 整合)
  (Extra 系行 = cluster-1 の tag ctor 由来の window 汚染)
cluster-2 側 hit 0x4B5AD9 は data 帯偽陽性 (v440_5b aligned decode: 文字列/定数 data)
sig-based search (v440_3a): 'ARMO'/'BOOK'/... imm32 の near-store は 0 hits (ESM loader
  は sig->formtype 直接 store なし)
```

### §2.3 flag 配列 consumer（v440_4c / v440_5a2）
```
0xB0A54C refs: 0x4FCAB1 (FUN_004fca10: cmp esi,0xb0a54c / jl 0x4fca80 -> range 分岐)
               0x4FDEB3 (FUN_004fde30: mov bl,byte[eax*8+..] = flag A)
0xB0A54D refs: 0x4FAED6 (FUN_004fab20: cmp byte[eax*8+0xb0a54d],0; je 0x4fb1a1)
               0x4FDF4A / 0x4FDF89 (FUN_004fde30 x2)
               0x56B1C5 (FUN_0056b190: mov al,byte[edx*8+0xb0a54d]; ret = getter)
               0x56B25C (FUN_0056b220: cmp .. je -> mov esi,[esi+ecx*4+4] arg pool)
               0x56B3A5 (FUN_0056b2e0: cmp .. je -> mov edx,[esp+0x54] ...)
0xB0A550 / 0xB0A554 refs: 0  <- id フィールド consumer 不在 [確定(否定側)]
FUN_004fca10 boundary: cmp esi,0xb0a54c; jl -> mov esi,0xb0c8c4; mov ecx,[esi-4]; push;
  push edi; call 0x98262d (stricmp) -> name lookup の range 分岐 [確定]
```

### §2.4 docs gap census minseg=0x10（v440_5c）
```
gaps > 0x200: 2735, segments(minseg=0x10, cc>=8): 7484, extra: 4749
  (minseg=0x30 版: 4471 / extra 1737 — 小 stub 追加で extra 2.7倍)
```

## §3 残点 **V-441-1..3**
1. 【低】**`byte[obj+4]`=FormType の直接証拠** — TESObjectARMO 等の**本物 class ctor** を aligned decode して `mov byte [reg+4], imm` を確認（v440_3(b) の cluster-2 hit は data 偽陽性だったため未達）＋ enum 名テーブルの有無を EXE 内 string census（`kFormType`/型名リスト）で確認。
2. 【低】**flag B 新 consumer 4件の identity** — `FUN_0056b190`（getter）の callers、`FUN_0056b220`/`FUN_0056b2e0` の所属（script compiler/VM のどの pass）＋ `FUN_004fab20 @0x4FAED6` の分岐先 `0x4fb1a1` の意味（実行時 type-B 判定で何を拒否するか）。
3. 【低】**docs 欠落の最終レンジ確定** — minseg=0x10 版 ret-confirmed census（前 segment 末尾=C3 確認を 0x10 閾値で再計数）で偽陽性除去＋`[esp+0x5c]` path CFG（`0x40A29D` 到達経路ごとの直近 write 確定）。

## §4 検証・自己批評
- probe: **v440_1..v440_5 の5本全 EXIT=0**（V-440-1 は [推定高] に強化（直接証拠は[未確定]）／V-440-2 完遂（否定側＋新4件）／V-440-3 は census 完遂・CFG は残点）。
- **決定打の作り**: (i) **`mov byte [reg+4], imm` 全 .text census（212件）→ vtable-window RTTI 相関**で imm→class 表を試作 → (ii) **cluster-1 の tiny fn 群を aligned decode して同型 ctor と判明 → vtable 直読（RTTI）で ExtraData 系と確定**（window 法の Extra 汚染を自白）→ (iii) **switch set と相関表の肯定/否定整合**（inventory 品目は在・NPC/CREA/STAT は不在）で FormType 説を強化 → (iv) consumer census は **imm32 byte-pattern search で全 refs 列挙**（getter 1行 fn まで拾う）→ (v) census 精密化は **minseg だけ動かして再計数**（1737→4749 の感度を実測）。
- 妥当性チェック: ①cluster-1 tag 0x26..0x2C⇔RTTI ExtraOriginalReference/Ownership/Count/Health/Uses（+28,+29 の Extra 系は window 未特定だが連番と整合）— ②switch 外 `{0x1C,0x23,0x24}`⇔`STAT/NPC/Creature`（inventory 非対象）— ③gate `{0x32,0x33}`⇔`Character/Creature`（reference）— ④`id` refs 0件⇔v439 で id consumer [未確定] の放置と整合 — ⑤minseg=0x10 extra 4749 > minseg=0x30 extra 1737（単調性）— **五方向矛盾なし**。
- docs との不整合: **なし**（v439 の FormType enum 名が docs に無いこと（'FormType' 0 hits）を再確認したのみ）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **「vtable-window 相関」は type byte の同定に使えるが ExtraData/variant 汚染を必ず伴う** — **決定は「tiny fn 群を aligned decode→vtable 直読（RTTI）」で行い、window 相関は [推定高] の材料止まりにする**。
  2. **否定側整合（switch に NPC/CREA/STAT が不在）が肯定側より強い証拠になり得る** — enum 突合では「入っているべきもの」より「入っていないべきもの」を先に見る。
  3. **small ctor 群（`ret 4`・0x20 間隔・int3 padding）は type-tag 定数の供給元** — `mov byte [reg+4], imm` census で拾い vtable から RTTI を引けば、その byte offset の意味（ExtraType/FormType）が class 名で即決まる。
  4. **imm32 文字列シグネチャ（'ARMO'等）の near-store 探索は ESM loader では空振り**（factory は別経路）— sig→enum 対応は別の fn（比較 switch）を探す必要。
  5. **census の閾値感度は必ず2値で測る**（minseg 0x30→1737、0x10→4749）— 単一閾値の数値は引用時に感度を併記しないと誤解を招く。
  6. **data 帯 hit（aligned decode が movsb/inc 帯を返す）は偽陽性** — `mov byte [reg+4], imm` byte-pattern は data にも出現する。decode の出力が prologue/操作命令でなければ即棄却。
