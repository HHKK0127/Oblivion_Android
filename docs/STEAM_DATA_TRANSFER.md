# Steam データ転送ガイド

**最終更新**: 2026-10-02
**バージョン**: 0.9.10

---

## 概要

Oblivion Android は、Steam 版 Oblivion のオリジナルデータ（BSA / ESM）を端末に取り込んで使用します。
ゲームデータの供給方法は **2 つの方式** から選択できます（デバッグパネルの **GAME DATA** → **DATA SOURCE**）。

- **方式A: APK Bundled Data（APK 内蔵）** — データを APK 内（`assets/data/`）に同梱し、起動時に展開して使用
- **方式B: Steam Data (Copied)（端末コピー・既定）** — SAF フォルダピッカーで Steam データを `filesDir/data` にコピーして使用

| 項目 | 方式A: APK 内蔵 | 方式B: Steam コピー |
|------|----------------|--------------------|
| データの置き場所 | `app/src/main/assets/data/` | 端末の `filesDir/data` |
| APK サイズ | 大きくなる（ESM だけで約 278MB） | 変わらない |
| 設定 | デバッグパネルで "APK Bundled Data" | デバッグパネルで "Steam Data (Copied)" |
| 使い分け | 小さ���必須データ（ESM 等） | 大きな BSA アーカイブ |

> 大きなデータ（数 GB の BSA）は APK の外に置く **BYO-data モデル** が推奨です。
> APK 内蔵は ESM などの小さい必須データに向いています。

---

## 必要なもの

- Steam 版 The Elder Scrolls IV: Oblivion（インストール済み）
- Android 端末（Android 10+ / API 29+）
- USB ケーブル（adb を使用する場合）

---

## 手順

### 方法A: APK 内蔵データを使う（小さい必須データ向け）

1. `app/src/main/assets/data/` に、内蔵したい `.bsa` / `.esm` を配置します。
   （例: `Oblivion.esm` のみを内蔵し、大きな BSA は方式Bで転送する運用が現実的）
2. アプリをビルド・インストールします。
3. デバッグパネル（**D** ボタン）の **GAME DATA** → **DATA SOURCE** で **APK Bundled Data** を選択します。
4. **Restart App (Load Data)** で再起動すると、`assets/data/` のファイルが `filesDir/data` へ展開され、エンジンがロードします。

> `assets/data/` 内の `.bsa` / `.esm` は `.gitignore` で管理外（BYO-data モデル）。コミットされません。

### 方法B: Steam データを端末へ転送（既定・大容量向け）

#### 1. 端末へファイルを転送

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

#### 2. アプリでデータフォルダを選択

1. アプリを起動します。
2. 左上の **D** ボタンでデバッグパネルを開きます。
3. **GAME DATA** セクションの **Select Steam Data Folder** をタップします。
4. ファイルマネージャーで、BSA / ESM を置いたフォルダを選択します。
5. コピーが自動で開始されます（進捗表示付き）。
6. 完了後、**Restart App (Load Data)** をタップしてアプリを再起動します。

#### 3. 確認

再起動後にログ（`adb logcat -s OblivionEngine`）で BSA のロード結果を確認できます。

```
[BSA] Loaded: Oblivion - Meshes.bsa
```

---

## 技術詳細

### データパス

- ゲームデータの場所: `filesDir/data`（アプリ専用領域、APK 外）
- **方式A（APK 内蔵）**: 起動時（バックグラウンド）に `assets/data/` 内の `.bsa` / `.esm` を `filesDir/data` へ展開します。同名ファイルはサイズ一致ならスキップ、不一致なら上書きします。
- **方式B（Steam コピー）**: SAF（Storage Access Framework）フォルダピッカーで選択したフォルダ内の `.bsa` / `.esm` を再帰的に検索し、`filesDir/data` へコピーします。
- どちらの方式でも、エンジンの BSA / ESM ロードは起動時に 1 回だけ実行されるため、データ準備後はアプリ再起動が必要です。

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
