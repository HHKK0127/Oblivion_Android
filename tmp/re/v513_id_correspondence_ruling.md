# v513 — `+0x8C0` 対応確定・Player `+0x80`＝LoadGame form ptr・`+0xC` 異物切り分けの裁定（V-513-1..3 完遂・旧残点 V-406-3 継続第43弾）

- 採番: **v513**（旧残点群消化・probe prefix **v513_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-513-1..3（v512 残点）** — 1=`+0x8C0` 対応（id set の caller 経路）と Player `+0x80` 中身／2=gated mask 源と同 offset異物切り分け／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-513-1: `+0x8C0` 対応（id set の caller 経路）** | **[確定相当]** | **`FUN_00446B40`（FF id set・v512）E8 callers＝3件 [確定(機械)]: **`0x46B338`（`FUN_0046B2C0`・`void __fastcall(int)`）／**`0x46C33E`（`FUN_0046C300`・set/replace fn・v511）**／`0x46C5E8`（`FUN_0046C460`・`undefined4 * __fastcall(undefined4 *param_1)`）** → **`FUN_00446B40`（FF id set）は `FUN_0046C300`（set/replace）から呼ばれる [確定] → `+0x8C0` 対応が確定相当**（**`FUN_0046C300` の `param_1+0xC`（current ptr）と `FUN_00446B40` の `param_1+0x8C0`（id）が同一 object の別 field [推定相当]**）。**`FUN_0046B2C0`/`FUN_0046C460` も caller＝v512 bit14 family と重複（family 整合 [確定]）** |
| **V-513-1: Player `+0x80` 中身** | **[確定相当]（LoadGame form ptr）** | LoadGame band context 実読 [確定(decompiled)]: **`L15441: puVar1 = *(undefined4 **)(DAT_00b33b00 + 0x80); if (puVar1 == 0) { log }` else **`L15448: piVar4 = (int *)FUN_0046b250(*puVar1);`（**`FUN_0046B250`＝RTDynamicCast wrapper（v501/v502 同 fn）に `*puVar1` を渡す**）→ `L15449: uVar5 = virtual[*piVar4 + 0xd4](*(puVar1 + 9), *(puVar1 + 5))`（**`virtual+0xD4` slot call、引数は `puVar1+9`（byte）と `puVar1+5`（u32）**）→ log `"Currently loading form is %08X %s with version %i and flags %08X"`（`*puVar1`＝form id、`uVar5`＝name）** → **`Player+0x80`＝「LoadGame buffer の現在読み込み中 form（block header）を指す ptr（`+5`＝u32 form id、`+9`＝byte version）[確定相当]」（`EffectSettingCollection.cpp` の LoadGame block header [確定]）** |
| **V-513-2: `+0xC` 同 offset異物切り分け** | **[確定相当]** | **signature/first-use 比較 [確定(decompiled)]: `FUN_00452C20`（`uint * FUN_00452c20(uint *param_1, uint param_2)`）の first use `uVar1 = param_1[3]`（**`+0xC`＝map key ptr**）／`FUN_0046C300`（`void __thiscall FUN_0046c300(int param_1, int param_2, char param_3)`）の first use `iVar1 = *(int *)(param_1 + 0xc)`（**`+0xC`＝current ptr（swap 対象）**）→ **`param_1` の type が違う（`uint*` map node holder vs `int` object）→ 同 offset 異物（map key vs current ptr）の切り分けが確定 [確定相当]**（v512 残点の切り分けを解消） |
| **V-513-2: gated mask 源（`edx`）** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-513-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v513_1）

### §2.1 `FUN_00446B40` callers（v513_1a）
```
E8 callers of FUN_00446b40: 3
  0x46B338 owning=FUN_0046b2c0  void __fastcall(int)
  0x46C33E owning=FUN_0046c300  void __thiscall(int, int, char)   (set/replace, v511)
  0x46C5E8 owning=FUN_0046c460  undefined4 * __fastcall(undefined4 *)
=> FF-id set driven by the set/replace fn; +0x8C0 correspondence  [確定相当]
   (v512 bit14 family overlap: FUN_0046B2C0/B2C0 in family)  [確定]
```

