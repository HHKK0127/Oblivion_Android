# v432 — keyword registry lookup と id→record lookup の裁定（V-432-1 完遂・派生残点第24弾）

- 採番: **v432**（派生残点群・probe prefix **v432_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v431 の残点 **V-432-1【低】（cell+73 参照（`0x4FCA7A`）の fn 特定と用途＋`0x4FCA30` validator identity）** を消化（V-431-1 の消費者の最後のピースを閉じる）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **cell+73（`0xB0A2CC`）の参照 fn** | **[確定]** | **`FUN_004FCA70`（head `0x4FCA70`・`FUN_004fca10` annotation の後続独立 fn）＝ keyword registry の name lookup**: `mov edi,[esp+0x10]`（検索文字列）→ `xor ebx,ebx`（i=0）→ **`mov esi,0xB0A2CC`（cursor＝cell+73）→ loop: `mov eax,[esi-4]`（＝rec0.name `0xB0A2C8`＝`'Begin'`）→ `push edi; push eax; call 0x98262d`（stricmp）→ 一致で `je 0x4FCB2B`（found）** → **`0xB0A2C8` keyword テーブルを name で linear 検索する lookup [確定]**（＝v431 の消費者質問に回答） |
| **`FUN_004FCA30` の identity** | **[確定]** | **id→record pointer lookup（stride `0x28` テーブル2帯）**: `id∈[0x100,0x182]` → **`lea eax,[ecx+ecx*4-0x500]; lea eax,[eax*8+0xB0B420]` ＝ `0xB0B420 + (id-0x100)×0x28`**（`×5×8=×40=0x28` ✓）／`id∈[0x1000,0x1170]` → **`0xB0C8C0 + (id-0x1000)×0x28`**／範囲外は **`xor eax,eax`＝0** → **stride0x28 の opcode/keyword レコード table への id 変換 fn [確定]（0x100 帯＝script opcode id [推定高]）** |
| **stride-0x28 テーブルの系統** | **[確定]（構造）** | **同一 stride の table が3系統存在**: `0xB0A2C8`（**keyword/type**: `Begin`0x10/`End`0x11/`Short`0x12...・name lookup＝`FUN_004FCA70`）／`0xB0B420`（id `0x100..0x182`＝131件・`FUN_004FCA30`）／`0xB0C8C0`（id `0x1000..0x1170`＝369件・`FUN_004FCA30`） |
| **`FUN_004FCA30` の consumers** | **[確定]**（E8=3） | `FUN_004FAB20`／`FUN_00516830`／`0x51703E`（head 未注記）— opcode id→record の利用側（compiler/VM 帯） |

## §2 証拠（v432_1）

### §2.1 `FUN_004FCA70`（machine 0x4FCA70..0x4FCA8F＋linear 続行）
```
4FCA70: push ebx/esi/edi
4FCA73: mov edi, [esp+0x10]        ; 検索文字列
4FCA77: xor ebx, ebx               ; i = 0
4FCA79: mov esi, 0xb0a2cc          ; ★ cursor = cell+73
loop:
4FCA80: mov eax, [esi - 4]         ; rec0.name = [0xB0A2C8] = 'Begin'
4FCA83: push edi / push eax
4FCA85: call 0x98262d              ; stricmp(rec_name, search)
4FCA8D: je 0x4fcb2b                ; found
```
- `find_head(0x4FCA7A)`＝`0x4FCA10` は annotation の境界誤り（linear が `0x4FCA6F ret` → `0x4FCA70 push` の独立 fn 開始を明示）＝**find_head 境界誤り4件目**。

### §2.2 `FUN_004FCA30`（machine 0x4FCA30..0x4FCA6F）
```
4FCA30: mov ecx, [esp+4]           ; id
4FCA34: lea edx, [ecx - 0x100]
4FCA3C: cmp edx, 0x82 / ja 4FCA53  ; id ∈ [0x100,0x182]
4FCA44: lea eax, [ecx+ecx*4-0x500] ; (id-0x100)×5
4FCA4B: lea eax, [eax*8+0xb0b420]  ; ×8 → (id-0x100)×0x28 + base
4FCA52: ret
4FCA53: lea edx,[ecx-0x1000] / cmp edx,0x170 / ja → eax=0
4FCA61/4FCA68: 同型 → 0xB0C8C0 + (id-0x1000)×0x28
```
- E8=3: `FUN_004FAB20`／`FUN_00516830`／`0x51703E`。

## §3 残点 **V-433-1..3**
1. 【低】`0xB0B420`（id0x100..0x182）／`0xB0C8C0`（id0x1000..0x1170）の**レコード内容実読**（stride0x28 内の name ptr 群＝opcode 名の確定と、2帯の id 空間の語（script command？ function？））。
2. 【低】`FUN_004FCA70` の callers（keyword lookup の利用側＝parser のどの pass）と found 分岐（`0x4FCB2B`）以降の処理（record のどこを使うか）。
3. 【低】keyword table（`0xB0A2C8`）の終端（id 連番がどこまでか＝`0x00..0x0F` 帯＝V-431-2 引き継ぎ）。

## §4 検証・自己批評
- probe: **v432_1 全 EXIT=0**（1本で V-432-1 完遂）。
- **決定打の作り**: (i) **head 起点 linear decode で「cursor 設定＋`[esi-4]` 読み＋stricmp」ループを復元**（find_head の annotation 誤りを linear が訂正）→ (ii) **`lea (id-base)×5 → ×8` の2段 lea を「stride0x28 計算」と即読**（`(×5)×8=×40=0x28`⇔keyword table の stride と一致）→ (iii) 2 id 帯と base2 本を特定。
- 妥当性チェック: ①`(id-0x100)×5×8 = (id-0x100)×0x28`⇔keyword table stride — ②cursor `0xB0A2CC`→`[esi-4]`=rec0⇔v430 の rec0 位置 — ③E8=3 consumers⇔opcode 利用側 — **三方向矛盾なし**。
- docs との不整合: **なし**（v431 の残点を確定）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`lea a×5` → `lea b×8` の連鎖は「×40＝stride」の即読パターン** — テーブル stride と id 変換式が1組で表れる。
  2. **cursor 定数＋`[cursor-4]` 読み＝「レコード0起点の linear 検索」の定型**（cell+73 が「rec0.name の直後セル」という一見変な位置でも、`[esi-4]` が rec0 を指す）。
  3. **annotation境界の find_head 誤りは linear decode が訂正する（4件目）** — head=annotation と独立 fn 開始を必ず突合。
