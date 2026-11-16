# v460 — `*(manager+0x30)`＝NiTStringTemplateMap 候補・singleton image 不定・hash fn の裁定（V-460-1..3 完遂・旧残点群消化第4弾）

- 採番: **v460**（旧残点群消化・probe prefix **v460_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-460-1..3（v459 の残点）** — 1=`X=*(manager+0x30)` の class（L498845 `X.vtable+0x5C` 実際 owner）／2=`iVar13` class と `DAT_00b3cd7c` 実行時文字列／3=旧残点（V-405-4..5／V-406-2..6）への移行判断を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-460-1: `X = *(manager+0x30)` の class** | **[推定高]（候補特定）＋[未確定]（書手）** | **L498845 の3引数 call（`(&uStack_29,1,&cStack_2a)`）と整合する vtable slot `+0x5C` を4候補で実読**: **`NiTStringTemplateMap<NiTPointerMap<PBD,seq*>,seq*>` vt=`0xA797E4` の `[vt+0x5C]=FUN_006C4260`（3引数 fn: `mov eax,[esp+4]`（第1引数）→ `call 0x68F970` → `mov ebx,[esi+0x30]`）＝唯一の3引数 fn [確定(機械)]** → **`X`＝`NiTStringTemplateMap` instance [推定高]**（L498845 式の `Y=*(manager+0x30)` と `FUN_006c4c20` の `StoreTargets(*(manager+0x30))` が同一 field で、StoreTargets の param_2＝Y＝map instance）。**他候補の `+0x5C` は fn でない/thunk**: `NiTArray` vt=`0xA7977C` と `NiTMapBase` vt=`0xA7975C` は `[+0x5C]=FUN_004F0F60`（`add ecx,0xc; jmp 0x44d810`＝offset-adjust thunk）／`NiTPointerMap` vt=`0xA797A4` は `[+0x5C]=0xAC5F78`（data）→ **X はこれら以外 [確定(否定側)]**。**`manager+0x30` への書手 fn は [未確定]＝残点**（v458/v459 否定側継続） |
| **V-460-2: `iVar13` class（`"Bip02"` lookup 返り値）** | **[推定高]（候補）＋[部分確定]（string lookup 系）** | StoreTargets 内の `virtual[manager+0x7C]+0x4C(str)` の候補 vtable `+0x4C` を実読: **`NiTPointerMap<PBD,seq*>` vt=`0xA797A4` の `[vt+0x4C]=FUN_006C4F80`（`cmp byte[ecx+0x10],0; je; lea edx,[eax+1]`＝string walk/hash [確定(機械)]）**／**`NiTStringTemplateMap` vt=`0xA797E4` の `[vt+0x4C]=FUN_006C42B0`（`call 0x715820`→`test al,al`＝string 判定 [確定(機械)]）**／`NiTMapBase` vt=`0xA7975C` の `[vt+0x4C]=FUN_006A9060`（**`div dword[ecx+4]; mov eax,edx; ret 4`＝key hash modulo [確定(機械)]**）→ **`manager+0x7C` は string-keyed map [推定高]**、**`iVar13`（lookup 返り値）はその entry/値（`NiTPointerMap` の value＝`NiControllerSequence*`）だが `+0x1C` を持つ構造 [確定（v458）]** → **`iVar13` の正式 class は map entry の layout 次第で [未確定]＝残点** |
| **V-460-2: `DAT_00b3cd7c` の image 中身（実行時文字列）** | **[確定]（否定側）＋[未確定]（残点）** | **image 上の `[0xB3CD7C]` は `0xC3FD4D24`＝vtable 範囲外・fn 範囲外、first 16 dwords が乱数風（`0xC3FD4D24 0x868DF2C9 0xBBFDC3F6 ...`）→ 静的 vtable/静的データでない [確定(否定側)]** → **runtime 構築 object（または pack/暗号化 image 領域）[推定高]**。**実行時文字列の中身は image からは読めず [未確定]＝残点**（V-419-1 引継ぎ維持） |
| **V-460-3: 旧残点への移行** | **[確定]（方針）** | **V-461 から旧残点（V-405-4..5／V-406-2..6）へ軸を移す [確定（作業方針）]**（`X` 書手／`iVar13` entry layout／`0xB3CD7C` 実行時文字列は低優先で V-462+ に併走） |

## §2 証拠（v460_1）

### §2.1 singleton image（v460_1a）
```
[0xB3CD7C] = 0xC3FD4D24   (not in vtable range 0xA28000..0xB00000, not in .text)
first 16 dwords:
  0xC3FD4D24 0x868DF2C9 0xBBFDC3F6 0x3C5E912F 0x1FF2C5F3 0x09CD1449
  0xBE719361 0x0859FB11 0x28852658 0x9B902A48 0xF27EEF66 0x12847DB6
  0x7FF51BEC 0x1AAAD62D 0x90C0B5EB 0x58416F64
  -> random-looking: not static vtable / not ASCII  [否定側]
```

