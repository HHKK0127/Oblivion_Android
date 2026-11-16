# v444 — 許可リスト case index 特定・write 供給・census 分離の裁定（V-444-1..3 完遂・派生残点第36弾）

- 採番: **v444**（派生残点群・probe prefix **v444_1..v444_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v443 の残点 **V-444-1【低】（許可リスト case の jump table index 特定と `FUN_0056ae00` の identity）／V-444-2【低】（`FUN_0056ad60` 呼出の case と `[esp+0x5c]` 非ゼロ経路の到達条件）／V-444-3【低】（docs census 機械分離の完成）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-444-1: `FUN_004fab20` の switch 構造（3 table）** | **[確定]** | **(1) char-opcode dispatch**: `movsx edx,bl; add edx,-0x47; cmp edx,0x33; ja default` → **opcode は `bl`、範囲 `'G'(0x47)..0x7A`**、byte remap `0x4FAE4C` → **`jmp [edx*4+0x4FAE3C]`（4 cases: `0x4FABC3/0x4FACE9/0x4FAD06/0x4FAD2E`）[確定]**（char-opcode を4分類へ写像）。**(2) arg type dispatch（35 cases）**: `mov ecx,[ebx]; add ecx,-3; cmp ecx,0x22; ja default` → **`jmp [ecx*4+0x4FB364]`（`type-3`）[確定]**（v438 の path A と同構造）。**(3) byte-remap dispatch（7 cases）**: `cmp eax,0x21; ja` → `movzx edx,byte [eax+0x4FB40C]` → **`jmp [edx*4+0x4FB3F0]`（`0x4FB1B8/0x4FB206/0x4FB25F/0x4FB30A/0x4FB2B8/0x4FB2E0/0x4FB355`）[確定]** |
| **V-444-1: 許可リスト/EffectShader gate の所属 case** | **[確定]** | **table (2) の index 33／34 に該当**（v443 の table dump `0x4FB3E8=0x4FB152`（許可リスト直前）／`0x4FB3EC=0x4FB183`（`cmp byte[eax+4],0x43` gate）＋ bound `type-3`）→ **`type = index + 3` で index33→arg type `0x24`、index34→arg type `0x25`**。**v437 の arg type 表: `0x24=Owner`、`0x25=Effect Shader ID`** → **許可リスト `{TESFaction, TESNPC}` ＝ arg type `Owner` の parameter 検証（owner は Faction/NPC のみ許可）[確定相当]**／**`EffectShader gate`（`FormType==0x43`）＝ arg type `Effect Shader ID` の検証（object の FormType が `TESEffectShader(0x43)` であるべき）[確定相当]** — **FormType 表（v441: `0x43=TESEffectShader`）と完全整合** |
| **V-444-1: `FUN_0056ae00` identity** | **[確定]** | L270604: **`undefined4 __fastcall FUN_0056ae00(undefined4 *param_1)`: `param_1[0..5]` の6-dword ゼロ化 → `FUN_0056a9c0(0x104c)`（`0x104C`=4172 bytes の構造/バッファ初期化）[確定(decompiled)]** → **emit/parse context の initializer（chain `FUN_0056ae00 → FUN_0056b2e0 → FUN_0056b220` の先頭）** |
| **V-444-2: `FUN_0056ad60` 呼出（`0x56A464`）の case** | **[部分確定]** | **`FUN_0056a3b0` に FF/4 jump table は無い → switch は cmp/jcc linear chain 形式 [確定(否定側)]**。`0x56A464` への direct branch は **0件（jcc は周辺の `0x56A455/0x56A458/0x56A45B/0x56A460/0x56A46A/0x56A46E` 短距離群のみ）→ fall-through（case 内 or case 間）[確定]**。**case index の特定は linear cmp chain の cmp 値列挙が要＝[未確定]（残点）** |
| **V-444-2: `[esp+0x5c]` 非ゼロ経路の供給** | **[確定]（供給式）＋[部分確定]（branch 源）** | **block `0x407CB4`**: `407CAE: mov ecx,[0xB3340C]` → **`0x407CB4: mov edx,eax; sub edx,ecx; test edx,edx` → `0x407CBA: mov [esp+0x5c],edx` ＝ `[esp+0x5c] = eax − dword[0xB3340C]`（グローバルとの差分値）[確定(命令)]**。**block `0x407E47`**: `mov eax,[edx+0x174]`（vtable 取得）→ `407E4D: call eax`（**virtual call**）→ `fild [esp+0x38]` → `407E53: mov ecx,[eax]` → **`0x407E5B: mov [esp+0x5c],ecx`＝virtual call 経由の値 [確定(命令近辺)]**。**両 block への branch 源（predicate）は source 表示に不足＝[部分確定]（残点）**。`0xB3340C` の identity は [未確定]（残点） |
| **V-444-3: docs census 機械分離（switch table 除外）** | **[確定]（実測）＋[未確定]（残余の切り分け）** | **non-ret 境界3171件のうち「直前が switch table run（≥8 text ptr）」は45件のみ（1.4%）→ table 境界説はほぼ棄却 [確定(実測)]**、residual **3126**。v442 の histogram（`0xFF=1859/0x00=1214`・jcc32=0）と併せ → **non-ret 大半は `FF..`/disp32 尾＝indirect jmp/call [推定高]**、ただし **`jmp [mem]`（function tail）と function 内 dispatch の切り分けは未達 [未確定]＝残点**。（注: v442 histogram は non-ret 3159、v444 は3171で12件差 — boundary 条件の細部疑い、量級は同 [部分確定]） |

