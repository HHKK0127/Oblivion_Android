# v466 — id `0x4C` float get/set ペア・`TESFullName` init・flag writers caller の裁定（V-466-1..3 完遂・旧残点 V-406-3 継続第3弾）

- 採番: **v466**（旧残点群消化・probe prefix **v466_1..v466_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-466-1..3（v465 残点）** — 1=旧残点 V-406-3 継続（id `0x4C` の意味、`form+0x24` flag bit0 setter 特定、top-band 残り typed）／2=旧残点 V-406-4（`DATA+0x2C/+0x31`）と V-406-5（`PTR_DAT_00B10D90`）／3=V-465-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-466-1: id `0x4C`（76）の意味** | **[確定相当]（consumer 系統）＋[未確定]（float の語）** | **`FUN_0041e210(0x4c)` の docs callers は2件とも `FUN_00420C40`/`FUN_00420C60`（get/set pair）[確定]** → **id `0x4C`＝「per-thread の float 値（`+0xC`）を持つ object」で、その唯一の consumer が float get/set ペア [確定相当]**。**`FUN_00420C60`（L23463）＝setter [確定(構造)]**: `if (param_1 == 0.0) { FUN_0041E160(0x4c); return; }`（**v==0 なら `FUN_0041E160(0x4C)`＝release/clear**）`iVar2=FUN_0041e210(0x4c); if (iVar2!=0) { *(float*)(iVar2+0xc)=param_1; return; }`（**set**）`iVar2=FUN_00401F00(0x10,uVar1); …`（**無ければ alloc 0x10 bytes**）→ **`FUN_00420C40`（get）⇔`FUN_00420C60`（set）⇔`FUN_0041E160`（release）の3点セット [確定相当]**。**`+0xC` float の語（何の値か）は [未確定]＝残点** |
| **V-466-1: `form+0x24` flag byte writers の caller identity** | **[確定]（signature）** | **E8 caller census: `FUN_004C97E0`（1行 setter）←1件 `FUN_004471D0`／`FUN_0052FB40`←1件 `FUN_0044DCF0`／`FUN_0052FE90`←1件 `FUN_0044AC20`／`FUN_004CDEC0`←E8 0件 [確定]**。各 caller signature **[確定(decompiled)]**: **`FUN_004471D0`（L54575）`int * FUN_004471d0(int param_1, undefined4 param_2, undefined4 param_3, int param_4)`（4引数）**／**`FUN_0044DCF0`（L59168）`void FUN_0044dcf0(int *****param_1, char param_2)`（5重ポインタ！）**／**`FUN_0044AC20`（L57513）`void __fastcall(int param_1)`** → **flag byte を立てる系統はこの3 fn 由来 [確定]** |
| **V-466-1: `FUN_0052FB40` の class** | **[確定]（RTTI）** | **vtable `0xA322A0` の RTTI＝`.?AVTESFullName@@`（COL `0xAB407C` sig=0・pTD `0xB03298` 実読）→ `FUN_0052FB40`＝`TESFullName` の init fn [確定]**（`param_1[6]=&PTR_FUN_00A322A0`＝`+0x18` field への TESFullName vtable store）→ **`form+0x24` flag byte の1系統は `TESFullName` init 由来 [確定相当]** |
| **V-466-1: top-band 残り typed** | **[未確定]（残点継続）** | 本 probe 未着手（budget は id 0x4C/flag writers に集中） |
| **V-466-2: 旧 V-406-4/5** | **[未確定]（残点継続）** | `DATA+0x2C/+0x31` 意味、`PTR_DAT_00B10D90` 所有 fn は未調査 |
| **V-466-3: V-465-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v466_1/v466_2）

### §2.1 id `0x4C` consumers（v466_1a + v466_2c）
```
docs: FUN_0041e210(0x4c) callers = 2, both in FUN_00420C40/60:
  L23454 (FUN_00420C40, L23447): iVar1 = FUN_0041e210(0x4c);
                                 if (iVar1) return *(float*)(iVar1+0xc); return 0;
  L23485 (FUN_00420C60, L23463): if (param_1 == 0.0) { FUN_0041e160(0x4c); return; }
                                 iVar2 = FUN_0041e210(0x4c);
                                 if (iVar2) { *(float*)(iVar2+0xc) = param_1; return; }
                                 iVar2 = FUN_00401f00(0x10, uVar1); ...  // alloc
  -> get / set / release triple for id-0x4C float  [確定相当]
```

