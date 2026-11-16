# v431 — キーワード/型レジストリの消費者と dump family 第3弾の裁定（V-431-1 部分解・派生残点第23弾）

- 採番: **v431**（派生残点群・probe prefix **v431_1・v431_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v430 の残点 **V-431-1【低】（キーワード/型レジストリの消費者 identity（`.text 0x6CA5C2` と cell+73 参照）と `name2`（`'int'` 等）の意味）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`'Begin'` 文字列の `.text` 消費者（`0x6CA5C2`）** | **[確定]（周辺）＋[推定高]（用途）** | **`FUN_006CA440`（print/dump fn・V-387-1 対象）内 `push 0xA51C1C('Begin'); call 0x70FB90`** — 同 band（0x6CA440..0x6CA700）の push リテラル群＝**`'State'/'ArraySize'/'Weight'/'CycleType'/'Freq'/'Begin'/'End'/'Last'/'Offset'/'Start'/'End'`**（各 `call` で format helper へ）＝ **NIF ブロック/コントローラ系の float フィールド dump [確定(実文字列)]** → **`FUN_006CA440`＝NIF dump/report fn [推定高]（＝V-387-1「fn 0x6CA440 正式名」への強い手がかり）** |
| **`FUN_0070FB90` の identity（dump family 第3弾）** | **[確定]** | **`"%s = %g"` format helper（name＋float 値の行生成・L553196-553211: strlen→malloc→`FUN_006c5d40(buf,n,"%s = %g",name,(double)val)`）** — dump family: **`FUN_0070F910`＝`"--- %s ---"`（v418）／`FUN_0070FAA0`＝`"%s = %hu"`（v419）／`FUN_0070FB90`＝`"%s = %g"`（本件）** |
| **`name2`（`'int'` 等）の消費者** | **[確定(否定側)]** | `'int'`（`0xA51C0C`）への **dword xref＝.data `0xB0A31C`（レコード自身）のみ・`.text` 0件** → **コードは直接参照せず、レジストリを base+offset アクセスで読む [確定]**（v427/v428/v430 の index 形と同一） |
| **cell+73（空文字列 ptr セル）の `.text` 参照（`0x4FCA7A`）** | **[未確定]（残点）** | v431_1(b) の初回 decode は window 境界で alignment 不明 → 正確位置は **`0x4FCA7A`（`FUN_004FCA10`（deleting dtor・ret 4 @0x4FCA2B）の後続 fn 帯）**。隣接 `0x4FCA30` は **`lea edx,[ecx-0x100]; cmp edx,0x82`＝id 範囲 validator（0x100..0x182＝script opcode id 帯 [推定高]）** → **registry 参照は validator/dtor 帯の別 fn で未 decode＝V-432-1** |

## §2 証拠（v431_1/v431_2）

### §2.1 `'Begin'` 消費者（v431_1(a)＋v431_2(c)）
```
FUN_006CA440 band（0x6CA440..0x6CA700）push リテラル群:
  6CA48C 'State' / 6CA4C8 'ArraySize' / 6CA507 'Weight' / 6CA543 'CycleType'
  6CA582 'Freq'  / 6CA5C1 'Begin'     / 6CA600 'End'     / 6CA63F 'Last'
  6CA67E 'Offset'/ 6CA6BD 'Start'     / 6CA6FC 'End'
（各 push の直後に call＝format helper（`0x70FB90` 等）へ渡す形）
6CA5C1: push 0xa51c1c ; call 0x70fb90   ← 'Begin'（aligned 確認済み）
```

### §2.2 `FUN_0070FB90`（L553196-553211）
```c
FUN_0070fb90(char *name, float val) {
  strlen → FUN_00401f00()（malloc）→ FUN_006c5d40(buf, n, "%s = %g", name, (double)val);
  return buf;
}
```

### §2.3 refs（v431_1(b)(c)＋v431_2(b)）
- `'int'`（`0xA51C0C`）: xref＝.data `0xB0A31C` のみ（`.text` 0件）。
- cell+73（`0xB0A2CC`）: `.text` `0x4FCA7A`（＝`FUN_004FCA10` ret4 @0x4FCA2B の後続帯・未 decode）。
- `FUN_004FCA10`＝deleting dtor（`call 0x4FC980`→flag で `0x401F20` free）。
- `0x4FCA30` 帯: `mov ecx,[esp+4]; lea edx,[ecx-0x100]; xor eax,eax; cmp edx,0x82` ＝ **id∈[0x100,0x182] 範囲 validator [推定高: script opcode id]**。

## §3 残点 **V-432-1..3**
1. 【低】**cell+73 参照（`0x4FCA7A`）の fn特定と用途**（keyword registry の reader/pass）＋`0x4FCA30` validator の identity（0x100..0x182 が opcode id か）。
2. 【低】**V-387-1 引き継ぎ**: `FUN_006CA440` の正式名の最終確定（dump fn [推定高] → NIF ブロック dump として RTTI/呼び出し元で確定）＋dump 対象ブロックの種類（State/ArraySize/Weight/CycleType/Freq が何の class の field か）。
3. 【低】keyword registry の **id→keyword lookup pass**（parser 側・base+offset reader）と id `0x00..0x0F` 帯（V-431-2）。

## §4 検証・自己批評
- probe: **v431_1・v431_2 の2本全 EXIT=0**（V-431-1 は [部分確定]＝消費者の周辺と name2 否定まで）。
- **決定打の作り**: (i) **文字列リテラルの push を fn band 全体で列挙**（1件の 'Begin' から field 名群へ拡大）→ (ii) **format helper の decompiled 実読で dump family を接続**（F910/FAA0/FB90 の3系統）→ (iii) **xref census の否定（name2＝.text 0件）** → (iv) **alignment を head 起点 linear decode で再検証**（初回 window decode の誤りを訂正＝0x4FCA7A へ）。
- 妥当性チェック: ①'Begin'/'End' push⇔`%s = %g`⇔dump fn — ②name2 direct xref 0⇔index access 形（v427..430）— ③cell+73 ref 位置の再計算（5,229,178=0x4FCA7A）⇔ret4 位置 — **三方向矛盾なし**。
- docs との不整合: **なし**（v430 の残点を部分確定）。**V-387-1（#152 由来の低残点）に新証拠**（dump fn [推定高]）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **文字列リテラルの consumer は「1件の xref」から「同 band の push 列挙」へ広げる** — field 名群が fn の役割を一撃で語る（State/ArraySize/Weight/CycleType/Freq→dump fn）。
  2. **dump/format helper は系列でそろえる**（`%s = %g`/`%s = %hu`/`--- %s ---`）— 同じ fn band の helper 群が同じ report システムの器官。
  3. **xref 位置の decimal→hex 変換と head 起点 linear decode をセットで**（本件も0x4FCA1A→0x4FCA7A の計算誤りを decode で検出）。
