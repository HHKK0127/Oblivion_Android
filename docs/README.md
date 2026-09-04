# Oblivion Android - ドキュメント目次

**最終更新**: 2026-09-04
**バージョン**: 1.6.0
**ステータス**: Phase 63 完了

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

### 履歴ドキュメント

| ファイル | 内容 | 対象者 |
|---------|------|--------|
| [DEVELOPMENT_HISTORY.md](DEVELOPMENT_HISTORY.md) | 開発履歴（全フェーズ） | 全員 |

---

## ドキュメント読了順序

### 新規開発者向け

1. **[README.md](../README.md)** (ルート) - プロジェクト概要
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

1. **[README.md](../README.md)** - プロジェクト概要
2. **DEVELOPMENT_HISTORY.md** - 開発進捗
3. **[RELEASE_NOTES.md](../RELEASE_NOTES.md)** - リリース変更点
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

---

## プロジェクト構成

### ソースコード

```
app/src/main/cpp/
├── engine/          # レンダリング、カメラ、シェーダー、Imperial Weave
├── game/            # NPC、戦闘、クエスト、魔法
├── world/           # セル、ドア、ワールド管理
├── assets/          # NIF、DDS、アセットマネージャー
├── audio/           # オーディオシステム（OpenAL-Soft）
├── ui/              # テキスト、デバッグHUD、設定UI、UIパネル
├── jni/             # JNIブリッジ
├── physics/         # Jolt Physics統合
├── ai/              # Radiant AI（スケジューラ、パッケージ）
├── animation/       # アニメーションプレーヤー、サブスクライバー
├── save_system/     # セーブマネージャー
├── system/          # 設定管理
├── localization/    # 多言語対応
├── profiling/       # パフォーマンス監視
├── include/         # GLM、stb_image等
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
└── DEVELOPMENT_HISTORY.md       # 開発履歴
```

---

## 現在のステータス

### バージョン情報

- **現在バージョン**: 1.6.0
- **現在フェーズ**: Phase 63 (最終統合完了)
- **目標**: リリースビルド

### 完了フェーズ

| フェーズ | バージョン | 主要成果 |
|---------|----------|---------|
| 1-28 | 0.1.0-0.9.10 | コアエンジン、ESM統合 |
| 29-36 | 0.9.10-1.1.0 | NAVM、Jolt Physics、Radiant AI |
| 37-41 | 1.2.0-1.6.0 | Script VM、クエスト、ダイアログ、バイナリセーブ |
| 42-49 | 2.0.0-2.4.0 | ゲームループ、UI/UX、最適化、テスト |
| 50-57 | 2.5.0-3.2.0 | LOD、SpeedTree、FaceGen、Bink、Gamebryo |
| 58-63 | 1.1.0-1.6.0 | アセット最適化、圧縮、リリース準備 |

---

## よくある質問 (FAQ)

**Q: どのドキュメントから読み始めればよい?**
A: ルートのREADME.mdから始めてください。全体像を理解した後、ARCHITECTURE.mdでシステム構造を把握します。

**Q: アセット統合の方法は?**
A: ASSET_GUIDE.mdを参照してください。BSA展開、Androidプロジェクトへの配置方法を詳細に説明しています。

**Q: オーディオシステムの実装状況は?**
A: AUDIO_SYSTEM.mdを参照してください。OpenAL-Soft統合、JNI Audio Bridge、EventBus連携の詳細があります。

**Q: 開発履歴を確認したい**
A: DEVELOPMENT_HISTORY.mdを参照してください。Phase 1からPhase 63までの全開発履歴が記載されています。

---

## 参考資料

### 公式ドキュメント
- [Android JNI Tips](https://developer.android.com/training/articles/perf-jni)
- [OpenGL ES 3.0](https://www.khronos.org/registry/OpenGL-Refpages/es3/)
- [Android NDK Guide](https://developer.android.com/ndk/guides)
- [OpenAL-Soft](https://openal-soft.org/)

### 関連ファイル
- [../README.md](../README.md) - メインドキュメント
- [../Handbook.md](../Handbook.md) - 開発ガイドライン
- [../CHANGELOG.md](../CHANGELOG.md) - 完全な変更履歴
- [../RELEASE_NOTES.md](../RELEASE_NOTES.md) - リリース変更点要約

---

**最終更新**: 2026-09-04
**ステータス**: ドキュメント統合完了
