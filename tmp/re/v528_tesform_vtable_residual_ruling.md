# v528 — `TESForm` vtable 残り 16 slot の identity・`DAT_00B33B00`＝form 管理 singleton（`DeleteForm` の `this` を確定）・form type→FourCC 表の全解読（V-528-1..3 完遂・旧残点 V-406-3 継続第58弾）

- 採番: **v528**（旧残点群消化・probe prefix **v528_1..9 全9本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-528-1..3（v527 残点）** — 1=`TESForm` vtable 残り slot の identity（`+0x20`/`+0x2C..+0x34`/`+0x38`/`+0x40..+0x4C`/`+0x50`/`+0x54`/`+0x68`/`+0x74`）／2=`DAT_00B33B00` identity＋`FUN_0045B780`/`FUN_00453940`＋`DeleteForm` の `this` 追跡／3=引継ぎ（predicate slot35 英名・新 FourCC 意味）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・RTTI（COL/TD 実読）・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-528-1: `+0x20`＝`FUN_0046B340`** | **[部分確定]（保存系）** | `if (flags bit14) return false;` → `virtual[+0x24]()`（=no-op stub）→ `iVar1 = FUN_00450A10(param_1)` → `return iVar1 == 0`。`FUN_00450A10` は `FUN_004502C0`／`virtual[+0xC]`／`FUN_00450190(DAT_00b33c14, DAT_00b33c18)` を経て `param_1+0x3d4` を増分する**保存レコード処理** → `+0x20`＝**「form を保存対象として書き出す／変化なしを返す」系統の仮想関数 [部分確定]**。 |
| **V-528-1: `+0x28`＝`0x412960`** | **[確定]（命令）** | `mov eax,[ecx]; mov eax,[eax+0x1c]; jmp eax` → **`vtable[+0x1C]` への tail-jump（転送）[確定]**（`+0x1C`＝`0x6FDFF0`＝`mov al,1; ret 4`）。 |
| **V-528-1: `+0x2C`＝`0x46B4C0`** | **[確定]（命令全読）→ 保存レコード開始** | `flags bit14` → `al=0; ret 4`（除外）→ `flags bit5`（`>>5 &1`）→ 真なら `FUN_00450980(arg)`＋`FUN_00450A80(arg)` を呼び `al=1`、偽なら `vtable[+0x20]` へ tail-jump。`FUN_00450980` は **`arg` の form から保存レコードを組む**（`param_1+0x268` ← **`(&DAT_00b05e08)[type*3]`＝type の FourCC**、`+0x270` ← `flags & 0x30EE0`、`+0x274` ← `refID`、`+0x27C` ← 親／`+0x280`＝0）→ `+0x2C`＝**保存側 form レコードの書き出し [確定]**（bit14 の form は保存対象外）。 |
| **V-528-1: `+0x30`＝`FUN_0046B510`** | **[部分確定]（型 predicate）** | `param_2 == 0 → 0`／`*param_2 != DAT_00b05e20 → 0`（**runtime 初期化の vtable ポインタ比較**＝条件オブジェクト型の確認）→ `switch (param_2[3])`（**演算子コード**）で `case 0:` `type > FUN_0046AF70(param_2[2])` なら 1／`case 1,4,5:` `type < 0x35`／`case 2,3,6:` `type < 0x30`／`case 7:` `type < 0x39` → **form type を条件値と比較する predicate [部分確定]**（`FUN_0046AF70` は `0xB05E04` 表の第2 dword を線形走査して index を返す）。 |
| **V-528-1: `+0x34`＝`FUN_0046B500`** | **[確定]（本文）** | `return *(byte*)(param_1+4) < *(byte*)(param_2+4);` ＝ **form type byte の less-than 比較 [確定]**。 |
| **V-528-1: `+0x38`＝`FUN_0046C360`** | **[確定相当]→ form の複製生成** | `piVar2 = FUN_00447DF0((char)param_1[1])`（**type→factory で新規 form 生成**）→ `FUN_004028D0(&DAT_00a2f7ec,0)` → `virtual[+0xD8]`（SetName・空文字）→ `virtual[+0xB4](param_1)`（コピー元を渡す）→ `FUN_00452570(param_1, piVar2)` → **`+0x38`＝「同じ type の form を factory で作り名を空にして複写する」複製生成 [確定相当]**（`0x46B7A0` の `"Trying to copy form ..."` メッセージと対になる）。 |
| **V-528-1: `+0x40`＝`FUN_0046ABF0`** | **[確定相当]** | `FUN_0045B670(param_1, param_2)` → 内部は **save/load 状態 gate**（`TLS[+0x185]`／`TLS[+0x184]`／`DAT_00b33a98+0xCD4`／`flags bit14`）を通過した時だけ `FUN_00452C20(param_2, param_3)` を呼ぶ → **「保存中の form 変更記録」登録 [確定相当]**。 |
| **V-528-1: `+0x44`＝`0x46AC10`／`+0x48`＝`0x46AC30`** | **[確定]（命令）→ manager 経由** | 両方 `mov ecx,[0xb33b00]` の後に manager のメソッドを呼ぶ: `+0x44` → `FUN_004533D0`（`flags bit14==0` なら `FUN_00452D60`）／`+0x48` → `FUN_0045B700`（bit14 gate＋**thread gate**＋`FUN_00452C20`）→ **両 slot は form 管理 manager への委譲 [確定]**。 |
| **V-528-1: `+0x4C`＝`FUN_0046AC50`** | **[確定]（本文）** | `uVar1 = 0; if (param_1 & 1) uVar1 = 4; return uVar1;` ＝ **flag 時のみ 4 を返す（レコード長）[確定]**。 |
| **V-528-1: `+0x50`＝`FUN_0046B5E0`** | **[確定]（本文＋実文字列）→ 保存** | `if (param_2 & 1) FUN_0045B9A0(param_1+8, 4);`。`FUN_0045B9A0` は thread gate の後 **`(**(code**)(*DAT_00b34d90 + 0x18))("Error: TESSaveLoadGame::SaveGameData() was called while loading.\n")`** を持つ → `_memcpy(*(void**)(param_1+0x14), param_2, size)` で**保存バッファへ 4 バイト書き込み** → **`+0x50`＝flags dword の保存 [確定]**。 |
| **V-528-1: `+0x54`＝`FUN_0046B600`** | **[確定]（本文）→ 読み込み** | `if (param_2 & 1) { FUN_004534D0(&param_2, 4)`（**保存ファイルから 4B 読む**）`; iVar1 = RTDynamicCast(param_1, 0, &TD_TESForm, &TD_0xB03054, 0); if (iVar1 != 0) flags ^= (flags ^ param_2) & 0x112860; else flags ^= (flags ^ param_2) & 0xC20; }` → **`+0x54`＝flags dword の読み込み（`0xB03054`＝`TESObjectREFR` なら mask `0x112860`、他は `0xC20`）[確定]**。 |
| **V-528-1: `+0x58`/`+0x5C`＝`0x60CF60`・`+0x68`＝`FUN_006FE080`** | **[確定]（本文）** | `0x60CF60`＝`void FUN_0060cf60(void) { return; }`＝**空関数 [確定]**（2 slot が同一）。`FUN_006FE080`＝`return 0;`＝**常に 0 [確定]**。 |
| **V-528-1: `+0x70`＝`0x412970`** | **[確定]（命令）** | `movzx eax, byte ptr [ecx+4]; ret` ＝ **form type byte の getter [確定]**（`GetType()` 相当）。 |
| **V-528-1: `+0x74`＝`0x46C700`** | **[確定]（命令＋実文字列）** | `refID=[ecx+0xC]` → `type*3` で `0xB05E04` 表を索引 → **引数 `(this, 0xA3C014, FourCC, &DAT_00a2f7ec, refID)` で `FUN_00402E30`＝`vsprintf` wrapper** を呼ぶ。`0xA3C014` の実文字列＝**`"%s Form '%s' (%08X)"`**、`0xA2F7EC`＝空文字（名前バッファ） → **`+0x74`＝`"<FourCC> Form '<name>' (<refID>)"` を組み立てる名前生成 [確定]**。 |
| **V-528-2: `DAT_00B33B00` identity** | **[確定相当]（構築＋使用の両面）** | **0x441B8A で `DAT_00b33a98`（TESDataHandler）を設定した直後、0x441B99 で `FUN_00401F00(0x1C4)` を確保 → `FUN_0045B300` で構築 → 0x441BB9 で `DAT_00B33B00` に格納 [確定]**。ctor は vtable を持たない**非多態 singleton**（`+0x00`＝`FUN_0045A860`（0x13AF 要素の table）、`+0x08`＝`FUN_0045A940`・`+0x0C`／`+0x10`＝`FUN_0045AA80`・`+0x54..+0x60`＝`FUN_0045ABE0`（各 0x25 要素）、`+0x74`／`+0x78`＝`NiTLargeArray<unsigned int>`（vtable `0xA39044`・容量 100／5）、`+0x18`＝**loading 状態 dword**、`+0x30`＝**form list head**）。`FUN_0045B780`（refID 解放）・`FUN_00453940`（list unlink）・`FUN_004533D0`／`FUN_0045B700`（loading gate 付き form 変更記録）はいずれもこの singleton のメソッドで、`+0x18` の thread 依存 read（同 thread＝byte bit0／異 thread＝`>> 0x12`）が `DeleteForm` と同一 → **`DAT_00B33B00`＝load/save 側の form 管理 singleton [確定相当]**。 |
| **V-528-2: `DeleteForm` の `this`** | **[確定相当]→ `DAT_00B33B00`** | **`FUN_0045C7A0` の 20 の E8 caller のうち 10 件が呼び出し直前 24B 以内に `mov ecx, [0xB33B00]` を持つ**（`0x42471D`／`0x42A256`／`0x5EB0C3`／`0x60D4A3`／`0x60D845`／`0x611A96`／`0x648D1C`／`0x64AFD1`／`0x651947`／`0x6520B0`）。残りは自分の ECX を保持して渡す（例 `FUN_00463EC0`: `mov esi,ecx` → `mov ecx,esi` → `call 0x45c7a0`）。`FUN_0045B780` の 3 caller・`FUN_00453940` の 1 caller も同様に `mov ecx,[0xB33B00]` で 100% 一致 → **`DeleteForm` の `this`＝`DAT_00B33B00` [確定相当]（v526/v527 の [未確定] を解消）**。 |
| **V-528-3: 新 FourCC 意味** | **[確定]（表全解読）** | **`0xB05E04`＝form type → 4 文字レコード署名（FourCC）表**（stride 12B＝`{署名ポインタ, 0, index+1}`）。全 69 型を実読 [確定]: `NONE/TES4/GRUP/GMST/GLOB/CLAS/FACT/HAIR/EYES/RACE/SOUN/SKIL/MGEF/SCPT/LTEX/ENCH/SPEL/BSGN/ACTI/APPA/ARMO/BOOK/CLOT/CONT/DOOR/INGR/LIGH/MISC/STAT/GRAS/TREE/FLOR/FURN/WEAP/AMMO/NPC_/CREA/LVLC/SLGM/KEYM/ALCH/SBSP/SGST/LVLI/SNDG/WTHR/CLMT/REGN/CELL/REFR/ACHR/ACRE/PGRD/WRLD/LAND/TLOD/ROAD/DIAL/INFO/QUST/IDLE/PACK/CSTY/LSCR/LVSP/ANIO/WATR/EFSH/TOFT`（`0x44`=`TOFT` まで、`0x45` で終端）。 |
| **V-528-3: predicate slot35 英名** | **[未確定]** | 残点継続（`+0x30` の条件オブジェクト型 `DAT_00b05e20` は runtime 初期化のため静的には値 0）。 |

