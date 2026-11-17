# v501 — `param_1+0x60` list＝Havok data map・sink `+0x18` log call・`FUN_00496D50` 引数対応の裁定（V-501-1..3 完遂・旧残点 V-406-3 継続第31弾）

- 採番: **v501**（旧残点群消化・probe prefix **v501_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-501-1..3（v500 残点）** — 1=Player `+0x18` writer 経路転換と `param_1+0x60` list 型（RTDynamicCast 先 type）→ bit24 semantic 完遂／2=`FUN_00496D50` value 引数の意味と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-501-1: `param_1+0x60` list の型（bit24 semantic 完遂への鍵）** | **[確定相当]（Havok data map）** | **`FUN_0045D030` 内 `L73494-73495` 実読 [確定(実文字列)]: `(**(code **)(*DAT_00b34d90 + 0x18))("LoadHavokData() call finished, but still has elements in the map.");` → **`param_1+0x60` list＝`LoadHavokData()` の内部 map（Havok data 読み込み用）[確定相当]**。**さらに `L73501: uVar6 = FUN_0046b250(local_10, 0, &PTR_PTR_00b02f9c, &PTR_PTR_00b03054, 0);` [確定(decompiled)] → `FUN_0046B250`＝`__RTDynamicCast` wrapper（引数: src, 0, type1=`PTR_PTR_00b02f9c`, type2=`PTR_PTR_00b03054`, 0）[確定相当]**、`iVar7 = FUN_009832e6(uVar6)`（v500）は第2段 cast → **bit24 対象（`FUN_00452C20(iVar7, 0x1000000)` の `iVar7`）＝Havok data map 要素を TESForm 系（`0xB02F9C`）＋`0xB03054` type へ cast した object [確定相当]**（`PTR_PTR_00b02f9c`＝TESForm（v478 で `NAME` cast と同一））。→ **bit24 semantic：「Havok data map の該当要素（form）が bit24＝読み込み済み/処理済み flag [推定相当]」**（完全確定には `0xB03054` type identity 残り＝[部分確定]） |
| **V-501-1: sink `DAT_00b34d90` identity（副次）** | **[確定相当]** | **`*DAT_00b34d90` の `virtual+0x18` が log 文字列出力に使われる [確定(構造)] → `DAT_00b34d90`＝log sink（v484 logging core の出力先 [推定]）を `+0x18` virtual call で確定相当**（v484「sink 正体は残点」への部分回答：**sink は log 出力 vtable object**）。`0x18` slot＝`Print/Log` 系 [推定] |
| **V-501-2: `FUN_00496D50` の value 引数対応** | **[部分確定]（key 側確定・value set path は cap 外）** | machine 0x496D50-0x496D8A decode [確定(機械)]: **`mov ebx,[esp+0x8]`（**param_1＝key＝stack 引数1**）→ `mov esi,ecx`（this）→ `virtual[*esi+4](key)`（hash）→ `mov eax,[esi+8]`（**bucket base**）→ `mov edi,[eax+ebp*4]`（bucket先頭）→ loop: `mov eax,[edi+4]`（node key）→ `virtual[*esi+8](key, node_key)`（**キー比較**）→ hit なら `jne 0x496dc1`（**set path＝cap 外**）／miss なら `mov edi,[edi]`（next）→ 末尾 `virtual[*esi+0x14]`（**node alloc**）** → **`param_2`（key）＝stack `[esp+8]` と machine 確定、`param_3`（value）は set path（`0x496DC1` 以降）で node へ書く [部分確定]（value→owner/count field の対応は cap 外＝残点）** |
| **V-501-1: Player `+0x18` writer 経路転換（`FUN_004211E0` 調査）** | **[部分確定]** | `FUN_004211E0` docs head 実読 [確定(decompiled)]: **`void __thiscall FUN_004211e0(int param_1, undefined4 param_2)`: `iVar2 = FUN_0041e210(0x4b)`（**`0x4B`＝75＝FormType 判定 [推定]**）→ `local_10 = param_1` → `if (iVar2==0) { local_10 = FUN_00401f00(0x1c, uVar1); … }`（**alloc `0x1C`＝28B**）** → **`[esi+0x18]` write の `esi` は本 fn の alloc/init 段では未特定（machine の `0x42126C` 帯で esi 出所を追う必要）[部分確定]**。`FUN_0041E210(0x4b)` の `0x4B` は FormType 値候補 [推定] |
| **V-501-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味（sink 正体は本裁定で相当） |

## §2 証拠（v501_1）

### §2.1 Havok data map + cast（v501_1a）
```
L73493: if (*(int *)(*(int *)(param_1 + 0x60) + 0xc) != 0) {
L73494:   (**(code **)(*DAT_00b34d90 + 0x18))
L73495:     ("LoadHavokData() call finished, but still has elements in the map.");
L73499: FUN_00452600(&local_c, &local_10, &local_14);
L73501: uVar6 = FUN_0046b250(local_10, 0, &PTR_PTR_00b02f9c, &PTR_PTR_00b03054, 0);
L73502: iVar7 = FUN_009832e6(uVar6);
L73504: if (iVar7 == 0) { ... }
=> param_1+0x60 = LoadHavokData() internal map  [確定相当]
=> iVar7 = double-cast object (TESForm 0xB02F9C + 0xB03054 type)  [確定相当]
=> FUN_00452C20(iVar7, 0x1000000) = bit24 on that form  [確定相当]
sink DAT_00b34d90 virtual+0x18 = log string output  [確定相当]
```

