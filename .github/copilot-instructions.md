# GitHub Copilot Custom Instructions

## セッションメモリの共有（GitHub Copilot ↔ opencode）

本プロジェクトのセッション記憶の**正本は `.github/memory/session-memory.json` の 1 本**（プロジェクト共有台帳）。**Supermemory** は Copilot 側の書き込み先として併存する。

- **2026-10-23 台帳の一本化**: グローバル台帳 `~/.config/opencode/memory/session-memory.json` にしか無かった Oblivion の記録 16 件（`re-s50-batch2-complete-20261017` 〜 `opencode-tasksimulate-struct-20261021`）をプロジェクト台帳へ取込済み（649 → 665 件）。同台帳の他プロジェクト分 19 件（`project:io` = PixPipe）は対象外として据え置き
- **グローバル台帳の扱い**: in-repo 台帳を持たないプロジェクト専用のフォールバック。**本プロジェクトでは追記しない**（二重管理により 16 件が片側にしか存在しない状態が発生していた）。取込は `python tools/merge_memory_ledger.py --dry-run` で差分確認 → 本実行（冪等・一時ファイル + `os.replace` の原子的書込・`tmp/` へ退避・検証付き）

- **Copilot の記憶の書き込み先は Supermemory**（2026-10-22 ユーザー指示「Supermemoryを使用してください」）。スペースは `opencode_project_470935b226aa5f24`（opencode と共有）。既定スペース `sm_project_default` は空なので namespace を必ず明示する
- **`.github/memory/session-memory.json` は Copilot にとって読み取り専用**。追記しない（二重書き込みによるエントリ消失を避ける。下記「過去のデータ損失」参照）

- **作業開始時**: 全件読まず、まず grep で関連語（機能名・ID・日付）を検索して該当エントリだけ読む。未確定事項・過去の決定・RE 結果はここが正本（Copilot は読み取り専用）
- **参照例**: RE 完了記録 id `re-s50-batch2-complete-20261017`、RE の生データは `tmp/re/`、永続文書は `docs/RE_EXE_REPORT.md`（§55 補遺 → §50 の優先で矛盾解消）
- **追記手順**（opencode 側の手順。Copilot は追記しない）:
  1. 書き込みの直前に**必ずファイルを再読込**する（メモリ上のスナップショットを丸ごと書き戻さない。書き戻すと他ツールの追記が消える）
  2. `entries` 配列の末尾に `{ "id", "timestamp"(ISO8601Z), "priority"(high|medium|low), "category", "tags"[], "content" }` を append
  3. `id` は `<tool>-<topic>-<yyyymmdd>` 形式（例: `copilot-<topic>-20261019`、`opencode-b-<topic>-20261019`）
  4. `metadata.totalEntries`・`metadata.compressedCount`・`metadata.lastUpdated`、`lastUpdated`(トップレベル) を更新
  5. `python tools/validate_memory_ledger.py` で検証（JSON 妥当性・ID 重複・件数整合・必須フィールド）。エラーがあればコミットしない
- **構造**: `{ version, lastUpdated, entries[N], metadata }`。`N` は実ファイルの件数（2026-10-23 時点で 665）。書式は 2 スペースインデント・**CRLF**・末尾改行なし。`compressed: true` は要約済みエントリ
- **過去のデータ損失**: 2026-10-22 のコミット `bfb986dd` で台帳が 8,290 行で途切れた不正 JSON になり、61 件が消失した。`74ec9ad9`（622 件）から復元済みで、詳細は台帳 id `copilot-memory-restore-20261022`
- **`.github/memory/session-memory.json.bak`（Git 追跡下）**: 2026-10-23 時点で本体と**バイト一致**（`sha256 66ccb8392693`・563,699 bytes）しており、復旧用の退避としての価値は失われている。履歴には `bfb986dd` の破損スナップショットが残るため削除可能だが、追跡下ファイルの削除はユーザー確認が必要（未実施）
- **同系統のファイル**: Copilot のタスク管理 TODO は `.copilot/session-state/9427ac74-*/session.db`（`todos` テーブル）。opencode 側と互換にするため `title` 先頭へ `[copilot:ID]` タグを付ける運用

### Supermemory の位置づけ（2026-10-22 改定: Copilot の書き込み先）

