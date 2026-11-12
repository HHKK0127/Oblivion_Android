# v243_process_havok_residuals7.md — process/Havok 残点第7束（V-236-7）

- 日付: 2026-11-02 / 作成: opencode サブエージェント（TODO serial 236 / V236・低）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v240_process_havok_residuals6.md` — §4 新規残点6件
- 手法: capstone 5.0.7（`v172_common.py` / `v217_util.py` 再利用）＋ `tmp/re/cache/{named_funcs,vtable_index,call_targets}`（参照のみ）
- 中間物: `%TEMP%\opencode\v243_1.py`〜`v243_2b.py`（各 `_out.txt` 付き）
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v243_*` のみ。ESM 読取 0 回
- selftest: `probe_harness.py selftest` **4/4 PASS**（着手時）
- **VA 換算**: `imm32_refs3()`（セクション個別 raw、v234/v240 罠注記遵守）
- 外部参照: Web 1件のみ（Havok `hkClass.h` 実物ヘッダ、GitHub `nitaigao/engine-showcase` `etc/vendor/havok/Source/Common/Base/Reflection/hkClass.h`＝Havok 2009 BUILD#20090216）。他の全ては EXE 内訳拠。

---

## §1 item1（V-236-7-1）descriptor `+0xC`（thunk arg5）の意味 — **解明: `m_numImplementedInterfaces`（実装インターフェース数）で確定。version/flag 仮説は棄却**

### 証拠A: hkClass 自己記述 member 配列が `+0xC` を名前付きで宣言（`v243_1_out.txt` §A）

`'hkClass'` の arg8 member 配列 `0xA9EFC8`（7件・stride 0x14 = `{name, type, enum, w3=型語, w4=(offset<<16)}`）を全件復元:

| # | member 名 | offset（w4>>16） | 型語 w3 |
|---|---|---|---|
| 0 | `name` | 0x00 | 0x00000214 |
| 1 | `parent`（type=0xBA8788= hkClass 自身） | 0x04 | 0x00001914 |
| 2 | `objectSize` | 0x08 | 0x00000007 |
| 3 | **`numImplementedInterfaces`** | **0x0C** | 0x00000007 |
| 4 | `declaredEnums`（type=0xBA8764） | 0x10 | 0x0000191A |
| 5 | `declaredMembers`（type=0xBA871C） | 0x18 | 0x0000191A |
| 6 | `defaults` | 0x20 | 0x00001413 |

- offset 列は ctor の写像（arg1→+0x00, arg2→+0x04, arg3→+0x08, **arg5→+0x0C**, arg6→+0x10, arg8→+0x18, **arg10→+0x20**）と**全項目一致**。
- `objectSize` と `numImplementedInterfaces` の型語がともに 7（整数型）で、ポインタ系（0x13/0x14/0x1A）と明確に別。
- w4 は上位16bit= offset・下位16bit=0（`parent`=0x00040000, `objectSize`=0x00080000, `numImplementedInterfaces`=0x000C0000, `defaults`=0x00200000）で9クラス分のオフセット完全整合。

### 証拠B: ctor 本体の再確認（`v243_1b_out.txt`）

`FUN_0090D190`（`ret 0x28`）:

```
0x90D1A3  mov edx, [esp+0x14]   ; arg5
0x90D1AE  mov [eax+0xC],  edx   ; ← arg5 → +0x0C
...
0x90D1C6  mov ecx, [esp+0x28]   ; arg10
0x90D1CD  mov [eax+0x20], ecx   ; ← arg10 → +0x20
```

`[esp+0x10]`（arg4）への読みは**全関数内で0回**（v240 の判定を再確認）。

### 証拠C: thunk 全150件の arg5 分布（`v243_1_out.txt` §C — サンプル9件から全数へ）

- **0 : 145件 / 1 : 4件 / 4 : 1件**（他の値は存在しない）
- 非ゼロ5クラス全列挙: `hkPairwiseCollisionFilter`=1, `hkDisableEntityCollisionFilter`=1, `hkRayShapeCollectionFilter`=1, `hkBaseObject`=1, `hkCollisionFilter`=4
- v240 の観測例（hkCollisionFilter=4 / hkBaseObject=1）を全数で再確認・矛盾なし。
- arg4（配列 ptr 候補）は**全150件 0**。

### 証拠D: 上流知識（hkClass.h 実物ヘッダ・Web 1件）

