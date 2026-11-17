# v474 — `elem[10]` の機械確定・`form+0x69` caller 検証・`virtual+0x170` semantic の裁定（V-474-1..3 完遂・旧残点 V-406-3/V-406-4 継続第4弾）

- 採番: **v474**（旧残点群消化・probe prefix **v474_1..v474_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-474-1..3（v473 残点）** — 1=旧残点 V-406-3 継続（`elem[10]` class と `self[+0xC]` 上位 byte `0xFF` semantic）／2=旧残点 V-406-4 残り（`form+0x69` byte 値の意味と `[esi+0x5F]` slot 特定）／3=V-473-3 引継ぎ（`FUN_005222D0` 一語名別法と predicate slot35 英名裏取り）を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-474-1: `elem[10]`（byte +40 float）の class/field** | **[確定]（機械）＋[推定高]（class）** | **`FUN_004CD960` の walk を 60 命令 decode [確定(機械)]**: `mov esi,ecx`（esi=self）→ `lea edi,[esi+0x48]`（list head）→ **`mov ebx,[edi]`（node）→ `mov eax,[ebx+8]; shr eax,3; test al,1`（node flag bit3）→ flag 未立なら `mov edx,[ebx]`（vtable）→ `mov eax,[edx+0x6c]; call eax`（virtual +0x6C＝unregister 系・`mov ecx,[0xB33B00]` 経由 `FUN_004637C0` 連動）** → **`test byte [esi+0x24],1`（class form flag bit0・v465 setter 系統の読手）→ `mov edx,[ebx]`→**`mov eax,[edx+0x170]; mov ecx,ebx; call eax`→`cmp eax,[0xB35EB8]`（TESObjectSTAT singleton・v470）** → **`fld dword [ebx+0x28]`（**byte +40 float＝decompiled `piVar8[10]` と完全一致**）→ `fmul qword [0xA3D360]`（-1.0・v469）→ `lea ecx,[esi+0x28]`（self+0x28＝receiver！）→ `call 0x420C60`（id 0x4C set）** [確定(機械)] → **`elem`＝`ebx`（list node）、`elem[+0x28]`（byte 40）float を `-1.0` 倍して self+0x28 の receiver 経由で id `0x4C` に SET [確定相当]**。**`virtual[elem]+0x170()` が `TESObjectSTAT` singleton と等しい elem だけが対象 → elem は TESObjectSTAT 系 form type を持つ object [推定高]**（elem の vtable 完全特定は [未確定]＝残点） |
| **V-474-1: `self[+0xC]` 上位 byte `0xFF` semantic** | **[部分確定]（整数 field）＋[未確定]（`0xFF` の語）** | docs band 提及: **`*(param_1+0xc) == 7`（L216781）／`param_1[3] != 7`（L217625）＝整数値として `7` と比較される field [確定(decompiled)]**。`FUN_00566830` band では `FUN_00446B80(param_1[3])` の4箇所（L266318/266325/266347/266354）→ **`+0xC` は「==7 比較と gate（上位 byte 0xFF）が併用される整数 field」[確定相当]**。**`0xFF` 上位 byte が何の状態を意味するかは [未確定]＝残点**（docs に `& 0xff000000` 比較が13件あり v473 gate と同型 pattern [確定(decompiled)]、L54115 `(param_2 & 0xff000000)==0xff000000` 等） |
| **V-474-2: `form+0x69` byte setter caller の検証（`[esi+0x5F]` 訂正）** | **[確定]（caller は正しい）＋[確定]（v473 context 表示を訂正）** | **raw bytes `0x522730: 5D F4 FF 8B 8F 04 01 00 00 E8 C2 6F F8 FF 8B CE 50 E8 2A 5B F4 FF ...` ＝ `0x522740: push eax / 0x522741: call 0x468270`（real E8・boundary decode で verified=True）[確定(機械)]** → **v473 の caller 記録（`0x522741` owning=`FUN_005222D0`）は正しい**。ただし **v473 の context 表示「`522745: call ptr [esi+0x5f]`」は int3 帯 linear decode のずれによる誤表示 [確定(否定側)→訂正]**: 実際の後続は **`add esp,0x1c; ret 4`（fn 末尾エピローグ）** → **`FUN_00468270` は `FUN_005222D0` の末尾（`push eax` 値＝byte を set）から呼ばれる [確定相当]**。**`[esi+0x5F]` slot は不存在（誤 decode）[確定負]** → V-474-2 の slot 特定は**打ち切り** |
| **V-474-3: `FUN_005222D0` 一語名別法と predicate slot35 英名** | **[未確定]（残点継続）** | 本 probe 未着手（budget は V-474-1/2 に集中） |
| **副次: `virtual+0x170()` の semantic** | **[推定高]** | docs `+0x170` usage **831件** [確定]。**L42487 `*(char *)(iVar2 + 4) == '\x1a'`＝`virtual+0x170()` の戻り値の `+4` byte が FormType と比較** [確定(decompiled)]、**v470 で TESObjectSTAT singleton の `byte[+4]=0x1C` を確定済み** → **`virtual+0x170()` ＝ form type singleton を返す getter（`GetType()` 相当）[推定高]**（`+4`＝FormType byte の構造が共通） |

