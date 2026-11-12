# v228_process_havok_residuals4.md — process/Havok 残点第4束（V-222-1..4）

- 日付: 2026-11-01 / 作成: opencode サブエージェント（TODO serial 221 / V221・低）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v222_process_havok_residuals3.md` §9 新規残点 V-222-1..4
- 手法: capstone 5.0.7（`v172_common.py` / `v217_util.py` 再利用）＋ `cache/{named_funcs,vtable_index,call_targets}`（参照のみ）＋ `docs/Oblivion_decompiled.c`（部分読取）
- 中間物: `%TEMP%\opencode\v228_1*.py` / `v228_2*.py` / `v228_3*.py` / `v228_4*.py` / `v228_*_out.txt`
- selftest: 着手時 **4/4 PASS**（`probe_harness.py selftest`）
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v228_*` のみ。ESM 読取 0 回
- **VA 換算の罠（v222 §ヘッダ継承）**: `.rdata+.data` を連続走査する imm32 リストは .data 部分が 0x600 過小。本タスクでは最初からセクション個別変換の `imm32_refs3()` のみを使用

---

## §1 item1（V-222-1）hkp3AxisSweep vtable slot7/11/13 の用途 — **解明: sortEntities / querySingleAabb / setWorldAabb（根拠付き命名）**

### 証拠A: slot7（`0x8E61E0`）= **sortEntities**（`v228_1_out.txt` 全文）

本文（`ret` なし void、`ret` 相当の `add esp,0x14; ret`）:
1. **3バッファ確保**（スレッドローカル `fs:[0x2c]` 経由）: `(m_max+1)*16`（マーカー配列コピー用）＋ `m_max*4` × 2（ソート用インデックス配列）。
2. **マーカー配列（stride 0x10）をコピー**（`[esi+0x44]` = マーカー総数、`[esi+0x40]` = マーカー配列）。
3. **3軸ソート配列（stride 4）から `(value, index)` ペアを構築**（`[esi+0x4C]/[+0x58]/[+0x64]` = 3軸ソート配列、`[esi+0x50]/[+0x5C]/[+0x68]` = 各軸カウント）。
4. **`FUN_008E1200` = クイックソート**（`v228_1b_out.txt` 全文: partition + 再帰、pivot = 中央要素、`cmp word [esi+edi*4], cx` のワード比較）。
5. **ソート結果でマーカー配列を並べ替え、各ノードの index を更新**（`[esi+0x78]` = ノード配列、`[esi+0x48]` = ノード総数）。
6. **deleted マーカーのビットフィールドを再構築**（`[esi+0x70]` = deleted カウント、`[esi+0x74]` = ビットフィールド）。
7. **3軸ソート配列をソート結果で更新**（3回のループ、stride 4）。
8. **3バッファを解放**（スレッドローカルスタックポインタ復元）。

> Havok 4.x/2005 の `hkp3AxisSweep::sortEntities()` は「全マーカーをソートして deleted マーカーを除去し、内部データ構造を再構築する」メソッドとして実在する。3バッファ確保（一時コピー用）とクイックソートの使用が特徴。

**命名**: **`sortEntities()`**（確度中）。

### 証拠B: slot11（`0x8E14E0`）= **querySingleAabb（ハンドル指定、重複ハンドル列挙）**（`v228_1b_out.txt` 全文）

本文（`ret 8` = 2引数）:
1. **arg1 = handle**（ポインタ経由でデリファレンス: `mov eax,[eax]`）、**arg2 = 出力配列**（hkArray, stride 8）。
2. **ビットフィールドをスレッドローカル確保**（`(m_max>>5)*4 + 0x30` バイト）。
3. **`FUN_008E0E90` を呼ぶ**（slot4 = removeEntity と同じヘルパ: 3軸リストからマーカー情報をビットフィールドに展開）。
4. **ビットフィールドを走査して各ビット（=各マーカー）の AABB を比較**（`test ecx, 0x80008000` = 符号ビット比較、`sub/or/test` の2軸同時比較）。
5. **重複したら `(handle, aabb?)` を出力配列に追加**（`call FUN_008A6EE0` = 配列 resize、`inc [edi+4]` = カウント増加）。

