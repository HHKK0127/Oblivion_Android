# V92: シェーダ/レンダラ束3点（RefreshShaders 経路 / B42754 vtable 全スロット / b42ec0 実行時再初期化）

- 作成日: 2026-10-25 / 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam, 手動 PE パース・pefile なし)
- ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（本ノート作成前・作成後の両方で実行）
- 手法: probe_harness `rd()`（セクション対応: .text const=0x400C00 / .rdata=0x400E00 / .data=0x401400、bss は None+警告）+ capstone 5.0.7 全文リニア走査（chunked resync）+ バイトパターン分類 + E8 rel32 辺走査 + dec_index 20,465 / Ghidra 台帳 `func_list_new.txt` 21,597 突合
- 成果物は **本ノート 1 ファイルのみ**。docs/・既存 tmp/re・TODO・session-memory.json・plan.md は無変更、git / memory 追記 / sync なし
- 中間スクリプト（すべて `%TEMP%\opencode`・Git 管理外）: `v92_a_refresh.py` `v92_a2_deep.py` `v92_a3_edges.py` `v92_a4_misc.py` `v92_a5_ident.py` `v92_b0_rtti.py` `v92_b1_vtable.py` `v92_b2_path.py` `v92_b3_state.py` `v92_b4_self.py` `v92_b5_matrix.py` `v92_b6_statevt.py` `v92_c1_slots.py` `v92_c2_writers.py` `v92_c3_final.py` + 都度 python -c 巡査

判定表記: **[確定]** = バイナリ実測で一意 / **[仮説]** = 構造証拠十分で名칭・意味論が推定 / **[未特定]** = 残点。

---

## §a RefreshShaders (opid 0x133) → FUN_007B81A0 のシェーダ再生成経路 — **[確定]**

### a-1. オペコードテーブル entry（実測）

`0xB0AF48 + 82*0x28 = 0xB0BC18`（idx 82 = 0x133 - 0xE1、0x100-0x183 帯）:

| +off | 値 | 内容 |
|---|---|---|
| +0x00 | 0xA50DF8 | `"RefreshShaders"` |
| +0x04 | 0xA50DE8 | `"REFRESHSHADERS"`（alias） |
| +0x08 | 0x133 | opid |
| +0x0C | 0xA50DC8 | `"Reload HLSL shaders from disk"` |
| +0x10/+0x12/+0x14 | 0 / 0 / 0 | flags, 引数0個, argdesc 無し |
| **+0x18** | **0x50AA20** | impl = FUN_0050AA20 |
| +0x1C | 0x4FDE30 | 共有 dispatcher（既知: 0x4FF9B9 `call ebx` 経由） |
| +0x20/+0x24 | 0 / 0 | 条件関数なし |

注: V80 記載の「0xB0BC30 = entry82」は **+0x18 フィールドの絶対アドレス**（0xB0BC18+0x18）と整合。entry 先頭は 0xB0BC18。

### a-2. impl FUN_0050AA20 本体（0x50AA20..0x50AA3F、32B、台帳 FUN_0050aa20／dec_index に本文なし）

```
0x50AA20  push    esi
0x50AA21  mov     esi, [0xb42e8c]        ; 現 callback 保存 (raw site 0x50AA23 = R)
0x50AA27  mov     [0xb42e8c], 0x50aa10   ; ★一時差替え (raw site 0x50AA29 = W)
0x50AA31  call    0x7b81a0               ; ★impl → FUN_007B81A0 直接 E8（実測・唯一の呼出し）
0x50AA36  mov     [0xb42e8c], esi        ; ★復元 (raw site 0x50AA38 = W)
0x50AA3C  mov     al, 1
0x50AA3E  pop     esi / ret
```

- `E8 → 0x50AA20` は **0 件**（opcode dispatcher の間接呼出のみ、+0x1C=0x4FDE30 経由）。
- `E8 → 0x7B81A0` は **1 件のみ（0x50AA31）**。dec_search `"FUN_007b81a0("` = 0 件（本文不在）。
- **0x7B81A0 は台帳 21,597 にも dec_index 20,465 にも無い FUN ラベルなし関数** [確定]: 前ラベル FUN_007b8150 は 0x7B819F で ret（直前に別ブロック `... c3 | b0 01 eb da ... 5e c3`）、**int3 パッド 0 バイトで 0x7B81A0 = `push esi` が直後開始**（隣接 gap 型）、次ラベル FUN_007b8200 @0x7B8200。

### a-3. FUN_007B81A0 の再生成手順（35 命令、0x7B81A0..0x7B81F6、復元済み）

```c
for (edi = &DAT_00b42ec0; edi < &DAT_00b42f30; edi += 4) {   // 28 スロット全域
    slot = *edi;            if (!slot)  continue;
    child = *(slot + 4);    if (!child) continue;
    p = child->vtable[1](child);            // +0x04 メソッド
    found = false;
    while (p) { if (p == &DAT_00b4257c) { found = true; break; } p = *(p+4); }
    if (found) child->vtable[0x8c]();       // +0x8C (idx35) = 再ロード系フック
}
```

- 実装メモ: フォールバック分岐は `xor al,al / neg al / sbb eax,eax / and eax,esi` → 恒等 0（skip）。TRUE は **ret を跨ぐ split-block** `0x7B81F3: mov al,1; jmp 0x7b81d1`。ループ境界 `cmp edi, 0xb42f30 / jl` = 28 スロットちょうど。
- 同型ループの参照実装が FUN_007b8150（dec L672600-672625）: sentinel は `&DAT_00b42884`、見つかると `FUN_00560920(&DAT_00b4257c, obj)`（annotate_db 名 = NiControllerManager_FindSeq、368 名中の 1 つで**低信頼**）。
- センチネル 0xB4257C の参照は .text に 18 site / 4 関数（fun_0076c760 / fun_007b8150 / fun_00a0a290 / fun_00a10e50）。