### §2.2 `FUN_00496D50` machine（v501_1b）
```
496D50: push ebx / mov ebx, [esp+8]        ; param_1 = KEY (stack arg)
496D57: mov esi, ecx                        ; this
496D59: mov eax,[esi] / mov edx,[eax+4] / push ebx / call edx   ; hash(key)
496D64: mov eax,[esi+8] / mov edi,[eax+ebp*4]                   ; bucket
496D6A: test edi,edi / je 0x496d88
496D70: mov eax,[edi+4] / mov edx,[esi] / mov edx,[edx+8]
496D78: push eax / push ebx / mov ecx,esi / call edx            ; key compare
496D7E: test al,al / jne 0x496dc1          ; HIT -> set path (beyond cap)
496D82: mov edi,[edi] / test edi,edi / jne 0x496d70             ; next
496D88: mov edx,[esi] / mov eax,[edx+0x14]                      ; node alloc (miss)
=> key via [esp+8] confirmed; value set path 0x496DC1 unread  [部分確定]
```

### §2.3 `FUN_004211E0` head（v501_1c）
```
L23890: void __thiscall FUN_004211e0(int param_1, undefined4 param_2) {
L23907:   iVar2 = FUN_0041e210(0x4b);          // 0x4B = FormType? [推定]
L23908:   local_10 = param_1;
L23909:   if (iVar2 == 0) { local_10 = FUN_00401f00(0x1c, uVar1); ... }  // alloc 0x1C=28B
  -> esi identity for the [esi+0x18] write still unread in machine  [部分確定]
```

## §3 残点 **V-502-1..3**
1. 【中】**`PTR_PTR_00b03054` type identity**（bit24 対象の完全確定）＋ **`FUN_004211E0` machine の `0x42126C` 帯 esi 出所**（Player `+0x18` writer 経路経路転換の継続）。
2. 【低】**`FUN_00496D50` set path（`0x496DC1` 以降）**＝value→owner/count field 対応の machine 確定＋gated mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v501_1 の1本 EXIT=0**（V-501-1 `param_1+0x60`＝Havok data map を実文字列で確定相当・sink `+0x18` log call を確定相当・cast 経路を2段で確定相当／V-501-2 `FUN_00496D50` key 側を machine 確定・value は cap 外で部分確定／V-501-1 writer 経路は部分確定）。
- **決定打の作り**: (i) **`FUN_0045D030` の L73475-73506 を全読** → `"LoadHavokData()…map"` 実文字列が list 型を一撃 → **`FUN_0046B250(…, &PTR_PTR_00b02f9c, &PTR_PTR_00b03054, 0)` の cast 引数で type ptr を獲得** → (ii) **`FUN_00496D50` machine decode** で key＝`[esp+8]` と bucket walk を確定、set path の残りを cap 外と明記 → (iii) **`FUN_004211E0` docs head** で `0x4B`（FormType [推定]）と alloc `0x1C` を拾い、esi 特定は machine へ残す。
- 妥当性チェック: ①`DAT_00b34d90`＝sink⇔v484 logging core の出力先（`virtual+0x18`＝log 呼び出し）— ②`PTR_PTR_00b02f9c`⇔v478 `NAME` cast の TESForm type（同物）— ③`[esp+8]`（key）⇔v500 acquire/release の stack 引数方式と同型 — ④bucket `[esi+8]`⇔v485/v494 `param_1[2]` bucket（`esi+8`＝`[2]`×4）— ⑤`0x4B`＝75⇔FormType 値域（`FUN_0041E210` の type 判定 [推定]）— **五方向矛盾なし**。
- docs との不整合: **なし**（実文字列・machine とも docs と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **map/list の型は「残存要素の warning 文字列」で決まる** — `"LoadHavokData()…still has elements in the map"` の1行が `param_1+0x60` の identity を確定（**error/warning 文言法の map 版＝v456/v484/v487 に続く4度目**）。
  2. **cast wrapper の引数（`&PTR_PTR_00b02f9c, &PTR_PTR_00b03054`）が type ptr を語る** — **`FUN_009832E6`/`FUN_0046B250` はどちらも `__RTDynamicCast` 系**（2段 cast の構造を docs で追える）。
  3. **sink の identity は「その sink を使う log 呼び出し」で決まる** — `*DAT_00b34d90` の `virtual+0x18`（文字列出力）で **v484 残点（sink 正体）に部分回答**。
  4. **set path が cap 外なら「key 側確定・value は残点」と分けて記録**（probe1本の収穫を最大化しつつ残点の形を守る）。
  5. **`0x4B`（FormType 75）や alloc `0x1C`（28B）の定数が「type 判定＋object size」の情報源**（v441/v469 に続く定数読みの反復）。
