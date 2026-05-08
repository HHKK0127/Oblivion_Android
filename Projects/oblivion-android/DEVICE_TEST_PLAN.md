# NPC AI システム - デバイステスト計画

**テスト対象**: NPC AI System (Phase 2-4 Implementation)  
**テスト環境**: 
- Xiaomi Miui 15 (Android 16)
- Amazon Fire OS (Android 9)

**テスト日**: 2026-05-03

---

## テスト実行ステップ

### ステップ 1: ビルド生成 🔨

```bash
# プロジェクトルートで実行
./gradlew clean assembleDebug

# 確認
ls -la app/build/outputs/apk/debug/*.apk
```

**期待**: 
- ✓ CMakeLists.txt のすべてのファイルが含まれる
- ✓ コンパイルエラーなし
- ✓ APK サイズ: 15-25 MB

---

### ステップ 2: デバイスへのデプロイ 📲

#### Xiaomi Miui 15 へのインストール

```bash
# デバイス接続確認
adb devices

# APK インストール
adb install -r app/build/outputs/apk/debug/oblivion-debug.apk

# ログストリーム開始
adb logcat -v time | grep -E "NPC|AIState|Dialogue|Combat"
```

#### Amazon Fire へのインストール

```bash
# Amazon Fire に接続
adb connect <fire-device-ip>:5555

# インストール
adb -s <device-id> install -r app/build/outputs/apk/debug/oblivion-debug.apk

# ログ確認
adb -s <device-id> logcat -v time
```

---

## テストシナリオ

### テスト 1: アプリ起動とNPC初期化 ✅

**実行手順:**
1. アプリを起動
2. タイトル画面から ゲーム開始
3. ワールドにロード

**期待される結果:**
```
[AIStateMachine] AIStateMachine created
[DialogueSystem] DialogueSystem created
[DialogueSystem] Dialogue tree registered: 1000 (6 nodes)
[DialogueSystem] Dialogue tree registered: 2000 (4 nodes)
[DialogueSystem] Dialogue tree registered: 3000 (4 nodes)
[DialogueSystem] Dialogue tree registered: 4000 (4 nodes)
[NPC] NPC created: ID=1001, Name=Warrior
[NPC] NPC created: ID=1002, Name=Mage
```

**監視対象ログ:**
- クラッシュなし
- メモリ使用量 < 100MB

**検証:**
- [ ] ゲーム起動成功
- [ ] NPCが表示される
- [ ] クラッシュなし

---

### テスト 2: NPC AI状態遷移 🤖

**実行手順:**
1. ワールドにロード後、NPC の近くに移動
2. NPCを観察して、以下の状態遷移を確認:
   - IDLE → WANDER（しばらく待機）
   - IDLE → FOLLOW_PLAYER（プレイヤーに近づく）
   - COMBAT（敵と戦闘開始）

**期待されるログ:**
```
[AIStateMachine] NPC 1001 transitioned: 0 -> 1 (event: 0)
[AIStateMachine] NPC 1001 processing event 0 (current state: 1)
[NPC] NPC AI state changed: ID=1001, State=1
```

**ビジュアル確認:**
- [ ] NPC が移動を開始する
- [ ] NPC がプレイヤーに向かって移動
- [ ] NPC が敵と戦闘する

---

### テスト 3: NPC 会話機能 💬

**実行手順:**
1. NPC に近づく
2. インタラクションボタン（例：長押し）を実行
3. 会話オプションを選択
4. 別のオプションを試す
5. 会話を終了

**期待されるログ:**
```
[DialogueSystem] Conversation started with NPC 1000 (name: Companion)
[DialogueSystem] NPC 1000: Selected option 0 -> node quest_offer
[DialogueSystem] Conversation ended with NPC 1000
```

**会話テキスト確認:**
- [ ] グリーティング表示
- [ ] オプションリスト表示
- [ ] 日本語/英語切り替え（SettingsUI）
- [ ] 別オプション選択時に別テキスト表示

---

### テスト 4: スペル選択と戦闘 ⚔️

**実行手順:**
1. NPC を敵に遭遇させる（またはプレイヤーが敵と戦闘中に NPC がいる）
2. NPC が スペルを使うまで待機（1-2秒）
3. 敵を倒す

**期待されるログ:**
```
[NPC] NPC 1002 spell selection: HP=80.0%, Mana=95.0%
[NPC] NPC 1002 priority: DAMAGE (selected spell 2000)
[NPC] NPC 1002: Selected option 0 -> node quest_offer
[Combat] NPC 1002 took damage: -45.0 HP
```

**戦闘確認:**
- [ ] NPC がスペルを実行する
- [ ] HP/マナが消費される
- [ ] クラッシュなし

---

### テスト 5: 低HP時の動作 💔

**実行手順:**
1. NPC のHP を人為的に低下させる（デバッグコマンド、または敵との戦闘）
2. NPC が20%以下のHPで治癒スペルを使うまで観察