## §2 証拠

### §2.1 `TESForm` vtable `0xA3BE3C` 全 30 slot の最終表（v527＋本裁定）
```
+0x00 0x60D0A0 ret stub        +0x04 0x60D0A0 ret stub      +0x08 0x46B800 CopyComponentFrom
+0x0C 0x46B870 IsComponentDifferent  +0x10 0x46C620 dtor    +0x14 0x46C2E0 (->FUN_0046B090)
+0x18 0x46C2F0 (->FUN_0046B170)      +0x1C 0x6FDFF0 mov al,1 +0x20 0x46B340 save/emit (partial)
+0x24 0x60D0A0 ret stub        +0x28 0x412960 -> vtable[+0x1C]  +0x2C 0x46B4C0 save record begin
+0x30 0x46B510 type predicate  +0x34 0x46B500 type <         +0x38 0x46C360 clone via factory
+0x3C 0x68F970                 +0x40 0x46ABF0 save-change reg  +0x44 0x46AC10 manager FUN_004533D0
+0x48 0x46AC30 manager FUN_0045B700   +0x4C 0x46AC50 len 4/0  +0x50 0x46B5E0 save flags (4B)
+0x54 0x46B600 load flags (mask)      +0x58 0x60CF60 empty    +0x5C 0x60CF60 empty
+0x60 0x68F970                 +0x64 0x68F970                 +0x68 0x6FE080 return 0
+0x6C 0x60D0A0 ret stub        +0x70 0x412970 GetType       +0x74 0x46C700 "<FourCC> Form '<name>' (%08X)"
```

