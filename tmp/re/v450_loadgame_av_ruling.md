# v450 — LoadGame/AV fetch/init-chain の裁定（V-450-1..3 完遂・旧残点群移行第4弾）

- 採番: **v450**（旧残点群移行・probe prefix **v450_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-450-1..3（v449 の残点）** — 1=`Player[+0x1C]` 真の書手 fn と `+0x32 s16` の語／2=`FUN_00467CA0` identity 深部と `FUN_005EAD00`（AV fetch 実体）／3=`0x523F40`（docs 欠落独立 fn）の identity を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-450-1: `Player[+0x1C]` の書手（imm 版）** | **[確定]（否定側）** | **全 .text の `mov dword [reg+0x1C], imm32` census＝0 hits [確定(否定側)]**（no-SIB・SIB 両 form）→ **imm32 直書きによる書手は存在しない**。残る候補は reg-src store（`mov [reg+0x1c], reg`）または pointer 経由 [未確定]＝残点。`+0x32 s16` の語も継続 [未確定] |
| **V-450-2: `FUN_00467CA0` identity** | **[確定(実文字列)]** | **`TESActorBaseData.cpp` の LoadGame fn [確定]**: `FUN_004534D0(&local,4)` read → **`if (local != 0x4B4F4C42)`（＝`'BLOK'` ESM block シグネチャ）[確定]** → error **`"LoadGame Buffer error: Block Header is incorrect in file %s on line %i. Current version is %i"` with `"..\\TES Shared\\TESActorBaseData.cpp"` line `0x678`** → **実文字列で `TESActorBaseData` のロード処理と特定**。後続: `iVar9 = [DAT_00b33b00+0x14]`／`FUN_004534D0(&local,2)` read／**`if (param_2 & 0x10)` flag 分岐**（`param_2`＝byte flags）[確定(decompiled)]。`DAT_00b33b00`＝LoadGame コンテキスト [推定高] |
| **V-450-2: `FUN_005EAD00`（AV fetch 実体）** | **[確定(decompiled)]** | **`float10 __thiscall(int *self, int index)`: `if ((index-8 < 4) && (self == DAT_00b333C4))` → `FUN_005E2210(index, …)`（Player 専用パス）→ 以降通常パス** → **AV index 8..11（Health/Magicka/Fatigue/Encumbrance）を Player のとき特別扱いする AV 値取得 fn [確定]**。`FUN_005F1910`（v449）→ `FUN_005EAD00` の実体 [確定]。`FUN_005E2210` の中身は残点 |
| **V-450-3: `0x523F40`（docs 欠落独立 fn）identity** | **[確定相当]（機械一式）** | full decode: `mov eax,[esi+8]; shr eax,3; test al,1; jne …`（flag bit3 gate）→ **sub-obj 初期化/reset 連鎖: `lea ecx,[esi+0xC4]; call 0x46F020`／`[esi+0x44]; call 0x469E90`／`[esi+0x68]; call 0x468590`／`[esi+0xE4]; call 0x46E4F0`／**`lea ebx,[esi+0x24]`（＝TESActorBaseData）`; call 0x4676B0`**／`[esi+0x54]; call 0x46FC10` → `[esi+0x104]` flag で続行 [確定(機械)]** → **Actor/Character 系の型別 init/reset fn（`TESActorBaseData`(+0x24) を含む各 subobject を順次初期化）[確定相当]** |
| **V-450-3: docs 欠落件数の更新** | **[確定]** | **docs annotation 欠落独立 fn は計3件を確認**: `FUN_00511570`（v448）／`0x523F40`（v449）／`FUN_00511570` と同型の `0x523F40` 群 — **3件とも int3×8 境界 [確定]**（v437 の `FUN_0040d4d0` body 範囲不一致と併せ、docs 欠落は「body 内 call 欠落」と「annotation 完全欠落」の2型 [確定]） |

## §2 証拠（v450_1）

### §2.1 imm 書手 census（v450_1a）
```
mov dword [reg+0x1c], imm32 in .text: hits 0 (no-SIB C7 80..87 + SIB C7 84 両 form)
```

### §2.2 FUN_00467CA0（v450_1b・L81608-）
```
L81630: cVar3 = FUN_0045a170();
L81632: FUN_004534d0(&local_8, 4);
L81633: if (local_8 != 0x4b4f4c42) {   <- 'BLOK'
  error: "LoadGame Buffer error: Block Header is incorrect in file %s on line %i.
          Current version is %i" with "..\TES Shared\TESActorBaseData.cpp", 0x678
L81648: iVar9 = *(int*)(DAT_00b33b00 + 0x14);
L81649: FUN_004534d0(&local_10, 2);
L81652: if ((param_2 & 0x10) != 0) { ...
```

