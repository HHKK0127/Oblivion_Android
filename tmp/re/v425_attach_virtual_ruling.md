# v425 — `FUN_0070AF00`（NiNode 系 slot33 attach virtual）の identity と `[+0x1C]` 賦流経路の裁定（V-425-1 部分解・派生残点第17弾）

- 採番: **v425**（派生残点群・probe prefix **v425_1..v425_4 の4本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v424 の残点 **V-425-1【低】（`0x70AF00` fn の identity（E8 callers・decompiled）と self-attach の実在・`[+0xAC]` container の正体）** を消化（V-423-1 の同一性問題への回答も含む）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_0070AF00` の identity** | **[確定(RTTI)]** | **NiNode 系の slot33（+0x84）attach virtual** — data refs 全13件が全て vtable slot33: **`.?AVNiNode@@`（vtable 0xA7E38C）**／SceneGraph／BSTempNodeManager／BSTempNode／BSClearZNode／BSFadeNode／BSScissorNode／NiBillboardNode／BSFaceGenNiNode／BSTreeNode／NiSortAdjustNode／NiBSPNode／ShadowSceneNode。**NiAVObject 本体の slot33 は別 fn（`0x6F43694E`）＝attach は NiNode から [確定(否定側)]** |
| **Bip01 `[+0x1C]` の賦流経路** | **[確定]** | **Bip01（NiNode）の `slot33(+0x84)` ＝ `FUN_0070AF00`** → attach: **`node[+0x1C] = obj`（refcounted・`FUN_007074B0` 経由）**、**flag==0 のときのみ** this の embedded 配列オブジェクト（`[this+0xAC]`・fields `+4=array(+0xB0)/+8=cap(+0xB4)/+0xA=count(+0xB6)/+0xE=growBy(+0BA)`＝**NiNode children と同一フィールド**）へ obj を index 登録（`FUN_004b34e0`＋grow `FUN_00523b10`）／**flag=1 は ref のみ（array append なし）** [確定] |
| **attach される obj（callers 3件）** | **[確定]** | ①`FUN_004112E0`（site `0x411601`）: **factory `FUN_0070B780` の作成物**（data refs=0＝direct factory）・flag=1 ②`FUN_004A12E0`（site `0x4A1470`）: **`[ebx+0xB0]` 配列の child[i]**（null スキップループ）・flag=1 ③`0x724700` wrapper（site `0x724710`）: **obj=arg1・flag=arg2 透過**（E8 callers=0＝virtual 経由） |
| **self-attach の実在** | **[未確定]（残点）** | 3 direct callers とも **this と obj が明示的に別**（factory/child/arg）。wrapper 経由の caller が self を渡すかは未調査 → **`Bip01[+0x1C] == Bip01` は [未確定]** |
| **V-423-1 の同一性への回答（副次）** | **[推定高]** | site2（child attach）が示す通り **`[+0x1C]` には通常「child/related node」が入る（self でない）** → **epilogue の `Bip01[+0x1C][+0xC]`（＝child の chain）と fallback の `Bip01[+0xC]`（node 自身の chain）は「別オブジェクトの同一構造リスト」[推定高]**（entry 型は v423 確定の `{vtable, next@+0x34}` で一致） |
| **`[+0xAC]` container の正体** | **[確定]** | **embedded 配列オブジェクト（NiTArray 系）**: `[this+0xAC]` を base とする view で `+4=array(+0xB0)/+8=cap(+0xB4)/+0xA=count(+0xB6)/+0xE=growBy(+0xBA)` — `add ebx,0xac` 後に `FUN_004b34e0(&this[+0xAC], idx, &val)`／grow `FUN_00523b10(u16[+0xBA]+count)`（v424 の `FUN_0070B250` と同一 fields）＝**NiNode children フィールド群と完全一致 [確定]** |

## §2 証拠（v425_1..v425_4）

### §2.1 `FUN_0070AF00` body（L549810-549840＋machine 0x70AF00..0x70AFD0）
```c
void __thiscall FUN_0070af00(this, obj, flag) {
  if (obj) {
    InterlockedIncrement(&obj->refcount);
    FUN_007074b0(this);              // ★ this[+0x1C] = obj（refcounted）
    if (flag == 0) {
      InterlockedIncrement(&obj->refcount);
      idx = u16[this+0xb6]; // count
      if (u16[this+0xb4] <= idx) FUN_00523b10(u16[this+0xba] + idx); // grow
      FUN_004b34e0(&this[+0xac], idx, &obj);                        // array 登録
    }
  }
}
```
- machine: `push1; push edi; mov ecx,ebp; call 0x70af00`（site1）／`add ebx,0xac`（0x70AF7E＝embedded base）／`lea ecx,[ebx+0xac]`（0x70AF59）。

### §2.2 slot33 判定（v425_3/v425_4）
- data refs（uncapped）**13件すべて vtable slot33(+0x84)**（RTTI: SceneGraph/BSTempNodeManager/BSTempNode/BSClearZNode/BSFadeNode/BSScissorNode/NiBillboardNode/BSFaceGenNiNode/BSTreeNode/**NiNode**/NiSortAdjustNode/NiBSPNode/ShadowSceneNode）。
- **NiNode vtable `0xA7E38C` slot33 = `0x70AF00` ✓／NiAVObject `0xA7DF24` slot33 = `0x6F43694E`（別 fn）**。
- `FUN_0070AF00` E8=3（`FUN_004112E0`/`FUN_004A12E0`/`0x724700`）・`0x724700` E8=0（virtual）・`FUN_0070B780` data refs=0（direct factory）。

### §2.3 callers の push 元（v425_2）
- site1 `0x411601`: `push 1; push edi; mov ecx,ebp` — edi＝`call 0x70b780` 結果（or xor→0）。
- site2 `0x4A1470`: `[ebx+0xB0]` 配列ループ `child=array[i]; push 1; push child; mov ecx,ebp`。
- site3 `0x724710`: `mov eax,[esp+8](arg2); mov ecx,[esp+0xc](arg1); push eax; push ecx; mov ecx,esi(this)`＝透過 wrapper。

## §3 残点 **V-426-1..2**
1. **【低】self-attach の完全否定** — `0x724700`（virtual）の実 callers が self を渡すか（＝`Bip01[+0x1C]==Bip01` の最後の可能性）。＋`FUN_0070B780` factory の作成クラス（NiNode::Create 相当か）。
2. 【低】NiAVObject `[+0xC]` の identity（旧 V-423-1 引き継ぎ）: slot33 が attach で `[+0x1C]` を書く一方、`[+0xC]` の virtual setter 群（`0x6C0B80` 等・E8=0）の vtable 所属未特定＝**どちらのフィールドが chain 先頭かの最終切り分け**。

## §4 検証・自己批評
- probe: **v425_1..v425_4 の4本全 EXIT=0**（V-425-1 は [確定]（identity・賦流経路・callers・container）＋self-attach のみ残点）。
- **決定打の作り**: (i) **`FUN_007074B0` の callers を push 元まで復元**して用途分類 → (ii) **`FUN_0070AF00` の decompiled body で attach semantics と embedded array を確定** → (iii) **callers 3件の push 元で attach 対象を特定** → (iv) **data refs の RTTI backward scan（uncapped13件）で「NiNode 系 slot33」を確定し Bip01 賦流経路を閉じる**。
- 妥当性チェック: ①slot33=FUN_0070AF00 ⇔NiNode ⇔Bip01（NiNode）— ②embedded fields（+0xB0/+0xB4/+0xB6/+0BA）⇔`FUN_0070B250` の同一 fields⇔v405 `children[+0xb0]` — ③flag=0/1⇔array append の有無⇔site2 が flag=1（children 済み）— **三方向矛盾なし**。
- docs との不整合: **なし**（v424 の残点を確定へ）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **attach/assign 系 fn は「callers の push 元の分類（factory/child/arg/self）」で対象の像が固まる**（v424 の push 分類教訓の継続）。
  2. **data refs の uncapped RTTI scan が virtual の owner を一撃で決める** — 6件で打ち切ると NiNode 本体を取りこぼす（v425_3→v425_4 の再調査事例・**cap は最後に必ず外して全数を確認する**）。
  3. **flag 引数の0/1 が「array 登録の有無」を分ける** — 同じ attach でも「新規登録」と「既存 ref のみ」が併存する。
