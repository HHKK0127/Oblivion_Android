# v651 — precheck `--all` の正体と baseline 化（過去「CN=0」の範囲訂正）

- **日付**: 2026-11-23
- **対象**: `scripts/precheck.py`（v650 で追跡下に再設置）／追跡下全文書
- **判定**: `[確定(データ)]` / `[確定(命令)(negative)]` / `[未確定]`

## §1 `--all` の結果（2026-11-23 実測）

`python scripts/precheck.py --all` = **231 hits**（dotAV 修正後。当初の 257 は dotAV 誤検出 26 を含んでいた）。

| 検査 | 件数 | 正体 |
|---|---|---|
| **CN** | **228** | **実在**（日本語文書に簡体字が混入） |
| **FFFD** | **3** | **実在**（文字化け） |
| **dotAV** | **0** | （現行の追跡下に RTTI の `?` 落ちは無し。下記 §4） |

## §2 CN 228 は実在 `[確定(データ)]`

- 検出例 3 件の実物（簡体字は U+ で表記）:
  - `tmp/re/v96_gap_instances_crosscheck.md:1` = `# V96: 無名 gap 新 U+5B9E U+4F8B と …`（U+5B9E U+4F8B。本来は「実例」）
  - `tmp/re/v96_gap_instances_crosscheck.md:24` = `各 U+5B9E U+4F8B を 6 重で照合`
  - `tmp/re/v_data_relmap.md:254` = `V96 新 U+5B9E U+4F8B 10`
  - （他）`docs/RE_EXE_REPORT.md:12606` = `無名 gap 新 U+5B9E U+4F8B 10 件`
- **U+5B9E = 「実」の簡体字**（日本語は「実」）。同様に U+636E=「拠」の簡体字(58)/U+8FD0=「運」(18)/U+8BA4=「認」(14)/U+8BDD=「話」(11) 等、**すべて簡体字専用字**。
- → 日本語文書への**簡体字の混入が実在**（変換ミス/IME 混入）。**誤検出ではない**。
- 分布: `tmp/re`（最多）・`.github/memory/session-memory.json`・`.ai-todo/state/TODO.md`・`TODO.md`・`docs/RE_EXE_REPORT.md`・`STATE.md`。

## §3 過去の「CN=0」の範囲訂正 `[確定(命令)(negative)]`

- 各ラウンドの「CN=0 / 全文書 CN=0」は、**検査対象が「変更ファイル」または「主要文書」に限定**されていた。`--all`（追跡下全体）では **CN 228 が残存**する。
- → 「全文書 CN=0」は**不正確**。正しくは「**変更ファイル／主要文書の CN=0**」。本節で訂正する。
- 228 の一括クリーンアップは **別 row（台帳 row628）**。**実装 lane はブロックしない**。

## §4 dotAV = MSVC RTTI 名の `?` 落ち `[確定(データ)]`

- 追跡下の実測: **`.?AV<name>@@`（正しい RTTI）= 1473 件**、**`.AV<name>`（`?` 落ち）= 23 件** → **両方出現** → 定義確定。
- → **定義 = `\.AV`（`?` 無し）を FAIL、`\.\?AV` は許可**。破損例: `.?AV`＋`RaceSexMenu@@` の `?` が落ちると `.AV`＋`RaceSexMenu@@` になる。
- 実測（修正後 `--all`）: dotAV = **0**（追跡下に RTTI の `?` 落ちは無し。当初の 3 件は `0..AV_STAMINA` の誤検出）。control: `.?AV` は非検出、`.AV`＋大文字 は検出。

## §5 negative / positive control `[確定(データ)]`

| control | 入力 | 結果 |
|---|---|---|
| negative（日本語常用字） | `実 話 運 点 拠` | **非検出**（OK） |
| positive（簡体字） | U+5B9E U+8BDD U+8FD0 | **検出**（FAIL, exit 1） |

→ `CN_CHARS` に日本語漢字の誤検出は無い（`実/話/運/点/拠` は集合外）。

## §6 `--all` の baseline 化 `[確定(命令)]`

- `scripts/precheck.py` に **`ALL_BASELINE = 257`** を焼き、**`--all` は baseline 超えのみ FAIL**（既存分は許容＝増加のみ ERROR）。現状把握と回帰防止を両立。
- 実測: `precheck: OK --all (257 hit(s) <= baseline 257; pre-existing)`。
- 変更ファイル検査（引数なし）は従来どおり **0 件必須**（`ALL_BASELINE` の対象外）。

## §7 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-651-1 | （**解決**）`dotAV` = MSVC RTTI 名の `?` 落ち `\.AV`。`\.\?AV` は許可 | `[確定(データ)]` |
| — | CN 228 の一括クリーンアップ | 台帳 row628（非ブロッカー） |
