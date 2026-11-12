# V213: 正本の擬似リンク切れ5件 整形式化ドラフト（分析完了・適用未実施）

- 作成: 2026-10-31 / 正本文書整備担当（分析専任・**docs 本体は一切未編集（読取のみ）**）
- 出典: `tmp/re/relcheck_20261029/rel3_fix_proposal.md` §C（読取のみ）※§A-1/A-2 は 2026-10-30 に適用済みで本ドラフト対象外
- 機械検査の判定方式（rel3 準拠）: `tmp/re/...`・`docs/...`・`%TEMP%` 形式のパスを inline code から抽出し、`*` は glob で実在判定。**`{a,b}` ブレースは展開されない**（Python glob 前提）→ ブレース記法は原理的に機械検査を通過できない。
- after の条件: ①機械検査（抽出→glob/実在確認）が通る ②**実ファイル名と 1:1 で対応**する表記。

---

## 1. アンカー実測表（2026-10-31 実測）

| # | 対象ファイル | 行番号(実測) | 改行コード | 一意性（置換安全性） |
|---|---|---|---|---|
| C-1 | `docs/RE_EXE_REPORT.md`（全文 12,351 行） | **L10643** | CRLF | before 断片 = 1/1 出現 ✓ |
| C-2 | `docs/RE_EXE_REPORT.md` | **L11849** | CRLF | before 断片 = 1/1 出現 ✓ |
| C-3〜5 | `docs/RE_ANALYSIS_ROADMAP.md`（全文 272 行・最終行 L272 は末尾改行なし） | **L267** | LF | 3 トークン一体の before 断片 = 1/1 出現 ✓ |

改行コード実測（`newline=''` 読取）:

| ファイル | CRLF 行数 | LF 単独行数 | 維持方針 |
|---|---|---|---|
| RE_EXE_REPORT.md | 12,351（100%） | 0 | **CRLF 維持** |
| RE_ANALYSIS_ROADMAP.md | 0 | 271（＋最終行は無改行） | **LF 維持** |

### 実測した before 行（現行テキスト・抜粋）

- **C-1 / L10643**（前後: L10642 空行、L10644 空行、L10645 `### §66.1 パイプライン全体（確定）`）

  ```
  証跡: `tmp\re\s90`〜`s98_magic_m*.md` (9ファイル)
  ```

  ※`tmp\re\s90` が機械判定のリンク切れ 1 件（バックスラッシュ＋拡張子なし断片）。`s98_magic_m*.md` は `tmp/re` 接頭辞なしのため抽出対象外（未検査のまま）。

- **C-2 / L11849**（前後: L11848 空行、L11850 空行、L11851 表ヘッダ `| ノート | 主要主題 | 検証 |`）

  ```
  `tmp/re/v60`〜`v76` の 12 本（2026-10-24 22:31〜23:54 生成、計約 208 KB）を全文レビューし本節へ統合した。各ノートの自己申告検証は下表のとおり。
  ```

  ※`tmp/re/v60` が機械判定のリンク切れ 1 件。`v76`（接頭辞なし）は抽出対象外。

- **C-3〜5 / L267**（当該行は約 2,600 字の長行。変更対象は行末の証跡部分のみ。前: L266、次: L268）

  行末証跡部分（これが before 置換単位）:

  ```
  証跡: `tmp/re/v82_absent_fid_classify.{py,txt}` / `tmp/re/v82b_absent_boundary.{py,txt}` / `tmp/re/v82c_absent_related_names.{py,txt}`。§70.14 に (4) 節を追記。
  ```

  ※機械判定では 3 組のブレース記法から `tmp/re/v82_absent_fid_classify` / `v82b_absent_boundary` / `v82c_absent_related_names`（拡張子なし断片）が抽出され、リンク切れ 3 件として計上。

---

## 2. before → after ドラフト全文

### C-1（RE_EXE_REPORT.md L10643・1 行丸ごと置換）

before:

```
証跡: `tmp\re\s90`〜`s98_magic_m*.md` (9ファイル)
```

after（推奨・範囲両端をフルパス化）:

```
証跡: `tmp/re/s90_*.md`〜`tmp/re/s98_*.md` (9ファイル)
```

機械検査: 抽出 2 トークン `tmp/re/s90_*.md` → glob 1 件（`s90_magic_m1_m2_pipeline.md`）✓ / `tmp/re/s98_*.md` → glob 1 件（`s98_magic_m15_misc.md`）✓。範囲 s90〜s98 の実ファイル合計 = 9 件で「(9ファイル)」と 1:1（§3 対応表参照）。末尾の `(9ファイル)` は原文の半角括弧をそのまま維持。

