# v464 — `FUN_006ECC80`（CLAS getter）受け手特定と band signature の裁定（V-464-1..3 完遂・旧残点 V-406-3 部分消化）

- 採番: **v464**（旧残点群消化・probe prefix **v464_1..v464_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-464-1..3（v463 残点）** — 1=旧残点 V-406-3（`FUN_006ECC80` 残り E8=500+ の CLAS 型再 typed【中】）／2=旧残点 V-406-4（`DATA+0x2C`/`+0x31` 意味）と V-406-5（`PTR_DAT_00B10D90` 所有 fn）／3=`FUN_005222D0` 一語名別法と predicate slot35 英名裏取りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-464-1（旧 V-406-3）: `FUN_006ECC80` 受け手の typed** | **[部分確定]（受け手2系統＋band signature 取得）** | **E8 caller 512件（cap 到達＝実際は更多）の band histogram と actor/chargen/menu band（`0x580000..0x5F0000`）73件を全列挙 [確定]**。**受け手2系統を特定**: **(i) `FUN_004CCE00`（L160783 `float10 __fastcall(int param_1)`: `if ((*(u8*)(param_1+0x24) & 1) != 0) { local_4 = FUN_00420C40(); }` → `param_1+0x24` byte flag bit0 が立つとき float 計算を返す [確定相当]）と `FUN_004CCE20`（L160801 `void __thiscall(int, undefined4*, undefined4*, char)`・float locals 多用＝class form の float 計算系 [推定高]）**（`FUN_005A6DE0`/`FUN_005B7180` から `mov ecx,eax; call 0x4CCE00/0x4CCE20` 経由で CLAS 結果が渡る）／**(ii) Player singleton 接続**: `FUN_005806D0`（L287569 `void __fastcall(int)`）が `call 0x6ECC80` 直後に **`mov ecx,[0xB333C4]`（Player singleton・v447 確定）** [確定(機械)] → **CLAS getter 結果を Player と接続する menu/処理 fn**。**band signature 一覧を取得 [確定(decompiled)]**: `FUN_005806D0` `void __fastcall(int)`／`FUN_005A6DE0` `void __fastcall(int)`／**`FUN_005B7180` `void(float *param_1, float *param_2)`**／`FUN_005B7550` `int *__fastcall(int)`／top-band: **`FUN_00603CA0` `void __thiscall(int *param_1, float param_2)`（0x603000）**／`FUN_00460710` `void(undefined4, uint)`（0x460000）／`FUN_006384B0` `void __thiscall(int *param_1, int *param_2)`（0x638000）／`FUN_006411E0` `void __thiscall(int *param_1, int *param_2)`（0x641000）／`FUN_004B72C0` `undefined1(int, int*, char, char)`（0x4B7000）→ **500+ 全数 typed は未達 [部分確定]＝残点継続**（受け手2系統＋band signature 一覧が typed の基盤） |
| **V-464-2（旧 V-406-4/5）** | **[未確定]（本 probe 未着手＝残点）** | `DATA+0x2C`（form+0x64）／`DATA+0x31`（form+0x69）の意味、`PTR_DAT_00B10D90` の所有 fn と spec 名前テーブル書手は本 probe で未調査（budget は V-406-3 に集中） |
| **V-464-3: `FUN_005222D0` 一語名別法と predicate slot35 英名** | **[未確定]（残点継続）** | v463 の残点を引き継ぐ（別法＝caller script opcode 名接続／`CanResolveDependencies` [推定] の裏取り） |

## §2 証拠（v464_1/v464_2）

### §2.1 E8 census と band（v464_1）
```
FUN_006ECC80 E8 callers: 512 (cap reached; actual more)
owning-fn band histogram (top): 0x603000:14 / 0x460000:13 / 0x638000:12 /
  0x641000:12 / 0x649000:12 / 0x5EB000:11 / 0x635000:11 / 0x4B7000:10 / ...
callers in 0x580000..0x5F0000 (actor/chargen/menu): 73
  FUN_005806D0: call 0x6ECC80 / mov ecx,[0xB333C4] (Player singleton)
  FUN_00596CF0: call 0x6ECC80 / push eax / call 0x52e7e0 (v406 chargen review)
  FUN_005A6DE0: call 0x6ECC80 / mov ecx,eax / call 0x4cce00
  FUN_005B7180: call 0x6ECC80 / mov ecx,eax / call 0x4cce20
  FUN_005B7550: call 0x6ECC80
class hints (vtable store in first 0x120): none in these fns (menu/chargen, not ctors)
```

