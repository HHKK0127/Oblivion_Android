# v506 — node field layout（key→`+4`/value→`+8`）・map fn は direct call・Player `+0x80/+0x14` の裁定（V-506-1..3 完遂・旧残点 V-406-3 継続第36弾）

- 採番: **v506**（旧残点群消化・probe prefix **v506_1 の1本 EXIT=0**（初回構文エラー2箇所→修正再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-506-1..3（v505 残点）** — 1=`FUN_006C4F80` 残り本文（value→field 対応完遂）と map vtable family 確認＋version 閾値表／2=Player `+0x80`/`+0x14` identity と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-506-1: `FUN_006C4F80` 残り本文（value→field 対応完遂）** | **[確定相当]** | machine 0x6C4FA6-0x6C5018 decode [確定(機械)]: **`mov edi,[esp+0x14]`（out node ptr）→ `push ebx; push esi; push eax; mov [edi+4], eax; call 0x984b6a`（**node `+4` ← key（alloc 済み string ptr）、`call 0x984B6A`＝string copy 系 [推定]**）→ `mov eax,[esp+0x28]; add esp,0x10; mov [edi+8], eax`（**node `+8` ← value（param_3 系）**）→ `ret 0xc`（3引数）／else 分岐（`0x6C4FC5`）: `mov eax,[esp+4]; mov ecx,[esp+8]; mov edx,[esp+0xc]; mov [eax+4], ecx; mov [eax+8], edx; ret 0xc`（**node `+4`←param_2、`+8`←param_3 を直接書く**）[確定(構造)]** → **node field layout 確定: `node[+4]`＝key、`node[+8]`＝value（3引数 set・`ret 0xc`）[確定相当]** → **value→field 対応完遂**。**lock 系と接続: `FUN_00496D50(key, owner, count)` → node `+4`←key、node `+8`←value → v496 の `piVar2[2]`（node `+8` u32・bit5＝`0x20`）と整合 → owner/count は node `+8` の u32 内 bit/field [推定相当]** |
| **V-506-1: map vtable family 確認（BST vs LockFree）** | **[確定]（否定側・経路転換）** | **`FUN_00452570`（insert）／`FUN_004524C0`（remove）／`FUN_00412D30`（setting-map insert）の `.rdata`/`.data` dword search → **raw hits 0件（3 fn とも）[確定(否定側)] → 3 fn は direct call（E8）のみで vtable に載らない** → **map family の vtable 逆引きは不可（直接呼び出し構造）** → **BST vs LockFree の判別は「this vtable の入手元（map ctor）」へ経路を転換 [確定（経路転換）]**（v504 BST/LockFree impl 分岐の判別は残点の方向を明記） |
| **V-506-2: Player `+0x80`（ptr）／`+0x14`（type 依存）identity** | **[確定相当]（型のみ）** | docs mentions [確定]: **`+0x80`＝77件、`puVar1 = *(undefined4 **)(DAT_00b33b00 + 0x80)`（**ptr field**・L15441/L15477/L28394/L29045/L50367 等）[確定相当]**／**`+0x14`＝329件、`local_10 = *(int *)(…+0x14)`（L15325）／`local_8 = *(short **)(…+0x14)`（L15331）／`iVar9 = *(int *)(…+0x14)`（L15385）／`uVar8 = *(uint *)(…+0x14)`（L15402）→ int/short*/uint と **type 依存の汎用 field**（LoadGame band では buffer size/ptr）[確定相当]** → **`+0x80`＝ptr、`+0x14`＝汎用（type 依存）と型まで確定・中身の semantic は残点 [部分確定]** |
| **V-506-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v506_1）

### §2.1 `FUN_006C4F80` node field layout（v506_1a）
```
6C4FA6: mov edi, dword ptr [esp + 0x14]        ; out node ptr
6C4FAA: push ebx / push esi / push eax
6C4FAD: mov dword ptr [edi + 4], eax           ; node[+4] = KEY (string ptr)
6C4FB0: call 0x984b6a                          ; string copy [推定]
6C4FB5: mov eax, dword ptr [esp + 0x28]        ; param_3 (value)
6C4FB9: add esp, 0x10
6C4FBC: mov dword ptr [edi + 8], eax           ; node[+8] = VALUE
6C4FC2: ret 0xc                                ; 3-arg callee clean
6C4FC5: mov eax,[esp+4] / mov ecx,[esp+8] / mov edx,[esp+0xc]
6C4FD1: mov dword ptr [eax + 4], ecx           ; node[+4] = param_2
6C4FD4: mov dword ptr [eax + 8], edx           ; node[+8] = param_3
6C4FD7: ret 0xc
=> node layout: [+4]=key, [+8]=value  [確定相当]
=> lock link: FUN_00496D50(key, owner, count) -> node[+8]=value
   v496 piVar2[2] (node[+8] u32, bit5=0x20) consistent  [推定相当]
```

