# V94: v79§6 の境界・重複決着（class3 バイナリ確認 / 境界判例9件 / V5 重複5件）

- 日付: 2026-10-25
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, ImageBase 0x400000, .text VA 0x401000 / raw 0x400）
- 前提確認: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**
  （`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl=0x504560`）
- スコープ: `v79_absorbed_tracking.md` §6 の残課題 **2 / 3 / 6**。§6-1「全 21,597 関数本文重複照合」は重量のため**非実施**（可否検討のみ §d）。

## §0 入力・手法・生成物

### 入力（読むだけ・編集なし）

| パス | サイズ | 用途 |
|---|---|---|
| `tmp/re/v79_absorbed_tracking.md` | 24,762 B / 376 行 | §6 残課題・判定規則・class2/3 全VAリスト |
| `tmp/re/v10_impl_decompile_join.csv` | 114,668 B | `impl` / `in_decompile` / `prev_fun` / `next_fun` / `gap_kind` / `cc_prefix` |
| `tmp/re/v5_ghidra_gaps_report.md` | 73,523 B | §3.1 全ギャップリスト 362 行 / §4.1 Impl Addr 一覧 75 行 / §4.2 / §5 |
| `docs/Oblivion_decompiled.c` | 28,995,463 B / 1,066,496 行 | `dec_index` 20,465 件、前後ラベル本文 |
| `%TEMP%\opencode\func_list_new.txt` | 477,294 B / 21,597 行 | Ghidra FunctionManager 台帳（prev/next 境界の正） |
| `%TEMP%\opencode\v79_bodies.json` | 220,358 B / 357 本 | V70 dump から再抽出したギャップ本文 |
| `%TEMP%\opencode\v79_classified.csv` | 34,980 B / 357 行 | V79 の cls / reason |
| `%TEMP%\opencode\v70_force_create.csv` | 27,259 B / 362 行 | V70 生成結果（補助） |
| `%TEMP%\opencode\v70_decomp_dump.txt` | 228,917 B | V70 デコンパイル全文（補助） |

> **注記**: タスク記載の `tmp/re/v70_gap_decompile.csv` は**リポジトリに存在しない**。V70 の実データは `%TEMP%\opencode\v70_force_create.csv`（362 行: `va/pre_status/entry/name/decomp_ok/decomp_chars`）と `v70_decomp_dump.txt`（362 ブロック）であり、本レポートは `v79_bodies.json`（dump から再抽出済み・V79 が L の正として使用した物）を本文の正として使う。

### 手法

- PE は手動パース（pefile なし）。`probe_harness.Ctx.rd()`（セクション対応: .text const=0x400C00 / .rdata=0x400E00 / .data=0x401400、bss は None+警告）、`enclosing_fun()` で decompiled.c 行→ラベル。
- capstone 5.0.7（`skipdata=True`, `md.detail=True`）。**境界の定義**: 「台帳上の次の開始点までの中、末尾連続 `0xCC` ランを取り除いた残り」を実体、その `0xCC` ランを pad_after、実体の直前の連続 `0xCC` 数を pad_before とする。終端は関数入口から線形デコードして最初の `ret/retn/iret/jmp/hlt`（入口起点でないとデータ起点のデコードがずれるため）。
- Ghidra 未接続。デコンパイル引用は `docs/Oblivion_decompiled.c`（部分検索のみ・ラベル小文字 hex）と V70 dump。

### 生成物（すべて repo 外）

- `%TEMP%\opencode\v94_probe.py`（17,350 B）/ `v94_probe2.py`（11,553 B）/ `v94_probe3.py`（6,006 B）/ `v94_probe4.py`（4,015 B）
- `%TEMP%\opencode\v94_probe_out.txt` / `v94_probe2_out.txt` / `v94_probe3_out.txt` / `v94_probe4_out.txt`
- `%TEMP%\opencode\v94_raw.json` / `v94_raw2.json`
- **repo 内の新規ファイルは本レポート 1 件のみ**（docs/・既存 tmp/re・TODO・memory・plan.md は未編集、git 未実行）

### 機械規則（§b 判定用・事前定義）

| 規則 | 内容 | 測定方法 |
|---|---|---|
| **R1a** 直前分離 | 台帳上の直前関数の本体最終命令が `ret`/`jmp` で終わる | 関数入口からの線形デコード |
| **R1b** 後続パディング | 本体終端の直後に `0xCC` ランが ≥1 B ある | pad_after ≥ 1 |
| **R2** カバレッジ | V70 dump 本文の呼び出し数 == 本文中の `E8 call` 数（過不足なし） | capstone `call 0x…` 計数 |
| **R3** 近傍非重複 | V5 の「直前/直後ラベル」＝`v10 prev_fun/next_fun` の decompiled.c 本文と**厳格骨格**（空白除去＋`FUN_*`/`func_0x*`/`0x…(4桁以上)` → `@`）が不一致 | 両方向 |
| **R4** 単一エントリ | オペコード表 `+0x18 impl` 列に現れるのは **1 エントリのみ** | 532 件全走査 |