- **2026-10-22 のユーザー指示により、Copilot は Supermemory に記憶を書く**（`tools/push_copilot_memory.py` 経由）。ローカル台帳への追記は行わない
- **既知の制約（未解決）**: MCP `add_memory` は LLM による抽出・要約が入るため逐語保存にならない。2026-10-22 の全件突合ではローカル 580 件に対し Supermemory 側 412 件・逐語一致 0 件だった。ローカル台帳にのみ存在する 2026-10-21 以前の記録は Supermemory から復元できない
- **ローカル台帳は opencode の正本として存続**（opencode は同ファイルへ書き込み続ける）。Copilot は grep / 読み取りで参照する
- **Copilot の記憶の書き込みコマンド**: `python tools/push_copilot_memory.py <entries.json>`（`--dry-run` で差分のみ確認、`--namespace` でスペース指定）。入力は台帳スキーマの JSON 配列（`id` / `timestamp` / `priority` / `category` / `tags` / `content`）。`metadata.entryId` を冪等キーとし、未存在は `POST /v4/memories` で逐語 push、`content` が相違する既存は `PATCH` で更新する収束同期。`metadata.source` / `metadata.ledger` は `copilot-direct` になり、台帳由来の `session-memory.json` と区別できる
- **台帳の同期コマンド**: `python tools/sync_memory_to_supermemory.py`（`--dry-run` で差分のみ確認、`--namespace` でスペース指定）。台帳の**追記（`to push`）と訂正（`to update`）の両方**を検出して反映する冪等な収束同期なので、台帳を編集した後は何度実行しても安全。差分が 0 になれば一致
- **冪等キー**: 各メモリの `metadata.entryId`（= 台帳エントリの `id`）。`content` は逐語コピー、`metadata` に `category` / `priority` / `recordedAt` / `source` / `ledger` / `compressed` / `tags` を保持
- **API キー**: `~/.copilot/mcp-config.json` の `mcpServers.supermemory.headers.Authorization` を第一候補とし、環境変数 `SUPERMEMORY_API_KEY` は第二候補。各候補を実際に認証させて通ったものを使う。**キーの値はコミットしない**
- **書き込み経路**: 逐語保存には REST `POST https://api.supermemory.ai/v4/memories` を使う（document ingestion は LLM 抽出が入り否定条件が落ちるため不可）。MCP ツールの `add_memory` は LLM 経由の要約が入るので台帳の複製には使わない
- **背景**: 2026-10-22 の全件突合でローカル 580 件に対し Supermemory 側は 412 件・逐語一致 0 件だった。台帳を捨てると 2026-10-21 以前の 537 件と制約・決定の根拠が失われるため、正本はローカルに固定した（詳細は台帳 id `copilot-supermemory-sync-policy-20261022`）。**2026-10-22 のユーザー指示で Copilot 側の書き込み先のみ Supermemory へ変更したが、上記の制約は未解決のまま残る**

## Git 操作ポリシー（厳守）

- **commit / push はユーザーの明示指示があるときのみ実行する**。定期・自動での commit / push は禁止（2026-10-23 ユーザー指示「プッシュの指示はこちらでします」「プッシュしてしまったのは仕方ないので次回から厳守してください」）
- コミット対象に `docs/RE_EXE_REPORT.md` を含める場合は、下記 RE 解析方針の「git commit はユーザーの明示承諾があるまで禁止」に従い、事前承諾を取る
- push 済みの内容を取り消す場合は `git push --force-with-lease` が必要になる。共有ブランチのため、影響を説明してユーザー確認を得てから実行する

## プロジェクト概要

**Oblivion Android** - The Elder Scrolls IV: Oblivion の完全ネイティブAndroid移植
- バージョン: 0.9.10 (Phase 64 完了、Phase 65 が次)。単一の真実源は `app/build.gradle`
- リポジトリ: HHKK0127/Oblivion_Android
- 技術スタック: C++17, OpenGL ES 3.0, Android NDK r26.1, JNI

## 言語ルール

### ユーザーコミュニケーション
- **ユーザーへの応答**: 常に日本語
- **エラーメッセージ**: 日本語
- **チャット・CLI出力**: 日本語

