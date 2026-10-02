# Steam データ転送ガイド

**最終更新**: 2026-10-02
**バージョン**: 0.9.10

---

## 概要

Oblivion Android は、Steam 版 Oblivion のオリジナルデータ（BSA / ESM）を端末に取り込んで使用します。
大容量データは APK の外（`filesDir/data`）に配置する **BYO-data モデル** を採用しています。

- コード・BGM・動画・UI: APK 内（`assets/`）
- ゲームデータ（BSA / ESM）: APK 外（`filesDir/data`）

---

## 必要なもの

- Steam 版 The Elder Scrolls IV: Oblivion（インストール済み）
- Android 端末（Android 10+ / API 29+）
- USB ケーブル（adb を使用する場合）

---

## 手順

### 1. 端末へファイルを転送

Steam 版の Data フォルダを探します。

```
C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Data
```

このフォルダ内の以下のファイルを端末の任意の場所（例: `/sdcard/OblivionData/`）へコピーします。

- すべての `*.bsa`（`Oblivion - Meshes.bsa` など、DLC / 拡張含む）
- `Oblivion.esm`

adb を使用する場合:

```bash
adb shell mkdir -p /sdcard/OblivionData
adb push "Oblivion - Meshes.bsa" /sdcard/OblivionData/
adb push "Oblivion.esm" /sdcard/OblivionData/
```

### 2. アプリでデータフォルダを選択

1. アプリを起動します。
2. 左上の **D** ボタンでデバッグパネルを開きます。
3. **GAME DATA** セクションの **Select Steam Data Folder** をタップします。
4. ファイルマネージャーで、BSA / ESM を置いたフォルダを選択します。
5. コピーが自動で開始されます（進捗表示付き）。
6. 完了後、**Restart App (Load Data)** をタップしてアプリを再起動します。

### 3. 確認

再起動後にログ（`adb logcat -s OblivionEngine`）で BSA のロード結果を確認できます。

```
[BSA] Loaded: Oblivion - Meshes.bsa
```

---

## 技術詳細

### データパス

- ゲームデータの場所: `filesDir/data`（アプリ専用領域、APK 外）
- SAF（Storage Access Framework）フォルダピッカー���選択したフォルダ内の `.bsa` / `.esm` を再帰的に検索し、`filesDir/data` へコピーします。
- コピー後はアプリ再起動が必要です（エンジンの BSA ロードは起動時に 1 回だけ実行されます）。

### 対応ファイル

| 拡張子 | 説明 |
|--------|------|
| `.bsa` | アーカイブ（メッシュ・テクスチャ・サウンド等） |
| `.esm` | マスターファイル（ワールドデータ） |

### ライセンス

Bethesda のゲームデータは再配布できません。本機能はユーザーが所有する Steam 版データを端末へ転送するためのものであり、ゲームデータ自体はリポジトリに含めません（`.gitignore` に `.bsa` / `.esm` / `.esp` を明記）。

---

## トラブルシューティング

### コピーが遅い

数 GB のデータは転送に時間がかかります。進捗表示に従い、完了までお待ちください。

### データが読み込まれない

1. 選択したフォルダに `.bsa` / `.esm` が正しく存在するか確認します。
2. **Restart App (Load Data)** でアプリを再起動します。
3. ログで `BSA not found (optional)` の有無を確認します。

### フォルダが選択できない

端末のファイルマネージャーからアクセス可能な場所（ダウンロード、共有ストレージ等）にデータを配置してください。
