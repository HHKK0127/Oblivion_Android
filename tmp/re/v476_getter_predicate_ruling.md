# v476 — `FUN_004A9700` getter 確定・`FUN_004B2A00` predicate 実読・v475 elem 推定撤回の裁定（V-476-1..3 完遂・旧残点 V-406-3/V-406-4 継続第6弾）

- 採番: **v476**（旧残点群消化・probe prefix **v476_1..v476_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-476-1..3（v475 残点）** — 1=旧残点 V-406-3 継続（elem class 裏取り＝`IngredientItem` 断定と `FUN_004D0C20` identity）／2=旧残点 V-406-4 完遂判断（`FUN_004A9700` identity）／3=V-475-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-476-2: `FUN_004A9700`（`form+0x69` byte source）identity** | **[確定]** | L135059: **`undefined1 __fastcall FUN_004a9700(int param_1) { return *(undefined1 *)(param_1 + 0x69); }`（1行 getter）[確定(decompiled)]** → **`FUN_005222D0` tail の `call 0x4A9700`（receiver＝`*(self+0x104)`）は「`*(self+0x104 + 0x69)` byte を読む」操作 → `FUN_00468270` で `self+0x68` subobject の `+0xD`（`*(self+0x75)`）へ転送 [確定相当]** → **`form+0x69` byte 系譜は「`self+0x104`（form ptr）の `+0x69` → `self+0x75`」の byte コピー経路と確定 [確定相当]**（`self+0x104` の指す form の `+0x69` が何の値かは v406-4 残点の semantic として残る） |
| **V-476-1: `FUN_004B2A00`（`IngredientItem [vt+0x170]` impl）の実読** | **[確定]** | body L140907-140954 実読 [確定(decompiled)]: **`bool __fastcall(int param_1)`: `if (param_1==0) return false;` → `switch(*(u8*)(param_1+4))`（FormType switch）: `case 0x12:` `"EditorMarker.NIF"` と `FUN_0046D3F0(param_1)` 名比較→等しければ true／`case 0x18: if (DAT_00B35EBC==param_1) return true;`／`case 0x1a:`→true／**`case 0x1c: if (DAT_00B35EA4/B35EB4/B35EC0/B35EC4/B35EB8/B35EA8/B35ED4/B35EAC/B35EB0 == param_1) return true`**／`case 0x29: return true;` → **末尾 `return *(int*)(param_1+0xc) - 100U < 0x14;`（`+0xC` が **100..119** 値域なら true）** [確定(decompiled)] → **`FUN_004B2A00`＝「FormType switch で singleton form 群（EditorMarker.NIF／STAT 群9個）を判定し、最終 gate で `+0xC` int の 100..119 range を問う bool predicate [確定相当]`** |
| **V-476-1: `DAT_00B35EB8` の位置づけ（v470 訂正・追補）** | **[確定]（群）** | **`case 0x1c`（FormType `0x1C`=TESObjectSTAT）で9個の `DAT_` singleton（`0xB35EA4/EB4/EC0/EC4/EB8/EA8/ED4/EAC/EB0`）と `param_1` を等値比較 [確定(decompiled)] → `DAT_00B35EB8` は「FormType `0x1C` の singleton 群9個の1つ」[確定]**（v470 の「TESObjectSTAT singleton」は正しく、**群に属する**ことを追補）。`0xB35EB8+4=0xB35EBC` も同群（v474 の `mov [0xb35ebc]` 連続書きと整合） |
| **V-476-1: `self[+0xC]` の値域（V-475-1 引継ぎに部分回答）** | **[確定相当]（predicate 内の usage）** | **`return *(int*)(param_1+0xc) - 100U < 0x14` ⇔ `100 <= +0xC <= 119` [確定]** → **`+0xC` は少なくとも本 predicate では「100..119 値域を持つ int（id/種別番号系 [推定]）」[確定相当]**。`FUN_005222D0` の `==7` 比較（v475）と併用 → **field は type 依存（v475 確定）で、form 系では int 数値**。**上位 byte `0xFF`（≥`0xFF000000`＝gate v473）は 100..119 range の外側・負値相当を含む値 [推定]、semantic は [未確定]＝残点** |
| **V-476-1: `virtual+0x170()` の semantic（v474 推定の精緻化）** | **[推定相当]** | L42487/42488（owning `FUN_00438060`）: **`iVar2 = virtual[*param_2 + 0x170](); *(char*)(iVar2 + 4) == '\x1a'` と `*(char*)(iVar2+4) != '\n'` の2回、結果 `iVar2` の `+4` byte が FormType と比較 [確定(decompiled)] → 戻り値の `+4` は FormType byte（v470 `byte[+4]=0x1C` 構造と一致）→ 戻り値＝form 本体 ptr [推定相当]** → **`virtual+0x170()`＝form ptr getter（`GetForm()`/`GetParentForm()` 相当）[推定相当]**（v474 の「form type singleton getter」から「form ptr getter」へ精緻化） |
| **V-476-1: elem class 断定（v475 の `IngredientItem` 推定）** | **[確定負]（撤回）** | **`FUN_004B2A00` は `bool` を返す（`return false/true/...`）が、v474 walk は `call eax` 直後に **`cmp eax, [0xB35EB8]`（ptr 等値比較）** [確定(機械)] → bool（0/1）と ptr の比較は常に不一致 → **`IngredientItem` の `[vt+0x170]=FUN_004B2A00` が elem の slot impl だと walk 条件が常に false になり矛盾 [確定]** → **v475 の「elem＝`IngredientItem`」推定は撤回 [確定負]**。elem の vtable は ptr を返す別 impl（`0xB35EB8` を body に含む必要が無い）→ **v475 census（1486→1）は方向違いだった**（impl 側に singleton 参照がある前提自体が誤り）→ **elem class は [未確定] に復帰** |
| **V-476-1: `FUN_004D0C20`（`A1 mov eax,[0xB35EB8]` owner）** | **[未確定]（残点継続）** | signature **`undefined4 * __thiscall(int, undefined4*, int, int)`（4引数）・large fn [確定(decompiled)]**、`0xB35EB8` load の文脈は body 深部＝本 probe 未到達 |
| **V-476-3: V-475-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v476_1/v476_2）

### §2.1 `FUN_004A9700`（v476_1c）
```
L135059: undefined1 __fastcall FUN_004a9700(int param_1) {
  return *(undefined1 *)(param_1 + 0x69); }