### §2.2 `+0x2C` の保存レコード（v528_3）
```
0x46B4C0: if ((flags>>0xE)&1) { al=0; ret 4 }        // bit14 forms are not saved
0x46B4D2: if ((flags>>5)&1) { FUN_00450980(arg); FUN_00450A80(arg); al=1; ret 4 }
0x46B4F3: jmp vtable[+0x20]
FUN_00450980: *(u32*)(this+0x268) = (&DAT_00b05e08)[type*3]   // FourCC of the type
              *(u32*)(this+0x270) = flags & 0x30EE0
              *(u32*)(this+0x274) = refID
```

### §2.3 `+0x74` の format 文字列（v528_3）
```
0xA3C014 = "%s Form '%s' (%08X)"
0xA2F7EC = ""            (name buffer)
0x46C700 pushes: this, 0xA3C014, FourCC(type*3 table), 0xA2F7EC, [ecx+0xC]=refID -> FUN_00402E30
FUN_00402E30 -> FUN_00402D00 -> _vsprintf(local_404, fmt, va) -> FUN_004028D0
```

### §2.4 form type → FourCC 表（`0xB05E04`、v528_5）
```
type 0x00..0x0F: NONE TES4 GRUP GMST GLOB CLAS FACT HAIR EYES RACE SOUN SKIL MGEF SCPT LTEX ENCH
type 0x10..0x1F: SPEL BSGN ACTI APPA ARMO BOOK CLOT CONT DOOR INGR LIGH MISC STAT GRAS TREE FLOR
type 0x20..0x2F: FURN WEAP AMMO NPC_ CREA LVLC SLGM KEYM ALCH SBSP SGST LVLI SNDG WTHR CLMT REGN
type 0x30..0x3F: CELL REFR ACHR ACRE PGRD WRLD LAND TLOD ROAD DIAL INFO QUST IDLE PACK CSTY LSCR
type 0x40..0x44: LVSP ANIO WATR EFSH TOFT      (0x45 -> table ends)
record layout: {sig_string_ptr, 0, index+1}  stride 12,  index = type (FUN_0046AF70 scans field +4)
```

