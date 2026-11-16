# v421 — edi（新規 data）と stride-48 配列要素の型照合と port `keyframe_data_index` 写像の裁定（V-405-2 完遂・派生残点第13弾）

- 採番: **v421**（派生残点群・probe prefix **v421_1..v421_11 の11本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c` ＋ `rust/`（port・読み取りのみ）＋ nif_oracle
- 依頼経緯: v405 の残点 **V-405-2【中】（`slot38` 作成物・`FUN_00560920(&DAT_00b3cc5c)` registry・`global(0xB3CD7C)[+0x3c]+idx*48` の3者の同一型性、port `keyframe_data_index` への厳密な写像、`target_node_index` の供給元）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・port ソース（読み取り）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **3者の同一型性** | **[確定]（ファミリ一致）＋[確定]（クラス内訳）** | **3者とも `NiBlendInterpolator` 系ファミリ [確定]**: ①**gate（`FUN_00560920(0xB3CC5C, x)`）＝ x の descriptor chain に `0xB3CC5C`（"NiBlendInterpolator" descriptor）が含まれるか**＝is-a NiBlendInterpolator 判定 [確定] ②**stride-48 要素＝`NiBlendTransformInterpolator`（48B）[確定]**（MTTC 配列は ctor `0x6CBCB0` が vtable `0xA79E54` を設定・要素 size `push 0x30`） ③**slot38 作成物＝flag で2系統: flag=0→`NiBlendTransformInterpolator`（48B・ctor `0x6CBC60`→vtable `0xA79E54`）／flag=1→`NiBlendAccumTransformInterpolator`（88B・ctor `0x6C37D0`→vtable `0xA79584`）[確定]**。**flag＝`NiControllerManager.Cumulative`（`manager[+0x6C]`・v420 確定）[確定]**。→ **完全同クラスは (gate合格時)＝(3)＝(③flag0)＝`NiBlendTransformInterpolator`、③flag1 のみ別クラス（88B accum・ただし同ファミリ）** |
| **chosen の identity** | **[確定]（構造）＋[確定]（クラス別実装の2系統）** | **chosen ＝ 対象ノード/プロパティの controller map から `クラス名(slot1 descriptor) == CtlrType` かつ `slot35 名 == CtlrID` で選択される controller オブジェクト**（選択 walk: `node[+0x9C]` → property（`FUN_006c63a0`=+6 property_type とクラス名一致 [推定高]）または fallback → map walk（bucket `[+0xC]`・next `[+0x34]`）→ `FUN_006c63c0`=+8 CtlrType・`FUN_006c63e0`=+0xA CtlrID）。**slot 実装はクラス依存で2系統とも EXE に存在 [確定]**: **MTTC 系**（slot31=`0x6CFB80`＝`[+0x40]` target 配列を `element[+8]`（NiFixedString 名）で linear 検索→idx／0xFFFF（`word[0xA7A160]`=0xFFFF 実測）／slot32=`0x6CFC00`＝`return [+0x3C]+idx*48`／slot38=`0x6CFDE0`＝同式 pure getter（ret 0x14）／slot33=`0x60CF60`＝**`ret 8` no-op**）・**NiTransformController 系**（slot31=`0x6CE1A0`＝`xor ax,ax; ret 4`（常に0）／slot32=`0x6CE200`＝`return [+0x3c]`（単一 interpolator）／slot33=`0x6CE210`＝**NiPointer assign（refcount 管理付き setter）**／slot38=`0x6C3B70`＝**真の create**（flag?88B:48B）） |
| **v405「slot38 新規作成／slot33 attach」の訂正** | **訂正 [確定]** | **slot38/slot33 はクラス依存**: MTTC-chosen なら slot38 は **pure getter（作成ではない）**・slot33 は **no-op**（＝type gate 失敗時も同じ要素を返すだけ＝create path は実質 dead）／NiTC-chosen なら slot38 は **真の create**（Cumulative?NiBlendAccumTransformInterpolator(88B):NiBlendTransformInterpolator(48B)）・slot33 は **m_spInterpolator への NiPointer 代入**（旧 ptr release＋新 ptr retain）＝**v405 の「attach」は NiTC 時のみ実体** |
| **port `keyframe_data_index` 厳密写像** | **[確定(式)]** | **`keyframe_data_index = (entry+8 ptr − MTTC[+0x3C]) / 48`** — EXE の entry+8 は MTTC 配列要素の生ポインタで、idx（slot31 の name→target idx／epilogue の target 検索 idx）＝**port の index と同一値**（並行配列: `MTTC[+0x40][idx]`＝target・`MTTC[+0x3C]+idx*48`＝blend）。port は read 時に `block.keyframe_data_index = block.controller_index`（`rust/crates/nif-core/src/block.rs:1283`）を seed。**制約 [確定]**: /48 式は **MTTC 配列 backed の entry+8 にのみ適用可** — NiTC-chosen で create された standalone blend（heap 48/88B）はどの配列にも属さず index 復元不可 → **port の「keyframe data array の index」モデルは MTTC 経路のみを覆う [残点 V-422-3]** |
| **nif-ffi.h コメントの妥当性** | **不一致あり（要訂正）** | `nif_ffi.h:395-398`「the two fields **the original seeds while reading**: keyframe_data_index from the controller reference, resolved_bone_index at -1」— **EXE の LoadBinary（`FUN_006c8820`）は entry+8 を書かない**（書くのは `+0`（Interpolator:Ref）／`+4`（Controller:Ref）／`+0xd`（priority）のみ＝v405 確定）→ **「original が read 時に seed する」は EXE 根拠と不一致**。port の read 時 seed（=controller_index）は **port 側プレースホルダ**（StoreTargets 解決前の仮値）。`resolved_bone_index=-1` は EXE に骨解決なし（v371/v405 確定否定側）と整合 |
| **`target_node_index` の供給元** | **[確定(否定側)]** | **EXE は per-entry の target index を永続化しない** — StoreTargets が entry へ書くのは **`+4`（controller ptr＝chosen）と `+8`（blend ptr）のみ**（epilogue `0x6C6A3F` も +8 のみ）。target 検索 idx（epilogue の `MTTC[+0x40]` 線形探索）は **transient** で、**keyframe_data_index と同一値**（並行配列）→ port に `target_node_index` 相当フィールドがあればその EXE 対応値は同一 idx（entry+8 から復元可）[推定高]・**EXE 側に独立フィールドは不存在** |

## §2 証拠（v421_1..v421_11）

### §2.1 chosen 選択 walk（v421_9・linear head 0x6C9590 同期）
```
palette->slot19(node_name[+4]) -> ebp = resolved NiAVObject        (6C96E0)
node[+0x9C] list walk: payload=[edi+8]; payload->slot1 -> pair[0] vs FUN_006c63a0(entry)
   match -> [esp+0x1c] = payload                                   (6C975F)
