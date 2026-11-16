# v436 — record flag consumer・name 配列の分離・dump fn 帰属の裁定（V-436-1..3 完遂・派生残点第28弾）

- 採番: **v436**（派生残点群・probe prefix **v436_1..v436_14 の14本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v435 の残点 **V-436-1【低】（console/function の flag byte consumer 特定）／V-436-2【低】（flat array 先頭10件の consumer と `0xB108EC` の index 源）／V-436-3【低】（`FUN_00407AE0` の command 名と type `0x3` vs `0x15` の語義差）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-436-1: flag byte（`+0x10`）の consumer** | **[確定]** | **2系統を特定**: **(A) events＝`FUN_004ff4c0 @0x4FF918`**（v435 確定・`cmp byte [eax+0x10],0` → quest 判定 → `'Invalid block type for quest script.'`）／**(B) console/function＝`FUN_004FAB20 @0x4FADB0`**: `cmp byte [eax+0x10],0; je 0x4fadba` → **flag≠0 かつ `test ecx,ecx; je 0x4fadde`（ecx==0）で FAIL（`xor al,al; ret`＝実行拒否）** → flag==0 なら **`mov eax,[eax+0x18]`（record handler）を6引数で `call eax`（`0x4FADD5`）**。→ **flag byte ＝「object/reference receiver 必要」[確定]**（event の「quest 不正」と同根） |
| **ecx（flag 判定の対象）の正体** | **[確定]** | `0x4FAD53..0x4FAD74`: `edx=[esp+0x1c]` → `edx=[edx+8]` → **`movzx ebx,byte [edx+4]; add ebx,-0x31; cmp ebx,2; ja → ecx=0` / else `mov ecx,edx`** ＝ **`byte[+4] ∈ {'1','2','3'}（0x31..0x33）の場合のみ ecx=reference object、否则 NULL** [確定(命令)]。→ **flag=1 は「reference 付き呼び出し」を要求** |
| **V-436-2: `0xB108EC` と `0xB10900` の関係** | **[確定]（v435 訂正）** | **v435 の「1配列に2段混在」は誤り・訂正**: **`0xB108EC`＝5件の sequence group 名（`LowerBody/LeftArm/LeftHand/RightArm/SpecialIdle`）** と **`0xB10900`＝16件の text key 名（`Start`×8/`End`×2/`Attach/Detach/Hit`×2/`Attack/Attach`）は「隣接する2つの独立配列」** — **base refs が別**（`0xB108EC`→`FUN_00407AE0`／`0xB10900`→`FUN_0051aa10`・`FUN_0051b490`）で**各 base が別 fn から使われる [確定]** |
| **V-436-2: flat array 先頭10件（`0xB10290..0xB102B4`）** | **[確定(否定側)]＋[推定高]** | **全セクションで ref 0 件**（`'Left Foot'` 文字列自体の ref も `.data 0xB10290` のみ）。consumer 候補 `FUN_0051b490`（animation name 系）は **`0xB102B8`/`0xB102C8`/`0xB102E0`/`0xB102EC`/`0xB102F0`/`0xB10900` の6 base のみ使用・`0xB10290` 不使用** → **先頭10件（foot4＋state6）は未使用 dead data [推定高]**、または signed negative index 経由の到達 [未確定] |
| **V-436-3: `FUN_00407AE0` の owning fn** | **[未確定]（docs 欠落を発見）** | **唯一 call site＝`0x40DEFC` [確定]**。machine 上 `0x40DEFC` は **`0x40D4D0..0x40DF74` の連続コード内（int3 境界なし）**で、annotation 範囲では `FUN_0040d4d0`。**しかし `FUN_0040d4d0` の decompiled body（L8522..L8659）には dump call が存在しない**（`FUN_00407ae0`／`FUN_005791a0`／`FUN_005791e0`／`FUN_00579220` 全て不在）＋ **body tail は machine `0x40D7BD..0x40D7FF` に対応**（`FUN_0040fda0`→`0x40D7BD`・`FUN_00411100`→`0x40D7D9`・`FUN_00977c50(2)`→`0x40D7E0`・ret `0x40D7FF`）→ **`0x40D800..0x40F640` 帯に annotation が1件も無い＝`docs/Oblivion_decompiled.c` の function 欠落（docs vs code の不整合）[確定(不整合)]**。dump fn の identity は docs から **[未確定]** |
| **V-436-3: `FUN_0040d4d0` の identity（v435 訂正）** | **[推定高]（v435 撤回）** | **v435 の「`FUN_0040d4d0`＝console command handler [推定高]」は撤回** — body 実読で **`_sprintf(acStack_108, "%s%03d", PTR_s_TestCameraPath_00b02d50, DAT_00b02d58); CreateDirectoryA(...)`（L8632-8633）** ＝ **`TestCameraPath001/002/...` の連番ディレクトリ作成**を含む → **camera-path test の debug tick [推定高]**（`void FUN_0040d4d0(void)`・`FUN_00977c50(3)`〜`(2)`・`FUN_00572e30(2)` カウンタ `DAT_00b33418` 0..10）。**`FUN_00977c50`＝`return 1` の stub（decompiled では `void(void)`・引数3/2 は未使用に見える）[確定]** |
| **dump gate `0xB3B72A`** | **[確定]**（v435 補強） | **set＝`FUN_005d6960`（ctor of `.?AVSleepWaitMenu@@`・vtable `0xA6D7C4`）内 `0x5D69CA: mov byte [0xb3b72a],1`**（`call 0x45a500` false かつ `virtual [..+0x38]()` 非0 の条件付き）／**test+clear＝`0x40DF10: cmp byte [0xb3b72a],bl; je` → `0x40DF23: mov byte [0xb3b72a],bl`**（dump call `0x40DEFC` の直後）→ **one-shot flag [確定(命令)]**。`FUN_005d6960` の E8 caller＝`0x5D6D03`（`FUN_005d6bc0` L340155 内・`param_1==4` gate） |
| **V-436-3: type `0x3` vs `0x15`（両方 name='ObjectID'）** | **[未確定]（残点）** | 調査せず。`AddItem desc0 'ObjectID' type=0x3`／`OnActorEquip desc0 'ObjectID' type=0x15` の語義差は **V-437-1 へ** |

## §2 証拠（v436_1..v436_14）

### §2.1 flag consumer（v436_4/v436_5）
```
FUN_004FAB20 (console/function record):
  4FAD53: mov edx, [esp+0x1c] / test / je 0x4fad76
  4FAD5F: mov edx, [edx+8] / test / je 0x4fad76
  4FAD66: movzx ebx, byte [edx+4] / add ebx,-0x31 / cmp ebx,2 / ja 0x4fad76
  4FAD74: mov ecx, edx                 ; ★ ecx = reference object（'1'/'2'/'3' 判定）
  4FAD76: cmp byte [esp+0x54],0 / mov edx,[eax+0x14] / je 0x4fadb0
  4FADB0: cmp byte [eax+0x10],0        ; ★ flag byte
  4FADB4: je 0x4fadba                  ; flag==0 -> proceed
  4FADB6: test ecx,ecx / je 0x4fadde   ; flag!=0 && no-ref -> FAIL
  4FADBA: mov eax,[eax+0x18] / test / je 0x4fadde
  4FADD5: call eax                     ; record handler（6 args / add esp,0x20）
  4FADDE: ... xor al,al / ret          ; FAIL
