# v448 — level bonus 連鎖・AV setter family・docs 欠落 fn の裁定（V-448-1..3 完遂・旧残点群移行第2弾）

- 採番: **v448**（旧残点群移行・probe prefix **v448_1..v448_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-448-1..3（v447 の残点）** — 1=Player `+0x170` virtual fn の identity と `TESActorBaseData+4` flags bit7 semantic／2=未注記2 fn の case-chunk formal identity／3=dispatcher `case 0x21+` の全列挙を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-448-1: Player `+0x170` virtual fn の identity** | **[確定]** | **Player ctor `FUN_0066A740` の vtable store 実読**: `66A774: mov [esi], 0xA73A0C` → **RTTI `.?AVPlayerCharacter@@` [確定]**（sub-vtables `+0x18/+0x5c/+0x68` も同 class）。**`[vt+0x170] = 0x4D9B40`** → **`FUN_004D9B40`（L171230）＝`mov eax,[ecx+0x1C]; ret` の1行 getter [確定(機械)]** → **`FUN_004677F0` の level bonus 連鎖が完全確定: `Player->vtable+0x170()`＝`Player[+0x1C]` を返す → その `+0x32 s16` を base level へ加算** |
| **V-448-1: `Player[+0x1C]` とその `+0x32 s16` の語** | **[未確定]（残点）** | ctor 内 `[esi+0x1C]` の直接 store は **0件**（`lea eax,[esp+0x1c]` は stack 別物）→ setter 経由 [確定（否定側）]。`+0x32 s16` の意味も未同定 |
| **V-448-1: `TESActorBaseData+4` flags bit7** | **[部分確定]** | ctor `FUN_00467320`: **`param_1[1] = 0`（`+4` u32 を0初期化）[確定(decompiled)]**、machine `467335: mov [eax+4],ecx`。**bit7 set の `or [reg+4],0x80` byte/dword/SIB 形は .text 全体で0 hits [確定(否定側)]** → **`or` ではなく byte store（`*(u8*)=value`）経由で立つ [推定高]**（`FUN_00468180` family が `*(u8*)(self+4)=v` を実行＝byte AV 値が flags フィールドの下位 byte 系に書き込まれる構造の可能性）。**semantic は [未確定]＝残点** |
| **V-448-2: 未注記2 fn の所属** | **[確定]（v447 訂正）＋[推定高]（identity）** | **`FUN_00511570` 直前の bytes 実読: `... 33 CC E8 7E FC 46 00 8B E5 5D C3` `CC CC CC CC CC CC CC CC`（前 fn の epilogue `xor ecx,esp; call __security_check_cookie; ret` の後に int3×8）→ `0x511570 fldz` が独立 fn entry [確定]** → **v447 の「owning fn 内 sub-case chunk」推定は訂正: annotation 無しの独立 fn ＝ docs annotation 欠落（v437 の docs 欠落類型）[確定]**。machine head 実読（`fldz; [ebp]=0.0`→`virtual [edi]+0x190()`→`call 0x5e02e0`→**`movzx eax,byte[ebx+4]; call 0x447df0`（FormType 引数）**）＝**AV 計算系 fn [推定高]**。`0x523F40` は同検証未実施（残点）。`FUN_00510e60` region の `jmp [reg*4]` table は0件 [確定(否定側)] |
| **V-448-3: dispatcher `case 0x21+` の全列挙** | **[確定(decompiled)＋AV 語実読]** | **`case 0x21: FUN_00468180`／`0x22: FUN_004681b0`／`0x23: FUN_004681e0`／`0x24: FUN_00468210`** → **AV 語（`0xB0A1A8` 実読）: `[33]Aggression/[34]Confidence/[35]Energy/[36]Responsibility` [確定]** ＝ **4つの byte AV の setter family**。**`default: FUN_0065cb00(param_1,(float)param_2); FUN_0046abf0(0x10000000)` ＝ float 変換系 setter＋notify [確定]**（index≥0x28 の float AV 系 [推定高]）。**`case 0x25..0x27` は no-op break ⇔ AV 語 `[37]Bounty/[38]Fame/[39]Infamy` → これらの AV は script SET dispatcher 上 set 不可 [確定]**。AV `[40]MagickaMultiplier/[41]NightEyeBonus/[42]AttackBonus/[43]DefendBonus/[44]CastingPenalty/[45]Blindness` も実読 |
| **V-448-3: `FUN_00468180` の identity** | **[確定(decompiled)]** | L81799: **`void __thiscall FUN_00468180(int *self, undefined1 v)`: `*(u8*)(self+4) = v;` → `(**vtable[*self] + 0x10)()`（virtual notify +0x10・docs は "jumptable recovery failed" 警告付きの tail-call）[確定]** → **byte AV setter（store＋notify）family**（`0x4681B0/81E0/8210` は連番同型 [推定高]） |

## §2 証拠（v448_1..v448_3）

### §2.1 level bonus 連鎖（v448_1a）
```
FUN_0066a740 (Player ctor):
  66A774: mov [esi], 0xA73A0C        -> RTTI .?AVPlayerCharacter@@
  (+0x18 -> 0xA73A04 / +0x5c -> 0xA739BC / +0x68 -> 0xA73988 all PlayerCharacter)
[vt 0xA73A0C + 0x170] = 0x4D9B40
FUN_004D9B40 (L171230): mov eax, [ecx + 0x1c]; ret     <- 1-line getter
=> FUN_004677F0: iVar1 = Player->vtable[0x170]() = *(Player+0x1C)
                 local_4 += s16[iVar1 + 0x32]
```