> Havok 4.x の `hkp3AxisSweep::querySingleAabb(hkpBroadPhaseHandle* entity, hkpBroadPhaseCastCollector* collector)` は「指定ハンドルの AABB と重複する全ハンドルを列挙する」メソッドとして実在する。ビットフィールドによる重複判定が特徴。

**命名**: **`querySingleAabb()`**（確度中）。

### 証拠C: slot13（`0x8E1160`）= **setWorldAabb**（`v228_1_out.txt` 全文）

本文（`ret 0xC` = 3引数）:
1. **arg1 = 入力ベクトル（16B）**、**arg2 = 出力ベクトル（16B）**、**arg3 = 整数（handle or flag）**。
2. **入力ベクトルを `xorps` で符号反転**（`0xA965C0` = `0x80000000 x4` の符号マスク、`v228_1c_out.txt` 実測）。
3. **`call [vt+0x30]`（slot12 = updateEntity）** を呼ぶ（量子化された値を取得）。
4. **`this+0x10`（offset）を更新**（`addps` で加算）。
5. **`this+0x20`（= offset + 1/scale）を更新**（`1.0/[this+0x30]` で逆スケール計算）。
6. **出力に符号反転を戻して返す**（`xorps` で再反転）。

> Havok 4.x の `hkp3AxisSweep::setWorldAabb(const hkpAabb& aabb)` は「世界の AABB を設定し offset/scale を再計算する」メソッドとして実在する。符号反転（ワールド座標→ブロードフェーズ量子化座標への変換）と offset/scale の更新が特徴。

**vcall 呼出し元**: `call [reg+0x34]` の `.text` 全走査で **2件のみ**（`v228_1c_out.txt`）:
- `FUN_008A70F0+0x28`: **誤検出**（allocator `0xBA7D98` の vtable+0x34、`ret 1` の deleting dtor 呼出し）。
- `FUN_008AD070+0x44`: **誤検出**（別クラスの1引数呼び出し、`[esi+0x30]` の sub-object、slot13 は3引数なので不一致）。

> ⇒ **slot13 への vcall 呼出し元は実質0件**（未使用、または内部専用ヘルパ）。setWorldAabb は world 構築時にのみ呼ばれるが、vcall 経由ではなく直接 E8 呼出しの可能性もある（E8 直接呼出しは slot13 本文に0件、`v222_5c_out.txt` で確認済み）。

**命名**: **`setWorldAabb()`**（確度中、ただし vcall 呼出し元0件なので実行時未使用の可能性あり）。

### 裁定

> hkp3AxisSweep vtable `0xA9A710` の slot7/11/13 を本文の挙動から根拠付きで命名:
> - **slot7 = `sortEntities()`**（全マーカーをソートして deleted マーカーを除去、3軸ソート配列を再構築、クイックソート使用）
> - **slot11 = `querySingleAabb()`**（ハンドル指定、ビットフィールドで重複ハンドルを列挙）
> - **slot13 = `setWorldAabb()`**（符号反転 → slot12 (updateEntity) 呼出し → offset/scale 更新、vcall 呼出し元0件）
>
> v222 §5 の slot2-6/8/9 命名と合わせ、slot0-13 がすべて根拠付きで命名済み（slot1 は内部状態ダンプ、slot10=querySingleAabb は v205 確定、slot12=updateEntity は v222 確定、slot14-17 は v195/v205 確定）。

**判定: 解明（根拠付き命名）**（証拠 `v228_1_out.txt` / `v228_1b_out.txt` / `v228_1c_out.txt` / `v222_5c_out.txt` / `v222_5d_out.txt`、v222 §5 継続）。

---

## §2 item2（V-222-2）`0xA96B78` の正式名と bhkCollisionFilter との関係 — **解明: 「並立（兄弟）」で確定、正式名は `hkNullCollisionFilter` [確度中〜高]**

### 証拠A: 2 ctor は同一ソースの別 concrete（`v228_2_out.txt`）

