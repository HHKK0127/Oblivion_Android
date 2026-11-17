# v481 — `FUN_005E3320`＝linked `+0x104` getter・REFR `+0x6C` impl・`+0x28` 数値 field の裁定（V-481-1..3 完遂・旧残点 V-406-3/V-406-4 継続第11弾）

- 採番: **v481**（旧残点群消化・probe prefix **v481_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-481-1..3（v480 残点）** — 1=旧残点 V-406-3 継続（`[ebx+0x28]` float の語と walk `+0x6C` slot impl）／2=旧残点 V-406-4 semantic 完遂（`DAT_00B37D00` 正体と donor 入手元）／3=V-480-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-481-2: `FUN_005E3320`（`DAT_00B37D00` 比較の入口）の完全 body** | **[確定相当]** | L347984-347998 実読（5行完結）[確定(decompiled)]: **`undefined4 __fastcall FUN_005e3320(int *param_1) { cVar1 = FUN_004d7f80(); if (cVar1 != 0) { iVar2 = virtual[*param_1 + 0x170](); if (iVar2 != 0) return *(undefined4*)(iVar2 + 0x104); } return 0; }`** → **`FUN_005E3320(param)`＝`*(*(param+0x1C) + 0x104)` を返す getter [確定相当]**（`virtual+0x170`＝`*(this+0x1C)`・v480 確定）→ **`DAT_00B37D00` 比較（L329513 等）は `*(*(linked+0x104))+0xc` の ptr 等値 [確定(構造)]** → **v479 の donor 構造（`*(*(param_1+0x104)+0xc)`＝`TESObjectCELL+0x104` の `+0xc`）と同一レイヤ [確定]** → **`DAT_00B37D00` は「`+0x104`（class/related ptr）の `+0xc` field が指す識別 singleton」[推定相当→強化]**（正体は [未確定]＝残点） |
| **V-481-1: walk の elem 側 `+0x6C` impl** | **[確定]** | **`[TESObjectREFR vt=0xA46C44 + 0x6C] = 0x4DF820` → `FUN_004DF820`（owning L175824）[確定(機械)]**、machine head **`push ebp; mov ebp,esp; and esp,0xffffffc0; sub esp,0x30; push ebx/ebp/esi; mov esi,ecx`（16-byte aligned stack・large fn）[確定(機械)]** → **walk（`FUN_004CD960`）が flag bit3 未立の elem に呼ぶ `virtual[elem]+0x6C` ＝ `FUN_004DF820`（`TESObjectREFR` の method）[確定]**（CELL の `+0x6C`＝walk 自身（v479）と**別 fn であることが並立で確定**＝self と elem の slot が同名でも別 impl）→ `FUN_004DF820` の本文 identity は large fn で本 probe 未読＝残点 |
| **V-481-1: `elem[+0x28]`（`TESObjectREFR+0x28`）float の語** | **[推定相当]** | REF band（0x4D9000..0x4DA500・25 fn）の `+0x28` usage 8件実読 [確定(decompiled)]: **`L171197: param_1[10]=0`（初期化）／`L171323: local_4=*(param_1+0x28)`／`L174096: param_3 = *(float*)(param_1+0x28) + param_3`（float 加算）／`L174577: iVar2=*(int*)(param_1+0x28)`（int 読み）／`L174626: param_1[10]=(int)(float)fVar8`（int⇄float 変換）** → **異常値検出と補正: `L175861: FUN_00983ffd((double)(float)param_1[10])`（==0 チェック）／`L175864: __isnan((double)(float)param_1[10]) != 0` → `L175868: param_1[10] = DAT_00b3f9b0`（デフォルト値で補正）[確定(decompiled)]**、`L172829: FUN_006faee0((float)*(ushort*)(param_3+3) / (float)_DAT_00a2fc70 + (float)param_1[10])`（割り算＋加算の scale 計算）→ **`+0x28`＝「NaN/0 チェック付き・`DAT_00b3f9b0` 補正を持つ float/int 兼用数値（scale/倍率系）[推定相当]**、使用 fn は `FUN_004DA4F0`（`+0x28` を5箇所使用）→ **`elem[+0x28]` は REFR のこの数値 field と確定相当、正式な field 名（何の scale か）は [未確定]＝残点** |
| **V-481-3: V-480-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v481_1）

### §2.1 `FUN_005E3320` body（v481_1c）
```
L347984: undefined4 __fastcall FUN_005e3320(int *param_1) {
L347990:   cVar1 = FUN_004d7f80();
L347991:   if (cVar1 != '\0') {
L347992:     iVar2 = (**(code **)(*param_1 + 0x170))();   // == *(param+0x1C) (v480)
L347993:     if (iVar2 != 0) {
L347994:       return *(undefined4 *)(iVar2 + 0x104);      // == *(*(param+0x1C)+0x104)
L347995:     } }
L347997:   return 0; }
compare (L329513 etc): DAT_00b37d00 == *(iVar3 + 0xc)   // *(*(linked+0x104))+0xc
```