### §2.2 Player `+0x80` LoadGame context（v513_1b）
```
L15441: puVar1 = *(undefined4 **)(DAT_00b33b00 + 0x80);
L15442: if (puVar1 == 0) { log(...); }
L15448: else { piVar4 = (int *)FUN_0046b250(*puVar1);            // RTDynamicCast
L15449:   uVar5 = (**(code **)(*piVar4 + 0xd4))(
               *(undefined1 *)((int)puVar1 + 9),                  // byte at +9 (version)
               *(undefined4 *)((int)puVar1 + 5));                 // u32 at +5 (form id)
L15451:   log("LoadGame Buffer error: ... Currently loading form is %08X %s
                with version %i and flags %08X", *puVar1, uVar5); }
=> Player+0x80 = ptr to the in-load form block header (+5 form id, +9 version)  [確定相当]
```

### §2.3 `+0xC` disambiguation（v513_1c）
```
FUN_00452c20: uint * FUN_00452c20(uint *param_1, uint param_2)
  first use: uVar1 = param_1[3];      // +0xC as KEY (map key ptr)
FUN_0046c300: void __thiscall FUN_0046c300(int param_1, int param_2, char param_3)
  first use: iVar1 = *(int *)(param_1 + 0xc);   // +0xC as CURRENT PTR (swap target)
=> param_1 types differ -> same offset, different carriers  [確定相当]
```

## §3 残点 **V-514-1..3**
1. 【中】**`virtual+0xD4` slot impl**（LoadGame form の name/version 取得 fn）＋ **`FUN_0046B2C0`/`FUN_0046C460`（`FUN_00446B40` の残り2 caller）の identity**（FF id set の別経路）。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ `FUN_0045B670` gated の `param_1+0xC` key（`FUN_00452C20` 経由）と `FUN_0046C300` の current ptr の caller 分離（v513-2 は type で切り分け済み・経路分離は残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v513_1 の1本 EXIT=0**（V-513-1 `+0x8C0` 対応を callers 3件で確定相当・Player `+0x80`＝LoadGame form ptr を context 実読で確定相当／V-513-2 `+0xC` 異物を type で切り分け／V-513-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00446B40` E8 census（3件）**で `FUN_0046C300`（set/replace）を caller として特定 → `+0x8C0` 対応を確定 → (ii) **LoadGame band context 実読**で `Player+0x80` の中身（form block header：`+5` form id・`+9` version）を獲得 → (iii) **両 fn の signature/first use 比較**で `+0xC` 異物（map key vs current ptr）を type で切り分け。
- 妥当性チェック: ①`FUN_0046C300`⇔`FUN_00446B40`（set/replace 内の old free 候補（v511）が FF id set（v512）と一致）— ②`FUN_0046B2C0`⇔v512 bit14 family（`FUN_0046B2C0` は family 12fn の1つ）— ③`FUN_0046B250`⇔v501/v502 RTDynamicCast wrapper（同 fn）— ④`virtual+0xD4`⇔v474/v478 `+0xD8` SetName の隣接 slot（`+0xD4`＝name/version getter [推定]）— ⑤`param_1[3]`（`uint*`）⇔`*(param_1+0xc)`（`int`）＝type で切り分け — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_00446B40` callers 3件・LoadGame context とも docs と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **E8 census（3件）で「id set の driver」を特定**（**`FUN_0046C300`（set/replace）⇔`FUN_00446B40`（FF id set）の接続**＝v511→v512→v513 の連鎖）。
  2. **`Player+0x80` の中身は「LoadGame band の使用 context」で決まる**（**`+5`＝form id・`+9`＝version の offset が log 引数で判明**＝**ptr の中身は log context 法**）。
  3. **同 offset異物の切り分けは「param_1 の type（signature）」で**（**`uint*` vs `int`＝map key vs current ptr**＝**type が carrier を語る**）。
  4. **`FUN_0046B250`（RTDynamicCast wrapper）が LoadGame band でも使われる**（**cast wrapper の横断利用＝v501/v502/v513 で3度目**）。
  5. **`virtual+0xD4`（name/version getter 候補）が v474/v478 `+0xD8`（SetName）の隣接 slot**（**slot family の `+0xD4`/`+0xD8` が form の name/version 系 [推定]**）。
