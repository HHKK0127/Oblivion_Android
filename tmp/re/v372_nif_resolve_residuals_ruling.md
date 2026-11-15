# v372 — NIF 参照解決の残点2件（V-371-2 clip 構築元 / V-371-3 骨解決経路）の裁定（#45 N5b ブロッカー）

- 採番: **v372**（#45 N5b の #145 残点消化、probe prefix v372_1..v372_5）
- 対象: Oblivion.exe v1.2.0416 ＋ port 実測（rust/crates/nif-core/src/block.rs）
- 依頼経緯: #145（v371）で `resolveControllerReferences` の正体＝`NiControllerSequence::StoreTargets`(0x6C9590) を確定したが、残点 **V-371-2（clip 構築元・NiKeyframeData 配列対応）／V-371-3（resolved_bone_index 経路）** が #45 N5b の③④に必須として残存 → 本 v372 で消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令または原本アセットのみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| V-371-3 骨解決経路 | **[確定(命令)]** | StoreTargets ループ終了後に **`root->FindChild('Bip02')`→`FindChild('Bip01')`→その子 `[+0x1C]`** を取り、**`0x6C6910(owner, Bip01子)`（E8 callers=1＝StoreTargets 専用）** が registry `0xB3CD7C` の**ポインタ配列 `[+0x40]`（count `[+0x44]`）と block 名を照合 → 一致 index cx なら `配列A[i].field8 = [+0x3C] + cx*48`**（0xFFFF なら skip）。消費側 `0x6C6A50` は field8 の `virtual+0x98(block,0.0,1.0,flag)` 呼び出し → 結果 byte を `field+0xC` へ |
| V-371-2 clip 構築元 | **[確定(否定側)]＋[部分確定]** | StoreTargets は **clip を構築しない**（成功パスの `0x6FFE90`＝SEH＋初期化/接続系で clip 構築の形跡なし・`'clip'` 文字列は XML 系のみ＝AnimationClip 名の EXE 文字列不在）。**port の `clip` は EXE に1対1の対応オブジェクトを持たない**＝port 側の再構成。EXE 側の「解決結果」は **field8（オブジェクトポインタ）＋field+0xC（byte）** の2つ [部分確定] |
| NiKeyframeData 配列対応 | **[部分確定]** | 対応表＝registry `0xB3CD7C` 由来の **`[obj+0x3C]`=ベース／`[obj+0x40]`=ポインタ配列／`[obj+0x44]`=count**（名前→オブジェクト）。**48B(0x30) stride**。これが keyframe data 群か骨群かは未切り分け＝残点 V-372-1 |

**#45 N5b への回答**: ③（resolved_bone_index）＝**EXE は骨を「index」ではなく「ポインタ（base+index*48）」で field8 に保持し、未解決＝NULL**。port の `-1` シード＋index は port 表現だが、解決の**正典アルゴリズム（名前→FindChild→registry 照合→base+idx*48）は本裁定で確定**。④（配列対応）＝**block 名（u16 offset）→root FindChild→registry ポインタ配列の線形照合＝名前ベース対応**（添字順ではない）[確定(命令)]。

---

## §2 証拠

### §2.1 骨解決（V-371-3）— StoreTargets ループ後のバンド（v372_3(a)・unfiltered）

```
6C9B54: cmp byte [esp+0x38], 0          ; special-case フラグ（Bip 処理済みか）
6C9B59: mov esi, [esp+0x10]             ; esi = root node（前処理の解決結果）
6C9B5D: jne 0x6C9B71                    ; 処理済み → Bip01 側へ
6C9B5F..6C9B6B: root->virtual+0x4C('Bip02'(0xA79CD0)) ; ★Bip02 FindChild
6C9B6D: test eax,eax; je 0x6C9B94       ; 見つからず → 終了（何もしない）
6C9B71..6C9B7D: root->virtual+0x4C('Bip01'(0xA30968)) ; ★Bip01 FindChild
6C9B7F: test eax,eax; je 0x6C9B94
6C9B83: mov eax, [eax+0x1c]             ; ★Bip01 の子 [0x1C]（NiNode 子ノード）
6C9B86: test eax,eax; je 0x6C9B94
6C9B8A: mov ecx, [esp+0xC]              ; ecx = owner（seq/manager）
6C9B8E: push eax
6C9B8F: call 0x6C6910                   ; ★★骨解決本体（E8 callers = 1 本＝本 call のみ）
6C9B94: pop edi; mov al,1; pop esi; add esp,0x2C; ret 4   ; StoreTargets 終了
```
- **`0xA79CD0` = 'Bip02'**（v372_3(b) 実読）・`0xA30968` = 'Bip01'。**Bip02 → Bip01 の順で root から骨ルートを FindChild**（v371 の D special-case と同一経路の末尾）。
- `0x6C6910` の **E8 callers＝`0x6C9B8F` の1本のみ**（v372_4(b)）＝ StoreTargets 専用ヘルパ [確定(命令)]。

