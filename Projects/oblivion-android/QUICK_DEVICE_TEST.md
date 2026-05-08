# デバイステスト クイックスタート

**所要時間**: 約20分

---

## 📋 事前準備

### 必要な環境
- ✅ Android Studio がインストール済み
- ✅ Android デバイス（USB接続 またはワイヤレス接続）
- ✅ 開発者オプションが有効

### 確認事項
```bash
# adb が認識されているか
adb devices

# 期待される出力:
# List of attached devices
# xxxxxxxxxxxxx           device
```

---

## 🚀 実行手順

### 方法 A: 自動スクリプト（推奨）

```bash
# プロジェクトルートで実行
bash RUN_DEVICE_TEST.sh
```

完全に自動化されます。

---

### 方法 B: 手動ステップ

#### ステップ 1️⃣: APK をビルド

```bash
cd プロジェクトルート
./gradlew clean assembleDebug
```

**出力例:**
```
> Task :app:compileDebugNdk
> Task :app:linkDebug
> Task :app:assembleDebug

BUILD SUCCESSFUL in 2m 34s
```

#### ステップ 2️⃣: デバイスに接続

**USB 接続の場合:**
```bash
adb devices
```

**ワイヤレス接続の場合:**
```bash
adb connect デバイスIP:5555
```

**確認:**
```bash
adb devices
# デバイスが表示されることを確認
```

#### ステップ 3️⃣: APK をインストール

```bash
adb uninstall com.example.oblivion 2>/dev/null || true
adb install -r app/build/outputs/apk/debug/oblivion-debug.apk
```

**成功時:**
```
Success
```

#### ステップ 4️⃣: ログを監視

```bash
adb logcat -c
adb logcat -v time | grep -E "NPC|AIState|Dialogue|Combat|ERROR"
```

#### ステップ 5️⃣: アプリを起動

**ターミナルで実行:**
```bash
adb shell am start -n "com.example.oblivion/com.example.oblivion.MainActivity"
```

**または手動:**
- デバイスのアプリドロワーから "Oblivion" をタップ

#### ステップ 6️⃣: テストを実行

以下の操作を順に行い、各操作後のログを確認:

### 📋 テストチェックリスト

```
[  ] 1. アプリ起動
     期待: ログに "NPC created" が表示される
     確認: ゲーム画面が正常に表示される

[  ] 2. NPC の移動観察
     期待: NPC が移動を開始する
     ログ: "[AIStateMachine] NPC X transitioned: 0 -> 1"
     操作: プレイヤーがワールド内を移動

[  ] 3. NPC との会話
     期待: 会話メニューが表示される
     ログ: "[DialogueSystem] Conversation started with NPC"
     操作: NPC に近づいてインタラクション

[  ] 4. 会話オプション選択
     期待: 別のテキストが表示される
     ログ: "[DialogueSystem] NPC X: Selected option 0"
     操作: 異なるオプションを選択

[  ] 5. 会話終了
     期待: 会話メニューが閉じる
     ログ: "[DialogueSystem] Conversation ended with NPC"
     操作: "さようなら" を選択 または戻る

[  ] 6. 敵との戦闘
     期待: NPC が敵と戦闘する
     ログ: "[Combat]" または "[NPC] selectSpellForCombat"
     操作: NPCが居る状態で敵に遭遇

[  ] 7. スペル使用確認
     期待: NPC が魔法を使用
     ログ: "[NPC] NPC X priority: DAMAGE (selected spell"
     観察: マナが消費される

[  ] 8. クラッシュチェック
     期待: FATAL エラーがない
     実行: 上記を複数回繰り返す
     確認: ログに "FATAL" や "CRASH" がない
```

---

## 📊 ログの見方

### 正常なログの例

```
05-03 10:15:32.456  1234  5678 I AIStateMachine: AIStateMachine created
05-03 10:15:32.789  1234  5678 I DialogueSystem: Dialogue tree registered: 1000 (6 nodes)
05-03 10:15:35.123  1234  5678 D NPC: NPC created: ID=1001, Name=Warrior
05-03 10:15:40.456  1234  5678 I AIStateMachine: NPC 1001 transitioned: 0 -> 1
05-03 10:15:45.789  1234  5678 I DialogueSystem: Conversation started with NPC 1000
05-03 10:15:50.123  1234  5678 D DialogueSystem: NPC 1000: Selected option 0 -> node quest_offer
```