### a-4. コールバック差替え（0xB42E8C）の全数 R/W とタイミング — **[確定]**

raw imm32 61 site をバイトパターン分類（`A1/A3/8B 05/89 05/C7 05/...` 判定、unresolved 0）:

| role | 件数 | site |
|---|---|---|
| **W (store)** | **3** | 0x40E8C3 = 既定セット `mov [0xb42e8c],0x405150`（fun_0040d4d0）/ 0x50AA29 = 差替 / 0x50AA38 = 復元 |
| R (load→間接 call) | 58 | fun_00852470 ×14、fun_008014e0 Shader_CompileVertex ×6、fun_00801c90 Shader_CompilePixel ×6、fun_008580e0 ×4、fun_008588e0 ×4、fun_0050a8a0 ×1、fun_00855e80/00856510/00856d60/00857750 各 ×2、他 16 関数各 ×1 |

- **既定セットは起動時 1 回のみ**（FUN_0040D4D0、caller: fun_00440c40 / fun_004455e0 / fun_0044e720 / fun_005bdcd0）。以降の W は RefreshShaders の差替/復元の 2 サイトだけ → **差替えウィンドウ = 0x50AA27〜0x50AA36（FUN_007B81A0 実行中の 15 バイト）**。
- 呼び出し形（fun_007b8200 0x7B835C 例）: `mov eax,[0xb42e8c]; test; lea edx,[buf]; push 0; push edx; call eax`（2 引数形、ctx が既定側）。

**既定 0x405150（台帳・dec 双方なしの無名関数）の機序**:
1. arg0 → `FUN_004DC270(msg)`（内部 call 0x660110 の lookup 系）→ **NULL なら即 return = 出力なし**。
2. 非 NULL なら `ctx->vtable[0xD4]()` で名前取得 → `sprintf(buf, "%s :: %s", name, msg)`（format 0xA2FC88、buf 0x104B スタック）。
3. 長さ ≥ 0xFF なら assert: `push 0x475; push 0xa2fcb8(".\Misc\Main.cpp"); push 0xa2fc94("Shader error string is too long."); call 0x6F7070`。
4. `FUN_004A7A60(buf)` → `FUN_004941A0`（スタック 0x32CC=13,004B、gate `[0xB34D90]!=0`、CR/LF 整形）= 出力先ログ系 **[仮説]**。

**一時差替先 0x50AA10（12B、台帳なしの無名関数）**: `mov eax,[esp+4]; push eax; call 0x579B60; pop ecx; ret`。
`FUN_00579B60` = コンソール出力（printf 風、E8 516 件: `"GetDistance >> %0.2f"` 等）。表示可否は `[console+0x31]` byte と `[0xB3B908]` byte でゲート → va_list 経由で整形し 0x585C90 へ **[確定(機序)] = Console_Print 系**。

**用途判定 [確定(機序) / 意図は仮説]**: 既定経路は「lookup 失敗 → 無出力」「成功 → 前付き + 0x104B assert」の重い経路。RefreshShaders 中は文脈 lookup が外れうる再コンパイル帯を、**無条件でコンソールへ生メッセージ出す素直な printer に差し替えてエラーの取りこぼしを防ぐ**（差替→実行→復元の単純スコープ）。

**Android 移植への示唆（1 段階）**:
1. 「単一グローバルのエラーコールバック ptr + 再生成中の一時差替え/復元」は移植コストが極小（関数 ptr 1 本 + スコープ保存/復元）。再読込中は logcat/インゲームコンソール直結、平常時は文脈付きリポーラにすれば V80 の `0xB42E8C` と同型。
2. 既定側の `Shader error string is too long.` assert（0x104B バッファ）は境界チェック+truncate に置換。モバイルでアサート中断は不適。
3. 「lookup 失敗で無出力」は Android ではフォールバック出力（生メッセージ）に統合すべき（現機序は黙杀しうる）。

### a-5. §a 残点

- child のクラス未特定 → vtable[1] / vtable[0x8C] の厳密なメソッド名が未同定（= FUN_007B4290 の各 factory 戻り型、§c と接続）。
- センチネル 0xB4257C の所有者（18 site / 4 関数、annotate 名は低信頼）。
- `FUN_004DC270` の lookup キー（メッセージ文字列か別キーか）、`FUN_004A7A60/004941A0` の出力先（ログファイル/OSM いずれか）**[未特定]**。
- opid 0x133 の実行時 dispatcher 走査（V83 の 0x4FF9B9 `call ebx` 既知機構に依拠、本タスクでは entry→impl→0x7B81A0 の直接 E8 のみ実測）。

---

## §b 0xB42754 の vtable 全スロット同定と突合 — **[確定]（vtable 同定・構築・+0xA00）/[確定(観測)]（呼出しスロット）**

### b-1. オブジェクト構築の証拠鎖（型情報の実測）