```
- events 側（v435 確定）: `FUN_004ff4c0 @0x4FF918 cmp byte [eax+0x10],0` → quest 判定 → `'Invalid block type for quest script.'`（`0xA4B524`）。

### §2.2 name 配列の分離（v436_3/v436_4）
```
0xB108EC: LowerBody/LeftArm/LeftHand/RightArm/SpecialIdle   (5, sequence groups)
0xB10900: Start×8/End×2/Attach/Detach/Hit×2/Attack/Attach    (16, text keys)
base refs: 0xB108EC -> FUN_00407AE0（[eax*4+0xb108ec] @0x40A2A1）
           0xB10900 -> FUN_0051aa10（@0x51AA2D）, FUN_0051b490（@0x51B890）
FUN_0051b490 使用 base（v436_3）: 0xB102B8(×2)/0xB102C8(×1)/0xB102E0(stricmp)/
                                  0xB102EC(×2)/0xB102F0(×2)/0xB10900(×1)  — 0xB10900 使用✓
```

### §2.3 dump fn 帰属の不整合（v436_7..v436_14）
```
machine: 0x40D4D0..0x40DF74 は int3 境界なし（唯一の CC run は 0x40D4CE → fn start 0x40D4D0）
         ret @0x40D7FF（CC padding=0）/ ret @0x40DF73（CC padding=12 → next fn 0x40DF80）
body tail mapping（v436_12/v436_14）:
  L8623 FUN_004032d0  -> 0x40D71C
  L8625 FUN_00411750  -> 0x40D72F
  L8651 FUN_0040fda0  -> 0x40D7BD   （range 内で唯一の call site）
  L8655 FUN_00411100  -> 0x40D7D9   （range 内で唯一）
  L8656 FUN_00977c50(2) -> 0x40D7E0  （0x40D7D9 直後）
  L8657 __security_check_cookie / L8658 return  -> ret @0x40D7FF
body 内 grep（L8522..L8662）: '407'/'ae0'/'FUN_005791a0'/'FUN_005791e0'/'FUN_00579220'/'FUN_00434020'/'FUN_005732d0' すべて NONE
                             'FUN_00440af0' -> L8556 のみ
annotation: 0x40D4D0..0x40F640 の間（両端除く）に annotation 0 件（raw grep `@ 0x40[de]`＝2件のみ:
            0x40d160/0x40d4d0、`0x40de`/`0x40df` は decompiled.c 内に1件も出現しない）
