# v504 — Player `+0x7C`＝version byte・`FUN_0041E210`＝GetExtraData 訂正・node-set impl 候補の裁定（V-504-1..3 完遂・旧残点 V-406-3 継続第34弾）

- 採番: **v504**（旧残点群消化・probe prefix **v504_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-504-1..3（v503 残点）** — 1=node-set impl fn 特定と Player `+0x7C` 閾値（35/63/36）の意味／2=`FUN_0041E210` 本文と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---| 
| **V-504-1: Player `+0x7C` byte の意味（閾値 35/63/36）** | **[確定相当]（version byte）** | **context 実読 [確定(実文字列＋構造)]: **`L15443-15445: FUN_004a7a60("LoadGame Buffer error: Block Header is incorrect in file %s on line %i.  Current version is %i", "..\\TES Shared\\Magic\\EffectSettingCollection.cpp", 0xa4, *(undefined1 *)(DAT_00b33b00 + 0x7c));` → **`+0x7C` byte は「Current version is %i」の version 値 [確定相当]**（`0xa4`＝164＝line）。**`L15482-15484`/`L15488-15490`（overrun/underrun log）でも同じ `0xb8`（184＝line）＋`+0x7C`（version）の組 [確定]** → **Player `+0x7C` byte＝SaveGame/LoadGame の version byte [確定相当] → v503 の閾値（`0x23 <`＝35超／`0x3f <`＝63超／`0x24`＝36）は version 比較（対応バージョン範囲 gate）[推定相当]**。副次: **`L15440: local_4 != 0x4b4f4c42`＝`"BLOB"` FourCC（LE 逆バイト）[確定]**、**`L15467: "Player Load: Failed to find known effect setting ID %08X"` [確定(実文字列)]**、**`L15470: *(iStack_c + 0x58) |= 0x200000`（effect setting `+0x58` bit21 set）[確定]** → **本帯は `EffectSettingCollection.cpp` の LoadGame 処理 [確定相当]**（`DAT_00b33b00+0x80`＝ptr（`puVar1`）・`+0x14`＝uint（`uVar2`＝buffer size）も同時判明） |
| **V-504-2: `FUN_0041E210(0x4b)` identity（v501 訂正）** | **[確定]（実文字列）→ v501 推定を訂正** | body L20067-20100 実読 [確定(decompiled)]: **`int __thiscall FUN_0041e210(int param_1, byte param_2)`: `if ((param_2 >> 3 < 0xc) && ((*(byte *)((param_2 >> 3) + 8 + param_1) & (byte)(1 << (param_2 & 7))) != 0))`（**type byte の bit mask 検査（12 bucket×8bit＝96種）**）→ TLS cache（`iVar1+0x10+param_2*4`・`param_2 < 0x5d`＝93 種上限）→ **`FUN_00401020("BaseExtraList::GetExtraData")`（**critical section 名＝実文字列**）** → linked list 走査（`*(iVar1+4)==type`）→ `return iVar2` [確定(構造＋実文字列)]** → **`FUN_0041E210`＝`BaseExtraList::GetExtraData`（type byte 指定で extra data を取得する fn）[確定] → v501「FormType/種別 byte 判定 [推定相当]」を本裁定で訂正**。`0x4b`（75）＝**extra data type byte** [確定相当]（`param_2 < 0x5d` 値域内） |
| **V-504-1: node-set impl fn（map 系 vtable slot `+0xC`）** | **[推定相当]（候補特定）** | ctor-store vtable census（1486個）から NiT*/Map* 463クラスを列挙 [確定(機械)]。**`BSTCaseInsensitiveStringMap<PAVTESForm>`（vt=0xA3BF1C）の `[vt+0xC] = 0x6C4F80` → `FUN_006C4F30` 内 [確定(機械)]、`BSTCaseInsensitiveStringMap<IDLE_ANIM_ROOT>`（vt=0xA53920）／`<PAVSetting>`（vt=0xA40454）も同一 impl `FUN_006C4F80` 共有 [確定]** → **`FUN_006C4F80`＝case-insensitive string key map の node-set impl 候補 [推定相当]**（`FUN_00412D30`（v485 setting map＝`<PAVSetting>` [推定]）の this vtable がこの family なら node-set 完遂）。**`LockFreeMap`/`LockFreeStringMap` 系の slot `+0xC` は別 impl（`FUN_0043E430`/`FUN_0055F4C0`/`FUN_00433F20` 等）[確定]** → **map family で impl が分岐（BST 系 vs LockFree 系）**。`FUN_00452570`（v494）の this がどちら family かは未確定＝残点 |
| **V-504-2: gated 側 mask 源（`edx`）** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-504-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v504_1）

### §2.1 Player `+0x7C` version byte（v504_1b）
```
L15441: puVar1 = *(undefined4 **)(DAT_00b33b00 + 0x80);        // Player +0x80 ptr
L15443: FUN_004a7a60("LoadGame Buffer error: Block Header is incorrect
          in file %s on line %i.  Current version is %i",
          "..\\TES Shared\\Magic\\EffectSettingCollection.cpp", 0xa4,
          *(undefined1 *)(DAT_00b33b00 + 0x7c));                // +0x7C = VERSION
L15440: if (local_4 != 0x4b4f4c42) ...                          // "BLOB" FourCC
L15467: "Player Load: Failed to find known effect setting ID %08X"
L15470: *(uint *)(iStack_c + 0x58) |= 0x200000;                 // effect +0x58 bit21
L15484 / L15490: "... line %i.  Current version is %i", 0xb8,
          *(undefined1 *)(DAT_00b33b00 + 0x7c));                // same pair
L15455: iVar7 = *(int *)(DAT_00b33b00 + 0x14);                 // Player +0x14 uint
=> Player +0x7C = save/load version byte  [確定相当]
=> thresholds 35/63/36 = version-range gates  [推定相当]
```