```
FUN_00498340 (0x498519) → FUN_0076BD40:
  0x76BD40 call FUN_00761DF0            ; Direct3DCreate9 wrapper (DAT_00B42154)
  0x76BD74 push 0xb00 / call 0x401F00   ; ★malloc(0xB00 = 2,816 B) = renderer オブジェクト確保
  0x76BD85 mov ecx,eax / call 0x769BE0  ; ctor: 0x769BFF `mov [esi], 0xa88ea4` (vtable 代入)
  0x76BDD8 mov ecx,esi / call 0x76A1C0  ; 15 引数初期化
     0x76A754 call 0x77BB00(renderer, &renderer[0x284], 1)   ; state 工場
     0x76A764 mov [ebp+0x8ac], eax                          ; +0x8AC = state ptr
     0x76A76A call 0x778F60                                 ; 後続セットアップ
        0x778F66 mov edi,[esi+0x8ac]
        0x778F73 call 0x772060 → ★DAT_00b42754 = renderer / +0x280→0xB42750 / +0x8AC→0xB42758
        0x778F79 mov [0xb42834], edi         ; 同一 state の sibling キャッシュ
        0x778F92 call state_vtable[+0x104] (this=state, 引数 0)
        0x778FBD malloc(0x20)+ctor 0x77F500 → 0xB428A8 shader program factory
                失敗時 0x60D0A0("Failed to create shader program factory!") (0xA8A798)
```

- clear 経路: `FUN_0076BED0`（renderer vtable[0] = deleting dtor）→ `FUN_0076B5D0`（0x76B5DF で vtable 再代入 0xA88EA4、0x76B5E7 で +0x8AC チェック、0x76B600 から state vtable+0xE0 を stage 分ループ）→ `FUN_00779010` → `FUN_00772060(0)`（0xB42754/0xB42750/0xB42758 を 0 化）+ `mov [0xb42834],0`。
- **確保サイズ 0xB00 = 2,816B** が +0xA00..+0xA3C 行列・+0xA94 member（0x76A788 `lea esi,[ebp+0xa94]`）等を包含し得ることを確認 [確定]。

### b-2. vtable 本体の同定 — **NiDX9Renderer vtable = 0xA88EA4（98 スロット）** [確定]

RTTI 復元: `vtable[-1] = COL 0xACC9D0` → pTypeDescriptor → `.?AVNiDX9Renderer@@`。`tmp/re/vtable_map.txt` の `0x00A88EA4 98` と完全一致（同ファイルの rtti_vtables.txt 行 1345 も同アドレス）。

**全 98 スロット表**（台帳 = func_list_new.txt の開始点完全一致のみラベル、**gap** = 台帳開始点なし）:

| # | +off | target | 台帳 |
|---|---|---|---|
| 0 | +0x000 | 0x76BED0 | FUN_0076bed0 |
| 1 | +0x004 | 0x769F00 | **gap** |
| 2 | +0x008 | 0x6F7070 | FUN_006f7070 |
| 3 | +0x00C | 0x6F7070 | FUN_006f7070 |
| 4 | +0x010 | 0x6F7070 | FUN_006f7070 |
| 5 | +0x014 | 0x6F7070 | FUN_006f7070 |
| 6 | +0x018 | 0x700640 | **gap** |
| 7 | +0x01C | 0x7008A0 | FUN_007008a0 |
| 8 | +0x020 | 0x68F970 | FUN_0068f970 |
| 9 | +0x024 | 0x700650 | FUN_00700650 |
| 10 | +0x028 | 0x68F970 | FUN_0068f970 |
| 11 | +0x02C | 0x700670 | FUN_00700670 |
| 12 | +0x030 | 0x701A30 | FUN_00701a30 |
| 13 | +0x034 | 0x68F970 | FUN_0068f970 |
| 14 | +0x038 | 0x700750 | FUN_00700750 |
| 15 | +0x03C | 0x68F970 | FUN_0068f970 |
| 16 | +0x040 | 0x6F7070 | FUN_006f7070 |
| 17 | +0x044 | 0x6F7070 | FUN_006f7070 |
| 18 | +0x048 | 0x68F970 | FUN_0068f970 |
| 19 | +0x04C | 0x6FDFF0 | **gap** |
| 20 | +0x050 | 0x762480 | **gap** |
| 21 | +0x054 | 0x769F20 | **gap** |
| 22 | +0x058 | 0x769F70 | **gap** |
| 23 | +0x05C | 0x769F80 | **gap** |
| 24 | +0x060 | 0x76A040 | FUN_0076a040 |
| 25 | +0x064 | 0x769F90 | FUN_00769f90 |
| 26 | +0x068 | 0x76A110 | **gap** |
| 27 | +0x06C | 0x76A170 | **gap** |
| 28 | +0x070 | 0x76A180 | **gap** |
| 29 | +0x074 | 0x764A20 | **gap** |
| 30 | +0x078 | 0x764BB0 | **gap** |
| 31 | +0x07C | 0x762590 | **gap** |
| 32 | +0x080 | 0x769F60 | **gap** |
| 33 | +0x084 | 0x769F10 | **gap** |
| 34 | +0x088 | 0x7625A0 | **gap** |
| 35 | +0x08C | 0x7625B0 | **gap** |
| 36 | +0x090 | 0x762550 | **gap** |
| 37 | +0x094 | 0x764770 | FUN_00764770 |
| 38 | +0x098 | 0x701610 | **gap** |
| 39 | +0x09C | 0x762570 | **gap** |
| 40 | +0x0A0 | 0x762580 | **gap** |
| 41 | +0x0A4 | 0x7625C0 | **gap** |
| 42 | +0x0A8 | 0x7625E0 | **gap** |
| 43 | +0x0AC | 0x7625F0 | **gap** |
| 44 | +0x0B0 | 0x768E90 | FUN_00768e90 |
| 45 | +0x0B4 | 0x767860 | FUN_00767860 |
| 46 | +0x0B8 | 0x763010 | **gap** |
| 47 | +0x0BC | 0x767810 | FUN_00767810 |
| 48 | +0x0C0 | 0x763020 | **gap** |
| 49 | +0x0C4 | 0x767990 | FUN_00767990 |
| 50 | +0x0C8 | 0x763050 | FUN_00763050 |
| 51 | +0x0CC | 0x765290 | **gap** |
| 52 | +0x0D0 | 0x7625D0 | **gap** |
| 53 | +0x0D4 | 0x764E50 | **gap** |
| 54 | +0x0D8 | 0x764FE0 | FUN_00764fe0 |
| 55 | +0x0DC | 0x765120 | FUN_00765120 |
| 56 | +0x0E0 | 0x769F30 | **gap** |
| 57 | +0x0E4 | 0x769F40 | **gap** |
| 58 | +0x0E8 | 0x76A190 | **gap** |
| 59 | +0x0EC | 0x76A1A0 | **gap** |
| 60 | +0x0F0 | 0x725FA0 | **gap** |
| 61 | +0x0F4 | 0x60CF60 | FUN_0060cf60 |
| 62 | +0x0F8 | 0x60CF60 | FUN_0060cf60 |
| 63 | +0x0FC | 0x4348A0 | **gap** |
| 64 | +0x100 | 0x6FDFF0 | **gap** |
| 65 | +0x104 | 0x763560 | **gap** |
| 66 | +0x108 | 0x768700 | **gap** |
| 67 | +0x10C | 0x7635C0 | FUN_007635c0 |
| 68 | +0x110 | 0x768740 | **gap** |
| 69 | +0x114 | 0x768780 | FUN_00768780 |
| 70 | +0x118 | 0x68F970 | FUN_0068f970 |
| 71 | +0x11C | 0x7687D0 | **gap** |
| 72 | +0x120 | 0x763620 | FUN_00763620 |
| 73 | +0x124 | 0x7636F0 | FUN_007636f0 |
| 74 | +0x128 | 0x60D0A0 | FUN_0060d0a0（ログ/報告ヘルパ） |
| 75 | +0x12C | 0x60D0A0 | FUN_0060d0a0 |
| 76 | +0x130 | 0x76BE00 | **gap** |
| 77 | +0x134 | 0x762600 | FUN_00762600 |
| 78 | +0x138 | 0x769340 | **gap** |
| 79 | +0x13C | 0x764C70 | FUN_00764c70 |
| 80 | +0x140 | 0x762640 | FUN_00762640 |
| 81 | +0x144 | 0x762BE0 | FUN_00762be0 |
| 82 | +0x148 | 0x7647E0 | **gap** |
| 83 | +0x14C | 0x768D60 | FUN_00768d60 |
| 84 | +0x150 | 0x762FB0 | **gap** |
| 85 | +0x154 | 0x766F30 | FUN_00766f30 |
| 86 | +0x158 | 0x76B310 | **gap** |
| 87 | +0x15C | 0x76B310 | **gap** |
| 88 | +0x160 | 0x7693E0 | FUN_007693e0 |
| 89 | +0x164 | 0x7693E0 | FUN_007693e0 |
| 90 | +0x168 | 0x7694F0 | FUN_007694f0 |
| 91 | +0x16C | 0x769700 | FUN_00769700 |
| 92 | +0x170 | 0x765990 | **gap** |
| 93 | +0x174 | 0x766880 | FUN_00766880 |
| 94 | +0x178 | 0x76B160 | FUN_0076b160 |
| 95 | +0x17C | 0x768830 | **gap** |
| 96 | +0x180 | 0x768850 | **gap** |
| 97 | +0x184 | 0x768870 | **gap** |

