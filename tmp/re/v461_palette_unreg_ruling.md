# v461 — palette insert/unregister・interpolator idx35 全数の裁定（V-461-1..3 完遂・旧残点 V-405-4..5 消化）

- 採番: **v461**（旧残点群消化・probe prefix **v461_1..v461_2 の2本全 EXIT=0**（v461_1 初回は `fn` に文字列が入る TypeError → `isinstance` 判定で修正再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **旧残点 V-405-4（palette insert `virtual+0x50` と `FUN_00716620`（NULL 登録）の経路整理）／V-405-5（未知 interpolator の idx35 と predicate slot35 の正式名）** を v461 として消化（V-461-1=V-405-4、V-461-2=V-405-5、V-461-3=移行判断の形）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **旧 V-405-4: palette insert（`virtual+0x50`）** | **[確定相当]（2用途 fn）** | **`NiDefaultAVObjectPalette` vt=`0xA798C4` の `[vt+0x50]=FUN_006C5460`（`FUN_006c5430+0x30`・L495004）[確定]**: machine **`mov eax,[esp+8]`（第2引数 obj）→ `add ecx,8`（self+8＝inner `NiTMapBase`）→ `test eax,eax; jne 0x6C5478`（obj≠0 → insert 側）／obj==0 → `mov eax,[esp+4]`（name）→ `push; call 0x4524C0; ret 8`（**remove 側**）[確定(機械)]** → **`virtual+0x50(name, obj)` ＝ `obj!=NULL` なら insert・`obj==NULL` なら name の entry を除去する2用途 fn [確定相当]**（v405 の「insert」記述に remove 経路を追補） |
| **旧 V-405-4: `FUN_004524C0`（remove 実体）** | **[確定相当]** | L63085: **`undefined4 __thiscall FUN_004524c0(int *self, undefined4 key)`: `iVar4 = virtual[self]+4(key)`（hash index）→ `puVar1 = self[2][iVar4]`（bucket 先頭）→ `if (puVar1) { cVar3 = virtual[self]+8(key, puVar1[1])`（キー比較）`if (cVar3) self[2][iVar4] = *puVar1; …`（unlink）** [確定(構造)] → **`NiTMapBase` の Remove/Erase（hash bucket walk＋キー比較＋unlink）fn [確定相当]** |
| **旧 V-405-4: `FUN_00716620`（NULL insert 対）** | **[確定相当]（unregister walker）** | L558418: **`void FUN_00716620(int *node, int *palette)`: `if (node && palette) { if (node[2] != 0) virtual[palette]+0x50(node[2], 0);`（**obj=NULL → remove**）` iVar1 = virtual[node]+8()`（childRoot）` for (i < *(u16*)(iVar1+0xb6)) FUN_00716620(*(iVar1+0xb0 + i*4), palette); }`** [確定(構造)] → **「node（と子・孫全ノード）の `node[2]`（name）を palette から remove する unregister walker」＝`FUN_007165b0`（register walker・v405 確定）の対 [確定相当]**。→ **palette の insert/release 経路は「register（`FUN_007165b0`＋KF stream `0x7166B0`）／unregister（`FUN_00716620`）／remove 単体（`virtual+0x50(name,0)`）」の3系統で整理完了 [確定]** |
| **旧 V-405-5: interpolator idx35 全数** | **[確定]（28/28 census）＋v405 訂正** | **ctor `mov [reg],imm32`×RTTI 法（v457 法）で interpolator 系28 クラスの vtable を全数特定→ `[vt+0x8C]`（idx35）を実読**: **27/28 が `0x6FDFF0`（CONST-TRUE＝`mov al,1; ret 4`）**、**唯一の override ＝ `NiLookAtInterpolator` vt=`0xA7B0D4` の idx35＝`FUN_006DF060` [確定]** → **v405 の「KF 実使用22 クラスすべて base 共通・override 0件」は22 クラス限定で、全28 クラスでは override 1 件あり＝本裁定で訂正**。`FUN_006DF060` の identity と predicate slot35 の正式名は [未確定]＝残点（override が判明したので slot35 は「恒真 predicate」でなく**class ごとに異なる predicate** の可能性が出てきた） |
| **旧 V-405-5: predicate slot35 の正式名** | **[未確定]（残点）** | idx35＝`+0x8C` は `NiInterpolator` 系の vtable slot（`NiController` 系の `0x6F7070` 恒0とは別 vtable）。**slot の機能名は「`FUN_006DF060`（唯一の override）が何を返すか」で決まる [推定]** → `FUN_006DF060` の body 実読が次 probe の鍵＝残点 |
| **V-461-3: 旧残点（V-406-2..6）への移行** | **[確定]（方針）** | **V-462 から旧残点 V-406-2..6（+10 適用 fn／間接登録 0xA73DA8／opcode 特定の残り等）へ軸を移す [確定（作業方針）]**（`FUN_006DF060` identity と predicate 正式名は低優先で V-463+ に併走） |

## §2 証拠（v461_1/v461_2）

### §2.1 palette insert/remove（v461_1a）
```
NiDefaultAVObjectPalette vt=0xA798C4 [vt+0x50] = 0x6C5460 (FUN_006c5430+0x30, L495004):
  6C5460: mov eax, [esp+8]      ; obj (2nd arg)
  6C5464: add ecx, 8            ; self+8 = inner map
  6C5467: test eax, eax
  6C5469: jne 0x6C5478          ; obj!=0 -> insert branch
  6C546B: mov eax, [esp+4]      ; name
  6C546F: push eax / call 0x4524c0 / ret 8   ; obj==0 -> REMOVE
FUN_004524c0 (L63085): __thiscall(int *self, undefined4 key):
  iVar4 = virtual[self]+4(key)            ; hash index
  puVar1 = self[2][iVar4]                 ; bucket head
  if (puVar1) { cVar3 = virtual[self]+8(key, puVar1[1]);
                if (cVar3) self[2][iVar4] = *puVar1; ... }   ; unlink
```

