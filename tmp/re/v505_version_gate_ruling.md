# v505 — version 閾値＝feature gate・`FUN_006C4F80` string set・`FUN_004524C0`＝remove の裁定（V-505-1..3 完遂・旧残点 V-406-3 継続第35弾）

- 採番: **v505**（旧残点群消化・probe prefix **v505_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-505-1..3（v504 残点）** — 1=`FUN_006C4F80` 本文（node-set value→field 対応）と map vtable family 確認＋version 閾値（35/63/36）の対応バージョン／2=gated mask 源と Player `+0x80`/`+0x14` identity／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-505-1: version 閾値（35/63/36）の意味** | **[確定相当]（feature gate）** | **context 実読 [確定(decompiled)]: **`L23799`（`FUN_00421000`）: **`if (0x23 < *(byte *)(DAT_00b33b00 + 0x7c)) { if (*(iVar2+0x10) != 0) { sink log("SetSavedAnimation() is being called when there is already a saved animation."); *(iVar2+0x10) = unaff_retaddr; } }` → `0x23 <`（35超）は `SetSavedAnimation()` の version gate [確定相当]**。**`L24063`（`FUN_00421340`）: `case 0x1f: iVar8=FUN_009832e6(iVar12,0,&PTR_PTR_00b03850,&PTR_PTR_00b03b9c,0)`（cast）→ `if ((0x3f < *(byte*)(Player+0x7c)) && (FUN_00446b80(*(iVar8+0xc)+0xc) != 0))` → `0x3f <`（63超）は `case 0x1f`（FormType 31）の cast 先 object の `+0xc` 判定と併用 [確定相当]**、**`L24451`（`FUN_00421A00`）も同型（`0x3f <`＋`FUN_00446b80`）[確定]** → **Player `+0x7C` version byte の閾値（`0x23 <`＝35超／`0x3f <`＝63超／`0x24`＝36）は「version が閾値超ならその処理を有効化する feature gate」[確定相当]**（v503 閾値 census→v504 version byte→v505 gate semantic の3段が完成） |
| **V-505-1: `FUN_006C4F80`（node-set impl 候補）本文** | **[推定相当]（string key set）** | machine head（docs annotation なし）[確定(機械)]: **`cmp byte [ecx+0x10], 0; je`（node `+0x10` byte 判定）→ `mov ebx,[esp+0xc]`（key＝stack 引数）→ **string length 計算（`lea edx,[eax+1]; mov cl,[eax]; …; sub eax,edx`＝strlen）→ `lea esi,[eax+1]; push esi; call 0x401f00`（alloc len+1）→ `mov edi,[esp+0x14]`** → **string key の alloc＋copy を行う node-set [推定相当]**（case-insensitive string map の key 文字列登録）。**value→owner/count field の対応は本 fn では node の `+0x10` byte 判定と key copy のみ＝`+0xC` slot が set＋key の複合 [部分確定]**（docs annotation がないため本文の残りは cap 外＝残点） |
| **V-505-1: `FUN_004524C0`（v500 で見た loop 末尾 fn）** | **[確定相当]（remove）** | L63085 実読 [確定(decompiled)]: **`undefined4 __thiscall FUN_004524c0(int *param_1, undefined4 param_2)`: `iVar4=virtual[*param_1+4](param_2)`（hash）→ bucket → `cVar3=virtual[*param_1+8](param_2, puVar1[1])`（キー比較）→ hit なら **`*(bucket)=*puVar1`（unlink）→ `virtual+0x10(puVar1)`（free 候補）→ `virtual+0x18(puVar1)`（destroy 候補）→ `param_1[3]--`（count decrement）→ `return 1`** [確定(構造)]** → **`FUN_004524C0`＝hash map の remove/delete fn [確定相当]**（v500 で loop 末尾に見た `FUN_004524c0(uVar9)` は remove 呼び出し）。→ **v500「ctor 候補」を訂正（ctor でなく remove）**。**map 4 fn family が「insert（v485/v494）／lookup（v496/v498）／set（v493/v502）／remove（本裁定）」の4種で完成 [確定相当]** |
| **V-505-2: Player `+0x80`（ptr）／`+0x14`（buffer size）** | **[未確定]（残点継続）** | 本 probe 未着手（budget は version gate／node-set／remove に集中） |
| **V-505-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v505_1）

### §2.1 version gates（v505_1c）
```
L23799 (FUN_00421000):
  if (0x23 < *(byte *)(DAT_00b33b00 + 0x7c)) {            // version > 35
    if (*(iVar2 + 0x10) != 0) {
      sink log("SetSavedAnimation() is being called when there is already a saved animation.");
      *(iVar2 + 0x10) = unaff_retaddr;
L24063 (FUN_00421340):
  case 0x1f: iVar8 = FUN_009832e6(iVar12, 0, &PTR_PTR_00b03850, &PTR_PTR_00b03b9c, 0);
  if ((0x3f < *(byte *)(DAT_00b33b00 + 0x7c)) &&          // version > 63
      (FUN_00446b80(*(iVar8 + 0xc) + 0xc) != 0)) { ... }
L24451 (FUN_00421a00): same shape (0x3f < ... && FUN_00446b80 ...)
=> version byte thresholds = feature gates  [確定相当]
```