### §2.2 flag-byte writers census（v466_1b/c）
```
FUN_004C97E0 (1-line setter): callers=1 -> FUN_004471D0 (L54575:
  int * FUN_004471d0(int, undefined4, undefined4, int))
FUN_004CDEC0: callers=0
FUN_0052FB40: callers=1 -> FUN_0044DCF0 (L59168:
  void FUN_0044dcf0(int *****param_1, char param_2))
FUN_0052FE90: callers=1 -> FUN_0044AC20 (L57513: void __fastcall(int))
```

### §2.3 `FUN_0052FB40` RTTI（v466_2a）
```
vtable 0xA322A0 [-1] = COL 0xAB407C (sig=0, offset=0, pTD=0xB03298)
  TD name: .?AVTESFullName@@
FUN_0052FB40 (L225995): undefined4 *__thiscall(undefined4 *param_1, undefined1 param_2) {
  FUN_0046c460(); param_1[6] = &PTR_FUN_00a322a0;   // +0x18 TESFullName vtable
  param_1[7] = 0; *(u16*)(param_1+8) = 0; *(u16*)((int)param_1+0x22) = 0; }
```

## §3 残点 **V-467-1..3**
1. 【中】**旧残点 V-406-3 継続** — **id `0x4C` の `+0xC` float の語**（何の値か・`FUN_00420C40/60` の利用箇所の文脈）＋ top-band 残り typed。
2. 【低】**旧残点 V-406-4／V-406-5** — `DATA+0x2C/+0x31` 意味（UESP 差分の docs 内裏取り）／`PTR_DAT_00B10D90` 所有 fn と spec 名前テーブル書手。
3. 【低】**V-466-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v466_1..v466_2 の2本全 EXIT=0**（V-466-1 は id `0x4C` consumer 系統＋flag writers caller＋`TESFullName` RTTI を確定／V-466-2/3 は未着手＝残点）。
- **決定打の作り**: (i) **`FUN_0041e210(0x4c)` の docs grep が2件（getter/setter pair）と即判明** → id `0x4C` の consumer が float get/set に限定 → (ii) **setter（`FUN_00420C60`）の body が `FUN_0041E160`（release）と `FUN_00401F00(0x10)`（alloc）まで語る** → 3点セット確定 → (iii) **flag writers の E8 census で caller を1件ずつ特定し signature を docs から取得** → (iv) **`FUN_0052FB40` の vtable `0xA322A0` を RTTI で `TESFullName` と即決**。
- 妥当性チェック: ①`FUN_0041E160(0x4c)`⇔setter の `v==0` release（id 0x4C の対）— ②`FUN_00401F00(0x10)`⇔object size 0x10（`+0xC` float＋header）— ③vtable `0xA322A0`⇔`.?AVTESFullName@@`⇔`param_1[6]`（+0x18）— ④`FUN_0044DCF0` の `int*****`⇔5重ポインタ（stack 解析警告と整合）— ⑤E8 caller 各件⇔v466_1 の census — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **id 定数 lookup の docs grep が「consumer の全体」を即時に与える**（`FUN_0041e210(0x4c)`→2件＝get/set pair）— **id 定数呼び出しを grep するだけで系統が見える**。
  2. **getter/setter/release の3点セットは setter の body で揃う**（`v==0`→release／hit→set／miss→alloc の3分岐）。
  3. **vtable store の imm32（`0xA322A0`）を RTTI で読めば init fn の class が即決**（`TESFullName`）— v441 法の応用。
  4. **E8 caller census は「caller 0件」も情報**（`FUN_004CDEC0`←0＝virtual/vtable 経由か inline）。
  5. **5重ポインタ（`int*****`）のような異常 signature は stack 解析失敗の兆候** — docs の WARNING 行（`alloca_probe`/`spacebase`）とセットで読む。
