# v423 — resolved node `[+0xC]` / `[+0x1C]` の layout 追跡の裁定（V-423-1 部分解・派生残点第15弾）

- 採番: **v423**（派生残点群・probe prefix **v423_1..v423_7 の7本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v422 の残点 **V-423-1【低】（resolved node `[+0xC]` の layout 謎（fallback walk が NiAVObject `[+0xC]` を bucket として読む）と `Bip01[+0x1C]`（epilogue container）との関係）** を追跡（旧 V-422-2 を引き継ぎ）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`[+0x1C]`（epilogue container 指すフィールド）の書手** | **[確定]** | **`FUN_007074B0` ＝ single-line setter `this[+0x1C] = arg`**（machine `0x70750F: mov [esi+0x1c], eax; ret 4`／`0x70751B: mov [esi+0x1c], edx`）— **ctor は0クリア（`0x7084B5: mov [esi+0x1c], ebx`）** → `[+0x1C]` は **runtime attach される pointer フィールド [確定]** |
| **`[+0xC]`（fallback walk の bucket）の書手構造** | **[確定]（構造）＋[未確定]（attach 元）** | **direct store は ctor の0クリアのみ [確定(否定側)]**（Ni band 0x707000..0x713000 の `[esi+0xc]` 書手7件すべて0/ebx/ecx=0）→ **ptr は single-line setter 群経由**: `0x6C0B80`/`0x6C0BF0`/`0x6C1FC0`（`mov [ecx+0xc], eax; ret`）は **E8=0＝virtual 登録（vtable entry）**／`FUN_006c2980`（`0x6C2A06` で `[+0xC]` 設定）は **factory（E8=164＝initializer bank thunk 全体がこれで singleton を構築・`call 0x6c2980; mov [global], eax; ret` 形）** |
| **`[+0x9C]` list / `[+0x1C]` の初期化** | **[確定]** | **NiNode 系 ctor `0x708450` が `0x708491: mov [esi+0x9c], ebx` と `0x7084B5: mov [esi+0x1c], ebx`（ともに0）＋`0x7084D9: mov [esi+0x24], ecx`** — **runtime attach 3フィールドの初期化を1処理で確認**（`[+0x9C]` list・`[+0x1C]` container・`[+0x24]`） |
| **fallback walk と epilogue walk の構造一致** | **[確定]** | ともに **`{[0]=vtable（slot1＝descriptor getter）・[+0x34]=next}` の entry chain を `[container+0xC]` から辿る**（fallback: `mov esi,[ebp+0xc]` → `slot1(esi)` → `esi=[esi+0x34]`／FUN_00700010: `piVar1=[param+0xc]` → `slot1` → `piVar1=piVar1[0xd](+0x34)`）＝**同一の entry 型** |
| **node `[+0xC]` と `Bip01[+0x1C][+0xC]` が同一リストか** | **[未確定]**（残点 V-424-1） | 同一構造だが **container が異なる**（node の `[+0xC]` 直接 vs `Bip01[+0x1C]`（ptr）→ その `[+0xC]`）→ **`FUN_007074B0` の callers を辿れば `[+0x1C]` に何が入るか（map object／self／別オブジェクト）が決まり同一性が閉じる** |
| **`NiAVObject` の Gamebryo 布局との齟齬** | **[未確定]（据え置き）** | stock 布局では `[+0xC]`＝flags 帯だが、本エンジンでは **`[+0xC]`＝entry chain 先頭を指す runtime フィールド**（ctor 0・virtual setter で賦流）＝**このエンジン固有の layout 差 [推定高]**（NiAVObject ctor `0x707C80`／NiNode ctor `0x70B6A0` は `[+0xC]` を直接書かない＝sub-ctor `0x708330`/`0x708450` 帯） |

## §2 証拠（v423_1..v423_7）

### §2.1 vtable/ctor 特定（v423_1/v423_2）
- RTTI resolve: **NiNode vtable `0xA7E38C`／NiAVObject vtable `0xA7DF24`**。vptr store から ctor 候補: NiNode ctor head `0x70B6A0`（vptr store `0x70B7C3`）／NiAVObject ctor head `0x707C80`（vptr store `0x707CCD`）。
- 両 ctor 内に `[esi+disp]` の直接 store なし（初期化は sub-ctor 委ね）＝`0x707C10`（base）／`0x708330`（NiNode ctor が最初に呼ぶ）／**`0x708450`**。

### §2.2 runtime attach 3フィールド（v423_3(b)）
```
FUN_00708450（NiNode ctor から call 0x70B7A9 帯で呼ばれる sub-ctor）:
  70847A: call 0x6ffd30
  708491: mov [esi+0x9c], ebx        ; [+0x9C] list = 0
  7084B5: mov [esi+0x1c], ebx        ; [+0x1C] container = 0
  7084BC/7084C4: call 0x718a50
  7084D9: mov [esi+0x24], ecx        ; [+0x24] = ecx
```