```

### §2.2 `FUN_004B2A00` body（v476_2a）
```
L140909: bool __fastcall FUN_004b2a00(int param_1) {
L140916:   if (param_1 == 0) return false;
L140919:   switch(*(undefined1 *)(param_1 + 4)) {        // FormType switch
L140920:   case 0x12: _Str2="EditorMarker.NIF"; _Str1=FUN_0046d3f0(param_1);
L140924:     if (__stricmp(_Str1,_Str2)==0) return true;
L140928:   case 0x18: if (DAT_00b35ebc == param_1) return true;
L140933:   case 0x1a: ... return true;
L140939:   case 0x1c: if ((DAT_00b35ea4==param_1 || DAT_00b35eb4==param_1 ||
L140941:      DAT_00b35ec0==param_1 || DAT_00b35ec4==param_1 || DAT_00b35eb8==param_1 ||
L140942:      DAT_00b35ea8==param_1 || DAT_00b35ed4==param_1 || DAT_00b35eac==param_1) ||
L140943:      DAT_00b35eb0==param_1) return true;
L140947:   case 0x29: return true;
L140950:   return *(int *)(param_1 + 0xc) - 100U < 0x14; }   // 100..119 range
```

### §2.3 `virtual+0x170` usage（v476_2b）
```
L42487 (FUN_00438060): iVar2 = virtual[*param_2 + 0x170](); *(char*)(iVar2 + 4) == '\x1a'
L42488:                iVar2 = virtual[*param_2 + 0x170](); *(char*)(iVar2 + 4) != '\n'
-> result's +4 byte compared as FormType twice -> result = form object ptr [推定相当]
```

### §2.4 elem 推定撤回（v476_2c）
```
[vt=0xA33EE0 + 0x170] = 0x4B2A00 (FUN_004b2a00, returns bool)
v474 walk: call eax / cmp eax, dword [0xb35eb8]   (ptr compare)
-> bool vs ptr: always false for a nonzero singleton -> contradiction
-> v475 "elem = IngredientItem" [確定負] (retracted)
```

## §3 残点 **V-477-1..3**
1. 【中】**旧残点 V-406-3 継続** — **elem class の再特定**（ptr を返す `[vt+0x170]` impl を探す別法：walk の `cmp` 型整合 impl＝「`[vt+0x170]` が `mov eax, <singleton ptr>` 系ではなく `return this`/`return [this+X]` 形」の vtable 列挙）＋ **`elem[+0x28]` float の語**。
2. 【低】**旧残点 V-406-4 semantic** — `self+0x104` の指す form の `+0x69` byte が何の値か（`FUN_004A9700` identity 確定済み・**残りは `self+0x104` の form 種別特定**）＋ `self[+0xC]` 上位 byte `0xFF` の語（100..119 range 外の何を意味するか）。
3. 【低】**V-476-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）＋ `FUN_004D0C20`（A1 singleton load owner）identity。

## §4 検証・自己批評
- probe: **v476_1..v476_2 の2本全 EXIT=0**（V-476-1 `FUN_004B2A00` 実読＋`0xB35EB8` 群確定＋`+0xC` 値域確定＋elem 推定撤回／V-476-2 `FUN_004A9700`＝1行 getter で identity 確定／V-476-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_004A9700` は decompiled 1行**（getter は即 identity）→ (ii) **`FUN_004B2A00` の docs body 全読**（switch＋末尾 return で value range が判明）→ (iii) **L42487 owning と `+4` byte 比較の再読**で `virtual+0x170` を form ptr getter へ精緻化 → (iv) **bool return ⇔ ptr compare の矛盾検出で v475 推定を撤回**。
- 妥当性チェック: ①`*(param_1+0x69)`⇔`FUN_00468270` の `*(self+0xD)` 転送（receiver `edi+0x68`）⇔`self+0x75` — ②`case 0x1c`⇔`byte[+4]=0x1C`（v470 TESObjectSTAT）— ③`+0xC - 100U < 0x14`⇔`100..119`（unsigned 算術）— ④`+4 == 0x1a / != 0x0a`⇔FormType byte 比較（v470 構造）— ⑤bool return⇔`cmp eax,[ptr]` 不整合＝撤回根拠 — **五方向矛盾なし（⑤は撤回の根拠として整合）**。
- docs との不整合: **①v475 ruling「elem＝`IngredientItem` [推定]」→ 本裁定で [確定負] 撤回（bool/ptr 不整合）／②v475 ruling「`[vt+0x170]` fn が singleton 参照＝GetType getter」→ `FUN_004B2A00` 実読で「FormType switch predicate」に訂正／③v470 ruling「`DAT_00B35EB8`＝TESObjectSTAT singleton」→「FormType `0x1C` singleton 群9個の1つ」へ追補／④v474 ruling「`virtual+0x170()`＝form type singleton getter」→「form ptr getter（`+4`＝FormType）」へ精緻化**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **bool を返す predicate と ptr compare の矛盾は「戻り値の型」で見つかる** — census の predicate（fn body が global を含むか）が**型不整合**を起こした時点で推定は撤回対象（**本裁定は v475 推定の自己訂正**）。
  2. **`switch(*(u8*)(param+4))` ＝ FormType switch の定番** — `param+4`＝FormType byte（v470/v441 で反復）が switch されるだけで class 判定 fn と分かる。
  3. **unsigned 比較（`x - 100U < 0x14`）は value range 判定**（`100..119`）— **int field の値域を predicate が語る**（`+0xC` の性質がこれで確定相当）。
  4. **singleton 群の `DAT_` 連番（`0xB35EA4..ED4`）を1行で拾える**（`case 0x1c` の並び）— **9個 singleton 群が一覧で確定**（v474/v469 の単体記載を群記載に更新）。
  5. **1行 getter（`return *(u8*)(p+0x69)`）は decompiled で即 identity** — 系譜（誰が呼んでどこへ送るか）まで machine tail decode（v475）と合わせると経路が完結する。
  6. **推定は「撤回」も裁定の価値** — `[確定負]` で v475 を明示的に覆し、次 probe の方向（ptr-return impl 探索）を切り替える。
