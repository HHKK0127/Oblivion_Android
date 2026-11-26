# docs/evidence/fixtures/nif/ — NIF fixture（実物）

`scripts/nif_coverage.py` のブロック型被覆ゲートと、`scripts/nif_oracle_check.py` の
バイト一致オラクルを CI で回すための NIF 抜粋です。

## 出典・ライセンス

`Oblivion - Meshes.bsa`（Bethesda Softworks / ZeniMax Media のゲーム資産）を展開した
NIF の**最小抜粋**です。本リポジトリはゲーム資産本体を同梱しません。ここに置くのは
テスト用途の最小限の抜粋で、再配布物ではありません（総量 6.3MB / 84 files）。

合成 fixture（`tools/host_tests/make_fixture_*.py` が作る DDS→NIF→BSA）とは**別系統**
です。合成は「経路が通る」こと、実物は「型が読める」ことを見ます。両方残します。

## 構成

- `markers/` — `meshes/` 直下の editor/marker 27 本。`NiWireframeProperty` の
  **唯一の供給源**。`meshes_extracted` 側が落としていた分そのもの。
- `cover/` — 57 本。**性格が 2 つある**（選定基準が違うので混同しないこと）:
  - **型被覆用（31 本）** — 残りの型を貪欲被覆する最小集合。`nif_fixture_select.py`
    の貪欲法が選ぶ。
  - **レイアウト被覆用（26 本）** — 同じ型でも**異なるレイアウト**を読めるかを見る。
    衝突形状の署名（`bhk*Shape` の集合）が異なる代表を 1 つずつ選ぶ。`--add
    scripts/data/nif_layout_add.txt` で固定投入。選び方は `nif_layout_select.py` を参照。
- `manifest.txt` — 選ばれたファイル一覧（markers / 型被覆 / レイアウト被覆の 3 節）
  ＋型→供給元の対応（supply map）。

### レイアウト被覆で採らなかった署名（費用対効果）

除外 58 本のあと、`bhkCollisionObject` を含む候補は **32 署名**に分かれる。
`nif_layout_select.py` は (1) 頻度上位 20 署名を必ず取り、(2) 残りを頻度順に
追加 256KB の予算内で足す。この結果 **26 本**（上位 20 ＋ 安い 6）になる。

**残り 6 署名は意図的に採っていない**（代表が大きいだけで、新形状が出ないため）:

| 署名（代表ファイル） | 代表サイズ |
|---|---|
| `Box,Capsule,ConvexVertices`（`clutter/fightersguild/targetheavy01.nif`） | 411KB |
| `ConvexVertices,List,MoppBvTree,NiTriStrips`（`oblivion/.../towerbridgeextralong02anim.nif`） | 601KB |
| `Capsule,ConvexVertices,MoppBvTree,NiTriStrips,SimpleShapePhantom`（`.../hallwaytrackbladetrap02b.nif`） | 245KB |
| `Box,Capsule,MoppBvTree,NiTriStrips`（`oblivion/gate/obliviongate_simple.nif`） | 632KB |
| `Box,ConvexVertices,MoppBvTree,NiTriStrips`（`architecture/statue/nightmotherstatuebase.nif`） | 834KB |
| `Box,ConvexVertices,MoppBvTree,NiTriStrips,SimpleShapePhantom`（`.../hallwaytrackbladetrap.nif`） | 539KB |

これら 6 署名の形状はすべて既存の 26 本で踏んでいる（**新しいのは `bhkConvexSweepShape`
1 つだけで、それは `clutter/farm/handscythe01.nif` で採っている**）。合計 +3.4MB で新形状
ゼロなので、署名軸はここで打ち切る。**32/32 は目標ではない。**

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
壊れやすい領域なので、この 4 本はオラクル（`oracle/`）にも入れてある。この領域を
触るときはこの 4 本のダンプを先に見ること。

## 限界

**この fixture は「選ばれた 84 本」の構造（型が読める）とバイト一致しか証明しません。**
実コーパスは 8032 本あるので、ファイル被覆は **84 / 8032 (1.0%)** です。fixture が緑＝
実コーパス全体を正しくパースできる、ではありません。型は 119/119 でも、ファイルは
1% です。この 2 つの数字は常に並べて読むこと。

## 再生成

```
python scripts/nif_fixture_select.py \
    --corpus tools/BSAFileExtractor/extracted/meshes \
    --manifest docs/evidence/fixtures/nif/manifest.txt \
    --copy-to docs/evidence/fixtures/nif \
    --max-file-bytes 2097152 --limit-bytes 6291456 \
    --add scripts/data/nif_layout_add.txt
```

決定的（候補パス昇順・同点パス順）なので毎回同じ manifest が出ます。`--corpus` は
gitignore 済みのローカル展開（`tools/BSAFileExtractor/extracted/meshes`）です。
`--add` のリストは `scripts/nif_layout_select.py` が生成します（手で編集しない）:

```
python scripts/nif_layout_select.py \
    --manifest docs/evidence/fixtures/nif/manifest.txt \
    --out scripts/data/nif_layout_add.txt --extra-bytes 262144
```

除外対象は manifest の markers＋型被覆節に固定してあるので、レイアウト分を足した後に
再実行しても同じ結果が出ます。除外が 0 件なら exit 2（効いていないのに気づかないのを防ぐ）。

## CI

```
python scripts/nif_coverage.py --corpus docs/evidence/fixtures/nif \
    --expect-corpus-types 119 --expect-corpus-files 84 \
    --min-impl 119 --max-missing 0 --max-stray 0
```

`--expect-corpus-types` / `--expect-corpus-files` は fixture が痩せた（ファイルが消えた・
型が減った）ことを検知します。B 全体（8032 files / 119 types）での計測はローカルで
回し、CI の数字と混ぜないこと。

## オラクル（型被覆だけでなくバイト一致）

**正典は `oracle/`（このディレクトリ）**。元 C++ パーサ（dump_orig）の出力で、リポジトリに
固定した期待値です。`markers/` / `cover/` を**鏡写し**にした構成で、`fixtures/nif/{markers,cover}`
の**全 NIF（84 本）**に対応します（期待値の無い NIF があれば FAIL）。

`tmp/nif_oracle` は**ハーネスのビルド置き場**（`dump_rust`/`dump_orig`/`orig_src`）であって、
オラクルの正典ではありません。その `out_orig`/`out_rust` はローカルの等価性ゲート専用で、
coverage の分母には使いません。

```
bash scripts/build_nif_oracle.sh      # harness（tmp/nif_oracle に dump_rust を作る）
python scripts/nif_oracle_check.py    # 不一致・欠落で exit 2。--update で焼き直し
```

`nif_coverage.py` の見出しは**2 軸**です。型が満点でもファイル被覆は別軸で、100% だけ出すと
「実コーパス全体を比較した」と誤読されます。`--corpus-total` に実コーパス B の本数を渡すと
ファイルの分母が B になります（fixture を分母にしない）:

```
oracle types = 119 / 119 (100.0%)  |  oracle files = 84 / 8032 (1.0%)
   (fixture corpus = 84 files / corpus-total = 8032)
```

- `--min-oracle 119`: 型（種類）のラチェット。上限に達しているので、今後は動かない。
- `--min-oracle-files 84`: **ファイル本数のラチェット（第 2 軸）**。オラクルを本数で広げる
  ときはこちらを上げる。型が満点でもファイル被覆は増やせる。

`NiKeyframeController` は oracle に居るがこの meshes コーパスには無い（`known_out_of_corpus.txt`）。
残り型・供給元の対応は `manifest.txt` の supply map を参照。
