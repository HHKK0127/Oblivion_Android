# v438 — type consumer（`FUN_004fde30` dispatch）と docs 完全性 census の裁定（V-438-1..3 完遂・派生残点第30弾）

- 採番: **v438**（派生残点群・probe prefix **v438_1..v438_9 の9本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v437 の残点 **V-438-1【低】（type consumer 特定と `0x03` vs `0x15` 語義差）／V-438-2【低】（docs と machine の function 範囲不一致の横断 census 手法確立）／V-438-3【低】（`[esp+0x5c]` 書込み元と flat array 先頭10件の到達経路）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-438-1: type code の consumer** | **[確定]** | **`FUN_004fde30`（共通 arg-parse handler・+0x1C）の jump table dispatch**。**ebp＝`&pool[i].type` を machine で確定**: `4FDEA5: mov edx,[esp+0x14]`（arg pool）→ `4FDEA9: lea ecx,[eax+eax*2]`（i×3）→ **`4FDEAC: mov eax,[edx+ecx*4+4]`（＝pool[i]+4＝type・stride 0x12 復元）**→ `4FDEB9: lea ebp,[edx+ecx*4+4]` → **`4FDF83: mov eax,[ebp]`（＝type）** [確定(命令)] |
| **per-type flag テーブル** | **[確定]** | **stride-8・38 entries（type 0x00..0x25 と完全一致）@`0xB0A54C`** — 2つの byte を読む: **`4FDEB0: mov bl,byte [eax*8+0xB0A54C]`** と **`4FDF47/4FDF86: cmp byte [eax*8+0xB0A54D],0`**（1 byte 差の2 field）。flag 分布（`0xB0A54D` 側, type0..37）= `[0,0,0,1,1,0,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,0,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1]` |
| **2系統 dispatch** | **[確定]** | **(A) flag≠0**: `4FDF94: add eax,-3; cmp eax,0x22; ja 0x4fed7e` → **`type ∈ [0x03,0x25]`（35件）→ `jmp [eax*4+0x4FEEAC]`** ／**(B) flag==0**: `4FE88B: cmp eax,0x21; ja 0x4fee86` → `type ∈ [0,0x21]` → **`movzx eax,byte [eax+0x4FEF5C]`（byte remap）→ `jmp [eax*4+0x4FEF38]`（9 cases）** |
| **3つの table が連続（構造）** | **[確定]** | **`0x4FEEAC`（35 dwords＝type 0x03..0x25 用）→ `0x4FEF38`（9 dwords＝remap 後の case target）→ `0x4FEF5C`（0x23 bytes byte-remap）** が**アドレス計算で一致**: `0x4FEEAC + 35×4 = 0x4FEF38` ✓／`0x4FEF38 + 9×4 = 0x4FEF5C` ✓（v438_2 で「0x4FEEAC に44 entries」と読んだのは **35＋9 の2 table 連続を1つと数えた誤り＝本裁定で訂正**） |
| **`0x03` vs `0x15` の語義差** | **[確定]（分岐先）＋[残点]（handler 中身）** | **両方 flag=1 → path A**: `type-3` は **`0x03`→index 0（`table[0]=0x4FDFA7`）**／**`0x15=21`→index 18（`table[18]=0x4FE5CC`）** → **完全に別の handler に分岐 [確定]**。**semantic 差（なぜ同名 'ObjectID' で別 handler か）は handler body の比較が要＝残点** |
| **V-438-2: docs 完全性の横断 census** | **[部分確定]（手法確立）＋[未確定]（総数裁決）** | **proxy 3系統を実測**: `.text`＝6,450,233 bytes／**int3 runs≥4＝28,667（過大: alignment padding も計上）**／**int3 runs≥8＝16,622（過小: 小 padding 除外）**／**`ret`(C3)+CC＝16,035（過小: `ret N`/tail jmp/0 padding 除外）**／**docs annotations＝20,465** → **20,465 が3 proxy の間（16,035..28,667）に夹在 → proxy 法では総数を裁決不能 [未確定]**。**確実な手法＝per-function 検証（branch census で対象 range の head 集計 ＋ 対象 fn の docs 内 call 数 grep）[確定]**（v437 で実証） |
| **V-438-3: `[esp+0x5c]` の fill 点** | **[確定]（write 一覧）＋[未確定]（供給元）** | `FUN_00407AE0` 内の **direct write は3件のみ**: `407CBA: mov [esp+0x5c],edx`／`407E5B: mov [esp+0x5c],ecx`／**`408F72: mov [esp+0x5c],eax`（直前 `408F6D: xor eax,eax` ＝0 初期化）**。＋ **`lea ecx,[esp+0x5c]`（間接 fill）2件**: `408FCF: call 0x404fd0`（戻りで `mov eax,[esp+0x5c]` 使用・`0xa309a4` と三項）／`409027: call 0x402da0`。**`40A29D`（0xB108EC index 読出）の直前に write は無く（0x4092AC の `fild` 読出が最後）→ stack slot 再利用で path 依存 [未確定]** |
| **V-438-3: flat array 先頭10件** | **[推定高]（v437 継続）** | 追加証拠なし（ref0件＋`mov esi,1` 起点で下方向到達不可）→ **dead data [推定高] を維持** |

