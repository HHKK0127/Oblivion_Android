# v282_crt_init_table_walker.md — 依頼3: CRT 初期化テーブル 0xA2ED04..0xA2F1B0（299 エントリ）の走査側特定（v93-bundle 2026-11-05）

- 依頼元: `tmp/re/handoff_20261105_v93_residuals_bundle.md` 依頼3 [優先度 c]（出典 v81 §7-1）
- 対象: Oblivion.exe v1.2.0416 (Steam)
- **判定: [確定] — 走査側 = 無名 gap 関数 `0x981C6B`（146 B）のインラインループ `0x981CC5..0x981CD4`。走査範囲は v81 の run 外周 `[0xA283D0, 0xA2F77C)` = 7,403 スロット。実行タイミング = WinMainCRTStartup が起動時に 1 回呼ぶ（WinMain 呼出しの 45 B 手前）。**
- **注記: 本件は既に V93（`tmp/re/v93_gmst_gt_residuals.md` §c、2026-10-25 検証）で確定済み。本レポートは依頼対応として結論を再提示し、2026-11-04 に再検証（数値完全一致）したもの。再解析は行っていない。**

---

## 1. v81 の「参照 0 件」が成立した理由（依頼の問1の前提）

- v81 の run `[0xA2ED04, 0xA2F1B0)` = 1,196 B = **299 × 4 B**、全 299 件が `.text` 0xA10000..0xA12000 を指す（再現 ✓、2026-11-04 再確認: `all in 0xA10xxx: True`）。
- ただしこれは**外側の巨大配列の一部**:
  - **実行時テーブル = `[0xA283D0, 0xA2F77C)` = 7,403 スロット（29,612 B）**
  - 内訳: `NULL 24 / .text 他 4,459 / バルク登録モジュール 0x9D9D00..0x9FD000 2,621 / シンクロン 0xA10000..0xA12020 299`
  - **「299」は 0xA10000 系シンクロンを指すスロットの全数**と一致し、それらが連続して `[0xA2ED04, 0xA2F1B0)` に並ぶ。
- **v81 が abs32 を 0 件と判定できた理由**: 走査側は**外周境界 2 点だけ**を参照している。run 端点・中央・末尾直後・`va−0x401000` 相対形をどれだけ試しても、コードから参照されていない（配列の一部は「外側の begin/end でまとめて歩く」だけ）。
  - 全セクションで run `[0xA2ED04,0xA2F1B0)` を指す abs32 = **.bind の 1 件（0xBD3765 → 0xA2F04F、非整列データ）のみ、.text は 0 件**（v81 の 0 件を再現、本日再確認一致）
  - .text 全体で `[0xA2D000, 0xA2F780)` を指す 4 バイト列 24 件を線形復元で operand 検証 → **成立は 0x981CBC の 1 件のみ**、残 23 件は drift（本日再確認: `24, operand-valid: 1`）

## 2. 走査側コードの特定（依頼の問1）

```
0x00981CB4: be d083a200   mov esi, 0xa283d0     ; begin（imm32 @0x981CB5）
0x00981CB9: 8bc6         mov eax, esi
0x00981CBB: bf 7cf7a200  mov edi, 0xa2f77c      ; end（opcode bf @0x981CBB / imm32 @0x981CBC）
0x00981CC0: 3bc7         cmp eax, edi
0x00981CC2: 59           pop ecx
0x00981CC3: 730f         jae 0x981cd4
0x00981CC5: 8b06         mov eax, dword ptr [esi]
0x00981CC7: 85c0         test eax, eax
0x00981CC9: 7402         je  0x981ccd
0x00981CCB: ffd0         call eax             ; 各初期化関数（NULL はスキップ）
0x00981CCD: 83c604       add esi, 4
0x00981CD0: 3bf7         cmp esi, edi
0x00981CD2: 72f1         jb  0x981cc5
0x00981CD4: 833d18bcba0000 cmp dword ptr [0xbabc18], 0
```

- 関数範囲 = **[0x981C6B, 0x981CFD) = 146 B**。開始は `0x981C6A: c3 ret` の直後（本日確認: `@0x981C68 = … 5e c3 83 3d 84 3e aa` → ret@0x981C6A、開始 0x981C6B ✓）、末尾は `0x981CFA xor eax,eax / 0x981CFC c3 ret`（本日確認 ✓）。**台帳 21,597 件に開始点なし = 無名 gap 関数**。
- **形は MSVC `_initterm` 相当だが、begin/end の imm32 を関数内に直接埋め込むインライン展開**（汎用 `__initterm` を呼ばず、固定 va を直接ロード）。これが「PE 標準 CRT イニシャライザと整合」する一方で abs32 参照が run 内部に現れない理由。
- 他の走査器は 299 ランに届かない: `0x981BD8`（begin/end 引数形）は `[0xA2F780,0xA2F7A4)` の 6 スロットのみ、`0x981BC0` は `[0xA2F7B0,0xA2F7BC)` と `[0xA2F7C0,0xA2F7C8)` のみ。