**台帳突合: 48/98 が開始点一致、50 が gap**。gap の中核帯は ctor の ret @0x769EF1 直後の **int3 パッド（14B）+ tiny getter クラスタ 0x769F00/0x769F10/0x769F20/0x769F60/0x769F70/0x769F80**（バイト列実測: `... 5d c3 cc*14 b8 68 21 b4 00 c3 ... 8a 81 90 0a 00 00 c3 ... 8b 81 e0 05 00 00 c3 ...`）。0x6F7070（slot2-5/16/17）は `(0, ".\Misc\Main.cpp", line)` 形の報告スタブ、0x68F970（slot8/10/13/15/18/70）は共有スタブ **[仮説]**。

### b-3. B42754 の vtable 経由「直接呼出し」全数（[0xB42754] 参照 10 site 全走査）

| site | 関数 | 内容 |
|---|---|---|
| 0x7716F0, 0x771705 | FUN_007716E0 | **renderer vtable +0x54 を ×2 呼出し**（引数 0、戻り al を `test al,8` / `test al,4`） |
| 0x77184C, 0x771A3B, 0x771BF5, 0x771D94, 0x771EDE | FUN_00771830 | データ読みのみ（+0xA00 行列、vtable 呼出なし） |
| 0x772067, 0x772099 | FUN_00772060 | setter 本体 |
| 0x772235 | FUN_007721E0 | `[renderer+0x8B8]` の vtable **+0x08** 呼出し（別オブジェクトの vtable） |

- **+0x54 の実体 = 0x769F20（gap）**: `mov eax, dword ptr [ecx+0x5e0]; ret` → **renderer オブジェクトの +0x5E0 dword（flags）を返す getter**。FUN_007716E0 は bit8/bit4 で分岐 → V80 の「能力ビット &8/&4」は **+0x5E0 フラグ読み**と確定（bit 名は **[未特定]**）。
- +0x8B8 メンバ: FUN_007721E0 が `mov ecx,[0xb42754]; mov ecx,[ecx+0x8b8]; mov edx,[ecx]; ... call [edx+8]`（4 引数、リソース解決系 **[仮説]**）。
- renderer vtable 内部のセルフ呼出候補（98 エントリ本体を this 起点レジスタ追跡で走査）: +0x10 / +0x44 / +0x48 / +0x9C / +0xBC / +0xFC / +0x12C / +0x148。ただし 0x6F7070 等の cdecl スタブ混入により**誤検の可能性あり [要再検証]**。

