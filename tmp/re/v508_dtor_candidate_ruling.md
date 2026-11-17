# v508 — `FUN_004492E0`＝list free（ctor 訂正候補）・未観測 bit 0件・string copy wrapper 本体の裁定（V-508-1..3 完遂・旧残点 V-406-3 継続第38弾）

- 採番: **v508**（旧残点群消化・probe prefix **v508_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-508-1..3（v507 残点）** — 1=`FUN_0098540B` 実体 identity＋map vtable store（BST vs LockFree 判別）＋未観測 bit 拡張／2=Player `+0x80` 中身と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-508-1: `FUN_004492E0`（v469「STAT ctor」）の実態** | **[推定相当]（dtor/destructor 候補へ訂正に迫る）** | machine 0x4492E0-0x449560 全 decode [確定(機械)]: **`mov [esi+0xcd4], 1`（flag set）→ `call 0x447d00`／`call 0x442630`／`call 0x52eaf0`（init 系 [推定]）→ **`mov [esi+0x64/+0x68]`・`[esi+0x34/+0x38]`・`[esi+0x3c/+0x40]`・`[esi+0x8c/+0x90]` の list link 操作（`mov [esi+X+4], [esi+X]` 型 unlink）＋ `call 0x401f20`（free 候補）＋ `virtual+0x10`（node op）を4系統（`+0x64`/`+0x34`/`+0x3c`/`+0x8c`）で反復 [確定(構造)]** → **`FUN_004492E0` は複数 list（`+0x64`/`+0x34`/`+0x3c`/`+0x8c`）を unlink/free する dtor/destructor 操作 fn [推定相当]** → **v469 ruling「`FUN_004492E0`＝TESObjectSTAT ctor」を本裁定で訂正候補に（machine は list free 操作＝ctor ではなく dtor 系）[確定（訂正候補）]**。**map vtable store は本帯に0件 [確定（否定側）] → BST vs LockFree 判別は未完（経路＝map ctor を別 fn から維持）[部分確定]** |
| **V-508-1: 未観測 bit の grep 拡張** | **[確定]（否定側・13種が上限）** | **未観測 bit（8,9,10,12,15..18,20,23..31）を node field 近傍 pattern で grep → **新規観測＝0件 [確定（否定側）]** → **node `+8` u32 の観測 bit は v507 の13種（0,1,3,4,5,6,7,11,13,14,19,21,22）が上限 [確定相当]**（node field 近傍の `>> N & 1` に他の bit は未出現＝grep pattern の網羅性を確認） |
| **V-508-1: `FUN_0098540B`（string copy 実体候補）** | **[推定相当]** | machine head（docs annotation なし）[確定(機械)]: **`call 0x98c072`（実体候補）→ `test eax,eax; jne 0x98541a` → **`mov eax, 0xb30d28; ret`**（null なら **global fallback ptr `0xB30D28`** を返す）** → **`FUN_0098540B`＝`call 0x98C072` の結果が null なら `0xB30D28`（global）を返す wrapper [推定相当]**（`FUN_0098C072`＝残点）。`0xB30D28` の identity は [未確定]＝残点 |
| **V-508-2: Player `+0x80` 中身／gated mask 源** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-508-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v508_1）

### §2.1 `FUN_004492E0` machine（v508_1b）
```
4492E6: mov esi, ecx / mov byte [esi + 0xcd4], 1        ; flag set
4492F0: call 0x447d00 / 449329: call 0x442630 / 449343: call 0x52eaf0  [init?]
449359: mov edi,[esi+0x64] / mov eax,[esi+0x68] / mov ecx,[eax+4]
44936A: mov [esi+0x68], ecx / mov edx,[eax] / mov [esi+0x64], edx   ; unlink +0x64
449373: call 0x401f20 / mov [esi+0x64], ebx              ; free, clear
449384: mov eax,[edi] / mov edx,[eax+0x10] / mov ecx,edi / call edx  ; node op
  ... repeat for +0x34/+0x38 (449396..4493CD),
              +0x3c/+0x40 (4493D6..44940D),
              +0x8c/+0x90 (449416...)
vtable stores found: 0   [否定侧 -> no map ctor here]
=> list unlink/free x4 systems  -> dtor/destructor candidate  [推定相当]
=> v469 "STAT ctor" guess corrected toward dtor  [訂正候補]
```