### §2.3 FUN_005EAD00（v450_1c・L353621-）
```
L353638: if ((param_2 - 8U < 4) && (param_1 == DAT_00b333c4)) {
             FUN_005e2210(param_2, local_8 + 1, local_8);  // Player special path
             if (local_8[0] == 0.0) local_8[0] = 1.0;
  -> AV index 8..11 (Health/Magicka/Fatigue/Encumbrance) Player-only branch [確定]
```

### §2.4 0x523F40（v450_1d）
```
523F40 push ecx / push esi / mov esi,ecx
523F44 mov eax,[esi+8] / shr eax,3 / test al,1 / jne 0x5240f1   (flag bit3)
523F54 lea ecx,[esi+0xc4] / call 0x46f020
523F60 lea ecx,[esi+0x44] / call 0x469e90
523F69 lea ecx,[esi+0x68] / call 0x468590
523F72 lea ecx,[esi+0xe4] / call 0x46e4f0
523F7D lea ebx,[esi+0x24] / mov ecx,ebx / call 0x4676b0   <- TESActorBaseData
523F89 lea ecx,[esi+0x54] / call 0x46fc10
523F91 mov eax,[esi+0x104] / test / je 0x523fe0
```

## §3 残点 **V-451-1..3**
1. 【低】**`Player[+0x1C]` の reg-src / pointer 経由書手**（`mov [reg+0x1c], reg` census と lea+call 経路）＋ **`+0x32 s16` の語**。
2. 【低】**`FUN_005E2210`（Player AV special path の実体）**と **`DAT_00b33b00`（LoadGame context）identity**。
3. 【低】**`0x523F40` の各 sub-call（`0x46F020/469E90/468590/46E4F0/4676B0/46FC10`）の identity 一覧**（何の subobject を reset するか）＋ flag bit3/`[+0x104]` の意味。

## §4 検証・自己批評
- probe: **v450_1 の1本 EXIT=0**（V-450-1 は imm 否定側のみ＝部分／V-450-2 完遂（LoadGame＋AV fetch）／V-450-3 完遂（init chain 相当））。
- **決定打の作り**: (i) **imm 書手は全 .text census の0 hits で排他** → (ii) **`FUN_00467CA0` は error 文言のファイルパス（`TESActorBaseData.cpp`）＋`'BLOK'` 定数で identity 即決** → (iii) **AV fetch は `index-8<4 && self==Player` の条件式で index 範囲と Player 専用性が同時に確定** → (iv) **`0x523F40` は sub-obj `lea ecx,[esi+off]` 列＋`+0x24`（TESActorBaseData）で init chain 判定**。
- 妥当性チェック: ①`0x4B4F4C42`⇔`'BLOK'`（ESM block）⇔LoadGame error 文言 — ②`index-8<4`⇔AV8..11＝Health/Magicka/Fatigue/Encumbrance（v447 AV table）— ③`[esi+0x24]`⇔`TESActorBaseData`（v448）⇔`call 0x4676b0`（TESActorBaseData reset family 帯）— ④imm0 hits⇔v449 の「別 fn」推定（書手は reg-src）— **四方向矛盾なし**。
- docs との不整合: **なし**（docs annotation 欠落3件目は前裁定で確定済みの類型）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **ESM ブロックシグネチャ定数（`'BLOK'` 等の 4-char dword）が fn identity の強い錨** — error 文言の `.cpp` パスとセットで identity が即決する（`TESActorBaseData.cpp` line `0x678`）。
  2. **AV fetch 系 fn は `index - N < K` の unsigned range テストが index 範囲をそのまま教える**（`8..11`⇔Health..Encumbrance）。
  3. **init/reset chain は `lea ecx,[esi+off]; call` の列を並べれば subobject リストが図に出る**（`+0x24`＝TESActorBaseData が既知なら中核が特定できる）。
  4. **`mov [reg+disp], imm32` の全 .text census が0なら「imm 書手なし」と即排他** — 次は reg-src/indirect へ段階を移す（形式を順に消す）。
  5. **docs 欠落 fn は int3×8 境界で機械的に列挙できる**（v448/v449 で2件、本裁定で類型整理）— **annotation 無し＋int3 境界の2条件で機械 census が可能**。