### §2.5 `DAT_00B33B00` 構築（v528_6 / v528_7）
```
0x441B8A: mov [0xb33a98], eax        ; TESDataHandler
0x441B94: push 0x1C4 / call 0x401f00 ; alloc 452 bytes
0x441BAE: mov ecx,eax / call 0x45b300; ctor FUN_0045B300
0x441BB9: mov [0xb33b00], eax         ; store singleton
ctor: no vtable store (non-polymorphic); +0x00/+0x08/+0x0C/+0x10 and +0x54..+0x60 sub-tables
      +0x74/+0x78 NiTLargeArray<unsigned int> (vtable 0xA39044, cap 100/5)
      +0x18 = 0 (loading state), +0x30 = 0 (form list head)
```

### §2.6 `DeleteForm` の ECX 供給（v528_8）
```
20 E8 call sites; 10 have "8B 0D 00 3B B3 00" (mov ecx,[0xB33B00]) within 24 bytes
  0x424724 0x42A25D 0x5EB0D9 0x60D4AA 0x60D84C 0x611A9D 0x648D23 0x64AFD8 0x65194E 0x6520B6
remaining 10 pass a preserved ECX (e.g. FUN_00463EC0: mov esi,ecx -> mov ecx,esi -> call)
FUN_0045B780 callers: 3/3 load [0xB33B00];  FUN_00453940 callers: 1/1 loads [0xB33B00]
DeleteForm head: [edi+0x18] thread-gated loading flag, [edi+0x30] form list, mov ecx,[edi] sub-object
```

