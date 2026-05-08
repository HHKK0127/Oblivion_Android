#!/bin/bash

# ============================================================
# NPC AI System - Device Test Execution Script
# ============================================================

set -e  # エラーで停止

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     NPC AI System - デバイステスト実行スクリプト           ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# ============================================================
# ステップ 1: ビルド生成
# ============================================================

echo "📦 ステップ 1: APK ビルド生成中..."
echo ""

if [ ! -f "gradlew" ]; then
    echo "❌ エラー: gradlew が見つかりません"
    echo "   プロジェクトルートで実行してください"
    exit 1
fi

# キャッシュをクリア（推奨）
echo "🧹 ビルドキャッシュをクリア..."
./gradlew clean

# デバッグAPKをビルド
echo "🔨 APK をビルド中（時間がかかる場合があります）..."
./gradlew assembleDebug

# APK の確認
APK_PATH="app/build/outputs/apk/debug/oblivion-debug.apk"
if [ ! -f "$APK_PATH" ]; then
    echo "❌ APK ビルドに失敗しました"
    exit 1
fi

APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
echo "✅ APK ビルド完了: $APK_SIZE"
echo "   パス: $APK_PATH"
echo ""

# ============================================================
# ステップ 2: デバイス接続確認
# ============================================================

echo "📱 ステップ 2: デバイス接続確認中..."
echo ""

# adb が利用可能か確認
if ! command -v adb &> /dev/null; then
    echo "❌ エラー: adb が見つかりません"
    echo "   Android SDK をインストールしてください"
    exit 1
fi

# デバイス一覧を取得
DEVICES=$(adb devices | grep -v "^.*emulator\|List of attached\|^$" | grep "device$")
DEVICE_COUNT=$(echo "$DEVICES" | wc -l)

if [ "$DEVICE_COUNT" -eq 0 ]; then
    echo "❌ 接続されたデバイスがありません"
    echo ""
    echo "接続方法:"
    echo "  - USB ケーブルで接続"
    echo "  - または: adb connect <device-ip>:5555"
    echo ""
    echo "接続を確認してから再度実行してください"
    exit 1
fi

echo "✅ 接続されたデバイス:"
adb devices | grep device

# 最初のデバイスを選択
DEVICE=$(echo "$DEVICES" | head -1 | awk '{print $1}')
echo ""
echo "選択デバイス: $DEVICE"
echo ""

# ============================================================
# ステップ 3: APK をデプロイ
# ============================================================

echo "📲 ステップ 3: APK をデバイスにインストール中..."
echo ""

# 既存アプリをアンインストール（オプション）
echo "既存アプリをアンインストール..."
adb -s "$DEVICE" uninstall com.example.oblivion 2>/dev/null || true

# APK をインストール
if adb -s "$DEVICE" install -r "$APK_PATH"; then
    echo "✅ APK インストール完了"
else
    echo "❌ APK インストールに失敗しました"
    exit 1
fi
echo ""

# ============================================================
# ステップ 4: ログ監視開始
# ============================================================

echo "📊 ステップ 4: ログ監視を開始します..."
echo ""
echo "==================================================="
echo "ログファイル: device_test_$(date +%Y%m%d_%H%M%S).log"
echo "==================================================="
echo ""

# ログファイル名
LOG_FILE="device_test_$(date +%Y%m%d_%H%M%S).log"

# ログをクリア
adb -s "$DEVICE" logcat -c

# アプリを起動
echo "🚀 アプリを起動中..."
adb -s "$DEVICE" shell am start -n "com.example.oblivion/com.example.oblivion.MainActivity" 2>/dev/null || true
sleep 2

# ログを監視
echo ""
echo "💬 NPC AI ログを監視中（Ctrl+C で停止）..."
echo ""

adb -s "$DEVICE" logcat -v time | tee "$LOG_FILE" | grep -E "NPC|AIState|Dialogue|Combat|ERROR|WARN|CRASH" || true

# ============================================================
# ステップ 5: テスト完了
# ============================================================

echo ""
echo "==================================================="
echo "✅ デバイステスト完了"
echo "==================================================="
echo ""
echo "ログ分析:"
echo ""

if [ -f "$LOG_FILE" ]; then
    echo "🔍 クラッシュ/エラーの確認:"
    CRASH_COUNT=$(grep -i "FATAL\|CRASH\|Exception\|SIGSEGV" "$LOG_FILE" | wc -l)
    ERROR_COUNT=$(grep "ERROR" "$LOG_FILE" | wc -l)
    WARN_COUNT=$(grep "WARN" "$LOG_FILE" | wc -l)

    echo "   - FATAL/CRASH: $CRASH_COUNT"
    echo "   - ERROR: $ERROR_COUNT"
    echo "   - WARN: $WARN_COUNT"
    echo ""

    echo "📊 NPC AI イベント統計:"
    NPC_COUNT=$(grep -c "NPC" "$LOG_FILE" || true)
    AI_STATE_COUNT=$(grep -c "AIStateMachine" "$LOG_FILE" || true)
    DIALOGUE_COUNT=$(grep -c "Dialogue" "$LOG_FILE" || true)

    echo "   - NPC ログエントリ: $NPC_COUNT"
    echo "   - AIStateMachine イベント: $AI_STATE_COUNT"
    echo "   - Dialogue イベント: $DIALOGUE_COUNT"
    echo ""

    if [ "$CRASH_COUNT" -eq 0 ] && [ "$ERROR_COUNT" -eq 0 ]; then
        echo "✅ テスト成功: クラッシュやエラーがありません"
    else
        echo "⚠️  警告: エラーが検出されました"
        echo ""
        echo "詳細なログを確認: $LOG_FILE"
    fi
else
    echo "❌ ログファイルが見つかりません"
fi

echo ""
echo "==================================================="
echo "📝 テスト完了報告"
echo "==================================================="
echo ""
echo "ログを確認してください: $LOG_FILE"
echo ""
echo "次のステップ:"
echo "  1. ゲーム内で NPC と会話"
echo "  2. NPC の移動を確認"
echo "  3. 敵との戦闘でスペル使用を確認"
echo "  4. デバイステスト結果を記録"
echo ""
