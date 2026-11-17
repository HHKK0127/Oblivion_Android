# v468 — id `0x4C` float の SET 経路と `FUN_00627680` needle 訂正の裁定（V-468-1..3 完遂・旧残点 V-406-3 継続第5弾）

- 採番: **v468**（旧残点群消化・probe prefix **v468_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-468-1..3（v467 残点）** — 1=旧残点 V-406-3 継続（id `0x4C` の `+0xC` float の正式な語、`FUN_004CD960` の flag と接続、`FUN_00627680` docs 確認）／2=旧残点 V-406-4（`DATA+0x2C/+0x31`）と V-406-5（`PTR_DAT_00B10D90`）／3=V-467-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-468-1: id `0x4C` float の SET 経路（`FUN_004CD960`）** | **[確定相当]** | L161432: **`void __fastcall FUN_004cd960(int *param_1)`**: `piVar8 = param_1 + 0x12`（`+0x48` list head）→ list walk 中で **`if (((*(u8*)(param_1 + 9) & 1) != 0) && (iVar7 = virtual[piVar8]+0x170(), iVar7 == DAT_00B35EB8)) { FUN_00420c60((float)piVar8[10] * (float)_DAT_00a3d360); }` [確定(構造)]** — **`param_1` が `int*` なので `param_1 + 9`（index9）＝byte offset `0x24` と一致 → `byte[class_form+0x24] & 1`（flag bit0・v465 確定の setter 系統の読手）が立つとき、リスト要素の `virtual+0x170()` が `DAT_00B35EB8` と等しければ `elem[10]`（float）× `_DAT_00A3D360`（.rdata 定数）を `FUN_00420C60`（set）で id `0x4C` float に格納 [確定相当]** → **id `0x4C` float ＝「class form の flag bit0 条件付きで計算される float 値（`elem[10] × 係数`）[推定高]**（`DAT_00B35EB8`＝比較対象の識別子・`_DAT_00A3D360`＝乗算係数の identity は [未確定]＝残点）。**`param_1[2] & 0x400`（flag bit10）と `FUN_006ECC80()`（CLAS getter）の併用 [確定(構造)]**（v467 で見た set 直後の命令が body 内の同一条件） |
| **V-468-1: `FUN_00627680` の docs 確認** | **[確定]（annotation 実在・v467 訂正）** | **`// FUN_00627680 @ 0x627680`（L392992）が FOUND [確定]** → **v467 の「signature 行 (none)」は docs needle の問題（signature が2行折り返し `undefined4 * __thiscall` / `FUN_00627680(...)` だったため1行検索で失敗）＝annotation 欠落ではない [確定]** → **v449 の `fn.lower()` バグと同型の needle 問題の再発・訂正**。signature **[確定(decompiled)]: `undefined4 * __thiscall FUN_00627680(int param_1, undefined4 *param_2, int *param_3, int *param_4, char param_5)`（5引数）** |
| **V-468-1: `FUN_004D2720` signature** | **[確定(decompiled)]** | L164873: **`void __thiscall FUN_004d2720(int *param_1, float *param_2)`**（float* 引数＝id `0x4C` float の3つ目の get consumer） |
| **V-468-2: 旧 V-406-4/5** | **[未確定]（残点継続）** | 本 probe 未着手（budget は V-468-1 に集中） |
| **V-468-3: V-467-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v468_1）

### §2.1 `FUN_004CD960` SET 経路（v468_1a）
```
L161432: void __fastcall FUN_004cd960(int *param_1) {
  piVar8 = param_1 + 0x12;                       // +0x48 list head
  while ((piVar4 = piVar8, piVar4 && (piVar4[1] || *piVar4))) {
    piVar8 = (int *)*piVar4;
    if (((uint)piVar8[2] >> 3 & 1) == 0) { virtual[piVar8]+0x6C(); FUN_004637c0(piVar8); }
    if (((*(byte*)(param_1 + 9) & 1) != 0) &&    // param_1+9 (int*) = byte +0x24
        (iVar7 = virtual[piVar8]+0x170(), iVar7 == DAT_00b35eb8)) {
      FUN_00420c60((float)piVar8[10] * (float)_DAT_00a3d360);   // SET id-0x4C float
    }
    if (((param_1[2] & 0x400U) == 0) && (piVar8 = FUN_006ecc80(), piVar8 != param_1)) { ... }
```

### §2.2 `FUN_00627680` / `FUN_004D2720`（v468_1b/c）
```
L392992: // FUN_00627680 @ 0x627680   <- FOUND (v467 "none" was needle issue)
L392996: undefined4 * __thiscall
L392997: FUN_00627680(int param_1, undefined4 *param_2, int *param_3,
                      int *param_4, char param_5)          (5 args)

L164873: void __thiscall FUN_004d2720(int *param_1, float *param_2)
```

## §3 残点 **V-469-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`DAT_00B35EB8`（`virtual+0x170()` の比較対象）と `_DAT_00A3D360`（乗算係数）の identity** → id `0x4C` float の正式な語確定へ。
2. 【低】**旧残点 V-406-4／V-406-5** — `DATA+0x2C/+0x31` 意味（UESP 差分の docs 内裏取り）／`PTR_DAT_00B10D90` 所有 fn と spec 名前テーブル書手。
3. 【低】**V-468-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v468_1 の1本 EXIT=0**（V-468-1 完遂相当（SET 経路＋needle 訂正＋signature3件）／V-468-2/3 は未着手＝残点）。
- **決定打の作り**: (i) **`param_1 + 9`（`int*` の index9）＝byte offset `0x24` という pointer arithmetic を読み替えて flag bit0 と接続** → v465 の setter 系統の読手と一致 → (ii) **`FUN_00420c60(...)` の引数式（`elem[10] × 定数`）が float の由来を語る** → (iii) **`FUN_00627680` の needle は「signature の2行折り返し」を認識して annotation 実在を確認**（v449 バグの再発を自覚的に回避）→ (iv) get/set caller の全 census（v467）と接続。
- 妥当性チェック: ①`param_1+9`⇔byte `+0x24`（`int*`＝4byte index）— ②`virtual+0x170()`⇔`DAT_00B35EB8` 比較（識別子判定）— ③`elem[10]`⇔float field（`piVar8[10]`＝+40）— ④`FUN_006ECC80()`（CLAS getter）⇔`param_1[2] & 0x400`（v467 set caller の直後命令と同一）— ⑤`FUN_00627680` annotation L392992⇔v467 band `0x627000` caller — **五方向矛盾なし**。
- docs との不整合: **①v467 ruling「`FUN_00627680` signature (none)＝annotation 欠落候補」→ annotation 実在で訂正（needle 問題）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`param_1 + N`（`int*` index）は byte offset `N×4` に読み替える** — `param_1 + 9`＝`+0x24` が flag bit0 と接続した（**pointer arithmetic の units を忘れずに**）。
  2. **SET 経路の引数式（`elem[10] × 定数`）が float の由来を直接語る** — get/set caller census から1件の setter body を読むだけで由来が分かる。
  3. **docs needle は「signature が2行に折り返す」ケースに注意** — `undefined4 * __thiscall\nFUN_xxx(...)` の形は1行検索で失敗する（**v449 `fn.lower()` バグに続く needle 類型2件目**）。
  4. **`virtual+0x170()` の戻り値と定数比較は「型/識別子判定」の定番**（`DAT_00B35EB8` が何の識別子かが float の語の鍵）。
  5. **float定数（`_DAT_00A3D360`）は .rdata に実体がある** — 値を読めば係数の意味（倍率/単位換算等）が推定できる（次 probe の鍵）。