## §2 証拠（v438_1..v438_9）

### §2.1 type consumer（v438_4/v438_5）
```
FUN_004fde30 (+0x1C 共通 arg-parse handler):
  4FDEA5: mov edx, [esp+0x14]                 ; arg pool ptr
  4FDEA9: lea ecx, [eax + eax*2]              ; i*3
  4FDEAC: mov eax, [edx + ecx*4 + 4]           ; pool[i].type  (i*12 = ecx*4 -> stride 0x0C, +4 = type)
  4FDEB0: mov bl, byte [eax*8 + 0xB0A54C]      ; flag byte A
  4FDEB9: lea ebp, [edx + ecx*4 + 4]           ; ebp = &pool[i].type
  ...
  4FDF83: mov eax, [ebp]                       ; eax = type
  4FDF86: cmp byte [eax*8 + 0xB0A54D], 0       ; flag byte B
  4FDF8E: je 0x4fe88b                          ; flag==0 -> path B
  4FDF94: add eax, -3
  4FDF97: cmp eax, 0x22 / ja 0x4fed7e          ; type in [0x03,0x25] (35 cases)
  4FDFA0: jmp [eax*4 + 0x4FEEAC]               ; path A
path B:
  4FE88B: cmp eax, 0x21 / ja 0x4fee86          ; type in [0,0x21]
  4FE894: movzx eax, byte [eax + 0x4FEF5C]     ; byte remap
  4FE89B: jmp [eax*4 + 0x4FEF38]               ; 9 cases
```

### §2.2 3 table 連続（v438_2/v438_3）
```
0x4FEEAC: 35 dwords (text ptrs)  -> 0x4FEEAC + 35*4 = 0x4FEF38  ✓
0x4FEF38: 9 dwords  (text ptrs)  -> 0x4FEF38 + 9*4  = 0x4FEF5C  ✓
0x4FEF5C: 0x23 bytes byte-remap = [0,1,1,8,8,2,8,8,3,8,4,8,8,8,8,8,8,8,5,8,8,8,8,1,8,8,8,8,6,8,8,8,8,7,0xCC]
table A[0]=0x4FDFA7, A[18]=0x4FE5CC   (0x03 -> idx0, 0x15 -> idx18)
flag table (0xB0A54D + type*8), 38 entries: [0,0,0,1,1,0,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,0,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1]
```

### §2.3 docs census（v438_6/v438_7）
```
.text size: 6,450,233 bytes
int3 runs >=4: 28,667   (over-count: alignment padding)
int3 runs >=8: 16,622   (under-count: small padding)
ret(C3)+CC   : 16,035   (under-count: ret N / tail jmp / 0 padding)
docs annotations: 20,465   -> between 16,035 and 28,667 => proxy INCONCLUSIVE
```