## §2 証拠（v474_1/v474_2）

### §2.1 walk 機械 decode（v474_1a）
```
4CD960: sub esp,0x14 / push ebx/ebp/esi/edi / mov esi,ecx
4CD969: lea edi,[esi+0x48]        ; list head (v471 +0x48 sentinel field と整合)
4CD976: cmp dword [edi+4],0 / jne / cmp dword [edi],0 / je end
4CD985: mov ebx,[edi]             ; node
4CD987: mov eax,[ebx+8] / shr eax,3 / test al,1 / jne skip
4CD991: mov edx,[ebx]             ; vtable
4CD993: mov eax,[edx+0x6c]        ; virtual +0x6C
4CD996: mov ecx,ebx / call eax
4CD99A: mov ecx,[0xb33b00] / push ebx / call 0x4637c0 (FUN_004637c0)
4CD9A6: test byte [esi+0x24],1 / je  ; class form flag bit0 (v465)
4CD9AC: mov edx,[ebx] / mov eax,[edx+0x170] / mov ecx,ebx / call eax
4CD9B8: cmp eax,[0xb35eb8] / jne    ; TESObjectSTAT singleton (v470)
4CD9C0: fld dword [ebx+0x28]        ; *** elem byte-40 float ***
4CD9C4: fmul qword [0xa3d360]       ; -1.0 (v469)
4CD9CA: lea ecx,[esi+0x28]          ; *** receiver = self+0x28 ***
4CD9D8: call 0x420c60               ; id-0x4C float SET
4CD9DD: test dword [esi+8],0x400 / jne
4CD9E6: mov ecx,ebx / call 0x6ecc80  ; CLAS getter
4CD9ED: cmp eax,esi / je
... loop: mov ebp,edi / mov edi,[edi+4] / jne 4cd976
```

### §2.2 `self[+0xC]` docs 提及（v474_1b）
```
FUN_005222D0 band (4): L216781 *(param_1+0xc) == 7 / L217407 == *(param_2+0x248)
                        L217625 param_1[3] != 7 / L217722 *(short*)(+0xc) += -1
FUN_00566830 band (5): L266292 FUN_00446b80(*(param_1+0xc))
                        L266318/266325/266347/266354 FUN_00446b80(param_1[3])
docs &0xff000000: 13 (L54115 (param_2 & 0xff000000)==0xff000000 / L61182 ...)
```

### §2.3 caller 検証（v474_2a/c）
```
raw @0x522730: 5D F4 FF 8B 8F 04 01 00 00 E8 C2 6F F8 FF 8B CE 50 E8 2A 5B F4 FF 5E 5F 83 C4 1C C2 04 00 6A 00 8D 4F 68 E8 D8 5A F4 FF
decode@0x522741: call 0x468270 (BYTES_OK)
strict from 0x522748: add esp,0x1c / ret 4 / push 0 / lea ecx,[edi+0x68] / call 0x468230
E8 census FUN_00468270: 1 candidate 0x522741 -> owning FUN_005222d0 verified=True
-> v473 caller record correct; v473 context line "call ptr [esi+0x5f]" was misdecode [訂正]
```

