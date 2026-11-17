# v502 — bit24 対象＝TESObjectREFR cast 確定・`FUN_004211E0` write 否定側・set path の裁定（V-502-1..3 完遂・旧残点 V-406-3 継続第32弾）

- 採番: **v502**（旧残点群消化・probe prefix **v502_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-502-1..3（v501 残点）** — 1=`PTR_PTR_00b03054` type identity と `FUN_004211E0` machine の esi 出所／2=`FUN_00496D50` set path（value→node field 対応）と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---| 
| **V-502-1: `PTR_PTR_00b03054` type identity（bit24 対象の完全確定）** | **[確定]（RTTI 実読）** | **`.rdata` TD 実読 [確定(実RTTI)]: `0xB03054` の TD name＝**`.?AVTESObjectREFR@@`**（ctx hex `2E 3F 41 56 54 45 53 4F 62 6A 65 63 74 52 45 46`）／`0xB02F9C` の TD name＝**`.?AVTESForm@@`**（ctx `54 45 53 46 6F 72 6D 40 40`）→ **`FUN_0046B250(src, 0, &TESForm, &TESObjectREFR, 0)`＝`TESForm`→`TESObjectREFR` の `__RTDynamicCast` [確定]** → **bit24 対象（`iVar7`）＝Havok data map 要素を `TESObjectREFR` に cast した object [確定相当]**（cast 成功時）→ **bit24 semantic：「Havok map 内の該当 REFR form が処理済み flag [推定相当]」へ確定（`0xB03054` 残点解消）**。docs mentions 204件（`PTR_PTR_00b03054`＝TESObjectREFR type は頻用 [確定]） |
| **V-502-1: `FUN_004211E0` の `[esi+0x18]` write（writer 経路転換）** | **[確定]（Player write 否定側）** | machine 0x421240-0x421280 decode [確定(機械)]: **`0x421245: mov eax,[0xb33b00]`（Player ptr load）→ `0x42124A: cmp byte [eax+0x7c], 0x24; jb 0x42129e`（**Player `+0x7C` byte が `0x24`（36）以上かの判定＝新事実**）→ `0x421250: cmp dword [esi+0x18], 0; je`（esi の `+0x18` が0 か）→ sink（`0xb34d90`）の `virtual+0x18` log call（`push 0xa35010` 文字列）→ **`0x421268: mov ecx,[esp+0x20]` → `0x42126C: mov [esi+0x18], ecx`（stack 引数（param_2）を `esi+0x18` へ）→ `ret 4`（1引数 callee clean）** → **`esi`＝`FUN_004211E0` の this（param_1 object）、`[esi+0x18]` write は「param_1 の `+0x18` field ← param_2」[確定相当]。Player は `+0x7C` byte 判定に使われるだけで Player 自体へ `+0x18` を書いていない → この候補は Player writer でない [確定]** → **v499「122候補」の否定側をさらに裏付け（writer 経路転換の到達点）** |
| **V-502-2: `FUN_00496D50` set path（value→node field 対応）** | **[部分確定]（slot 判明・impl は残点）** | machine 0x496DC1-0x496DE4 decode [確定(機械)]: **`mov eax,[esi]; mov edx,[eax+0x10]; push edi; mov ecx,esi; call edx`（**`virtual+0x10(node)`＝既存 node op**）→ **`mov ecx,[esp+0x1c]`（param_3＝value2）→ `mov edx,[esp+0x18]`（param_2 系）→ `push ecx; push edx; push ebx（key）; push edi（node）; mov ecx,esi; call eax`（**`virtual+0xC(node, key, …)`＝node set**）→ `pop edi/esi/ebp/ebx; ret 0xc`（**3引数 callee clean**）** → **set は `virtual+0xC`（v485/v493 の node set slot と同 slot）で実行 [確定相当]**、**value→owner/count field の対応は `virtual+0xC` impl 側（this vtable の `+0xC` fn）＝残点 [部分確定]**（`ret 0xc` が3引数契約を裏付け [確定]） |
| **V-502-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v502_1）

### §2.1 RTTI TD 実読（v502_1a）
```
0xB03054 (cast dst): first dword = 0x00AA3E80
  TD name = ".?AVTESObjectREFR@@"
  ctx: 80 3E AA 00 00 00 00 00 2E 3F 41 56 54 45 53 4F 62 6A 65 63 74 52 45 46
0xB02F9C (cast src): first dword = 0x00AA3E80
  TD name = ".?AVTESForm@@"
  ctx: ... 54 45 53 46 6F 72 6D 40 40
docs PTR_PTR_00b03054 mentions: 204 (L18946/L18984/L27202... FUN_009832e6 / FUN_0046b250 casts)
=> FUN_0046B250(x, 0, &TESForm, &TESObjectREFR, 0) = TESForm -> TESObjectREFR cast  [確定]
=> bit24 target = Havok map element cast to TESObjectREFR  [確定相当]
```