```
FUN_0088A570 (bhkCollisionFilter ctor):          FUN_00899330 (0xA96B78 ctor):
  [+6]=1(refcount)                                 [+6]=1(refcount)
  [+8]=0xA95C60 (hkCollidableCollidableFilter)     [+8]=0xA95C60 (同一)
  [+0xC]=0xA95C3C (hkShapeCollectionFilter)        [+0xC]=0xA95C3C (同一)
  [+0x10]=0xA95C48 (hkRayShapeCollectionFilter)    [+0x10]=0xA95C48 (同一)
  [+0x14]=0xA95C68 (hkRayCollidableFilter)         [+0x14]=0xA95C68 (同一)
  concrete: [+0]=0xA95CE4 [+8]=0xA95CDC            concrete: [+0]=0xA96B78 [+8]=0xA96B64
            [+0xC]=0xA95CD0 [+0x10]=0xA95CC4                 [+0xC]=0xA96B70 [+0x10]=0xA96B68
            [+0x14]=0xA95CBC                                   [+0x14]=0xA96B64
```
- **両 ctor はいずれも他 ctor を呼ばない**（E8 呼出しゼロ、本文が mov 羅列のみ）。MSVC では派生 ctor は基底 ctor を必ず連鎖呼出しするため、**継承関係にない（同一 Havok 抽象インターフェイス束から並立で派生した兄弟クラス）**。
- 既定4 vtable（第1段）は **RTTI 版 Havok 抽象インターフェイス**（COL 照合: `0xA95C60`= `.?AVhkCollidableCollidableFilter@@` / `0xA95C3C`= `.?AVhkShapeCollectionFilter@@` / `0xA95C48`= `.?AVhkRayShapeCollectionFilter@@` / `0xA95C68`= `.?AVhkRayCollidableFilter@@`、`v228_2e_out.txt`）＝両クラスとも **hkpCollisionFilter 相当の4インターフェイス多重継承レイアウト**（+0/+8/+0xC/+0x10/+0x14 の5 vptr）を共有。

### 証拠B: 挙動差（`v228_2c_out.txt`）— allow-all vs レイヤー行列

- **bhkCollisionFilter の4フィルタ関数**（`0x8A8060`/`0x8A8090`/`0x8A80E0`/`0x8A8110`）: すべて共通コア **`FUN_008A7F70`（レイヤー/グループ判定）** を呼ぶ:
  - collidable×collidable: `[cA+0x1C]`/`[cB+0x1C]`（collisionFilterInfo）を引数に `FUN_008A7F70`。
  - shapeCollection: `+0xC` リンクを遡った先端 collidable の `+0x1C` 同士で判定。
  - ray 系: collidable の vtable `+0x2C`（形状取得）経由＋`+0x20`/`+0x24` のレイヤーで判定。
- **`0xA96B78` の4フィルタ関数**: `0x899380`/`0x899390`/`0x927A70` = **`*out = 1; ret N`（恒 全許可）**（v222 確定の再確認）。
- **deleting dtor も別実装**: bhkCollisionFilter `0x88A5E0`（`+0x10/+0xC` を既定へ戻し `[0]=hkBaseObject`、allocator `0xBA7D98` で free、サイズ word `[+4]`） vs `0xA96B78` 系 `0x8993A0`→`FUN_008DA1D0`（concrete view 復元→既定化→hkBaseObject）。**互いの dtor を呼ばない**（さらに継承不在を強化）。

### 証拠C: 正式名の根拠 — 'hkNullCollisionFilter' 文字列＋生成/配線経路（`v228_2c/2e_out.txt`）

- **`.rdata 0xA9A2BA = 'hkNullCollisionFilter'`**（他に 'hkPairwiseCollisionFilter'/'hkGroupCollisionFilter'/'hkDisableEntityCollisionFilter'/'hkCollisionFilterList'/'hkCollisionFilter' の Havok SDK フィルタ名群と連続配置。imm32 直接参照は0件＝Havok シリアライズ/トレース用の名前プール）。
- **生成/配線の2経路**:
  1. `0xA96B78` 系 ctor `FUN_00899330` の caller は `FUN_0089D340+0x23` の1件（= ahkWorld フィルタ setter、v222 確定）。ahkWorld ctor `FUN_0089A230` はこれを**インライン展開**して **world+0x78 に内蔵**（allow-all の初期既定）。
  2. **bhkCollisionFilter はシングルトン生成 `FUN_0088B070`（caller `FUN_005350F0`）**: allocator で 0x18B 確保 → ctor `FUN_0088A570` → **`[0xBA7904]` に格納**。その後 `FUN_00889A70`（ahkWorld 生成直後、caller `FUN_0088AEB0`）が **`[0xBA7904]` が非0なら setter `FUN_0089D340(world, bhkCollisionFilter)` で allow-all を差替える**（`v228_2e_out.txt` §FUN_00889A70: `cmp dword [0xba7904],0; je skip; ... push eax([0xBA7904]); call 0x89d340`）。