**期待される動作:**
```
[AIStateMachine] NPC 1002 processing event 4 (HEALTH_CRITICAL)
[NPC] NPC 1002 priority: HEALING (HP critical at 18.0%)
[NPC] NPC 1002 priority: HEALING (selected spell 2001)
```

**ビジュアル確認:**
- [ ] NPC が自身に治癒スペルを使用
- [ ] HP バーが回復する
- [ ] 逃走状態に入らない（またはすぐに回復して戻る）

---

### テスト 6: メモリリークとクラッシュ 🔍

**実行手順:**
1. NPC と複数回会話する（5-10回）
2. 戦闘を複数回繰り返す
3. セルを切り替える（移動範囲外に出て、戻る）
4. ログを監視

**監視項目:**
```
Logcat 監視:
  ✓ "FATAL EXCEPTION" なし
  ✓ "ERROR" が異常に増加していない
  ✓ "Memory Warning" なし
  ✓ メモリ使用量が継続的に増加していない
```

**メモリ確認:**
```bash
# デバイスから メモリ使用量を確認
adb shell dumpsys meminfo | grep com.example.oblivion

# 期待値:
# - Native Heap: < 50MB
# - Dalvik Heap: < 100MB
# - Total: < 200MB
```

**パフォーマンス確認:**
```bash
# FPS 確認（開発者オプション有効時）
adb shell dumpsys gfxinfo | grep -E "Janky|Frame"

# 期待値: 
# - 60 FPS 安定
# - Janky フレーム < 2%
```

---

### テスト 7: エラーハンドリング ⚠️

**実行手順:**
1. 存在しないNPCとの会話を試みる（デバッグ）
2. 無効なスペルIDを使用（デバッグ）
3. ゲーム中に急激なメモリ圧力をシミュレート

**期待される動作:**
```
[DialogueSystem] LOGW("No dialogue tree found for NPC %u", npcId)
[NPC] LOGW("NPC has no spell of type %d", type)
→ クラッシュなし、適切なログ出力
```

---

## ログ収集と分析

### ログキャプチャ

```bash
# テスト開始時刻を記録
adb logcat -c  # ログクリア

# バックグラウンドでログ保存
adb logcat > device_test_log.txt &

# テスト実行（10-15分間）

# ログ停止
kill %1  # バックグラウンドプロセス終了
```

### ログ分析スクリプト

```bash
# エラー/警告の統計
grep -E "ERROR|WARN|FATAL" device_test_log.txt | wc -l

# クラッシュ検出
grep -i "exception\|crash\|fatal" device_test_log.txt

# NPC AI ログの確認
grep -E "NPC|AIStateMachine|DialogueSystem" device_test_log.txt | head -50

# メモリ警告
grep -i "memory\|out of\|OOM" device_test_log.txt
```

---

## テスト結果テンプレート

```
[デバイス名]: Xiaomi Miui 15
[Android版]: 16
[テスト日時]: 2026-05-03 10:00

テスト 1: アプリ起動 ............................ [ PASS / FAIL ]
テスト 2: NPC AI状態遷移 ....................... [ PASS / FAIL ]
テスト 3: NPC 会話機能 ......................... [ PASS / FAIL ]
テスト 4: スペル選択と戦闘 .................... [ PASS / FAIL ]
テスト 5: 低HP時の動作 ........................ [ PASS / FAIL ]
テスト 6: メモリリークとクラッシュ ........... [ PASS / FAIL ]
テスト 7: エラーハンドリング ................. [ PASS / FAIL ]

総合評価: [ PASS / FAIL ]
クラッシュ件数: 0
メモリ警告: 0
エラー数: 0

コメント:
- (テスト中の観察事項)
- (問題があれば詳細)
- (パフォーマンス観察)
```

---

## トラブルシューティング

### ビルドエラーが発生した場合

```bash
# キャッシュをクリア
./gradlew clean

# NDK バージョン確認
cat android/build.gradle | grep ndkVersion

# CMake バージョン確認
cat CMakeLists.txt | grep cmake_minimum_required
```

### APK がインストールできない場合

```bash
# 既存 APK を削除
adb uninstall com.example.oblivion

# ストレージ確認
adb shell df -h

# 再インストール
adb install -r app/build/outputs/apk/debug/oblivion-debug.apk
```

### ゲームがクラッシュする場合

```bash
# 詳細なクラッシュログを取得
adb logcat -v time | grep -E "FATAL|CRASH|Exception"

# ネイティブクラッシュの場合
adb logcat | grep -A 20 "signal 11"
```

---

## テスト完了後

1. **ログを保存**: `device_test_log_[device]_[date].txt`
2. **テスト結果を記録**: 上記テンプレートで報告
3. **問題があれば記録**: バグ番号、ログ行番号、対応方法
4. **パフォーマンス数値を記録**: FPS, メモリ使用量, CPU使用率

---

## 期待される最終結果

✅ **全テスト PASS**
- クラッシュ: 0件
- メモリリーク: 0件  
- エラーログ: 0件
- FPS: 60 (安定)
- メモリ: < 200MB
