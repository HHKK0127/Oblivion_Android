# v473 — gate `FUN_00446B80` と `form+0x69` setter caller の裁定（V-473-1..3 完遂・旧残点 V-406-3/V-406-4 継続第3弾）

- 採番: **v473**（旧残点群消化・probe prefix **v473_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-473-1..3（v472 残点）** — 1=旧残点 V-406-3 継続（`elem[10]` class特定＋`FUN_00446B80` gate identity）／2=旧残点 V-406-4 残り（`form+0x69` byte semantic）／3=V-472-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-473-1: `FUN_00446B80`（`FUN_00566830` gate）identity** | **[確定]** | L54126: **`bool FUN_00446b80(uint param_1) { return 0xfeffffff < param_1; }`（1行）[確定(decompiled)]** → **`param_1 > 0xFEFFFFFF` ⇔ `param_1 >= 0xFF000000` ⇔ 上位 byte==0xFF [確定]** → **bit11 flag gate＝「`self[+0xC]` の u32 上位 byte が `0xFF`（`0xFF000000..0xFFFFFFFF`）のとき true」[確定相当]**（`self[+0xC]` が「flags/状態 u32」で上位 byte 0xFF＝特定状態 [推定]） |
| **V-473-1: `elem[10]`（byte +40 float）の class特定** | **[部分確定]（field アクセス）＋[未確定]（class）** | **`FUN_004CD960` machine: `lea edi,[esi+0x48]`（2箇所・list head アドレス）と `mov ecx,[esi+0x50]`（count/field）[確定(機械)]** — ただし **elem の vtable/field は linear scan で未捕捉（walk 中身が decode 範囲外）[部分確定]** → **class特定は [未確定]＝残点**（v470 `TESObjectSTAT` singleton 比較との接続は維持） |
| **V-473-2: `form+0x69` byte setter（`FUN_00468270`）の caller** | **[確定]（1件）＋[確定相当]（連動）** | **E8 census＝1件: `0x522741` owning=`FUN_005222D0`（L216747）[確定]**、context **`522745: call ptr [esi+0x5f]`（indirect call）** → **`FUN_00468270`（`*(u8*)(self+0xD)=v`）は `FUN_005222D0`（v462 derived AV 初期適用 fn）から indirect 経由で呼ばれる [確定]** → **`form+0x69` byte は derived AV 初期適用（v462 の `param_2==0` ブロック）内で setされる [確定相当]**（`[esi+0x5F]` の slot 特定は [未確定]＝残点） |
| **V-473-3: V-472-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v473_1）

### §2.1 gate（v473_1a）
```
L54126: bool FUN_00446b80(uint param_1) {
  return 0xfeffffff < param_1; }        // param_1 >= 0xFF000000 (top byte == 0xFF)
```

### §2.2 `FUN_004CD960` machine（v473_1b）
```
0x4CD969: lea edi, [esi + 0x48]     (list head address)
0x4CD9FA: lea edi, [esi + 0x48]
0x4CDADB: mov ecx, dword ptr [esi + 0x50]   (count/field)
  -> elem vtable/field not captured in linear scan = residual
```

### §2.3 `FUN_00468270` caller（v473_1c）
```
E8 census: 1
  0x522741 owning=FUN_005222d0 (L216747)
  context: 522745: call ptr [esi + 0x5f]   (indirect call)
```

## §3 残点 **V-474-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`elem[10]`（byte +40 float）の class**（`FUN_004CD960` list walk の machine decode 深部）＋ `self[+0xC]` 上位 byte `0xFF` の semantic（何の状態か）。
2. 【低】**旧残点 V-406-4 残り** — `form+0x69` byte の値の意味（derived AV 初期適用内で何を set するか・`[esi+0x5F]` slot 特定）。
3. 【低】**V-473-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v473_1 の1本 EXIT=0**（V-473-1 gate identity 確定・elem class は部分／V-473-2 caller1件確定＋連動相当／V-473-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00446B80` の decompiled が1行比較（`0xfeffffff < param_1`）で gate 意味を即決**（mask 比較から上位 byte 判定へ読み替え）→ (ii) **`FUN_00468270` の E8 census が1件のみ＝`FUN_005222D0` と即特定**、context の `call ptr [esi+0x5f]` で indirect 経由を確認 → (iii) elem class は machine scan の field アクセス（`+0x48/+0x50`）までで、class は未達と自覚。
- 妥当性チェック: ①`0xfeffffff < x`⇔`x >= 0xFF000000`⇔上位 byte `0xFF` — ②`FUN_00468270` caller⇔`FUN_005222D0`（v462 既知 fn）— ③`[esi+0x5F]`⇔indirect call（`esi` が vtable なら slot 23.75＝不正 → object/field 側 [推定]）— ④`lea edi,[esi+0x48]`⇔v470 list head — ⑤`mov ecx,[esi+0x50]`⇔count/field — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_00446B80`/`FUN_00468270` に annotation 実在）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **gate の1行比較（`CONST < param_1`）は「上位 byte 判定」等に読み替えられる** — mask 定数（`0xFEFFFFFF`）から条件（`>= 0xFF000000`）が即決。
  2. **caller が1件の setter は「その1件の fn」と連動関係が即決**（`FUN_00468270`⇔`FUN_005222D0`）。
  3. **`call ptr [reg+disp]`（indirect）は vtable slot 候補だが、`disp` が4で割り切れない（`0x5F`）なら object field 側を疑う**（mid-decode の可能性も併記）。
  4. **elem class特定は list walk の machine decode 深部が必要**（`lea edi,[esi+0x48]` まででは不十分＝残点の形を正直に）。
  5. **`form+0x69` byte も derived AV 初期適用（v462）と連動** — `form+0x64`（dword）と同 fn（`FUN_005222D0`）由来で揃った。