### 裁定

> - **`0xA96B78` クラスの正式名は `hkNullCollisionFilter`（Havok SDK 組込の全許可フィルタ）[確度中〜高]**: (a) SDK 名文字列が EXE 内に実在、(b) 挙動が「全判定 true」で唯一一致（Group/Pairwise/Disable は条件分岐あり）、(c) ahkWorld 構築時に world+0x78 へ**既定として内蔵**する配線は Havok の「world は null フィルタで始まり、ゲーム側が実フィルタに差替える」慣例と完全一致。RTTI 不在は Havok 静的ライブラリの該当 TU が RTTI 無しでコンパイルされたため（抽象インターフェイス側 0xA95Cxx は RTTI あり＝ビルド設定の混在）。
> - **bhkCollisionFilter との関係は「並立（兄弟）」で確定**: 互いの ctor/dtor を呼ばない・同一レイアウト（5 vptr + refcount + 0x18B）・同一の既定インターフェイス vtable を第1段に書く。**継承の向きは存在しない**（v222 の「系統関係 [確度高]、継承の向きは[残]」への回答: どちら向きでもない）。
> - **実行時の配線**: ahkWorld は構築時に hkNullCollisionFilter（+0x78 内蔵）→ 直後にシングルトン `bhkCollisionFilter`（`[0xBA7904]`、レイヤー行列 `FUN_008A7F70` 実装）へ差替え（`FUN_00889A70`）。null フィルタ本体は setter 経由でのみ再導入可能。

**判定: 解明（正式名推定つき・並立確定）**（証拠 `v228_2_out.txt` / `v228_2b_out.txt` / `v228_2c_out.txt` / `v228_2d_out.txt` / `v228_2e_out.txt`、v222 §6 継続）。

---

## §3 item3（V-222-3）`FUN_00667BE0` 喚起元18 vcall サイトの受信クラス照合 — **解明（＋v161 前提を訂正: 「18件＝全て Player 喚起」は誤り、slot83 は多重オーバーロード）**

### 証拠A: slot83（vt+0x14C）の全クラス実装 census（`v228_3_out.txt`、vtable_index 全クラス×実 vtable 読出し）

| impl | 意味（本文から） | 受信クラス |
|---|---|---|
| `0x667BE0` | **PlayerCharacter のみ**: 1st person ブロック再構築（+0x5C8/+0x5CC 生成、Show1stPerson 強制→`0x659F30`→復元、v161 確定） | PlayerCharacter |
| `0x659F30` | **Get3D 系**: `call 0x4E4080`（REFR 版）→ 戻りを process `vt+0x470` へ → IsActor ガード → 戻り値を NiNode* で返す | Actor / Character / Creature / MobileObject |
| `0x4E4080` | REFR 版 Get3D（本体、NiNode* 返却） | TESObjectREFR |
| `0x69FD40` | MagicProjectile 版（`ret` 0引数） | MagicBall/Bolt/Fog/Spray/MagicProjectile |
| `0x607710` | Arrow 版（`call 0x659F30`＋NiNode 検証） | ArrowProjectile |
| `0x629280` | **`mov [ecx+0x1E], arg; ret 4`（1引数セッタ）** | HighProcess / MiddleHighProcess / MiddleLowProcess / LowProcess |
| `0x68F970` | no-op `ret 4` | BaseProcess |
| `0x4A9830` | **`fld [ecx+0x74]; ret`（float ゲッタ）** | TESCombatStyle |
| `0x4A9810` | `fld [ecx+0x64]; ret` | TESObjectTREE |
| `0x625B50` | `fld [0xB35670]; ret`（グローバル既定値） | DefaultCombatStyle |
| `0x629410` | `mov [ecx+0x44], arg; ret 4` | TESGrass |
| `0x982D85`/`0x768D60` | （無関係: レンダラ帯） | NiRenderer / NiDX9Renderer |