**判定**: R1a ∧ R1b ∧ R2 ∧ R3 ∧ R4 = 充足 → **[格上げ]**（V79 class2 → 完全関数）。1 つでも欠く → **[維持]**。

---

## §a class3 3 件のバイナリ確認

### a.1 結論

**3/3 とも「最小実装（ハンドラ）」。データ誤分割ではない。**

### a.2 判定表

| VA | 実体サイズ | 命令数 | 終端 | pad_after | pad_before(int3) | 台帳 span（prev→next） | `+0x18 impl` 参照 | 他列 | .text imm32 | .data imm32 | flagA/argn | 判定 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0x00500820 | **8 B** | 3 | `ret` @0x500827 | **8** | **1** | 16 B（FUN_005007d0→FUN_00500830） | **1 件** idx113 / opid 0x0152 `ShowRenderPasses`(srp) | 0 件 | **0** | 1（=表エントリ自身） | 0x00 / 0 | **最小実装** |
| 0x00501A60 | **18 B** | 5 | `ret` @0x501A71 | **14** | **7** | 32 B（FUN_00501a00→FUN_00501a80） | **1 件** idx57 / opid 0x011A `ToggleGrassUpdate`(TGU) | 0 件 | **0** | 1 | 0x00 / 0 | **最小実装** |
| 0x005087A0 | **18 B** | 5 | `ret` @0x5087B1 | **14** | **2** | 32 B（FUN_00508700→FUN_005087c0） | **1 件** idx67 / opid 0x0124 `ToggleShadowVolumes`(TSV) | 0 件 | **0** | 1 | 0x00 / 0 | **最小実装** |

「他列」= `+0x14 argdesc` / `+0x1C dispatcher` / `+0x20 cond` にこの VA を持つエントリは **0 件**。
`.text imm32 = 0` = .text 内にこの VA を 32bit 定数として持つ箇所は無く（= データ参照ではなくテーブル経由の間接到達のみ）、`.data imm32 = 1` はオペコード表の `+0x18` エントリそのもの。

### a.3 実体バイト列（V5 §3.1 の prologue hex と完全一致）

```
0x00500820  b0 01 a2 d8 2c b4 00 c3                    ; V5: b001a2d82cb400c3 + cc… ✓
            mov al,1 ; mov byte ptr [0xb42cd8], al ; ret
0x00501A60  80 3d e4 9a b0 00 00  0f 94 c0  a2 e4 9a b0 00  b0 01  c3   ; V5: 803de49ab000000f94c0a2e49ab000b0… ✓
            cmp byte ptr [0xb09ae4],0 ; sete al ; mov [0xb09ae4],al ; mov al,1 ; ret
0x005087A0  b0 01  38 05 3c 2f b4 00  0f 95 c1  88 0d 3c 2f b4 00  c3   ; V5: b00138053c2fb4000f95c1880d3c2fb4… ✓
            mov al,1 ; cmp [0xb42f3c],al ; setne cl ; mov [0xb42f3c],cl ; ret
```

- 3 件とも **read-modify-write（フラグ反転/設定）+ `mov al,1` + `ret`** の完結した 3〜5 命令。
- V5 の prologue hex とバイナリが **先頭 16 B 完全一致** → V5 表の数値はバイナリと矛盾しない。

### a.4 境界（int3 パディング）の実測

```
0x00500820 周辺（V5: CC前置 (1)）   0x50081D 59 (pop ecx) | 0x50081E c3 (ret) ← FUN_005007d0 の終端
                                     0x50081F cc                ← int3 1 B
                                     0x500820 b0 01 … c3        ← class3 本体 8 B
                                     0x500828..0x50082F cc×8    ← pad_after 8 B
                                     0x500830 80 3d …           ← FUN_00500830 開始

0x00501A60 周辺（V5: cc×7）          0x501A58 c3 (ret) ← FUN_00501a00 の終端 | 0x501A59..0x501A5F cc×7
                                     0x501A60..0x501A71 c3 (本体 18 B) | 0x501A72..0x501A7F cc×14 | 0x501A80 開始

0x005087A0 周辺（V5: cc×2）          0x50879D c3 (ret) ← FUN_00508700 の終端 | 0x50879E..0x50879F cc×2
                                     0x5087A0..0x5087B1 c3 (本体 18 B) | 0x5087B2..0x5087BF cc×14 | 0x5087C0 開始
```

