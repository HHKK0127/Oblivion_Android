# v521 — allocator hash map＝`NiTMapBase<…, TESForm*>`（RTTI）・`FUN_0041E2D0`＝`GetPrevExtraData`・allocator 呼び出し3fn の裁定（V-521-1..3 完遂・旧残点 V-406-3 継続第51弾）

- 採番: **v521**（旧残点群消化・probe prefix **v521_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-521-1..3（v520 残点）** — 1=`FUN_0041E2D0`（type で extra 検索）identity＋`FUN_0055E000` の map object／2=hash vtable・bucket identity と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-521-2: allocator の hash vtable（`PTR_PTR_00b0613c`）identity** | **[確定]（RTTI）→ `NiTMapBase<NiTPointerAllocator<uint>, TESForm*>`** | `.rdata` 実読＋RTTI 実読 [確定(実RTTI)]: **dword `@0xB0613C`＝`0x00A3BC94` → `rtti_name(0xA3BC94) = .?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@IPAVTESForm@@@@` → `NiTMapBase<NiTPointerAllocator<unsigned int>, TESForm*>` [確定]** → **allocator（`FUN_00448D40`）の hash map＝`TESForm*` を保持する `NiTMapBase`＝TESForm レジストリ map [確定]**。vtable slots [確定(機械)]: **`[+0x0]`＝`FUN_0046B030`（ctor/dtor [推定]）／`[+0x4]`＝`FUN_006A9060`（**hash fn**・allocator の `virtual[*v+4]` 実体）／`[+0x8]`＝`FUN_00763E80`（**compare fn**・`virtual[+8]` 実体）／`[+0xC]`＝`FUN_0067F130`／`[+0x10]`＝`FUN_0068F970`／`[+0x14]`＝`FUN_00982D85`** → **allocator の hash（`FUN_006A9060`）と compare（`FUN_00763E80`）を特定 [確定相当]**。`DAT_00b06144`（bucket）は runtime init（dword=0）[確定相当] |
| **V-521-1: `FUN_0041E2D0`（type で extra 検索）identity** | **[確定]（実文字列）→ `BaseExtraList::GetPrevExtraData`** | L20109 実読 [確定(decompiled)＋実文字列]: **`int __thiscall FUN_0041e2d0(int param_1, byte param_2)`: `if ((param_2 >> 3 < 0xc) && ((*(byte *)((param_2 >> 3) + 8 + param_1) & (1 << (param_2 & 7))) != 0))`（**96-type bit mask 検査＝v504/v520 と同一型**）→ **`FUN_00401020("BaseExtraList::GetPrevExtraData")`（critical section 名＝実文字列）**→ `iVar3 = *(param_1 + 4)`（list head）→ walk [構造]** → **`FUN_0041E2D0`＝`BaseExtraList::GetPrevExtraData`（type byte 指定で extra data を検索する fn）[確定]** → **`BaseExtraList` family が「`FUN_0041E210`＝GetExtraData（v504）／`FUN_0041E2D0`＝GetPrevExtraData（本裁定）／`FUN_00422E20`＝RemoveExtra（v520）」の3種で揃う [確定相当]**（**3 fn とも同一の bit mask＋TLS cache 構造**） |
| **V-521-1: allocator の map object（`FUN_0055E000` の this）** | **[確定相当]（TESForm registry の `NiTMapBase`）** | **`FUN_00448D40` E8 callers＝3件 [確定(機械)]: `0x4160E8`（`FUN_00416000`・`void __thiscall(int, undefined4)`）／`0x45C854`（`FUN_0045C7A0`・`void __thiscall(int, int*)`）／**`0x46C58F`（`FUN_0046C460`・TESForm ctor・v515/v517）** → allocator は TESForm ctor 他3fn から呼ばれ、hash map が `NiTMapBase<…, TESForm*>`（RTTI 確定）→ **`FUN_0055E000` の map object＝TESForm レジストリ [確定相当]**（v520 残点「reserved map が何の map か」への回答） |
| **V-521-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v521_1）

### §2.1 `FUN_0041E2D0` = GetPrevExtraData（v521_1a）
```
L20109: int __thiscall FUN_0041e2d0(int param_1, byte param_2) {
L20116:   if ((param_2 >> 3 < 0xc) &&
L20117:      ((*(byte *)((param_2 >> 3) + 8 + param_1) & (1 << (param_2 & 7))) != 0)) {
L20118:     FUN_00401020("BaseExtraList::GetPrevExtraData");   // REAL STRING
L20119:     iVar3 = *(int *)(param_1 + 4);                     // list head
L20120:     iVar2 = 0; ... walk ...
=> BaseExtraList::GetPrevExtraData  [確定]
   same 96-type bit mask shape as GetExtraData (v504) / RemoveExtra (v520)
   BaseExtraList family: Get / GetPrev / Remove = 3 fns  [確定相当]
```