- ctor 引数順: `(className, parentClass, objectSizeInBytes, implementedInterfaces, numImplementedInterfaces, declaredEnums, numDeclaredEnums, declaredMembers, numDeclaredMembers, defaults, attributes=HK_NULL, flags=0, version=0)` — **引数5 = numImplementedInterfaces** で binary の arg5→+0xC と一致。
- ヘッダのフィールド宣言順も `m_name, m_parent, m_objectSize, [m_implementedInterfaces はコメントアウト: ＝"//const hkClass** m_implementedInterfaces;" ], m_numImplementedInterfaces, m_declaredEnums, ...` — **ptr メンバが消えて count メンバだけ残る非対称**が binary で arg4 が保存されず arg5 が保存される現象と一致。
- `flags` は ctor **引数12**・フィールド `m_flags` は+0x28、`version` は引数13・`m_describedVersion` は+0x2C。本ビルドの hkClass は stride **0x24（9 dword）で +0x20 で終端** → **flag/version フィールドがレイアウト上存在しない**。

### 裁定

> - **`+0xC`（arg5）= `m_numImplementedInterfaces`（このクラスが実装するインターフェース数）で確定**（確度: 高）。hkClass 自己記述の member 名・オフセット・ctor 引数順の3点が収束。
> - **v240 §4-1 の「version/flag 候補」は棄却**: flag/version は本ビルドの hkClass レイアウト（0x24）に存在せず、値4も `FlagValues{FLAGS_NONE=0, FLAGS_NOT_SERIALIZABLE=1}` と矛盾。
> - 値の実態: 5クラスのみ非ゼロ（コリジョンフィルタ族4件＋hkBaseObject）。arg4（インターフェース配列 ptr）は保存されず全件0 → 「count のみ残る」は hkClass.h のコメントアウト状態と整合。
> - 補足（未解明要素）: count としての **live 読手は未検出**。既知 descriptor 消費17系を走査（`v243_1b_out.txt`）し、`+0xC` アクセスの大半は stack（`[esp+0xC]`）・別構造体（`FUN_0090E860` の ebp は +0x48/+0x50 まで持つ serializer 系オブジェクトで hkClass 0x24 と不一致）の誤検出と判定。→ 実運用では write-only 的残存計数の可能性（確度: 中）。

**判定: 解明（フィールド同定）＋ live 読手のみ未解明**（証拠 `v243_1_out.txt` / `v243_1b_out.txt` / `v243_1c.py` 出力、Web hkClass.h 1件）。

---

## §2 item2（V-236-7-2）descriptor `+0x20`（thunk arg10）の意味 — **解明: `m_defaults`（既定値ブロック ptr）。全150スロット0＝予備（reserved）を負証拠強化**

### 証拠

1. **全数0**（`v243_1_out.txt` §C）: arg10 の分布 = **0 : 150件／他0件**。v240 の「全サンプル0」（9件相当）を **thunk 全150件に拡張**。
2. **フィールド名の確定**（`v243_1_out.txt` §A）: hkClass 自己記述 member[6] = **`defaults`、offset 0x20**（型語 0x1413＝ポインタ系）。ctor 引数10 もヘッダでは `const void* defaults = HK_NULL` で完全一致。
3. **ヘッダの `//+nosave` 注記**: `m_defaults` はシリアライズ対象外（`const void* m_defaults; //+nosave`）＝ packfile に載らない予備スロットという上流の位置づけ。
4. **読手未検出**（`v243_1b_out.txt`）: descriptor 消費関数群の `+0x20` アクセスはすべて stack（`[esp+0x20]`）または他構造体。hkClass +0x20 の読手は0件。writer は `FUN_00940D90` の `mov [esi+0x20], ebx`（ebx=0 のクリア形）のみで、これも hkClass ctor 以外の別オブジェクトの可能性（ebp/esi が 0x48+ を持つため）。

### 裁定

> - **`+0x20`（arg10）= `m_defaults`（クラス既定値ブロックのポインタ）で確定**（確度: 高）。全150スロットが0＝**全クラスで defaults 未設定（HK_NULL）**、読手も0件 → 「**reserved/未使用**」という v240 の負証拠仮説を**全数＋フィールド名＋上流注記の3方向で強化**。
> - 本ビルドの hkClass は +0x20 が終端（stride 0x24）であり、後続の `m_attributes/m_flags/m_describedVersion`（+0x24 以降）は存在しない。

**判定: 解明（m_defaults＝予備0スロット）**（証拠 `v243_1_out.txt` / `v243_1b_out.txt`、Web hkClass.h 1件）。

---

## §3 item3（V-236-7-3）登録 thunk 150件と list2（94件）の差分56の正体 — **解明: list2＝トップレベル型レジストリ94件、残56は「member 型スロットのみ参照の抽象基底・メンバ構造体型」で別リスト不在**

