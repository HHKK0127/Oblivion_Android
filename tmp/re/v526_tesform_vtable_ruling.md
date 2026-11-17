# v526 — `FUN_0046B090`/`FUN_0046B170`＝`TESForm` の vtable slot `+0x14`/`+0x18`（仮想関数）・`FUN_00466D80`/`DA0`/`DD0`/`E10`＝component view の vtable slot `+0`/`+4`/`+8`/`+0xC` 呼び出し・`DeleteForm` caller 経路の裁定（V-526-1..3 完遂・旧残点 V-406-3 継続第56弾）

- 採番: **v526**（旧残点群消化・probe prefix **v526_1..2 全2本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-526-1..3（v525 残点）** — 1=`FUN_00466D80`/`FUN_00466DA0` identity と `FUN_0046C2E0`/`FUN_0046C2F0` thunk 接続／2=`param_1` identity と caller 分離／3=引継ぎ残り。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE データ・decompiled.c・RTTI（COL/TD 実読）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-526-1: `FUN_0046C2E0`/`FUN_0046C2F0` thunk の接続** | **[確定]（生バイト）** | `0x46C2E0: jmp 0x46b090`／`0x46C2F0: jmp 0x46b170`（各 2 バイト＋`int3` 埋め）→ **`thunk_FUN_0046B090`/`thunk_FUN_0046B170` は裸の jmp thunk [確定]**。 |
| **V-526-1: 両 thunk の vtable 所属** | **[確定]（RTTI: COL→TD 実読）** | `.rdata` の dword 出現を全走査し、直前に有効な COL（signature 0＋TD 実読）を持つ列を vtable 起点として逆引き → **`0xA3BE3C`（class `.?AVTESForm@@`）の `+0x14`＝`0x46C2E0`／`+0x18`＝`0x46C2F0` [確定]**。同 dword は派生 vtable にも同 slot で出現（`.?AVMagicItemObject@@` `0xA32364`／`.?AVEffectSetting@@` `0xA32B14`／`.?AVMagicItemForm@@` `0xA33DBC`／`.?AVTESRegion@@` `0xA3FEE4`／`.?AVTESKey@@` `0xA42A4C`／`.?AVTESAmmo@@` `0xA4068C` 等）→ **両 fn は `TESForm` の仮想関数（slot 5／6）で階層全体に継承される [確定]**。 |
| **V-526-1: `TESForm` vtable slot map** | **[確定]（.rdata 実読）** | **`0xA3BE3C`（TESForm）: `+0x00`＝`0x60D0A0`（共通 stub）／`+0x04`＝`0x60D0A0`／`+0x08`＝`0x46B800`／`+0x0C`＝`0x46B870`／`+0x10`＝`0x46C620`／`+0x14`＝`0x46C2E0`（→`FUN_0046B090`）／`+0x18`＝`0x46C2F0`（→`FUN_0046B170`）／`+0x1C`＝`0x6FDFF0` [確定]**。 |
| **V-526-1: `FUN_00466D80` / `FUN_00466DA0` identity** | **[確定]（本文）** | **`FUN_00466D80`: `for i in 0..0x19 { if (table[i] != 0) (**(code**)**(undefined4**)(table + i*4))(); }`＝component view の `vtable[+0x00]` を 26 回呼ぶ [確定]**。**`FUN_00466DA0`: 同ループで `(**(code**)(*(int**)(table + i*4) + 4))()`＝`vtable[+0x04]` を 26 回呼ぶ [確定]**。既裁定の `FUN_00466DD0`＝`vtable[+0x08]`、`FUN_00466E10`＝`vtable[+0x0C]`（比較）と合わせ、**component view の vtable slot `+0`/`+4`/`+8`/`+0xC` を巡回する4種のヘルパが揃った [確定]**。 |
| **V-526-1: component view 側 vtable の実読** | **[部分確定]** | **`.?AVTESFullName@@` `0xA322A0`: `+0x00`＝`0x5A6A20`／`+0x04`＝`0x60D0A0`／`+0x08`＝`0x46C840`／`+0x0C`＝`0x46C880`／`+0x10`＝`0xAB40A4`**、**`.?AVTESHealthForm@@` `0xA3C02C`: `+0x00`＝`0x46C9C0`／`+0x04`＝`0x60D0A0`／`+0x08`＝`0x46CA60`／`+0x0C`＝`0x46C9D0`**、**`.?AVTESValueForm@@` `0xA3C680`: `+0x00`＝`0x46C9C0`／`+0x08`＝`0x470480`／`+0x0C`＝`0x4704E0`／`+0x10`＝`0x470400`** → **`+0x04` に共通 stub `0x60D0A0` を持つ型が多く、`FUN_00466DA0`（`+4` 巡回）は多くの型で no-op 相当になる [部分確定]**（型ごとの差は slot 内容で説明可能）。 |
| **V-526-2: `DeleteForm` の引数経路（caller 分離）** | **[確定相当]** | **`FUN_00463EC0`（20 caller の1つ・site `0x463EF8`）: `iVar2 = FUN_0046b250(param_1, uVar1); if (iVar2 == 0) { FUN_00452df0(param_1, 1); } else { FUN_0045c7a0(iVar2); … }` → `DeleteForm` は「**id（refID）を渡して `FUN_0046B250` で form を引き当て、その結果を渡す**」経路から呼ばれる [確定相当]**（`FUN_0046B250` は `FUN_0055e000(param_1, &local_4)` の wrapper＝v520 の registry lookup）。 |
| **V-526-2: `DeleteForm` の `this`（`param_1`）identity** | **[未確定]** | `__thiscall` の ECX 供給元は docs 上 `FUN_0045c7a0(iVar2)` に現れず（Ghidra が暗黙引数を落としている）、TESDataHandler 候補 [推定] から前進なし。残点継続。 |
| **V-526-3: 引継ぎ残り** | **[未確定]** | predicate slot35 英名・新 FourCC 意味（残点継続） |

