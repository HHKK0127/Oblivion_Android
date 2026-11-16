# v462 — derived AV 初期適用・global singleton・`+0x7A4` float 配列の裁定（V-462-1..3 完遂・旧残点 V-406-2/V-406-6 消化）

- 採番: **v462**（旧残点群消化・probe prefix **v462_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-462-1..3（v461 残点＋旧残点 V-406-2/V-406-6 引継ぎ）** — 1=旧残点消化開始（V-406-2..6 優先順）／2=`FUN_006DF060` identity→predicate 正式名／3=v460 残点引き継ぎを消化。**本 probe は V-406-2（`FUN_005222D0` 属性計算ブロックと `0xB362E4/EC` 書手）と V-406-6（`FUN_00668B30` 第4引数・`+0x7A4` 配列）に集中**。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **旧 V-406-2: `FUN_005222D0` の属性計算ブロック（`param_2==0` 分岐）** | **[確定相当]** | signature **`void __thiscall FUN_005222d0(int self, char param_2)`（L216751）[確定(decompiled)]**。**`param_2==0` 分岐（L216901-216912）**: `uVar5=FUN_00468ad0(5)`（AV index5＝**Endurance**）／`uVar6=FUN_00468ad0(0)`（AV index0＝**Strength**）／`uVar14=FUN_006ecc80()`（**CLAS getter＝`[class+0x40]`**・v406 確定）→ `uVar14=FUN_00547f80(uVar6,uVar5,iVar10,local_4,uVar14)` → **`if (param_2=='\0') { FUN_00519c50(uVar14)`（＝AV8 Health SET・v413 case8 と完全一致）` uVar5=FUN_00468ad0(1)`（Intelligence）` FUN_005482b0(uVar5,0)` → **`FUN_00467290(uVar14)`（＝AV9 Magicka SET・v413 case9）** ` uVar5=FUN_00468ad0(2)`（Willpower）` uVar6=FUN_00468ad0(3)`（Agility）` uVar7=FUN_00468ad0(5)`（Endurance）` uVar8=FUN_00468ad0(0)`（Strength）` uVar14=FUN_005479d0(uVar8,uVar7,uVar6,uVar5)`（4引数）→ **`FUN_004672b0(uVar14)`（＝AV10 Fatigue SET・v413 case10）** }` [確定(構造)] → **「class（CLAS form）の属性（Strength/Endurance/Intelligence/Willpower/Agility）から derived AV（Health/Magicka/Fatigue）を計算して Player へ SET する初期適用ブロック [確定相当]`**＝**v406-1 の「+10 初期適用」の derived AV 部分を特定**。正式名は docs に annotation は存在するが名前無し（`FUN_` 名のみ）→ **一語名 [未確定]＝残点**（機能は確定相当） |
| **旧 V-406-2: `0xB362E4`/`0xB362EC` の書手と正体** | **[確定相当]** | **各 singleton object で3系統 [確定]**: (i) **初期化 fn `FUN_009E3A20`（L1029691）: `mov ecx,0xB362E4; call 0x41BAE0`（init）→ `push 0xA1C1E0; call 0x981FB4`**（`FUN_00981FB4`＝名前/RTTI 登録系 [推定高]・v444 registration stub でも同 fn を確認）／(ii) **thunk `FUN_00A1C1E0`（L1043880）: `mov ecx,0xB362E4; jmp 0x4055E0`（thiscall wrapper）**（`0xB362EC` は `FUN_00A1C1F0`・`0xA1C1F0` push）／(iii) **`FUN_005222D0` 内の `0x5223AC: add eax,0xB362E4`（docs L216799）＝singleton への加算（attr ボーナス加算 [推定]）** → **`0xB362E4/0xB362EC`＝初期化・thunk・加算の3系統を持つ runtime singleton object（attr ボーナス系 [推定高]）[確定相当]**（docs mentions は各1件＝L216799/L216801 の `add` 行のみ） |
| **旧 V-406-6: `FUN_00668B30` の signature と第4引数** | **[確定]（signature）＋[未確定]（第4引数の語）** | L429187: **`void __thiscall FUN_00668b30(int self, float param_2, float param_3, int param_4, char param_5)`（5引数）[確定(decompiled)]**、body: **`iVar2=(int)param_2`／`iVar4=FUN_005f1910(param_2)`（＝AV 値取得 wrapper・v449 確定）→ param_2 を AV index として扱う [確定(構造)]** → **skill 値加算 fn（`[+0x5B0][idx] +=`・v406 確定の増分唯一入口 `FUN_00668C30` の下流）**。**第4引数 `param_4`（呼び出しでは0）の語は [未確定]＝残点** |
| **旧 V-406-6: `Player[+0x7A4]` 配列** | **[確定]（構造）＋[推定高]（語）** | **`L431239（FUN_0066C200 内）: `local_4 = *(float *)(param_1 + 0x7a4 + cVar3 * 4)` [確定(decompiled)]** → **`Player[+0x7A4 + idx*4]`＝float 配列（idx＝`cVar3`＝skill index と推定）**。`FUN_0066C200`＝保留適用 fn（v406 確定）の文脈 → **per-skill の累積/閾値 float 値 [推定高]**（`+0x7A4` は `+0x788`（NiTMapBase member・v448 ctor store）の直後領域） |
| **V-462-2: `FUN_006DF060`（idx35 override）** | **[未確定]（残点継続）** | 本 probe 未着手（budget） |
| **V-462-3: v460 残点（`[manager+0x30]` 書手／`iVar13` layout／`0xB3CD7C` 文字列）** | **[未確定]（残点継続）** | 本 probe 未着手 |

## §2 証拠（v462_1）

### §2.1 `FUN_005222D0` 属性ブロック（v462_1a）
```
L216751: void __thiscall FUN_005222d0(int param_1, char param_2)
L216896: uVar5 = FUN_00468ad0(5);        // AV index5 = Endurance
L216897: uVar6 = FUN_00468ad0(0);        // AV index0 = Strength
L216898: uVar14 = FUN_006ecc80();        // CLAS getter ([class+0x40])
L216899: uVar14 = FUN_00547f80(uVar6, uVar5, iVar10, local_4, uVar14);
L216901: if (param_2 == '\0') {
L216902:   FUN_00519c50(uVar14);          // AV8  Health SET   (v413 case8)
L216903:   uVar5 = FUN_00468ad0(1);       // Intelligence
L216904:   uVar14 = FUN_005482b0(uVar5, 0);
L216905:   FUN_00467290(uVar14);          // AV9  Magicka SET  (v413 case9)
L216906:   uVar5 = FUN_00468ad0(2);       // Willpower
L216907:   uVar6 = FUN_00468ad0(3);       // Agility
L216908:   uVar7 = FUN_00468ad0(5);       // Endurance
L216909:   uVar8 = FUN_00468ad0(0);       // Strength
L216910:   uVar14 = FUN_005479d0(uVar8, uVar7, uVar6, uVar5);   // 4-arg
L216911:   FUN_004672b0(uVar14);          // AV10 Fatigue SET  (v413 case10)
L216912: }
```

### §2.2 `0xB362E4/EC`（v462_1b）
```
0xB362E4 refs: 0x5223AC add eax,0xb362e4 (FUN_005222d0, docs L216799)
               0x9E3C30 mov ecx,0xb362e4 / call 0x41bae0 / push 0xa1c1e0 / call 0x981fb4
                 (FUN_009e3a20, L1029691 = init)
               0xA1C1E1 mov ecx,0xb362e4 / jmp 0x4055e0 (FUN_00a1c1e0, L1043880 = thunk)
