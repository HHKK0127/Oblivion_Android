# ByroRedux / OpenMW-Android から移植する CI 差分

対象: 直近の停滞（**リリース 0・既定経路の実機検証 0・同じ機能の二重実装**）を、既に先に出している
プロジェクトの作法で埋める。本書は「何を学ぶか」と「どこが違うか」を分け、そのまま移植できる差分を示す。

実在確認は 2026-11-23 に GitHub / GitLab API で実施（`[データ]`）。

---

## 1. 参照先の実在確認（`[データ]`）

| 系統 | リポジトリ | 状態 | 最終更新 | 学ぶ点 |
|---|---|---|---|---|
| 同型（Morrowind 再実装） | `Andiweli/OpenMW-Android` | 活動中（68★, Kotlin） | 2026-11-23 | **配布**: GitHub Release に APK 添付（`0.51.0-15/16/17` を数日間隔） |
| 同型（旧定番） | `xyzz/openmw-android` | **archived（2021-10-31 で停止）** | — | 個人移植は息切れする、という警告 |
| 同型（再現ビルド） | `modding-openmw/openmw-android-docker`（GitLab） | 活動低調（33★） | 2026-08-18 | **Docker で再現可能なビルド** → Release に APK（2.7.2/2.7.3/2.7.4） |
| 競合 | Winlator（x86 エミュ） | — | — | 「Oblivion を Android で遊ぶ」だけなら Winlator で足りる。README で差別化を明記すべき |
| 同系統（Unity 再実装） | `Interkarma/daggerfall-unity` | 活動中（3591★） | 2026-11-13 | **元データ非同梱モデル**・MOD エコシステム・厳格なバージョン管理 |
| 同系統（C++ 再実装） | `afritz1/OpenTESArena` | 活動中（1459★） | 2026-11-22 | 少人数で細く長く続ける規模感 |
| **本命（Gamebryo/Creation を Rust+C++ で再構築）** | `matiaszanolli/ByroRedux` | 活動中（45★, Rust） | **2026-11-23（当日）** | **ゲート設計とリリース**（後述） |
| 本命の分家 | `ForrestOgden/ByroRedux_Fallout3` | 活動中 | 2026-10-20 | iPhone 移植の分家まで出ている |
| フォーマット層 | `niftools/nifskope`（活動中）/ `stephen-bunn/bethesda-structs`（**archived**）/ `llde/xOBSE`（活動中） | — | — | NIF のデバッグ表示 / BSA ハッシュ照合 / **`#169` の実名取込の判断材料** |

**結論**: 実在するのは 9 件中 9 件。ただし **`xyzz/openmw-android` と `bethesda-structs` は archived**。
読むべきは **ByroRedux（技術とゲート）と OpenMW-Android（配布）の 2 本**。

---

## 2. ByroRedux の構成（`[データ]` 実読）

### 2.1 ワークフロー 5 本

| ファイル | 役割 |
|---|---|
| `ci.yml`（32KB） | 通常の CI（`cargo test --workspace`） |
| **`real-data-gates.yml`**（15.5KB） | **実データを使うパース網羅率ゲート**。NIF コーパスの `#[ignore]` ハーネスを走らせる |
| **`playable-smoke.yml`**（5.7KB） | **プレイ可能スモーク**。`xvfb-run` で headless 実行 |
| **`release.yml`**（7.2KB） | **タグ起動のリリース**。ドライラン可 |
| `rt-correctness.yml` | レイトレの正しさ |

### 2.2 `release.yml` の作法（そのまま移植できる）

- トリガ: タグ `v0.*` ＋ `workflow_dispatch`（入力: release name / publish bool / runner）。
- **パッケージ前に 2 つのゲート**:
  1. `Require a green CI run on this commit`（このコミットの CI が緑であること）
  2. `Require the tagged commit to be on main`（タグが main 上にあること）
- **GPU なしでバイナリをスモークテスト**（`Smoke-test the binaries without a GPU`）。
- `Package` → **`SHA256SUMS`** を同梱。
- リリースノートに**検証手順**（`sha256sum -c SHA256SUMS`、`--version` がタグを返すこと）。
- **ドライラン**: `workflow_dispatch` かつ `publish: false` なら release ではなく artifact として上げる。
- 公開は **pre-release**。
- 実績: `v0.107`（11-03）→ `v0.117`（11-23）。**約 3 週間で 5 リリース**、毎回 `SHA256SUMS` 付き。

### 2.3 `real-data-gates.yml` の作法（ゲート設計の核心）

- コメントに**正直な現状**が書かれている: 「登録済みラベルの runner が無いので **MANUAL-ONLY**。schedule は無効化してある。夜間に queue に積まれて“通った”と誤認されるのを防ぐため」。
- **なぜ必要か**が実例付き: 「`ci.yml` は `cargo test --workspace` を回すが、`#[ignore]` されたハーネスは実行しない。`#3918` が 296 個の `NiSkinPartition` ブロックを落としたまま **CI 緑で 2 日間 main に居座った**。落ちたブロック名と差分を出すゲートが呼ばれていなかった」。
- 対策:
  - **タイトルごとに 1 ジョブ**（赤ならどのゲームか分かる）。
  - **`BYROREDUX_REQUIRE_GAME_DATA=1`** ＝「コーパスが無い」を**失敗**にする（黙って ok にしない）。
  - **実行後にフィルタが 1 件以上マッチしたかを検査**（空振り緑を防ぐ）。

