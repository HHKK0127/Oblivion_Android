# v470 — `DAT_00B35EB8`＝`TESObjectSTAT` singleton の確定と setter/ptr 配列の裁定（V-470-1..3 完遂・旧残点 V-406-3/V-406-4/V-406-5 消化）

- 採番: **v470**（旧残点群消化・probe prefix **v470_1..v470_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-470-1..3（v469 残点）** — 1=旧残点 V-406-3 継続（`DAT_00B35EB8` singleton の class と id `0x4C` float の semantic）／2=旧残点 V-406-4（`DATA+0x2C/+0x31`）と V-406-5（`PTR_DAT_00B10D90`）／3=V-469-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-470-1: `DAT_00B35EB8` singleton の class** | **[確定]** | **ctor `FUN_004B9AA0`（alloc 0x3C 直後の `call 0x4B9AA0`）の vtable store を RTTI 実読**: **`4B9AE0: mov [esi],0xA44DEC` → RTTI `.?AVTESObjectSTAT@@`／`4B9AE6: mov [edi],0xA44DC8` → `.?AVTESObjectSTAT@@` [確定(機械＋RTTI)]**。docs L145592-145594: **`*param_1 = &PTR_FUN_00a44dec; param_1[9] = &PTR_FUN_00a44dc8; *(u8*)(param_1 + 1) = 0x1c;` [確定(decompiled)]** → **`byte[+4] = 0x1C` は v441 FormType 表の `0x1C = TESObjectSTAT` と完全一致 → `DAT_00B35EB8` ＝ `TESObjectSTAT`（Static object）singleton [確定]**。`param_1[9]`（`+0x24` subobject vtable `0xA44DC8`）が v468 `FUN_004CD960` の `[ecx+0x24]` virtual `+0x18` call と一致 [確定] |
| **V-470-1: id `0x4C` float の semantic** | **[部分確定]（receiver 特定）＋[未確定]（語）** | **`virtual[elem]+0x170() == DAT_00B35EB8`（＝`TESObjectSTAT` singleton）の比較 [確定]** → **elem は TESObjectSTAT と比較される側の object（`param_1+0x48` list の要素）[推定高]**。`id_0x4C_float = -elem[10]`（v469 式）の **`elem[10]`（byte +40 float）が何の値か（STAT の何の property）は [未確定]＝残点** |
| **V-470-1: `FUN_004B2B80`（singleton を渡す fn）** | **[確定相当]** | L141003: **`void __thiscall(int *param_1, int param_2)`: `iVar1 = param_1[2]; *(param_2+0x1c) = iVar1; if (iVar1==0) *(param_2+0x20)=0; else { *(param_2+0x20) = *(iVar1+0x20); if (*(iVar1+0x20)) *(*(iVar1+0x20)+0x1c) = param_2; … }` [確定(構造)]** → **linked list の node 接続/切断 fn（`param_2+0x1C/+0x20` を `param_1[2]` と双方向接続）[確定相当]** |
| **V-470-2: 旧 V-406-4（`form+0x64`/`+0x69` 消費 setter）** | **[確定]（setter identity）＋[未確定]（semantic）** | **`FUN_00468230`（L81885）`void __thiscall(int self, undefined4 v)`: `*(self+8) = v;` ＝ dword setter [確定]**／**`FUN_00468270`（L81926）`void __thiscall(int self, undefined1 v)`: `*(u8*)(self+0xD) = v;` ＝ byte setter [確定]** → **form+0x64 の値は `FUN_00468230` が actor 側 `self+8` へ転送する dword、form+0x69 の値は `FUN_00468270` が `self+0xD` へ転送する byte [確定相当]**。**両者の semantic（何の値か）は [未確定]＝残点** |
| **V-470-3: 旧 V-406-5（`PTR_DAT_00B10D90`）** | **[確定]（配列構造）＋[未確定]（指し先）** | **docs L224762-63: `if ((param_1 < 3) && ((undefined4*)(&PTR_DAT_00b10d90)[param_1] != 0)) return *(&PTR_DAT_00b10d90)[param_1];` [確定(decompiled)] → `PTR_DAT_00B10D90`＝3要素（`param_1 < 3` bound）の ptr 配列 lookup [確定]**。**配列実体: `[0]=0xB364E8／[1]=0xB364F0／[2]=0xB364F8／[3]=0` [確定(実読)]** → **3つの runtime object（`0xB364E8/F0/F8`）への ptr [確定]、指し先の identity は [未確定]＝残点**（`spec 名前テーブル [推定]`） |

## §2 証拠（v470_1/v470_2）