### エラーログの例

```
❌ FATAL/CRASH を含むログ
05-03 10:20:15.456  1234  5678 F libc: FATAL SIGNAL 11 (SIGSEGV), code 1, fault addr 0x0

⚠️ 異常な ERROR
05-03 10:25:30.123  1234  5678 E NPC: Failed to load NPC data

✓ 警告は許容（多くない限り）
05-03 10:30:45.789  1234  5678 W DialogueSystem: No dialogue tree found for NPC 9999
```

---

## 🔍 トラブルシューティング

### アプリがクラッシュしてしまう

```bash
# 詳細なクラッシュログを取得
adb logcat | grep -A 5 "FATAL\|CRASH\|Exception"

# ネイティブクラッシュの場合は、以下を確認
adb logcat | grep "signal 11\|SIGSEGV\|abort"
```

**対応:**
1. CMakeLists.txt を確認（全ファイルが含まれているか）
2. 最新のビルドキャッシュをクリア
3. `./gradlew clean assembleDebug` を再実行

### デバイスが認識されない

```bash
# USB デバッグを有効にしているか確認
adb devices

# 認識されない場合:
adb kill-server
adb start-server
adb devices
```

### ログが表示されない

```bash
# ログバッファをクリア
adb logcat -c

# ログレベルを確認
adb logcat -v time

# 特定のタグのみ表示
adb logcat -v time | grep "NPC"
```

---

## 📈 パフォーマンス計測

### FPS 確認

**Android デバイスで:**
1. 設定 → 開発者向けオプション → GPU のレンダリング プロファイル
2. 「画面に統計情報を表示」を有効にする
3. ゲーム画面左上に FPS が表示される
4. 期待値: **60 FPS 安定**

### メモリ使用量

```bash
# テスト開始時
adb shell dumpsys meminfo | grep com.example.oblivion

# 出力例:
# Native Heap: 15,234 KB
# Dalvik Heap: 45,678 KB
# Total: 60,912 KB (期待値: < 200MB)
```

### CPU 使用率

```bash
# テスト中
adb shell top -n 1 | grep oblivion

# または
adb shell dumpsys cpuinfo | grep oblivion
```

---

## ✅ テスト完了チェック

全テストが成功するための条件:

```
[ ] クラッシュなし (FATAL/CRASH = 0)
[ ] エラーなし (ERROR = 0)
[ ] 警告は少ない (WARN < 5)
[ ] FPS が 60 (または 安定)
[ ] メモリ < 200MB
[ ] NPC AI が動作している
[ ] 会話が正常に機能している
```

すべてにチェックできれば **テスト成功** ✅

---

## 📝 テスト結果の記録

テスト完了後、以下を記録してください:

```
デバイス: [Xiaomi / Fire / その他]
Android版: [バージョン番号]
テスト日時: [日時]

テスト結果:
- アプリ起動: [ PASS / FAIL ]
- NPC AI: [ PASS / FAIL ]
- 会話機能: [ PASS / FAIL ]
- 戦闘スペル: [ PASS / FAIL ]
- クラッシュ: [ なし / あり ]
- FPS: [ 60 / 不安定 / その他 ]
- メモリ: [ 正常 / 警告 ]

問題があれば詳細を記述:
[ここに入力]
```

---

## 🎯 次のステップ

テストが成功したら:

1. ✅ ログファイルを保存
2. ✅ テスト結果を記録
3. ✅ 問題があれば報告
4. ➡️ ゲームループ統合に進む

テストが失敗したら:

1. ❌ ログから原因を特定
2. ❌ コードを修正
3. ❌ 再度テスト実行
4. ➡️ 問題が解決するまで繰り返す

---

## 📞 サポート

トラブルが発生した場合:

1. **ログファイルを確認**: device_test_*.log
2. **ログメッセージを検索**: ERROR や FATAL
3. **詳細をメモ**: デバイス、Android版、操作内容
4. **報告**: ログと詳細情報を記録