### C-2（RE_EXE_REPORT.md L11849・当該文のみ置換）

**重要**: 提案 §C の after 案 `` `tmp/re/v60_*.md`〜`v76_*.md` `` は glob 実測で **14 ファイル**に一致する（v60-v64/v66/v67/v69/v72/v75a/v75b/v76 の 12 本 ＋ 対象外の `v70_force_create.md` / `v73_func_list_diff.md`）。「12 本」と実ファイルが 1:1 で対応しない。よって 2 案を提示する。

before:

```
`tmp/re/v60`〜`v76` の 12 本（2026-10-24 22:31〜23:54 生成、計約 208 KB）を全文レビューし本節へ統合した。各ノートの自己申告検証は下表のとおり。
```

after 案 A（推奨・12 本を明示列挙して 1:1 を厳守。§70.11 L11996 の列挙スタイルと同一）:

```
`tmp/re/v60_esm_only_keys.md` / `v61_headless_relabel.md` / `v62_v59_residuals.md` / `v63_settings_vtable_attribution.md` / `v64_section7_remaining.md` / `v66_v68_findings.md` / `v67_v74_memory_api_findings.md` / `v69_show1stperson_link.md` / `v72_global_b333c4_map.md` / `v75a_findings.md` / `v75b_findings.md` / `v76_data_relationship_map.md` の 12 本（2026-10-24 22:31〜23:54 生成、計約 208 KB）を全文レビューし本節へ統合した。各ノートの自己申告検証は下表のとおり。
```

- 機械検査: 先頭トークン `tmp/re/v60_esm_only_keys.md` が実在 ✓（残り 11 個は接頭辞なし＝抽出対象外だが、§70.11 と同一の表記慣行。全 12 名は §3 で実在確認済み）。
- 1:1: 列挙 12 名 ＝「12 本」＝§70.11 の一次資料一覧と完全一致。

after 案 B（範囲記法・親指示の形式。除外注記を付ければ 1:1）:

```
`tmp/re/v60_*.md`〜`tmp/re/v76_*.md` の 12 本（範囲内実在 14 本のうち v70_force_create.md / v73_func_list_diff.md は本レビュー対象外。2026-10-24 22:31〜23:54 生成、計約 208 KB）を全文レビューし本節へ統合した。各ノートの自己申告検証は下表のとおり。
```

- 機械検査: `tmp/re/v60_*.md` → 1 件 ✓ / `tmp/re/v76_*.md` → 1 件 ✓。
- 1:1: 除外注記込みなら成立。ただし冗長になり、`v70`/`v73` の存在を今後readerに説明し続ける必要がある。**推奨は案 A**。

### C-3〜C-5（RE_ANALYSIS_ROADMAP.md L267・行末証跡部分のみ置換）

**重要**: 提案 §C の after 案 `v82*_absent_*.{py,txt}` は **ブレースを含むため Python glob では 0 件**（実測済み）→ そのままでは機械検査を通過できない。ブレースなしの表記に分解する必要がある。

before（置換単位・一意）:

```
証跡: `tmp/re/v82_absent_fid_classify.{py,txt}` / `tmp/re/v82b_absent_boundary.{py,txt}` / `tmp/re/v82c_absent_related_names.{py,txt}`。§70.14 に (4) 節を追記。
```

after 案 A（推奨・2 グロブに分解。簡潔かつ 1:1）:

```
証跡: `tmp/re/v82*_absent_*.py` / `tmp/re/v82*_absent_*.txt`（計 6 ファイル = v82 / v82b / v82c の各 .py + .txt）。§70.14 に (4) 節を追記。
```

- 機械検査: `tmp/re/v82*_absent_*.py` → glob 3 件 ✓ / `tmp/re/v82*_absent_*.txt` → glob 3 件 ✓。
- 1:1: 3+3 = 6 ファイル。`v82d_report_write.py` は `_absent_` を含まないため誤マッチしない（実測確認済み）。

after 案 B（6 個を完全列挙・最長だが完全明示）:

```
証跡: `tmp/re/v82_absent_fid_classify.py` / `tmp/re/v82_absent_fid_classify.txt` / `tmp/re/v82b_absent_boundary.py` / `tmp/re/v82b_absent_boundary.txt` / `tmp/re/v82c_absent_related_names.py` / `tmp/re/v82c_absent_related_names.txt`。§70.14 に (4) 節を追記。
```

- 機械検査: 6 トークンすべて実パスで実在 ✓（6/6）。行末証跡部分がさらに長くなる点のみ留意。

---

## 3. 実在ファイル対応表（glob / isfile 実測 2026-10-31）

### C-1: s90〜s98（9 ファイル・`tmp/re/` 直下・md のみ）

