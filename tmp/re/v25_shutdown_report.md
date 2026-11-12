# V25 Shutdown Chain Report - Oblivion.exe (Steam v1.2.0416)

Target: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
Mapping: ImageBase 0x400000, .text VA=0x400C00+file_off, .rdata VA=0xA28000 Raw=0x627200, .data VA=0xB02000 Raw=0x700C00 (RSz=0x30C00, VSz=0xA9C1C). PE parsed via `struct`, capstone 5.0.7.

## 1. Self-test (4/4 measured before analysis)

| # | check | expected | measured | result |
|---|---|---|---|---|
| 1 | bytes@0x504050 | `518b4c24188b5424` | `518b4c24188b5424` | **PASS** |
| 2 | bytes@0x4FB580 | `578b7908` | `578b7908` | **PASS** |
| 3 | string@0xA7035C | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | **PASS** |
| 4 | opcode opid0x0015 impl | `0x504560` | `0x504560` | **PASS** |

All 4 PASS (script aborts on any FAIL before producing artifacts).

## 2. Second-system function

- Header (L690871): `// FUN_007d1170 @ 0x7d1170`
- **VA = 0x007D1170**, signature: `undefined4 __fastcall FUN_007d1170(undefined4 param_1)`
- Contains `InterlockedIncrement` copy loop with `} while (uVar8 < 0x68c);` at **L690938**.
- Branch form is the inverted twin of the first system:
  - first system `FUN_007c8710 @ 0x7c8710` (L684979): `if (1 < DAT_00b42f48) { ...11 ext...; return; } FUN_0081f330();`
  - second system: `if (DAT_00b42f48 < 2) { FUN_0081f330(); } else { ...11 ext...; }` - identical semantics, both gate on shader-model level.

### Window L690900-690970

```c
690900:     iVar2 = *(int *)((int)&DAT_00b455a0 + uVar8);
690901:     local_4 = 0;
690902:     if (iVar2 != *piVar6) {
690903:       if (iVar2 != 0) {
690904:         piVar1 = (int *)(iVar2 + 0x60);
690905:         *piVar1 = *piVar1 + -1;
690906:         if (*piVar1 == 0) {
690907:           FUN_007604d0();
690908:         }
690909:       }
690910:       iVar2 = *piVar6;
690911:       *(int *)((int)&DAT_00b455a0 + uVar8) = iVar2;
690912:       if (iVar2 != 0) {
690913:         *(int *)(iVar2 + 0x60) = *(int *)(iVar2 + 0x60) + 1;
690914:       }
690915:     }
690916:     local_4 = 0xffffffff;
690917:     if ((local_14 != 0) &&
690918:        (*(int *)(local_14 + 0x60) = *(int *)(local_14 + 0x60) + -1, *(int *)(local_14 + 0x60) == 0))
690919:     {
690920:       FUN_007604d0();
690921:     }
690922:     puVar4 = DAT_00b45290;
690923:     iVar2 = *(int *)((int)&DAT_00b455a0 + uVar8);
690924:     puVar3 = *(undefined4 **)(iVar2 + 0x58);
690925:     if (puVar3 != DAT_00b45290) {
690926:       if (puVar3 != (undefined4 *)0x0) {
690927:         LVar7 = InterlockedDecrement(puVar3 + 1);
690928:         if ((LVar7 == 0) && (puVar3 != (undefined4 *)0x0)) {
690929:           (**(code **)*puVar3)(1);
690930:         }
690931:       }
690932:       *(undefined4 **)(iVar2 + 0x58) = puVar4;
690933:       if (puVar4 != (undefined4 *)0x0) {
690934:         InterlockedIncrement(puVar4 + 1);
690935:       }
690936:     }
690937:     uVar8 = uVar8 + 4;
690938:   } while (uVar8 < 0x68c);
690939:   FUN_00820910();
690940:   FUN_00814430();
690941:   FUN_00815db0();
690942:   FUN_0081aa00();
690943:   FUN_0081b120();
690944:   FUN_0081bce0();
690945:   FUN_0081d090();
690946:   FUN_0081dc40();
690947:   FUN_0081ea70();
690948:   FUN_008203d0();
690949:   if (DAT_00b42f48 < 2) {
690950:     FUN_0081f330();
690951:   }
690952:   else {
690953:     FUN_0082d990();
690954:     FUN_00820c00();
690955:     FUN_00828280();
690956:     FUN_00831910();
690957:     FUN_00832740();
690958:     FUN_008357b0();
```