### §2.2 `FUN_006C4F80` machine head（v505_1a）
```
6C4F80: cmp byte ptr [ecx + 0x10], 0 / je 0x6c4fc5    ; node +0x10 byte gate
6C4F87: mov ebx, dword ptr [esp + 0xc]                 ; key (stack arg)
6C4F8C: mov eax, ebx / lea edx, [eax+1] / mov cl, [eax] / ... / sub eax, edx
        -> strlen(key)
6C4F9D: lea esi, [eax + 1] / push esi / call 0x401f00  ; alloc len+1
6C4FA6: mov edi, dword ptr [esp + 0x14]                ; out ptr
=> string-key node-set (alloc + copy)  [推定相当]  (body beyond cap -> residual)
```

### §2.3 `FUN_004524C0` = remove（v505_1b）
```
L63093: iVar4 = virtual[*param_1 + 4](param_2);              // hash
L63096: cVar3 = virtual[*param_1 + 8](param_2, puVar1[1]);   // key compare
L63098: *(param_1[2] + iVar4*4) = *puVar1;                   // unlink
L63099: virtual[*param_1 + 0x10](puVar1);                    // free [推定]
L63100: virtual[*param_1 + 0x18](puVar1);                    // destroy [推定]
L63101: param_1[3] = param_1[3] + -1;                        // count--
L63102: return 1;
=> remove/delete fn  [確定相当]   (v500 ctor guess corrected)
map family: insert / lookup / set / remove = 4 fns  [確定相当]
```

## §3 残点 **V-506-1..3**
1. 【中】**`FUN_006C4F80` の残り本文**（value→owner/count field 対応の完遂）と **map vtable family の最終確認**（`FUN_00452570`/`FUN_00412D30` の this が BST 系（`BSTCaseInsensitiveStringMap`）か LockFree 系か）＋ **version 閾値の対応バージョン表**（35/63/36 がどの Oblivion version か＝`0x4b4f4c42`＝`"BLOB"` ブロック形式との関係）。
2. 【低】**Player `+0x80`（ptr）／`+0x14`（buffer size）identity** ＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v505_1 の1本 EXIT=0**（V-505-1 version 閾値＝feature gate を context 実読で確定相当・`FUN_006C4F80` string set を推定相当・`FUN_004524C0`＝remove を確定相当／V-505-2 は budget 外で残点／V-505-3 引継ぎ）。
- **決定打の作り**: (i) **version 閾値3箇所（L23799/L24063/L24451）の context を並べる** → `0x23 <` が `SetSavedAnimation()`、`0x3f <` が `case 0x1f` cast と結びつき **feature gate と即決**（**v503 census→v504 version→v505 gate の3段完成**）→ (ii) **`FUN_006C4F80` machine head** で strlen＋alloc が string key set と判明 → (iii) **`FUN_004524C0` decompiled** で `count--`＋unlink が remove と判明（**v500 ctor 猜を訂正**）。
- 妥当性チェック: ①`0x23 < x`⇔`x >= 36`⇔v502 `cmp 0x24`（同一）— ②`0x3f < x`⇔`x >= 64`— ③`FUN_004524C0` の `param_1[3]--`⇔v485/v494 insert の `param_1[3]++`（対）— ④`virtual+0x10`/`+0x18`⇔free/destroy（v494 の `+0x10` existing op と整合）— ⑤strlen＋`FUN_00401F00(len+1)`⇔string key alloc（v492 alloc `0x7C`／v493 alloc `8` に続く size alloc 系）— **五方向矛盾なし**。
- docs との不整合: **①v500 ruling「`FUN_004524C0` は map ctor 候補 [推定]」→ 本裁定で remove（count--）と確定訂正**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **version byte の閾値が「feature gate」として機能する** — **閾値比較の owning fn（`SetSavedAnimation()` 等の実名）を読むと gate の意味が分かる**（**閾値 census→log→gate の3段 field調査法が完成形**）。
  2. **`count--`＋unlink＋`virtual+0x10/+0x18` が remove の定番**（insert の `count++` と対で覚える）。
  3. **vtable slot family の4 fn（insert/lookup/set/remove）が揃うと container が完成**（**map 系の解剖が v485→v505 で10回の積み上げ**）。
  4. **machine head の strlen 計算（`lea edx,[eax+1]`→`sub eax,edx`）が「string key」の目印**（alloc size＝len+1 と併せて）。
  5. **推定の訂正（v500 ctor→v505 remove）も裁定に明記**（**誤推定の軌跡が残る＝追跡可能性**）。