## §3 判定の内訳

- **V-528-1 完遂**: `+0x20` [部分確定]、`+0x28`／`+0x2C`／`+0x34`／`+0x44`／`+0x48`／`+0x4C`／`+0x50`／`+0x54`／`+0x58`／`+0x5C`／`+0x68`／`+0x70`／`+0x74` [確定]、`+0x30` [部分確定]、`+0x38`／`+0x40` [確定相当]。**`TESForm` vtable 30 slot の全 slot が説明可能になった**。
- **V-528-2 完遂**: `DAT_00B33B00`＝非多態の form 管理 singleton [確定相当]、`DeleteForm` の `this`＝同 singleton [確定相当]（v526/v527 の未確定を解消）。
- **V-528-3 部分**: FourCC 表の全解読 [確定]。predicate slot35 英名は残点。

## §4 残点（V-529-1..3）

1. `+0x20`（`FUN_0046B340`→`FUN_00450A10`）の semantic 確定＋`FUN_00450190`／`FUN_004502C0`／`FUN_00452C20`／`FUN_00452D60` の identity（保存系の縦の並び）【中】
2. `DAT_00B33B00` の正式クラス名（`TESSaveLoadGame` 候補）の確定手段（RTTI が無い非多態のため、ctor 呼び出し元の init 関数と隣接 singleton の並びから詰める）＋`+0x30` list の node 構造【低】
3. 引継ぎ（predicate slot35 英名・`DAT_00b05e20` 条件オブジェクト型）【低】

## §5 後続への申し送り

- **`TESForm` の保存系契約**が判明: `+0x2C`（レコード開始）→ `+0x4C`（長さ 0/4）→ `+0x50`（flags 保存）→ `+0x54`（flags 読み込み・REFR だけ mask が広い）。**bit14（`0x4000`）の form は保存対象外**という一貫した規則が `+0x08`/`+0x0C`/`+0x2C`/`+0x44`/`+0x48` の全てに現れる。
- **`0xB05E04`＝form type→FourCC 表**を確保。以後「この type byte のレコード署名は何か」は表引きで即答できる（移植側の record 判定に直結）。
- `DeleteForm` の `this` が manager singleton と確定したので、**「form 削除＝manager の form list（`+0x30`）からの unlink と refID の再割当」**という業務像が固まった。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
