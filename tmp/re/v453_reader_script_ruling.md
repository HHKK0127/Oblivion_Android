# v453 — mid-layer 分類・`Player[+0x1C]` reader 確定・Script TD の裁定（V-453-1..3 完遂・旧残点群移行第7弾）

- 採番: **v453**（旧残点群移行・probe prefix **v453_1..v453_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-453-1..3（v452 の残点）** — 1=docs 欠落 census 第3系統（`d≤0x200` 層の真偽）／2=`Player[+0x1C]` 書手残ルート（reader 逆引き）／3=subobject 名の別法を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-453-1: mid-layer（`0x40<d≤0x200` ∧ prologue＝1920）の真偽** | **[部分確定]** | **prev-segment 長さで2分類: `prev≥0x100`＝135件（長い fn の直後＝独立 fn 確率高い [推定]）／`prev<0x100`＝1785件**。**samples は small stub 集積帯と判明**: `0x412950/412960/412970/412980`（owning `FUN_00412880` 直後に d=0xD0..0x100 で4連続）・`0x401730/401740`（prev=0x7 の tiny 連続）→ **owning fn 直後に small stub が並ぶ構造（v444 registration stub 同型）＝独立 small fn の可能性 [推定高]**、ただし owning body 内 label の可能性は機械的切り分け不能 [未確定]。**docs 欠落下限は SAFE 2202 に mid 長135 を加えた 2337 レンジ下限 [推定]**（上限は境界法 6038 維持） |
| **V-453-2: `Player[+0x1C]` reader 逆引き** | **[確定]（reader）＋[部分確定]（使用形）** | **`FUN_004D9B40`（getter）の E8 caller は2件とも `FUN_004E82E0`（L182198）[確定]** → **`Player[+0x1C]` を読むのはこの fn のみ [確定]**。context 実読: **`mov ecx,edi; call 0x4D9B40` → `test eax,eax; je skip` → 再 getter → `mov edx,[eax]`（vtable）→ `mov edx,[edx+0x84]`（virtual slot）→ `push 1; push ecx; mov ecx,eax; call edx` ＝ `Player[+0x1C]` オブジェクトの virtual `+0x84(1, param)` call [確定(機械)]**。`FUN_004E82E0` は `void __thiscall(int,int,char)`・body 213 lines・float locals・`ret 8` [確定(構造)]、purpose の keyword 検索は0 hits（docs は `FUN_` 名のみ）→ **purpose [未確定]＝残点** |
| **V-453-3: subobject 名別法（RTTI TD 路線）** | **[部分確定]** | **`0xB03358 = .?AVScript@@` [確定]** — `FUN_0046F020` の `FUN_0046B250(local_4, 0, &TESForm, &Script, 0)` lookup が **form→`Script` オブジェクトを引く [確定相当]** → **subobject `+0xC4` は Script-related 構造 [推定高]**。**`FUN_00469E90` body（L83683-）の `0xB0xxxx` TD 直接参照＝0件 [確定(否定側)]** → 同法では class 不明。**`0xB02F9C=.?AVTESForm@@`／`0xB03054=.?AVTESObjectREFR@@`／`0xB036FC=.?AVTESObjectWEAP@@`／`0xB03778=.?AVTESObjectARMO@@` を再確認（v441-444 と整合）**。残る subobject 名（`+0x44/+0x68/+0xE4/+0x54`）は [未確定]＝残点 |

## §2 証拠（v453_1..v453_3）

### §2.1 mid-layer 分類（v453_1a）
```
mid-layer (0x40<d<=0x200, prologue): 1920
  prev >= 0x100 : 135   (long fn before -> likely independent)
  prev < 0x100  : 1785  (small stub clusters)
samples:
  0x401730 (8B) d=0xA0 prev=0x7  own FUN_00401690
  0x412950 (83) d=0xD0 prev=0xC1 own FUN_00412880
  0x412960 (8B) d=0xE0 prev=0x8  own FUN_00412880   <- 4 consecutive stubs
  0x412970 (0F) d=0xF0 prev=0x7  own FUN_00412880
  0x412980 (8B) d=0x100 prev=0x5 own FUN_00412880
```