property offset(+6)==0xFFFF -> first walk skip                     (6C96F2 je)
ebp = [esp+0x1c] (payload or node) -> 6C9852:
map walk: esi=[ebp+0xC]; entry->slot1 -> class name vs FUN_006c63c0(entry=+8 CtlrType)
          AND entry->slot35(+0x8C) name vs FUN_006c63e0(entry=+0xA CtlrID)
   both equal -> 6C9940 = chosen found                             (6C98EA je)
6C9940: name = palette[+0xc](interpolator_id) -> slot31(chosen, name) -> bp idx
        cmp bp, word[0xA7A160](=0xFFFF) -> je FAIL
        uVar7 = slot32(chosen, idx) -> FUN_00560920(0xB3CC5C, uVar7)  (type gate)
        gate!=0 -> use uVar7 (6C9AF5)
        gate==0 -> slot38(chosen, idx, 1, flag, 0.0f, 2) -> slot33(chosen, elem, idx) -> use
6C9AF5: chosen[+8] |= 0x20(word); entry+4 = NiPointer(chosen); entry+8 = edi   (0x75FA90 assign)
```
- `FUN_006c63c0`（L496004-496011）＝`*(u16*)(entry+8)` → CtlrType name [確定]／`FUN_006c63e0`（L496017-496024）＝`*(u16*)(entry+0xA)` → CtlrID [確定]／`FUN_006c63a0`＝+6 property_type [推定高（family 0x6C6380/+4,63A0/+6,63C0/+8✓,63E0/+0xA✓,6400/+0xC の並び）]。
- **`word[0xA7A160]` = 0xFFFF（dword 0x0000FFFF）実測** ＝ not-found sentinel [確定]。

### §2.2 slot 実装の2系統（v421_6/v421_8/v421_10）
- **MTTC vtable `0xA79984`**: `+0x7C=0x6CFB80` / `+0x80=0x6CFC00` / `+0x84=0x60CF60` / `+0x98=0x6CFDE0` [確定(RTTI)]。
  - `0x6CFB80`（slot31）: null→0xFFFF; count=`[ecx+0x44]`; array=`[ecx+0x40]`; `element[+8]` と strcmp → idx or 0xFFFF（**name→target idx lookup**）。
  - `0x6CFC00`（slot32）: `movzx eax,[esp+4]; lea ×3; shl 4; add [ecx+0x3c]; ret 4` ＝ **pure getter**。
  - `0x6CFDE0`（slot38）: 同式 `ret 0x14` ＝ **pure getter**。
  - `0x60CF60`（slot33）: **`ret 8` no-op**。
- **NiTransformController vtable `0xA79634`**: `+0x7C=0x6CE1A0` / `+0x80=0x6CE200` / `+0x84=0x6CE210` / `+0x98=0x6C3B70` [確定(RTTI)]。
  - `0x6CE1A0`（slot31）: `xor ax,ax; ret 4`（常に0）。
  - `0x6CE200`（slot32）: `mov eax,[ecx+0x3c]; ret 4`（単一 interpolator 取得）。
  - `0x6CE210`（slot33）: `[ebx+0x3c]` の旧 ptr を `InterlockedDecrement`→0 で dtor、新 ptr を代入→`InterlockedIncrement` ＝ **NiPointer setter**。
  - `0x6C3B70`（slot38）: `cmp byte[esp+0x1c],0; je → push 0x30 new + call 0x6CBC60` / else `push 0x58 new + call 0x6C37D0` ＝ **真の create（2系統）**。

### §2.3 create 2系統の型（v421_7/v421_8/v421_11）
- **MTTC 配列確保**: `push 0x6cbca0(dtor); push 0x6cbcb0(ctor); push 0x30; ... call 0x984bcf; mov [esi+0x3c], ebx`（0x6CFF93-0x6CFFB5／0x6D0080-0x6D00A3・2箇所）— **ctor `0x6CBCB0`＝`call 0x6cc4e0` → `mov [esi], 0xA79E54`（NiBlendTransformInterpolator）**、size **0x30=48B** [確定]。
- **create flag=0**: `push 0x30; call 0x401F00; call 0x6CBC60` → ctor `0x6CBC60`＝`call 0x6CCE40` → **`mov [esi], 0xA79E54` ＝ NiBlendTransformInterpolator（48B）** [確定]。
- **create flag=1**: `push 0x58; call 0x401F00; call 0x6C37D0` → ctor `0x6C37D0`＝`call 0x6CCE40` → **`mov [esi], 0xA79584` ＝ `.?AVNiBlendAccumTransformInterpolator@@`（88B）** ＋ `[esi+0x30..+0x38]` に globals `0xB24260/64/68` をコピー（accum 初期状態）[確定]。
- **flag の出所**: `6C9A1A: mov ecx,[eax+0x40]`（seq→manager）→ `cmp byte [ecx+0x6c],0; mov [esp+0x28],1` ＝ **flag = `NiControllerManager.Cumulative`（v420 確定の `[mgr+0x6C]`）** [確定]。
- **descriptor 階層（実読）**: `0x6CBC90: mov eax,0xB3CBF8; ret`（NiBlendTransformInterpolator の descriptor getter）／pairs: **`0xB3CBF8`={name "NiBlendTransformInterpolator"→link `0xB3CC5C`}**（thunk A02880）・**`0xB3CD1C`={name "NiBlendAccumTransformInterpolator"→link `0xB3CC5C`}**（thunk A02D00）・**`0xB3CC5C`={name "NiBlendInterpolator"→link `0xB3EB8C`}`**（thunk A02A00）→ **両 create 系とも chain に `0xB3CC5C` を含む＝gate 合格 [確定]**。

