# v398 — slot6（vtable +0x18）の正式名 Clone vs Copy の裁定（#153 ⑥-4・V-385-2）

- 採番: **v398**（ai-todo #153 束の第9弾＝低10件の第4弾、probe prefix v398_1..v398_5）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v385 の残点 **V-385-2（slot6 正式名 Clone vs Copy）** を消化（NiMultiTargetTransformController slot6(+0x18) = `0x6D02E0`）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **slot6 の正式名** | **[確定(命令＋文字列)]** | **Clone**。三方面の一致: ①**操作形状＝alloc＋ctor＋copy＋return new**（2クラスで独立確認: NiMultiTargetTransformController `FUN_006d02e0`＝`new(0x48)`→ctor `FUN_006d04e0`→vtable `0xA79984`→3フィールド0→`FUN_006d0170(new, ctx)`→**`return new`**（L503905-503920）／NiControllerManager `FUN_006c5cc0`＝**`new(0x80)`→ctor `FUN_006c5520`（v395 の manager ctor）→copy `FUN_006c5a10(new, ctx)`→`return`**（L495545-495554））②**要素レベルの使用も fresh object 返却**（v394: `dst_elem.obj = src_elem.obj->vtable[6](ctx)`＋refcount 遷移）③**EXE 自身の用語が "Clone"**（ログ `"BackgroundCloneThread"`＝L44053 の thread ctor・文字列 `0xA37090`／コンソール変数 **`iAnimationClonePerLoop:BackgroundLoad`**（`0xA3CB88`）＝アニメーションをバックグラウンドで反復 Clone する仕組み） |
| **Copy か？** | **[確定負]** | **Copy 形状は本バイナリに別物として存在**: `FUN_006cd3d0`（v394）＝**this=source／stack1=dest（既存オブジェクト）／戻り void** の dest 書込型。slot6 は **new を確保して返す**＝API 規約上 Copy ではなく Clone。**slot6＝Copy は構造的に矛盾** |
| 適用範囲 | **[確定(観察)]** | 上記は **NiObject 系**（NiControllerManager／NiMultiTargetTransformController／interpolator 要素）の slot6 に適用。**vtable の +0x18 は階層ごとに独立**（例: `FUN_00419b30` 帯の +0x18 は int を返し `==7` 比較＝別系統・L16532）— **全クラス共通の名前ではない** |
| BackgroundCloneThread との接続 | **[推定高]** | thread ctor（`FUN_00439c10`）が `"BackgroundCloneThread"` をログし vptr `0xA37088`（**2スロット: {`0x43EEB0`, `0x43CC20`}**＋直後に文字列）を設定。**thread→slot6 の直接 call chain は virtual dispatch のため未追跡**（thread 家族の行範囲に `+0x18))` 呼び出し0件）＝残点 |

**V-385-2 への回答**: slot6 の正式名は **Clone [確定]**（alloc＋copy＋return new の操作が EXE 自身の "Clone" 用語と一致）／**Copy は [確定負]**（dest 書込・void の別形状）。

---

## §2 証拠

### §2.1 MTCtrl slot6（v385 再掲＋v398_1(c) decompiled L503888-503921）

```
L503890: undefined4 * FUN_006d02e0(undefined4 param_1)   ; this(hidden ecx) + ctx
L503905:   puVar2 = FUN_00401f00(0x48)                    ; ★new(0x48)
L503911:   FUN_006d04e0(); *puVar2 = &PTR_FUN_00a79984; 3 fields = 0   ; ctor
L503918:   FUN_006d0170(puVar2, param_1)                  ; copy(new, ctx)（hidden this=source）
L503920:   return puVar2                                  ; ★return new
```

### §2.2 manager slot6（v398_2(b) decompiled L495527-495555）