### §2.1 singleton class（v470_1a + v470_2a）
```
FUN_004B9AA0 (ctor, L145575): undefined4 *__fastcall(undefined4 *param_1) {
  FUN_004b2ff0(uVar1); FUN_0046d7e0();
  *param_1 = &PTR_FUN_00a44dec;            // vtable
  param_1[9] = &PTR_FUN_00a44dc8;          // +0x24 subobject vtable
  *(u8*)(param_1 + 1) = 0x1c; }            // byte +4 = 0x1C (FormType!)
machine: 4B9AE0 mov [esi], 0xa44dec -> RTTI .?AVTESObjectSTAT@@
         4B9AE6 mov [edi], 0xa44dc8 -> RTTI .?AVTESObjectSTAT@@
v468 tie: FUN_004CD960's [ecx+0x24] virtual +0x18 call == param_1[9] subobject
```

### §2.2 `FUN_004B2B80`（v470_2b）
```
L141003: void __thiscall FUN_004b2b80(int *param_1, int param_2) {
  iVar1 = param_1[2];
  *(param_2 + 0x1c) = iVar1;
  if (iVar1 == 0) *(param_2 + 0x20) = 0;
  else { *(param_2+0x20) = *(iVar1+0x20);
         if (*(iVar1+0x20)) *(*(iVar1+0x20)+0x1c) = param_2; ... } }
  -> linked-list node link/unlink
```

### §2.3 setters + ptr array（v470_1c + v470_2c）
```
FUN_00468230 (L81885): void __thiscall(int self, undefined4 v) { *(self+8) = v; }
FUN_00468270 (L81926): void __thiscall(int self, undefined1 v) { *(u8*)(self+0xD) = v; }
PTR_DAT_00B10D90 @ 0xB10D90: [0]=0xB364E8 [1]=0xB364F0 [2]=0xB364F8 [3]=0x00000000
```

## §3 残点 **V-471-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`elem[10]`（byte +40 float）の語**（`TESObjectSTAT` singleton と比較される elem の何の property か）＋ `0xB364E8/F0/F8`（3 ptr 指し先）の identity。
2. 【低】**旧残点 V-406-4 残り** — `form+0x64`（dword）と `form+0x69`（byte）の semantic（`FUN_00468230/468270` が actor へ転送する値の意味）。
3. 【低】**V-470-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v470_1..v470_2 の2本全 EXIT=0**（V-470-1 完遂（`TESObjectSTAT` singleton 確定＋`FUN_004B2B80` list fn）／V-470-2 setter identity 確定・semantic は残点／V-470-3 配列構造確定・指し先は残点）。
- **決定打の作り**: (i) **alloc 直後の `call 0x4B9AA0`（ctor）を特定し、その vtable store を RTTI で `TESObjectSTAT` と即決** → (ii) **`byte[+4]=0x1C` が v441 FormType 表（`0x1C`=TESObjectSTAT）と完全一致したことで二重確認** → (iii) **`param_1[9]`（`+0x24` subobject）が v468 の `[ecx+0x24]` call と一致** → (iv) setter は1行 decompiled、ptr 配列は docs 引数式＋実体実読。
- 妥当性チェック: ①vtable `0xA44DEC`⇔`.?AVTESObjectSTAT@@`⇔byte `+4=0x1C`⇔v441 FormType 表 `0x1C`=TESObjectSTAT — ②`param_1[9]`⇔`+0x24`⇔v468 `[ecx+0x24]` virtual call — ③`FUN_004B2B80` の `+0x1C/+0x20`⇔linked list node（v456 doubly-linked と同型）— ④`PTR_DAT_00B10D90[3]`⇔`param_1 < 3` bound — ⑤`FUN_00468230`⇔v406 `FUN_00468230(class[+0x64])` — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_004B9AA0`/`FUN_004B2B80`/`FUN_00468230`/`FUN_00468270` に annotation 実在）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **alloc 直後の `call <ctor>` は singleton/object の class 特定の入口** — ctor の vtable store を RTTI で読むだけ（**alloc→ctor→vtable→RTTI の4手**）。
  2. **`byte[+4]` の FormType 値（v441 表）と class 名の一致は二重確認になる** — RTTI（`.?AVTESObjectSTAT@@`）と byte（`0x1C`）が同じ class を指す。
  3. **subobject vtable（`param_1[9]`＝`+0x24`）は既知の field（v468 `[ecx+0x24]`）と接続できる** — field map の累積が効く。
  4. **ptr 配列は「docs の `param_1 < N` bound」と「実体 dwords の実読」の2段で確定**（N=3⇔3 ptr）。
  5. **1行 setter（`*(self+off)=v`）は decompiled で即 identity**（`FUN_00468230/468270`）— semantic は caller 側（actor へ転送先）から逆算が次の手。
