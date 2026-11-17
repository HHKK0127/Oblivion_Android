# v527 — `TESForm` vtable slot `+0x08`＝component 複写／`+0x0C`＝component 比較（cast 先 TD＝`BaseFormComponent`）・`+0x10`＝dtor（`FUN_0046C280` で manager 解除）・`0x60D0A0`＝裸 `ret` stub の裁定（V-527-1..3 完遂・旧残点 V-406-3 継続第57弾）

- 採番: **v527**（旧残点群消化・probe prefix **v527_1..3 全3本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-527-1..3（v526 残点）** — 1=`FUN_0046B800`/`FUN_0046B870`/`FUN_0046C620`（`TESForm` vtable `+0x08`/`+0x0C`/`+0x10`）identity と `0x60D0A0` stub 実体／2=`DeleteForm` の `this` identity と ECX 追跡／3=引継ぎ残り。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・RTTI（COL/TD 実読）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-527-1: `0x60D0A0`（vtable `+0x00`/`+0x04`）の実体** | **[確定]（生バイト）** | **`0x60D0A0: C3`（単一 `ret`）＋`int3` 埋め → 引数なしで何もしない no-op 仮想関数 [確定]**（`ret N` でない点も確認＝引数契約なし）。`TESForm` vtable の `+0x00`/`+0x04` と `+0x24`、component view の `+0x04` がこの stub。 |
| **V-527-1: `FUN_0046B800`（vtable `+0x08`）identity** | **[確定]（命令全読）→ component 複写** | **`0x46B800`…`+0x5F`: `arg = [esp+4]` → `RTDynamicCast(arg, 0, &TD_TESForm(0xB02F9C), &TD_BaseFormComponent(0xB031EC), 0)` → 成功時 `if (((comp+8 >> 1) & 1) != ((this+8 >> 1) & 1)) virtual[+0x90](bit)` → `*(byte*)(this+4) = *(byte*)(comp+4)`（**type byte 複写**）→ `edx = ((this+8) ^ (comp+8)) & 0x4000; edx ^= comp+8; *(uint*)(this+8) = edx`（**flags は comp から取り bit14 だけ this を保持**）→ `ret 4` → `FUN_0046B800`＝`TESForm::CopyComponentFrom(BaseFormComponent*)` 相当 [確定]**。 |
| **V-527-1: `FUN_0046B870`（vtable `+0x0C`）identity** | **[確定]（命令全読）→ component 比較（1＝相違）** | **`0x46B870`…`: cast(arg→BaseFormComponent)` → `arg == 0` なら `al = 1` で return（**引数なし＝相違扱い**）→ type byte（`+4`）不一致で `al = 1` → `FUN_00404E50(this)` / `FUN_00404E50(comp)`（getter）比較 → 両方非0なら `virtual[+0xD4]` の戻りを `FUN_0098262D`（文字列比較系）で比較 → 不一致 `al = 1` → 最後に `(this+8) ^ (comp+8)` と `0xFFFFBFF4` の AND が非0なら `al = 1`（**bit14 と下位フラグを比較対象から除外**）→ `FUN_0046B870`＝`TESForm::IsComponentDifferent(BaseFormComponent*)`（1＝異なる／0＝同一）[確定]**。 |
| **V-527-1: `FUN_0046C620`（vtable `+0x10`）identity** | **[確定]（本文）→ scalar deleting destructor** | **`FUN_0046C620(param_1, byte param_2)`: `FUN_0046C280()`（base dtor）→ `if (param_2 & 1) FUN_00401F20(param_1)`（this free）→ `return param_1` → `TESForm` の scalar deleting destructor [確定]**（v505/v523 の dtor pattern と同型）。 |
| **V-527-1: `FUN_0046C280`（dtor 本体）** | **[確定相当]** | **`0x46C280`: `*(this) = &vtable 0xA3BE3C`（TESForm vtable 復元）→ `FUN_0046B2C0()`（reset・v514）→ `+0x14` の node 連鎖を `FUN_00401F20` で全 free → `*(this+0x10) = 0` → `if (!((flags >> 0xe) & 1) && (DAT_00B33B00 != 0)) { FUN_0045B780(0, this); FUN_00453940(this); }`（**bit14 が立っていなければ global manager から解除**）[確定相当]**。 |
| **V-527-1: cast 先 TD `0xB031EC`** | **[確定]（TD 実読）→ `.?AVBaseFormComponent@@`** | **`0xB031EC` の TD 実読: `80 3e aa 00｜00 00 00 00｜..BaseFormComponent@@` → `.?AVBaseFormComponent@@` → `+0x08`/`+0x0C` の引数は `BaseFormComponent*`＝26 component interface の共通基底 [確定]**。RTTI の cast 元が `TESForm`・先が `BaseFormComponent` なので **`BaseFormComponent` は `TESForm` の階層に属する（downcast）[確定相当]**。 |
| **V-527-1: `TESForm` vtable 全 slot（0xA3BE3C 起点）** | **[確定]（.rdata 実読）** | **`+0x00`=`0x60D0A0`(ret stub)／`+0x04`=`0x60D0A0`／`+0x08`=`0x46B800`(CopyComponent)／`+0x0C`=`0x46B870`(IsComponentDifferent)／`+0x10`=`0x46C620`(dtor→`0x46C280`)／`+0x14`=`0x46C2E0`(→`FUN_0046B090`・v526)／`+0x18`=`0x46C2F0`(→`FUN_0046B170`・v526)／`+0x1C`=`0x6FDFF0`（`mov al,1; ret 4`＝常に真）／`+0x20`=`0x46B340`／`+0x24`=`0x60D0A0`／`+0x28`=`0x412960`／`+0x2C`=`0x46B4C0`／`+0x30`=`0x46B510`／`+0x34`=`0x46B500`／`+0x38`=`0x46C360`／`+0x3C`=`0x68F970`／`+0x40`=`0x46ABF0`／`+0x44`=`0x46AC10`／`+0x48`=`0x46AC30`／`+0x4C`=`0x46AC50`／`+0x50`=`0x46B5E0`／`+0x54`=`0x46B600`／`+0x58`=`0x60CF60`／`+0x5C`=`0x60CF60`／`+0x60`=`0x68F970`／`+0x64`=`0x68F970`／`+0x68`=`0x6FE080`／`+0x6C`=`0x60D0A0`／`+0x70`=`0x412970`／`+0x74`=`0x46C700` [確定]**。 |
| **V-527-2: `DeleteForm` の `this` identity** | **[未確定]** | ECX 供給元は docs に現れず（Ghidra の暗黙引数）。`FUN_0046C280` が `DAT_00B33B00`（global manager）へ解除通知する構造から **`this`＝manager に登録される TESForm 側 object [推定]** まで。確定は残点。 |
| **V-527-3: 引継ぎ残り** | **[未確定]** | predicate slot35 英名・新 FourCC 意味（残点継続） |