### 証拠A: 完全 partition（`v243_2_out.txt`）

- `FUN_0090D190` の E8 thunk = **150件、this は全て相異なる150アドレス**（重複0）。
- `list2 @0xA9AA48` = **94件**、`list2 ⊂ thunk this`（**list2 に thunk 無しの項目 = 0**）。
- thunk this のうち list2 **不在 = ちょうど56件**（150 = 94 + 56）。差分56は「enum 系」ではなく **hkClass ctor thunk の未登録分そのもの**（enum は bss this を持たない静的 rdata 記述子で、この thunk 群には含まれない）。

### 証拠B: 56件の DATA 参照は100% member 型スロット（`v243_2b_out.txt`）

- 56件の全 DATA 参照 = **60箇所すべて**が member 構造体（stride 0x14）の **type スロット（+4）** と判定（name が有効文字列・w4>>16 が妥当 offset の構造テスト100%通過）。
- 非 member スロット参照 = **0件**。
- 56件の DATA 参照アドレスを4バイト整列で連鎖（run）走査 → **4連以上の連続列0件＝「56件を含む別の配列/リスト」は存在しない**（list3 不存在の負証拠）。
- `.data 0xB30090/0xB302A4/0xB302CC/0xB30308/0xB30454` 等の `.data` 参照も member 配列（v240 の「member 配列は .rdata/.data 両方あり」と整合）。

### 証拠C: 56件の内容（`v243_2_out.txt` §一覧）

- **抽象基底/中間クラス**: `hkBaseObject`, `hkReferencedObject`, `hkWorldObject`, `hkShape`, `hkConvexShape`, `hkSphereRepShape`, `hkBvTreeShape`, `hkHeightFieldShape`, `hkSampledHeightFieldShape`, `hkShapeCollection`, `hkPhantom`, `hkShapePhantom`, `hkAction`, `hkUnaryAction`, `hkBinaryAction`, `hkConstraintData`, `hkConstraintMotor`, `hkMotion`, `hkMotionState`, `hkRigidMotion`, `hkEntityDeactivator`, `hkRigidBodyDeactivator`, `hkCollisionFilter`, `hkRayShapeCollectionFilter`, `hkClass`, `hkPackfileHeader`, `hkPackfileSectionHeader` 等。
- **メンバ構造体型**: `Triangle`, `Ray`, `ChildInfo`, `CodeInfo`, `CollisionPair`, `ConstraintBasisA`（5種）, `ConstraintBasisB`（5種）, `DisplayTransformPair`, `FourVectors`, `Item`, `NamedVariant`, `Sample`, `StringMap`, `Subpart`, `SubpartStorage` 等。
- いずれも「単体でトップレベル直列化されない」クラス群（親の member 配列の type ポインタ、または親候補＝他 thunk の arg2 push としてのみ参照される）。

### 裁定

> - **差分56 = list2（`__types__` レジストリ登録＝トップレベル型94件）に登録されない「member 型スロット参照のみの抽象基底クラス＋メンバ構造体型」**。enum 系ではなく、別リストも存在しない（run 0件）。
> - 150 thunk = 全 hkClass 構築 thunk、94 が list2 に登録（包含率100%・余剰0）、56 は親クラス/member 型としてのみ使われる補助型。
> - list1（76 レコード {name, factory, vtable}）と list2（94）の非対称も、この「レコード＝生成可能な具象クラス（factory あり）」「descriptor＝型情報のみ必要な補助型を含む」分類と整合する（factory write-only だった v240 §2(b) の観測とも矛盾しない）。

**判定: 解明**（証拠 `v243_2_out.txt` / `v243_2b_out.txt`、v240 §2A-3 継続）。

---

## §4 item4（V-236-7-4）'__data__' セクションのファクトリノード — **解明（前提訂正）: 専用ノードは存在しない。'__data__' は静的スロット `0xB30590` ＋ virtual slot2 経路、ノード帯（遅延シングルトン5体）とは別系**

### 証拠A: ノード帯 `0xB30560..0xB30680` 全生ダンプ＝ノード5体を確定（`v243_3_out.txt`）

