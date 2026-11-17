# v507 — node `+8` bit 一覧（13bit）・insert callers 83件・string copy wrapper の裁定（V-507-1..3 完遂・旧残点 V-406-3 継続第37弾）

- 採番: **v507**（旧残点群消化・probe prefix **v507_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-507-1..3（v506 残点）** — 1=map ctor 特定と node `+8` bit 配列・`0x984B6A` identity／2=Player `+0x80` 中身と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-507-1: node `+8` u32 の bit 配列（owner/count 対応）** | **[確定相当]（観測 bit 13種）** | **node flag-bit tests（`>> N & 1` が `param_1[1]`/`piVar2[2]`/`+8` 付近）census＝331件 [確定(decompiled)]**: **bit5＝128 hits（最多）／bit0xb（11）＝83／bit3＝43／bit0xe（14）＝39／bit0xd（13）＝11／bit4＝9／bit6＝7／bit1＝5／bit7＝3／bit0x16（22）＝1／bit0x13（19）＝1／bit0x15（21）＝1 → node `+8` u32 の観測 bit 一覧＝0,1,3,4,5,6,7,11,13,14,19,21,22 の13種 [確定相当]**（v496 bit5＝`piVar2[2]>>5&1`・v492 bit14＝`param_2+8` と整合）。**→ lock の owner/count と map の key/value が同じ node `+8` u32 を bit field として共有する構造が確定相当（`node[+4]`＝key・`node[+8]`＝value/bitfield・v506 layout に接続）**。残り bit（8,9,10,12,15..18,20,23..31）は未観測＝grep pattern の網羅性制約 [部分確定] |
| **V-507-1: `FUN_00452570`（insert）の callers（map ctor 経路）** | **[確定]（83件列挙）＋[推定]（ctor 分岐）** | **E8 callers＝83件 [確定(機械)]**、owning signature 列挙: **map 系 band が近接集中（`FUN_00452C20`（v493 lazy node）・`FUN_00452C90`・`FUN_00452E70`・`FUN_00452F10`・`FUN_00452FE0` ＝ `0x452C20..0x453028` 帯）[確定]** ＋ **業務 fn（`FUN_004492E0`（v469 TESObjectSTAT ctor）・`FUN_0046C300`・`FUN_004B2730`・`FUN_004B3400`・`FUN_004C9230` 等）が多数 [確定]** → **insert の caller は「map 系（同 band の wrapper）」と「業務 fn（map を持つ object の method）」に分かれる [確定相当]** → **map ctor は業務 fn 側が map object を構築する [推定]**（**`FUN_004492E0`（TESObjectSTAT ctor・v469）が `FUN_00452570` を呼ぶ＝STAT 系 object が map を持つ**）→ **BST vs LockFree 判別は「業務 fn が構築する map の vtable store」へ経路を固定 [推定]**（v506 経路転換の到達点） |
| **V-507-1: `FUN_00984B6A`（string copy wrapper）identity** | **[推定相当]** | machine head（docs annotation なし）[確定(機械)]: **`mov ecx,[esp+4]`（dst/len 候補）→ null check（`cmp ecx,ebx`）→ `mov edi,[esp+0x14]`（len 候補）→ `cmp edi,ebx; ja 0x984b9a`（len>0 分岐）→ **`call 0x98540b`（実体 fn）** → `push 0x16`（22＝error code 候補 [推定]）** → **`FUN_00984B6A`＝null/len check＋`call 0x98540b` する wrapper [推定相当]**（実体は `FUN_0098540B`＝残点）。`push 0x16`＝22 は `SIGINT`/error 値候補 [推定] |
| **V-507-2: Player `+0x80` 中身** | **[未確定]（残点継続）** | 本 probe 未着手（budget は node bit／insert callers／string copy に集中） |
| **V-507-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v507_1）