### §2.4 MTTC 配列と epilogue（v421_4/v421_5/v421_10）
- **MTTC layout [確定]**: `[+0x3C]`＝`NiBlendTransformInterpolator × m_usNumInterps`（48B×N）／`[+0x40]`＝`NiAVObject* target × N`（epilogue は `palette->Get(m_pcAVObjectName)` 結果と pointer 等値で idx 探索・slot31 は `element[+8]`＝NiFixedString 名で idx 探索）／`[+0x44]`＝**`m_usNumInterps`（u16 count）**（v419 の dump 文言と同一フィールド）— **並行配列**。
- **consumer `0x6C6A50` [確定(命令)]**: 配列A 全 entry で `entry[+8]`（blend）の **vtable slot38 を `slot38(entry[+0]=NIF Interpolator, 0.0f, priority([+0xd] or 既定), 1.0f)` で呼出し** → 戻り bool を `entry[+0xc]` へ＝**NIF interpolator を blend へ priority 付き登録**（NiBlendInterpolator の slot38＝`0x6CC5C0`＝内部24B 配列の空き探索）。
- **epilogue store**: `lea ecx,[ecx+ecx*2]; shl 4; add ecx,[esi+0x3c]; mov [eax+8],ecx`（0x6C6A34-0x6C6A3F）＝**entry+8 = MTTC[+0x3C]+idx*48**（epilogue は main path の +8 を上書きしうる）。