```
→ **`FUN_0040d4d0` body は machine `0x40D7FF` で終了、次 annotation は `0x40F640` ＝ `0x40D800..0x40F640`（8560 bytes）に function annotation が無い**。

### §2.4 dump gate と SleepWaitMenu（v436_1/v436_5/v436_6）
```
FUN_00977c50: L977627 `undefined1 FUN_00977c50(void) { return 1; }`  [stub]
FUN_0040d4d0 body: L8632 _sprintf(acStack_108,"%s%03d",PTR_s_TestCameraPath_00b02d50,DAT_00b02d58)
                   L8633 CreateDirectoryA(acStack_108,0)
FUN_005d6960: RTTI vtable 0xA6D7C4 -> `.?AVSleepWaitMenu@@`（v436_5）
              0x5D69CA: mov byte [0xb3b72a],1   （0x5D69C4 call [edx+0x38] 非0 時）
              E8 caller: 0x5D6D03（FUN_005d6bc0 L340155・`param_1==4 && DAT_00b3b728==0` gate）
0x40DEFC: call 0x407ae0 (dump) / 0x40DF01: call 0x5791a0 / 0x40DF10: cmp byte [0xb3b72a],bl
          0x40DF23: mov byte [0xb3b72a],bl (clear) / 0x40DF29: call 0x440af0
          0x40DF54: mov ecx,ebp; call 0x40d4d0
```

## §3 残点 **V-437-1..3**
1. 【低】**type `0x3` vs `0x15`（両方 name='ObjectID'）の語義差** — arg descriptor の type コード体系（0..0x1F の対応表）の特定。
2. 【低】**`docs/Oblivion_decompiled.c` の function 欠落範囲の特定** — `0x40D800..0x40F640` 帯に annotation 0 件。他の帯にも同種の gap があるかの census（欠落が広範か単発か）。**＋ dump call（`0x40DEFC`）の owning fn identity**（欠落 fn の machine 特定＝`0x40D800..0x40DF74`）。
3. 【低】flat array 先頭10件（`0xB10290..0xB102B4`）の到達経路（signed negative index か dead data か）＋ `FUN_00407AE0` の `%s` 4本目の index 源 `[esp+0x5c]` の由来。

## §4 検証・自己批評
- probe: **v436_1..v436_14 の14本全 EXIT=0**（V-436-1 完遂／V-436-2 完遂（v435 訂正含む）／V-436-3 は owning fn を [未確定] とし docs 欠落を発見）。
- **決定打の作り**: (i) **`cmp byte [reg+0x10],0` の全 .text census（143件）から record-base のものだけを context decode で選別** → `FUN_004FAB20` を特定 → (ii) **ecx の来源を aligned decode で遡り `byte[+4]-0x31<=2` ゲートを発見**して flag 語義を確定 → (iii) **`0xB10900` base ref census で `0xB108EC` と分離**（v435 訂正）→ (iv) **dump 帰属は「body tail の call site が range 内で唯一」の対応で machine 範囲を逆算**し、**body 内 grep の不在（`407`/`5791a0`）で docs 欠落を確定**。
- 妥当性チェック: ①`FUN_0040fda0`/`FUN_00411100` が range 内に各1 call site⇔body tail 対応 — ②body 内 `FUN_00440af0` は L8556 存在⇔`FUN_005791a0` は不在（＝body が `0x40DF01` を含まない）— ③`0x40D4D0..0x40F640` の annotation 0 件⇔raw grep `@ 0x40[de]`＝2件 — ④`FUN_00977c50`＝`return 1` stub⇔`push 3/2` の引数が body 上見えない — **四方向矛盾なし**（ただし ②が docs 欠落の根拠）。
- docs との不整合: **①`docs/Oblivion_decompiled.c` に function 欠落（`0x40D800..0x40F640` 帯に annotation 0 件）— dump call `0x40DEFC` を含む fn が docs に無い [確定(不整合)]**／**②v435 ruling の「`FUN_0040d4d0`＝console command handler [推定高]」は撤回**（body は `TestCameraPath` 連番ディレクトリ作成＝camera-path test debug tick [推定高]）／**③v435 ruling の「`0xB108EC` に5 group＋16 key が混在」は撤回**（隣接2配列）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **flag/byte field の consumer 特定は「全 .text の該当 byte-pattern census → context decode で record-base のみ選別」** — 143件の `cmp byte [reg+0x10],0` の中から record を見るのは context（直前の `mov eax,[eax+0x14]`＝arg pool 読み）で判別できる。
  2. **consumer の第2引数（ecx 等）まで遡ると flag の語義が確定する** — `byte[+4]-0x31<=2` ゲートが「reference object の型判定」だと分かれば flag＝「reference 必要」と一撃で決まる。
  3. **docs 欠落の発見法＝「body tail の call site が machine range 内で唯一」の対応で body の machine 範囲を逆算し、範囲外に annotation が無いか突合する** — annotation 範囲（次 annotation まで）と body tail（最後の return の machine 位置）が乖離すれば欠落。
  4. **body 内 grep（`FUN_00407ae0` 等）の「不在」は docs 欠落の決定打** — machine に call があるのに body に無い＝その machine位置が docs の body 外。
  5. **v435 の2つの [推定高]（console handler／1配列混在）が本 probe でともに訂正** — **[推定高] は次の probe で必ず反証を試す**。
