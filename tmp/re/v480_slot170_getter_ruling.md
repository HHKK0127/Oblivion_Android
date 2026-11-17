# v480 — `+0x170` slot＝`*(this+0x1C)` getter の確定・elem＝Reference 系推定・`FUN_005E3320` の裁定（V-480-1..3 完遂・旧残点 V-406-3/V-406-4 継続第10弾）

- 採番: **v480**（旧残点群消化・probe prefix **v480_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-480-1..3（v479 残点）** — 1=旧残点 V-406-3 継続（elem class 完全特定＝Cell reference 系 vtable 逆引きと `elem[+0x28]` float の語）／2=旧残点 V-406-4 semantic 完遂（`DAT_00B37D00` 正体と donor 入手元）／3=V-479-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-480-1: `virtual+0x170()` の実装（slot 同定）** | **[確定相当]** | **ctor-store vtable 1486個の `[vt+0x170]` 先頭命令 census [確定(機械)]: shape 分布＝`non-fn/data 779`／`other 647`／**`mov eax,[ecx+X] 37`**／`jmp (thunk) 22`／`push; mov eax,[ecx+X] 1`。**`disp=0x1C` の field getter impl＝`FUN_004D9B40` を持つ vtable が7クラス [確定]**: **`TESObjectREFR`（vt=0xA46C44）／`Actor`（0xA6E074）／`ArrowProjectile`（0xA6F08C）／`Character`（0xA6FC9C）／`Creature`（0xA710F4）／`MobileObject`（0xA732FC）／`PlayerCharacter`（0xA73A0C）**（＋その派生の `NiFloatController` 等は別 impl `FUN_006D9400`）→ **`virtual+0x170()` ＝ `*(this+0x1C)` を返す getter（impl `FUN_004D9B40`）[確定相当]**（v476 の「form ptr getter [推定相当]」を実装判明で精緻化＝**`this+0x1C` field getter**）。他 disp: `0x10`（`MagicTarget`）／`0x18`（`LockFreeMap`）／`0x30/0x5C/0xA8/0xC4/0xF0`（Ni/Shader 系）と一覧取得 [確定] |
| **V-480-1: elem class（walk 条件の翻訳）** | **[推定相当]（強い）** | **walk 条件 `virtual[elem]+0x170() == DAT_00B35EB8` ⇔ `*(elem+0x1C) == NorthMarker singleton` [確定(構造)] → elem は `+0x1C` field に NorthMarker ptr を持つ object → `TESObjectCELL +0x48` list（v479）の elem は `TESObjectREFR` 系 [推定相当]**（Cell の reference list と整合）。**`[ebx+0x28]` float（byte 40）の語は `TESObjectREFR` の field 調査待ち＝残点**（`+0x6C` slot＝unregister 系（v474）も REF と整合） |
| **V-480-2: `FUN_005E3320`（`DAT_00B37D00` 比較の入口）identity** | **[確定相当]** | L347984: **`undefined4 __fastcall FUN_005e3320(int *param_1)`: `cVar1 = FUN_004d7f80(); if (cVar1 != 0) { iVar2 = virtual[*param_1 + 0x170](); if (iVar2 != 0) …` [確定(decompiled)]** → **`FUN_005E3320(param)` は `param` の `virtual+0x170`（＝`*(param+0x1C)`）を呼ぶ fn [確定相当]**。**`FUN_005C28F0`（L329493 `void(void)`）body 実読 [確定(decompiled)]**: `FUN_00589b70(0x40c)`→`FUN_005898f0()`→`FUN_0058ceb0(0x1772,…)`→`[iVar2+0x864]` 処理→`DAT_00b3b5d8=1`→**L329511 `virtual[*DAT_00B333C4 + 0x170]()`（Player singleton の `+0x170` 呼び出し）→ L329512 `iVar3=FUN_005e3320()` → L329513 `if (DAT_00b37d00==*(iVar3+0xc)) { L329514 FUN_005222d0(1); } else { L329517 virtual[*Player+0x268]() → `[+0x3c]` list walk → L329521/533 virtual[*Player+0x2e0/0x2dc](elem) → …→ L329525 FUN_005222d0(1) }`** → **Player（`0xB333C4`）の `+0x170`／`+0x268`／`+0x2e0`／`+0x2dc` virtual が連打され、`DAT_00B37D00` 比較が class 選択の分岐になる [確定(構造)]**。**`DAT_00B37D00` 正体は [未確定]＝残点**（`FUN_005E3320()` 戻り値の `+0xc` と照合される object） |
| **V-480-3: V-479-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_004CBA80` の `param_3` flag、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v480_1）

### §2.1 `[vt+0x170]` shape census（v480_1a）
```
ctor-store vtables: 1486
shape: {'non-fn/data': 779, 'other': 647, 'mov eax,[ecx+X]': 37,
        'jmp (thunk)': 22, 'push; mov eax,[ecx+X]': 1}