### §2.2 REFR `+0x6C` slot（v481_1a）
```
[TESObjectREFR vt=0xA46C44 + 0x6C] = 0x4DF820  owning=FUN_004df820 (L175824)
  4DF820: push ebp / mov ebp,esp / and esp,0xffffffc0 / sub esp,0x30
          / push ebx/ebp/esi / mov esi,ecx       (aligned large fn)
  -> the fn the walk calls on each elem (flag bit3 clear)
  (contrast: TESObjectCELL slot +0x6C = FUN_004cd960 = the walk itself)
```

### §2.3 `+0x28` in REF band（v481_1b）
```
L171197 (FUN_004d9a70): param_1[10] = 0;
L171323 (FUN_004d9c30): local_4 = *(param_1 + 0x28);
L172829 (FUN_004da4f0): FUN_006faee0((float)*(ushort*)(param_3+3)
                          / (float)_DAT_00a2fc70 + (float)param_1[10]);
L174096 (FUN_004da4f0): param_3 = *(float *)(param_1 + 0x28) + param_3;
L174577 (FUN_004da4f0): iVar2 = *(int *)(param_1 + 0x28);
L174626 (FUN_004da4f0): param_1[10] = (int)(float)fVar8;
L175861/864 (FUN_004da4f0): FUN_00983ffd((double)(float)param_1[10]) / __isnan(...)
L175868 (FUN_004da4f0): param_1[10] = DAT_00b3f9b0;      // default-value fixup
```

## §3 残点 **V-482-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`FUN_004DF820`（REFR `+0x6C` impl）の本文 identity**（large fn・walk が各 elem に呼ぶ処理の中身）＋ **`+0x28` の正式 field 名**（`FUN_004DA4F0` の `+0x28` 計算の意味＝scale/倍率の何たるか）。
2. 【低】**旧残点 V-406-4 semantic 完遂** — **`DAT_00B37D00` 正体**（`+0x104` の `+0xc` field が指す singleton・donor（`TESObjectCELL+0x104`）の入手元）→ `+0x64` 中身の語。
3. 【低】**V-481-3 引継ぎ** — `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v481_1 の1本 EXIT=0**（V-481-1 `+0x6C` impl＝`FUN_004DF820` 確定＋`+0x28` 語を推定相当／V-481-2 `FUN_005E3320` 5行 body 完結で `+0x104` 経由を確定相当・正体は残点／V-481-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_005E3320` は decompiled 5行完結** → `return *(iVar2+0x104)` の1行が「`DAT_00B37D00` 比較の源」を確定（**v478 で取得した donor `+0x104` と同 offset が一致**）→ (ii) **REFR vtable slot `+0x6C` を実読**し `FUN_004DF820` を特定、CELL（walk）と別 fn を並立確定 → (iii) `+0x28` は **band usage 8件（float 加算・int⇄float・`__isnan`＋デフォルト補正）**で数値 field と相当まで。
- 妥当性チェック: ①`virtual+0x170`⇔`*(param+0x1C)`（v480）⇔L347992 — ②`*(iVar2+0x104)`⇔v479 donor `*(self+0x104)`（同 offset）— ③`DAT_00B37D00==*(iVar3+0xc)`⇔v479 L216783 donor `+0xc` 比較（同一構造）— ④CELL `+0x6C`＝walk⇔REF `+0x6C`＝`FUN_004DF820`（self/elem 別 impl）— ⑤`__isnan`＋`DAT_00b3f9b0` 補正⇔float field（NaN-safe）— **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **小さな getter（5行 body）が比較式の「源」を確定させる** — `return *(obj+0x104)` の1行で `DAT_00B37D00` 比較のレイヤが donor と一致（**offset `0x104` の横断一致が連結の決め手**）。
  2. **同名 slot（`+0x6C`）でも self（CELL＝walk）と elem（REF＝`FUN_004DF820`）で別 impl** — **vtable ごとに slot を実読する**（クラス横断の slot 推測をしない）。
  3. **`__isnan`＋`DAT_00b3f9b0` デフォルト補正が「float field（NaN-safe）」の目印** — band usage を拾うと field の性格（float/int 兼用・異常補正）が分かる。
  4. **`param_1[10]`（index10＝byte 0x28）と `*(param_1+0x28)` の同一視**（index×4 と byte offset 突合の反復・v474 と同じ）。
  5. **large fn（aligned stack）は本文 identity に時間がかかる** — 「slot で呼ばれる fn である」ことを先に確定し本文は残点化する（**構造確定と本文 identity を分離**）。