⇒ **slot83 は「クラス階層ごとに意味が異なる多重オーバーロード・スロット」**。TESObjectREFR 系（プレイヤー含む）では **Get3D（NiNode* 返却）** が正統なスロット意味で、PlayerCharacter の `FUN_00667BE0` はその**オーバーライド**（Get3D の前に1st person アタッチブロックを確実に生成する）。

### 証拠B: vcall サイトは **20 件**（v161 の18件＋本タスクで2件追加）（`v228_3b_out.txt`）

- 走査方法: `mov r32,[reg+0x14C]`（disp32）＋直近 `call reg`。v161 の18件は全て再検出、追加2件は **`0x40F86F`（FUN_0040f760 内）** と **`0x4AAFA7`（FUN_004aa1f0 内、v161 は同関数を1件計上）**。
- `call [reg+0x14C]`（FF /2 disp32）形は **0 件**（全て「ロード→call reg」形）。

### 証拠C: 受信クラスの静的照合（`v228_3b/3c/3d_out.txt`）

| サイト | 受信オブジェクト（静的証拠） | 実際に到達する実装 | 判定 |
|---|---|---|---|
| `0x5F6526`（FUN_005f6330） | `ecx=[esi+0x58]`（＝Actor の process）＋`push 0` | **process `0x629280`（`[+0x1E]=0`）** | Player でない |
| `0x67CE01`（FUN_0067cdb0） | `ecx=[ebp+0x58]`（process、ebp は g_thePlayer と比較される Actor）＋`push 0` | **process `0x629280`** | Player でない |
| `0x4AAF83`（FUN_004aa1f0） | `ecx=0xB35788`（**グローバル戦闘スタイル**、setter `FUN_004A98C0` 系が書く） | **TESCombatStyle/Default 系 float ゲッタ** | Player でない |
| `0x4AC2E4`/`0x4AC2F4`（FUN_004abe50） | `call eax` 直後 `fcomp`＝**float 返却** | float ゲッタ系（`0x4A9830` 等） | Player でない |
| `0x560B66`（FUN_00560ac0） | `call edx; fstp [esp+0x14]`＝float 返却 | float ゲッタ系 | Player でない |
| `0x6228A2`/`0x62296D`（FUN_00622820） | `ecx=call 0x5E0F50`（`[esi+0x3c]` のアクセサ）＋`fcomp` | float ゲッタ系（戦闘スタイル） | Player でない |
| `0x69F224`（FUN_0069f1e0） | `ecx=esi`（MagicProjectile 帯） | `0x69FD40`（MagicProjectile 版） | Player でない |
| `0x43AFC3`/`0x4421B5`/`0x44EC0E`/`0x4DF47C`/`0x7D6B2A` | `ecx`=ローカル（ポインタ返却・0引数） | **REFR 系 Get3D**（`0x4E4080`/`0x659F30`/`0x667BE0` の多重） | **Player の可能性は実行時のみ** |
| `0x646166`/`0x64A128`/`0x64C133`/`0x659762`/`0x4AAFA7`/`0x40F86F` | `push` 付き（1〜4引数） | `ret 4` セッタ系（process/Grass 等）または規約不明（`0x40F86F` は this オンスタック形で [残]） | Player でない（引数形が Get3D 系と不一致） |

### 裁定

> - **v161 の「slot83 vcall 18件＝FUN_00667BE0 の喚起元」は否定**: slot83 はクラス多重オーバーロードであり、vcall サイト（実測20件、v161 から+2）の受信クラスは**プロセス・戦闘スタイル・MagicProjectile・木・草等に分散**。少なくとも **8 サイトは Player でないことが静的に証明**（process セッタ2・戦闘スタイル float ゲッタ6）。
> - **`FUN_00667BE0` へ到達し得るのは「ポインタ返却・0引数の Get3D 系 vcall のうち、実行時受信オブジェクトが PlayerCharacter である5サイトのみ」**（`0x43AFC3`/`0x4421B5`/`0x44EC0E`/`0x4DF47C`/`0x7D6B2A`）。静的解析では受信クラスを確定できない（同一 vcall が他の REFR/Actor にも届くため）。
> - **命名の副産物**: TESObjectREFR 系 slot83 = **Get3D**（NiNode* 返却、`0x4E4080`）。Actor 版 `0x659F30` は Get3D＋process `vt+0x470` 連動。Player 版 `FUN_00667BE0` は Get3D の1st person ブロック確保つきオーバーライド。process 系 slot83 = `[+0x1E]` セッタ（別物）。TESCombatStyle 系 = float ゲッタ（+0x74 = 戦闘技能値）。
> - 追加2件のうち `0x40F86F`（FUN_0040f760）は this がスタック経由の特殊形で受信クラス未確定 [残]。