## §2 証拠（v444_1/v444_2）

### §2.1 3 switch（v444_1a）
```
(1) 4FAB9E: mov bl,[eax+edi] / movsx edx,bl / add edx,-0x47 / cmp edx,0x33 / ja default
    4FABB5: movzx edx, byte [edx+0x4FAE4C]  (remap)
    4FABBC: jmp [edx*4+0x4FAE3C]  -> 4 cases (0x4FABC3,0x4FACE9,0x4FAD06,0x4FAD2E)
(2) 4FAF42: mov ecx,[ebx] / add ecx,-3 / cmp ecx,0x22 / ja 0x4fb357
    4FAF50: jmp [ecx*4+0x4FB364]  -> type-3 (35 cases)
    table[33] @0x4FB3E8 = 0x4FB152 (allow-list entry)
    table[34] @0x4FB3EC = 0x4FB183 (EffectShader gate entry)
(3) 4FB1A1: cmp eax,0x21 / ja 0x4fb355
    4FB1AA: movzx edx, byte [eax+0x4FB40C]  (remap)
    4FB1B1: jmp [edx*4+0x4FB3F0]  -> 7 cases
bound mapping: index33 -> type 0x24 (Owner) / index34 -> type 0x25 (Effect Shader ID)  [v437 table]
FormType gate: cmp byte[eax+4],0x43 == TESEffectShader (v441 table)  -> full consistency
```

### §2.2 FUN_0056ae00（v444_1b）
```
L270604: undefined4 __fastcall FUN_0056ae00(undefined4 *param_1) {
  param_1[0..5] = 0;                 ; 6-dword zero
  FUN_0056a9c0(0x104c);              ; 4172-byte struct init
  return extraout_ECX; }
```

### §2.3 write supply（v444_2b）
```
0x407CB4: mov ecx,[0xB3340C]; mov edx,eax; sub edx,ecx; test edx,edx
          -> 0x407CBA: mov [esp+0x5c], edx      ; value = eax - global0xB3340C
0x407E47: mov eax,[edx+0x174] (vtable) ... 407E4D: call eax (virtual)
          -> fild [esp+0x38] / mov ecx,[eax] -> 0x407E5B: mov [esp+0x5c],ecx
branches targeting these entries: decode 表示はあるが source kind 未記録（残点）
```

