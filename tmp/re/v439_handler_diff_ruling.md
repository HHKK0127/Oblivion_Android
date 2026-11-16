# v439 — type handler 差（0x03 vs 0x15）・flag A/B semantic・docs 欠落 census の裁定（V-439-1..3 完遂・派生残点第31弾）

- 採番: **v439**（派生残点群・probe prefix **v439_1..v439_8 の8本全 EXIT=0**（v439_8 初回は section 境界 annotation の .text 外越界で IndexError → range guard 追加で再実行 EXIT=0））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v438 の残点 **V-439-1【低】（`0x03` vs `0x15` の handler 差と per-type flag A/B の semantic）／V-439-2【低】（docs 完全性の最終裁決＝欠落率推定）／V-439-3【低】（`[esp+0x5c]` の path 依存供給元と flat array 先頭10件の到達経路）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-439-1: type `0x03` handler（`table[0]=0x4FDFA7`）の中身** | **[確定]** | **`movzx eax, byte [eax+4]`（resolved object の byte フィールド）→ `call 0x469520`（switch validation）→ 失敗で error**。error 文言 **`"Invalid inventory object '%s' for parameter %s.\r\nCompiled script not saved!"`（`0xA4B050`）** [確定(実文字列)]。`FUN_00469520`（L83027）＝**byte 引数 switch: case `{0x13,0x14,0x15,0x16,0x19,0x1A,0x1B,0x21,0x22,0x26,0x27,0x28,0x2A,0x2B}` → return 1**（default 返り値は body 截断で未確認）→ **inventory 資格の byte-set 判定 [確定]（値集合＝FormType enum の対応は [未確定]）** |
| **type `0x15` handler（`table[18]=0x4FE5CC`）の中身** | **[確定]** | **`push 0; push 0xB03268; push 0xB02F9C; push 0; push eax; call 0x9832e6`** — `FUN_009832e6`（L985474）の signature は **`int FUN_009832e6(int*, int, _s_RTTICompleteObjectLocator*, TypeDescriptor*, int)` ＝ MSVC `__RTDynamicCast`** [確定(decompiled)]。引数データ実読: **`0xB02F9C`＝TypeDescriptor `".?AVTESForm@@"`／`0xB03268`＝`".?AVTESObject@@"`**（隣接で `0xB03054=".?AVTESObjectREFR@@"`/`".?AVActor@@"`、`".?AVTESNPC@@"` も確認）→ **resolved object を `TESForm`→`TESObject` に RTTI dynamic_cast で検証 [確定]**。失敗 error **`"Invalid object '%s' for parameter %s.\r\nCompiled script not saved!"`（`0xA4AB30`）** [確定(実文字列)] |
| **`0x03` vs `0x15` の語義差** | **[確定]** | **`0x03`＝FormType の byte-set 判定（inventory 资格）／`0x15`＝RTTI class 判定（dynamic_cast）** — 検証機構が異なる（**byte field switch vs RTTI**）。error 文言も `inventory object` vs `object` で分離 [確定] |
| **per-type flag 配列 `0xB0A54C` の layout** | **[確定]（実データ）＋[部分確定]（+4 フィールド）** | **stride-8・38 records（type 0x00..0x25）＝`{u8 A @+0, u8 B @+1, u16 pad @+2, u32 id @+4}`**。**`id` ＝ `type+1`（type0→1, type1→2, ..., type0x24→37）が全件一致、ただし type0x25 のみ `id=0x00A51B8C`（rdata ptr）で不一致 → 表末尾の特別扱い [部分確定]**。`A`/`B` は consumer（`bl` / `cmp byte`）で実使用 [確定]、`id` の consumer は **[未確定]** |
| **v438 残点の「B=1,2,3... incrementing」の正体** | **[確定]（誤読の訂正）** | **incrementing していたのは `+4` の u32（＝`type+1`）であって `B` ではない。`B`（byte@+1）は `{0,1}` の二値**（v439_2 の full dump で全38件確認）→ **v438 ruling の当該記述は本裁定で訂正** |
| **flag A（byte@+0）の semantic** | **[確定]（分岐構造・error 文言・値集合）＋[推定高]（語義命名）** | **`A=1 は type ∈ {0x01 Integer, 0x02 Float, 0x17 Stage} のみ**（38件中3件、v439_2 full dump）[確定]。consumer: `4FDEB0: mov bl,[eax*8+0xB0A54C]` → `4FDF22: test bl,bl; jne 0x4FDF44`（**A≠0 は variable-guard を skip**）／**A==0 は guard 実行**: `mov edx,[esp+0x228]; test; ja 0x4FECDD` + `mov cl,[esp+0x22c]; test; jne 0x4FECDD` → error **`"Parameter %s may not be a variable.\r\nCompiled script not saved!"`（`0xA4A800`）** [確定(命令+実文字列)] → **A=0 の type では引数に変数（variable）を使えない／A=1（Integer/Float/Stage）のみ変数許容 [確定(構造)]** → **A＝「変数許容 type」マーカー [推定高]**。guard の operand（`[esp+0x228]` dword / `[esp+0x22c]` byte）は **直書き・`lea` とも全 fn 内で検出0件**（`4FDE3D` の cookie 書き込みは push 前 displacement で別アドレス）→ **`FUN_004fd7c0` 呼出しの out-param 経由と推定 [未確定]** |
| **flag B（byte@+1）の semantic** | **[確定]（分岐）＋[推定高]（語義命名）** | **B==0 → 9-case generic path（`cmp eax,0x21; ja` → byte remap `0x4FEF5C` → `jmp [eax*4+0x4FEF38]`）／B≠0 → 35-case object path（`add eax,-3; cmp eax,0x22; ja error; jmp [eax*4+0x4FEEAC]`）** [確定（v438 で機械確認・本 probe で flag 値と突合）]。**B=1 は27 object/form 系 type（0x03/0x04/0x06/0x07/0x09/0x0B..0x11/0x13..0x15/0x18..0x1B/0x1D..0x20/0x22..0x25）、B=0 は11 の scalar/keyword 系（0x00 String/0x01/0x02/0x05/0x08/0x0A/0x12/0x16/0x17/0x1C/0x21）** [確定（v439_2 dump）→ **B＝「object resolve が必要な type」マーカー [推定高]** |
| **V-439-2: docs 完全性の最終裁決** | **[確定]（docs は不完全）＋[部分確定]（下限推定）＋[未確定]（総数）** | **(i) 欠落の実在 [確定]**: gap `0x9E62B0(FUN_009e62b0)→0x9F1C00`（0xB950）内で **segment start 3件（`0x9E6640/0x9E6720/0x9E6CC0`）を machine decode → すべて実 function**（`push imm; push imm; mov ecx,imm; call 0x419a70; push imm; call 0x981fb4; pop ecx; ret` ＝ registration stub、head annotation=(none)）＋ **docs raw grep `'0x9e66'/'0x9e67'` が0 hits・`'0x9e62b0'` は annotation 1件のみ**、docs body は22行 → **この gap 内に ≥64 個の未注記 function 実体 [確定]**。(ii) **集計 [部分確定]**: gaps>0x200 の2734件 → func-like segments **4471**、**1/gap 超過分＝1737 candidates**（minseg=0x30 で `<0x30` の小 stub は**除外済み＝下限**）／別条件 census では内部境界4558件のうち **ret 確認（前 segment 末尾＝C2/C3/CB）＝2037（44.7%）**。大 gap では CC-run 2394件（0xA100）/1572件（0x6BC0）等の **stub 集積地帯**を検出 → **docs 欠落は実在・下限 ~1.7k candidates、総数は [未確定]**（proxy 法（v438）と segment 法は threshold 依存で厳密一致しない） |
| **V-439-3: `[esp+0x5c]` の供給元** | **[部分確定]** | **fill 点6件を確定**: direct write 3（`407CBA` edx／`407E5B` ecx／`408F72` eax=0）＋ **`lea ecx,[esp+0x5c]` 経由2 call**（`408FCF→FUN_00404fd0`＝`uint __fastcall(uint*)`・`ushort(+4)==0xffff` 時のstrlen系 loop [推定高]／`409027→FUN_00402da0`＝`void __fastcall(uint*)`: `FUN_00401f20(*p); *p=0; +6=0; +4=0` ＝ **string-struct dtor/reset [確定(decompiled)]`）＋ `408FC6` 付近の `lea ecx,[esp+0x64]; call 0x4028d0`（隣接 string ctor）。**`0x40A29D` への branch は0件（fall-through のみ）[確定]** → 直近 write は path 依存（`408F72`=0 か dtor ゼロ化か string build 経由か）→ **value at read = [未確定]** |
| **V-439-3: flat array 先頭10件** | **[推定高]（維持）** | 本 v439 群で追加証拠なし → v437/v438 と同結果（ref0件＋`mov esi,1` 起点）で **dead data [推定高] 維持** |