### §2.2 map fn vtable census（v506_1b）
```
insert FUN_00452570 (0x452570): rdata/data raw hits 0
remove FUN_004524c0 (0x4524c0): rdata/data raw hits 0
setting-map insert FUN_00412d30 (0x412d30): rdata/data raw hits 0
=> all three are direct-call only, not vtable-registered  [否定側]
=> BST vs LockFree must be resolved via the map object's ctor  [経路転換]
```

### §2.3 Player `+0x80` / `+0x14`（v506_1c）
```
+0x80 mentions: 77   (puVar1 = *(undefined4 **)(DAT_00b33b00 + 0x80) -> PTR)
  L15441 / L15477 / L28394 / L29045 / L50367 ...
+0x14 mentions: 329  (type-dependent generic field)
  L15325: local_10 = *(int *)(DAT_00b33b00 + 0x14);
  L15331: local_8  = *(short **)(DAT_00b33b00 + 0x14);
  L15385: iVar9    = *(int *)(DAT_00b33b00 + 0x14);
  L15402: uVar8    = *(uint *)(DAT_00b33b00 + 0x14);
=> +0x80 = ptr field; +0x14 = type-dependent generic (buffer size/ptr in LoadGame)
```

## §3 残点 **V-507-1..3**
1. 【中】**map ctor の特定**（this vtable 入手元→BST vs LockFree 判別完遂）＋ **node `+8` u32 の owner/count bit 配列**（v496 `piVar2[2]` bit5 に続く bit 群）＋ **`0x984B6A`（string copy fn）identity**。
2. 【低】**Player `+0x80`（ptr）の中身**（LoadGame の何を指すか）＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v506_1 の1本 EXIT=0**（初回構文エラー2箇所（余分な閉じ括弧）→修正）。V-506-1 node field layout（key→`+4`/value→`+8`）を machine 確定相当で完遂／V-506-1 map family は vtable 逆引き0件の否定側で経路転換／V-506-2 Player `+0x80`＝ptr・`+0x14`＝type 依存を型まで確定相当／V-506-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_006C4F80` の alloc 後（`0x6C4FA6`）を `int3` まで decode** → `[edi+4]`←key／`[edi+8]`←value と else 分岐（直接書く形）で **node layout を両分岐で確定** → (ii) **3 map fn の vtable census（0件）**で direct call 確定＋経路転換を明記 → (iii) **`+0x80`/`+0x14` の docs mentions（77/329件）を type別に数える**と ptr/汎用が判別。
- 妥当性チェック: ①`ret 0xc`⇔3引数（v500/v502 に続く epilogue 法）— ②node `+4`＝key⇔v496 `FUN_004CBA80` の `virtual+0x170()` getter（node key 取得）と整合 — ③node `+8`＝value⇔v496 `piVar2[2]>>5&1`（node `+8` u32 bit5）— ④`0x984B6A`⇔string copy（v505 strlen alloc の後段）— ⑤`+0x14` type 依存⇔v478 `param_1[3]`（`+0xC`）の type 依存性と同型の field 性格 — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **node layout は「alloc 後の store（`[edi+4]`/`[edi+8]`）」で確定** — **else 分岐（直接書く形）も拾うと両 path で layout が一致し確実**（**set fn の両分岐を読む**）。
  2. **vtable 逆引き0件（3 fn とも）は「direct call 専用 fn」の証拠** → **判別経路を ctor（this vtable 入手元）へ転換**（**v479/v488/v503 の vtable 法が効かない case の対処法**）。
  3. **field の型は mentions を type 別に数える**（`*(int*)`/`*(short**)`/`*(uint*)` の混在＝type 依存汎用 field、`*(undefined4**)` のみ＝ptr）——**77/329件の census が型判定を支える**。
  4. **`ret 0xc`（3引数）が set fn の契約**（v500 release／v502 set に続く **epilogue 法の3度目**）。
  5. **value→field 完遂で lock（owner/count）と map（key/value）が同じ node layout に接続**（**v496 `piVar2[2]` bit5 が node `+8` と一致＝横断整合**）。