### b-4. +0x8AC の「描画ステート群」= NiDX9RenderState（新確定）とそのスロット

- 工場 `FUN_0077BB00(renderer, &renderer[0x284], 1)`: **malloc(0x1148 = 4,424B)** → member ctor 0x7802D0(renderer) → `mov [esi], 0xa8a9f4` → `[esi+0x1018]` に 0x130B 領域 → `vtable[+0x108]` 呼出し。
- **state vtable = 0xA8A9F4、RTTI = `.?AVNiDX9RenderState@@`、67 コードスロット**（COL 0xACD6DC）。台帳一致 13/67、gap 54。
- キャッシュ: `0xB42758`（setter が +0x8AC を転記）、`0xB42834`（FUN_00778F60 が同一 ptr を保存）。**V80 の「+0xC8/0xD0/0xE0/0x104 はこの state オブジェクトの vtable」は RTTI で裏付け**。
- state オブジェクト側の部分（全 67 スロット表は末尾付録 A）:

| +off | target | 実測の呼び出し形（引数=push 順、this=ecx=state） | 呼出し元（主要） |
|---|---|---|---|
| +0x64 | 0x77B000 (FUN_0077b000) | 3 引数 (idx, val, 0)。cache `this+0x120+idx*8` → 変化時 `[this+0xFF8]` の vtable **+0xE4** へ転送 | FUN_007716E0 ループ ×8 |
| +0xC8 | 0x77B140 (FUN_0077b140) | 4 引数 (obj, stage, val, 0) 〜 5 引数。remap word 表 **0xB427E0**（<8 ゲート）→ cache `this+0x920+…` | FUN_00771790 ×3, FUN_007721E0, FUN_00772150, FUN_007732B0 ×2 等 |
| +0xD0 | 0x77B610 (FUN_0077b610) | 4〜5 引数。remap word 表 **0xB427B0**（<5 ゲート）→ cache `this+(…+0x1a4…)*8` | FUN_007716E0 ×2, FUN_007721E0, FUN_00773330 ×2 等 |
| +0xE0 | 0x77B280 (**gap**) | 2 引数 (idx, val)。cache `this+0xFA0+idx*4` → 変化時 `[this+0xFF8]` の vtable **+0x104** へ転送 | FUN_00771790 ×2, FUN_007721E0, FUN_0076B5D0(dtor ループ) |
| +0x104 | 0x77B7B0 (FUN_0077b7b0) | 引数 0。自 vtable +0x54 → +0x6C → +0xB8 … 連鎖 = **リセット系合成** | FUN_00778F60（初期化） |
| +0x108 | 0x77BA10 (FUN_0077ba10) | 引数 0 | FUN_0077BB00（工場内） |
| +0x74 | 0x77B730 (FUN_0077b730) | 自 +0xA0 → tail jmp 自 +0xB0（合成） | — |

- `[this+0xFF8]` = 転送先 inner オブジェクト（+0xE4/+0x104 を使用）。正体（DeviceWrapper 0xBA7D98 と同一か否か）は **[未特定]**。
- 既知と report の突合: state vtable **+0x4C = 0x77AFE0 ≠ 0x8A9E20**（RE_EXE_REPORT §62.1/§65.1 が SetTransform 実体とするアドレス）→ **NiDX9RenderState と DeviceWrapper(0xBA7D98) は別 vtable の別オブジェクト** [確定]。

### b-5. +0xA00 メンバ（4x4 行列）の位置付け — **[確定]**

- **位置**: renderer オブジェクト（確保 0xB00B）内の **+0xA00..+0xA3C = float 16 語**。近傍: +0x8AC（state ptr）/ +0x8B8（別 obj）/ 0xA94（別 member、0x76A788）より後、0xB00 末尾より前。
- **書手（.text 全域で disp 0xA00..0xA3C の store を全走査、58 site 中 renderer メンバは以下のみ）**:
  - **slot +0x140 = FUN_00762640**: `fstp [esi+0xA00] … [esi+0xA3C]` 16 語連続（0x76278A..0x762805）。
  - **slot +0x144 = FUN_00762BE0**: 16 語の `fst/fstp`（0x762CC1..0x762D29）。
  - 他はすべて `[esp+0xAxx]` スタック局部（無関係）。
- **読手 = FUN_00771830**: `switch (arg0-1 ∈ 0..5)`（jump table は **.text 内 0x772024**）。case1 @0x77184A は `[renderer+0xA00..0xA28] × arg行列 [eax+0..0x2C]` を fmul/fadd し **scratch グローバル 0xB42760..0xB42788+** へ `fstp`。caller FUN_00772150 は結果 `&0xB42760` を FUN_007721E0 経由で child vtable +0xB0 へ渡す。
- 判定: **renderer の現在トランスフォーム（ビュー相当）4x4 を slot+0x140/+0x144 が更新し、FUN_00771830 が任意行列と合成して scratch へ出力する** [確定(機序)]。名前（World/View いずれか）は **[仮説]**。

### b-6. 「RE_EXE_REPORT の NiDX9Renderer スロット表」との突合結果