## 3. DAT_00b42f48 (VA 0xB42F48) - full references

- Raw `.text` scan of `48 2f b4 00` (backtrack decode preferring mem-operand): **total 149 instructions = 139 reads + 10 writes**.
- **.data initial value: `00000000`** - RVA delta 0x40F48 >= .data RawSize 0x30C00 -> zero-initialized tail (.data VSz=0xA9C1C extends to 0xBABC1C; file off would be 0x741B48 >= RawEnd 0x731800)
- Decompile occurrences: **137 lines** total; first 40 below.

### 3.1 Writes (complete, 10)

| VA | bytes | instruction | enclosing function (decompile L) |
|---|---|---|---|
| 0x007B4615 | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B4664 | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B46A3 | `c705482fb40001000000` | `mov dword ptr [0xb42f48], 1` | `FUN_007b45f0` L670304 |
| 0x007B46BD | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B46D8 | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B46F0 | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B472F | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B474E | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B4768 | `8915482fb400` | `mov dword ptr [0xb42f48], edx` | `FUN_007b45f0` L670304 |
| 0x007B7362 | `891d482fb400` | `mov dword ptr [0xb42f48], ebx` | `FUN_007b7250` L671874 |

**Writer identity:** all 9 writes inside `FUN_007b45f0` (L670304, GPU vendor/shader-level detection: `__strnicmp("ati")`/`"nv"` + device id thresholds 0x60/0x61/0xff -> assigns 0/1/2/3/4/5/6), 1 write inside `FUN_007b7250` (L671874) which resets the whole graphics-settings block to defaults (`DAT_00b42f48 = 0`). Callers measured: `FUN_007b45f0` <- E8 @0x4982BF (in `FUN_004980d0` L121782) and @0x49885B (in `FUN_00498340` L121898) = D3D device creation; `FUN_007b7250` <- E8 @0xA10EB5 (in `FUN_00a10e50` L1038640).

### 3.2 Reads (sample 20 of 139)

| VA | bytes | instruction |
|---|---|---|
| 0x00404F0C | `a1482fb400` | `mov eax, dword ptr [0xb42f48]` |
| 0x00405A80 | `833d482fb40003` | `cmp dword ptr [0xb42f48], 3` |
| 0x00406B09 | `393d482fb400` | `cmp dword ptr [0xb42f48], edi` |
| 0x004073D3 | `833d482fb40003` | `cmp dword ptr [0xb42f48], 3` |
| 0x00407484 | `833d482fb40003` | `cmp dword ptr [0xb42f48], 3` |
| 0x0040CD08 | `3935482fb400` | `cmp dword ptr [0xb42f48], esi` |
| 0x0043F6AB | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x00497AB6 | `833d482fb40005` | `cmp dword ptr [0xb42f48], 5` |
| 0x00498863 | `833d482fb40005` | `cmp dword ptr [0xb42f48], 5` |
| 0x00498DD9 | `833d482fb40005` | `cmp dword ptr [0xb42f48], 5` |
| 0x0049BDFF | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x0049DA08 | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x0049E4A4 | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x0049EC1B | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x004D01DF | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x004D0A27 | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x004D0A8A | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x00507E7E | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x0050800E | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |
| 0x00508192 | `833d482fb40002` | `cmp dword ptr [0xb42f48], 2` |

Dominant read forms: `83 3d 48 2f b4 00 02/03/05` (`cmp dword [0xb42f48], imm`) and `a1`/`8b`/`39 35` register forms - all capability range tests.

### 3.3 Verdict

**`DAT_00b42f48` = detected pixel-shader / shader-model level (enum 0..7), written exactly once at D3D device init, never a menu/INI setting.**

Evidence:
- `.data` initial = `00000000` (bss-tail zero) -> value only ever set by `FUN_007b45f0` device-detection code.
- `FUN_007b4780` (L670385) maps value -> profile string: 1=`ps_1_3`, 2=`ps_2_0`, 3/5=`ps_2_a`, 4/6=`ps_2_b`, 7=`ps_3_0`; `FUN_007b47e0` (L670416) maps -> `vs_1_1`/`vs_2_0`/`vs_3_0`; `FUN_007b4590` (L670283) maps -> budget 2/6/8. This is decisive: the variable IS the shader model level.
- 137 decompile uses are almost all `if (1 < DAT_00b42f48)` / `if (DAT_00b42f48 < 2)` / `< 5` / `== 0` gates on shader-dependent rendering paths (both shutdown chains included).
- No INI/menu writer exists among the 10 raw writes.