### コード開発
- **C++**: snake_case（変数・関数）、PascalCase（クラス）、UPPER_SNAKE_CASE（定数）
- **Kotlin/Java**: camelCase（メソッド・変数）、PascalCase（クラス）
- **ファイル名**: snake_case（C++）、PascalCase（Kotlin）
- **コメント**: 英語
- **ユーザー向け文字列**: バイリンガル（英語/日本語）

### 禁止事項
- 絵文字の使用（README、ドキュメント含む）
- 個人メールアドレスの記載
- kebab-case識別子（C++/KotlinではSyntaxError）

## プロジェクトアーキテクチャ

### コアパターン
- **Imperial Weave**: 12フェーズの更新コーディネーター
- **EventBus**: 疎結合メッセージング
- **Manager Pattern**: initialize() → update(dt) → cleanup()
- **Subscriber Pattern**: AnimationSubscriber, AudioSubscriber

### ディレクトリ構成
```
app/src/main/cpp/
├── engine/          # レンダリング、カメラ、シェーダー
├── game/            # NPC、戦闘、クエスト、魔法
├── world/           # セル、ドア、ワールド管理
├── assets/          # NIF、DDS、アセットマネージャー
├── audio/           # オーディオシステム（OpenAL-Soft）
├── ui/              # テキスト、デバッグHUD、設定UI
├── jni/             # JNIブリッジ
└── CMakeLists.txt
```

### 主要コンポーネント
- **Renderer**: OpenGL ES 3.0レンダリングループ
- **NpcManager**: 100+ NPC管理、AI状態機械
- **CombatManager**: 戦闘計算、EventBus経由でサウンド連携
- **QuestManager**: クエスト進行、報酬管理
- **AudioManager**: OpenAL-Soft 3Dオーディオ
- **SaveManager**: JSONベースのセーブ/ロード

## ドキュメント構成

### ルートディレクトリ
- `README.md` - メインドキュメント（英語+日本語）
- `CHANGELOG.md` - 変更履歴
- `Handbook.md` - プロジェクト哲学・ガイドライン
- `plan.md` - 実装計画（Phase 9〜72、唯一の正典）

### docs/ディレクトリ
- `README.md` - ドキュメント目次
- `ARCHITECTURE.md` - システムアーキテクチャ
- `IMPLEMENTATION_GUIDE.md` - JNI実装ガイド
- `JNI_BRIDGE_DESIGN.md` - JNIブリッジ設計
- `ASSET_GUIDE.md` - アセット統合ガイド
- `AUDIO_SYSTEM.md` - オーディオシステム
- `DEVELOPMENT_HISTORY.md` - 開発履歴
- `RE_EXE_REPORT.md` - RE解析の主レポート（Oblivion.exe 解析 §1〜§55。RE 関連の質問はまずこのファイルを参照）
- `RE_ANALYSIS_ROADMAP.md` - RE完全解析ロードマップ（全解析完了までの恒久タスクと方針。KPI 表・フェーズ管理）

## 一時ファイル・スクリーンショット

一時ファイルとスクリーンショットは Git 管理外の専用フォルダに保存する。GitHub にはアップロードしない。

| 種別 | 保存先 | 備考 |
|---|---|---|
| スクリーンショット（デバッグ・検証用） | `screenshots/` | `.gitignore` の `/screenshots/` で管理外 |
| 一時ファイル（ビルドログ、実行ログ、UI/XML ダンプ、ローカル APK 出力） | `tmp/` | `.gitignore` の `/tmp/` で管理外 |

- 出力先は最初から上記フォルダを指定する（`adb shell screencap` の pull 先は `screenshots/`、`./gradlew ... > tmp/build_log.txt` のリダイレクト先は `tmp/`）。リポジトリ直下に `*.png` / `*.jpg` / `*.jpeg` / `*.webp` / `*.xml` / `*.log` / `*.apk` を生成しない
- 散らばった既存ファイルを整理する場合は移動のみを行い、削除しない
- 追跡対象（コミット済み）のファイルを移動すると Git 上は削除扱いになる。コミットするか否かは必ずユーザーに確認する

## 開発ガイドライン

### コードスタイル
- C++17標準
- 既存の命名規則に従う
- コメントは英語、ユーザー向け文字列はバイリンガル
- RAIIパターンでリソース管理
- `unique_ptr`で所有権を明確化