| ノード | factory | out slot | factory 実体（`v243_3b_out.txt`） |
|---|---|---|---|
| 0xB30598 | 0x943210 | 0xBA94BC | alloc 0x14 → jmp 0x943070（v240 確定の型レジストリ ctor） |
| 0xB305A4 | 0x945BD0 | 0xBA94E8 | alloc 8、vt **0xAA28A4** |
| 0xB305B8 | 0x947C00 | 0xBA950C | alloc 0x1C → 0x947B00（vt **0xAA2A0C**、memset 0x7D0 のバッファ付） |
| 0xB305E8 | 0x94A5F0 | 0xBA9510 | alloc 0x14、vt **0xAA2BD0** |
| 0xB305F4 | 0x94A5F0 | 0xBA9514 | 同上（別インスタンス） |

- 全5体が `FUN_00a15870` 内の小初期化子（push ブロック @`0xA1598A`/`0xA159DA`/`0xA159F6`/`0xA15A16`/`0xA15A36` 近辺）で **head `0xBA8188` へ push**（`FUN_008BB420` の遅延実行対象）— v240 のノード構造 `{+0 factory, +4 next, +8 out}` を5体全件で確認。

### 証拠B: `0xB30590`（'__data__'）はノード構造に該当しない（`v243_3_out.txt` / `v243_5c_out.txt`）

- **書手0件**: `0xB30590` の imm32 参照は `.text 0x941191` のみ（読手）。初期化子による書込なし＝**静的定数スロット**（`[0xB30590]=0xAA21E0 '__data__'`）。
- 隣接の `0xB30594='__types__'` は読手7件（ctor `FUN_0090D430`/`FUN_0090D480`/`FUN_0090DC60+0x701`、`FUN_00940F60`×3、`FUN_00941070+0x81`）— もとセクション名用途。
- ノード判定テスト（+0 が .text factory か）で `0xB30588/8C/90/94` は**全て不該当**: `0xB30588` = list1 レコード（hkBvShape, `0xB30580`）の +8 書込スロット（初期化子が定数 ptr を書く＝record の runtime 埋め）、`0xB3058C` = float 定数（読手 `FUN_00936810` が `fld`）。`'__data__'`/`'__types__'` 文字列アドレスの直接参照は `.data` の各スロット1件のみ（v240_2f 結果を再確認）。

### 証拠C: 全5 factory はセクション名文字列を参照しない（`v243_3b_out.txt` / `v243_3c_out.txt`）

- 5本の factory 本体（`0x943210`/`0x945BD0`/`0x947C00`/`0x94A5F0`）＝allocator 呼出し＋ctor 尾 jump のみで `'__data__'`/`'__types__'` 参照なし。
- **'__data__' の実利用経路**: `0x941190`（packfile 3クラス共通 virtual slot2、後述 §5）が `mov eax,[0xB30590]` を第5引数にして **`FUN_00941070`**（`ret 0x14`、`[0xB30594]` '__types__' と `0xBA8788` hkClass desc も参照する section 登録/解決系）へ渡す。`'__types__'` は ctor 系7件が `FUN_00940EF0`（セクション名リスト挿入: `[this+0x2c]` map 查→strdup→`[this+0x20]` 配列へ追加）へ渡す。
- ⇒ **ノード帯＝遅延シングルトン・レジストリ群（型レジストリ＋別4種）、セクション名スロット＝packfile クラス群の静的定数**で、系統が分離している。v240 §4-4 の「0xB30590 と対応するノード/初期化子」は**存在しない前提の誤り**（'__types__' の0xB30598 も factory はセクション名非参照で、命名は内容=型レジストリに由来）。

### 裁定

> - **'__data__' 専用ファクトリノード・初期化子は存在しない**（負証拠3系統: スロット書手0／ノード構造非該当／全 factory でセクション文字列非参照）。'__data__' の登録経路＝静的スロット `0xB30590` → virtual slot2 `0x941190` → `FUN_00941070`（§5）。
> - 'ノード帯の残4体'の out グローバル読手も特定済み（`0xBA94E8`←FUN_00917FC0/FD0、`0xBA950C`←30箇所、`0xBA9510/14`←FUN_0094A630/0094BE00）— いずれもセクション名非参照で '__data__' 無関係。

**判定: 解明（前提訂正込み）**（証拠 `v243_3_out.txt` / `v243_3b_out.txt` / `v243_3c_out.txt` / `v243_5c_out.txt`、v240 §4-4 継続）。

---

## §5 item5（V-236-7-5）`FUN_00940D60` の受信クラス名 — **解明（機能同定）: RTTI 不在の packfile 保存/読込系3クラス（size 0x6C、base `0xAA21EC` ＋ 兄弟 `0xA9C8B4`（XML 出力）/ `0xA9C990`（3インデックスセクション））の virtual slot3**

### 証拠A: 到達は virtual のみ・v240 の直接呼出し主張は誤り（`v243_5_out.txt`）