- **直前に int3 あり**（1 / 7 / 2 B）→ 直前関数は `ret` で明示終了し、本体へ**フォールスルーしない**。
- **直後に int3 あり**（8 / 14 / 14 B）→ 本体は `ret` で完結し、後続関数へはみ出さない。
- 3 件とも **Ghidra 台帳（21,597）に独立関数として登録済み**（`func_list_new.txt` の前後 2 エントリが `prev/next` で挟む）。
- `docs/Oblivion_decompiled.c` には**未収載**（`dec_index` に該当 FUN なし = V5 の「欠落/吸収」側）。enclosing_fun 検証: 前ラベル行から見て 0x00500820 は `fun_00500650`(L198654) と `fun_00500d90`(L198679) の**間のギャップ**、0x00501a60 は `fun_00501a00`(L198826)/`fun_00501a80`(L198846)、0x005087a0 は `fun_005084a0`(L201238)/`fun_005087c0`(L201296)。

### a.5 オペコードテーブル `+0x18` 消費箇所の走査（根拠の補強）

テーブル基底 0xB0AF48 を絶対アンカーとして参照する .text 内の全サイトを、`(imm32 − base − field) % 0x28 == 0` かつ `0 ≤ idx < 532` で分類（= どのエントリのどのフィールドを読むかを全数確定）:

| フィールド | .text 内アンカー数 | 主な消費箇所 |
|---|---|---|
| +0x00 name | 18 | 0x004FF904 の `lea eax,[ebx+0xb0af48]`（imm32 は 0x004FF906、FUN_004ff710 の線形名前走査）ほかエントリ固定 17 |
| +0x04 alias | 12 | 0x004FF807（名前照合）、0x004FCABA 等 |
| +0x08 opid | 6 | 0x004FF892（バイトコードへ opid 書き込み）ほか |
| +0x10 flagA | 8 | **0x00516AA1**（実行時）ほか |
| +0x14 argdesc | 8 | **0x00516A48**（実行時）ほか |
| **+0x18 impl** | **2** | **①0x00516A5D（imm32 は 0x00516A5F・動的 idx）②0x005E8725（entry374 固定）** |
| +0x1C disp | 2 | 0x0052DE21 / 0x00552B7A |
| +0x20 cond | 4 | 0x0056ACA8 等 |
| +0x24 flag | 2 | 0x0060D90F / 0x007BF16A |

`+0x18` の動的読み取り箇所 **0x00516A5F**（`FUN_005169a0` @0x005169A0, decompiled.c L207897）:

```
00516A3E  lea eax,[ecx+ecx*4]              ; idx*5
00516A41  add eax,eax / add eax,eax        ; idx*20
00516A45  mov ecx,[eax+eax+0xb0af5c]       ; entry[idx]+0x14 argdesc  (0xb0af5c = base+0x14)
00516A4C  add eax,eax                      ; idx*40 = idx*0x28
00516A5D  mov ecx,dword ptr [eax+0xb0af60] ; entry[idx]+0x18 IMPL     (0xb0af60 = base+0x18)  ← ここ
00516A67  je 0x516a9f
00516A9F  cmp byte ptr [eax+0xb0af58],0    ; entry[idx]+0x10 flagA
00516AA6  je 0x516ad6
00516AD6  test ecx,ecx                     ; impl == 0 を明示チェック
00516AD8  je 0x516b2a                      ;   → スキップ（impl=0 の 5 エントリと整合）
00516AF3  call ecx                         ; ハンドラ本体を 8 引数で呼び出し
00516AF5  add esp,0x20
00516AF8  test al,al                       ; ハンドラの bool 戻り値を検査
```

→ **`+0x18` は「実行時に `call` されるハンドラポインタ」であることが逆アセンブルで直接確認できる**。これが class3 3 件の「最小実装（ハンドラ）」判定の column 意味論的根拠。

> **残点 R-a**: 上記 0x00516A5D の経路は直前に `cmp ecx,0x1e / ja 0x516acf`（→ `xor al,al; return 0`）があり **opid ≤ 0x1E（band1 = スクリプトイベント 31 種）に限定**される。class3 の opid（0x0152 / 0x011A / 0x0124 = band2 0x100–0x183）はこの経路を通り得ないため、**band2/band3 の実行時 `+0x18` 消費箇所は本タスクでは未同定**。絶対アンカー方式の走査では band2/band3 用アンカー（`base − 0xE1*0x28` / `base − 0xF5D*0x28` 系）は 0 件だった（後述 §e）。

### a.6 判定の根拠まとめ（機械規則）

