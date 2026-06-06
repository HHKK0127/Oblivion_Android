# Oblivion Android - ドキュメント目次

このディレクトリには、Oblivion Android 移植プロジェクトの JNI ブリッジ設計に関するドキュメントが含まれています。

---

## ドキュメント一覧

### 1. **JNI_BRIDGE_DESIGN.md** - メイン設計書
   - **内容**: JNI ブリッジアーキテクチャの詳細設計
   - **対象者**: アーキテクト、リードエンジニア
   - **セクション**:
     - システム概要
     - システムアーキテクチャ図
     - Java/Kotlin 側の設計
     - C++ 側の JNI ブリッジ
     - 通信プロトコル
     - ライフサイクル管理
     - エラーハンドリング戦略
     - メモリ管理
     - 実装フロー
     - テスト方針

   **ポイント**: このドキュメントは、プロジェクト全体のアーキテクチャを把握するための出発点です。最初にこれを読んでください。

---

### 2. **IMPLEMENTATION_GUIDE.md** - 実装ガイド
   - **内容**: 実装時の詳細手順、ベストプラクティス、トラブルシューティング
   - **対象者**: 実装エンジニア、デバッグ担当者
   - **セクション**:
     - セットアップ手順（CMakeLists.txt, build.gradle）
     - JNI 関数シグネチャ
     - スレッドセーフティ戦略
     - デバッグテクニック
     - ベストプラクティス
     - トラブルシューティング

   **ポイント**: 実装時、デバッグ時、問題発生時に参照してください。

---

### 3. **JNI_QUICK_REFERENCE.md** - クイックリファレンス
   - **内容**: コマンド、テンプレートコード、スニペット集
   - **対象者**: すべての開発者
   - **セクション**:
     - コマンドライン操作
     - JNI 関数テンプレート
     - Kotlin コードスニペット
     - デバッグマクロ
     - データ型変換
     - エラーコード
     - adb コマンド集

   **ポイント**: 頻繁に参照するドキュメント。ブックマークに登録すると便利です。

---

## ソースコード構成

### Java/Kotlin ファイル

```
app/src/main/java/com/example/oblivion/
├── OblivionEngine.kt           # JNI ブリッジ (シングルトン)
├── GameActivity.kt             # メイン Activity
├── InputHandler.kt             # タッチ入力処理
└── OblivionException.kt        # カスタム例外
```

**重要なクラス**:
- `OblivionEngine`: ネイティブメソッド定義、スレッドセーフなラッパー
- `GameActivity`: Android ライフサイクル管理
- `InputHandler`: マルチタッチ入力処理

### C++ ファイル

```
app/src/main/cpp/
├── jni/
│   ├── com_example_oblivion_OblivionEngine.h     # JNI ブリッジ定義
│   ├── com_example_oblivion_OblivionEngine.cpp   # JNI 実装
│   └── jni_utils.h                              # ユーティリティ
├── engine/
│   ├── Engine.h                # メインエンジンクラス定義
│   ├── Engine.cpp              # エンジン実装
│   └── (その他のサブシステム)
└── CMakeLists.txt              # ビルド設定
```

**重要なクラス**:
- `OblivionEngineJNI`: JNI → C++ のゲートウェイ
- `Engine`: ゲームエンジンのメインクラス

---

## クイックスタート

### 1. 環境セットアップ

```bash
# プロジェクトのクローン
git clone <repository>
cd oblivion-android

# 依存関係のインストール
./gradlew clean build
```

### 2. ドキュメント読了順序

1. **JNI_BRIDGE_DESIGN.md** を読んで全体像を理解
2. **IMPLEMENTATION_GUIDE.md** で実装方法を確認
3. **JNI_QUICK_REFERENCE.md** で必要なコマンド・コードを検索

### 3. 実装開始

```bash
# ビルド
./gradlew assembleDebug

# デバイスにインストール
adb install -r app/build/outputs/apk/debug/app-debug.apk

# ログを見ながらテスト
adb logcat -s OblivionEngine OblivionEngineJNI | head -100
```

---

## ファイル説明表

| ファイル | 用途 | 開発者向け | PM向け |
|---------|------|----------|--------|
| JNI_BRIDGE_DESIGN.md | システムアーキテクチャ | 必読 | 必読 |
| IMPLEMENTATION_GUIDE.md | 実装ガイド | 必読 | 参考 |
| JNI_QUICK_REFERENCE.md | 開発支援 | 頻読 | 参考 |
| OblivionEngine.kt | JNI ラッパー | 実装対象 | - |
| GameActivity.kt | メイン Activity | 実装対象 | - |
| com_example_oblivion_OblivionEngine.cpp | JNI 実装 | 実装対象 | - |
| Engine.h/cpp | エンジンコア | 実装対象 | - |