## §2 証拠（v439_1..v439_8）

### §2.1 handler 差（v439_1/v439_2/v439_3）
```
(A) type 0x03 handler @0x4FDFA7:
  4FDFA7: cmp dword [esp+0x234],0 / jne 0x4fe85b
  4FDFB5: mov eax,[esp+0x238] / test / je 0x4fdfd5
  4FDFC0: movzx eax, byte [eax + 4]        ; object の byte field
  4FDFC5: call 0x469520                    ; FUN_00469520 switch
  4FDFCD: test al,al / jne 0x4fe85b        ; 1 -> OK
  4FDFD5..: error push 0xa4b050
            -> "Invalid inventory object '%s' for parameter %s.\r\nCompiled script not saved!"
FUN_00469520 (L83027): switch(param_1) { case 0x13,0x14,0x15,0x16,0x19,0x1a,0x1b,
                                          0x21,0x22,0x26,0x27,0x28,0x2a,0x2b: return 1;
                                          default: ... }

(B) type 0x15 handler @0x4FE5CC:
  4FE5E5: push 0 / push 0xb03268 / push 0xb02f9c / push 0 / push eax
  4FE5F4: call 0x9832e6                    ; __RTDynamicCast
  4FE5FC: test eax,eax / jne 0x4fe85b      ; non-null -> OK
  4FE619: push 0xa4ab30
          -> "Invalid object '%s' for parameter %s.\r\nCompiled script not saved!"

data: 0xB02F9C: 80 3E AA 00 | ".?AVTESForm@@"      (TypeDescriptor)
      0xB03268: 80 3E AA 00 | ".?AVTESObject@@"    (TypeDescriptor)
      0xB03054: 80 3E AA 00 | ".?AVTESObjectREFR@@" / ".?AVActor@@"
                ... ".?AVTESNPC@@"
FUN_009832e6 (L985474): int FUN_009832e6(int*, int, _s_RTTICompleteObjectLocator*,
                                          TypeDescriptor*, int)  == __RTDynamicCast
```

