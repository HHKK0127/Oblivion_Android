#include "com_example_oblivion_OblivionEngine.h"
#include "../engine/Engine.h"
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <mutex>
#include <unordered_map>

#define LOG_TAG "OblivionJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Thread-safe engine instance management
static std::mutex engineMutex;
static std::unordered_map<jlong, std::unique_ptr<oblivion::Engine>> engines;
static jlong nextEngineHandle = 1;

// Legacy: Static engine instance (deprecated)
std::unique_ptr<oblivion::Engine> OblivionEngineJNI::sEngine = nullptr;

// Helper: Get engine from handle
static oblivion::Engine* getEngineFromHandle(jlong handle) {
    if (handle == 0) return nullptr;
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = engines.find(handle);
    return (it != engines.end()) ? it->second.get() : nullptr;
}

// JNI_OnLoad
jint JNI_OnLoad(JavaVM* /* vm */, void* /* reserved */) {
    LOGI("JNI_OnLoad called");
    return JNI_VERSION_1_6;
}

// JNI Methods
extern "C" {

JNIEXPORT jlong JNICALL Java_com_example_oblivion_OblivionEngine_nativeInitialize(
    JNIEnv* env,
    jobject /* obj */,
    jobject surface,
    jboolean enableValidation) {

    LOGI("nativeInitialize called (enableValidation=%d)", enableValidation);

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        LOGE("Failed to get ANativeWindow from Surface");
        return 0;
    }

    auto engine = std::make_unique<oblivion::Engine>();

    oblivion::InitParams params;
    params.androidApp = nullptr;
    params.nativeWindow = window;

    LOGI("Calling engine->init()...");
    if (!engine->init(params)) {
        LOGE("Failed to initialize engine");
        ANativeWindow_release(window);
        return 0;
    }

    // Store engine in thread-safe map
    {
        std::lock_guard<std::mutex> lock(engineMutex);
        jlong handle = nextEngineHandle++;
        engines[handle] = std::move(engine);
        LOGI("Engine initialized successfully with handle %ld", handle);
        return handle;
    }
}

JNIEXPORT jlong JNICALL Java_com_example_oblivion_OblivionEngine_nativeCreate(
    JNIEnv* /* env */,
    jobject /* obj */,
    jobject androidApp) {

    LOGI("nativeCreate called");

    if (!OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine = std::make_unique<oblivion::Engine>();

        oblivion::InitParams params;
        params.androidApp = androidApp;
        params.nativeWindow = nullptr;

        if (!OblivionEngineJNI::sEngine->init(params)) {
            LOGE("Failed to initialize engine");
            OblivionEngineJNI::sEngine.reset();
            return 0;
        }
    }

    return reinterpret_cast<jlong>(OblivionEngineJNI::sEngine.get());
}

JNIEXPORT jlong JNICALL Java_com_example_oblivion_OblivionEngine_nativeGetHandle(
    JNIEnv* /* env */,
    jobject /* obj */) {

    if (OblivionEngineJNI::sEngine) {
        return reinterpret_cast<jlong>(OblivionEngineJNI::sEngine.get());
    }
    return 0;
}