### §2.2 unregister walker（v461_1b）
```
L558418: void FUN_00716620(int *param_1, int *param_2) {
  if (param_1 && param_2) {
    if (param_1[2] != 0)
      (**(code**)(*param_2 + 0x50))(param_1[2], 0);   // (name, NULL) -> REMOVE
    iVar1 = (**(code**)(*param_1 + 8))();             // childRoot
    if (iVar1) for (uVar3 < *(u16*)(iVar1+0xb6))
      FUN_00716620(*(iVar1+0xb0 + uVar3*4), param_2); // recurse children
  } }
  -> unregister walker (mirror of FUN_007165b0 register walker, v405)
```

### §2.3 interpolator idx35 census（v461_2）
```
ctor mov [reg],imm32 x RTTI: 28 Interpolator classes, all vtables found
  27/28 idx35 = 0x6FDFF0 (CONST-TRUE: mov al,1; ret 4)
  EXCEPTION: .?AVNiLookAtInterpolator@@ vt=0xA7B0D4 idx35 = 0x6DF060
             -> FUN_006df060 (owning)   [唯一の override]
distribution: {'CONST-TRUE': 27, 'fn(other)': 1}
v405 ruling の「22 クラスすべて base 共通・override 0件」は22 クラス限定
  -> 全28 クラスで override 1 件＝v461 で訂正
```

## §3 残点 **V-462-1..3**
1. 【低】**旧残点の消化開始** — **V-406-2..6**（+10 適用 fn／間接登録 `0xA73DA8`／`FUN_0050D020` opcode 特定の残り）を優先順に着手（V-461-3 の方針どおり）。
2. 【低】**`FUN_006DF060`（`NiLookAtInterpolator` の idx35 override）の identity** → predicate slot35 の正式名（恒真でない唯一の実装を読めば slot の意味が決まる）。
3. 【低】**`[manager+0x30]` 書手 fn／`iVar13` entry layout／`DAT_00b3cd7c` 実行時文字列**（v460 残点を引き継ぐ）。

## §4 検証・自己批評
- probe: **v461_1..v461_2 の2本全 EXIT=0**（初回 TypeError → `isinstance(fn,int)` で修正再実行）（V-405-4 完遂（3系統整理）／V-405-5 完遂（28/28 census＋override 発見）／V-461-3 方針確定）。
- **決定打の作り**: (i) **`[vt+0x50]` の machine で `test/jne`（obj==0 分岐）→ remove 側 `call 0x4524C0` を発見** → 2用途 fn と即決 → (ii) **remove 実体 `FUN_004524C0` は decompiled の hash bucket walk（`virtual+4`→bucket→`virtual+8` 比較→unlink）で `NiTMapBase` Remove と特定** → (iii) **`FUN_00716620` は `virtual+0x50(node[2], 0)` の「0＝NULL」が unregister の決定打**（`FUN_007165b0` の register 対）→ (iv) **idx35 census は v457 法（ctor store×RTTI）で28 クラス全数**（v461_1 の COL 法が `.rdata`/`.text` 限定で全件失敗したため手法切替）。
- 妥当性チェック: ①`ret 8`⇔2引数 thiscall（name, obj）— ②`virtual+4(key)`＝hash index⇔`NiTMapBase` の bucket 算出（v460 の `div [ecx+4]` と同系）— ③`node[2]`⇔register walker の `m_kName[+8]`（v405）と同一 field — ④28 クラス⇔v461_1 の TD 列挙28件と一致（内部整合）— ⑤`NiLookAtInterpolator` のみ override⇔LookAt は dependency（palette）依存の predicate が必要な特殊ケース [推定] — **五方向矛盾なし**。
- docs との不整合: **①v405 ruling「override 0件」→ 28 クラス中1件（`NiLookAtInterpolator`）で訂正**／②v405「`FUN_00716620` は同一 walker 形で `insert(name,0)`（NULL 登録）＝unregister 派生か [推定]」→ **本裁定で unregister walker（remove）と確定**（「NULL 登録」ではなく「NULL＝remove 引数」）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`test reg,reg; jne` の直後にある「引数==0 の分岐」は insert/remove の2用途 fn の目印** — palette の `virtual+0x50` はこれで remove 経路を発見。
  2. **「NULL 登録」という推測は「NULL＝remove 引数」と読む** — `virtual+0x50(name, 0)` の `0` が第2引数（obj）と分かれば、登録でなく除去と分かる（**v405 の推測を本裁定で確定・訂正**）。
  3. **全数 census は COL 法が失敗したら ctor store×RTTI 法へ即切替**（v461_1→v461_2 で2本目が成功・v457 法の再利用性を確認）。
  4. **「override 0件」という否定的結論は「対象クラスの列挙範囲」に依存する** — 22 クラスで0件、28 クラスで1件（**全数を出さないと否定的結論が覆る**）。
  5. **unregister walker は register walker の対で覚える**（`FUN_007165b0`⇔`FUN_00716620`・子孫再帰構造が同一で insert/remove の差だけ）。