**判定: 解明（前提訂正つき、1サイトは[残]）**（証拠 `v228_3_out.txt` / `v228_3b_out.txt` / `v228_3c_out.txt` / `v228_3d_out.txt`、`v161_quest_extra_residuals.md` §2-13 継続）。

---

## §4 item4（V-222-4）[0xB33E9C]（フレーム実時間 delta）の構造体経由書手 — **解明: `FUN_0047D170`（タイマークラスメソッド、GetTickCount 差分の秒化）**

### 証拠A: タイマー構造体の実体は `0xB33E90` ベース（v222 の「0xB33E80 帯」を訂正）（`v228_4b/4c/4d_out.txt`）

- `FUN_0047D170(this=0xB33E90, arg)` の本文から逆算したレイアウト:

| this+ | VA | 型 | 用途 |
|---|---|---|---|
| +0x00 | 0xB33E90 | byte | **ポーズカウンタ**（非0の間、delta は0に強制） |
| +0x04 | 0xB33E94 | float | 最小デルタしきい値（main loop `0x40D7A3` が毎フレーム `fild(tick)/1000.0` で書く＝フレーム時間バジェット） |
| +0x08 | 0xB33E98 | float | 固定ステップ累積（fixed-step 分割時の端数） |
| **+0x0C** | **0xB33E9C** | **float** | **フレームデルタ（秒）← 対象** |
| +0x10 | 0xB33EA0 | dword | 前回 GetTickCount（差分基準。`FUN_00407C9D` が読む） |
| +0x14 | 0xB33EA4 | dword | ポーズ補正基準 GetTickCount（reset 時に現在値） |

- `0xB33E80`/`0xB33E88` は別物（getter `FUN_0043F4F0`/`FUN_0043F520` がアドレスを返すが **E8 caller 0 件**、static-init 登録のみ）。

### 証拠B: 書手 = `FUN_0047D170`（`v228_4d_out.txt` 全文）

```
FUN_0047D170(this, now=GetTickCount()):
  edx = now - [+0x14]                       ; ポーズ補正後の経過 ms
  [0xB39DB4] = [0xB39DB0]; [0xB39DB0] = edx ; グローバル履歴2本へ転記
  if [+0x00] != 0: [+0x0C] = 0.0; return    ; ポーズ中は delta 0
  delta_ms = edx - [+0x10]; if 0: delta_ms = 1      ; 0ms フレームは 1ms に
  if delta_ms >= 166.66667 (0xA3D14C): delta_ms = 166 ; スパイク上限 166ms
  [+0x10] = edx
  [+0x0C] = delta_ms × 0.001 (0xA30E40)     ; ms → 秒
  [+0x0C] ×= [0xB06704]（.data 初期値 1.0）  ; ゲーム速度倍率
  ret 4
```
- **引数 `now` の正体**: 呼出し元全3件が `call dword [0xA280D0]; push eax; mov ecx,<timer>; call 0x47D170` 形。**IAT `0xA280D0` = `kernel32!GetTickCount`**（hint/name 0xB00BC6 実測、`v228_4f_out.txt`）。
- **定数実値**（`v228_4e_out.txt`）: `0xA30E40`=0.001（ms→秒）、`0xA3D14C`=166.66667（デルタ上限 ms）、`0xB06704`=1.0（倍率初期値）、`0xA2FC78`=2^32.0（負 tick 符号補正用）。

### 証拠C: 喚起元・補助経路（`v228_4d_out.txt`）

