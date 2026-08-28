# Oblivion Android - ドキュメント目次

**最終更新**: 2026-08-27  
**バージョン**: 0.9.0  
**ステータス**: Phase 9 進行中

---

## 概要

このディレクトリには、Oblivion Android移植プロジェクトの全ドキュメントが含まれています。プロジェクトのアーキテクチャ、実装ガイド、開発履歴、各種システムの詳細を網羅しています。

---

## ドキュメント一覧

### コアドキュメント

| ファイル | 内容 | 対象者 |
|---------|------|--------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | システムアーキテクチャ全体像 | 全開発者 |
| [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) | JNI実装ガイド | 実装エンジニア |
| [JNI_BRIDGE_DESIGN.md](JNI_BRIDGE_DESIGN.md) | JNIブリッジ詳細設計 | アーキテクト |
| [JNI_QUICK_REFERENCE.md](JNI_QUICK_REFERENCE.md) | JNIクイックリファレンス | 全開発者 |

### システムドキュメント

| ファイル | 内容 | 対象者 |
|---------|------|--------|
| [ASSET_GUIDE.md](ASSET_GUIDE.md) | アセット統合ガイド | 実装エンジニア |
| [AUDIO_SYSTEM.md](AUDIO_SYSTEM.md) | オーディオシステム | 実装エンジニア |
| [FPS_CONTROL_GUIDE.md](FPS_CONTROL_GUIDE.md) | FPS制御ガイド | 実装エンジニア |
| [SAVE_LOAD_IMPLEMENTATION.md](SAVE_LOAD_IMPLEMENTATION.md) | セーブ/ロード実装 | 実装エンジニア |
| [CODE_QUALITY_IMPROVEMENTS.md](CODE_QUALITY_IMPROVEMENTS.md) | コード品質改善 | 全開発者 |

### 計画・履歴ドキュメント

| ファイル | 内容 | 対象者 |
|---------|------|--------|
| [DEVELOPMENT_HISTORY.md](DEVELOPMENT_HISTORY.md) | 開発履歴（全フェーズ） | 全員 |
| [PHASE9_PLAN.md](PHASE9_PLAN.md) | Phase 9実装計画 | PM、リードエンジニア |

---

## ドキュメント読了順序

### 新規開発者向け

1. **README.md** (このファイル) - プロジェクト概要
2. **ARCHITECTURE.md** - システムアーキテクチャ理解
3. **DEVELOPMENT_HISTORY.md** - 開発経緯の把握
4. **IMPLEMENTATION_GUIDE.md** - 実装方法の習得
5. **JNI_QUICK_REFERENCE.md** - 必要に応じて参照

### 実装エンジニア向け

1. **ARCHITECTURE.md** - 全体構造の理解
2. **ASSET_GUIDE.md** - アセット統合作業
3. **AUDIO_SYSTEM.md** - オーディオシステム実装
4. **IMPLEMENTATION_GUIDE.md** - JNI実装詳細
5. **JNI_BRIDGE_DESIGN.md** - JNI設計詳細

### PM・リード向け

1. **README.md** - プロジェクト概要
2. **DEVELOPMENT_HISTORY.md** - 開発進捗
3. **PHASE9_PLAN.md** - 今後の計画
4. **ARCHITECTURE.md** - 技術的な全体像

---

## クイックスタート

### 環境構築

```bash
# プロジェクトのクローン
git clone https://github.com/HHKK0127/Oblivion_Android.git
cd Oblivion_Android

# ビルド
./gradlew clean build

# デバイスにインストール
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

### ドキュメント参照

```bash
# ドキュメントディレクトリ
cd docs/