### §2.3 書手 census（v423_3(c)/v423_6/v423_7）
- **`[esi+0xc]` direct 書手（0x707000..0x713000）7件すべて0クリア**（`0x709E7D/0x70B8C9/0x70BCE0/0x70BED7/0x70DF8E/0x70E0E6/0x70E15F`）[確定(否定側)]。
- **single-line `[+0xC]` setters**: `0x6C0B80`/`0x6C0BF0`/`0x6C1FC0`（`mov [ecx+0xc], eax; ret`）は **E8 callers=0＝virtual 登録**／**`FUN_006c2980`（`0x6C2A06`）＝factory：E8=164 件全てが initializer bank thunk（`call 0x6c2980 → mov [global], eax → ret`・globals `0xB33298`/`0xB3C234`/`0xB3C290`/`0xB3C2EC`/`0xB3C358`/`0xB3C3C4`/`0xB3C420`/`0xB3C47C`…）**＝**singleton 群（map/set 候補）を構築する起動時 factory [確定]**。
- **`[+0x1C]` heap 書手（0x700000..0x713000）**: **`FUN_007074B0`＝`0x70750F: mov [esi+0x1c], eax; ret 4`／`0x70751B: mov [esi+0x1c], edx`（single-line setter）** [確定]。他は `[esp+0x1c]`（ローカル）偽陽性が多数。
- **`[+0x34]` 書手（chain insert 候補・0x6C0000..0x713000）**: NiBlendAccum ctor 帯（`0x6C3827`＝accum 初期化・無関係）・`[esp+0x34]` 偽陽性・`0x6D423A`/`0x6D470B`/`0x6D4B67`（`[esi+0x34]=eax`＝別オブジェクトの ctor フィールド初期化）— **entry chain insert 本体は未特定＝残点**。

### §2.4 walk 構造の一致（v421/v422 と本 probe の突合）
- fallback（StoreTargets 6C9852-6C98F1）: `esi=[ebp+0xc]` → `slot1(esi)`（`[esi]`=vtable→`[vtable+4]`）→ class 名比較 → `esi=[esi+0x34]`。
- epilogue（FUN_00700010）: `piVar1=[param+0xc]` → `slot1` → `piVar1=piVar1[0xd](+0x34)`。
- → **entry 型は同一（{vtable, next@+0x34}・slot1=descriptor）[確定]**。

## §3 残点 **V-424-1..3**
1. **【低】`FUN_007074B0`（`[+0x1C]` setter）の E8 callers** → `Bip01[+0x1C]` に格納される対象（map object／self／別）→ **node `[+0xC]` との同一性（V-423-1 の核心）がここに閉じる**。
2. **【低】virtual `[+0xC]` setter 群（`0x6C0B80`/`0x6C0BF0`/`0x6C1FC0`・E8=0）の vtable 所属**（data ref → RTTI）→ どのクラスが bucket を賦流するか＋**entry chain の insert 本体（`[+0x34]` 書手）**。
3. 【低】`FUN_006c6380`（+4 node_name reader）の machine body（旧 V-423-2 引き継ぎ）。

## §4 検証・自己批評
- probe: **v423_1..v423_7 の7本全 EXIT=0**（V-423-1 は [部分確定]＝構造解明まで・同一性は残点）。
- **決定打の作り**: (i) **RTTI→vptr store→ctor 特定**（NiNode/NiAVObject）→ (ii) **ctor の sub-ctor まで辿って runtime attach 3フィールド（+0x9C/+0x1C/+0x24）の0初期化を確認** → (iii) **`[reg+disp]` direct 書手 census の否定（0クリアのみ）→ single-line setter 群へ** → (iv) **E8 census で「virtual 登録（0件）」と「factory（164件＝initializer bank）」を分離** → (v) `[+0x1C]` 書手を Ni band 限定 census で `FUN_007074B0` に特定 → (vi) walk 構造の machine 突合。
- 妥当性チェック: ①ctor0初期化⇔direct 書手0クリア⇔single-line setter 賦流 — ②fallback⇔FUN_00700010 の entry 構造完全一致（vtable/slot1/+0x34）— ③factory thunk bank⇔v419 の initializer bank と同型（`call; mov [global],eax; ret`）— **三方向矛盾なし**。
- docs との不整合: **なし**（stock Gamebryo 布局との差は「エンジン固有 layout」として記録）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **ctor が0クリアする pointer フィールド＋direct 書手が0件なら「runtime attach フィールド」**—書手は single-line setter/virtual setter 経由と切り分ける（本件 `[+0xC]`/`[+0x1C]`/`[+0x9C]`）。
  2. **E8 census が0件の single-line setter＝virtual 登録候補、164件の factory＝initializer bank** — callers 数の分布が fn の役割を語る。
  3. **`[reg+disp]` 書手 census は `[esp+disp]`（ローカル）の偽陽性を大量に出す → dest のベースレジスタ（esi/ ecx vs esp）で必ず濾す**（本 probe の v423_5/6 の両方で発生・修正してから有効化）。
  4. **byte-pattern census の context decode は「VA でなく file offset」で region を切る**（v423_5 のバグ事例: `region[VA-10:]` で全 context 欠落）。