### §2.2 TESActorBaseData+4（v448_1b / v448_2a / v448_3c）
```
ctor L80994: param_1[1] = 0            ; u32 +4 = 0
machine 467335: mov [eax+4], ecx
or [reg+4], 0x80 patterns in .text: 0 hits   [確定(否定側)]
FUN_00468180 (L81799): *(u8*)(param_1+4) = v ; virtual +0x10 notify
  -> byte store 経由の set が唯一の観測経路 [推定高]
```

### §2.3 FUN_00511570 boundary（v448_3b）
```
bytes 0x511550..0x511570:
  00 00 59 5F 5E 5B 8B 8C 24 B0 04 00 00 33 CC E8 7E FC 46 00 8B E5 5D C3
  CC CC CC CC CC CC CC CC
  <- prev fn epilogue (pop/ xor ecx,esp / call 0x46FC7E / mov esp,ebp / pop / ret)
     + int3 x8 -> 0x511570 = independent fn entry, docs annotation ABSENT
FUN_00510e60 region jmp [reg*4] tables: 0  [確定(否定側)]
```

### §2.4 dispatcher case 0x21+ / AV 語（v448_2c / v448_3a / c）
```
L210416..210432: case 0x21 -> FUN_00468180 / 0x22 -> FUN_004681b0 /
                 0x23 -> FUN_004681e0 / 0x24 -> FUN_00468210 / return;
                 default -> FUN_0065cb00(param_1,(float)param_2);
                            FUN_0046abf0(0x10000000)
AV table (0xB0A1A8): [33]Aggression [34]Confidence [35]Energy [36]Responsibility
                 [37]Bounty [38]Fame [39]Infamy [40]MagickaMultiplier
                 [41]NightEyeBonus [42]AttackBonus [43]DefendBonus ...
(no-op group 0x25..0x27 <=> Bounty/Fame/Infamy)
```

## §3 残点 **V-449-1..3**
1. 【低】**`Player[+0x1C]` の identity**（どの setter が store するか＋`+0x1C` の class）と **`+0x32 s16` の語**（level bonus の実体）。
2. 【低】**`TESActorBaseData+4` bit7 の set 経路実検証**（`*(u8*)=v` 系の byte store 書手 census）と semantic ＋ **`0x523F40` の docs 欠落確認**（`FUN_00511570` 同様の int3 境界検査）。
3. 【低】**docs 欠落 fn `FUN_00511570` の identity**（machine 全文 decode での役割特定）＋ default float AV の index 範囲（`case default` が受ける AV index の集合）。

## §4 検証・自己批評
- probe: **v448_1..v448_3 の3本全 EXIT=0**（V-448-1 は連鎖を確定（`Player[+0x1C]`/bit7 は残点）／V-448-2 は v447 訂正（独立 fn＝docs 欠落）／V-448-3 完遂（case0x21+ 全列挙＋AV 語＋setter family））。
- **決定打の作り**: (i) **vtable は ctor の `mov [reg], imm32` 直後に RTTI**（1手）→ (ii) **slot fn は `[vt+0x170]` を読んで machine head を見るだけ**（`mov eax,[ecx+0x1C]; ret` が即 getter）→ (iii) **bit7 set は「形式別 census（or）→0 hits→byte store へ推論」の否定側アプローチ** → (iv) **fn 境界は int3×8 の bytes 実読で確定**（backward annotation grep は直前 annotation を返すだけなので、**int3 境界と突き合わせないと誤所属**）→ (v) **AV 語は table 実読（v447 と同手法）で case 対応を一括**。
- 妥当性チェック: ①vtable 0xA73A0C⇔`.?AVPlayerCharacter@@`⇔`FUN_0066a740`＝Player ctor（v447 docs 代入行と整合）— ②`[vt+0x170]=0x4D9B40`⇔`+0x1C` getter⇔`+0x32` 加算（docs L81359 と機械一致）— ③`or-0x80`=0hits⇔ctor は u32 0 初期化（bit7 は非 or 系で立つ）— ④`0x511570` 直前 int3×8⇔独立 fn⇔docs annotation 無し— ⑤case0x21..0x24⇔{Aggression,Confidence,Energy,Responsibility}⇔`FUN_00468180` family の byte setter— ⑥case0x25..0x27 no-op⇔{Bounty,Fame,Infamy}（script から set 不可）— **六方向矛盾なし**。
- docs との不整合: **①`FUN_00511570`（と候補 `0x523F40`）は docs annotation 欠落 [確定（v437 類型の再発見）]**／②**v447 の「sub-case chunk」推定を本裁定で自己訂正**（int3 境界で独立 fn と判明）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **virtual slot の identity は「ctor の vtable store→RTTI→`[vt+off]` 読み→1行 getter」の4手で完結**（`Player+0x170` を1 probe で確定）。
  2. **flag set の探索は形式別 census の否定側結果をそのまま推理の材料に**（`or`=0hits → byte store 経由と推論）。
  3. **backward annotation grep は「直前 annotation」を返すだけで、int3 境界で独立 fn なら誤所属** — **必ず int3 境界（`CC×N` の bytes 実読）と突き合わせる**（v447 の sub-chunk 推定はここで訂正）。
  4. **dispatcher の case→AV 語対応は「case 番号＝AV index」と table 実読の組合せで即完結**（v447 の教訓の継続）。
  5. **setter family は連番 fn（`0x468180/B0/E0/8210`）＋同型 decompiled で「byte AV setter 群」と一括同定**（1本読めば family 判定）。
