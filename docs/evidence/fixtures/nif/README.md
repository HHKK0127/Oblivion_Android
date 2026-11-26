# docs/evidence/fixtures/nif/ — 型被覆 NIF fixture（実物）

`scripts/nif_coverage.py` のブロック型被覆ゲートを CI で回すための NIF 抜粋です。

## 出典・ライセンス

`Oblivion - Meshes.bsa`（Bethesda Softworks / ZeniMax Media のゲーム資産）を展開した
NIF の**最小抜粋**です。本リポジトリはゲーム資産本体を同梱しません。ここに置くのは
テスト用途の最小限の抜粋で、再配布物ではありません（総量 5.0MB / 58 files）。

合成 fixture（`tools/host_tests/make_fixture_*.py` が作る DDS→NIF→BSA）とは**別系統**
です。合成は「経路が通る」こと、実物は「型が読める」ことを見ます。両方残します。

## 構成

- `markers/` — `meshes/` 直下の editor/marker 27 本。`NiWireframeProperty` の
  **唯一の供給源**。`meshes_extracted` 側が落としていた分そのもの。
- `cover/` — 残りの型を貪欲被覆する 31 本。markers とは役割が違うので分ける。
- `manifest.txt` — 選ばれたファイル一覧＋型→供給元の対応（supply map）。

## 外せない 4 本

以下は**特定の型の唯一の供給元**で、消すとその型が fixture から死にます
（`manifest.txt` に `*唯一の供給元` と併記）。

| ファイル | 供給する型 | 備考 |
|---|---|---|
| `cover/creatures/minotaur/minotaurold.nif`（519KB） | `BSKeyframeController` / `NiKeyframeData` | 他に無い |
| `cover/sky/snow.nif`（1.44MB） | `NiDitherProperty` | 他に無い。**費用対効果の悪い 1 本**（この型のためだけに載る） |
| `cover/sky/rainlight.nif`（1.52MB） | `NiMeshPSysData` / `NiMeshParticleSystem` / `NiPSysMeshUpdateModifier` | 他に無い |
| `cover/architecture/leyawiin/blackwoodpump01.nif`（505KB） | `NiPSysResetOnLoopCtlr` | 他に無い |

`NiKeyframeData` / `NiMeshPSysData` / `NiPSysResetOnLoopCtlr` は粒子とキーフレームで
壊れやすい領域なので、差分オラクルの拡張はこの 4 本を先頭に回すこと。

## 限界

**この fixture は構造（型が読める）の被覆しか証明しません。** バイト一致は差分
オラクル（`tmp/nif_oracle`、ローカル専用）の仕事で、別物です。fixture が緑＝正しく
パースできる、ではありません。

## 再生成

```
python scripts/nif_fixture_select.py \
    --corpus tools/BSAFileExtractor/extracted/meshes \
    --manifest docs/evidence/fixtures/nif/manifest.txt \
    --copy-to docs/evidence/fixtures/nif \
    --max-file-bytes 2097152 --limit-bytes 6291456
```

決定的（候補パス昇順・同点パス順）なので毎回同じ manifest が出ます。`--corpus` は
gitignore 済みのローカル展開（`tools/BSAFileExtractor/extracted/meshes`）です。

## CI

```
python scripts/nif_coverage.py --corpus docs/evidence/fixtures/nif \
    --expect-corpus-types 119 --expect-corpus-files 58 \
    --min-impl 119 --max-missing 0 --max-stray 0
```

`--expect-corpus-types` / `--expect-corpus-files` は fixture が痩せた（ファイルが消えた・
型が減った）ことを検知します。B 全体（8032 files / 119 types）での計測はローカルで
回し、CI の数字と混ぜないこと。

## オラクル（型被覆だけでなくバイト一致）

`oracle/` は元 C++ パーサ（dump_orig）の出力で、リポジトリに固定した期待値です。
ローカル専用の `tmp/nif_oracle` を使わずに、`dump_rust` の出力を期待値と 1 バイト比較
できます:

```
bash scripts/build_nif_oracle.sh      # harness（dump_rust）を作る
python scripts/nif_oracle_check.py    # 不一致で exit 2。--update で焼き直し
```

ローカルの `nif_coverage.py --oracle tmp/nif_oracle` の `--min-oracle` は **20** で凍結
（`oracle_types` の 1 つ `NiKeyframeController` はこの meshes コーパスに無いため、
`oracle_types ∩ corpus_types = 20`）。