### §2.2 `FUN_0041E210` = GetExtraData（v504_1c）
```
L20079: if ((param_2 >> 3 < 0xc) && ((*(byte *)((param_2 >> 3) + 8 + param_1)
            & (byte)(1 << (param_2 & 7))) != 0)) {          // 96-type bit mask
L20083:   if ((param_1 == *(int *)(iVar1 + 8)) && (FUN_0041dea0(), param_2 < 0x5d)) {
L20084:     iVar2 = *(int *)(iVar1 + 0x10 + (uint)param_2 * 4);   // TLS cache, 93 types
L20087:   FUN_00401020("BaseExtraList::GetExtraData");           // REAL STRING
L20090:   if (*(char *)(iVar1 + 4) == unaff_retaddr) ...          // type match walk
=> FUN_0041E210 = BaseExtraList::GetExtraData(type byte)  [確定]
=> 0x4B = extra data type byte (v501 FormType guess corrected)
```

### §2.3 node-set impl candidates（v504_1a）
```
ctor-store vtables: 1486 / NiT*|Map* classes: 463
  vt=0xA3BF1C .?AV?$BSTCaseInsensitiveStringMap@PAVTESForm@@ [+0xC]=0x006C4F80 (FUN_006c4f30)
  vt=0xA40454 .?AV?$BSTCaseInsensitiveStringMap@PAVSetting@@ [+0xC]=0x006C4F80 (shared)
  vt=0xA53920 .?AV?$BSTCaseInsensitiveStringMap@PAUIDLE_ANIM_ROOT@@ [+0xC]=0x006C4F80 (shared)
  LockFreeMap* / LockFreeStringMap* family: +0xC = FUN_0043E430 / FUN_0055F4C0 / FUN_00433F20
=> FUN_006C4F80 = BST case-insensitive string map node-set impl candidate  [推定相当]
=> map family splits: BST vs LockFree impls  [確定]
=> FUN_00452570's family still unconfirmed  [残点]
```

## §3 残点 **V-505-1..3**
1. 【中】**`FUN_006C4F80` 本文**（node-set の value→field 対応完遂）と **`FUN_00452570`/`FUN_00412D30` の this vtable family 確認**（BST vs LockFree）＋ **version 閾値（35/63/36）の対応バージョン**（`0x4b4f4c42`＝`"BLOB"` ブロック形式との関係）。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ Player `+0x80`（ptr）／`+0x14`（buffer size）の identity（本裁定で型判明）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v504_1 の1本 EXIT=0**（V-504-1 Player `+0x7C`＝version byte を実文字列で確定相当・閾値を version gate に接続／V-504-2 `FUN_0041E210`＝`BaseExtraList::GetExtraData` を実文字列で確定（v501 推定を訂正）／V-504-1 node-set impl 候補 `FUN_006C4F80` を推定相当／V-504-3 引継ぎ）。
- **決定打の作り**: (i) **`+0x7C` の docs grep（410件）から log context を拾う** → `"Current version is %i"` の引数が `+0x7C` と一致 → **version byte と即決**（**v503 の閾値 census が version gate に接続**）→ (ii) **`FUN_0041E210` body で `"BaseExtraList::GetExtraData"` 実文字列**を獲得 → **v501「FormType 判定 [推定]」を確定で訂正** → (iii) **census（1486 vtable）から NiT*/Map* を filter して slot `+0xC` 実読** → BST 系 impl 一覧で候補特定。
- 妥当性チェック: ①`"Current version is %i"`⇔`+0x7C` byte（log 引数）⇔`0xa4`/`0xb8`＝line（別 log）— ②`param_2 >> 3 < 0xc`⇔96 type bit mask⇔`param_2 < 0x5d`（93）＝extra type 値域 — ③`"BaseExtraList::GetExtraData"`⇔critical section 名（v498/v499 の `DAT_00a2f830` 型と同定番）— ④`BSTCaseInsensitiveStringMap@PAVSetting`⇔v485 setting map（`FUN_00412D30`）family 候補 — ⑤`0x4b4f4c42`＝`"BLOB"`⇔LoadGame ブロック FourCC（v484/v485 LE 逆バイト法）— **五方向矛盾なし**。
- docs との不整合: **①v501 ruling「`FUN_0041E210`＝FormType/種別 byte 判定 [推定相当]」→ 本裁定で `BaseExtraList::GetExtraData`（extra data type byte 取得）と確定訂正**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`"…%i"` log の引数が field の意味を語る** — `Current version is %i` の引数＝`+0x7C` byte で **version と即決**（**閾値 census（v503）→log context（v504）の2段が field semantic を完成**）。
  2. **critical section の名前文字列（`FUN_00401020("…")`）が fn identity を一撃**（`"BaseExtraList::GetExtraData"`＝v498 `DAT_00a2f830` に続く**名前法の再現**）。
  3. **推定（v501 FormType）は実文字列で訂正される** — **[推定] はいつでも [確定] で上書きできる**（裁定の更新が正しい運用）。
  4. **census filter（NiT*/Map*）＋slot `+0xC` 実読で impl family が判別**（BST 系 vs LockFree 系で impl 分岐 [確定]＝**class family の型情報が slot に載る**）。
  5. **`"BLOB"` FourCC（`0x4b4f4c42`）が LoadGame ブロック形式の目印**（v484/v485 に続く LE 逆バイト法の反復）。