### §2.2 allocator hash map RTTI（v521_1c）
```
dword @0xB0613C = 0x00A3BC94
rtti_name(0xA3BC94) = .?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@IPAVTESForm@@@@
  -> NiTMapBase<NiTPointerAllocator<unsigned int>, TESForm*>  [確定]
  -> allocator hash map = TESForm registry map  [確定]
vtable slots:
  [+0x0] = 0x0046B030  owning=FUN_0046b030      (ctor/dtor [推定])
  [+0x4] = 0x006A9060  owning=FUN_006a9060      (HASH fn - allocator's virtual[+4])
  [+0x8] = 0x00763E80  owning=FUN_00763e80      (COMPARE fn - allocator's virtual[+8])
  [+0xC] = 0x0067F130  owning=FUN_0067f100
  [+0x10] = 0x0068F970 owning=FUN_0068f970
  [+0x14] = 0x00982D85 owning=FUN_00982c4d
DAT_00b06144 dword = 0x00000000 (bucket array, runtime-init)  [確定相当]
```

### §2.3 allocator callers（v521_1b）
```
E8 callers of FUN_00448d40: 3
  0x4160E8 owning=FUN_00416000  void __thiscall(int, undefined4)
  0x45C854 owning=FUN_0045c7a0  void __thiscall(int, int*)
  0x46C58F owning=FUN_0046c460  undefined4 * __fastcall(undefined4 *)  (TESForm ctor, v515/v517)
=> allocator driven by the TESForm constructor and two methods;
   map object = TESForm registry (NiTMapBase of TESForm*)  [確定相当]
```

## §3 残点 **V-522-1..3**
1. 【中】**`FUN_006A9060`（hash fn）／`FUN_00763E80`（compare fn）本文**（registry の key 比較法＝form id 比較か pointer 比較か）＋ **`FUN_0046B030`（`NiTMapBase` ctor/dtor 候補）identity**。
2. 【低】**`FUN_00416000`/`FUN_0045C7A0`（allocator の残り2 caller）identity** ＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v521_1 の1本 EXIT=0**（V-521-2 hash vtable＝`NiTMapBase<…, TESForm*>` を RTTI 実読で確定・hash/compare fn を特定／V-521-1 `FUN_0041E2D0`＝`GetPrevExtraData` を実文字列で確定・allocator 呼び出し3fn を列挙／V-521-3 引継ぎ）。
- **決定打の作り**: (i) **`PTR_PTR_00b0613c` の `.rdata` dword→`rtti_name` 実読** → `NiTMapBase<…, TESForm*>` で **allocator の map＝TESForm レジストリと即決**（v520「reserved map が何か」に回答）→ (ii) **vtable slots 実読**で hash（`FUN_006A9060`）／compare（`FUN_00763E80`）を特定 → (iii) **`FUN_0041E2D0` docs head** で `"BaseExtraList::GetPrevExtraData"` 実文字列を獲得 → family3種が揃う → (iv) **allocator E8 census（3件）**で TESForm ctor との接続を確認。
- 妥当性チェック: ①`NiTMapBase<…, TESForm*>`⇔allocator が form id を key に TESForm* を管理（v519 allocator 構造と整合）— ②`virtual[+4]`（hash）⇔`FUN_006A9060`／`virtual[+8]`（compare）⇔`FUN_00763E80`（v519/v520 の slot 呼び出しと一致）— ③`"BaseExtraList::GetPrevExtraData"`⇔`"BaseExtraList::GetExtraData"`（v504）／`"BaseExtraList::RemoveExtra"`（v520）＝**同一 critical section 群** — ④`param_2 >> 3 < 0xc`⇔v504/v520 と同一 mask 形 — ⑤`FUN_0046C460`（TESForm ctor）が allocator caller⇔v517 ctor が `param_1+0xC`（current id）を初期化する構造と整合 — **五方向矛盾なし**。
- docs との不整合: **なし**（RTTI・docs head とも一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`PTR_PTR_xxx`（vtable ptr）の RTTI 逆引きが container の class を確定**（**`NiTMapBase<…, TESForm*>`＝allocator の map が TESForm レジストリと即決**＝v502/v516 に続く RTTI 法の3度目）。
  2. **`BaseExtraList` family は「Get／GetPrev／Remove」の3種で揃う**（**3 fn とも critical section 名（`FUN_00401020("…")`）＋同一 bit mask 構造**＝名前法＋構造の横断一致）。
  3. **hash fn／compare fn が vtable slot（`+4`/`+8`）から特定できる**（**container の hash/compare 実体が判明＝map の key 比較法が次の手**＝v519/v520 の slot 呼び出しに接続）。
  4. **allocator の caller census（3件）が「誰が id counter を使うか」を語る**（**TESForm ctor＋2 method＝form id の生成経路が判明**）。
  5. **`DAT_00b06144`（bucket）の runtime init（dword=0）が「起動時に確保される配列」の目印**（**bucket は static null→runtime 分配 [確定相当]**）。