### 3.4 Decompile occurrences (first 40 of 137)

```
L4063:     uVar1 = DAT_00b42f48;
L4610:   if (2 < DAT_00b42f48) {
L5139:   if ((DAT_00b06cbc == '\0') || (DAT_00b42f48 < 2)) {
L5474:   if (((DAT_00b42f48 < 3) || (bVar19 = (DAT_00b42f40 & 0x10) != 0, !bVar19)) ||
L5494:   if (((DAT_00b42cdb == '\0') && (2 < DAT_00b42f48)) && ((DAT_00b42f40 & 0x10) != 0)) {
L8240:   if (((DAT_00b42f3e == '\0') || (DAT_00b42e84 == '\0')) || (DAT_00b42f48 < 2)) {
L48694:   if (((DAT_00b07050 != '\0') && (DAT_00b42f3e != '\0')) && (1 < DAT_00b42f48)) {
L121444:   DAT_00b430b0 = (DAT_00b42f48 < 5) - 1 & 2;
L122099:   if (DAT_00b42f48 < 5) {
L122309:   DAT_00b430b0 = (DAT_00b42f48 < 5) - 1 & 2;
L124386:   if (1 < DAT_00b42f48) {
L125562:   if (1 < DAT_00b42f48) {
L125966:     if (((DAT_00b07050 != '\0') && (DAT_00b42f3e != '\0')) && (1 < DAT_00b42f48)) {
L126305:   if (((DAT_00b07050 != '\0') && (DAT_00b42f3e != '\0')) && (1 < DAT_00b42f48)) {
L163250:   if (((DAT_00b42d40 == '\0') || (DAT_00b42f3e == '\0')) || (DAT_00b42f48 < 2)) {
L163625:     if ((((DAT_00b42d40 != '\0') && (DAT_00b42f3e != '\0')) && (1 < DAT_00b42f48)) &&
L163636:     if (((DAT_00b42d40 != '\0') && (DAT_00b42f3e != '\0')) && ((1 < DAT_00b42f48 && (iVar3 != 0))))
L201189:   if (DAT_00b42f48 < 2) {
L203299:       if ((DAT_00b42f3e != '\0') && ((DAT_00b42e84 != '\0' && (1 < DAT_00b42f48)))) {
L322957:     if (((DAT_00b42d40 == '\0') || (DAT_00b42f3e == '\0')) || (bVar9 = true, DAT_00b42f48 < 2)) {
L344036:       if (DAT_00b42f48 < 5) {
L344468:       if (DAT_00b42f48 < 5) {
L344542:       if ((DAT_00b42f48 < 5) && (FUN_0058ceb0(0xfaf,0x3f800000), *(int *)(iVar5 + 0x78) != 0)) {
L355628:   if ((((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) &&
L358772:   if ((((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) &&
L459881:   if ((((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) &&
L459922:        (((DAT_00b42e84 == '\0' || (DAT_00b42f48 < 2)) ||
L460001:   if (((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) {
L460035:   if (((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) {
L461530:   if ((((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) &&
L461548:   if ((((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) &&
L461598:      ((DAT_00b42f48 < 2 || (piVar5 != DAT_00b333c4)))) {
L461618:     if (((DAT_00b42f3e != '\0') && (DAT_00b42e84 != '\0')) && (1 < DAT_00b42f48)) {
L668750:     if ((DAT_00b42f48 < 2) || (DAT_00b43070 != '\0')) {
L668975:   if (DAT_00b42f48 == 0) goto LAB_007b3817;
L669017:       uVar19 = ((DAT_00b42f48 < 2) - 1 & 0x94) + 0x50;
L670148:     if (DAT_00b42f48 < 2) {
L670172:     if (1 < DAT_00b42f48) {
L670215:     if (1 < DAT_00b42f48) {
L670230:     if (1 < DAT_00b42f48) {
```

## 4. `uVar8 < 0x68c` loop interpretation