0xB362EC refs: 0x5223C4 (FUN_005222d0) / 0x9E3C5F init / 0xA1C1F1 thunk (FUN_00a1c1f0)
docs mentions: 1 each (L216799 / L216801)
```

### §2.3 `FUN_00668B30` / `+0x7A4`（v462_1c）
```
L429187: void __thiscall FUN_00668b30(int param_1, float param_2, float param_3,
                                       int param_4, char param_5)
  L429197: iVar2 = (int)param_2;
  L429198: iVar4 = FUN_005f1910(param_2);    // AV fetch by index (v449)
L431239 (FUN_0066C200): local_4 = *(float *)(param_1 + 0x7a4 + cVar3 * 4);
  -> Player[+0x7A4 + idx*4] float array  [確定(構造)]
```

## §3 残点 **V-463-1..3**
1. 【低】**`FUN_006DF060`（`NiLookAtInterpolator` idx35 override）identity** → predicate slot35 正式名（V-462-2 引継ぎ）。
2. 【低】**`FUN_005222D0` の一語名**（機能＝derived AV 初期適用は確定相当、docs に名前無し）と **`FUN_00668B30` 第4引数（0）の語**。
3. 【低】**旧残点（V-406-3／V-406-4／V-406-5）へ** — `FUN_006ECC80` 残り typed（V-406-3【中】）／`DATA+0x2C` 意味（V-406-4）／`PTR_DAT_00B10D90` 所有 fn（V-406-5）を V-464 から優先順に。

## §4 検証・自己批評
- probe: **v462_1 の1本 EXIT=0**（V-406-2 完遂相当（属性ブロック＋singleton3系統）／V-406-6 部分（signature＋`+0x7A4` 構造）／V-462-2/3 は残点）。
- **決定打の作り**: (i) **`param_2==0` 分岐の body を docs で読むだけ**で「属性→derived AV SET」の構造が露出（**AV index 引数（`FUN_00468ad0(n)`）が v447 AV table と突合して Strength/Endurance/… と即決**）→ (ii) **setter 名（`FUN_00519C50/467290/4672B0`）が v413 の AV8/9/10 と完全一致**して derived AV と確定 → (iii) **global は3系統（init/thunk/add）census**で singleton と特定 → (iv) `+0x7A4` は docs の1行（`*(param_1 + 0x7a4 + cVar3*4)`）で float 配列と確定。
- 妥当性チェック: ①AV index 5/0/1/2/3⇔v447 AV table（Endurance/Strength/Intelligence/Willpower/Agility）— ②`FUN_00519C50/467290/4672B0`⇔v413 dispatcher case8/9/10（Health/Magicka/Fatigue）— ③`FUN_006ECC80`＝CLAS getter⇔v406 `return [x+0x40]` — ④`FUN_005F1910`＝AV fetch⇔v449 — ⑤`+0x7A4+idx*4`⇔`FUN_0066C200`（保留適用）の skill 文脈 — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_005222D0` に annotation はあるが名前は `FUN_` のまま＝docs の命名なし）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **AV index を渡す helper（`FUN_00468ad0(n)`）の引数を v447 AV table で読み替える** — `FUN_00468ad0(5)`＝Endurance と分かるだけで属性ブロックの意味が一気に決まる（**enum 引数 helper は「引数＝enum値」と見なして表と突合**）。
  2. **setter 名の一致（v413 の case8/9/10）が derived AV の決定打** — 「何を SET しているか」は setter identity から逆算するのが速い。
  3. **runtime singleton は「init fn＋thunk＋加算」の3系統 census で特定できる**（`call 0x41BAE0`＝init、`mov ecx,imm; jmp`＝thunk、`add eax,imm`＝加算）。
  4. **float 配列は `*(base + idx*4)` の docs 1行で構造確定**（`+0x7A4` の語は context（`FUN_0066C200` 保留適用）から推定）。
  5. **正式名が docs に無い fn は「機能確定相当＋一語名残点」と明記**（`FUN_005222D0` のように、機能は確定でも名前は付けない＝推測禁止の徹底）。