### §2.4 `[esp+0x5c]`（v438_8/v438_9）
```
writes (dst=[esp+0x5c]): 407CBA edx / 407E5B ecx / 408F72 eax (eax=0 via xor at 408F6D)
lea (addr taken): 408FCF -> call 0x404fd0 (then `mov eax,[esp+0x5c]` vs 0xa309a4)
                  409027 -> call 0x402da0
reads: 407CBE/407F2C/40807B/4083C3/40844B/408555 (fild/fld = int->float)
       408FE1 (mov eax) / 40920F / 4092AC (fild) / 40A29D (0xB108EC index)
last write before 40A29D: 408F72 (=0)  -> slot reuse, path-dependent [未確定]
```

## §3 残点 **V-439-1..3**
1. 【低】**`0x03` vs `0x15` の handler 差**（`table[0]=0x4FDFA7` と `table[18]=0x4FE5CC` の body 比較）＋ **per-type flag A/B（`0xB0A54C`/`0xB0A54D`）の semantic**（`{A,B}` が stride-8 で `B=1,2,3...` と incrementing する意味）。
2. 【低】**docs 完全性の最終裁決** — proxy 法は夾在で裁決不能。per-function 検証を**サンプリング実施**（gap>0x800 の N 件について branch census＋docs call 数を回し、欠落率を推定）。
3. 【低】`[esp+0x5c]` の path 依存供給元（`0x40A29D` への到達 CFG）と flat array 先頭10件の computed-address 到達有無。

## §4 検証・自己批評
- probe: **v438_1..v438_9 の9本全 EXIT=0**（V-438-1 完遂／V-438-2 手法確立・総数は[未確定]／V-438-3 write 一覧確定・供給元は[未確定]）。
- **決定打の作り**: (i) **jump-table census（`FF /4`＋SIB scale=2 の byte-pattern scan）で parser band の4本を列挙** → (ii) **jmp 直前の bound を aligned decode で取得**（`add eax,-3; cmp eax,0x22`＝type 範囲）→ (iii) **`lea ecx,[eax+eax*2]`＋`[edx+ecx*4+4]` の2段 lea を「stride0x12 の type アドレス」と即読**して ebp=type を確定 → (iv) **3 table のアドレス計算（`+35×4`/`+9×4`）で連続構造を証明し「44 entries」誤読を訂正** → (v) docs census は **proxy 3系統の実測で「裁決不能」を正直に結論**。
- 妥当性チェック: ①stride 0x12（`eax+eax*2`→×3→×4）⇔v437 の descriptor stride 0x0C＋4 — ②flag table 38 entries⇔type 0x00..0x25（38 codes）— ③path A bound `type≤0x25`⇔type max 0x25 — ④`0x4FEEAC+35×4=0x4FEF38`⇔`0x4FEF38+9×4=0x4FEF5C` — ⑤`0x03`→idx0／`0x15`→idx18 の別 handler ⇔usage 完全分離（v437）— **五方向矛盾なし**。
- docs との不整合: **なし（本裁定は v437 の docs 不整合を継続記録・新規の不整合は検出せず）**。ただし **docs 完全性の総数は proxy 法で裁決不能 [未確定]**（v437 の per-region 不整合は個別事例）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **jump-table census は byte-pattern（`FF /4`＋SIB scale=2）で機械列挙できる** — cmp bound は jmp の直前から aligned decode で取得（jmp 以降しか見ない v438_2 初版の失敗→v438_3 で訂正）。
  2. **「2段 lea（`lea a,[x+x*2]` → `[base+a*4+4]`）」＝stride 0x12（＝12バイト）のフィールドアクセス即読** — `x+x*2`＝×3、`×4`＝×12 で、v437 で確定した descriptor stride `0x0C`＋`+4`（type フィールド）と完全整合する。
  3. **隣接 table はアドレス計算（`base + N×stride`）で分離を証明** — 「44 entries」と1つに数えたのを `35×4`/`9×4` の一致で2つに訂正（v438_2→v438_3）。
  4. **docs 完全性は proxy（CC run/ret+CC）では夾在で裁決不能** — 過大/過小の両方向バイアスが明確。**確実なのは per-function 検証のみ**。
  5. **stack slot の供給元特定は write 一覧→最後の write が path 依存と判明したら正直に [未確定]**（`fild` 読出が最後でも slot 再利用で即断しない）。