- **毎フレームの喚起元 = メインループ `FUN_0040D4D0+0x3DB`（`0x40D8A6`）**: `GetTickCount() → FUN_0047D170(0xB33E90, now)` → 直後に `fld [0xB33E9C]; ... call FUN_00889810`（**`FUN_00889810` = Havok ワールドステップ**: `[0xB2E2E4]=delta; [0xB2E2E0] = [0xBA7910]+delta`、166.667 でクランプ＝hkpWorld step 系）と `0x40DB8D` の `fld [0xB33E9C] → FUN_004029E0`（v222 §3 確定の時計進行）の**両消費者の直前**にある。
- **ポーズ補正 = `FUN_0047D0F0`**（4 callers: `FUN_00405CE0+0x48E`/`FUN_0040D4D0+0x1602`/`FUN_004949E0+0x47E`/`FUN_004E1580+0x346`）: `[+0x00]--`、0 になった瞬間に GetTickCount() を再取得して `[+0x10]`/`[+0x14]` を補正（ポーズ時間分を差分から除外）。
- **初期化 = `FUN_009DC2A0`**（v222 のリセット点の正体）: `GetTickCount() を [+0x14] へ保存、[+0x00..+0x10] を0化`（`0x9DC2C8: mov [0xB33EA4], eax`、eax = 直前の GetTickCount 戻り）。
- **クラス性の副証**: `FUN_005AD0D0+0x257` / `FUN_005AD980+0x42` は **heap オブジェクト（`[obj+0x54]`）を this にして同じ `FUN_0047D170` を呼ぶ**（GetTickCount 経由、差分→秒化まで同一）。⇒ 本体は再利用可能なタイマークラスで、`0xB33E90` はその**シングルトンインスタンス**。

### 裁定

> **[0xB33E9C] の書手は `FUN_0047D170`（タイマークラスの update(now)）で確定**: GetTickCount（IAT 0xA280D0）差分 ms を「0→1ms に丸め・166ms 上限・ms→秒（×0.001）・倍率 [0xB06704] 掛け」して this+0x0C へ格納。毎フレームの喚起はメインループ `FUN_0040D4D0@0x40D8A6`（Havok step と時計進行の両消費者の直前）。ポーズは [+0x00] カウンタで delta=0 強制＋`FUN_0047D0F0` で基準補正、初期化は `FUN_009DC2A0`（基準 tick 保存＋0化）。v222 の「0xB33E80 帯タイマー構造体」は**実体 0xB33E90（0x18B 未満の小構造体）**へ訂正。設計は「ゲーム速度倍率を掛けた実秒デルタ」＝Bethesda の fElapsedGameTime 相当。

**判定: 解明**（証拠 `v228_4_out.txt` / `v228_4b_out.txt` / `v228_4c_out.txt` / `v228_4d_out.txt` / `v228_4e_out.txt` / `v228_4f_out.txt`、v222 §3 継続）。

---

## §5 新規残点（本タスクから起票）

| ID | 内容 | 優先 |
|---|---|---|
| **V-228-1** | hkp3AxisSweep slot13 (setWorldAabb) の vcall 呼出し元0件の解消（E8 直接呼出しの有無、または world 構築時の内部呼出経路の特定） | 低 |
| **V-228-2** | Player slot83 vcall 追加2件のうち `0x40F86F`（FUN_0040f760）の受信クラス未確定（this がスタック経由の特殊形、4引数） | 低 |
| **V-228-3** | `0xA96B78` = `hkNullCollisionFilter` の正式名を「文字列＋挙動＋配線」で推定（RTTI 無しのため決定的証明は不可）— Havok SDK のクラス名レジストリ（シリアライズ型名テーブル）の参照経路を突き合わせれば確度高にできる | 低 |

---

## §6 自己検証・証跡

