# v456 — `NiControllerSequence::StoreTargets` 同定と `Bip01[+0x1C]` lookup 系統の裁定（V-456-1..3 完遂・旧残点 V-415-1..2 消化）

- 採番: **v456**（旧残点群へ軸戻し第1弾・probe prefix **v456_1..v456_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **旧残点 V-415-1（`Bip01[+0x1C]` 正体＝`FUN_00700010` identity【中】）／V-415-2（`L498845 [manager+0x30]→slot23` 呼び出し【低】）** を v456 として消化（V-456-1=V-415-1 継続、V-456-2=V-415-2 継続、V-456-3=subobject 残3＋purpose 併走の形で起票）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-415-1: `FUN_00700010` identity** | **[確定(decompiled)]** | **`int *__thiscall FUN_00700010(int *self, int key)`: `piVar1 = self[3]`（`+0xC`）→ outer loop: null→0／**`iVar2 = virtual [piVar1]+4()()`（first item）→ inner linked walk（`iVar2+4`）→ `if (iVar2 == key) return piVar1`** → **`piVar1 = piVar1[0xD]`（`+0x34` outer next）**（machine `mov esi,[esi+0x34]` と一致）[確定]** → **二重 linked list（outer `+0x34`／inner slot+4 chain）の key-based node lookup fn [確定]** |
| **V-415-1: `Bip01[+0x1C]` の系統** | **[確定]（lookup 形）＋[推定高]（正体）** | **docs L498848-51**: **`iVar13 = virtual[local_24]+0x4C("Bip02")` → `FUN_004D9B40...` ではなく `iVar13+0x1c` を取得 → `FUN_006c6910(*(iVar13 + 0x1c))` [確定(decompiled)]** — **`local_24` の virtual `+0x4C(str)`＝名前（`"Bip02"`／`DAT_00a30968`）で Ni node を探す getter、その `+0x1C` を `FUN_006C6910` へ渡す [確定（構造）]** → **`Bip01[+0x1C]` ＝ skeleton bone（`"Bip02"`/Bip01 名 lookup 結果）の `+0x1C` ptr [推定高]**（`FUN_006C6910` 側の消費で語義確定へ） |
| **V-415-2: `L498845` slot23 呼び出しの構造** | **[確定]（構造）＋[推定高]（class）** | **docs L498845**: **`(**(code **)(**(int **)(*(int *)(local_28 + 0x40) + 0x30) + 0x5c))(&uStack_29, 1, &cStack_2a)` ＝ `[seq+0x40]（manager）→ [+0x30]（sub-manager）→ vtable slot +0x5C` を3引数で virtual call [確定(構造)]**。machine 対応（owning band）: **`6C9B3B: mov eax,[edx+0x40]` → `6C9B3E: mov ecx,[eax+0x30]` → `6C9B43: mov edx,[edx+0x5C]`**（vtable slot 読み）＋ `6C9914/6C9929` の `mov eax,[edi+0x40]; cmp ebp,[eax+0x30]`（manager+0x30 比較）[確定(機械)] → **`[seq+0x40]`＝NiControllerManager 系 manager [推定高]、`[manager+0x30]`＝controller/sub-manager [推定高]、vtable `+0x5C`＝store/register 系 virtual [推定高]** |
| **V-456-1 紐: owning fn の identity** | **[確定]（実文字列）** | **L498833 の owning＝`FUN_006C9590`（L498520）**、error 文言 **`"NiControllerSequence::StoreTargets '%s' failed to find target with the following identifiers: ... m_pcAVObjectName ... m_pcCtlrType ... m_pcPropertyType"` [確定(実文字列)]** → **`FUN_006C9590` ＝ `NiControllerSequence::StoreTargets` [確定]**。machine head: `mov eax,[esi+0x40]`（seq+0x40＝manager）→ **`mov edi,[eax+0x7C]`（manager+0x7C）**→ `mov eax,[esi+0x5C]`（seq+0x5C）→ `virtual [edi]+0x4C` [確定(機械)] |
| **V-415-1: `FUN_006C6910` identity** | **[未確定]（残点継続）** | L496358 `void __fastcall(int)`・SEH prologue（`LAB_009c7568`/`DAT_00b30aac` cookie）— 22行 cap では local 宣言中、body 実体（string push 等）は未読 [未確定] |
| **V-456-3: subobject 残3＋purpose** | **[未確定]**（本裁定では未着手＝v455 残点そのまま） |

## §2 証拠（v456_1/v456_2）

### §2.1 `FUN_00700010`（v456_1a/c）
```
L541175: int *__thiscall FUN_00700010(int *param_1, int param_2) {
  piVar1 = *(int**)(param_1 + 0xc);
  do { if (piVar1 == 0) return 0;
       for (iVar2 = (**(code**)(*piVar1 + 4))(); iVar2 != 0; iVar2 = *(int*)(iVar2+4))
         if (iVar2 == param_2) return piVar1;
       piVar1 = (int*)piVar1[0xd];   // +0x34 outer next
  } while (true); }
machine: mov esi,[ecx+0xc] / mov eax,[esi]; mov edx,[eax+4]; call / cmp eax,edi; je
         / mov eax,[eax+4] walk / mov esi,[esi+0x34] / jne / ret 4
```