### §2.2 flag 配列と dispatch（v439_1/v439_2/v439_4）
```
0xB0A54C stride 8, 38 records: {u8 A, u8 B, u16 pad, u32 id}
  type 0x00: A=0 B=0 id=1      type 0x13: A=0 B=1 id=20
  type 0x01: A=1 B=0 id=2      type 0x14: A=0 B=1 id=21
  type 0x02: A=1 B=0 id=3      type 0x15: A=0 B=1 id=22
  type 0x03: A=0 B=1 id=4      type 0x16: A=0 B=0 id=23
  type 0x04: A=0 B=1 id=5      type 0x17: A=1 B=0 id=24
  type 0x05: A=0 B=0 id=6      ... (id == type+1 for 0x00..0x24)
  type 0x12: A=0 B=0 id=19     type 0x25: A=0 B=1 id=0x00A51B8C (不一致)
  A=1 集合 = {0x01 Integer, 0x02 Float, 0x17 Stage}（38件中3件）
  B=1 = 27 object/form 系 / B=0 = 11 scalar 系

FUN_004fde30 dispatch (v439_4):
  4FDEB0: mov bl, byte [eax*8 + 0xB0A54C]      ; A
  ...
  4FDF22: test bl, bl / jne 0x4FDF44           ; A!=0 -> guard skip
  4FDF24: mov cl, byte [esp+0x22c]
  4FDF2B: mov edx, dword [esp+0x228]
  4FDF34: test edx,edx / ja 0x4FECDD           ; -> error
  4FDF3C: test cl,cl  / jne 0x4FECDD           ; -> error
  4FECF1: push 0xA4A800 -> "Parameter %s may not be a variable.\r\nCompiled script not saved!"
  4FDF44: mov eax,[ebp] ; 4FDF47: cmp byte [eax*8+0xB0A54D],0 (B)
  4FDF83: mov eax,[ebp] ; 4FDF86: cmp byte [eax*8+0xB0A54D],0 ; je 0x4fe88b (B==0 -> 9-case)
                          else add eax,-3 / cmp eax,0x22 / ja / jmp [eax*4+0x4FEEAC] (35-case)
operand provenance: 直書き [esp+0x218/0x21c]（push 前 displacement）= 0件、
                    lea [esp+0x218/0x21c/0x228/0x22c] = 0件（全 fn 範囲 linear scan）
  -> out-param 経由（FUN_004fd7c0 呼出し）と推定 [未確定]
```