# 主要ドキュメント
cat README.md           # このファイル
cat ARCHITECTURE.md     # アーキテクチャ
cat DEVELOPMENT_HISTORY.md  # 開発履歴
```

---

## プロジェクト構成

### ソースコード

```
app/src/main/cpp/
├── engine/          # レンダリング、カメラ、シェーダー
├── game/            # NPC、戦闘、クエスト、魔法
├── world/           # セル、ドア、ワールド管理
├── assets/          # NIF、DDS、アセットマネージャー
├── audio/           # オーディオシステム
├── ui/              # テキスト、デバッグHUD、設定UI
├── system/          # 設定管理
├── geometry/        # メッシュ、キューブ
├── include/glm/     # 数学ライブラリ
└── CMakeLists.txt
```

### ドキュメント

```
docs/
├── README.md                    # このファイル（目次）
├── ARCHITECTURE.md              # システムアーキテクチャ
├── IMPLEMENTATION_GUIDE.md      # JNI実装ガイド
├── JNI_BRIDGE_DESIGN.md         # JNIブリッジ設計
├── JNI_QUICK_REFERENCE.md       # JNIクイックリファレンス
├── ASSET_GUIDE.md               # アセット統合ガイド
├── AUDIO_SYSTEM.md              # オーディオシステム
├── FPS_CONTROL_GUIDE.md         # FPS制御ガイド
├── SAVE_LOAD_IMPLEMENTATION.md  # セーブ/ロード実装
├── CODE_QUALITY_IMPROVEMENTS.md # コード品質改善
├── DEVELOPMENT_HISTORY.md       # 開発履歴
└── PHASE9_PLAN.md               # Phase 9計画
```

---

## 技術スタック

| コンポーネント | 技術 |
|--------------|------|
| レンダリング | OpenGL ES 3.0 |
| ネイティブコード | C++17 (NDK r26.1) |
| Java連携 | JNI (Java Native Interface) |
| オーディオ | OpenAL-Soft + Android MediaPlayer |
| ビルドシステム | CMake 3.16+ / Gradle 9.4+ |
| 対象API | Android 10+ (API 29+) |

---

## 現在のステータス

### バージョン情報

- **現在バージョン**: 0.9.0
- **現在フェーズ**: Phase 9 (最終統合)
- **目標**: v1.0.0リリース

### 完了フェーズ

- ✅ Phase 1: 基盤構築
- ✅ Phase 2: アセット管理
- ✅ Phase 3: ゲームワールド
- ✅ Phase 4: NPC＆インタラクション
- ✅ Phase 5: 戦闘＆クエスト
- ✅ Phase 6: パフォーマンス最適化
- ✅ Phase 7: リリース準備
- ✅ Phase 8: オーディオシステム

### パフォーマンス

| 指標 | 目標 | 実績 |
|------|------|------|
| FPS | 30 fps | 60 fps ✅ |
| メモリ | < 1 GB | 40 MB ✅ |
| CPU | < 10% | < 0.1% ✅ |
| APKサイズ | < 100 MB | 8.4 MB ✅ |

---

## よくある質問 (FAQ)

**Q: どのドキュメントから読み始めればよい?**  
A: このREADME.mdから始めてください。全体像を理解した後、ARCHITECTURE.mdでシステム構造を把握します。

**Q: アセット統合の方法は?**  
A: ASSET_GUIDE.mdを参照してください。ISO抽出、BSA展開、Androidプロジェクトへの配置方法を詳細に説明しています。

**Q: オーディオシステムの実装状況は?**  
A: AUDIO_SYSTEM.mdを参照してください。OpenAL-Soft統合、JNI Audio Bridge、MediaPlayer連携の詳細があります。

**Q: 開発履歴を確認したい**  
A: DEVELOPMENT_HISTORY.mdを参照してください。Phase 1からPhase 9までの全開発履歴が記載されています。

**Q: JNI実装で問題が発生した**  
A: IMPLEMENTATION_GUIDE.mdのトラブルシューティングセクションを確認してください。一般的なエラーと解決策を記載しています。

---

## コントリビューション

このドキュメントを改善するための提案は以下の形式で報告してください:

1. **誤りの報告**: 正確な箇所と修正案を記述
2. **内容追加**: 不足している内容とその理由を記述
3. **例示更新**: より良い例やサンプルコードを提案

---

## 参考資料

### 公式ドキュメント
- [Android JNI Tips](https://developer.android.com/training/articles/perf-jni)
- [OpenGL ES 3.0](https://www.khronos.org/registry/OpenGL-Refpages/es3/)
- [Android NDK Guide](https://developer.android.com/ndk/guides)
- [OpenAL-Soft](https://openal-soft.org/)

### 関連ファイル
- `app/build.gradle` - Gradleビルド設定
- `app/src/main/cpp/CMakeLists.txt` - CMakeビルド設定
- `CHANGELOG.md` - 変更履歴

---

**最終更新**: 2026-08-27  
**ステータス**: ドキュメント統合完了