### §2.2 `FUN_004211E0` machine（v502_1b）
```
421245: mov eax, dword ptr [0xb33b00]        ; Player ptr
42124A: cmp byte ptr [eax + 0x7c], 0x24 / jb 0x42129e   ; Player +0x7C byte >= 0x24?
421250: cmp dword ptr [esi + 0x18], 0 / je 0x421283      ; esi+0x18 == 0?
421256: mov ecx,[0xb34d90] / mov edx,[ecx] / mov eax,[edx+0x18]
421261: push 0xa35010 / call eax             ; sink log call
421268: mov ecx, dword ptr [esp + 0x20]      ; param_2 (stack)
42126C: mov dword ptr [esi + 0x18], ecx      ; WRITE -> param_1's +0x18
421280: ret 4                                ; 1-arg callee clean
=> esi = this (param_1); write is NOT to the Player  [確定]
   Player used only for +0x7C byte >= 0x24 gate  [new fact]
```

### §2.3 `FUN_00496D50` set path（v502_1c）
```
496DC1: mov eax,[esi] / mov edx,[eax+0x10] / push edi / mov ecx,esi / call edx
        -> virtual+0x10(node)  (existing-node op)
496DCB: mov ecx,[esp+0x1c]     ; param_3 (value2)
496DCF: mov edx,[esp+0x18]     ; param_2
496DD3: mov eax,[esi] / mov eax,[eax+0xc]
496DD8: push ecx / push edx / push ebx(key) / push edi(node) / mov ecx,esi / call eax
        -> virtual+0xC(node, key, ...)  node set
496DE0: pop edi/esi/ebp/ebx / ret 0xc         ; 3-arg callee clean
=> set slot = +0xC (same as v485/v493 node set); value->field mapping in impl  [部分確定]
```

## §3 残点 **V-503-1..3**
1. 【中】**`virtual+0xC` impl（node set）の本文**（value2/value3 が node のどの field へ＝owner/count 対応の完遂）＋ **Player `+0x7C` byte（`>=0x24` 判定）の semantic**（新事実）。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ `FUN_0041E210(0x4b)`（FormType 判定 [推定]）identity。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v502_1 の1本 EXIT=0**（V-502-1 `0xB03054`＝TESObjectREFR を RTTI 実読で確定・`FUN_004211E0` write の Player 否定側を machine 確定／V-502-2 set path を slot（`+0xC`）まで確定・field 対応は impl 残点／V-502-3 引継ぎ）。
- **決定打の作り**: (i) **`PTR_PTR_xxx` の `.rdata` TD 実読（`+8` name）で type 名を直接獲得**（`0xB03054`＝`.?AVTESObjectREFR@@`、`0xB02F9C`＝`.?AVTESForm@@`）→ cast を完全確定 → (ii) **`FUN_004211E0` machine を write 帯まで decode** → `mov [esi+0x18], [esp+0x20]` と `ret 4` で「param_1 へ param_2」が判明し、**Player は `+0x7C` 判定のみ**と分離 → (iii) **set path を `ret 0xc` まで decode** → `virtual+0xC` set と3引数契約を確定。
- 妥当性チェック: ①`0xB03054` TD⇔docs `FUN_0046b250(..., &PTR_PTR_00b02f9c, &PTR_PTR_00b03054, 0)`（src=TESForm, dst=TESObjectREFR）— ②`ret 4`⇔1引数（param_2）callee clean（`[esp+0x20]` 読みと整合）— ③`ret 0xc`⇔3引数 set（`push ecx/edx/ebx/edi` 後の `virtual+0xC`）— ④`cmp byte [eax+0x7c],0x24`⇔Player の field 読み（write でない）— ⑤`virtual+0xC`⇔v485/v493 node set slot（同型）— **五方向矛盾なし**。
- docs との不整合: **なし**（RTTI 名・machine とも docs cast 引数と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`PTR_PTR_xxx` 型 symbol は `.rdata` の TD（`+8` name）を直接読めば type 名が取れる** — **RTTI TD 実読が cast 引数の identity を確定**（`.?AV…@@` 形式で class 名が即分かる）。
  2. **`cmp byte [Player+0x7C], 0x24` のような「Player を gate に使うだけ」の pattern を見分ける** — **load して compare するだけなら write でない**（**load→write 絞り込み（v500）の補完として「load→compare」も分類に加える**）。
  3. **`ret N`（callee clean）が引数契約を語る** — `ret 4`＝1引数／`ret 0xc`＝3引数（**machine の epilogue が docs signature の裏付け**＝v500 に続く）。
  4. **set path の `virtual+0xC` が v485/v493 の node set と同 slot** — **map/lock 系の slot family が揃いつつある**（`+0x4` hash／`+0x8` compare／`+0xC` set／`+0x10` existing op／`+0x14` alloc）。
  5. **新事実（Player `+0x7C` byte `>=0x24` gate）も裁定に明記**（**別 residual の種として記録**＝v503 で消費可）。