### §2.2 candidate vtable slots（v460_1b/c）
```
vt=0xA7977C .?AV?$NiTArray@V?$NiPointer@VNiControllerSequence@@@@@@
  [+0x4C]=0x7DAED0 (FUN_007daed0: mov dl,[esi]; test; je = string walk)  owning=FUN_007daed0
  [+0x5C]=0x4F0F60 (add ecx,0xc / jmp 0x44d810 = thunk)
vt=0xA7975C .?AV?$NiTMapBase@...PBDPAVNiControllerSequence@@@@
  [+0x4C]=0x6A9060 (FUN_006a9060: mov eax,[esp+4]; xor edx,edx; div dword[ecx+4];
                     mov eax,edx; ret 4  = key hash % bucket)  [確定(機械)]
  [+0x5C]=0x4F0F60 (thunk)
vt=0xA797A4 .?AV?$NiTPointerMap@PBDPAVNiControllerSequence@@@@
  [+0x4C]=0x6C4F80 (FUN_006c4f30+0x50: cmp byte[ecx+0x10],0; je; lea edx,[eax+1] = string walk)
  [+0x5C]=0xAC5F78  -> data (not fn)   [否定側]
vt=0xA797E4 .?AV?$NiTStringTemplateMap@V?$NiTPointerMap@...@@PAVNiControllerSequence@@@@
  [+0x4C]=0x6C42B0 (FUN_006c42b0: call 0x715820; test al,al = string check)
  [+0x5C]=0x6C4260 (FUN_006c4200+0x60: mov eax,[esp+4]; push ebx/esi/edi/eax;
                     mov esi,ecx; call 0x68f970; mov ebx,[esi+0x30]  = 3-arg fn!)  [確定(機械)]
```

## §3 残点 **V-461-1..3**
1. 【低】**旧残点の消化開始** — **V-405-4..5**（palette insert／unknown interpolator idx35）／**V-406-2..6** を優先順に着手（V-460-3 の方針どおり）。
2. 【低】**`manager+0x30` 書手 fn**（NiTStringTemplateMap insert の caller census）と **`iVar13` entry layout**（`+0x1C`＝seq value の map entry 構造）。
3. 【低】**`DAT_00b3cd7c` 実行時文字列**（V-419-1 引継ぎ・runtime 構築 object の中身は動的解析要）。

## §4 検証・自己批評
- probe: **v460_1 の1本 EXIT=0**（V-460-1 は `X`＝NiTStringTemplateMap [推定高]・書手は残点／V-460-2 は string lookup 系3 fn を実読・entry layout は残点／V-460-3 方針確定）。
- **決定打の作り**: (i) **L498845 の3引数 call という「arity 照合」で4候補の `+0x5C` を一斉実読** → 唯一の3引数 fn（`FUN_006C4260`）が `NiTStringTemplateMap` と即決 → (ii) **`+0x4C`（string lookup）も同一4候補で実読**（`div`＝hash modulo が決定打）→ (iii) **image 中身を読めば静的 object か否かが即判定**（乱数＝runtime 構築）。
- 妥当性チェック: ①`FUN_006C4260` の `mov eax,[esp+4]`⇔L498845 の第1引数 `&uStack_29` — ②`div [ecx+4]`⇔NiTMapBase の bucket 数フィールド（`+4`）— ③`lea edx,[eax+1]`⇔string の `+1`（NUL 送り）— ④image 乱数⇔`FUN_00700010(&DAT_00b3cd7c)` が runtime map として機能（静的 vtable なら image に vtable ptr が見えるはず）— ⑤`NiTPointerMap [+0x5C]`＝data⇔X がこれでない（3引数 call が必要）— **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **call site の arity（引数の数）が vtable slot の同定に効く** — L498845 の「3引数」が4候補の `+0x5C` を1 probe で絞った（**fn の signature を先に確定してから slot を当てる**）。
  2. **hash function の機械形（`div [reg+N]`→余り返し）は map class の即同定素材** — `NiTMapBase` の `+0x4C` が hash modulo と分かれば、その class の `+0x4C` が key→index の役割と分かる。
  3. **image 中身の1 dword で「静的 vtable か runtime object か」が即判定できる** — 0xA28000..0xB00000（vtable 範囲）外＋乱数＝runtime（**static object なら image に ptr が残る**）。
  4. **`[vt+off]` が fn でない（data/thunk）ことも重要的な否定側証拠** — thunk（`add ecx,N; jmp`）と data（ASCII/不定値）を区別して「3引数 fn だけが残る」。
  5. **候補列挙は「既知 TD から vtable を引く」方が forward 法（ctor→vtable）より速い**（v457 で TD と vtable を既に特定済みなので、本 probe は slot 読みだけ）。