- `e8(0x940D60)` = **0**、`imm32_refs3(0x940D60)` = **`.rdata 0xA9C8C0 / 0xA9C99C / 0xAA21F8` の3件のみ**（+0xC = slot3 位置を生ダンプで確認）。
- **v240 §2A-3「FUN_00940D60 は FUN_00942170+0x37C から呼出」は誤りを発見・訂正**: 同アドレス `0x9424EC` の call 目的地は **`FUN_00940D20`**（`0x940D56` ret4 で終端、int3 pad 後 `0x940D60` は別関数）。`FUN_00940D60` への直接経路は存在しない。

### 証拠B: 3スロットの vtable 実体とクラス階層（`v243_3c_out.txt` / `v243_5b_out.txt`）

- vtable 基底は `0xA9C8B4` / `0xA9C990` / `0xAA21EC`（slot0 @base, slot2 @+8=**0x941190**, slot3 @+0xC=**0x940D60**、slot5-7 @+0x14..+0x1C = `0x941140/0x941150/0x941170`＝`FUN_00940EF0` thunk 群を3本とも共有）。
- **base `0xAA21EC`**: ctor `FUN_00940D90`（refcnt=1、`+0x14/+0x2C/+0x38` map、`+0x20` 配列、`+0x44/+0x48=-1`、`+0x54/+0x60` 文字列、0x6C バイト埋め）、dtor `FUN_00940E30`、slot4 = `0x982D85`（`__access+0x14` = CRT `_access` 相当＝**ファイル存在確認**）。
- **兄弟A `0xA9C8B4`**: ctor `FUN_0090D430` = base ctor → vtable 設定 → **セクション名 `'__types__'`（[0xB30594]）を追加**。dtor `FUN_0090D450`。**slot4 = `FUN_0090D4D0` = XML 出力本体**（`0x90D4D0` 起点 `sub esp,0x88`、this の `[+0x20]` セクション配列を走査し `sprintf("<hksection name=\"%s\">\n")` → 末尾 `"</hksection>"`→`"</hkpackfile>"` → `0x8BC000` 書出。XML 文字列3件の参照は全てこの関数内＝`FUN_0090D480`（dtor）への fname 誤括りを訂正）。
- **兄弟B `0xA9C990`**: ctor `FUN_0090D7D0` = base ctor → vtable 設定 → **セクション名 `'__classnames__'`/`'__classindex__'`/`'__dataindex__'`（`[0xB2FF04/08/0C]`）を3件追加**。dtor `FUN_0090D810`。slot4 = `0x90EF80`、追加メソッド `0x90D840..0x90D8E0`。セクション名文字列は EXE 内で各1コピーのみ（ctor2 以外に読手なし）。
- **RTTI 不在**: 3 vtable 直前は文字列（例: `0xA9C8A8 'hkMeshShape'` — これは registration thunk 帯のみ参照＝隣接は偶然）で COL ポインタ無し。`vtable_index.tsv`（RTTI 有り1715行由来）にも3本とも**行なし** → **型名の RTTI 断定は不可**。

### 証拠C: 生成・利用経路と機能（`v243_4b_out.txt` / `v243_5b_out.txt` / `v243_6_out.txt`）

- `FUN_008BC540`（sole 生成点、`e8`=FUN_008BC660+0x4A）: **size 0x6C を allocator(tag6) 確保 → flag により `FUN_0090D430`（A, '__types__'）or `FUN_0090D7D0`（B, 3インデックス）を選択** → **slot3 = `FUN_00940D60` を vcall** → **slot4 を vcall**（引数に4バイトタグ **`'snap'`** を構築）→ bool 返却。refcount リリース＋`FUN_008BC370` 後始末。
- `FUN_008BC660` は型デスクリプタ **`0xBA8320='hkRootLevelContainer'`** と **`0xBA82D8='hkPhysicsData'`**（thunk 名前を `v243_5c` で特定）を渡す。
- 上流 `FUN_008BC6C0` ← **`FUN_00889D60`**（SEH 保護、`call [this vtable+0x58]` = **slot22 = ahkWorld getter（v240 §1C と完全一致）**＋空 hkString ＋'snap' 系引数、sole caller = `FUN_00444340+0x489`（'Running Cell Test' 文字列））。
- `FUN_00940D60` 本体の機能（`v243_1b_out.txt`）: `ecx=this; [0xBA94BC]` 型レジストリ → `call [reg+0x10]`（slot2, arg3）→ 結果と引数を **`call [edi+8]`（= this の slot2 = `0x941190` = '__data__' 経路）**へ引き渡し → `ret 0xC`。**「型レジストリ照合 → packfile セクション機構へ橋渡しする virtual メソッド」**。

