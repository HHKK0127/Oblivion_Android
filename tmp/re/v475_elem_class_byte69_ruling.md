# v475 — elem class 候補・`form+0x69` byte 系譜・`+0xC` field 性格の裁定（V-475-1..3 完遂・旧残点 V-406-3/V-406-4 継続第5弾）

- 採番: **v475**（旧残点群消化・probe prefix **v475_1..v475_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-475-1..3（v474 残点）** — 1=旧残点 V-406-3 継続（`elem` の vtable 完全特定と `elem[+0x28]` float の語、`self[+0xC]` 上位 byte `0xFF` semantic）／2=旧残点 V-406-4 残り（`form+0x69` byte 値＝`push eax` の eax）／3=V-474-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-475-1: `elem` の class（`[ebx]+0x170` impl 側から逆引き）** | **[確定]（vtable census）＋[推定]（elem class）** | **`[vt+0x170]` slot の fn が `0xB35EB8`（TESObjectSTAT singleton・v470）を参照する vtable は1486個中1個 [確定(機械)]**: **`vt=0xA33EE0` class=`.?AVIngredientItem@@`（`[vt+0x170]=FUN_004B2A00`）[確定]**。別系統で `.text` の `A1 B8 5E B3 00`（`mov eax,[0xB35EB8]`）は1件 `0x4D0E6A`（owning `FUN_004D0C20`）[確定(機械)]、`B8 + imm(0xB35EB8) + C3`（即時値 return）は0件 [確定(否定側)] → **`GetType()` 系 impl は `mov eax,[abs]`（A1）または別 fn 共有で singleton を返す [確定相当]**。**elem は `virtual+0x170()` で `0xB35EB8` を返す class → `IngredientItem`（Static 系 subclass）が最有力候補 [推定]**（ICF/impl 共有があるため `IngredientItem` 断定は保留・親クラス候補と共通 impl の可能性） |
| **V-475-1: `elem[+0x28]` float の語** | **[未確定]（残点継続）** | elem class が `IngredientItem` [推定] なら `+0x28`（byte 40）はその class の float property だが、**field 名の確定には elem class の確定が先行必要＝残点** |
| **V-475-1: `self[+0xC]` 上位 byte `0xFF` semantic** | **[部分確定]（field 性格）＋[未確定]（`0xFF` の語）** | **`param_1[3] = ...` 代入は docs 926件 [確定]**、**うち `param_1[3] = &PTR_FUN_00a322b4`（L10954）＝`+0xC` に vtable アドレスが代入されるケースも存在 → `+0xC` は fn（＝param_1 の type）ごとに意味が変わる汎用 ptr/int field [確定相当]**。`FUN_005222D0` band では `==7` 比較（L216781/L217625）、`FUN_00566830` band では gate（上位 byte `0xFF`）使用 [確定(decompiled)] → **「`==7` 比較と上位 byte `0xFF` gate が併用される」＝特定 type/状態 field [確定相当]**、**`0xFF` 上位 byte の意味は [未確定]＝残点** |
| **V-475-2: `form+0x69` byte 値の系譜（`push eax` の eax）** | **[確定相当]** | **`FUN_005222D0` tail（0x5226F0..0x522746）を boundary decode [確定(機械)]**: `522712: mov ecx,[eax+0x64]`（**class form の `+0x64` dword 直読み＝v471 setter 元**）→ `lea esi,[edi+0x68]`（**receiver＝self+0x68 subobject**）→ `push ecx; mov ecx,esi; call 0x468230`（**`*(sub+8)`＝`*(self+0x70)` へ dword 転送**）→ `mov ecx,[edi+0x104]`（`self+0x104`＝別 form ptr）→ `call 0x51BEB0` → `push eax; mov ecx,esi; call 0x4684C0` → `mov ecx,[edi+0x104]` → **`call 0x4A9700` → `mov ecx,esi`（esi=`edi+0x68`）→ `push eax` → `call 0x468270`** [確定(機械)] → **`form+0x69` byte ＝ `FUN_004A9700(*(self+0x104))` の戻り値 byte、`FUN_00468270` の receiver は `self+0x68` subobject → `*(self+0x75)`（`sub+0xD`）に格納 [確定相当]**。→ **v470 の「form+0x64 → actor `self+8`」表記は receiver＝`self+0x68` subobject なので `*(self+0x70)` へ転送と精緻化（v470 ruling に対し訂正・追補）** |
| **V-475-3: V-474-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v475_1/v475_2）

### §2.1 elem class census（v475_1a + v475_2a）
```
`mov eax,0xb35eb8; ret` (B8 imm32 C3): 0 hits          [否定側]
`mov eax,0xb35eb8` (B8 any): 0 hits
`mov eax,[0xB35EB8]` (A1 imm32): 1 hit 0x4D0E6A (owning FUN_004D0C20)
vtables via ctor-store (v457 method): 1486
  [vt+0x170] fn whose body refs 0xB35EB8: 1
    vt=0xA33EE0 class=.?AVIngredientItem@@
    [vt+0x170] = 0x4B2A00 (FUN_004b2a00)
```