### §2.3 docs 欠落 census（v439_4..v439_8）
```
sampled gaps (CC runs >= 4 in [prev_start, next_start)):
  0x11000 gap: 2 runs / 0xB950 gap: 718 runs / 0xA100 gap: 2394 runs /
  0x6BC0 gap: 1572 runs / 0xEA0 gap: 0 / 0xE80 gap: 1 or 2
segment structure (CC >= 8, seg >= 0x30):
  0xB950 gap -> 66 segments (0x54/0x45/0x85/0x41/0x65... all >= 0x30)
  0x61F0 gap -> 2 segments (0x1AE1 + 0x4700)
  0x0BF0 gap -> 1 segment  /  0x080A gap -> 2 segments (0x7BC + 0x40)
verified segment starts (all real fns, head annotation = none):
  0x9E6640: push 0x64 / push 0xa57480 / mov ecx,0xb36768 / call 0x419a70
            / push 0xa1d5f0 / call 0x981fb4 / pop ecx / ret
  0x9E6720: push 0x3e8 / push 0xa574d8 / mov ecx,0xb36790 / call 0x419a70 ...
  0x9E6CC0: push 0xf / push 0xa577a4 / mov ecx,0xb36888 / call 0x419a70 ...
docs: FUN_009e62b0 body = L1031385..L1031406 (22 lines); next annotation L1031407
      raw grep '0x9e66'/'0x9e67' = 0 hits / '0x9e62b0' = 1 hit (annotation only)
aggregate (gaps > 0x200): 2734 gaps, 4471 func-like segments (table-like 0),
      extra beyond 1/gap = 1737 candidates (minseg 0x30 -> small stubs excluded)
ret-boundary census: internal boundaries 4558, ret-confirmed 2037 (44.7%),
      unconfirmed 2521 (tail-jmp E9 epilogue / intra-function CC padding を含む)
      NOTE: v439_7(v439_8?) boundary 定義差 — seg_start reset 位置が
      「run>=8 のみ」(v439_7) vs 「run>=8 かつ prev>=0x30」(v439_8) で
      計数が異なる（4471 vs 4558）→ 閾値依存を明記。
```

### §2.4 `[esp+0x5c]`（v439_8）
```
branches (E9/EB/0F8x/7x) targeting [0x40A280, 0x40A2A8) in FUN_00407AE0: 0
  -> 0x40A29D の read は fall-through のみ [確定]
fill sites (v438_8+v439_8): direct 407CBA/407E5B/408F72(=0) + lea 2:
  408FCF -> FUN_00404fd0: uint __fastcall(uint* p): if (*(ushort*)(p+1)==0xffff)
           { strlen-style loop over (char*)*p }  [推定高: string inspect/assign]
  409027 -> FUN_00402da0: void __fastcall(uint* p): FUN_00401f20(*p); *p=0;
           *(u16*)(p+6)=0; *(u16*)(p+1)=0;      [確定: string-struct dtor/reset]
  (adjacent 408FC6: lea ecx,[esp+0x64] / call 0x4028d0 = neighbor string ctor)
```

## §3 残点 **V-440-1..3**
1. 【低】**`FUN_00469520` の case 値集合の意味付け** — `{0x13,0x14,0x15,0x16,0x19,0x1A,0x1B,0x21,0x22,0x26,0x27,0x28,0x2A,0x2B}` が Oblivion FormType enum のどの値か（docs/GT の FormType 定義との突合）＋ **`0x03` handler が読む `byte[obj+4]` フィールドの identity**（FormType か他か）。
2. 【低】**flag A の他 consumer**（`bl` 設置 `4FDEB0` 以外に A を読む箇所）と **flag 配列 `+4` の `id=type+1` を読む consumer**（`0x00A51B8C` の末尾不一致を含む表境界の意味）。
3. 【低】**docs 欠落数の精密化** — (i) `minseg=0x10` で小 stub（`<0x30`）を含む再 census、(ii) ret-confirmed 2037 gap 候補の docs-body-line vs machine-span 突合サンプリング（Ghidra 単一巨大 fn に包含されたケースを除外）＋ **`[esp+0x5c]` の path CFG**（`0x40A29D` 到達時の直近 write を到達経路ごとに確定）。