### §2.2 reader（v453_1b + v453_2a）
```
FUN_004D9B40 callers: 2, both in FUN_004E82E0 (0x4E8707, 0x4E8712) L182198
context:
  4E8705 mov ecx,edi / 4E8707 call 0x4d9b40 / test eax,eax / je 0x4e8729
  4E8710 mov ecx,edi / 4E8712 call 0x4d9b40
  4E8717 mov ecx,[ebp+0x28] / 4E871A mov edx,[eax] (vtable)
  4E871C mov edx,[edx+0x84] / push 1 / push ecx / mov ecx,eax / call edx
  -> Player[+0x1C] object's virtual +0x84(1, param)   [確定(機械)]
FUN_004E82E0: void __thiscall(int,int,char), body 213 lines, ret 8, float locals
  keyword search in body: 0 hits -> purpose [未確定]
```

### §2.3 RTTI TDs（v453_3a）
```
0xB03358 = .?AVScript@@       (FUN_0046F020's lookup target: form -> Script)
0xB02F9C = .?AVTESForm@@  /  0xB03054 = .?AVTESObjectREFR@@
0xB036FC = .?AVTESObjectWEAP@@  /  0xB03778 = .?AVTESObjectARMO@@
FUN_00469E90 body 0xB0-ptr refs: 0
FUN_004E82E0 body keyword hits: 0 (docs uses FUN_ names only)
```

## §3 残点 **V-454-1..3**
1. 【低】**`Player[+0x1C]` オブジェクトの class**（virtual `+0x84` slot を持つ class＝書手 fn 特定と表裏一体）＋ **`FUN_004E82E0` の purpose**（body 内の call 先群 `FUN_00405680` 等から追跡）。
2. 【低】**subobject 名（`+0x44/+0x68/+0xE4/+0x54`）** — sub-call fn 内の `call` 先 RTTI/`FUN_0046B250` lookup の TD 突合を各 field で継続。
3. 【低】**docs 欠落 census の総まとめ** — `d≤0x200` 層の残り1785 件を「owning body が機械 span をカバーするか」の docs body-line 対 machine 比で検証するか、旧残点（V-415-2 等）へ移行するかの判断。

## §4 検証・自己批評
- probe: **v453_1..v453_3 の3本全 EXIT=0**（V-453-1 は mid-layer 2分類＝部分／V-453-2 reader 確定＋使用形確定／V-453-3 Script TD 確定・残 subobject は残点）。
- **決定打の作り**: (i) **mid-layer は「prev segment 長さ」で独立性を切り分け**（≥0x100＝135／<0x100＝1785）→ (ii) **reader 逆引きは getter の E8 census（2件）＋context decode で virtual slot `+0x84` まで特定** → (iii) **`FUN_0046F020` の lookup 引数 TD を読めば subobject の機能が当たる**（`0xB03358=Script`）。
- 妥当性チェック: ①getter caller=2⇔`FUN_004E82E0` の null check（`test eax,eax; je`）と再 getter 2回 call の整合 — ②`[eax]→[edx+0x84]`⇔vtable virtual call 形 — ③`0xB03358=Script`⇔`FUN_0046B250` lookup が `&TESForm,&Script` の2引数（v413 の lookup 形と一致）— ④mid-layer samples の `FUN_00412880` 4連続⇔prev=0x5..0x8（tiny stub）— **四方向矛盾なし**。
- docs との不整合: **なし**（docs は `FUN_004E82E0` に annotation あり・keyword 検索0 hits は docs 命名の限界＝不整合ではない）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **mid-layer の真偽は「prev segment 長さ」で機械2分類できる** — `prev≥0x100` が SAFE 上乗せ、`prev<0x100` は stub 集積帯として別扱い（**閾値を1つ足すだけで解像度が上がる**）。
  2. **getter の caller census は「件数1〜2」のとき context decode まで一気にやる** — reader の identity（`FUN_004E82E0`）と使用形（virtual `+0x84` call）が同時に取れる。
  3. **lookup 系 fn の第4引数 TD（RTTI name）が subobject の機能を語る** — `&TESForm,&Script`→`Script` のように **RTTI TD 実読は class/機能の素早い手がかり**。
  4. **docs の keyword 検索は `FUN_` 名しかない環境では無力** — purpose 判定は **call 先群の identity 突合**へ移す（次 probe の鍵）。