1. **RE_EXE_REPORT.md に "NiDX9Renderer スロット表" は実在しない**（`NiDX9Renderer` / `NiRenderer` / `A88EA4` の grep すべて 0 件）→ 前提資料の記述は訂正を要する。
2. 報告書に実在するスロット表は 3 種のみ: §61.1「D3D9 vtable 呼出 16 種」/ §62.1 DeviceWrapper(0xBA7D98) 表 / §65.1 DeviceWrapper 既知メソッド表 — **いずれも DeviceWrapper 用**で b-4 の実測（+0x4C 不一致）により NiDX9RenderState と別物。
3. **既知/新規の整理**（既知 = V80 §70.15(5) の言及、それ以外はすべて新規）:

| スロット | 受け手 vtable | 旧記載 | 今回実体 | 判定 |
|---|---|---|---|---|
| renderer +0x54 | NiDX9Renderer 0xA88EA4 | 「能力ビット &8/&4 (FUN_007716E0)」 | 0x769F20 = `mov eax,[ecx+0x5E0]; ret`（flags getter、gap 関数） | **既知言及→実体新同定 [確定]** |
| state +0xC8 | NiDX9RenderState 0xA8A9F4 | 「RenderState/TextureStage 設定系」 | 0x77B140（remap 0xB427E0 + cache + 転送） | 既知言及→実体新同定 [確定] |
| state +0xD0 | 同上 | 同上 | 0x77B610（remap 0xB427B0 + cache） | 同上 |
| state +0xE0 | 同上 | 同上 | 0x77B280（cache → inner +0x104 転送） | 同上 |
| state +0x104 | 同上 | 同上 | 0x77B7B0（+0x54/+0x6C/+0xB8 連鎖リセット） | 同上 |
| state +0x64 / +0x108 / +0x74 等 | 同上 | 記載なし | b-4 表のとおり | **新規** |
| renderer +0x000/+0x130/+0x140/+0x144 等 残り 97 | NiDX9Renderer | 記載なし | b-2 全表 | **新規（98 スロット全列挙）** |
| +0xA00 メンバ | （データ） | 「4x4 float 行列 [V80]」 | writer=+0x140/+0x144、reader=FUN_00771830、位置=0xA00..0xA3C/obj 0xB00 | 既知言及→書読手新同定 [確定] |

### b-7. §b 残点

- renderer gap 50 / state gap 54 の個別ラベル化（0x769F00-0x769F80 クラスタ等、tiny getter は実測だが台帳未登録）。
- +0x5E0 flags の各ビット意味（&8/&4 の名称）**[未特定]**。
- `[state+0xFF8]` inner オブジェクトの正体（DeviceWrapper 同一説は未検証）**[未特定]**。
- renderer vtable 内部セルフ呼出リストは誤検含み **[要再検証]**。
- slot +0x140/+0x144 の呼び出し元（行列更新の触发条件）未走査 **[未特定]**。

---

## §c b42ec0..b42f2c（28 スロット）の実行時再初期化経路 — **[確定]**

### c-1. 走査手法（網羅性の保証）

1. **絶対アドレス形**（`[0xB42EExx]`, base=0）: capstone 2,258,077 命令を chunked resync（0x8000 + last_end 再開）で全文走査。※`op.mem.base is None` 判定だと base==0 を逃れて**全漏れ**する（初版で実際に発生→修正）。
2. **インデックス形**（`[reg*4 + 0xB42EC0]`）: disp は常に 0xB42EC0 なので **0xB42EC0 の raw imm32 全 37 site** をアライメント照合で全会計: memW 31 / memR 3 / cmp R 1 / imm(V) 2 = 37。
3. 上記を 28 スロット分で合算: **raw 66 site = memW 59 / memR 4 / cmp 1 / imm 2、unresolved 0**（= 絶対形 W29+R4 + インデックス形 W30+R3 の内訳と整合）。

### c-2. 書き手全数（W = 59 site、関数 4）

| writer | 形式 / site | 書く範囲 | 実行時期（呼出し証拠） | 判定 |
|---|---|---|---|---|
| **FUN_007b7250** | 絶対 store ×28（0x7B725A..0x7B72E1、5B 刻み） | 全 28 を 0 化（+既定セット群） | **起動時のみ**: E8 = 1 件 = CRT 初期化スタブ 0xA10EB5（.CRT$XCU 相当 0xA2EFF0 エントリ、V81 §2 済みを本タスクで再確認） | [確定] |
| **FUN_007B4290** | **インデックス形** switch（0x7B42C5 `mov [esi*4+0xb42ec0], eax`、case 0x00..0x1B）+ null チェック `cmp [esi*4+0xb42ec0], ecx` + clear path（dec L670254） | 1 スロット/回（i ∈ 0..27） | **実行時（遅延確保）**: E8 **40 件**（FUN_0040d4d0, FUN_00499570, FUN_0049b930, FUN_0049d7b0 ×3, FUN_004d0190, FUN_004ed1b0, FUN_00551140, FUN_005610c0 ×2, FUN_0056cff0, FUN_0057f170, FUN_007a9cc0/9cd0, FUN_007aa130, FUN_007aaa30, FUN_007ac9a0, FUN_007ad270 … + 自再帰 case4）。ゲート: case7 `b42f48<2`、case8 `b43070!=0`、case0x1A `b42ea5!=0` | **[確定] 実行時再初期化経路** |
| **FUN_007B8400** | 絶対（read 0x7B8400 / **W 0x7B8410**） | slot[9] = 0xB42EE4 のみ。`if (slot==0) slot = FUN_007fa910(); if (!slot->child) assert(0,".\BSShaderManager.cpp",0xCA4); FUN_007f9d10(child,0)` | **初期化時 1 回**: E8 = 1 件 = 0x40D6F1（FUN_0040D4D0） | [確定] |
| **FUN_007B8530（台帳なし・無名関数）** | **インデックス形**（read 0x7B8560/0x7B8585、**W 0x7B85A0** `mov [esi*4+0xb42ec0], ebx(=0)`）、esi 0..0x1C ループ | **全 28 を free + 0 化**（各 slot: refcount 検査 `*(child+4) > 1` → `"Shader %i is leaking.  There are %i references to it on shutdown."`（0xA8F508, 0x4A7A60 経由）→ `FUN_007b7170(slot)` → `free(slot)`（0x401F20））、後続で 0xB430EC/DC/F0 等の singleton release | **実行時（致命的レンダエラー時）**: E8 = 1 件 = 0x497B56 ← FUN_00497B20(msg) ← FUN_00497E70 / FUN_004980D0（`"Failed to Recreate Gamebryo Render in desired dimensions."` / `"Windowed mode not supported on Adapter."` / `"Desired render mode not found on Adapter."`） | [確定] |