### 裁定

> - **受信クラス＝Havok packfile 保存/読込系の3クラス（size 0x6C）**: base `0xAA21EC`（file アクセス slot4）／ 兄弟A `0xA9C8B4`（**XML packfile 書出**を slot4 に持ちセクション '__types__'）／ 兄弟B `0xA9C990`（**セクション `__classnames__`/`__classindex__`/`__dataindex__`** を登録）。
> - `FUN_00940D60` = 3クラス共通 **virtual slot3**（唯一の到達）。v240 の「packfile reader 系」推定は**妥当と裏付け**。ただし RTTI（COL）が無く vtable_index 行も無い → **SDK 正式名の確定は不可**（確度: 中）、機能命名とする。
> - v240 の「FUN_00942170 から直接呼出」は **FUN_00940D20 への call との取り違えと訂正**（同所 e8 目的地を生確認）。

**判定: 解明（機能同定。SDK 正式名のみ未解明）**（証拠 `v243_1b_out.txt` / `v243_3c_out.txt` / `v243_4_out.txt` / `v243_4b_out.txt` / `v243_5_out.txt` / `v243_5b_out.txt` / `v243_5c_out.txt` / `v243_6_out.txt`）。

---

## §6 item6（V-236-7-6）上流文脈名 `FUN_004820CF` / `FUN_0040D4D0` — **解明（役割名確定・うち1件は v240 チェーンの誤括りを訂正）**

### (a) `FUN_004820CF` — **実体: fn@`0x482090`「セル移動時 Havok ワールド原点同期ハンドラ」の中の `FUN_004CCCC0` 呼出し**（`v243_6_out.txt` / `v243_6f_out.txt`）

- `FUN_004820CF` は**関数先頭でなく中間アドレス**（named_funcs は `FUN_00482050+0x7F` と誤括り。実際は pad＋thunk(`0x482080`) 境界を挟んだ **新関数 fn@`0x482090`** の末尾 call）。以降の命名は fn@`0x482090` に対するもの。
- 本体: 引数セル座標 `(x,y)` と `this+4/+8`（前回セル）の差を `this->vtable+0x14` へ通知（差==0 なら偽を返す＝**移動検知**）→ 移動ありなら **`(x<<12, y<<12, 0.0f)` を float ベクトル `this+0x14..+0x1C` に構築**（`shl 0xC` = ×4096 ＝ Oblivion セル座標→ワールド座標）→ **`push &vec; call FUN_004CCCC0`**（内部 `ecx=[0xB35C24]` bhkWorldM シングルトン → `FUN_004CA2F0` → `FUN_008A7D50` → `FUN_0088C600` ＝ v240 §1D の原点シフト）→ `this+0x20=0`、真を返す（`ret 8`）。
- 到達: `e8=0`/`e9=0`、`imm32_refs3(0x482090)` = **`.rdata 0xA3D7AC` のみ**。周辺は関数ポインタ列（`0x483570/0x482080/0x481E20/0x4821E0/0x482090/0x481E60/0x482200…`）＋文字列 **`'uGridToLoad:General'`**（Oblivion.ini `[General] uGridToLoad`）が直隣 → **ini 設定/グリッド読込系のコールバック表に登録された間接到達ハンドラ**（確度: 中）。
- 役割名: **「セル（グリッド）移動を検知して bhkWorldM シングルトンへワールド原点（cell×4096）を同期する設定通知コールバック」**。

### (b) `FUN_0040D4D0` — **読込/状態遷移ステップの共通ステートマシン・ポンプ**（`v243_6_out.txt` / `v243_6b_out.txt` / `v243_6d_out.txt` / decompiled L8522-）