Byte confirmation inside `FUN_007d1170`: `83 c3 04` (`add ebx,4`) @0x7D1247, `81 fb 8c 06 00 00` (`cmp ebx,0x68c`) @0x7D124A (decompiler renders counter as uVar8). `0x68c` never appears as `3d 8c 06 00 00` (that form is absent; ebx-form is the real encoding).

- 0x68c = 1676 bytes; +4 step -> **419 elements**.
- Array = **`DAT_00b455a0`, VA 0xB455A0..0xB45C2C** (0xB455A0 + 0x68C = 0xB45C2C exactly). Cross-check: `FUN_007c8b30` (L685243) releases exactly `&DAT_00b455a0 ... while (piVar4 < 0xb45c2c)`; `FUN_007d1320` (L691049) bounds its index with `if (0x1a2 < uVar1)` = 418 max = 419 elements.
- **What is traversed:** each element is a pointer to a refcounted render-state cache object. Loop body per element:
  1. `FUN_007606a0(&local_14,...)` fetches the current shared/default instance for that slot;
  2. if slot object differs: decrement `obj+0x60` refcount (0 -> `FUN_007604d0` destroy), store new pointer, increment ref;
  3. replace `obj+0x58` (a NiRefObject handle) with `DAT_00b45290` (the default vertex-shader handle array base) via `InterlockedDecrement/InterlockedIncrement(puVar3+1)` - note `puVar3+1` is undefined4* +1 = byte offset +4 = the interlocked refcount field.
- `DAT_00b45290` context: 129-entry array 0xB45290..0xB45494 of compiled `SLS1%03i.vso` vertex shaders (`vs_1_1`, filled L689009-689025; cleared in `FUN_007c8b30`/`FUN_007c8f50`); `FUN_007c90b0` iterates the same range `uVar1 < 0x204` calling vt+0x5c on each -> these are shader-program objects. So `obj+0x58` = the state object's bound shader, forced back to the default entry (`DAT_00b45290` scalar = slot0 pointer) during reset.
- **Meaning: the loop resets all 419 render-state/shader state-cache slots to their defaults (default state object + default shader binding) before the tiered slot-reset chain runs.**

### Loop excerpt (L690938 +/-20)

```c
690918:        (*(int *)(local_14 + 0x60) = *(int *)(local_14 + 0x60) + -1, *(int *)(local_14 + 0x60) == 0))
690919:     {
690920:       FUN_007604d0();
690921:     }
690922:     puVar4 = DAT_00b45290;
690923:     iVar2 = *(int *)((int)&DAT_00b455a0 + uVar8);
690924:     puVar3 = *(undefined4 **)(iVar2 + 0x58);
690925:     if (puVar3 != DAT_00b45290) {
690926:       if (puVar3 != (undefined4 *)0x0) {
690927:         LVar7 = InterlockedDecrement(puVar3 + 1);
690928:         if ((LVar7 == 0) && (puVar3 != (undefined4 *)0x0)) {
690929:           (**(code **)*puVar3)(1);
690930:         }
690931:       }
690932:       *(undefined4 **)(iVar2 + 0x58) = puVar4;
690933:       if (puVar4 != (undefined4 *)0x0) {
690934:         InterlockedIncrement(puVar4 + 1);
690935:       }
690936:     }
690937:     uVar8 = uVar8 + 4;
690938:   } while (uVar8 < 0x68c);
690939:   FUN_00820910();
690940:   FUN_00814430();
690941:   FUN_00815db0();
690942:   FUN_0081aa00();
690943:   FUN_0081b120();
690944:   FUN_0081bce0();
690945:   FUN_0081d090();
690946:   FUN_0081dc40();
690947:   FUN_0081ea70();
690948:   FUN_008203d0();
690949:   if (DAT_00b42f48 < 2) {
690950:     FUN_0081f330();
690951:   }
690952:   else {
690953:     FUN_0082d990();
690954:     FUN_00820c00();
690955:     FUN_00828280();
690956:     FUN_00831910();
690957:     FUN_00832740();
690958:     FUN_008357b0();
```

## 5. 22-slot chain table (same content as CSV)

