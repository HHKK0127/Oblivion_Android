# v182_duplicate_override_note.md — V170 重複起動と上書き事故の記録＋Q9-Q12 出典マップ（新規ファイル）

タスク: V170（serial 170 / 束B アセット形式統合）**重複起動として打ち切り**
作成: 2026-10-29・本セッション（`ses_e7a709bdcffe9bI7UmhGirATRd`）・分析専任
位置づけ: 他セッションからの注意依頼（`warn_duplicate_v170_v172.md` 2026-10-29 12:29）に従い、
**既存成果物 `tmp/re/v182_asset_integration.md` / `v184_engine_small_residuals.md` / `TODO_re49.md` は本ノート作成時点で非改変（以後も変更しない）**。
V170 正本 = A側成果物 `tmp/re/v182_asset_integration.A_completed_0748.md`（26,322 B・バイト一致復元済み＝A側 DB 照合）。

---

## 1. 手法 / 入力

- **読取のみ**: `tmp/re/SUBAGENT_CONTRACT.md`、`tmp/re/GT_CHEATSHEET.md`、`tmp/re/cache/README.md`、`tmp/re/TODO_re49.md`（L583 = V170 行）、`tmp/re/v180_letter_rows_audit.md`（Q9-Q12 行 L121-124・束B 行 L158・ロードマップ L179）。
- **検証**: `python "$env:TEMP\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時）。
- **出典所在特定**: glob（tmp/re / copilot_archive / %TEMP%\opencode）で Q9-Q12 の入力ファイルを列挙（§3）。
- **状態観測（本セッション実測）**: TODO V170 行を2回 grep → 1回目（11:52 時点）`[ ]`、2回目（12:01:56 の TODO mtime 後）`**[x] 完了 (2026-10-29)**` かつ要約に `26,322B` を確認。→ 実行中に他側の完了が反映されたことを検出。
- **上書き検出**: `Get-Item` 実測で v182 の CreationTime=**07:45:57**（＝A側が作成済み）／本セッション `write` 後 LastWriteTime=**11:53:09**・Length=**648 B**（PLACEHOLDER）。
- **復元調査（途中で打ち切り）**: `opencode.db`（2,194,354,176 B）を read-only SQLite で照合 → `session_message.data` に `v182_asset_integration` **47ヒット**（スクリプト `%TEMP%\opencode\v182_recover1.py` / `v182_recover2.py`）。実際の復元はA側が同手法（write+edit×4 のメッセージ再生）で完了済みのため、本セッションでは検証のみ実施。

## 2. 事実タイムライン（証拠・すべて実測）

| 時刻 (2026-10-29) | 事象 | 出典 |
|---|---|---|
| 07:37:17 | A側 `ses_e9ccb3190ffeDPSxT2BsblKHKr` が V170/V172 をディスパッチ | 他セッション DB 照合（添付 warn） |
| 07:45:57 | `tmp/re/v182_asset_integration.md` 作成（A側） | 本セッション Get-Item CreationTime |
| 07:48:39 | A側 V170 完走（`idle_outcome=succeeded`・26,322 B） | 他セッション DB 照合 |
| 11:52:33 / mtime 11:53:09 | **本セッションの `write` が A側成果物を 648 B PLACEHOLDER に上書き** | 他セッション検出＋本セッション mtime 実測（一致） |
| 11:5x | 本セッション第1grep: TODO V170 = `[ ]`（＝この時点では完了記録が未反映で重複に気づけず） | 本セッション grep 出力 |
| 12:01:56 | TODO 最終更新後 第2grep: V170 = `[x] 完了 26,322B` → 重複を検知 | 本セッション grep 出力 + TODO LastWriteTime |
| 12:27:57 | 復元ファイル `v182_asset_integration.A_completed_0748.md` = 26,322 B を確認 | 本セッション Get-Item |
| 12:29 | 他セッションから注意依頼（本セッション ID 指定・3点依頼） | `warn_duplicate_v170_v172.md` |

## 3. 独自出典マップ（Q9-Q12 入力・将来の統合再実行用・本セッション glob 実測）

> v180 L158 のとおり統合先は `docs/RE_ASSET_REPORT.md`（**未開設**）。以下は再実行時の入口。

- **Q9 BSA 全形式（v103/v104・圧縮/非圧縮）**: `copilot_archive/bsa_probe{,2,3,5}.py`・`bsa_paths.py`・`bsa_entries.txt`・`list_bsa_landscape.py`・**`textures_bsa_index.txt`（484,153 B）** / `tmp/re/s76_bsa_v104.md`（10,794 B）＋ `s76_bsa_header{,2}.txt`・`s76_bsa_regions.txt`・`s76_bsa_v104.py`・`_s76_*.py`・`_s77_bsa*.py` / `tmp/re/s73_bsa_dds_tel.md`（11,055 B）。ハッシュ文書は v180 L121 により `docs/BSA_HASH_ALGORITHM.md` へ**docs 側化済み**（再作成不要）。
- **Q10 NIF（20.0.0.4 / user 2817 / BSVER 11）**: `copilot_archive/nif_oblivion_layout_spec.md`（**13,375 B**・17 実アセット byte 検証済＝v180 L122）＋ `tmp/re/s69_nif.md` ＋ 実アセット `tmp/re/lodnif/`（LOD NIF 群＋manifest.txt）・`tmp/re/lodb/nif_probe.exe`。注意: 統合先 main レポート側に NIF 節は未設。
- **Q11 DDS（DXT1/3/5）**: `copilot_archive/dds_decode_probe.py`（**7,434 B**）＋ `s73_bsa_dds_tel.md` ＋ 拡張テーブル `0xB25380`（NIF/KF/DDS/WAV＝v180 L123、GT §規則により引用前に probe 要）。
- **Q12 ドア/テレポート国勢調査**: `copilot_archive/x_tel_census.py`（**6,995 B**）＋ `xtel_census.cpp`・`x_tel_pairs.py`・`x_tel_cellkind.py` / `tmp/re/s69_wrld_a6_teleport_entry.txt`・`s69_wrld_a8_teleport_extended.txt`・`s69_wrld_a15_cell_sub_xtel.txt`・`s69_wrld_a16_refr_sub_xtel.txt`・`s77_ft_1_teleport_entry.txt`・`s77_oth_1_fasttravel.py`（＝v180 L124）。

## 4. 結論（3行）

1. V170 はA側で 07:48 に完走済み。本セッションは spawn 時に TODO が `[ ]` だったため重複に気づかず、11:52:33 に成果物を上書きする事故を1件起こした（A側がバイト一致で復元済み）。
2. 本セッションは依頼3点（v182/v184 非改変・TODO 非編集・知見は新規ファイル）を遵守し、独自確認事項のみ本ノートに保存した。
3. 再統合が要る場合の正本は A側 `v182_asset_integration.A_completed_0748.md`、入口は §3 の出典マップ。

## 5. 残留不明点

- R-a: 親セッションが spawn 前に `opencode.db` の `session_v2.idle_outcome` を照合する運用がなく重複が起きた（規約改善提案＝照合手順の追加要否は親判断）。
- R-b: `tmp/re/v182_asset_integration.md` 本体は当面 648 B の PLACEHOLDER のまま（正の集約先・削除の可否はA側/親の判断。本セッションは依頼により不干渉）。
- R-c: A側復元ファイルと元 A側成果物の**バイト一致はA側の DB 照合結果による**（本セッションは 26,322 B のサイズ一致のみ独立確認。sha256 比較は未実施）。

## 6. 自己検証

- `probe_harness.py selftest` → **4/4 PASS**（着手時実行・`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl`）。
- 内部整合: TODO 要約値 `26,322B` == 復元ファイル実サイズ 26,322 B（**一致**）／本セッション検出 mtime `11:53:09` == 他セッション検出 `11:53:09`（**一致**）／v180 Q9-Q12 4行（L121-124）⇔ §3 出典マップ 4件の対応 **4/4**／依頼遵守 = v182+v184+TODO への書き込み **0 件**（11:52 の事故以降）。
- 作成ファイルは本ノート1件のみ（契約 §2「成果物 新規1ファイル」内）。

## 7. Android 移植への示唆

BSA/NIF/DDS/テレポートの移植技術知見そのものは A側成果物（26,322 B）が正本であり本ノートは重複しない。運用上の示唆は1点のみ——アセット統合系タスクは `opencode.db` の完了状態照合を spawn 前必須工程とすることで、本件のような「成果物上書き」事故（復元に DB メッセージ再生が必要）を回避できる。