- 実スパン = **`0x40D4D0..0x40DF73`**（`v243_6d` の int3-gap 走査で pad は `0x40DF74..0x40DF80` の1本のみ）。void、0x10C frame、stack cookie、`ebp`=this（decompiled の `void(void)` は ecx 引数見落とし）。
- 中身: 状態カウンタ **`[0xB33418]` 0→10** のステップ機械。`FUN_00572E30(2)` 状態テスト成功時: step1=`FUN_00440AF0(0,0,0)`+`FUN_00674500`、step10=`FUN_00572EC0(2,0)`＋`[0xB33B00+0x18]` フラグ操作＋`FUN_00466B70`＋refcount オブジェクト `[0xB333E8]` のリリース（InterlockedDecrement＋vcall dtor）。**自己呼び出し `0x40DF54`**（`push 5; FUN_00434020` … `mov ecx,ebp; call 0x40D4D0`）。
- 呼出し **5件**: E8×4（`FUN_004455E0+0xD3`（Gates 付き読込）、`FUN_0044E720+0x404`（`_sprintf("Loading \"%s\" for %s \"%s\".")` 直後＝フォーム読込、`PTR_s_Activator` 型名表）、`FUN_0050D4C0+0xA7`、`FUN_005BDCD0+0x83`（ディスプレイモード/レンダ初期化帯））＋**E9×1（`FUN_00440C40+0x38`）** — v240 の「5呼び出し」は件数維持（v240 は E9 を未走査、本バッチで補完）。
- 役割名: **「オブジェクト読込時に呼ばれる状態遷移ステップ（loading pump step）」**（callers の Loading 文脈と状態カウンタ機械から、確度: 中〜高）。

### (c) **v240 §2A-2 到達チェーンの誤括りを訂正**

- v240 は「`FUN_0040D4D0`（+0x165C = `0x40EB2C`）→ `FUN_00406360`」としたが、**`FUN_0040D4D0` は `0x40DF73` で終端**。`0x40EB2C`（'Initializing Sky...' を呼ぶ `FUN_00406360` の唯一 e8）、および `'\\My Games\\'`/`'Oblivion\\'`/`'_default.ini'`/`'CopyProtectionStrings'`/`'Default Pool 8..200'`/`'BSFadeNode Pool'`/`'Loading initial area...'` の文字列群（`0x40E08C..0x40EBB0`）は**直後の別関数 fn@`0x40DF80`**（`sub esp,0x6A8`、cookie、sole e8 = `0x98780E`（named_funcs: `_qsort+0x47E`、`push (0x400000, 0, eax, 0xA)`））の物。named_funcs が境界未分割のため fname が `FUN_0040d4d0+0xAB0/+0x165C` と誤括りした。
- **Havok 初期化到達チェーンの冒頭は fn@`0x40DF80`**（→ `FUN_00406360` → `FUN_00441970` → `FUN_005350F0` → `FUN_0088B070` once → …）。
- 役割名: **fn@`0x40DF80` = 「起動オーケストレータ（ini パス構築・コピープロテクション文字列・Gamebryo メモリプール生成・初期エリア読込・Sky/Havok 初期化を含む boot sequence 本体）」**（確度: 中〜高）。decompiled.c は `FUN_0040d4d0`（L8522）の次ヘッダが `FUN_0040f640`（L8663）で `0x40DF80..0x40F63F` は未デコンパイル（docs 側ギャップ）。

### 裁定

> - `FUN_004820CF` = **fn@0x482090（＝設定通知コールバック表 0xA3D7AC 登録）の「セル移動→bhkWorldM 原点同期」実行部**。実体関数の新規命名と、named_funcs の誤括りを明示。
> - `FUN_0040D4D0` = **読込/状態遷移のステートマシン・ポンプ（5呼出し: E8×4＋E9×1）**。
> - v240 の「Havok 初期化の起点」は **隣接 fn@0x40DF80 へ移動**（FUN_0040D4D0 から直接 `FUN_00406360` への経路は無い）— v240 §2A-2 および §4-6 の記述を訂正。

**判定: 解明（役割名確定＋v240 チェーン訂正）**（証拠 `v243_6_out.txt` / `v243_6b_out.txt` / `v243_6c_out.txt` / `v243_6d_out.txt` / `v243_6e_out.txt` / `v243_6f_out.txt`、`docs/Oblivion_decompiled.c` L8522/L50029/L53511/L60044 参照のみ）。

---

## §7 自己検証（内部整合・制約遵守）