| 観点 | 結果 | データ誤分割なら？ |
|---|---|---|
| `+0x18 impl` 参照 | 3/3 で **1 件ずつ**（他列 0） | テーブル参照は**無い**はず |
| .text 内の imm32 参照 | 3/3 で **0 件**（間接到達のみ） | データとして読まれるなら .data/.rdata に参照が残る |
| 直前の分離 | `ret` → int3（1/7/2 B） | 前本体から連続（int3 0 B かつ終端なし） |
| 直後の分離 | `ret` → int3（8/14/14 B） | 本体に終端が無い／後続に食い込む |
| 本体構造 | フラグ RMW + `mov al,1` + `ret`（3〜5 命令） | 不整合・継続不能な断片 |
| 台帳登録 | 21,597 に独立エントリ（前後が別 VA） | 1 エントリに含まれる |
| decompiled.c | 未収載（ギャップ内・V70 で decomp_ok=1） | — |
| V5 prologue hex | 先頭 16 B **完全一致** | — |

---

## §b 境界判例 9 件の格上げ判断

### b.1 結論

**9/9 とも [格上げ]（class2 → 完全関数）。[維持] は 0 件。**
V79 の class2 判定は V79 自身の形状閾値（規則3「CF無 かつ 呼び出し≤2 かつ exec≤3」/ 規則8「residual-thin」）に由来するもので、**断片性（前後ボディへの埋め込み・本文欠落・フォールスルー）を示す証拠は 9 件すべてで 0 件**。

### b.2 判定表

| # | VA | name | V79 cls/reason | L | exec/call/CF | decompiled 行 | 実体 B / insn | pad 前/後 | R1a | R1b | R2 (dec_call/ bin E8) | R3 直前/直後 骨格dup | R4 | **判定** |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 0x005062b0 | EnableLinkedPathPoints | 2 / residual-thin | 156 | 2 / 1 / **有** | decompiled.c **未収載**・V70 dump 15 行 | 22 / 9 | 0 / 10 | ✓ `ret`@0x5062AF（pad_before=0＝int3 は無し・ただし前本体が ret で明示終了） | ✓ | 1 / 1 **FULL** | F / F | 1件 idx461 0x112A | **[格上げ]** |
| 2 | 0x005062d0 | DisableLinkedPathPoints | 2 / residual-thin | 156 | 2 / 1 / 有 | 未収載・dump 15 行 | 22 / 9 | 10 / 10 | ✓ `ret`@0x5062C5 | ✓ | 1 / 1 **FULL** | F / F | 1件 idx462 0x112B | **[格上げ]** |
| 3 | 0x00508180 | ToggleRefractionDebug | 2 / residual-thin | 180 | 2 / 0 / 有 | 未収載・dump 15 行 | 45 / 11 | 4 / 3 | ✓ `jmp`@0x508177 | ✓ | 0 / 0 **FULL** | F / F | 1件 idx121 0x015A | **[格上げ]** |
| 4 | 0x005022d0 | GameMode | 2 / call+return-forwarder | 207 | 3 / 1 / 無 | 未収載・dump 19 行 | 29 / 12 | 11 / 3 | ✓ `ret`@0x5022C4 | ✓ | 1 / 1 **FULL** | F / F | 1件 idx0 0x0000 | **[格上げ]** |
| 5 | 0x00502370 | GetDisabled | 2 / call+return-forwarder | 236 | 2 / 1 / 無 | 未収載・dump 13 行 | 26 / 10 | 1 / 6 | ✓ `ret`@0x50236E | ✓ | 1 / 1 **FULL** | F / F | 1件 idx198 0x1023 | **[格上げ]** |
| 6 | 0x00502390 | GetLocked | 2 / call+return-forwarder | 236 | 2 / 1 / 無 | 未収載・dump 13 行 | 26 / 10 | 6 / 6 | ✓ `ret`@0x502389 | ✓ | 1 / 1 **FULL** | F / F | 1件 idx168 0x1005 | **[格上げ]** |
| 7 | 0x005023b0 | GetLockLevel | 2 / call+return-forwarder | 236 | 2 / 1 / 無 | 未収載・dump 13 行 | 26 / 10 | 6 / 6 | ✓ `ret`@0x5023A9 | ✓ | 1 / 1 **FULL** | F / F | 1件 idx228 0x1041 | **[格上げ]** |
| 8 | 0x00504010 | OnYield_unused | 2 / call+return-forwarder | 374 | 3 / 1 / 無 | 未収載・dump 21 行 | 62 / 23 | 11 / 2 | ✓ `ret`@0x504004 | ✓ | 1 / 1 **FULL** | F / F | 1件 idx7 0x0007 | **[格上げ]** |
| 9 | 0x0050baa0 | GetPCSleepHours | 2 / call+return-forwarder | 221 | 3 / 0 / 無 | 未収載・dump 19 行 | 20 / 6 | 7 / 12 | ✓ `ret`@0x50BA98 | ✓ | 0 / 0 **FULL** | F / F | 1件 idx351 0x10BC | **[格上げ]** |

`R3` の F/F = 厳格骨格で「直前ラベル本文」「直後ラベル本文」と**どちらも不一致**。
`R4` は 532 エントリ全走査で該当 VA を `+0x18` に持つエントリが各 1 件（`+0x1C`/`+0x20` は 0）。