### c-3. 読み手・走査系

| 関数 | 動き | 呼出し |
|---|---|---|
| FUN_007B81A0（無名） | `mov edi, 0xb42ec0`（imm, 0x7B81A2）で 28 全走査、読むだけ | E8 1 件 = 0x50AA31（RefreshShaders、§a） |
| FUN_007B84E0 | `mov esi, 0xb42ec0`（imm, 0x7B84E1）で 28 全走査: `if (slot && child) { r = child->vtable[0x1C](); if (r != -1) child->vtable[0x90](); }`（dec L672791-672806） | E8 4 件 = 0x43FC6A (FUN_0043fc20), 0x4456E5 (FUN_004455e0), 0x4DB391 (FUN_004db360), 0x7BA334 |
| FUN_007B4290 | null チェック + 既存返却（default: `return slot`） | 自身 |
| FUN_007B8530 | free 前 refcount 検査（R @0x7B8560/0x7B8585） | 自身 |

### c-4. 役割分担の判定 vs 0x7B8530 — **[確定]**

| 経路 | 役割 | 時期 |
|---|---|---|
| FUN_007b7250 | **起動時の既定化クリア**（28 全 0 + 既定値セット、CRT スタブ経由 1 回のみ） | プロセス起動（main 前） |
| **FUN_007B4290** | **実行時の（再）構築 = 遅延確保**（空きスロットを factory で生成して格納、shader level / HDR / flag ゲート付き、既存があればそのまま返す） | 実行時・初回アクセス時 |
| FUN_007B8400 | slot[9] の個別遅延生成（BSShaderManager 系） | 初期化時（FUN_0040D4D0） |
| **FUN_007B8530** | **実行時の破棄**（refcount 検査付き free + 0 化 → b430xx singleton release） | 実行時・致命的レンダエラー時（FUN_00497B20 経由） |

- **V81 §7-6 の問い（「FUN_007b7250 以外に書く手がいないか」）への回答: 3 本ある** = FUN_007B4290（実行時・主経路）/ FUN_007B8400（slot9 のみ）/ FUN_007B8530（無名・実行時破棄）。**[確定]**
- **V80 の「0x7B8530 = シェーダ系シングルトン一括リセット」は拡大訂正**: b430xx シングルトンだけでなく **シェーダテーブル 28 スロット本体を free + 0 化する**（leak 警告付き）。**[確定]**
- 役割分担: **7250（起動クリア）/ 78530（実行時破棄）/ 74290（実行時再構築）の 3 点セット**。78530 → 74290 の直列（破棄後に同じ呼び出しで再構築されるか）は callers の性質（致命エラー→終了系）から **次のアクセス時に 74290 が再構築する設計と推定 [仮説]**。
- 0x7B8530 の arg0 分岐（`cmp byte [esp+0xc], bl; je 0x7b88d9`）: FUN_00497B20 は msg 文字列ポインタをそのまま渡している（byte は非ゼロ）→ 真値時がテーブル破棄ループ。**`je 0x7b88d9`（arg0=0）側の意味は未精査 [未特定]**。

### c-5. §c 残点

- FUN_007B4290 の各 factory（FUN_007cbdc0 / 007e9840 / 008122a0 / 00810b90 / 0080ee10 / 007f1410 / 007b1070 / 007eae80 / 007c0b80 / 007fc7d0 / 007eee40 / 007b1f20 …）が返す 28 種オブジェクトのクラス名対応表 **[未特定]**（= §a の child クラス特定と同一問題）。
- case 0（param_1=0）の factory と、case 0x1A の gate `b42ea5` の意味論 **[未特定]**。
- 0x7B8530 の arg0=0 分岐（0x7B88d9 以降）の完全な処理内容は未走査 **[未特定]**。
- FUN_007B4290 の clear path（dec L670254 `slot=0; return 0`）の触发条件（gate 失敗時のみか）は decompiled 上 `break` 直後と判明済みだが、どのケースが実行時 gate 失敗するかの動的検証は未実施 **[記録のみ]**。

---

## 付録 A: NiDX9RenderState vtable 0xA8A9F4 全 67 スロット