- **§1/§2**: member 配列の offset（w4>>16 = 0/4/8/0xC/0x10/0x18/0x20）と ctor の写像（`[esp+4..0x28]→[eax..+0x20]`、`[esp+0x10]` 読取0回）を独立2系統で照合。arg5/arg10 分布は thunk **全150件**（`e8(0x90D190)`＝150・this 重複0）で取得し、v240 のサンプル観測（hkCollisionFilter=4 / hkBaseObject=1 / 全0）を再現。外部知識は hkClass.h 1件のみで、ctor 引数順・フィールド順（`m_implementedInterfaces` コメントアウト状態）が binary と一致することを確認。
- **§3**: 150 thunk this と list2 94 の集合演算は完全 partition（包含100%・余剰0）。56件の DATA 参照60件は member 型スロット判定（name 文字列＋offset 妥当性＋下位16bit=0）を100%通過、4バイト整列 run 走査で連続列0＝別リスト不存在。`imm32_refs3` はセクション個別 raw（v234 罠注記遵守）。
- **§4**: ノード判定＝「+0 が .text factory / +4 が list head push / +8 が out slot」の3条件で5体。'__data__' スロットは書手0・読手1を `.text/.rdata/.data` 全走査で確認。`0xB30588`（record+8 書込）と `0xB3058C`（float、`fld` 読手）の誤ノード判定を生ダンプで除外。
- **§5**: `FUN_00940D60` の到達は e8=0＋imm32=3 で virtual-only を確定。v240 の直接呼出し主張は `0x9424EC` の call 目的地（`FUN_00940D20`）を disasm で直接確認して訂正。vtable slot 位置は生ダンプ（slot0=dtor 形・+8=0x941190・+0xC=0x940D60）で3本とも検証。XML 文字列の所属は参照先（`0x90D635/0x90D72D/0x90D75A`）が `0x90D4D0` の範囲内であることを確認し、`FUN_0090D480`（dtor）への fname 誤括りを訂正。
- **§6**: fn 終端は int3-pad 走査（12B pad `0x40DF74..0x40DF80`）で機械判定。E8/E9 両走査で FUN_0040D4D0 の呼出し5件（E8×4＋E9×1）を確定し、decompiled の4呼出し（L50029=E9 経由を含む）と突合。`0x98780E→0x40DF80` は call 命令列（`push 0x400000 … call`）を生確認して実物と判定。
- **外部参照**: Web 1件（hkClass.h）。他は全て EXE 内訳拠＋既存 cache/decompiled（参照のみ）。
- **制約遵守**: ESM 読取0回・ソース編集0回・git/docs/GT/TODO/session-memory/plan 無編集・既存 `tmp/re/*.md` 無編集（新規は本1ファイル）・`cache/` 参照のみ。スクリプトは全て `%TEMP%\opencode\v243_*.py`（`python "<path>"`、ヒアドック・`python -c` 正規表現なし）。selftest 4/4 PASS。新規中間物: `v243_1,1b,1c,2,2b,3,3b,3c,4,4b,5,5b,5c,6,6b,6c,6d,6e,6f`（各 `_out.txt` 付き）。

## §8 新規残点

1. **[残] fn@`0x40DF80`（起動オーケストレータ）の命名確定と sole caller `0x98780E` の境界特定**（低）: fname は `_qsort+0x47E`（CRT 帯）だが `push (0x400000,0,eax,0xA)` 型の呼出しで周辺境界未確定。decompiled.c にも該当ヘッダ無し（`0x40DF80..0x40F63F` 未デコンパイル）。
2. **[残] fn@`0x482090` の登録表 `0xA3D79C..0xA3D7C0` の所有者**（低）: 関数ポインタ列＋文字列 `'uGridToLoad:General'` 隣接＝ini/グリッド設定通知系と推定だが、表基底の `.text` 参照（相対走査の可能性）未特定。上流 observer クラスの命名は未了。
3. **[残] 0x6C packfile 3クラス（`0xAA21EC`/`0xA9C8B4`/`0xA9C990`）の SDK 正式名と 'snap' タグの語義**（低）: RTTI（COL）不在・vtable_index 行なし。XML 出力（A）と3インデックスセクション（B）、`FUN_00889D60`（'Running Cell Test' 帯）経由の生成から機能命名に留まる。
4. **[残] descriptor `+0xC`（m_numImplementedInterfaces）の live 読手最終確認**（低）: 既知消費17系で未検出（write-only 傾向）。`FUN_0090E860` の `cmp [ebp+0xC]` は別構造体（serializer 系、+0x48/+0x50 を持つ）と判明済みで、残るはフィールドを持つ hkClass 本体を this とする全関数の機械走査。
5. **[残] ノード2-5 のレジストリ命名**（低）: out=`0xBA94E8`（vt 0xAA28A4, 8B, 読手2件）/ `0xBA950C`（vt 0xAA2A0C, 0x1C＋0x7D0 バッファ, 読手30件）/ `0xBA9510`・`0xBA9514`（vt 0xAA2BD0, 読手各2件）。'__data__' 無関係は確認済み（本タスク範囲外の未命名）。
6. **[訂正記録]** v240 §2A-2・§4-6 の「`FUN_0040D4D0+0x165C` → `FUN_00406360`」および「`FUN_00942170+0x37C` → `FUN_00940D60`」は本 md §5(c)・§6(c) で訂正。後続バッチでの GT/ledger 反映は本タスクの禁止範囲のため未実施。