### b.3 decompiled.c の前後ラベル（`enclosing_fun()` 検証）

| VA | 直前ラベル（行） | 直後ラベル（行） | gap（前/後, B） |
|---|---|---|---|
| 0x005062b0 | `fun_005061b0` (L200475) | `fun_00506310` (L200510) | +256 / +96 |
| 0x005062d0 | `fun_005061b0` (L200475) | `fun_00506310` (L200510) | +288 / +64 |
| 0x00508180 | `fun_00507e60` (L201165) | `fun_005084a0` (L201238) | +800 / +800 |
| 0x005022d0 | `fun_00502270` (L199077) | `fun_00502690` (L199097) | +96 / +960 |
| 0x00502370 | `fun_00502270` (L199077) | `fun_00502690` (L199097) | +256 / +800 |
| 0x00502390 | `fun_00502270` (L199077) | `fun_00502690` (L199097) | +288 / +768 |
| 0x005023b0 | `fun_00502270` (L199077) | `fun_00502690` (L199097) | +320 / +736 |
| 0x00504010 | `fun_00503c60` (L199891) | `fun_00504150` (L199917) | +944 / +320 |
| 0x0050baa0 | `fun_0050b3e0` (L202480) | `fun_0050bb70` (L202515) | +1728 / +208 |

いずれも **前後ラベルの本文は decompiled.c に実在**し、その本文と当該 VA の骨格は一致しない（R3 充足）。`v10_impl_decompile_join.csv` の `prev_fun`/`next_fun` は上表と完全一致。

### b.4 「前後ボディの重複」の内訳（厳格 vs 緩和）

- **厳格骨格**（住所のみ `@` 化）: 9 件とも**直前・直後とも不一致 = 重複 0 組**。
- **緩和骨格**（数値リテラルも正規化）: 9 件内に **2 組**のみ
  - `{0x005062b0, 0x005062d0}` — `Enable/DisableLinkedPathPoints` 対（`push 1` / `push 0` のみ相違、呼出先は同一 `0x4e51f0`）
  - `{0x00502370, 0x00502390, 0x005023b0}` — `GetDisabled/GetLocked/GetLockLevel` 3 件族（V79 は呼出先 helper が `0x004f60e0` / `0x004f4760` / `0x004f47b0` と一件ずつ別と記載）
- これらは「同じ型の定型ハンドラ族」であって、**片方が他方の断片（前関数に埋め込まれた残骸）ではない**。根拠: それぞれが独立の `ret` + int3 パディング境界を持ち、独立した `+0x18 impl` エントリ（1 件ずつ別の opid）としてテーブルから到達する。V79 §2.4 の「骨格重複 13 組 115 件・呼出先は一件ずつ全て別」との整合も取れる。

### b.5 V5 表の `CC前置` と本セッション実測の突合（`pad_before`）

| VA | V5 CC前置 | 実測 pad_before | 整合 |
|---|---|---|---|
| 0x005062b0 | (0) | 0 | ✓ |
| 0x005062d0 | cc×8 | 10 | ✓（飽和表記 ≥8） |
| 0x00508180 | cc×4 | 4 | ✓ |
| 0x005022d0 | cc×8 | 11 | ✓ |
| 0x00502370 | (1) | 1 | ✓ |
| 0x00502390 | cc×6 | 6 | ✓ |
| 0x005023b0 | cc×6 | 6 | ✓ |
| 0x00504010 | cc×8 | 11 | ✓ |
| 0x0050baa0 | cc×7 | 7 | ✓ |

`cc×8` は V79 が「≥8 の飽和表記」と解した通りで、実測 ≥8 と矛盾しない。
うち **6 件は実数が一致**（0 / 4 / 1 / 6 / 6 / 7）、**3 件（0x005062d0=10, 0x005022d0=11, 0x00504010=11）は飽和表記**のため V5 側に実数が無い。

### b.6 影響（V79 分類への波及）

- 本判定により V79 の class2 は 98 → **89 件**、class1 は 256 → **265 件**（class3 3 件は §a で「データ/無意味」ではなく
  「最小実装（ハンドラ）」と確定したため、class2 相当への扱い直しが別途必要 → §e **R-e**）。
- ただし機械規則 R1–R4 は残り class2 89 件にもほぼ同様に成立する見込み（V80 で class2+3 = 101/101 が `+0x18` 一致済み）ため、
  9 件だけを切り出すこと自体よりも **class2（薄片/断片）という呼称の再設計**が本質（§e **R-e**）。
- 本レポートは `v79_classified.csv` / V5 表を**書き換えない**（成果物は本 1 ファイルのみの制約、docs/・既存 tmp/re は編集禁止）。

---

## §c V5 表の重複 5 件

### c.1 結論

