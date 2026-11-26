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
`markers/` / `cover/` を**鏡写し**にした構成で、`fixtures/nif/{markers,cover}` の
**全 NIF（58 本）**に対応します（期待値の無い NIF があれば FAIL）。ローカル専用の
`tmp/nif_oracle` を使わずに、`dump_rust` の出力を期待値と 1 バイト比較できます:

```
bash scripts/build_nif_oracle.sh      # harness（dump_rust）を作る
python scripts/nif_oracle_check.py    # 不一致・欠落で exit 2。--update で焼き直し
```

`nif_coverage.py --oracle docs/evidence/fixtures/nif/oracle` の見出しは**2 軸**です。
型が満点でもファイル被覆は別軸で、100% だけ出すと「実コーパス全体を比較した」と誤読されます:

```
oracle types = 119 / 119 (100.0%)  |  oracle files = 58 / 58 (100.0%)   # fixture 内
oracle types = 20 / 119  (16.8%)   |  oracle files = 127 / 8032 (1.6%)  # 実コーパス B 全体
```

- `--min-oracle 119`: 型（種類）のラチェット。上限に達しているので、今後は動かない。
- `--min-oracle-files 58`: **ファイル本数のラチェット（第 2 軸）**。オラクルを本数で広げる
  ときはこちらを上げる。型が満点でもファイル被覆は増やせる。

`NiKeyframeController` は oracle に居るがこの meshes コーパスには無い（`known_out_of_corpus.txt`）。
残り型・供給元の対応は `manifest.txt` の supply map を参照。