## §4 検証・自己批評
- probe: **v439_1..v439_8 の8本（v439_8 初回 IndexError → range guard 追加で再実行 EXIT=0、他7本は初回 EXIT=0）**（V-439-1 完遂／V-439-2 完遂（不完全性は確定・総数は[未確定]）／V-439-3 部分解）。
- **決定打の作り**: (i) **handler body の直接 decode（`movzx eax,[eax+4]`→`call 0x469520` vs `push …TD…; call 0x9832e6`）で検証機構の種別を分離** → (ii) **error 文言2種の実読（`Invalid inventory object` vs `Invalid object`）で semantic を裏付け** → (iii) **`FUN_009832e6` の decompiled signature（`_s_RTTICompleteObjectLocator*`/`TypeDescriptor*`）で `__RTDynamicCast` 同定、引数 data 実読（`?AVTESForm@@`/`?AVTESObject@@`）で検証先 class を確定** → (iv) **flag 配列を38件 full dump して `{A,B,pad,id}` を構造化** → (v) **`test bl,bl` の分岐先を trace し error 文言 `may not be a variable` に接続** → (vi) docs 欠落は **segment 列挙→segment start の machine decode（registration stub 群）→docs raw grep 0 hits** の3段で実証、総数は **aggregate census（2 段階閾値）** で下限推定。
- 妥当性チェック: ①`0x03` error=`inventory object`⇔`FUN_00469520` の byte-set switch — ②`0x15` error=`object`⇔RTTI `TESForm→TESObject` cast — ③A=1={Integer,Float,Stage}⇔variable 許容（script variable が存在する型）⇔error 文言 — ④`id=type+1`（0..0x24 全一致）⇔type 0x25 のみ不一致＝表末尾 — ⑤unannotated stub の docs grep 0 hits⇔docs body 22行 vs machine 47KB — ⑥`0x40A29D` への branch 0 件⇔fall-through — **六方向矛盾なし**（④の末尾1件のみ [部分確定]）。
- docs との不整合: **①docs に function 欠落が実在 [確定]（gap `0x9E62B0→0x9F1C00` で ≥64 個の未注記 registration stub を machine 実読＋docs grep 0 hits で実証、aggregate で下限1737 candidates）**／②v438 ruling の「flag B=1,2,3... incrementing」記述は **`+4` u32（type+1）の誤読と訂正（B は二値）**／③v437 ruling は本裁定と整合（不整合なし）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **同名 type（'ObjectID'）の別 handler は「error 文言＋呼び出し helper の種別（byte switch vs RTTI cast）」で一撃分離できる** — handler body 先頭の `call` 先を decompiled signature で照合すれば検証機構が即決まる（`__RTDynamicCast` は引数が RTTI TD の型で判別）。
  2. **flag/byte 配列は full dump して struct（`{u8,u8,u16,u32}`）に復元する** — 「incrementing B」のような byte 列の局所観察は、実際には `+4` の u32 だった（**盤面全体 dump が誤読を検出する**）。
  3. **docs 欠落の実証手順＝「gap 内 segment start を machine decode →実 function と確認→docs raw grep が0 hits」** — aggregate 数値だけでは不十分、**最低1クラスタの実 function 実読が必須**（本件 registration stub 群）。
  4. **census の閾値（minseg/mincc）と boundary 条件のネスト位置で計数が変わる（4471 vs 4558）** — 数値を引用するときは**定義を併記**する。
  5. **section 境界 annotation（`.text` 外の次 annotation）で range が破綻する** — gap walk には `o<0 or end>len(region)` guard が必須（v439_8 初回 IndexError の教訓）。
  6. **`[reg+disp]` の provenance は「直書き0件＋lea 0件」なら out-param 経由を疑う**（push 前後の displacement 別名も忘れない：push 前 `X` ＝ push 後 `X+0x10`）。