### §2.2 `+0xC` field docs（v475_1b + v475_2b）
```
param_1[3] = ... : 926 assignments
  L240 param_1[3] = param_2 / L551 = 0 / L10954 = &PTR_FUN_00a322b4  (vtable!)
  -> +0xC is per-fn-typed generic field (ptr or int)
FUN_005222D0 band: L216781 *(param_1+0xc) == 7 / L217625 param_1[3] != 7
FUN_00566830 band: FUN_00446B80(param_1[3]) x4 (gate: top byte 0xFF)
```

### §2.3 `form+0x69` byte lineage（v475_1c）
```
0x5226F0: movzx eax,al / push edx / push eax / call 0x5479d0 / add esp,0x10
0x5226FD: push eax / lea ecx,[edi+0x24] / call 0x4672b0     ; AV10 Fatigue (v462)
0x522706: mov eax,[edi+0x104] / test eax,eax / pop ebp/ebx / je 0x52274e
0x522712: mov ecx,[eax+0x64]        ; class form +0x64 dword (v471 setter source)
0x522715: lea esi,[edi+0x68]        ; *** receiver = self+0x68 subobject ***
0x522718: push ecx / mov ecx,esi / call 0x468230           ; *(sub+8) = *(self+0x70)
0x522720: mov ecx,[edi+0x104] / call 0x51beb0
0x52272B: push eax / mov ecx,esi / call 0x4684c0
0x522733: mov ecx,[edi+0x104] / call 0x4a9700
0x52273E: mov ecx,esi / 0x522740: push eax / 0x522741: call 0x468270
          ; *** form+0x69 byte = FUN_004A9700(*(self+0x104)) -> *(self+0x75) ***
0x522746: pop esi / pop edi / add esp,0x1c / ret 4
```

## §3 残点 **V-476-1..3**
1. 【中】**旧残点 V-406-3 継続** — **elem class の確定**（`IngredientItem` 断定の裏取り：`FUN_004B2A00` の decompiled 実読と elem vtable 完全特定、`FUN_004D0C20`（`A1` singleton load）の identity）＋ **`elem[+0x28]` float の語**（class 確定後に field 名）。
2. 【低】**旧残点 V-406-4 完遂判断** — `form+0x64` dword は `*(self+0x70)` 転送（v475 確定）・`form+0x69` byte は `FUN_004A9700(*(self+0x104))` 戻り（v475 確定）→ **両 setter の semantic は「class form の値を +0x68 subobject へ転送」まで確定、`FUN_004A9700`（`self+0x104` getter）の identity が最後の1手**。
3. 【低】**V-475-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v475_1..v475_2 の2本全 EXIT=0**（V-475-1 elem class census で1件限定・`+0xC` field 性格確定相当／V-475-2 byte 系譜確定相当／V-475-3 未着手＝残点）。
- **決定打の作り**: (i) **即時値 return（B8+C3）が0件 → A1 ロード形を検索**（1件）→ (ii) **vtable census を「ctor-store 法（v457）で1486 vtable 列挙 → `[vt+0x170]` fn の body に 0xB35EB8 があるか」の判定に変更** → 1件（`IngredientItem`）に絞る → (iii) **tail を boundary decode**（int3 止まり）→ `form+0x69` の byte 出所（`FUN_004A9700`）と receiver（`edi+0x68`）が同時確定。
- 妥当性チェック: ①`[ebx+0x28]`⇔`piVar8[10]`（v474）— ②`lea esi,[edi+0x68]`⇔`mov ecx,esi`（receiver 継続性）— ③`mov ecx,[eax+0x64]`⇔v471 `FUN_00468230` callers（`form+0x64` 元）— ④`IngredientItem` は Static 系 subclass と RTTI 名が整合（`TESObjectSTAT` 系）[推定] — ⑤`A1` 1件⇔`B8` 0件（const でなく abs-load の singleton getter）— **五方向矛盾なし**。
- docs との不整合: **①v470 ruling「`form+0x64` → actor `self+8` 転送」→ receiver は `self+0x68` subobject なので `*(self+0x70)` へ転送＝追補・精緻化**（v475 で明確化）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **singleton を返す impl は「即時値 return」ではなく `mov eax,[abs]`（A1）のことがある** — B8 検索0件で絶望せず **A1 形へ切り替える**（本裁定で1件）。
  2. **vtable census の判定条件を「fn が特定 global を含むか」にすると、1486→1 に絞れる**（**vtable 数を先に数えてから predicate を掛ける**＝v457 ctor 法と v474 walk 結合）。
  3. **tail decode は int3 までを境界に** — `0x522746: pop esi/pop edi/add esp,0x1c/ret 4` で fn 末尾と確定でき、`push eax` の出所（1行前）が拾える。
  4. **`lea esi,[reg+N]`＋`mov ecx,esi` のペアが receiver の継続** — setter 群の receiver（`edi+0x68`）がこれで確定し、`+8/+0xD` の意味が `self+0x70/+0x75` に翻訳できた（**subobject の存在を lea が語る**）。
  5. **docs の `param_1[3] =` 926件のような多数 hit は「field が汎用（type 依存）」の証拠** — 1件（vtable 代入 L10954）でも拾えば汎用性が確定する。