### §2.2 `0x6C6910` 本体（v372_4(a)・unfiltered 全 decode）

```
6C6937: mov edi, ecx                    ; edi = owner
6C6939: mov ecx, [esp+0x2C]             ; arg1 = Bip01 子ノード
6C693D: push 0xB3CD7C                   ; ★registry（v348 attach パイプラインと同一）
6C6942: call 0x700010                   ; registry walk → esi = 対象オブジェクト
...
6C6969: cmp [edi+0xC], ebx(0)           ; count = [owner+0xC]（block 数）
6C696C: mov ecx, [edi+0x40]             ; [owner+0x40]
6C696F: mov ebp, [ecx+0x7C]             ; ★root node（v371 前処理の [seq+0x40]→[+0x7C]）
ループ（i<count・stride16 = ebx += 0x10）:
6C6984: mov eax, [edi+0x18]             ; ★配列B（[owner+0x18] = palette 記録配列）
6C6987: movzx ecx, word [eax+ebx+4]     ; u16 field+4（node 名 offset）
6C698E: cmp cx, 0xFFFF → NULL
6C69A3..6C69AC: root->virtual+0x4C(name) ; ★FindChild（node 名）
6C69B0: je skip                          ; 見つからず → 次
6C69B2: movzx edx, word [esi+0x44]      ; ★registry obj の count
6C69BD: mov esi, [esi+0x40]             ; ★registry obj のポインタ配列
6C69C0..6C69CE: for cx<count: cmp [esi+cx*4], eax → je 0x6C6A21（一致）
6C69D4: i++ / 次へ
6C6A21: cmp cx, [0xA7A160](=0xFFFF)     ; 一致 index が sentinel なら skip
6C6A2A: mov eax, [edi+0x14]             ; ★配列A（[owner+0x14] = block 構造配列）
6C6A31: movzx ecx, cx
6C6A34: lea ecx, [ecx+ecx*2]            ; cx * 3
6C6A37: add eax, ebx                    ; &配列A[i]（ebx = i*16）
6C6A39: shl ecx, 4                      ; * 16 → cx * 48 (0x30)
6C6A3C: add ecx, [esi+0x3C]             ; ★base = [registry obj + 0x3C]
6C6A3F: mov [eax+8], ecx                ; ★★配列A[i].field8 = base + index*48
```
- **決定打**: `base + index*48`（48 = 0x30 stride）を **block[i] の field8（+8）へ格納**。未解決時は store が飛ぶ（＝**NULL のまま**）。
- registry オブジェクトの3フィールド: **`[+0x3C]`=ベース配列／`[+0x40]`=ポインタ配列／`[+0x44]`=count**（名前→オブジェクト対応表）[確定(命令)]。
- **`0xB3CD7C`** は v348 §6 の attach パイプライン（型名照合→NiTransformController 構築）と**同一 registry** [確定]。

### §2.3 field8 の格納先・消費側の統合（配列A = stride16 の field 構造）

| site | 命令 | 意味 |
|---|---|---|
| `0x6C9B12`（StoreTargets 成功パス） | `mov [esi+eax+8], edi`（esi=i*16・eax=[owner+0x14]） | **field8 = 解決済みノード/オブジェクト** [確定(命令)] |
| `0x6C6A3F`（0x6C6910） | `mov [eax+8], ecx`（eax=&配列A[i]） | **field8 = 骨/keyframe オブジェクト（base+idx*48）** [確定(命令)] |
| `0x6C6A50`（消費・callers=0x6C9BEA/0x6C9D70/0x6C9E55） | `ecx=[esi+8]` → `virtual+0x98(block, 0.0, 1.0, flag)` → `[esi+0xC]=al` | field8 の vtable を叩き byte 結果を field+0xC へ [確定(命令)] |
| `0x6C6AC0`（近接 fn） | `[eax+8]!=0` → `virtual+0x9C(byte [eax+0xC])` | field+0xC の消費 [確定(命令)] |