**5/5 とも「正当な二重登録（opid 多義＝オペコード表の `+0x18` 共有）」。データ事故ではない。**
V5 §3.1 は **オペコード表 532 エントリを走査した結果（行キー = 表 idx）**であり、行が VA をキーにしていないため、impl を共有する 2 エントリを持つ VA は 2 行に現れる。バイナリの実体と完全に一致する。

### c.2 判定表

| VA | V5 §3.1 の 2 行（行番号 = 表 idx / 名前 / CC前置 / 判定） | オペコード表 `+0x18`（idx / opid / name / alias / argn） | §4.1 一覧 | 実体 | **判定** |
|---|---|---|---|---|---|
| 0x005007d0 | L152 `112 ShowSubtitle` cc×3 欠落 / L417 `519 ShowDialogSubtitles` cc×3 欠落 | 112 / 0x0151 `ShowSubtitle` (alias 無) argn=1  ＋  519 / 0x1164 `ShowDialogSubtitles` argn=1 | L498 に **0x1164 のみ** | 本体 79 B / 30 insn / `ret`@0x50081E、pad_after 1 B（→0x500820）、pad_before 3 B | **正当（opid 多義）** |
| 0x00500890 | L156 `116 OutputMemContexts` cc×2 / L170 `136 ResetMemContexts` cc×2（いずれも欠落） | 116 / 0x0155 `OutputMemContexts` (omc) **argn=1**  ＋  136 / 0x0169 `ResetMemContexts` (rmc) **argn=0** | **該当なし** | 本体 323 B / 84 insn / `ret`@0x5009D2、pad_after 13 B、pad_before 2 B | **正当（opid 多義）** ※意味論は §e R-c |
| 0x00503e10 | L69 `4 OnEquip` cc×8 / L90 `28 OnActorEquip` cc×8（欠落） | 4 / 0x0004 `OnEquip` argn=1  ＋  28 / 0x001C `OnActorEquip` argn=1 | L442 に **0x001C のみ** | 本体 113 B / 51 insn / `ret`@0x503E80、pad_after 15 B、pad_before 12 B | **正当（opid 多義）** |
| 0x00503e90 | L70 `5 OnUnequip` cc×8 / L91 `29 OnActorUnequip` cc×8（欠落） | 5 / 0x0005 `OnUnequip` argn=1  ＋  29 / 0x001D `OnActorUnequip` argn=1 | L444 に **0x001D のみ** | 本体 113 B / 51 insn / `ret`@0x503F00、pad_after 15 B、pad_before 15 B | **正当（opid 多義）** |
| 0x0050d020 | L312 `375 AdvancePCSkill` cc×4 / L355 `434 ModPCSkill` cc×4（欠落） | 375 / 0x10D4 `AdvancePCSkill` (AdvSkill) argn=2  ＋  434 / 0x110F `ModPCSkill` (ModPCS) argn=2 | L478 に **0x110F のみ** | 本体 956 B / 277 insn / `ret`@0x50D3DB、pad_after 4 B、pad_before 4 B | **正当（opid 多義）** ※意味論は §e R-d |

- 5 件とも `+0x14 argdesc` / `+0x1C dispatcher` / `+0x20 cond` に**この VA を持つエントリは 0 件**（共有は `+0x18` のみ）。
- 5 件とも `.text imm32 = 0`、`.data imm32 = 2`（= 表の 2 エントリ）。
- `docs/Oblivion_decompiled.c` には**未収載**（2 行とも `in_decompile=no`）。
- 実体はいずれも **1 本の連続した本体**（データ断片の連なりではない）。`0x00503e10` と `0x00503e90` は台帳上で**隣接**（prev=0x00503e10 / next=0x00503f10）だが、本体 113 B 同士で、同一相対位置の即値 `push 2`（0x503E68）と `push 8`（0x503EE8）の差以外に違いがなく、/OPT:ICF による同一化ではなく **別オペコードの別ハンドラ**（同一本文ではない = exact_dup 0 組と整合）。

### c.3 原因の機械的再現（V5 側成果物 × バイナリ）

`v5_ghidra_gaps_report.md` §3「impl フィールド (+0x18) のうち `FUN_{impl:08X}` がデコンパイルに存在しないもの」を本セションで**完全再現**:

| 指標 | V5 の記載 | 本セッション実測 | 整合 |
|---|---|---|---|
| 非ゼロ impl 総数 | 527 / 532 | **527 / 532** | ✓ |
| デコンパイルに存在 | 165 | **165** | ✓ |
| ギャップ（欠落）行数 | 362 | **362** | ✓ |
| ギャップのユニーク VA | （362 行 = 357 unique + 重複 5） | **357**（`missing impl VA appearing >1` = 上記 5 VA が各 2 回のみ） | ✓ |
| §3.1 の行 idx 集合 | 362 行 | 362 行、`missing idx set == §3.1 idx set` → **True** | ✓ |
| `v10_impl_decompile_join.csv` | — | `in_decompile`: **no=362 / yes=165** | ✓（V5 と同値） |