JNIEXPORT jboolean JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnSurfaceCreated(
    JNIEnv* env,
    jobject /* obj */,
    jlong /* handle */,
    jobject surface) {

    LOGI("nativeOnSurfaceCreated called");

    if (!OblivionEngineJNI::sEngine) {
        LOGE("Engine not created");
        return JNI_FALSE;
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window from surface");
        return JNI_FALSE;
    }

    if (!OblivionEngineJNI::sEngine->onSurfaceCreated(window)) {
        LOGE("Failed to create surface");
        ANativeWindow_release(window);
        return JNI_FALSE;
    }

    // Start game loop thread
    OblivionEngineJNI::sEngine->startLoop();

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnSurfaceChanged(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */,
    jint width,
    jint height) {

    LOGI("nativeOnSurfaceChanged called: %dx%d", width, height);

    if (!OblivionEngineJNI::sEngine) {
        return JNI_FALSE;
    }

    if (!OblivionEngineJNI::sEngine->onSurfaceChanged(width, height)) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnSurfaceDestroyed(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */) {

    LOGI("nativeOnSurfaceDestroyed called");

    if (OblivionEngineJNI::sEngine) {
        // Stop game loop first
        OblivionEngineJNI::sEngine->stopLoop();
        OblivionEngineJNI::sEngine->onSurfaceDestroyed();
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativePause(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */) {

    LOGI("nativePause called - Game loop paused");

    if (OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine->pause();
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeResume(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */) {

    LOGI("nativeResume called - Game loop resumed");

    if (OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine->resume();
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnTouchEvent(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */,
    jfloat x,
    jfloat y,
    jint action,
    jint pointerId) {

    if (OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine->queueTouchEvent(pointerId, x, y, action);
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeDestroy(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong handle) {

    LOGI("nativeDestroy called (handle=%ld)", handle);

    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        LOGI("Destroying engine with handle %ld", handle);
        it->second->shutdown();
        engines.erase(it);
    } else {
        LOGE("Engine not found: handle=%ld", handle);
    }
}

// UI Control Methods

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeToggleCharacterSheet(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeToggleCharacterSheet called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->toggleCharacterSheet();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeToggleSpellbook(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeToggleSpellbook called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->toggleSpellbook();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeToggleQuestLog(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeToggleQuestLog called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->toggleQuestLog();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeCloseDialogue(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeCloseDialogue called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->closeDialogue();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeCloseShop(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeCloseShop called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->closeShop();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeStartCharacterCreation(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeStartCharacterCreation called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->startCharacterCreation();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeEndCharacterCreation(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeEndCharacterCreation called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->endCharacterCreation();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeTogglePauseMenu(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeTogglePauseMenu called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->togglePauseMenu();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeSelectQuickSlot(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint slotIndex) {

    LOGI("nativeSelectQuickSlot called: slotIndex=%d", slotIndex);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto quickSlotBar = uiManager->getQuickSlotBar();
            if (quickSlotBar) {
                quickSlotBar->selectSlot(slotIndex);
            }
        }
    }
}

JNIEXPORT jint JNICALL Java_com_example_oblivion_OblivionEngine_nativeGetSelectedQuickSlot(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeGetSelectedQuickSlot called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto quickSlotBar = uiManager->getQuickSlotBar();
            if (quickSlotBar) {
                return quickSlotBar->getSelectedSlot();
            }
        }
    }
    return -1;
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeSetCompassRotation(
    JNIEnv* /* env */,
    jobject /* obj */,
    jfloat yawDegrees) {

    LOGI("nativeSetCompassRotation called: yaw=%f", yawDegrees);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto compass = uiManager->getHudCompass();
            if (compass) {
                compass->setPlayerRotation(yawDegrees);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeAddCompassMarker(
    JNIEnv* env,
    jobject /* obj */,
    jint markerId,
    jfloat angle,
    jstring label,
    jint markerType) {

    LOGI("nativeAddCompassMarker called: id=%d, angle=%f, type=%d", markerId, angle, markerType);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto compass = uiManager->getHudCompass();
            if (compass) {
                const char* labelStr = env->GetStringUTFChars(label, nullptr);
                glm::vec3 markerColor(1.0f, 1.0f, 1.0f);

                compass->addMarker(markerId, angle, std::string(labelStr),
                    static_cast<UIHudCompass::CompassMarker>(markerType), markerColor);

                env->ReleaseStringUTFChars(label, labelStr);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeRemoveCompassMarker(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint markerId) {

    LOGI("nativeRemoveCompassMarker called: id=%d", markerId);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto compass = uiManager->getHudCompass();
            if (compass) {
                compass->removeMarker(markerId);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeClearCompassMarkers(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeClearCompassMarkers called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto compass = uiManager->getHudCompass();
            if (compass) {
                compass->clearMarkers();
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeShowFloatingText(
    JNIEnv* env,
    jobject /* obj */,
    jstring text,
    jfloat screenX,
    jfloat screenY,
    jint textType,
    jfloat duration) {

    LOGI("nativeShowFloatingText called: type=%d, duration=%f", textType, duration);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto floatingText = uiManager->getFloatingText();
            if (floatingText) {
                const char* textStr = env->GetStringUTFChars(text, nullptr);
                floatingText->addText(std::string(textStr), screenX, screenY,
                    static_cast<UIFloatingText::TextType>(textType), duration);
                env->ReleaseStringUTFChars(text, textStr);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeShowDamage(
    JNIEnv* env,
    jobject /* obj */,
    jint damage,
    jfloat screenX,
    jfloat screenY) {

    LOGI("nativeShowDamage called: damage=%d", damage);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto floatingText = uiManager->getFloatingText();
            if (floatingText) {
                std::string damageText = std::to_string(damage);
                floatingText->addText(damageText, screenX, screenY,
                    UIFloatingText::DAMAGE, 1.5f);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeShowHeal(
    JNIEnv* env,
    jobject /* obj */,
    jint healAmount,
    jfloat screenX,
    jfloat screenY) {

    LOGI("nativeShowHeal called: heal=%d", healAmount);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto floatingText = uiManager->getFloatingText();
            if (floatingText) {
                std::string healText = "+" + std::to_string(healAmount);
                floatingText->addText(healText, screenX, screenY,
                    UIFloatingText::HEAL, 1.5f);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeClearFloatingText(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeClearFloatingText called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto floatingText = uiManager->getFloatingText();
            if (floatingText) {
                floatingText->clear();
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeSetTarget(
    JNIEnv* env,
    jobject /* obj */,
    jstring name,
    jint type,
    jfloat health,
    jfloat maxHealth,
    jint level,
    jstring faction,
    jboolean isHostile) {

    LOGI("nativeSetTarget called: type=%d, health=%f/%f, level=%d, hostile=%d",
        type, health, maxHealth, level, isHostile);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto targetInfo = uiManager->getTargetInfo();
            if (targetInfo) {
                UITargetInfo::TargetData target;
                target.name = std::string(env->GetStringUTFChars(name, nullptr));
                target.type = static_cast<UITargetInfo::TargetType>(type);
                target.health = health;
                target.maxHealth = maxHealth;
                target.level = level;
                target.faction = std::string(env->GetStringUTFChars(faction, nullptr));
                target.isHostile = isHostile == JNI_TRUE;

                targetInfo->setTarget(target);

                env->ReleaseStringUTFChars(name, target.name.c_str());
                env->ReleaseStringUTFChars(faction, target.faction.c_str());
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeUpdateTargetHealth(
    JNIEnv* /* env */,
    jobject /* obj */,
    jfloat health) {

    LOGI("nativeUpdateTargetHealth called: health=%f", health);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto targetInfo = uiManager->getTargetInfo();
            if (targetInfo) {
                targetInfo->updateTargetHealth(health);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeClearTarget(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeClearTarget called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto targetInfo = uiManager->getTargetInfo();
            if (targetInfo) {
                targetInfo->clearTarget();
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeAddAction(
    JNIEnv* env,
    jobject /* obj */,
    jint actionType,
    jstring label,
    jstring description,
    jchar keyCode,
    jfloat distance) {

    LOGI("nativeAddAction called: type=%d, key=%c, distance=%f",
        actionType, static_cast<char>(keyCode), distance);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto actionPrompt = uiManager->getActionPrompt();
            if (actionPrompt) {
                const char* labelStr = env->GetStringUTFChars(label, nullptr);
                const char* descStr = env->GetStringUTFChars(description, nullptr);

                actionPrompt->addAction(static_cast<UIActionPrompt::ActionType>(actionType),
                    std::string(labelStr), std::string(descStr),
                    static_cast<char>(keyCode), distance);

                env->ReleaseStringUTFChars(label, labelStr);
                env->ReleaseStringUTFChars(description, descStr);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeClearActions(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeClearActions called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto actionPrompt = uiManager->getActionPrompt();
            if (actionPrompt) {
                actionPrompt->clearActions();
            }
        }
    }
}

JNIEXPORT jint JNICALL Java_com_example_oblivion_OblivionEngine_nativeGetActionCount(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeGetActionCount called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto actionPrompt = uiManager->getActionPrompt();
            if (actionPrompt) {
                return actionPrompt->getActionCount();
            }
        }
    }
    return 0;
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeAddEffect(
    JNIEnv* env,
    jobject /* obj */,
    jint effectId,
    jstring name,
    jint effectType,
    jfloat duration,
    jfloat r,
    jfloat g,
    jfloat b,
    jstring iconChar) {

    LOGI("nativeAddEffect called: id=%d, type=%d, duration=%f", effectId, effectType, duration);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto activeEffects = uiManager->getActiveEffects();
            if (activeEffects) {
                const char* nameStr = env->GetStringUTFChars(name, nullptr);
                const char* iconStr = env->GetStringUTFChars(iconChar, nullptr);

                glm::vec3 color(r, g, b);
                activeEffects->addEffect(effectId, std::string(nameStr),
                    static_cast<UIActiveEffects::EffectType>(effectType),
                    duration, color, std::string(iconStr));

                env->ReleaseStringUTFChars(name, nameStr);
                env->ReleaseStringUTFChars(iconChar, iconStr);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeRemoveEffect(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint effectId) {

    LOGI("nativeRemoveEffect called: id=%d", effectId);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto activeEffects = uiManager->getActiveEffects();
            if (activeEffects) {
                activeEffects->removeEffect(effectId);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeUpdateEffectDuration(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint effectId,
    jfloat newDuration) {

    LOGI("nativeUpdateEffectDuration called: id=%d, duration=%f", effectId, newDuration);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto activeEffects = uiManager->getActiveEffects();
            if (activeEffects) {
                activeEffects->updateEffectDuration(effectId, newDuration);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeClearAllEffects(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeClearAllEffects called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto activeEffects = uiManager->getActiveEffects();
            if (activeEffects) {
                activeEffects->clearAllEffects();
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeSetLevelData(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint currentLevel,
    jint totalExp,
    jint expInCurrentLevel,
    jint expNeededForNext) {

    LOGI("nativeSetLevelData called: level=%d, exp=%d/%d",
        currentLevel, expInCurrentLevel, expNeededForNext);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto levelProgress = uiManager->getLevelProgress();
            if (levelProgress) {
                UILevelProgress::LevelData data;
                data.currentLevel = currentLevel;
                data.totalExperience = totalExp;
                data.experienceForCurrentLevel = expInCurrentLevel;
                data.experienceNeededForNextLevel = expNeededForNext;
                levelProgress->setLevelData(data);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeUpdateExperience(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint totalExp,
    jint expInCurrentLevel) {

    LOGI("nativeUpdateExperience called: total=%d, current=%d", totalExp, expInCurrentLevel);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto levelProgress = uiManager->getLevelProgress();
            if (levelProgress) {
                levelProgress->updateExperience(totalExp, expInCurrentLevel);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeNotifyLevelUp(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint newLevel) {

    LOGI("nativeNotifyLevelUp called: level=%d", newLevel);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto levelProgress = uiManager->getLevelProgress();
            if (levelProgress) {
                levelProgress->notifyLevelUp(newLevel);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeUpdateSkillIncreases(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint increases) {

    LOGI("nativeUpdateSkillIncreases called: increases=%d", increases);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto levelProgress = uiManager->getLevelProgress();
            if (levelProgress) {
                levelProgress->updateSkillIncreases(increases);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeAddAlert(
    JNIEnv* env,
    jobject /* obj */,
    jstring message,
    jint alertType,
    jint priority,
    jfloat duration) {

    LOGI("nativeAddAlert called: type=%d, priority=%d, duration=%f",
        alertType, priority, duration);

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto alertNotif = uiManager->getAlertNotification();
            if (alertNotif) {
                const char* msgStr = env->GetStringUTFChars(message, nullptr);
                alertNotif->addAlert(std::string(msgStr),
                    static_cast<UIAlertNotification::AlertType>(alertType),
                    static_cast<UIAlertNotification::AlertPriority>(priority),
                    duration);
                env->ReleaseStringUTFChars(message, msgStr);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeDismissAlert(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeDismissAlert called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto alertNotif = uiManager->getAlertNotification();
            if (alertNotif) {
                alertNotif->dismissAlert();
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeClearAllAlerts(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeClearAllAlerts called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto alertNotif = uiManager->getAlertNotification();
            if (alertNotif) {
                alertNotif->clearAllAlerts();
            }
        }
    }
}

JNIEXPORT jboolean JNICALL Java_com_example_oblivion_OblivionEngine_nativeHasActiveAlert(
    JNIEnv* /* env */,
    jobject /* obj */) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto alertNotif = uiManager->getAlertNotification();
            if (alertNotif) {
                return alertNotif->hasActiveAlert() ? JNI_TRUE : JNI_FALSE;
            }
        }
    }
    return JNI_FALSE;
}

// ---- Minimap ----

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeSetMinimapPlayerPos(
    JNIEnv* /* env */,
    jobject /* obj */,
    jfloat worldX,
    jfloat worldZ,
    jfloat yawDegrees) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                minimap->setPlayerPosition(worldX, worldZ);
                minimap->setPlayerRotation(yawDegrees);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeRevealMinimapCircle(
    JNIEnv* /* env */,
    jobject /* obj */,
    jfloat worldX,
    jfloat worldZ,
    jfloat radius) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                minimap->revealCircle(worldX, worldZ, radius);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeAddMinimapMarker(
    JNIEnv* env,
    jobject /* obj */,
    jint markerId,
    jfloat worldX,
    jfloat worldZ,
    jint markerType,
    jstring label) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                const char* labelStr = env->GetStringUTFChars(label, nullptr);
                minimap->addMarker(markerId, worldX, worldZ,
                    static_cast<UIMinimap::MarkerType>(markerType),
                    std::string(labelStr));
                env->ReleaseStringUTFChars(label, labelStr);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeUpdateMinimapMarker(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint markerId,
    jfloat worldX,
    jfloat worldZ) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                minimap->updateMarker(markerId, worldX, worldZ);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeRemoveMinimapMarker(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint markerId) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                minimap->removeMarker(markerId);
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeClearMinimapMarkers(
    JNIEnv* /* env */,
    jobject /* obj */) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                minimap->clearMarkers();
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeToggleMinimap(
    JNIEnv* /* env */,
    jobject /* obj */) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                minimap->toggleVisible();
            }
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeSetMinimapSize(
    JNIEnv* /* env */,
    jobject /* obj */,
    jint mapWidth,
    jint mapHeight) {

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            auto minimap = uiManager->getMinimap();
            if (minimap) {
                minimap->setMapSize(mapWidth, mapHeight);
            }
        }
    }
}

} // extern "C"