| tier | idx | fun_label | va | size | dec_line | cc_prefix | e8_first5 | strings | slot419 | role |
|---|---|---|---|---|---|---|---|---|---|---|
| base | 0 | FUN_00820910 | 0x00820910 | 751 | 739928 | void | 0x00772DF0;0x00772CD0;0x00772DF0;0x00772CD0;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 0 | reset render-state slot 419[0] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 1 | FUN_00814430 | 0x00814430 | 2048 (cap) | 733608 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 3 | reset render-state slot 419[3] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 2 | FUN_00815db0 | 0x00815DB0 | 2048 (cap) | 0 | unknown(not decompiled) | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 25 | reset render-state slot 419[25] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 3 | FUN_0081aa00 | 0x0081AA00 | 1810 | 734946 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 49 | reset render-state slot 419[49] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 4 | FUN_0081b120 | 0x0081B120 | 2048 (cap) | 735322 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 226 | reset render-state slot 419[226] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 5 | FUN_0081bce0 | 0x0081BCE0 | 2048 (cap) | 735934 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 260 | reset render-state slot 419[260] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 6 | FUN_0081d090 | 0x0081D090 | 2048 (cap) | 736960 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 271 | reset render-state slot 419[271] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 7 | FUN_0081dc40 | 0x0081DC40 | 2048 (cap) | 737568 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 384 | reset render-state slot 419[384] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 8 | FUN_0081ea70 | 0x0081EA70 | 2048 (cap) | 738312 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 400 | reset render-state slot 419[400] to default (slot obj swap + default VS rebind + state-id reapply) |
| base | 9 | FUN_008203d0 | 0x008203D0 | 1335 | 739638 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 396 | reset render-state slot 419[396] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 0 | FUN_0082d990 | 0x0082D990 | 2048 (cap) | 0 | unknown(not decompiled) | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 74 | reset render-state slot 419[74] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 1 | FUN_00820c00 | 0x00820C00 | 2048 (cap) | 0 | unknown(not decompiled) | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 118 | reset render-state slot 419[118] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 2 | FUN_00828280 | 0x00828280 | 2048 (cap) | 0 | unknown(not decompiled) | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 172 | reset render-state slot 419[172] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 3 | FUN_00831910 | 0x00831910 | 2048 (cap) | 740101 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 232 | reset render-state slot 419[232] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 4 | FUN_00832740 | 0x00832740 | 2048 (cap) | 740845 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 277 | reset render-state slot 419[277] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 5 | FUN_008357b0 | 0x008357B0 | 2048 (cap) | 743135 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 388 | reset render-state slot 419[388] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 6 | FUN_00836230 | 0x00836230 | 1493 | 743691 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 394 | reset render-state slot 419[394] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 7 | FUN_00836810 | 0x00836810 | 2048 (cap) | 0 | unknown(not decompiled) | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 72 | reset render-state slot 419[72] to default (slot obj swap + default VS rebind + state-id reapply) [tier-exclusive counterpart] |
| extended | 8 | FUN_00839f90 | 0x00839F90 | 2048 (cap) | 744299 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 352 | reset render-state slot 419[352] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 9 | FUN_0083a7e0 | 0x0083A7E0 | 1010 | 744769 | void | 0x00772DF0;0x00772CD0;0x00772DF0;0x00772CD0;0x00772DF0 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 357 | reset render-state slot 419[357] to default (slot obj swap + default VS rebind + state-id reapply) |
| extended | 10 | FUN_00839a50 | 0x00839A50 | 1335 | 744009 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 398 | reset render-state slot 419[398] to default (slot obj swap + default VS rebind + state-id reapply) |
| fallback | 0 | FUN_0081f330 | 0x0081F330 | 2048 (cap) | 738780 | void | 0x00772630;0x00772560;0x00801110;0x00760010;0x00772630 | 0x00A94024:Invalid sub texture in decal;0x00A95138:SHADER ERROR : no shader to handle AMBDIFFTEX_FGFgAVc ( fog & glow & facegen & alpha & vertexcolors );0x00A950D8:SHADER ERROR : no shader to handle AMBDIFFTEX_GFgAVc ( glow & facegen & alpha & vertexcolors ) | 72 | reset render-state slot 419[72] to default (slot obj swap + default VS rebind + state-id reapply) [tier-exclusive counterpart] |

