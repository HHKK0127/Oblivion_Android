# v280_sSpecialization_gmst_count.md — 依頼1: sSpecialization 登録経路 → GMST 377 vs 378（v93-bundle 2026-11-05）

- 依頼元: `tmp/re/handoff_20261105_v93_residuals_bundle.md` 依頼1 [優先度 a]（出典 v81 §7-3）
- 対象: Oblivion.exe v1.2.0416 (Steam) / 7,898,624 B
- **判定: [確定] — 378 が正。377 は反証。**
- **注記: 本件は既に V93（`tmp/re/v93_gmst_gt_residuals.md` §a、2026-10-25 検証）で確定済み。本レポートは依頼対応として結論を再提示し、2026-11-04 に再検証（数値完全一致）したもの。再解析は行っていない。**

---

## 1. 判定と基準の明示（依頼の求める「数値の一方への確定」）

ESM `GMST` 382 レコード × EXE 全セクションの全数再計数:

| 基準 | 件数 | 未一致 |
|---|---|---|
| (1) **文字列一致基準**: EXE 内に `name + NUL` が在る | **378** | 4 件 = `fRegionGenTexPlacedNotMatch` / `PlacedMatch` / `GenNotMatch` / `GenMatch` |
| (2) **直接 push 基準**: (1) の文字列 VA に .text 内 abs32 参照がある | **378** | **0 件** |

- **(1) と (2) の差 = 0**。両基準とも 378。
- `sSpecialization` は (2) でもヒットする（下記 §2）。したがって**「直接 push 基準で 377」は成立しない**。
- 377 は v81 が誤った .rdata VA（§3）を走査した結果で、**基準の差ではなく走査エラーに起因する過小計数**。

## 2. 登録経路（依頼の「間接参照経路の有無」への回答）

**間接経路ではなく、直接 abs32 参照が実在する**（v81 の「全セクション 0 件」は誤 VA 走査の結果）。

```
0x009F0D50: 68 a8e1a500   push 0xa5e1a8   ; "You will gain +10 in all specialization skills. ..."
0x009F0D55: 68 94e1a500   push 0xa5e194   ; "sSpecialization"      <-- 直接 push（abs32 = 0x9F0D56）
0x009F0D5A: b9 4086b300   mov ecx, 0xb38640
0x009F0D5F: e8 0c8da2ff   call 0x419A70   ; FUN_00419a70 = Setting 構築 + FUN_00412d30 = map.insert(0xB35574)
0x009F0D64: 68 a013a200   push 0xa213a0   ; destructor thunk
0x009F0D69: e8 4612f9ff   call 0x981FB4   ; atexit 登録
0x009F0D6E: 59            pop ecx
0x009F0D6F: c3            ret
```

- この 0x20 ブロックは .rdata の関数ポインタ表スロット **0xA2A1BC = 0x9F0D50** に格納され、**起動時の初期化テーブル走査（v93 §c の 0x981C6B ループ）から呼ばれる**ため実行時に到達する。
- 2026-11-04 再検証（`v280_spot.py`）:
  - `push@0x9F0D55` の imm32 = `0x00A5E194` ✓
  - `@0xA5E194` の実バイト列 = `b'sSpecialization\x00'` ✓
  - `abs32(0xA5E194)` = .text の **1 件のみ = 0x9F0D56** ✓（部分文字列 `0xA5D55D` は 0 件）
- 事後処理: `FUN_004055e0`（erase + `name[0]=='S'` のみ free）は **プロセス終了時のみ**走る（v93 §b）ため、登録済み 378 は実行中保持される。

## 3. v81 の前提に誤りがあったこと（本依頼が残った理由）

| 項目 | v81 記載 | 実測（.rdata 定数 **0x400E00**） |
|---|---|---|
| .rdata 定数 | 0x4009D0（**誤り**、差 0x430） | **0x400E00** |
| 「byte 0x73 長文字列の一部」 | 0xA5D12D | **0xA5D55D**（含む C 文字列 `sClassSpecialization` @0xA5D558） |
| 「独立 null 終端」 | 0xA5DD64 | **0xA5E194**（`sSpecialization\0` 独立） |

- v81 の 2 VA は file offset 0x65C32D / 0x65CF64 に相当し、どちらも無関係な英文（`b'- there are limits…'` / `b'est Items from your Inventory.\x00'`）。
- → v81 の「絶対ポインタ参照 0 件」は**存在しない VA に対する走査結果**であり、前提が成立しない。

## 4. 再検証（2026-11-04）

`python "%TEMP%\opencode\v93_final_verify.py"` 再実行（`probe_harness.py selftest` 4/4 PASS 後）:

```
[a] records=382 errs=0
    (1) name+NUL present : 378   missing = fRegionGenTex* 4 件
    (2) .text abs32 ref  : 378   no-ref: []
    sSpecialization cands: ['0xa5d55d', '0xa5e194']
    abs32(0xA5E194)      : [('.text', '0x9f0d56')]
```

`v280_spot.py`（バイトレベル）: §2 の push/文字列を全一致確認。

## 5. 残点

- なし（依頼1の範囲は完全に確定）。v81 が定数を 0x4009D0 とした原因の特定は v93 残点4のまま（軽微・記録のみ）。

## 6. 証跡

| スクリプト | 内容 |
|---|---|
| `%TEMP%\opencode\v93_final_verify.py` | ESM 382 × EXE 全数再計数（本日再実行） |
| `%TEMP%\opencode\v280_spot.py` | push imm32 / 文字列実体 / abs32 のバイト確認（本日新規） |
| `%TEMP%\opencode\v93_a_scan.py` 等 | V93 当時の走査群（`v93_gmst_gt_residuals.md` §証跡参照） |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。正本 `tmp/re/v93_gmst_gt_residuals.md` §a と数値完全一致。