### §2.2 `FUN_006C9590` = StoreTargets（v456_2a）
```
owning of L498833: FUN_006c9590 @0x6C9590 (L498520)
machine head: sub esp,0x2c / push esi / mov esi,ecx /
  mov eax,[esi+0x40] / push edi / mov edi,[eax+0x7c] / mov eax,[esi+0x5c] / ...
  mov edx,[edi]; push eax; mov eax,[edx+0x4c]  (virtual +0x4C)
error string (L498833): "NiControllerSequence::StoreTargets '%s' failed to find
  target ... m_pcAVObjectName / m_pcCtlrType / m_pcPropertyType"
```

### §2.3 Bip01 lookup + slot23 call（v456_1b / v456_2c）
```
L498848: iVar13 = (**(code**)(*local_24 + 0x4c))("Bip02");   (name lookup)
L498849: ... (**(code**)(*piVar2 + 0x4c))(&DAT_00a30968) ...
L498850: if (*(int*)(iVar13 + 0x1c) != 0) FUN_006c6910(*(int*)(iVar13 + 0x1c));
L498845: (**(code**)(**(int**)(*(int*)(local_28 + 0x40) + 0x30) + 0x5c))(
             &uStack_29, 1, &cStack_2a);
machine band: 6C9B3B mov eax,[edx+0x40] / 6C9B3E mov ecx,[eax+0x30] /
              6C9B43 mov edx,[edx+0x5c]
              6C9914 mov eax,[edi+0x40] / 6C9929 cmp ebp,[eax+0x30]
```

## §3 残点 **V-457-1..3**
1. 【低】**`FUN_006C6910` の identity**（body 深部・bone `+0x1C` 消費側）→ `Bip01[+0x1C]` の一語名確定。
2. 【低】**`[seq+0x40]` manager の class**（NiControllerManager の RTTI 判定＝seq vtable/manager vtable の slot 検査）と **vtable `+0x5C` virtual の用途名**。
3. 【低】**V-456-3 も併走**: subobject 残3（`+0x44/+0x68/+0x54`）＋`FUN_004E82E0` purpose 一語名（旧 v455 残点を引き継ぐ）。

## §4 検証・自己批評
- probe: **v456_1..v456_2 の2本全 EXIT=0**（V-415-1 は lookup 形確定＋`FUN_00700010` 完遂・正体は [推定高]／V-415-2 は構造確定・class は [推定高]／owning fn＝StoreTargets を実文言で確定）。
- **決定打の作り**: (i) **owning fn は「docs line の backward annotation」で address を取り、その machine head の `mov eax,[esi+0x40]` 等を L498845 の式と突合** → StoreTargets を一手で確定 → (ii) **`FUN_00700010` は decompiled の doubly-linked walk と machine `[esi+0x34]` の一致で outer/inner の構造を確定** → (iii) **`Bip01[+0x1C]` は docs の lookup 式（`virtual+0x4C("Bip02")→+0x1C`）を読むだけ**。
- 妥当性チェック: ①`piVar1[0xd]`⇔`+0x34`（0xd×4）⇔machine `[esi+0x34]` — ②error 文言 `NiControllerSequence::StoreTargets`⇔owning `FUN_006C9590`（L498520 annotation）— ③L498845 式⇔machine `6C9B3B..6C9B43`（`+0x40→+0x30→+0x5C`）— ④`virtual+0x4C("Bip02")`⇔v435 の bone/name getter 族と整合 — **四方向矛盾なし**。
- docs との不整合: **なし**（v415 の epilogue 整合記載と本裁定が一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **error 実文言（`Class::Method '...'` 形）は owning fn の identity を一撃で与える** — `NiControllerSequence::StoreTargets` のように**文言中の `::` 付きメソッド名がそのまま fn 名**（本裁定の最速の手）。
  2. **docs の call 式（`(**(code**)(**(int**)(X+0x30)+0x5c))`）は機械の `mov` 連鎖と1対1** — `+0x40→+0x30→+0x5C` を machine band で grep すれば構造が即一致。
  3. **decompiled doubly-linked walk は `list[n]` の index（0xd×4=0x34）で machine offset に変換** — docs の `piVar1[0xd]` をそのまま `[esi+0x34]` と対応づけられる（**index×4 換算を忘れない**）。
  4. **name-based lookup 式（`virtual+0x4C("name")→+0x1C`）は bone/name getter 系の定型** — `"Bip02"` という実文字列が skeleton 系と即分かる。
  5. **probe は owning fn の head machine まで取る** — head の `mov eax,[esi+0x40]`／`[eax+0x7C]` が「seq/manager の field 一覧」を同時に教えてくれる（副次収穫）。