### テーブル先頭/末尾の取得方法（依頼が求める「証拠」）

- **固定 va の直接ロード**: `mov esi, imm32(0xA283D0)` @0x981CB5 / `mov edi, imm32(0xA2F77C)` @0x981CBC。シンボル境界・セクション走査・PE ヘッダ経由は**使っていない**。
- 本日再検証（`v280_spot.py`）: `imm32@0x981CB5 = 0x00A283D0` ✓ / `bytes@0x981CB4 = be d0 83 a2 00 8b c6 bf 7c f7 a2 00 …` ✓ / ループ内に `ff d0`（call eax）実在 ✓

## 3. 実行タイミングと 0xA10EB0 の役割（依頼の問2）

- **E8 → 0x981C6B = 1 件のみ = 0x9877E1**（WinMainCRTStartup 内、**WinMain 呼出し 0x98780E の 45 B 手前**）。abs32 / E9 = 0 件、big array 自体にも 0x981C6B は不含（自己再帰なし）。
- → **DllMain CRT initialization / WinMain 前 = プロセス起動時に 1 回**。WinMain（依頼2）の手前で全初期化関数が消化される。
- **`0xA10EB0`（run エントリ 0xA2EFF0 の指す先）は「呼び出し」ではなく「atexit 登録スタブ」**:
  - 本日確認: `@0xA10EB0 = b9 d830b400 e8 9663daff …` = `mov ecx,0xB330D8` + `call 0x7B7250`（**FUN_007b7250 teardown**、`E8 @0xA10EB5` の解析先 = `0x007B7250` ✓）
  - 隣接エントリは全て同型の atexit 登録スタブ（`push 0xA27150; call 0x981FB4` 形）。つまり **0xA10EB0 系は起動時に初期化関数として呼ばれるのではなく、「終了時に teardown を実行するための登録」を初期化の並びの中で行う**。
  - 0xA10EB0 の唯一の E8 caller は 0xA10EB5 内部（自前 call）で、外部 caller は初期化テーブル経由（起動時 run）のみ。
- 関連: 0xA2EFF0 エントリ由来の `FUN_007b7250` 実行経路は**起動時の既定化リセット**であり、実行時 teardown ではない（v81 ②の確定、v93 でも維持）。

## 4. 再検証（2026-11-04）

`v93_final_verify.py` 再実行:

```
[c] array slots=7403 : {'NULL': 24, 'text': 4459, 'bulk': 2621, 'sync0xA10': 299}
    v81 run [0xA2ED04,0xA2F1B0): 299 entries, all in 0xA10xxx: True
    abs32(0xA283D0) : [('.text', '0x981cb5')]
    abs32(0xA2F77C) : [('.text', '0x981cbc')]
    abs32 into run : [('.bind', '0xbd3765', '0xa2f04f')] のみ
    .text 4B in [0xA2D000,0xA2F780): 24, operand-valid: 1 (expect 1 = 0x981CBC)
```

`v280_spot.py`: begin/end imm32・ループ `call eax`・関数フレーム境界・`0xA10EB0` の `call 0x7B7250` を全一致確認。

## 5. 残点

- `0x981C6B` の第1引数（ebx）の意味、`[0xAA3E84]` callback（ループ後 `cmp dword [0xAA3E84],0` 帯）、`[0xBABC18]` の正体（v93 残点2）— 依頼の範囲外。
- 299 エントリの個別関数名づけ（atexit 登録スタブの内訳詳細）— 未実施。

## 6. 証跡

| スクリプト | 内容 |
|---|---|
| `%TEMP%\opencode\v93_final_verify.py` | 配列分布・abs32・operand 検証（本日再実行） |
| `%TEMP%\opencode\v280_spot.py` | 走査ループ/関数境界/0xA10EB0 のバイト確認（本日新規） |
| `%TEMP%\opencode\v93_step5/8/9/10/11/12.py` 等 | V93 当時の走査器探索（`v93_gmst_gt_residuals.md` §証跡参照） |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。正本 `tmp/re/v93_gmst_gt_residuals.md` §c と数値完全一致。