### §2.2 受け手 identity（v464_2a/b）
```
FUN_004CCE00 (L160783): float10 __fastcall(int param_1) {
  if ((*(byte*)(param_1 + 0x24) & 1) != 0) { local_4 = (float)FUN_00420c40(); }
  return local_4; }                      // +0x24 flag bit0 -> float calc
FUN_004CCE20 (L160801): void __thiscall(int, undefined4*, undefined4*, char)
  { float locals ... }                   // class-form float calc (3-arg call)
FUN_005806D0 (L287569): void __fastcall(int)
FUN_005A6DE0 (L313422): void __fastcall(int)
FUN_005B7180 (L322603): void FUN_005b7180(float *param_1, float *param_2)
FUN_005B7550 (L322795): int *__fastcall(int)
```

### §2.3 top-band signatures（v464_2c）
```
0x603000 FUN_00603CA0: void __thiscall(int *param_1, float param_2)
0x460000 FUN_00460710: void FUN_00460710(undefined4 param_1, uint param_2)
0x638000 FUN_006384B0: void __thiscall(int *param_1, int *param_2)
0x641000 FUN_006411E0: void __thiscall(int *param_1, int *param_2)
0x4B7000 FUN_004B72C0: undefined1 FUN_004b72c0(int, int*, char, char)
```

## §3 残点 **V-465-1..3**
1. 【中】**旧残点 V-406-3 継続** — `FUN_004CCE00` の `param_1+0x24` flag bit0 の意味と `FUN_00420C40()`（float 計算）の identity、＋ top-band（`0x603000`/`0x460000`/`0x638000`/`0x641000`/`0x4B7000`）の残り caller の typed。
2. 【低】**旧残点 V-406-4／V-406-5** — `DATA+0x2C`/`+0x31` の意味（UESP 48/52byte 差分・docs 内で裏取り）／`PTR_DAT_00B10D90` 所有 fn と spec 名前テーブル書手。
3. 【低】**V-464-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v464_1..v464_2 の2本全 EXIT=0**（V-406-3 部分消化（受け手2系統＋band signature）／V-406-4/5 は未着手＝残点／V-464-3 引継ぎ）。
- **決定打の作り**: (i) **E8 census を band histogram で可視化**して typed の対象帯を特定 → (ii) **`mov ecx,eax; call <target>` の context decode で受け手 fn を特定**（`0x4CCE00/0x4CCE20`）→ (iii) **受け手 fn の decompiled で `param_1+0x24` flag byte と float 計算を確定** → (iv) **`mov ecx,[0xB333C4]`（Player singleton）の発見で Player 接続系統を分離** → (v) **band owning fn の signature 行を docs から一括取得**。
- 妥当性チェック: ①`FUN_004CCE00` の `param_1+0x24`⇔class form の flag byte（`FUN_005A6DE0` が CLAS 結果を渡す）— ②`FUN_005806D0` の `[0xB333C4]`⇔v447 Player singleton — ③`FUN_00596CF0`⇔v406 chargen レビュー（`call 0x52E7E0`＝名前取得と一致）— ④`FUN_005B7180(float*,float*)`⇔float 系 menu fn — ⑤top-band signature が float/int* 引数中心⇔CLAS の float 計算系 consumer — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_004CCE00/4CCE20` に annotation 実在）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **E8 census は band histogram を先に可視化**すると「typed の対象帯」が一目で分かる（512件を全 decode する前に帯で絞る）。
  2. **`mov ecx,eax; call <fn>` の2命令ペアが「getter 結果→受け手」の接続点** — 受け手 fn の identity が typed の鍵（本件 `FUN_004CCE00/4CCE20`）。
  3. **Player singleton（`0xB333C4`）を含む call 直後は「Player と接続する系統」として分離できる**（v447 の singleton 確定がここでも効く）。
  4. **band owning fn の signature 行を docs から一括取得**すれば、band全体の typed の材料が揃う（本件5帯分）。
  5. **`param_1+0x24 & 1` の flag byte は class form の flag（v406 `flags bit0 autocalcs` と整合）** — 過去の flags bit0 調査（v261/v406）と接続できる。