### 2.4 `playable-smoke.yml` の作法

- **ゲームとゲートでパラメータ化**（`inputs.game` / `inputs.gate`）。
- `xvfb-run -a --server-args="-screen 0 1280x720x24"` で headless。
- ゲートは `docs/smoke-tests/<gate>.sh`、フィクスチャは `docs/smoke-tests/fixtures/<GAME>.env`。
- 別途 `scripts/check-playable-smoke-contracts.sh` で**スモークの契約**（何を検査すべきか）を機械検査。
- スクリーンショットを artifact として公開。

---

## 3. こちらの現状との差分

| 項目 | ByroRedux / OpenMW-Android | こちら | 差分 |
|---|---|---|---|
| リリース | タグ起動で APK/バイナリを Release 添付 | **0 本**。CI は debug APK を artifact（14 日保持）のみ | **`release.yml` が無い** |
| チェックサム | 毎回 `SHA256SUMS` | 無し | 配布の作法が無い |
| 実機スモーク | headless スモークをリリース前に必須 | **既定経路の実機検証 0** | **スモークゲートが無い** |
| 実データゲート | コーパス必須・空振り緑を検査 | oracle ハーネスは**ローカルのみ**（`tmp/*_oracle`） | CI から呼ばれていない |
| 空振りの扱い | データ欠如＝失敗 | 未定義 | 黙って緑になり得る |
| 再現ビルド | Docker コンテナ | ローカル Gradle＋NDK | 任意（後回し可） |

---

## 4. 移植する差分（具体）

### 4.1 `.github/workflows/release.yml`（新規・本書と同時に作成）

`release.yml` を追加した。要点:

- トリガ: タグ `v*` ＋ `workflow_dispatch`（`publish` / `release_name` 入力、既定はドライラン）。
- パッケージ前ゲート: **①このコミットの CI が緑 ②タグが master 上**（ByroRedux と同じ 2 条件）。
- `assembleRelease` をビルド（署名が無い場合は debug を使う旨をコメントで明示）。
- **エミュレータによる headless スモーク**（`docs/smoke-tests/newgame.sh`）を通す。
- **`SHA256SUMS`** を生成して同梱。
- リリースノートに検証手順（`sha256sum -c SHA256SUMS`）を書く。
- ドライランは artifact、`publish=true` で **pre-release**。

### 4.2 `docs/smoke-tests/newgame.sh`（新規・本書と同時に作成）

ByroRedux の `docs/smoke-tests/<gate>.sh` に相当。**既定経路が成立しているか**を機械判定する:

- アプリを起動し、logcat に `LoadingScreen auto-hidden (scenario built)` が出るまで待つ（＝ESM ロード＋シナリオ構築が通った）。
- スクリーンショットを `artifacts/` に保存（**ゴールデン画像比較の材料**）。
- 出なければ**失敗**（タイムアウト）。「データが無いので ok」を許さない。

### 4.3 追加する（次段）: `real-data-gates.yml`

- 既存の `tmp/esm_oracle` / `tmp/nif_oracle` / `tmp/bsa_oracle` / `tmp/dds_oracle` を**ゲート化**する。
- コーパスごとに 1 ジョブ。`OBLIVION_REQUIRE_GAME_DATA=1` 相当で**データ欠如＝失敗**。
- 実行後に「フィルタが 1 件以上マッチしたか」を検査（空振り緑の防止）。
- データを持つ runner が無い間は、ByroRedux と同じく **MANUAL-ONLY ＋ コメントで理由と issue を明記**。

### 4.4 追加する（次段）: 見た目のゴールデン判定

- `newgame.sh` が撮るスクリーンショットを、原版スクショとの差分比較に置き換える。
- 項目ごとの RE 目視ではなく、**画像差分**をゲートにする（スケールするのはこちら）。
- **暫定（V688 → 解決 2026-11-24）**: エミュレータの PNG ゴールデンは **V688a park**（hosted runner に hypervisor 無し＝ubuntu は KVM 無し / macOS arm64 は HVF 無し / TCG は adb に登録されず）のため未採取のまま。代替として **host smoke（`smoke-host` ジョブ・A 案）** を追加し、その golden `docs/evidence/golden/newgame-no-esm.json` を CI linux-gnu で採取して blocking 化した（`compare_golden.py` に JSON 完全一致モードを追加）。エミュレータの `smoke` ジョブは self-hosted runner（KVM/HVF あり）が用意できるまで `continue-on-error: true` ＋ `if: github.event_name != 'push'` で残す。`--require` は**外さない**。

---

## 5. README の訴求点（Winlator 対策）

「Oblivion を Android で遊ぶ」だけなら Winlator で足りる。ネイティブ移植の存在意義を
**「低スペックで動く・タッチ最適化・起動が速い」**に置き、README に明記する。

---

## 6. 完了の定義（再掲）

「ホスト緑＋verifier 承認」ではなく、**「実機で確認済み＋master にマージ済み」**。
ホストテストは完了条件の一部であって、完了そのものではない。`docs/claims.md` の WIP 上限
（未マージ 3 本）と `scripts/check_wip_limit.py` がこれを機械的に支える。