### §2.4 `virtual+0x170` semantic（v474_2b）
```
docs +0x170 usage: 831
L42487: (iVar2 = virtual[*param_2 + 0x170](), *(char *)(iVar2 + 4) == '\x1a')
L32194/32221/32222/32224/39983/42323: virtual[X + 0x170]() -> result used as form
v470 tie: TESObjectSTAT singleton byte[+4] = 0x1C (FormType)
-> virtual+0x170() returns form-type singleton (byte +4 = FormType) [推定高]
```

## §3 残点 **V-475-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`elem` の vtable 完全特定**（`[ebx]+0x6C`/`+0x170` の実装 fn→vtable→RTTI）と **`elem[+0x28]` float の語**（TESObjectSTAT 系 form の何の property か）＋ `self[+0xC]` 上位 byte `0xFF` の semantic（何の状態か・`==7` 比較との関係）。
2. 【低】**旧残点 V-406-4 残り** — `form+0x69` byte の値（`push eax` の eax が何を含むか・derived AV 初期適用内での位置づけ）。
3. 【低】**V-474-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v474_1..v474_2 の2本全 EXIT=0**（V-474-1 `elem[10]` 機械確定＋`self[+0xC]` 整数 field 確定／V-474-2 caller 検証で v473 訂正・slot 打切／V-474-3 未着手＝残点）。
- **決定打の作り**: (i) **walk を 60 命令 linear decode** → `fld [ebx+0x28]`→`fmul -1.0`→`call 0x420C60` の連鎖が v473 decompiled 式と機械で完全一致 → elem field 確定 → (ii) **raw bytes 実読＋boundary decode** で E8 検証（`BYTES_OK`＋forward decode 一致）→ v473 の caller は正しい/context 表示のみ誤りと切り分け → (iii) **docs `+0x170` usage 831件から L42487（`+4 == 0x1a`）を抽出** → FormType 比較と v470 の `byte[+4]=0x1C` が接続。
- 妥当性チェック: ①`[ebx+0x28]`⇔`piVar8[10]`（int index10=byte40=0x28 ✓）— ②`fmul [0xA3D360]`⇔v469 `-1.0` — ③`lea ecx,[esi+0x28]`⇔`FUN_00420C60` は `__thiscall`（ecx=receiver）→ **receiver＝self+0x28 が新事実** — ④`cmp eax,[0xB35EB8]`⇔v470 TESObjectSTAT singleton — ⑤`+4==0x1a`⇔`byte[+4]=0x1C` FormType 構造（v470）— **五方向矛盾なし**。
- docs との不整合: **①v473 ruling の context 表示「`522745: call ptr [esi+0x5f]`」→ 本裁定で誤 decode と訂正（caller は正しい）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`lea ecx,[reg+N]` が `__thiscall` setter の receiver** — `FUN_00420C60` の receiver が `self+0x28`（class form の +0x28 field）と分かった（**float setter の receiver field が field map に加わる**）。
  2. **E8 candidate 検証は「raw bytes 実読＋境界 decode＋forward walk の3点」** — int3 帯 linear decode のずれ（v473 の `call ptr [esi+0x5f]` 誤表示）を切り分けられる。
  3. **`[reg+disp]` を跨ぐ decompiled↔machine 突合は index×4 と byte offset で照合**（`piVar8[10]`⇔`[ebx+0x28]`）。
  4. **`virtual+0x170()` の結果の `+4` byte 比較（`==0x1a`）が「form type singleton getter」の目印** — usage 831件のうち1行で semantic が当たる（**大人数 usage から構造を語る1行を拾う**）。
  5. **誤 decode の訂正も裁定に明記**（v473 の context 誤りを v474 で特定して row に残す＝追跡可能性）。