| 範囲プレフィックス | 実ファイル名 | サイズ |
|---|---|---|
| s90 | `s90_magic_m1_m2_pipeline.md` | 8,194 B |
| s91 | `s91_magic_m3_mgef_dispatch.md` | 3,172 B |
| s92 | `s92_magic_m9_handlers.md` | 5,444 B |
| s93 | `s93_magic_m10_apply.md` | 3,984 B |
| s94 | `s94_magic_m11_cost.md` | 2,746 B |
| s95 | `s95_magic_m12_delivery.md` | 2,599 B |
| s96 | `s96_magic_m13_alchemy_poison.md` | 2,883 B |
| s97 | `s97_magic_m14_ai_spell.md` | 2,862 B |
| s98 | `s98_magic_m15_misc.md` | 3,496 B |

計 9 件 = 「(9ファイル)」と一致。※`s99_phase1_residual_analysis.md` も実在するが**範囲外**（s90〜s98 の 9 件には含めない。`s9*.md` 一括 glob は s99 を含む 10 件になるため使用不可）。

### C-2: 「12 本」＝§70.11 の一次資料一覧（実測で全 12 名実在）

| # | 実ファイル名 | mtime | サイズ |
|---|---|---|---|
| 1 | `v60_esm_only_keys.md` | 2026/10/24 18:38:58 | 13,134 B |
| 2 | `v61_headless_relabel.md` | 2026/10/24 21:18:18 | 25,945 B |
| 3 | `v62_v59_residuals.md` | 2026/10/24 19:30:02 | 19,076 B |
| 4 | `v63_settings_vtable_attribution.md` | 2026/10/24 20:02:03 | 26,352 B |
| 5 | `v64_section7_remaining.md` | 2026/10/24 20:11:18 | 17,686 B |
| 6 | `v66_v68_findings.md` | 2026/10/24 22:42:12 | 11,627 B |
| 7 | `v67_v74_memory_api_findings.md` | 2026/10/24 23:54:09 | 11,632 B |
| 8 | `v69_show1stperson_link.md` | 2026/10/24 21:29:58 | 17,807 B |
| 9 | `v72_global_b333c4_map.md` | 2026/10/24 21:49:52 | 17,193 B |
| 10 | `v75a_findings.md` | 2026/10/24 23:34:53 | 17,210 B |
| 11 | `v75b_findings.md` | 2026/10/24 23:10:58 | 14,944 B |
| 12 | `v76_data_relationship_map.md` | 2026/10/25 1:25:21 | 23,989 B |

- 範囲 `v60_*.md`〜`v76_*.md` の glob 実測は **14 件**。上記 12 件に加えて **`v70_force_create.md`（2,439 B）と `v73_func_list_diff.md`（3,453 B）が範囲内に実在するが「12 本」には含まれない**（§70.11 の列挙・§70.1 の表にも v70/v73 の行なし）。
- 欠番プレフィックス: v65 / v68 / v71 / v74（単独ファイルなし。v66v68 / v67v74 は枝番を統合した 1 ファイル名）。
- 補足（範囲外の参考）: `v76_data_relationship_map.md.bak1` / `.bak2` は `*.md` glob に一致しない（末尾が `.bakN`）ため影響なし。また本文の生成時間帯表記「22:31〜23:54」は mtime（最早 18:38）と一致しないが、mtime ≠ 生成時刻の可能性があり本ドラフトでは時刻表記を検証・修正対象外とした（書式のみが対象）。

### C-3〜5: v82 ファミリ（`tmp/re/` 直下・6 ファイル対応＋参考 1 件）

| トークン | .py | .txt |
|---|---|---|
| `v82_absent_fid_classify` | `v82_absent_fid_classify.py`（5,424 B）✓ | `v82_absent_fid_classify.txt`（8,744 B）✓ |
| `v82b_absent_boundary` | `v82b_absent_boundary.py`（6,439 B）✓ | `v82b_absent_boundary.txt`（6,499 B）✓ |
| `v82c_absent_related_names` | `v82c_absent_related_names.py`（5,210 B）✓ | `v82c_absent_related_names.txt`（7,094 B）✓ |

計 6 ファイル実在。案 A の 2 グロブ（`v82*_absent_*.py` / `v82*_absent_*.txt`）は実測で各 3 件に一致し 1:1。参考: `v82d_report_write.py`（13,360 B）は実在するが本文から参照されず（`_absent_` なし）グロブにも誤マッチしない。

---

## 4. 適用チェックリスト（適用セッション用）

適用は本ドラフトの承認後、別セッションで実施すること。手順順:

1. **.bak 排番**（適用直前にも再確認）
   - `docs/RE_EXE_REPORT.md` → `RE_EXE_REPORT.md.bak21`（現存最大は bak20 / 2026-10-30 15:18:38 / 1,038,606 B）
   - `docs/RE_ANALYSIS_ROADMAP.md` → `RE_ANALYSIS_ROADMAP.md.bak8`（現存: .bak / bak2 / bak3 / bak4 / bak6 / bak7。bak5 は欠番だが次番は bak8）
   - コピーは編集前に実施。編集は Python で `newline=''` 読取→部分置換→`newline=''` 書出（CRLF/LF を自動変換させない）。あるいはバイト列置換。
2. **anchor==1 assert**（置換直前に再実行必須。2026-10-31 実測では 3 件とも 1/1 だったが、並行編集があり得るため置換直前の再確認を行う）
   - C-1: `証跡: \`tmp\re\s90\`〜\`s98_magic_m*.md\` (9ファイル)` → RE_EXE_REPORT.md 内 1 出現を assert
   - C-2: `` `tmp/re/v60`〜`v76` の 12 本 `` → RE_EXE_REPORT.md 内 1 出現を assert
   - C-3〜5: `証跡: \`tmp/re/v82_absent_fid_classify.{py,txt}\` / \`tmp/re/v82b_absent_boundary.{py,txt}\` / \`tmp/re/v82c_absent_related_names.{py,txt}\`。` → RE_ANALYSIS_ROADMAP.md 内 1 出現を assert
   - 0 件または複数件なら即中断し状況を報告（行ずれの可能性）。
3. **置換の実行**（本文 §2 の after 案 A を採用推奨。案 B は承認時選択）
   - C-1 は行丸ごと、C-2 は当該文のみ、C-3〜5 は行末証跡部分のみ。いずれも**行内完結**（改行文字を含めない置換）→ 行数は REPORT 12,351 / ROADMAP 272 のまま変わらない。
4. **改行コード維持の確認**
   - 適用後に再度 `newline=''` で読み、REPORT = CRLF 12,351 / LF 単独 0、ROADMAP = CRLF 0 / LF 単独 271（＋最終行無改行）であることを assert。
5. **round_verify（rel3 方式の再抽出）**
   - C-1 / C-2 / C-3〜5 の 5 出現がリンク切れ判定から消えることを確認。
   - **既知の残存 dangling（本バッチの失敗ではない）**: `%TEMP%\opencode\analyze_pop_final.py`（REPORT L11843・B-1 は任意で未適用）＋ `docs/RE_ESM_REPORT.md`（ROADMAP L42, L123）＋ `docs/RE_ASSET_REPORT.md`（ROADMAP L43, L144）＝計 5 出現。これらが残っていれば round_verify は**本バッチとしては ALL OK**。A-1（ROADMAP L125 → `tmp/wbDefinitionsTES4.pas`）と A-2（REPORT L11923 → handover パス）は適用済み・検証済み。
   - 機械検査は rel3 と同じ抽出方式（`tmp/re`・`docs`・`%TEMP%` 接頭辞＋`*` glob 展開、ブレース非対応）を用いること。
6. **回帰チェック**
   - 適用後の 2 ファイルで、他箇所に同種のバックスラッシュパス / ブレース記法が新規混入していないこと（REPORT 内 `tmp\re\` 表記は L10643 のほか L10783 `### §66.12 証跡 (すべて \`tmp\re\`)` に存在するが、これは見出しの補足表記でパス断片抽出の対象外（rel3 でも未計上）→ **本バッチ対象外・将来の参考情報**として記録のみ）。
   - KPI 表記の既存ドリフト（ROADMAP L266「RE_EXE_REPORT 12,085 行」⇔実測 12,351 行）は本バッチ対象外（書式修正とは無関係）。同様に C-2 の生成時刻「22:31〜23:54」と mtime の差も対象外。

---

## 5. 総括

- 5 件すべてアンカー実測・一意性確認・実在ファイル照合まで完了。適用は未実施（docs 未編集）。
- 提案 §C の after 案に **2 箇所の不備**を検出した: ①C-2 の `v60_*.md`〜`v76_*.md` は実在 14 件で「12 本」と不一致（除外注記 or 明示列挙が必要）②C-3〜5 の `{py,txt}` ブレースは glob 展開不可のため機械検査を通過できない（ブレースなし 2 グロブか 6 個列挙に分解が必要）。本ドラフトの after 案 A はこれらを解消した形。
- 適用手順は .bak 排番 → anchor==1 assert → 置換（CRLF/LF 維持）→ round_verify（既知残存5出現を除外して ALL OK 判定）まで §4 に記載。