### §2.5 port 側（読み取り）
- `rust/crates/nif-core/src/block.rs:1283`: `block.keyframe_data_index = block.controller_index;`（read 時 seed・コメントは "the original assigns both here"）／`resolved_bone_index = -1`（:1284）。
- `rust/crates/nif-ffi/include/nif_ffi.h:395-398`: "the two fields **the original seeds while reading**: keyframe_data_index from the controller reference…" — **EXE LoadBinary は +8 を書かないため不一致（§1 訂正要）**。
- `nif_controlled_block`（44B・offset36=keyframe_data_index）に `target_node_index` フィールドは**存在しない**。

## §3 残点 **V-422-1..4**
1. 【低】`FUN_006c63a0`（+6 property_type 読み）の body 未取得（family 並びからの [推定高]）と first walk の payload＝property オブジェクト説の最終裏取り。
2. 【低】fallback walk の container 謎: property 未マッチ時に **NiAVObject `node[+0xC]`** を bucket として walk するが、NiAVObject の `[+0xC]` は flags 帯のはず（Gamebryo layout）— `[+0xC]` が list 先頭を指すこのエンジン固有 layout の特定、および **epilogue の `Bip01[+0x1C]`（→[+0xC]）との関係**。
3. **【中→低】port モデルの制約**: `keyframe_data_index = (ptr−base)/48` は MTTC 配列 backed の entry+8 のみ有効。**NiTC-chosen で create された standalone blend（heap 48/88B）は index 復元不可** → port の single-index モデルがカバーしない経路の扱い（#82 依存の port 実装方針で確認要）。
4. 【低】NiBlendAccumTransformInterpolator の `[+0x30..+0x38]`初期値 globals（`0xB24260/64/68`）の中身と accum 挙動への影響。

## §4 検証・自己批評
- probe: **v421_1..v421_11 の11本全 EXIT=0**（V-405-2 完遂）。
- **決定打の作り**: (i) `FUN_00560920` の decompiled 実読（chain walk）→ (ii) `0xB3CC5C` の name 実読（"NiBlendInterpolator"）＋link 先 pairs の thunk 実読で **gate＝is-a 判定と階層を確定** → (iii) **mangled TD 名（`.?AV…@@`）で vtable を RTTI 解決し slot4本の fn を両候補クラスで比較**（MTTC=pure getter 系 vs NiTC=create/setter 系）→ (iv) **create の `push 0x30/0x58` と ctor の vtable store 実読で2系統の型を確定**、flag の出所（`[mgr+0x6C]`=Cumulative）を machine で接続 → (v) MTTC 配列確保（ctor=0x6CBCB0・size 0x30）と consumer の slot38 呼出しで **配列要素＝blend＝consumer 受け手** を閉環 → (vi) port 側 seed と EXE LoadBinary の書き込み範囲を突合。
- 妥当性チェック: ①gate の chain（NiBlendTransform/AccumTransform→NiBlendInterpolator）⇔create ctor の vtable⇔MTTC 配列 ctor — ②slot31 の `element[+8]` 名比較⇔epilogue の pointer 等値比較（同一 `[+0x40]` 配列）⇔`m_usNumInterps` count（v419） — ③flag=Cumulative⇔Cumulative 時に accum blend（論理整合）— ④port seed⇔EXE LoadBinary 書込範囲（+0/+4/+0xd）— **四方向矛盾なし**。
- docs との不整合: **①v405 ruling の「slot38＝新規 data 作成」「slot33＝attach」はクラス依存に訂正**（MTTC では getter/no-op）**②`nif_ffi.h` の "the original seeds while reading" は EXE LoadBinary 根拠と不一致（port プレースホルダとして再表現すべき）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **同一 virtual（slot番号）の実装はクラスごとに全く違う** — 「slot38＝create」と「slot38＝pure getter」が同一バイナリに併存する。**virtual の identity は「どのクラスの vtable か」で決まる**ので、chosen の class を確定せずに slot 意味を断定しない（v405 の訂正事例）。
  2. **mangled RTTI 名（`.?AVFoo@@`）で TD→COL→vtable を辿る解決手順**（`Foo\0` 部分一致では TD に当たらない＝v421_4/5 の失敗事例）。
  3. **`push size`＋ctor の vtable store を読めば「配列要素/生成物の型」と sizeof が同時に決まる**（0x30→NiBlendTransformInterpolator・0x58→NiBlendAccumTransformInterpolator）。
  4. **flag の値の出所（manager Cumulative）まで遡ると「なぜ2系統あるのか」の設計理由まで確定できる**。
  5. **port コメントの「original はこうする」は EXE の書込範囲と必ず突合する**（本件 +8 は read 時に書かれない）。