field-getter classes (disp != None):
  disp=0x10 .?AVMagicTarget@@            fn=FUN_00780d10
  disp=0x18 .?AV?$LockFreeMap@PBDPAVModel@@@@ fn=FUN_00438db0
  disp=0x1C .?AVTESObjectREFR@@  vt=0xA46C44 fn=FUN_004d9b40
            .?AVActor@@ vt=0xA6E074 / .?AVArrowProjectile@@ 0xA6F08C
            .?AVCharacter@@ 0xA6FC9C / .?AVCreature@@ 0xA710F4
            .?AVMobileObject@@ 0xA732FC / .?AVPlayerCharacter@@ 0xA73A0C
            (all fn=FUN_004d9b40)          <- shared impl
  disp=0x30 .?AVNiDX9ShaderConstantManager@@ / 0x5C NiD3DShaderInterface
  disp=0xA8 NiTPointerListBase<RenderedTextureData> / 0xC4,0xF0 SpeedTree
```

### §2.2 `FUN_005E3320` + `FUN_005C28F0`（v480_1b）
```
L347984: undefined4 __fastcall FUN_005e3320(int *param_1) {
  L347990: cVar1 = FUN_004d7f80();
  L347991: if (cVar1) { iVar2 = virtual[*param_1 + 0x170](); if (iVar2) ... }
L329493: void FUN_005c28f0(void) {
  L329500 FUN_00589b70(0x40c) / L329502 FUN_005898f0() / L329504 FUN_0058ceb0(0x1772,...)
  L329511 virtual[*DAT_00B333C4 + 0x170]()          // Player +0x170 call
  L329512 iVar3 = FUN_005e3320();
  L329513 if (DAT_00b37d00 == *(iVar3 + 0xc)) { L329514 FUN_005222d0(1); }
  else { L329517 virtual[*Player + 0x268]() -> walk [iVar3+0x3c] list
         L329521 virtual[*Player + 0x2e0](*piVar1) ... L329525 FUN_005222d0(1) ... }
```

## §3 残点 **V-481-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`[ebx+0x28]` float（`TESObjectREFR+0x28`）の語**（REFR field 調査）＋ walk の `+0x6C` slot（unregister 系）impl 確認で elem 系統の最終裏付け。
2. 【低】**旧残点 V-406-4 semantic 完遂** — **`DAT_00B37D00` 正体**（`FUN_005E3320()` 戻り値の `+0xc` field が何か・donor（`TESObjectCELL+0x104`）の入手元）→ `+0x64` 中身の語。
3. 【低】**V-480-3 引継ぎ** — `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v480_1 の1本 EXIT=0**（V-480-1 `+0x170`＝`*(this+0x1C)` getter を7クラス共有 impl で確定相当＋elem＝REFR 系推定相当／V-480-2 `FUN_005E3320` identity 相当と `FUN_005C28F0` body 構造確定・正体は残点／V-480-3 引継ぎ）。
- **決定打の作り**: (i) **`[vt+0x170]` を「先頭命令形」で census する**（v475 の「body に global があるか」の誤 predicate から修正）→ `mov eax,[ecx+X]` 37件＋disp 一覧 → **`disp=0x1C` が7クラス共有 impl `FUN_004D9B40` に集中** → slot 意味が確定 → (ii) **walk 条件を `*(elem+0x1C)==NorthMarker` に翻訳**し elem＝REFR 系へ → (iii) `FUN_005E3320` decompiled で `virtual+0x170` 呼び出しを確認。
- 妥当性チェック: ①`disp=0x1C`⇔7クラスは全て `TESObjectREFR` 派生（Actor/Character/Creature/MobileObject/Player は REF 継承系統）— ②`FUN_004D9B40` 共有⇔ICF/単一 impl — ③`*(elem+0x1C)==DAT_00B35EB8`⇔v474 walk `cmp eax,[0xb35eb8]` — ④`TESObjectCELL+0x48` list⇔Cell reference list（推定）— ⑤`FUN_005E3320` の `virtual+0x170`⇔`FUN_005C28F0` の Player `+0x170`（同 slot 呼び出し）— **五方向矛盾なし**。
- docs との不整合: **なし**（v476 の「form ptr getter」推定は本裁定で「`this+0x1C` getter」と実装確定に更新＝**推定の上書き（精緻化）**）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **slot の意味は「先頭命令形 census」で決まる** — 同一 slot を持つ vtable を先頭命令で分類（`mov eax,[ecx+X]`＝field getter）→ **disp が揃えば（本件 `0x1C` が7クラス）field getter と即決**（v475 の body-global predicate 誤りからの修正版）。
  2. **共有 impl（ICF）の fn は「どの class が載せるか」の列挙が class family を語る**（`FUN_004D9B40` の7 vtable＝`TESObjectREFR` 派生系統）。
  3. **walk 条件は「virtual call ＝ field access」に翻訳すると単純式になる**（`virtual[elem]+0x170()==singleton`⇔`*(elem+0x1C)==singleton`）。
  4. **shape 分布（`non-fn 779 / other 647 / getter 37 / thunk 22`）は「その slot が実装されている class 数」の分布**＝vtable が短い class が多数（slot 外 data）と分かる（v477 ASCII overflow の全体像）。
  5. **Player singleton（`0xB333C4`）の virtual 連打（`+0x170/+0x268/+0x2e0/+0x2dc`）が「gameplay メニュー/refresh ルーチン」の目印**（`FUN_005C28F0`）。