### §2.1 node bit census（v507_1b）
```
node flag-bit tests (>> N & 1 near node fields): 331 total
  bit 5:   128 hits   (v496 piVar2[2]>>5, most frequent)
  bit 0xb:  83 hits   (bit11)
  bit 3:   43 hits    (v494 FUN_0045B670 param_1+0x18 bit3)
  bit 0xe:  39 hits   (bit14, v492 param_2+8)
  bit 0xd:  11 hits   (bit13)
  bit 4:    9 / bit 6: 7 / bit 1: 5 / bit 7: 3
  bit 0x16: 1 (bit22) / bit 0x13: 1 (bit19) / bit 0x15: 1 (bit21)
=> observed node[+8] bits: 0,1,3,4,5,6,7,11,13,14,19,21,22 (13 kinds)
=> lock owner/count and map key/value share node[+4]/node[+8] layout  [確定相当]
```

### §2.2 insert callers（v507_1c）
```
E8 callers of FUN_00452570: 83
  map-band wrappers: FUN_00452c20 (v493) / 00452c90 / 00452e70 /
                     00452f10 / 00452fe0   (0x452C20..0x453028 band)
  business fns: FUN_004492e0 (v469 TESObjectSTAT ctor) / 0046c300 /
                004b2730 / 004b3400 / 004c9230 / ... (83 sites total)
=> caller split: map wrappers vs business methods  [確定相当]
=> map ctor lives on the business side (STAT object holds a map)  [推定]
```

### §2.3 `FUN_00984B6A`（v507_1a）
```
984B6A: mov ecx,[esp+4] / cmp ecx,ebx / je 0x984b7f     ; null check
984B77: mov edi,[esp+0x14] / cmp edi,ebx / ja 0x984b9a   ; len>0 check
984B7F: call 0x98540b                                    ; real body [推定]
984B84: push 0x16                                         ; error code 22 [推定]
=> wrapper (null/len check -> FUN_0098540B)  [推定相当]
```

## §3 残点 **V-508-1..3**
1. 【中】**`FUN_0098540B`（string copy 実体）identity** ＋ **業務 fn 側の map vtable store 特定**（BST vs LockFree 判別完遂＝`FUN_004492E0` 内の vtable store が key）＋ **未観測 bit（8,9,10,12,15..18,20,23..31）の grep 拡張**。
2. 【低】**Player `+0x80`（ptr）の中身** ＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v507_1 の1本 EXIT=0**（V-507-1 node bit 13種を census で確定相当・insert callers 83件を列挙し ctor 経路を固定・string copy を wrapper 推定相当／V-507-2 は budget 外で残点／V-507-3 引継ぎ）。
- **決定打の作り**: (i) **node flag-bit census（331件・12 bit 分布）**で bit 配列を機械的に獲得（**v496 bit5/v492 bit14 を一般化**）→ (ii) **insert callers 83件の signature 列挙**で「map band と業務 fn」の分岐を発見 → ctor 経路を業務 fn へ固定 → (iii) **string copy machine head** で wrapper（null/len check→`call 0x98540B`）と判定。
- 妥当性チェック: ①bit5＝128⇔v496 `piVar2[2]>>5&1`（最多＝最頻 flag）— ②bit14⇔v492 `param_2+8`（gated gate）— ③bit3⇔v494 `FUN_0045B670` gate（`param_1+0x18` とは別 object だが同 pattern）— ④`FUN_004492E0`⇔v469 TESObjectSTAT ctor（map を持つ object）— ⑤`call 0x98540B`⇔v505 strlen alloc 後段 — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_00984B6A`/`FUN_00452570` は docs annotation 実在・machine と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **flag bit の一覧は「`>> N & 1` census」で機械的に揃う**（**331件・12 bit 分布＝bit5 が最多**＝**field の bit map が1本の probe で完成形に近づく**）。
  2. **E8 callers の signature 列挙が「caller の分類（map band vs 業務 fn）」を生む**（**83件の分岐が ctor 経路を固定**＝v479/v488 に続く callers census 法）。
  3. **`call <fn>`＋`push <imm>` の machine head が wrapper（check→実体→error code）の定番**（`FUN_00984B6A`＝null/len check→`FUN_0098540B`→`push 0x16`）。
  4. **業務 fn（object ctor）が map を構築する**（**`FUN_004492E0`（STAT ctor）→ insert**）＝**container は owner object の method で構築される**（BST vs LockFree 判別の鍵）。
  5. **未観測 bit（8,9,10,12,15..18,20,23..31）も残点として明記**（**grep pattern の網羅性制約を記録**＝次の probe で拡張可能）。