→ **「impl を 2 つのオペコードエントリが共有している 5 VA」が V5 §3.1 に 2 行ずつ現れる**という因果が一対一で説明できる。行キーは **表 idx（`opid` 列とラベルされているが実体は 0 始まり表 index）**であり、VA 不是。

補足（V5 内部の表の性格差）:

- §3.1（全ギャップリスト）= **表 idx をキー** → 5 VA が 2 行（= 今回の重複）。
- §4.1「Impl Addr 一覧 (75件)」= `svm_495_miss_addrs` 由来の**別フィルタ 75 件** → 5 組のうち 4 組は 1 行のみ（0x005007d0→0x1164、0x00503e10→0x001C、0x00503e90→0x001D、0x0050d020→0x110F）、0x00500890 は 0 行。**これは欠落ではなく §4.1 の定義差**（75 件フィルタ）。
- オペコード表全体の「impl を複数エントリが共有する組」は **7 組**:

| impl | エントリ数 | gaplist（§3.1）に登録 | 名前 |
|---|---|---|---|
| 0x005007D0 | 2 | **✓ idx 112, 519** | ShowSubtitle / ShowDialogSubtitles |
| 0x00500890 | 2 | **✓ idx 116, 136** | OutputMemContexts / ResetMemContexts |
| 0x00502270 | 2 | ✗（impl が decompiled.c に**存在**） | MenuMode / MenuMode（opid 0x0001 と 0x1024） |
| 0x00503E10 | 2 | **✓ idx 4, 28** | OnEquip / OnActorEquip |
| 0x00503E90 | 2 | **✓ idx 5, 29** | OnUnequip / OnActorUnequip |
| 0x0050D020 | 2 | **✓ idx 375, 434** | AdvancePCSkill / ModPCSkill |
| 0x00977C50 | **7** | ✗（impl が decompiled.c に**存在**） | TestCode / BeginTrace / ShowNameMenu / TrapUpdate / AddAchievement / StartMasterFileSeekData / DumpMasterFileSeekData |

→ gaplist に載らない 2 組は「デコンパイルに存在」= V5 の欠落集合にそもそも入らない。**V5 の重複 5 行は 7 組の共有のうち「欠落側」に該当した 5 組と正確に一致**し、過不足がない。

### c.4 判定

| 問い | 判定 |
|---|---|
| 正当な二重登録（opid 多義）か | **正当**（5/5）。バイナリの `+0x18` が実際に 2 エントリで共有されており、V5 §3.1 の行キー（表 idx）が VA ではないための必然的な二重出力。 |
| データ事故か | **否**。V5 の数値（527 / 165 / 362 / 357）はバイナリから全数再現、prologue hex も 5 件とも実体と一致、§3.1 の idx 集合は missing idx 集合と完全一致。 |

---

## §d v79§6-1「全 21,597 関数本文重複照合」の可否検討

**方針 1 行**: 実施は**可** — ペア全挙（O(N²)）ではなく**骨格正規化 → ハッシュ・バケット（O(N)）**で行えば 1 段階で回るため、§6-1 は次タスクで着手可能。

概算（`v94_probe3.py` で**準備工程のみ実測**、照合は未実施）:

| 工程 | 実測/概算 |
|---|---|
| `dec_lines` 読込（1,066,496 行 / 28,995,463 B） | 0.00 s（遅延ロード込み・2 回目） |
| ヘッダ走査（`// FUN_xxx @ 0x` → 20,465 件） | **0.438 s** |
| 本文スライス + 骨格化 + blake2b(16B) ハッシュ（20,465 本 / 27,928,884 字） | **3.168 s** |
| **準備合計（＝解析コスト）** | **約 4.0 s** |
| V70 dump 357 本の追加 | +0.1 s 未満（既に `v79_bodies.json` 化済） |
| バケット集計（`Counter`） | +0.5 s 未満（概算） |
| **合計（ハッシュ方式）** | **約 5–10 s / メモリ < 1 GB**（dec_lines 29 MB ＋ 骨格文字列 28 MB ＋ オーバーヘッド） |
| 参考: ペア全挙 O(N²) | 20,465 本で **209,397,880 対**、21,597 本で **233,204,406 対** → Python 直列比較で**数時間〜十数時間**、**非推奨** |

注意点（実装時に）: 「本文完全一致」はハッシュ・バケットで足りるが、V79 の**骨格重複**（住所正規化）も同じ骨格キーで 1 回の走査で拾える。近傍（前後関数同士）だけを見る場合は 21,596 対のみで **数秒**。類似（near-dup）まで広げる場合は shingle + MinHash/LSH が必要でコストは 1 段階上がる（分・十分オーダー）。