## §2 証拠

### §2.1 thunk 実バイト（v526_1 b0）
```
0x46C2E0: E9 AB ED FF FF   -> jmp 0x46B090   (thunk_FUN_0046b090)
0x46C2E5..: CC CC CC
0x46C2F0: E9 7B EE FF FF   -> jmp 0x46B170   (thunk_FUN_0046b170)
0x46C2F5..: CC CC CC
```

### §2.2 vtable 逆引き（v526_1 b）
```
dword 0x46C2E0 hits -> vtable 0xA32364 .?AVMagicItemObject@@  +0x14
                       vtable 0xA32B14 .?AVEffectSetting@@    +0x14
                       vtable 0xA33DBC .?AVMagicItemForm@@    +0x14
                       vtable 0xA3BE3C .?AVTESForm@@          +0x14   <== base class
                       vtable 0xA3FEE4 .?AVTESRegion@@        +0x14
                       vtable 0xA42A4C .?AVTESKey@@           +0x14
dword 0x46C2F0 hits -> same classes, +0x18
=> FUN_0046B090 / FUN_0046B170 are TESForm virtual methods (slots 5 and 6)
```

### §2.3 TESForm vtable `0xA3BE3C`（v526_2）
```
+0x00 0x60D0A0   +0x04 0x60D0A0   +0x08 0x46B800   +0x0C 0x46B870
+0x10 0x46C620   +0x14 0x46C2E0 (->FUN_0046B090)    +0x18 0x46C2F0 (->FUN_0046B170)
+0x1C 0x6FDFF0
```

### §2.4 26-slot ヘルパの slot 対応（v526_1 a ＋ v525_4）
```
FUN_00466D80 : for i in 0..0x19 -> vtable[+0x00]   (this probe)
FUN_00466DA0 : for i in 0..0x19 -> vtable[+0x04]   (this probe)
FUN_00466DD0 : for i in 0..0x19 -> vtable[+0x08]   (v525)
FUN_00466E10 : compare loop     -> vtable[+0x0C]   (v525, 0 = equal)
```

## §3 判定の内訳

- **V-526-1 完遂**: thunk 接続 [確定]、vtable 所属＝`TESForm` slot `+0x14`/`+0x18` [確定]、`FUN_00466D80`/`DA0` 本文 [確定]、component view vtable の実読 [部分確定]。
- **V-526-2 部分前進**: `DeleteForm` の呼び出し経路（id→`FUN_0046B250` lookup→`DeleteForm`）[確定相当]。`this` identity は [未確定]。
- **V-526-3 残点継続**。

## §4 残点（V-527-1..3）

1. `FUN_0046B800`／`FUN_0046B870`／`FUN_0046C620`（`TESForm` vtable slot `+0x08`/`+0x0C`/`+0x10`）の identity＋`0x60D0A0`（共通 stub）の実体確認【中】
2. `DeleteForm` の `this` identity（TESDataHandler 候補）＋20 caller の ECX 供給元の静的追跡（`0x463EF8` 以外の経路）【低】
3. 引継ぎ残り（predicate slot35 英名・新 FourCC 意味）【低】

## §5 後続への申し送り

- **`TESForm` の vtable slot 表**を確保（`+0x14`/`+0x18` が component 表系仮想関数）。今後 `FUN_0046B090`/`FUN_0046B170` は「TESForm の virtual」として docs 側でも扱える。
- **vtable 逆引き手順の確立**: `.rdata` の dword 全出現 → 直前に COL（signature 0＋`+12` の TD 実読）→ vtable 起点＋class 名＋slot オフセット。1700 vtable を列挙できるので、以後は「この関数は誰の vtable か」を機械的に決められる。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