---

## 設計の主要ポイント

### アーキテクチャの特徴

1. **シングルトンパターン**
   - `OblivionEngine` はシングルトンとして実装
   - グローバルにアクセス可能、インスタンスは一つ

2. **スレッドセーフ設計**
   - `ReentrantReadWriteLock` で読み取り多重化
   - ホットパス（タッチイベント）での優適化
   - Condition Variable でスレッド同期

3. **例外安全性**
   - JNI 層で全例外をキャッチ
   - Java 側で `OblivionException` として再スロー
   - エラーコードとメッセージのペアで返却

4. **リソース管理**
   - RAII パターンで自動クリーンアップ
   - `unique_ptr` で所有権明確化
   - メモリリーク検出テスト実装

5. **ライフサイクル管理**
   - 状態マシンで厳密なステート遷移
   - Surface 生成・破棄時のタイミング制御
   - Render thread の安全な終了

---

## 開発フロー

### Phase 1: セットアップ
- [ ] ドキュメント読了
- [ ] 開発環境構築
- [ ] ビルド確認

### Phase 2: 基本実装
- [ ] OblivionEngine.kt 完成
- [ ] GameActivity.kt 実装
- [ ] JNI ブリッジ実装
- [ ] 単体テスト

### Phase 3: Vulkan 統合
- [ ] Engine.h/cpp 実装
- [ ] Vulkan 初期化
- [ ] Surface 管理
- [ ] レンダリングループ

### Phase 4: 入力・ライフサイクル
- [ ] InputHandler 実装
- [ ] タッチイベント処理
- [ ] Pause/Resume
- [ ] 統合テスト

### Phase 5: 最適化・本番化
- [ ] パフォーマンス測定
- [ ] メモリリーク検査
- [ ] ストレステスト
- [ ] リリース準備

---

## トラブルシューティングフロー

```
問題が発生
  ↓
IMPLEMENTATION_GUIDE.md の「トラブルシューティング」を確認
  ↓
該当する項目あり? 
  ├─ YES → 解決策を実行
  └─ NO → 次へ
  ↓
JNI_QUICK_REFERENCE.md でコマンド・コード例を検索
  ↓
問題解決 OR ドキュメント更新要望を報告
```

---

## 参考資料リンク

### 公式ドキュメント
- [Android JNI Tips](https://developer.android.com/training/articles/perf-jni)
- [Vulkan Quick Start](https://www.khronos.org/vulkan/how-to-guide)
- [Android NDK Guide](https://developer.android.com/ndk/guides)

### 関連ファイル
- `app/build.gradle` - Gradle ビルド設定
- `app/src/main/cpp/CMakeLists.txt` - CMake ビルド設定
- `gradle.properties` - グローバルプロパティ

---

## よくある質問 (FAQ)

**Q: どのドキュメントから読み始めればよい?**  
A: `JNI_BRIDGE_DESIGN.md` から始めてください。全体像を理解した後、`IMPLEMENTATION_GUIDE.md` で詳細を確認します。

**Q: コードのサンプルはどこにある?**  
A: `JNI_QUICK_REFERENCE.md` に Kotlin/C++ のスニペット集があります。

**Q: エラーメッセージの意味がわからない**  
A: `IMPLEMENTATION_GUIDE.md` の「トラブルシューティング」セクションで一般的なエラーと解決策を記載しています。

**Q: パフォーマンスを改善したい**  
A: `IMPLEMENTATION_GUIDE.md` の「ベストプラクティス」セクションで最適化方法を紹介しています。

---

## ドキュメント更新履歴

| 日付 | バージョン | 更新内容 |
|------|----------|--------|
| 2026-06-06 | 1.0 | 初版作成 |

---

## コントリビューション

このドキュメントを改善するための提案は以下の形式で報告してください:

1. **誤り の報告**: 正確な箇所と修正案を記述
2. **内容追加**: 不足している内容とその理由を記述
3. **例示更新**: より良い例やサンプルコードを提案

---

**最後に更新**: 2026-06-06  
**作成者**: Claude Code  
**ステータス**: ドキュメント完成