```
L495545:   iVar2 = FUN_00401f00(0x80)                     ; ★new(0x80)＝manager size（v395 ctor [+0x7c] まで）
L495549:   if (iVar2) uVar3 = FUN_006c5520();             ; ★v395 の NiControllerManager ctor
L495552:   FUN_006c5a10(uVar3, param_1)                   ; copy(new, ctx)
L495554:   return uVar3                                   ; ★return new
```
- **2つの独立クラスで同一形状**（alloc→ctor→copy→return）＝slot6 は「新規生成して複製を返す」仮想。

### §2.3 要素レベル使用（v394 再掲）

- `dst_elem.obj = src_elem.obj->vtable[6](ctx)`（refcount: 旧 release＋新 InterlockedIncrement）＝**戻りを fresh object として格納**。

### §2.4 Copy 形状との対照（v394 再掲）

- `FUN_006cd3d0`（v394）: **this=source／stack1=既存 dest／`ret 8`／戻り void**＝**dest 書込型 Copy**。slot6（new 返却型）と構造が明確に異なる。

### §2.5 EXE 自身の "Clone" 用語（v398_2(d)/v398_3）

- 文字列: **`BackgroundCloneThread`**（`0xA37090`・thread ctor L44053 `FUN_00430e00(3,"BackgroundCloneThread")` でログ）／**`iAnimationClonePerLoop:BackgroundLoad`**（`0xA3CB88`・コンソール変数）。
- thread ctor `FUN_00439c10`（L44037-44068）: vptr `&PTR_FUN_00a37088`＋`new(0x1c)`→`FUN_00438870(param_2, 8)`（8要素のキュー系・`FUN_00438870` def＝L42880）。
- **thread vtable 実体（file 0x636288）**: `{0x43EEB0, 0x43CC20}` の2スロット＋直後に `BackgroundCloneThread` 文字列（＝vptr とクラス名が隣接する小規模構造）。`FUN_0043CC20` body 94 行に `+0x18`/`new` 直呼び0件＝**thread→slot6 の直接接続は未追跡 [推定高]**。

### §2.6 適用範囲の注意（v398_1(b)）

- decompiled の `+0x18))` 呼び出しは系統ごとに多様（`FUN_00419b30` は int 戻りで `==7` 比較＝**別階層の slot6**）。**NiObject 系のみが Clone 形状**（vtable は階層ごとに独立）。

## §3 残点 **V-398-1..3**
1. **BackgroundCloneThread（vtable `{0x43EEB0, 0x43CC20}`）の run 経路と slot6 の直接接続の追跡**（virtual dispatch 未達・thread が clone をどの fn 経由で実行するか）【低】
2. **隣接 slot（slot7 等）に CloneRecursive 相当が別立しているか**（NiObject 系の Clone/CloneRecursive スロット対の確認）【低】
3. **`FUN_006c5a10`（manager copy）と `FUN_006d0170`（MTCtrl copy）の関係**（同一テンプレートのインスタンスか・中身の完全比較）【低】

## §4 検証・自己批評
- probe: **v398_1..v398_5 全 EXIT=0**。
- **決定打の作り**: (i) 2クラスの slot6 decompiled で alloc→ctor→copy→return を逐語確認 → (ii) v394 の要素レベル fresh 返却と接続 → (iii) **Copy 形状（`FUN_006cd3d0`）との構造対照** → (iv) **EXE 文字列 "Clone"（thread ログ＋コンソール変数）** → (v) 適用範囲（階層依存 slot）の確認。
- 妥当性チェック: ①2クラスの slot6 形状一致 — ②`new(0x80)`⇔v395 manager size — ③`new(0x48)`⇔v385 MTCtrl size — ④Copy 形状の別立⇔slot6 の new 返却 — ⑤"Clone" 文字列⇔アニメ背景処理 — **五方向で矛盾なし**。
- docs との不整合: **なし**（v385 の V-385-2 を解消）。
- **教訓**: **"正式名"はバイナリにメソッド名として存在しないため、①操作形状（alloc 返却 vs dest 書込）②EXE 自身のログ/コンソール変数用語 の二点で裁定する**（RTTI はクラス名のみ・メソッド名なし）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