---

## §e 残点（本タスクで決着しなかった事項）

| ID | 内容 | 影響 |
|---|---|---|
| **R-a** | `+0x18` の**動的**消費箇所 0x00516A5D（`FUN_005169a0`）は `cmp ecx,0x1e / ja` で **opid ≤ 0x1E（band1）限定**。class3 の band2 opid（0x0152/0x011A/0x0124）および §b の 9 件の多く（0x1023/0x1005/0x1041/0x10BC/0x112A/0x112B/0x015A）が**実行時にどの箇所から `call` されるかは未同定**。アンカー走査では band2/band3 用のアンカー（`base − 0xE1*0x28` = 0xB08C38 系、`base − 0xF5D*0x28` = 0xAE48D8 系）は **0 件**。残る `+0x18` アンカーは 0x005E8725（entry374 固定）の 1 箇所のみ。 | 「+0x18 = ハンドラ」の column 意味論は 0x00516A5D で証明済みのため §a 判定は成立。ただし**個別ハンドラへの実到達経路**は未証明。 |
| **R-b** | §b の R3 は V5/V10 の「直前/直後ラベル」＝decompiled.c 実在ラベルとの比較のみ。ギャップ同士（357 VA 内）の前後比較は V79 §2.4 の既出（exact_dup 0 組 / 骨格 13 組 115 件）を引用した。**ギャップ外 21,240 本との照合は §d 記載どおり未実施**。 | 格上げ判定の R3 は decompiled.c 側に限定。全域照合は §d。 |
| **R-c** | `0x00500890`（OutputMemContexts omc argn=1 / ResetMemContexts rmc argn=0）は**同一 impl を argn が異なる 2 コマンドで共有**し、本文は `s_OutputMemContexts_xls_00a4b818`（"OutputMemContexts.xls"）固定・`ResetMemContexts` 用の分岐が無い（本文 3,010 字を全確認）。 | 共有の**意味論**（rmc が omc と同じ動作になるのか、ゲート `FUN_004fae80` の引数差で分岐するのか）は未決。V5 の重複自体は正当（§c 判定は不変）。 |
| **R-d** | `0x0050d020`（AdvancePCSkill / ModPCSkill、両方 argn=2）は 4,222 字・956 B の大本体を共有。Advance と Mod の差分が本体のどこにあるか（差異コードの所在）未特定。 | 同上。共有の意味論のみ未決。 |
| **R-e** | §b で 9 件を [格上げ] とした機械規則（R1–R4）は **残り class2 89 件にも同様に適用可能**（V80 で 101/101 が `+0x18` 一致済み）。規則を適用するなら class2 はほぼ全数が「完全関数（＝薄いが自己完結）」になり、**class2 の呼称自体の再設計**が必要。本レポートは V79 の `v79_classified.csv` を書き換えない（成果物 1 ファイル制約）。 | V79 §5.2 の反映案（未適用・ユーザー判断待ち）と合わせて要判断。 |
| **R-f** | V5 §3.1 の第 1 列は見出し `opid` だが実体は**表 idx（0 始まり 532 エントリの index）**。opid 0x00–0x1E 帯では idx==opid のため表上は区別できない（例: 行「112」は表 idx=112 / opid=0x0151）。 | 表の見出しラベルの誤記。V5 は既存ファイルのため**編集していない**（禁止事項）。 |
| **R-g** | `cc×8` は「≥8 の飽和表記」と解して実測と突合した。§b の 9 件は **完全一致 6 / 飽和 3**（0x005062d0=10, 0x005022d0=11, 0x00504010=11 が V5 表記 `cc×8`）。§c の 5 件は **完全一致 3**（0x005007d0=3, 0x00500890=2, 0x0050d020=4）**/ 飽和 2**（0x00503e10=12, 0x00503e90=15 が `cc×8`）。飽和箇所は **V5 が実数を保持していない**ため厳密一致は確認不能。 | V5 の CC前置列は飽和値。厳密な再計算が必要なら §6-5 の通り probe_harness `rd()` を使う（.rdata末/.data 開始の −0x600 シフト既知バグは V77 で修正済）。 |

---

## §f 未実施（スコープ外）

- **v79§6-1**（ギャップ外 21,240 + 新規 357 = 21,597 本の本文・骨格重複照合）→ §d に可否と概算のみ記載。**照合は実施していない**。
- v79§6-4（フォワーダ族の呼出先 helper 役割マップ）、§6-5（cc前置のバイナリ再計算）、§6-7（`decomp_chars` と L の差）、§6-8（v10 `gap_kind` 呼称衝突）は本タスク対象外。
- `docs/`・既存 `tmp/re/*`・`TODO_re49.md`・`session-memory.json`・`plan.md` は**一切編集していない**。git 実行なし・memory 追加なし・sync ツール未実行。
