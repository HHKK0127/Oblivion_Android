#pragma once

#include "npc.h"
#include "npc_manager.h"
#include "navmesh_manager.h"
#include "../world/world_manager.h"
#include <unordered_map>
#include <memory>
#include <android/log.h>

// Forward declaration for Imperial Weave
namespace weave {
    class EventBus;
}

#define LOG_TAG "CombatManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Weapon Type System
// ============================================================================

enum class WeaponType : uint8_t {
    NONE = 0,
    SWORD_ONE_HAND,    // 片手剣: 速攻、バランス型
    SWORD_TWO_HAND,    // 両手剣: 高火力、遅い
    AXE_ONE_HAND,      // 片手斧: 中火力、出血効果
    AXE_TWO_HAND,      // 両手斧: 最高火力、最も遅い
    MACE_ONE_HAND,     // 片手メイス: 防御無視
    MACE_TWO_HAND,     // 両手メイス: 防御無視、遅い
    DAGGER,            // ダガー: 最速、クリティカル率高い
    BOW,               // 弓: 遠距離
    STAFF,             // 杖: 魔法增幅
    COUNT
};

struct WeaponProperties {
    WeaponType type = WeaponType::NONE;
    float baseDamage = 0.0f;
    float attackSpeed = 1.0f;       // 攻撃速度倍率（1.0が基準）
    float range = 3.0f;             // 攻撃範囲（メートル）
    float criticalChance = 0.05f;   // クリティカル確率（5%）
    float criticalMultiplier = 2.0f; // クリティカル倍率
    float staminaCost = 10.0f;      // スタミナ消費
    bool isRanged = false;          // 遠距離武器か
    bool ignoresArmor = false;      // 防御無視（メイス系）
    float bleedChance = 0.0f;       // 出血確率（斧系）
};

// ============================================================================
// Hitbox System
// ============================================================================

struct Hitbox {
    glm::vec3 center;       // ワールド座標での中心
    glm::vec3 halfExtents;  // 半径（x, y, z）
    float damage = 0.0f;
    uint32_t ownerEntityId = 0;
    float lifetime = 0.1f;  // 存続時間（秒）
    float elapsed = 0.0f;
    bool isActive = false;
};

// ============================================================================
// Combat Event (for Imperial Weave integration)
// ============================================================================

struct CombatEvent {
    enum class Type : uint8_t {
        ATTACK_START,
        ATTACK_HIT,
        ATTACK_MISS,
        CRITICAL_HIT,
        BLOCK,
        PARRY,
        DODGE,
        DEATH,
        BLEED_APPLIED,
        MAGIC_CAST
    };

    Type type;
    uint32_t attackerId;
    uint32_t defenderId;
    uint32_t targetId;  // Alias for defenderId (for clarity)
    float damage;
    float timestamp;
    glm::vec3 hitPosition;
    WeaponType weaponType;
    std::string weaponName;
    bool isCritical = false;
    bool isBlocked = false;
};

// ============================================================================
// Combat Instance
// ============================================================================

struct CombatInstance {
    std::shared_ptr<NPC> attacker;
    std::shared_ptr<NPC> defender;
    float lastAttackTime;
    float combatDuration;
    WeaponProperties attackerWeapon;
    float distanceToTarget = 0.0f;
    bool isInAttackRange = false;
};

// ============================================================================
// CombatManager
// ============================================================================

class CombatManager {
private:
    WorldManager* worldManager;
    NpcManager* npcManager;
    class SpellManager* spellManager;
    class CheatManager* cheatManager;
    oblivion::NavMeshManager* navMeshManager;
    class weave::EventBus* eventBus = nullptr;  // Imperial Weave EventBus
    std::unordered_map<uint32_t, CombatInstance> activeCombats;

    // Hitbox management
    std::vector<Hitbox> activeHitboxes;

    // Combat event queue (for Imperial Weave)
    std::vector<CombatEvent> eventQueue;

    // Weapon database (loaded from ESM)
    std::unordered_map<uint32_t, WeaponProperties> weaponDatabase;

    static constexpr float DAMAGE_CALCULATION_COOLDOWN = 1.0f;
    static constexpr float HITBOX_LIFETIME = 0.15f;
    static constexpr float PARRY_WINDOW = 0.3f;    // パリィ受付時間
    static constexpr float DODGE_COOLDOWN = 1.0f;
    static constexpr float BLEED_DURATION = 5.0f;
    static constexpr float BLEED_DAMAGE_PER_SEC = 3.0f;

public:
    CombatManager();
    ~CombatManager();

    bool initialize(WorldManager* wm, NpcManager* nm, class SpellManager* sm = nullptr,
                   class CheatManager* cm = nullptr, oblivion::NavMeshManager* nvm = nullptr,
                   class weave::EventBus* eb = nullptr);
    void setEventBus(class weave::EventBus* eb) { eventBus = eb; }
    void cleanup();
    void update(float deltaTime);

    // Combat lifecycle
    void initiateCombat(std::shared_ptr<NPC> attacker, std::shared_ptr<NPC> defender);
    void endCombat(uint32_t defenderId);

    // Damage calculation
    float calculateDamage(const CharacterStatus& attacker, const CharacterStatus& defender,
                         const WeaponProperties& weapon);
    float getDefenderDamageMitigation(const CharacterStatus& defender);
    void applyDamage(std::shared_ptr<NPC> target, float damage);
    void applyHeal(std::shared_ptr<NPC> target, float amount);

    // Weapon system
    void registerWeapon(uint32_t weaponId, const WeaponProperties& props);
    WeaponProperties getWeaponProperties(uint32_t weaponId) const;
    WeaponProperties getDefaultWeapon(WeaponType type) const;

    // Hitbox system
    void createHitbox(const glm::vec3& position, const glm::vec3& halfExtents,
                     float damage, uint32_t ownerEntityId);
    void updateHitboxes(float deltaTime);
    bool checkHitboxCollision(const Hitbox& hitbox, std::shared_ptr<NPC> target) const;

    // Critical hit system
    bool rollCritical(float criticalChance) const;
    float applyCriticalDamage(float damage, float criticalMultiplier) const;

    // Block/Parry system
    bool attemptParry(std::shared_ptr<NPC> defender, float attackTime);
    bool attemptBlock(std::shared_ptr<NPC> defender, float incomingDamage);
    bool attemptDodge(std::shared_ptr<NPC> defender);

    // Bleed system
    void applyBleed(std::shared_ptr<NPC> target, float duration, float damagePerSec);

    // Player combat
    void playerAttack(uint32_t playerEntityId, uint32_t targetNpcId, uint32_t weaponId);

    // Find nearest enemy to player
    std::shared_ptr<NPC> findNearestEnemyToPlayer(const glm::vec3& playerPos, float detectionRadius = 30.0f);
    void playerCastSpell(uint32_t playerEntityId, uint32_t spellId, uint32_t targetNpcId);

    // Event system (Imperial Weave integration)
    void emitCombatEvent(const CombatEvent& event);
    std::vector<CombatEvent> consumeEvents();

    // Utility
    std::shared_ptr<NPC> findNearestEnemy(std::shared_ptr<NPC> npc, float detectionRadius = 30.0f);
    bool isInCombat(uint32_t npcId) const;
    const CombatInstance* getCombat(uint32_t defenderId) const;
    void clearCombats();
    void logCombatStatus() const;
};
