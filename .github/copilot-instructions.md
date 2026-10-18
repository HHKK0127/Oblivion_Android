# GitHub Copilot Custom Instructions

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