| # | +off | target | 台帳 |
|---|---|---|---|
| 0 | +0x000 | 0x77BAD0 | FUN_0077bad0 |
| 1 | +0x004 | 0x780840 | **gap** |
| 2 | +0x008 | 0x77F8B0 | **gap** |
| 3 | +0x00C | 0x77F970 | **gap** |
| 4 | +0x010 | 0x77F990 | FUN_0077f990 |
| 5 | +0x014 | 0x77FBD0 | **gap** |
| 6 | +0x018 | 0x77FC00 | **gap** |
| 7 | +0x01C | 0x77FC30 | **gap** |
| 8 | +0x020 | 0x77FC50 | **gap** |
| 9 | +0x024 | 0x77FD50 | **gap** |
| 10 | +0x028 | 0x77FD70 | **gap** |
| 11 | +0x02C | 0x77BAC0 | **gap** |
| 12 | +0x030 | 0x77AF50 | **gap** |
| 13 | +0x034 | 0x77FDE0 | **gap** |
| 14 | +0x038 | 0x68F970 | FUN_0068f970 |
| 15 | +0x03C | 0x629290 | **gap** |
| 16 | +0x040 | 0x77AF70 | **gap** |
| 17 | +0x044 | 0x77AFB0 | **gap** |
| 18 | +0x048 | 0x77AFD0 | **gap** |
| 19 | +0x04C | 0x77AFE0 | **gap** |
| 20 | +0x050 | 0x77FE40 | **gap** |
| 21 | +0x054 | 0x77FE50 | **gap** |
| 22 | +0x058 | 0x77FEA0 | **gap** |
| 23 | +0x05C | 0x77B060 | **gap** |
| 24 | +0x060 | 0x77FEC0 | FUN_0077fec0 |
| 25 | +0x064 | 0x77B000 | FUN_0077b000 |
| 26 | +0x068 | 0x77B050 | **gap** |
| 27 | +0x06C | 0x77B6E0 | **gap** |
| 28 | +0x070 | 0x77B710 | **gap** |
| 29 | +0x074 | 0x77B730 | FUN_0077b730 |
| 30 | +0x078 | 0x77B080 | **gap** |
| 31 | +0x07C | 0x77B0C0 | **gap** |
| 32 | +0x080 | 0x77B0D0 | **gap** |
| 33 | +0x084 | 0x77B0F0 | **gap** |
| 34 | +0x088 | 0x77B350 | **gap** |
| 35 | +0x08C | 0x77B390 | **gap** |
| 36 | +0x090 | 0x77B3A0 | **gap** |
| 37 | +0x094 | 0x77B3C0 | **gap** |
| 38 | +0x098 | 0x77B410 | **gap** |
| 39 | +0x09C | 0x77B470 | **gap** |
| 40 | +0x0A0 | 0x77B490 | **gap** |
| 41 | +0x0A4 | 0x77B4B0 | **gap** |
| 42 | +0x0A8 | 0x77B510 | **gap** |
| 43 | +0x0AC | 0x77B570 | **gap** |
| 44 | +0x0B0 | 0x77B590 | **gap** |
| 45 | +0x0B4 | 0x77B5B0 | **gap** |
| 46 | +0x0B8 | 0x77FF40 | FUN_0077ff40 |
| 47 | +0x0BC | 0x780060 | **gap** |
| 48 | +0x0C0 | 0x77B1D0 | **gap** |
| 49 | +0x0C4 | 0x7800E0 | **gap** |
| 50 | +0x0C8 | 0x77B140 | FUN_0077b140 |
| 51 | +0x0CC | 0x77B1A0 | **gap** |
| 52 | +0x0D0 | 0x77B610 | FUN_0077b610 |
| 53 | +0x0D4 | 0x77B670 | **gap** |
| 54 | +0x0D8 | 0x77B6A0 | FUN_0077b6a0 |
| 55 | +0x0DC | 0x77B210 | **gap** |
| 56 | +0x0E0 | 0x77B280 | **gap** |
| 57 | +0x0E4 | 0x77B2B0 | **gap** |
| 58 | +0x0E8 | 0x77B2C0 | **gap** |
| 59 | +0x0EC | 0x77B750 | FUN_0077b750 |
| 60 | +0x0F0 | 0x77B7A0 | **gap** |
| 61 | +0x0F4 | 0x77B310 | **gap** |
| 62 | +0x0F8 | 0x77B320 | **gap** |
| 63 | +0x0FC | 0x77B330 | **gap** |
| 64 | +0x100 | 0x77B340 | **gap** |
| 65 | +0x104 | 0x77B7B0 | FUN_0077b7b0 |
| 66 | +0x108 | 0x77BA10 | FUN_0077ba10 |

（67 帯は +0x10C が非コード = COL 0xACD6DC で終端）

## 付録 B: 既知グローバルの本ノート分の確定値

| VA | 本ノートでの確定内容 |
|---|---|
| 0xB42E8C | シェーダエラー callback ptr。W = 3 site のみ（既定セット 0x40E8C1 / RefreshShaders 差替 0x50AA29 / 復元 0x50AA38）、他 58 は読手 |
| 0xB42754 | NiDX9Renderer インスタンス ptr（vtable 0xA88EA4、確保 0xB00B） |
| 0xB42758 / 0xB42834 | NiDX9RenderState ptr（renderer+0x8AC のキャッシュ、vtable 0xA8A9F4、確保 0x1148B） |
| 0xB42750 | renderer+0x280 の refcounted child（addref/release 付きキャッシュ） |
| 0xB428A8 | shader program factory（malloc 0x20 + ctor 0x77F500） |
| 0xB42760.. | FUN_00771830 の 4x4 合成結果 scratch |
| 0xB427E0 / 0xB427B0 | NiDX9RenderState の word remap 表（+0xC8 用 <8 / +0xD0 用 <5） |
| 0xB4257C / 0xB42884 | シェーダ系リンクリスト sentinel（0x7B81A0 / 0x7B8150 の走査終端） |
| 0xB42EC0..0xB42F2C | シェーダ 28 スロット遅延確保表（writer 4 / reader 4、§c） |

---

検証: `python probe_harness.py selftest` → **4/4 PASS**（本ノート作成前・作成後の両実行）。バイナリ・docs/・既存 tmp/re・TODO・session-memory.json・plan.md は無変更。生成ファイルは本ノート 1 件のみ。