### ビルド
```bash
./gradlew clean build                    # ビルド
adb install -r app/build/outputs/apk/debug/app-debug.apk  # インストール
adb logcat -s OblivionEngine             # ログ確認
```

### テスト
- すべてのオリジナル機能はオプション強化なしで動作必須
- オプション機能は独立したテストカバレッジが必要
- Android 9+でのデバイステスト必須

### RE解析方針
- RE 解析は `docs/RE_ANALYSIS_ROADMAP.md` に従って継続する。完成の定義 (L1〜L3) の達成まで全対象の解析を進め、§ 完了ごとに `docs/RE_EXE_REPORT.md` と KPI 表を更新する
- 生データは `tmp/re/`、永続文書は `docs/`。git commit はユーザーの明示承諾があるまで禁止
- RE 関連の質問・実装時は、まず `docs/RE_EXE_REPORT.md` を読む（正典）。内容の矛盾時は §55 補遺 → §50 の優先順で解決する
- 便2 (B1〜B13 / C1〜C6 / A5 / A6) の最終レポートは `tmp/re/s50_*.md`（11 本: a5/dmg/input/magic_stack/gmst/container/audio/sleep_events/menus_a6/menus_c6/b13_plan）。証跡は `tmp/re/s50_*.txt` / `*.py`、共有ヘルパーは `tmp/re/re_util.py`（変更禁止）
- `tmp/` は `.gitignore` 対象のため、GitHub Web / クラウド Copilot から参照できるのはコミット済みの `docs/` と `.github/` のみ（VS Code ローカルでは `tmp/re/` も直接参照可能）
- RE 完了記録は `.github/memory/session-memory.json`（id: re-s50-batch2-complete-20261017）。未確定残件一覧は `docs/RE_EXE_REPORT.md` §55.8

## 代理AI連携ワークフロー

### 工程1：仕様書生成フェーズ
- Markdown形式で詳細な仕様書を生成
- 以下を必ず含める：
  - 機能概要と目的
  - 入力・出力定義（型、バリデーション）
  - 技術スタック（言語、フレームワーク、DB）
  - APIエンドポイント / コンポーネント一覧（シグネチャ付き）
  - 制約条件・前提条件
- テキストとして表示（ファイル保存はユーザーが行う）

### 工程2：代理AIへの引き渡しフェーズ
- 仕様書全文をコピー可能な形で表示
- 「この仕様書をコピーして代理AIに貼り付けてください」と指示

### 工程3：コード実装フェーズ
- 代理AIから返却されたコードを**忠実に**実装
- 設計変更は行わない
- 不明な実装仕様はコード実装前にユーザーに確認

### 工程4：エラーチェックフェーズ
- 構文エラー・型エラーを検出
- 未定義変数・不正なAPI呼び出しを検出
- エラー報告形式：
  ```
  [ERROR] Type: {種別}
  File: {ファイル名}
  Line: {行番号}
  Message: {メッセージ}
  Suggestion: {修正案}
  ```

## 禁止事項

- 根拠なく実装を進めない
- ユーザーへの確認なしに仕様書を変更しない
- 不明点を放置して先に進めない
- 代理AIのコード案を勝手に「改善」しようとしない

## 圧縮ルール

- ターン数が20ターンを超えた場合、Mini Compact を提案
- 重要な決定事項・技術的判断は完全に保持
- 完了したタスクは1行に圧縮

## 必須確認事項

以下は必ずユーザーに確認：
- 戻り値の型が未指定の場合
- 認証方式が不明な場合
- エラーハンドリング方針が不明な場合
- パフォーマンス要件が不明な場合

## パフォーマンス目標

| 指標 | 目標 | 実績 |
|------|------|------|
| FPS | 30 fps | 60 fps |
| メモリ | < 1 GB | 40 MB |
| CPU | < 10% | < 0.1% |
| APKサイズ | < 100 MB | 115.4 MiB（デバッグ、3 ABI + ローカル動画 82.9 MiB 込み。動画を除くと約 32.5 MiB） |

## 参考資料

- [docs/README.md](docs/README.md) - ドキュメント目次
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - システムアーキテクチャ
- [Handbook.md](Handbook.md) - プロジェクト哲学
- [CHANGELOG.md](CHANGELOG.md) - 変更履歴