Notes:
- `slot_index` = index within its tier; `slot419` = index into the 419-entry `DAT_00b455a0` array, derived from the first in-range absolute `mov` (validated 17/17 against decompiled ground truth).
- 5 slots have `decompile_line=0` (SEH-protected, not decompiled: `FUN_00815db0`, `FUN_0082d990`, `FUN_00820c00`, `FUN_00828280`, `FUN_00836810`); prologue/state-id structure identical to decompiled siblings so role confidence stays high.
- `size_bytes`: terminal `ret`+`0xCC` or int3-run within cap 0x800; `2048 (cap)` = no terminal found within 0x800 (large functions).
- state-ids (push-immediate before `call 0x772cd0`): common set {0x1b,0xf,0x7,0x17,0xe,0x34,0x13,0x14,0x19,0x18,0x98,0xab,0x16,0xa8} = NiDX9Renderer/D3D render-state ids (0x1b=ALPHAREF-region etc. in the NiDX9 state enum); per-slot subsets differ slightly.
- slot419=72 is shared by fallback `FUN_0081f330` and extended `FUN_00836810` - they are mutually exclusive tiers (DAT<2 vs >=2), not a duplication bug.

## 6. vt+0xA8 delegate

**determined**
- `0x7C8710` pointer found at `.rdata+0x00A906C0` (single occurrence). It sits inside vtable base **0x00A90634** (48 methods, `.?AVShadowLightShader@@` = `ShadowLightShader`) at **slot 35 (+0x8C)** - so `param_1` of `FUN_007c8710` is a **`ShadowLightShader*`** (this pointer, fastcall ECX).
- `vtable_map.txt` lookup `base+0xA8` (slot 42, within the 48-method table) -> target **0x007FAA90** = `FUN_007faa90 (decompile L719887)`.
- Measured body of `FUN_007faa90` (L719889): `void __fastcall FUN_007faa90(int *param_1) { (**(code **)(*param_1 + 0xac))(); ... (**(code **)(*param_1 + 0xb0))(); }` - it is itself a two-virtual-call trampoline (vt+0xAC then vt+0xB0 of the same object).
- Fun fact: pointer of second system `0x7D1170` is at 0xa906e8 = slot 45 (+0xB4) of the SAME ShadowLightShader vtable; `FUN_007c8710` is slot 35. Both are ShadowLightShader virtual methods.

## 7. Porting implications (confirmed range only)

Shutdown/reset order to reproduce:
1. Call the object's vt+0xA8 delegate (`ShadowLightShader` slot42 -> `FUN_007faa90` trampoline) exactly as `FUN_007c8710` does first.
2. Reset all 419 `DAT_00b455a0` state-cache slots (default state object + default shader binding with refcount discipline) as `FUN_007d1170` does before its chain.
3. Run the 10 base slot resets in chain order: `0x820910, 0x814430, 0x815db0, 0x81aa00, 0x81b120, 0x81bce0, 0x81d090, 0x81dc40, 0x81ea70, 0x8203d0`.
4. Branch on shader-model level (`DAT_00b42f48` in original):
   - level >= 2 (ps_2_0+): run the 11 extended resets `0x82d990, 0x820c00, 0x828280, 0x831910, 0x832740, 0x8357b0, 0x836230, 0x836810, 0x839f90, 0x83a7e0, 0x839a50` (both chains use this order; branch polarity differs but semantics identical).
   - level < 2: run only `0x81f330`.
5. Each slot reset: swap the 419[idx] cache object to its default (refcount +/-) and reapply its state-id subset via the 0x772cd0-style setter; idx table is in the CSV.
6. Port note: the level variable must be derived from detected shader capability at device init (enum 0..7), never from settings UI - no INI writer exists.

## 8. Undetermined / limitations

- Exact semantic names of individual state-ids (0x98, 0xab, 0xa8...) within the NiDX9 state enum: not resolved (no string cross-ref in scope).
- `decompile_line=0` for the 5 SEH functions (known Ghidra decompilation gap); signatures inferred as `void f(void)` from call sites.
- `FUN_007faa90` vt+0xAC / vt+0xB0 targets (0x7cd090 L687484, 0x7cefd0 L689093) roles: out of scope this task.
- `size_bytes=2048 (cap)` for 12 large slots: true sizes exceed 0x800 (measured earlier up to 30335 for 0x820C00 via bound+ret+CC method), cap applied per task spec.
- .data initial value is bss-zero (file off beyond raw) - runtime value only from `FUN_007b45f0`/`FUN_007b7250`.

---
Artifacts:
- `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v25_shutdown_slots.csv`
- `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v25_shutdown_report.md`
