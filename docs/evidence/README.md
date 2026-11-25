# docs/evidence — 見た目の基準（ゴールデン）

## なぜここに置くか

「描画順・UV・ブレンド・文字寸法の誤り」はスクリーンショットの目視では見つからない。
原版（または承認済みの基準）との差を数値にして、閾値超過で CI を落とす。

**生のスクリーンショットはコミットしない。** 画面には Bethesda のアセットが写るため、
このリポジトリの「再配布しない」方針（`.gitignore` の Bethesda アセット節）に反する。
代わりに **32x18 の輝度グリッド（整数）を JSON** で保存する。これは派生物であり、
元アートの再配布にはならない。

- 検出できる: 文字寸法の倍率違い、下降字の欠落、要素の欠落・移動、背景の入れ替わり、色調のずれ。
- 検出できない（グリッドを過信しない）:
  - **文字の内容**（フォントの正しさ・字面）→ 別途ホストテストと目視で見る。
  - 数 px の細部。
  - **UV・ブレンド・描画順** → RenderDoc / Android GPU Inspector の領分。

判定は「グリッド（位置・寸法・色・有無）／ホストテストと目視（文字）／RenderDoc（UV・ブレンド）」
の 3 層に分ける。グリッドが緑でも崩れていることはある。

## golden #1 の前提（ESM 不在・現状ピン）

`docs/evidence/golden/newgame-no-esm-prefix.json` は **現状のピン留めであり、正解の基準ではない**。

- **ESM 不在前提**: CI の smoke は実データ（`Oblivion.esm` 約 200MB）を積まない。したがって race 一覧が空の chargen を撮る。
- **プレースホルダ前提**: chargen の見た目は現状プレースホルダ（羊皮紙・ボタン素材・中央寄せが未実装）。これは「原版にどれだけ近いか」の基準には使えない。
- 役割は **変化検出器** である。今後レイアウト・素材・修正を入れた瞬間に、差分として数値で検出する。

### 再採取のトリガ（この 2 つが来たら `--update` してレビューする）

1. **`initialize()` 修正のマージ後**（chargen が描画されるようになった時点。赤くなるのは修正が効いた証拠）。
2. **ESM 供給時**（race 一覧が入って正当に変化する時点）。

ファイル名の `-no-esm-prefix` は、この 2 つの前提（ESM 不在・プレースホルダ）を忘れないため。

### 来歴（meta）— 画素差と環境差を混同しないため

golden の JSON には撮影条件を埋める（PNG はアセットで未コミットなので、JSON から実行を辿れる必要がある）:

```json
{"meta": {"taken_at", "commit", "app_version",
          "wm_size", "density", "rotation", "source_png_sha256"},
 "grid_w": 32, "grid_h": 18, "cells": [...]}
```

- `docs/smoke-tests/newgame.sh` が `artifacts/smoke-newgame.env` に `wm_size`/`density`/`rotation` を書く。
- `compare_golden.py` は 引数 → サイドカー の順で現環境を決め、golden の meta と比べる。
- **`wm_size`/`density`/`rotation` が違えば「環境不一致」として、画素差とは別のメッセージで落とす**
  （密度が変わっただけの赤を延々と追わないため）。`--allow-env-mismatch` で画素比較へ進める。

## 合成フィクスチャ（`docs/evidence/fixtures/`）

実データ（`Oblivion.esm`・`Oblivion - *.bsa`）は再配布できないので、**自作の最小バイト列**を
コミットしてパーサの挙動を固定する。`tools/host_tests/make_fixture_*.py` が生成器で、
再実行しても同一バイト列になる（時刻・乱数・絶対パスを使わない）。

| フィクスチャ | 生成器 | 形式の出所（逐語） |
|---|---|---|
| `minimal.dds`（256B） | `make_fixture_dds.py` | `app/src/main/cpp/tests/dds_upload_tests.cpp:27-57` の受理バイト列 |
| `minimal.esm`（578B） | `make_fixture_esm.py` | Panel レーンの newgame smoke 用（RE v664 実測のレイアウト） |
| `minimal.nif`（400B） | `make_fixture_nif.py` | `rust/crates/nif-core/src/lib.rs` の受理テスト `shape_data_decodes_vertices_colors_uvs_and_triangles`（Gamebryo 20.0.0.5 / NiTriShape + NiTriShapeData） |
| `minimal.bsa`（485B） | `make_fixture_bsa.py` | `rust/crates/bsa-core` の `{format.rs,hash.rs}` と `tests/parse.rs` の `build()`。中身は `meshes\test.nif`（＝`minimal.nif`）で **BSA→NIF の連鎖**を通す |

- 形式の出所は**リポジトリのパーサと受理済みテスト**。推測では書かない。生成器は
  `nif-core`/`bsa-core` のテストと **SHA256 一致**を確認してある（NIF: `7EE12A60…`）。
- ゲートは `scripts/run_fixture_gates.sh`（CI: `.github/workflows/real-data-gates.yml`）。
  - 生成器を再実行して **バイト一致**（drift）を確認。
  - フィクスチャ **0 件は失敗**（欠如を黙って通さない）。
  - `tools/host_tests/fixture_parse_main.cpp` を**実パーサ**に通し期待値を assert（DDS/NIF/BSA→NIF）。
  - **陰性対照 4 件**: DDS/NIF の magic 破壊、NIF のブロック型差し替え、BSA の magic 破壊。
    消費者が**期待の落ち方**（`"loaded":false` / `"hasNiTriShape":0` / `"opened":false`）で落ちることを確認。
- **限界（混同しない）**: 合成フィクスチャは **パーサ挙動の固定のみ**で、**実データ互換は証明しない**。
  実コーパス（`tmp/*_oracle`）のオラクルは別ラベル（ローカル／リリース前ゲート）で扱う。

## ファイル

| パス | 内容 |
|---|---|
| `docs/evidence/golden/<gate>.json` | 基準（輝度グリッド）。人がレビューしてコミットする |
| `docs/smoke-tests/<gate>.sh` | スモーク。スクリーンショットを `artifacts/` に出す |
| `scripts/compare_golden.py` | 比較（`--update` で基準を書き出す） |

## 使い方

```powershell
# 1) 端末で基準を作る（人が画面を見て承認する）
$adb = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
python scripts/compare_golden.py artifacts/smoke-newgame.png --update
git add docs/evidence/golden/newgame-no-esm.json

# 2) 比較（CI と同じ）
python scripts/compare_golden.py artifacts/smoke-newgame.png --require --diff artifacts/smoke-newgame.diff.png
```

`--require` を付けると、基準が無いときに**失敗**する（黙って通さない）。
CI の `smoke` ジョブは `--require` で回すので、**最初の 1 回は基準を作るまで赤くなる**。
これは意図的（「データが無いので ok」を許さない、ByroRedux の real-data-gates に倣う）。

## ローカルの目視（scrcpy）

自動比較の前に、人が画面を大きく見たいときは scrcpy を使う。

```powershell
# 端末を PC に映して操作する（撮影しながら確認できる）
scrcpy --max-size 1280 --stay-awake
# スクリーンショットを 1 枚だけ取る
& $adb exec-out screencap -p > artifacts/manual.png
```

## 原版との比較（次段）

原版のスクリーンショットを入手できたら、同じ `--update` の手順で
`docs/evidence/golden/<gate>-vanilla.json` を作り、`--golden` で指定して差分を取る。
`v368` の下降字クリップや `1.2×scale` の文字寸法は、この比較で数値として落ちる。