→ **port 対応**: `target_node_index`（u32・-1シード）⇔ EXE の **field8（ポインタ・NULLシード）**。解決アルゴリズム（名前ベース）は同一 [確定]。

### §2.4 clip（V-371-2）— [確定(否定側)]＋[部分確定]

- StoreTargets 成功パスの `0x6FFE90`（v372_1(a) unfiltered）＝`push -1; push 0x9C9360`（SEH）＋`mov [esi+0x34],0` ×2＋`call 0x75FA90/0x6C61E0/0x7016A0`（IAT/初期化ヘルパ）＝**clip 構築の形跡なし（オブジェクト初期化/接続系）** [確定(否定側)]。
- `'clip'` 文字列は XML 系（`clipwindow`/`clips`/`ClipDist`）のみで **AnimationClip 名の EXE 文字列不在** [確定(否定側)]。NiTextKeyExtraData ctor `0x6D7780` の **E8 callers=0**（virtual のみ）[確定(否定側)]。
- → **port `AnimationClip`（name/duration/frequency/keyframes）は EXE に1対1対応オブジェクトを持たない**＝port 側の再構成（block.rs コメントの "the original assigns both here" は port 解釈）。EXE 側の実体は **field8（解決済み参照）＋field+0xC（byte）** [部分確定]。
- **NiKeyframeData 配列対応**: registry の名前→オブジェクト対応表（`[+0x3C]/[+0x40]/[+0x44]`・48B stride）＝**名前ベースで対応付け**（block 添字順ではない）[確定(命令)]。対象が keyframe data 群か骨群かの切り分けは残点 V-372-1。

### §2.5 その他の実測（補足）

- `0x6F94A0` = **name→node FindChild ヘルパ**（`test esi,esi; je NULL` → `push 0xB3CAC0; call 0x700010` registry 経由）・E8 callers=40（v372_3(c)/v372_4(c)）。
- `0x478070`（head `0x478010`）: `push 'Bip01'; push ebp; call 0x6F94A0; mov [ebx],eax` = **skinned mesh 系の Bip01 検索→骨テーブル初期化**（StoreTargets とは別経路）[部分確定]。
- `'skeleton.'(0xA3C8DC)` の .text imm xref = **0件** [確定負]。
- ループヘッダ（v372_4(d)）: `[esp+0x24]`=i（素のインデックス）・`shl eax,4`→i*16・`[owner+0x14]+i*16`＝配列A、`[owner+0x18]+i*16`＝配列B（v371 の stride16・2配列と整合）。

## §3 残点 **V-372-1..3**
1. **registry `0xB3CD7C` の正体と 48B stride オブジェクトの種別**（NiKeyframeManager の keyframe data 群か、NiSkinnedMesh の骨オブジェクト群か。`[+0x3C]/[+0x40]/[+0x44]` の初期化元 decode で切り分け可）【高・#45 N5b ④の最終確定に必要】
2. **`0x6C9BA0` 帯 fn 群（0x6C6A50 の他 callers `0x6C9D70`/`0x6C9E55`）の所属**（NiControllerManager の activate/cleanup 系 [推定]）【中】
3. **`[esp+0x38]` special-case フラグの設定元**（Bip02→Bip01 の分岐条件。`0x6C95CE: mov [esp+0x38],0` の他書き手）【中】

## §4 検証・自己批評
- probe: **v372_1..v372_5 全 EXIT=0**（SyntaxError/TypeError なし）。
- **決定打の作り**: (i) StoreTargets ループ後の epilogue を unfiltered で取得 → (ii) `push 'Bip01'` xref census で `0x6C9B77` を検出 → (iii) `0x6C6910` を E8 callers=1 で同定 → (iv) 全 decode で `base+index*48` 格納を直接確認（推測ゼロ）。
- 妥当性チェック: ①`0x6C6910` の stride16/2配列/[owner+0xC] が StoreTargets のループ構造と完全一致 ②`0xA7A160`=0xFFFF sentinel（v371 実測）と 0x6C6A21 の比較が一致 ③field8 格納2サイト（0x6C9B12/0x6C6A3F）が同一スロット（+8）— **三方向で矛盾なし**。
- docs との不整合: **なし**（本裁定は新規。v371 の V-371-2/3 を解消し、v371 本文の「③④=[未確定]」は本 v372 で上書き）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし（rust/AGENTS.md の gate は未変更のため未実行）。