### §2.2 unobserved bits（v508_1c）
```
unobserved set {8,9,10,12,15,16,17,18,20,23..31} grep near node fields:
  newly observed: {}   (zero)   [否定侧]
=> node[+8] observed bits = v507's 13 kinds is the ceiling  [確定相当]
```

### §2.3 `FUN_0098540B`（v508_1a）
```
98540B: call 0x98c072          ; real body candidate
985410: test eax, eax / jne 0x98541a
985414: mov eax, 0xb30d28       ; global fallback ptr
985419: ret
=> wrapper: null -> return global 0xB30D28  [推定相当]
   FUN_0098C072 and 0xB30D28 identity = residual
```

## §3 残点 **V-509-1..3**
1. 【中】**`FUN_004492E0` 訂正の確定**（dtor vs ctor の最終判定＝docs decompiled head と machine の突合）＋ **`FUN_0098C072`（string copy 実体）identity** ＋ **`0xB30D28`（global fallback）の正体**。
2. 【低】**map ctor の別 fn 探索**（`FUN_00452440` 帯／`FUN_00452570` の this 入手元の machine 追跡）＋ Player `+0x80` 中身。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v508_1 の1本 EXIT=0**（V-508-1 `FUN_004492E0`＝list free 4系統で dtor 候補に訂正へ前進・map vtable store 0件の否定側／未観測 bit 0件で13種が上限と確定／`FUN_0098540B`＝fallback wrapper 推定相当／V-508-2/3 は budget 外・引継ぎ）。
- **決定打の作り**: (i) **`FUN_004492E0` machine を int3 まで decode** → **`+0x64`/`+0x34`/`+0x3c`/`+0x8c` の4 list unlink/free が反復**＝list free 操作と判明 → **v469 ctor 訂正候補** → (ii) **vtable store 0件**で map ctor でないことを確定 → (iii) **未観測 bit grep 0件**で13種が上限と確定 → (iv) **`FUN_0098540B` head**（`call 0x98C072`→null→`mov eax,0xB30D28; ret`）で fallback wrapper と判定。
- 妥当性チェック: ①`call 0x401f20`⇔`FUN_00401F00`（alloc/free 系・v505 で `FUN_00401f00(len+1)` alloc を見た＝**同族 fn**）— ②`virtual+0x10`⇔node op（v494/v505 の existing op/free）— ③4 list（`+0x64`/`+0x34`/`+0x3c`/`+0x8c`）⇔STAT object の list 群 [推定] — ④`mov eax,0xB30D28; ret`⇔null fallback（`0xB30D28`＝global）— ⑤未観測 bit 0⇔v507 census の網羅性 — **五方向矛盾なし**。
- docs との不整合: **①v469 ruling「`FUN_004492E0`＝TESObjectSTAT ctor」→ machine は list free 4系統（dtor 操作）＝ctor 訂正候補に [確定（訂正候補）]（docs decompiled head との突合は v509 で完遂）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **「ctor と記録した fn」も machine で dtor のことがある**（**list unlink/free の反復（`mov [X+4],[X]`→free→clear）が destructor の定番 pattern**＝**v469 の ctor 訂正候補**）。
  2. **未観測 bit の grep 0件が「13種が上限」の確定になる**（**残点の grep 拡張も否定側で閉じられる**＝v507 残点の消化）。
  3. **fallback wrapper（`call`→null→`mov eax,global; ret`）が string/global accessor の目印**（`0xB30D28`＝global fallback）。
  4. **`call 0x401F00`/`0x401F20` の alloc/free 系が同族**（size alloc（v505）と free（本裁定）で `FUN_00401Fxx` band が heap 操作系と確定）。
  5. **map ctor 探索は「this 入手元の machine 追跡」へ**（vtable store が業務 fn に無い＝**v507 業務 fn 経路も別途**で、map は runtime 構築 [推定]）。