- **`probe_harness.py selftest` 4/4 PASS**（着手時）。
- 内部整合（独立一致 10 件）:
  1. slot7 の3バッファ確保（`(m_max+1)*16` + `m_max*4` × 2）とクイックソート `FUN_008E1200` の使用が、Havok `sortEntities()` の実装（一時コピー＋ソート＋全配列再採番）と一致。slot7 本文の deleted マーカー再マップ（[+0x78] Markers 配列）と v222 §5 のレイアウト（+0x48 フリーリスト/+0x70 deleted カウント）が整合。
  2. slot13 の符号反転マスク `0xA965C0` = `0x80000000 x4`（`v228_1c_out.txt` 実測）と、slot12 (updateEntity) 呼出し後の offset（+0x10）/scale（+0x20 = offset + 1/[+0x30]）更新が、Havok `setWorldAabb()`（ワールド→量子化座標変換）と一致。`0xA2F948` = 1.0f。
  3. slot13 の vcall 呼出し元2件はいずれも別クラス（allocator `0xBA7D98` の deleting dtor / 1引数呼び出し）＝誤検出。slot13 への実ディスパッチは0件で、slot7/11 は21件等の実呼出しと対照的。
  4. bhkCollisionFilter と 0xA96B78 の両 ctor が**互いの ctor を呼ばない**（E8 ゼロ）＋両 deleting dtor も互いを呼ばない ＝ 継承不在（兄弟）の MSVC 的証明。第1段の既定4 vtable（0xA95C60/3C/48/68）が両者でバイト一致。
  5. 既定4 vtable の RTTI 型名（`.?AVhkCollidableCollidableFilter@@` 等4種）と v222 §6 の typedesc 照合結果が一致。
  6. `FUN_0088B070` が `[0xBA7904]` へ bhkCollisionFilter シングルトンを格納 → `FUN_00889A70`（ahkWorld 生成）が非0チェックして setter `FUN_0089D340` で差替え、の配線が v222 §6 の「ahkWorld+0x78 内蔵＋実行時 setter」と二段で整合。
  7. slot83 census（vtable_index 全クラス×実 vtable 読出し）で PlayerCharacter のみ impl=`0x667BE0`、Actor 系4クラスが `0x659F30`（`call 0x4E4080`＝REFR 版のラップ）— 上位オーバーライドの継承構造と一致。process 系 `0x629280` は `ret 4`（1引数）で、vcall サイトの `push 0` 形と一致。
  8. float 返却 vcall サイト（`fcomp` 直後6件）が TESCombatStyle `0x4A9830`（`fld [ecx+0x74]; ret`）等の float ゲッタ実装と整合。
  9. [0xB33E9C] 書手 `FUN_0047D170` の定数: `0xA30E40`=0.001（ms→秒）、`0xA3D14C`=166.66667（上限 ms）、`0xB06704`=1.0（倍率）、IAT `0xA280D0`=`kernel32!GetTickCount`（hint/name 0xB00BC6 実測）— GetTickCount ms 差分を秒化する書手として代数が閉じる。
  10. `FUN_0047D170` の heap 版利用（`FUN_005AD0D0`/`FUN_005AD980` が `[obj+0x54]` を this に同一ヘルパ呼出し）が、シングルトン `0xB33E90` が「タイマークラスの1インスタンス」であることの副証。
- 制約遵守: 新規ファイルは本 1 件＋`%TEMP%\opencode/v228_*`（スクリプト 11 本＋出力 9 本）。docs/・既存 tmp/re・GT・TODO・session-memory・plan・cache・git は編集 0 件。ESM 読取 0 回。

---

## §7 Android 移植への示唆（実装しない）

- hkp3AxisSweep の sortEntities/querySingleAabb/setWorldAabb を移植する場合、符号反転（ワールド→量子化座標変換）と offset/scale の更新を再現すること。slot13 は vcall 呼出し元0件だが、world 構築時に直接呼出される可能性があるため、E8 直接呼出しの有無を確認してから実装方針を決定すること。
- コリジョンフィルタは「実行時に差替可能な4インターフェイス集約体」として設計し、既定を allow-all（hkNullCollisionFilter 相当）にしておけば、レイヤー行列実装（bhkCollisionFilter 相当）を後から差し込む構成を再現できる。差替え API は ahkWorld+0x78 setter（`FUN_0089D340`）が参照。
- Player の Get3D（slot83）はプレイヤー専用オーバーライドで1st person アタッチブロック（+0x5C8）を惰性生成する。移植版でも「Get3D の前に1st person ブロックを確実に構築」するオーバーライド構造を再現すると、Show1stPerson 切替時の挙動が原文と一致する。
- フレームデルタは「GetTickCount 差分 → 0/1ms 丸め → 166ms 上限 → ×0.001 → ゲーム速度倍率」の5段パイプライン＋ポーズカウンタ（delta=0 強制＋基準補正）を再現すると、ポーズ中・スパイク時の挙動が原文と一致する。Havok step と時計進行（TimeScale×delta/3600）の両方が同じ delta を消費する配線も再現すること。