### §2.4 census split（v444_2c）
```
gaps>0x200, minseg=0x10: non-ret boundaries 3171
  immediate-preceded by switch-table run (>=8 consecutive text ptrs): 45  (1.4%)
  residual: 3126
(v442 histogram: 0xFF=1859 / 0x00=1214 / jcc32=0; v442 non-ret=3159 vs v444=3171, diff 12 に説明未)
```

## §3 残点 **V-445-1..3**
1. 【低】**`FUN_0056a3b0` の linear cmp chain から `0x56A464` の case index 判定**（cmp 値列挙）＋ **`[esp+0x5c]` branch 源**（`0x407CB4`/`0x407E47` への flow predicate）と **`global 0xB3340C` の identity**。
2. 【低】**non-ret residual 3126 の切り分け** — 前 segment 末尾 `FF..` の SIB 種別（`FF /4 jmp [mem]`=tail vs `FF /4 jmp [reg*4+tbl]`=intra-fn dispatch）で機械分離し、function end 数を確定。
3. 【低】**残存する旧残点群への移行** — V-414-2..3／V-413-2／V-415-2 ほか（V-445 で型番を旧群へ回すか新規継続するかは着手時に判断）。

## §4 検証・自己批評
- probe: **v444_1..v444_2 の2本全 EXIT=0**（V-444-1 完遂（case index＋chain 端 identity）／V-444-2 は供給式を確定・predicate は残点／V-444-3 は table 除外を実測・残余切り分けは残点）。
- **決定打の作り**: (i) **jump-table 探索は「fn 内 linear decode で `jmp [reg*4+disp]` を列挙」が最速**（v438 手法の再利用で3本一覧）→ (ii) **case index は「table dwords のアドレス範囲」＋bound の `type-3` で即算出**（v443 の dump と組み合わせ1手）→ (iii) **FormType gate と arg type の表を突合して「Owner/EffectShader」の semantic を確定**（v437×v441 の表が交汇点）→ (iv) write 供給は **block entry からの短い decode** で式が直接見える（`eax−global`）→ (v) census 分離は **「直前に table run が続くか」** を機械判定（45件＝1.4% という否定的結果も重要）。
- 妥当性チェック: ①index33→type0x24=Owner⇔許可 `{Faction,NPC}`（owner の型）— ②index34→type0x25=EffectShader ID⇔gate `FormType==0x43=TESEffectShader` — ③`FUN_0056ae00` の6-dword ゼロ化⇔`FUN_0056b2e0`（uint* 2本引数）の呼び出し整合 — ④table 直後45件≪non-ret 3171⇔v442 の histogram（FF/00 尾99%）— ⑤`[esp+0x5c] = eax−[0xB3340C]`⇔`test edx,edx` 直後の store — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **switch 構造の3 table は「fn 内 `jmp [reg*4+disp]` 全列挙」で一覧化し、bound（`cmp`/`add -N`）と table の距離で各 dispatch の enum を特定する**（char-opcode / arg type / byte-remap の3系統が1 fn に共存する例）。
  2. **case index の意味は「既存の enum 表（v437 arg type／v441 FormType）との突合」で確定できる** — 新規 decode を増やすより表同士の交差を使う（`Owner`/`EffectShader` が1手で一致）。
  3. **jump table dwords が見える段階で index 算出は完了している** — `table[33]=0x4FB152` が見えれば残りは bound（`type-3`）だけ。
  4. **「機械分離」はまず「除外候補の発生率」を測ってから手法を深掘る** — table 直後は45件（1.4%）と判明した時点で table 説は打ち切り、`FF` の SIB 種別分離へ移るのが正しい判断（v444 はその順序で wasted effort を回避できた）。
  5. **write 供給式は block entry 直後の `mov/sub` に表れる**（`eax−global` のように）— predicate より先に「式」を確定させると残点が小さくなる。
