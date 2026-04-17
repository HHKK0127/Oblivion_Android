# Oblivion アセット手動抽出ガイド

自動抽出に問題が発生した場合の手動ガイドです。

## ステップ 1: ISO をマウント

### Windows 10/11 での方法（推奨 - 最も簡単）

1. **エクスプローラーで ISO ファイルを見つける**
   - `I:\Closet\Oblivion\Oblivion GOTY 1.iso`

2. **右クリック → マウント**
   - ISO がドライブとしてマウントされます（例：D:\）

3. **ドライブの内容を確認**
   - マウントされたドライブを開く
   - `Data` フォルダが見えるはずです

## ステップ 2: 必要なフォルダをコピー

### コピー元
```
[マウントドライブ]:\Data\meshes\furniture\      → 小さい、テスト用に最適
[マウントドライブ]:\Data\meshes\architecture\   → ドア、窓などの建築物
[マウントドライブ]:\Data\textures\furniture\    → 対応するテクスチャ
[マウントドライブ]:\Data\textures\architecture\ → 対応するテクスチャ
```

### コピー先
```
C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\meshes\
C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\textures\
```

### 手順

1. **assets フォルダを開く**
   - `C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\`

2. **meshes フォルダを開く（なければ作成）**
   - 右クリック → 新規 → フォルダ
   - 名前: `meshes`

3. **textures フォルダを作成**
   - 同様に `textures` フォルダを作成

4. **マウントドライブから家具メッシュをコピー**
   ```
   [マウント]\Data\meshes\furniture → assets\meshes\furniture
   ```
   - Ctrl+C（コピー）
   - Ctrl+V（ペースト）

5. **対応するテクスチャもコピー**
   ```
   [マウント]\Data\textures\furniture → assets\textures\furniture
   ```

6. **（オプション）ドアメッシュもコピー**
   ```
   [マウント]\Data\meshes\architecture\doors → assets\meshes\doors
   [マウント]\Data\textures\architecture\doors → assets\textures\doors
   ```

## ステップ 3: ISO をアンマウント

1. **エクスプローラーで ISO ドライブを右クリック**
2. **取り出す（Eject）をクリック**

## 確認

コピー後、以下のコマンドで確認できます：

```powershell
# PowerShell で実行
$assetsPath = 'C:\Users\E1192\Projects\oblivion-android\app\src\main\assets'
Get-ChildItem -Path $assetsPath -Recurse -Filter '*.nif' | Measure-Object | Select-Object Count
Get-ChildItem -Path $assetsPath -Recurse -Filter '*.dds' | Measure-Object | Select-Object Count
```

## 予想されるコピー量

- **furniture** フォルダ: 約 100-200 MB
- **doors** フォルダ: 約 50-100 MB
- **合計**: 200-400 MB

これなら数分でコピーできます。

## トラブルシューティング

### ISO がマウントできない場合
- Windows 7 以前: 7-Zip で抽出（複雑）
- Windows 10/11: システムの更新が必要かもしれません

### マウント後、Data フォルダが見えない場合
- ISO ファイルが破損している可能性
- 別の ISO ファイル（GOTY 2.iso）を試してください

### コピーが遅い場合
- ネットワークドライブを使用していないか確認
- USB 3.0 接続か確認

---

完了後、Phase 2 統合テストに進むことができます。
