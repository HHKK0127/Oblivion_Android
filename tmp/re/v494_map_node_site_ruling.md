# v494 — lazy node 登録先＝hash map・4push site 完遂・`+0x18` usage 否定側の裁定（V-494-1..3 完遂・旧残点 V-406-3/V-406-4 継続第24弾）

- 採番: **v494**（旧残点群消化・probe prefix **v494_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-494-1..3（v493 残点）** — 1=lazy node 用途と bit5 最終 semantic（`FUN_00452570` identity と `FUN_0045A500` の `param_1+0x18`）／2=`0x4D0E74` site（4push 形）の引数確認／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-494-1: `FUN_00452570`（lazy node 登録先）identity** | **[確定相当]** | L63124 実読 [確定(decompiled)]: **`void __thiscall FUN_00452570(int *param_1, undefined4 param_2)`: `iVar2 = virtual[*param_1+4](param_2)`（**hash**）→ `puVar3 = *(param_1[2] + iVar2*4)`（**bucket 配列 `param_1[2]`**）→ loop: null なら `virtual+0x14()`（node alloc）＋`virtual+0xC(node,param_2,param_2)`（set key/value）→ prepend → **`param_1[3]++`（count）**、既存なら `cVar1=virtual+0x8(param_2, puVar3[1])`（キー比較）→ `virtual+0x10(node)`（既存処理）＋`virtual+0xC(node,…)` 更新 [確定(構造)]** → **`FUN_00452570`＝bucket 配列＋chain の hash map insert/update fn（`FUN_00412D30`（v485）と同型の別 impl）[確定相当]** → **`FUN_00452C20`（v493）の `uVar1=param_1[3]`（`+0xC`）は「key→node の hash map ptr」で、lazy node をこの map に登録 [確定相当]** → **`+0xC`＝key/map ptr の field map が完成**（v481/v485 と整合）→ **bit5 semantic 経路: `+0x8` bit5 →notify→ `FUN_00452C20` → map node に `|=` [確定相当]（node の flag 群としての最終用途は map の val（`node[1]`==0 gate と `*node |=`）から「未設定時の mask 集積 [推定]」＝残点） |
| **V-494-2: `0x4D0E74` site（`FUN_004CBA80` 4push 形）** | **[確定]** | **machine decode（`0x4D0E5B..0x4D0E7D`）[確定(機械)]: `mov eax,[edi]; mov edx,[eax+0x84]; push 1; push ebp; mov ecx,edi; call edx`（**virtual `+0x84`**）→ **`mov eax,[0xb35eb8]`（NorthMarker singleton ptr）→ `push 1` → `push eax` → `mov ecx,ebx` → `call 0x4CBA80`（**`FUN_004CBA80(this=ebx, NorthMarker, 1)`**）→ `test eax,eax; je 0x4d0ee6` → **`fld dword [eax + 0x28]`（戻り node の `+0x28` float 読み）**` [確定相当]** → **arg3＝`1`（v492 `0x65FECF` site と一致＝**2site とも arg3=1 [確定]**）**、**戻り値（`int*`）の `+0x28` float を `fld`＝`FUN_004D0C20`（v478 既知 large fn）が marker/registry node の float を消費 [確定相当]** → **`FUN_004CBA80` の戻り node が `+0x28` float を持つ＝v482/v485 の `+0x28` 3float 群と offset 一致**（node 側か form 側かの区別は別問題＝残点の方向を明記） |
| **V-494-1: `FUN_0045A500` の `param_1+0x18` bit semantic** | **[確定]（否定側）＋[未確定]（外部 caller）** | **owning（`FUN_0045A500`）band 内の `+0x18` usage は body の2行（v493 と同一: 下位 byte／`>>0x12` bit18）のみで**他 usage **0件 [確定(否定側)]** → **bit18/byte の意味は caller 側（`FUN_0045A500` 呼び出し元が `param_1+0x18` をどう書くか）＝ [未確定]＝残点** |
| **V-494-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v494_1）

### §2.1 `FUN_00452570`（v494_1a）
```
L63124: void __thiscall FUN_00452570(int *param_1, undefined4 param_2) {
L63132:   iVar2 = virtual[*param_1 + 4](param_2);            // hash
L63133:   puVar3 = *(param_1[2] + iVar2 * 4);                // bucket
L63135:   if (puVar3 == 0) { puVar3 = virtual[*param_1 + 0x14]();
L63137:     virtual[*param_1 + 0xc](puVar3, param_2, param_2);
L63138:     *puVar3 = bucket; bucket = puVar3; param_1[3]++; return; }
L63143:   cVar1 = virtual[*param_1 + 8](param_2, puVar3[1]);   // key compare
L63147:   virtual[*param_1 + 0x10](puVar3);                   // existing node op
L63148:   virtual[*param_1 + 0xc](puVar3, param_2, unaff_retaddr); }
=> bucket+chain hash map insert/update  [確定相当]
```

### §2.2 `0x4D0E74` site（v494_1b）
```
4D0E5B: mov eax,[edi] / mov edx,[eax+0x84] / push 1 / push ebp / mov ecx,edi / call edx   (virtual +0x84)
4D0E6A: mov eax, dword ptr [0xb35eb8]      ; NorthMarker singleton ptr
4D0E6F: push 1                              ; param_3 = 1  (matches v492 site)
4D0E71: push eax                            ; param_2 = 0xB35EB8
4D0E72: mov ecx, ebx                        ; param_1 (this) = ebx
4D0E74: call 0x4cba80
4D0E79: test eax, eax / je 0x4d0ee6
4D0E7D: fld dword ptr [eax + 0x28]          ; result node's +0x28 float
```

### §2.3 `FUN_0045A500` band（v494_1c）
```
owning = FUN_0045A500 itself (L71174)
+0x18 usages inside owning fn: body's 2 lines only (low byte / >>0x12) -> other: 0  [否定側]
=> bit semantics live with the caller side (residual)
```

## §3 残点 **V-495-1..3**
1. 【中】**map node の `*node |= mask`（`FUN_00452C20`）の最終用途**（`node[1]==0` gate と `param_1[1]` の意味・`FUN_00452C20` caller 側の mask 値）＋ **`FUN_0045A500` callers**（`param_1+0x18` を書く fn＝bit18/byte の実体）。
2. 【低】**`[node+0x28]` float と form `+0x28`（v482/v485 3float 群）の同異**（offset 一致・载体の区別）＋ `FUN_004D0C20`（v478 A1 load owner）の本文用途。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v494_1 の1本 EXIT=0**（V-494-1 登録先＝hash map を確定相当・bit semantic は caller 残点／V-494-2 4push site を machine 確定（arg3=1 一致・`[ret+0x28]` fld 発見）／V-494-1 `+0x18` usage 否定側／V-494-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00452570` decompiled が bucket＋chain insert/update**で登録先＝hash map と即決（**`FUN_00412D30`（v485）との型一致**）→ (ii) **4push site を `0x4D0E40` から boundary decode**（int3 前まで）で **`push 1; push 0xb35eb8; mov ecx,ebx`** を実読 → arg3=1 と v492 一致、**直後の `fld [eax+0x28]` が node float 消費を発見** → (iii) own band の usage 0件を否定側記録。
- 妥当性チェック: ①`FUN_00452570`⇔`FUN_00412D30`（同型 hash insert・bucket `param_1[2]`・count `param_1[3]`）— ②`push 1; push eax(0xb35eb8); mov ecx,ebx`⇔3引数 thiscall（v492 docs 表示と整合）— ③`fld [eax+0x28]`⇔戻り `int*` の float（offset 0x28＝v482/485 と一致）— ④own band usage 0⇔v493 body のみ — ⑤`virtual+0x84` call が site 直前にある＝別の slot（混同しない区別）— **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **同一構造（bucket＋chain insert）の fn が複数 impl ある**（`FUN_00412D30`／`FUN_00452570`）— **同型判定で登録先（map container）を即確定**。
  2. **site 検証は「int3 前の boundary から decode」**（v474 同様）→ **直後の命令（`fld [eax+0x28]`）まで拾うと「戻り値の用途」も同時に確定**。
  3. **arg3=1 が2 site 一致**で呼び出し契約が確定（v492→v494 で確定の反復確認）。
  4. **own band usage 0件の否定側**は「bit の意味は caller 側」と経路を固定（残点の方向を明示）。
  5. **virtual slot（`+0x84`）が同 window に混在**しても「呼んでいる slot を個別に特定」すれば混同しない。