## §2 証拠

### §2.1 `0x60D0A0` 共通 stub（v527_1）
```
bytes: C3 CC CC CC ... -> ret / int3...
=> shared no-op virtual (not "ret 4", so zero-argument contract)
```

### §2.2 `0x46B800` = CopyComponentFrom（v527_3）
```
46B800: eax = [esp+4]                       ; BaseFormComponent*
        RTDynamicCast(eax, 0, &TD_TESForm(0xB02F9C), &TD_BaseFormComponent(0xB031EC), 0)
46B825: eax = [comp+8]; ecx = [this+8]; shr 1; and 1
46B834: cmp cl, al; je skip
46B83B: call [vtbl+0x90](bit)               ; side effect on bit change
46B845: byte [this+4] = byte [comp+4]       ; type byte copy
46B84E: edx = ([this+8] ^ [comp+8]) & 0x4000; edx ^= [comp+8]; [this+8] = edx
46B85F: ret 4
```

### §2.3 `0x46B870` = IsComponentDifferent（v527_3）
```
46B870: cast(arg -> BaseFormComponent); if (comp == 0) return 1
46B89C: if (byte[this+4] != byte[comp+4]) return 1
46B8A6: FUN_00404E50(this) vs FUN_00404E50(comp)  (getter)
46B8DA: std::strcmp-like FUN_0098262D on virtual[+0xD4] results -> != 0 => return 1
46B8F6: (([this+8] ^ [comp+8]) & 0xFFFFBFF4) != 0 -> setne -> return 1
=> 1 = different, 0 = same (bit14 excluded from comparison)
```

### §2.4 `0x46C280` = TESForm dtor body（v527_3）
```
46C283: [this] = 0xA3BE3C            ; vtable restore
46C289: FUN_0046B2C0()               ; reset (v514)
46C295: loop  free([this+0x14])      ; node chain free
46C2AC: [this+0x10] = 0
46C2B6: if (!(([this+8] >> 0xe) & 1) && DAT_00B33B00 != 0) {
46C2CA:   FUN_0045B780(0, this); FUN_00453940(this);   ; global manager detach
46C2DC: ret
```

## §3 判定の内訳

- **V-527-1 完遂**: `0x60D0A0`＝裸 `ret` stub [確定]、`+0x08`＝component 複写 [確定]、`+0x0C`＝component 相違判定 [確定]、`+0x10`＝dtor＋`FUN_0046C280` 本体 [確定／確定相当]、cast 先 TD＝`BaseFormComponent` [確定]、`TESForm` vtable 全 30 slot を実読 [確定]。
- **V-527-2**: `this`＝manager 登録 object [推定] まで（ECX 供給元は残点）。
- **V-527-3**: 残点継続。

## §4 残点（V-528-1..3）

1. `TESForm` vtable 残り slot の identity（`+0x20`＝`0x46B340`／`+0x2C..+0x34`＝`0x46B4C0`/`0x46B510`/`0x46B500`／`+0x38`＝`0x46C360`／`+0x40..+0x4C`＝`0x46ABF0`/`0x46AC10`/`0x46AC30`/`0x46AC50`／`+0x50`＝`0x46B5E0`／`+0x54`＝`0x46B600`／`+0x68`＝`0x6FE080`／`+0x74`＝`0x46C700`）【中】
2. `DAT_00B33B00`（global manager）identity＋`FUN_0045B780`/`FUN_00453940`（登録解除経路）＋`DeleteForm` の `this` 追跡【低】
3. 引継ぎ残り（predicate slot35 英名・新 FourCC 意味）【低】

## §5 後続への申し送り

- **`TESForm` の component 契約が確定**: vtable `+0x08`＝`CopyComponentFrom`、`+0x0C`＝`IsComponentDifferent`、`+0x14`/`+0x18`＝26 slot component 表の load/unload 系（v526）、`+0x10`＝dtor。**26 component interface（`TESFullName` 他）は `BaseFormComponent` 派生で、`TESForm` との相互運用はこの 2 virtual が担う**。
- `FUN_00404E50`（getter）と `FUN_0098262D`（文字列比較）は component 比較の部品。次段で identity を詰める候補。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
