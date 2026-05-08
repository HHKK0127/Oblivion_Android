#include <gtest/gtest.h>
#include "../game/npc.h"
#include "../game/ai_state_machine.h"
#include "../game/dialogue_system.h"
#include "../localization/localization_manager.h"
#include <memory>
#include <iostream>

// ============================================================
// NPC AI Integration Test Suite
// ============================================================

class NPCIntegrationTest : public ::testing::Test {
protected:
    std::shared_ptr<NPC> npc1;
    std::shared_ptr<NPC> npc2;
    std::unique_ptr<AIStateMachine> stateMachine;
    std::unique_ptr<DialogueSystem> dialogueSystem;
    std::shared_ptr<LocalizationManager> localizationManager;

    void SetUp() override {
        // Create NPCs
        npc1 = std::make_shared<NPC>(1001, "Warrior");
        npc2 = std::make_shared<NPC>(1002, "Mage");

        // Initialize character status
        npc1->status.initialize(100.0f, 50.0f, 5);
        npc2->status.initialize(80.0f, 100.0f, 5);

        // Add spells
        npc1->status.knownSpells = {2000};  // Fireball
        npc1->status.equippedSpells = {2000};

        npc2->status.knownSpells = {2001, 2002, 2003};  // Heal, Mana Restore, Fortify
        npc2->status.equippedSpells = {2001, 2002, 2003};

        // Create state machine
        stateMachine = std::make_unique<AIStateMachine>();
        stateMachine->initializeDefaultTransitions();

        // Create dialogue system
        dialogueSystem = std::make_unique<DialogueSystem>();
        dialogueSystem->initializeDialogues();

        // Create and initialize localization manager
        localizationManager = std::make_shared<LocalizationManager>();
        localizationManager->initializeTranslationDatabase();
        localizationManager->setLanguage(Language::ENGLISH);
    }
};

// ============================================================
// Test 1: AIStateMachine Basic Functionality
// ============================================================

TEST_F(NPCIntegrationTest, AIStateMachineInitialState) {
    // Verify initial state is IDLE
    AIState initialState = stateMachine->getCurrentState(npc1);
    EXPECT_EQ(initialState, AIState::IDLE);

    std::cout << "✓ AIStateMachine: Initial state is IDLE" << std::endl;
}

TEST_F(NPCIntegrationTest, AIStateMachineStateTransition) {
    // Transition from IDLE to WANDER
    stateMachine->processEvent(npc1, AIEvent::PLAYER_APPROACHED);

    AIState newState = stateMachine->getCurrentState(npc1);
    EXPECT_EQ(newState, AIState::WANDER);

    std::cout << "✓ AIStateMachine: Transition IDLE → WANDER successful" << std::endl;
}

TEST_F(NPCIntegrationTest, AIStateMachineTimeTracking) {
    // Check state time
    float initialTime = stateMachine->getStateTime(npc1);
    EXPECT_EQ(initialTime, 0.0f);

    // Simulate update
    stateMachine->update(npc1, 1.0f);  // 1 second

    float newTime = stateMachine->getStateTime(npc1);
    EXPECT_GT(newTime, 0.0f);

    std::cout << "✓ AIStateMachine: Time tracking works (initial: "
              << initialTime << ", after 1s: " << newTime << ")" << std::endl;
}

// ============================================================
// Test 2: NPC Spell Selection with Protected Division
// ============================================================

TEST_F(NPCIntegrationTest, SpellSelectionNormalCase) {
    // Set up combat with enemy
    npc1->enterCombat(npc2);
    npc1->status.currentHealth = 80.0f;
    npc1->status.maxHealth = 100.0f;
    npc1->status.currentMana = 45.0f;
    npc1->status.maxMana = 50.0f;

    // Select spell (should select damage spell)
    uint32_t selectedSpell = npc1->selectSpellForCombat();
    EXPECT_EQ(selectedSpell, 2000);  // Fireball

    std::cout << "✓ Spell Selection: Selected spell 2000 (Fireball)" << std::endl;
}

TEST_F(NPCIntegrationTest, SpellSelectionWithZeroMaxHealth) {
    // Edge case: maxHealth is 0 (should not crash)
    npc1->status.maxHealth = 0.0f;
    npc1->status.currentHealth = 0.0f;

    // This should not crash due to division by zero protection
    uint32_t selectedSpell = npc1->selectSpellForCombat();

    // NPC is not alive, should return 0 (melee)
    EXPECT_EQ(selectedSpell, 0);

    std::cout << "✓ Spell Selection: Handles maxHealth=0 safely" << std::endl;
}

TEST_F(NPCIntegrationTest, HealingPriority) {
    // Set up low health scenario
    npc2->enterCombat(npc1);
    npc2->status.currentHealth = 20.0f;  // Critical (20%)
    npc2->status.maxHealth = 100.0f;
    npc2->status.currentMana = 80.0f;
    npc2->status.maxMana = 100.0f;

    // Should prioritize healing
    uint32_t selectedSpell = npc2->selectSpellForCombat();
    EXPECT_EQ(selectedSpell, 2001);  // Heal spell

    std::cout << "✓ Spell Priority: Low HP triggers healing (spell 2001)" << std::endl;
}

// ============================================================
// Test 3: DialogueSystem Basic Functionality
// ============================================================

TEST_F(NPCIntegrationTest, DialogueStartConversation) {
    // Start conversation with Companion NPC
    bool started = dialogueSystem->startConversation(npc1);

    // Companion NPC (1001) might not have dialogue tree, so this might fail
    // But let's test with a known NPC ID
    auto companion = std::make_shared<NPC>(1000, "Companion");
    bool companionStarted = dialogueSystem->startConversation(companion);

    EXPECT_TRUE(companionStarted);
    EXPECT_TRUE(dialogueSystem->isConversationActive(1000));

    std::cout << "✓ Dialogue: Started conversation with Companion NPC" << std::endl;
}

TEST_F(NPCIntegrationTest, DialogueGetCurrentNode) {
    auto merchant = std::make_shared<NPC>(2000, "Merchant");
    dialogueSystem->startConversation(merchant);

    const DialogueNode* node = dialogueSystem->getCurrentNode(2000);
    EXPECT_NE(node, nullptr);
    EXPECT_EQ(node->nodeId, "greeting");

    std::string speech = dialogueSystem->getCurrentNodeSpeech(2000);
    EXPECT_FALSE(speech.empty());

    std::cout << "✓ Dialogue: Retrieved greeting node for Merchant" << std::endl;
}

TEST_F(NPCIntegrationTest, DialogueSelectOption) {
    auto guard = std::make_shared<NPC>(3000, "Guard");
    dialogueSystem->startConversation(guard);

    // Select option 0 (help)
    bool selected = dialogueSystem->selectOption(3000, 0);
    EXPECT_TRUE(selected);

    // Check new node
    const DialogueNode* newNode = dialogueSystem->getCurrentNode(3000);
    EXPECT_NE(newNode, nullptr);
    EXPECT_EQ(newNode->nodeId, "help_response");

    std::cout << "✓ Dialogue: Selected option and transitioned to help_response" << std::endl;
}

TEST_F(NPCIntegrationTest, DialogueEndConversation) {
    auto mage = std::make_shared<NPC>(4000, "Mage");
    dialogueSystem->startConversation(mage);

    EXPECT_TRUE(dialogueSystem->isConversationActive(4000));

    bool ended = dialogueSystem->endConversation(4000);
    EXPECT_TRUE(ended);
    EXPECT_FALSE(dialogueSystem->isConversationActive(4000));

    std::cout << "✓ Dialogue: Ended conversation" << std::endl;
}

// ============================================================
// Test 4: NPC AI State + Dialogue Integration
// ============================================================

TEST_F(NPCIntegrationTest, NPCConversationState) {
    auto companion = std::make_shared<NPC>(1000, "Companion");

    // Start conversation via NPC
    bool started = companion->startConversation();
    EXPECT_TRUE(started);
    EXPECT_TRUE(companion->isInConversation());

    // End conversation
    companion->endConversation();
    EXPECT_FALSE(companion->isInConversation());

    std::cout << "✓ NPC Dialogue: Conversation state management works" << std::endl;
}

TEST_F(NPCIntegrationTest, NPCAIStateUpdate) {
    // Test NPC update cycle
    float deltaTime = 0.016f;  // ~60 FPS

    npc1->aiState = AIState::WANDER;
    npc1->status.currentHealth = 100.0f;
    npc1->status.maxHealth = 100.0f;

    // Update should not crash
    npc1->update(deltaTime);

    // Check mana regeneration
    float expectedMana = 50.0f + (50.0f * 0.1f * deltaTime);
    EXPECT_LE(npc1->status.currentMana, expectedMana + 0.1f);

    std::cout << "✓ NPC Update: Update cycle completed successfully" << std::endl;
}

// ============================================================
// Test 5: Combat System Integration
// ============================================================

TEST_F(NPCIntegrationTest, CombatInitiation) {
    EXPECT_FALSE(npc1->inCombat);

    npc1->enterCombat(npc2);

    EXPECT_TRUE(npc1->inCombat);
    EXPECT_EQ(npc1->combatTarget, npc2);
    EXPECT_EQ(npc1->aiState, AIState::COMBAT);

    std::cout << "✓ Combat: NPC1 entered combat with NPC2" << std::endl;
}

TEST_F(NPCIntegrationTest, CombatExitAndFlee) {
    npc1->enterCombat(npc2);
    npc1->status.currentHealth = 15.0f;  // Critical
    npc1->status.maxHealth = 100.0f;

    bool shouldFlee = npc1->shouldFlee();
    EXPECT_TRUE(shouldFlee);

    npc1->exitCombat();
    EXPECT_FALSE(npc1->inCombat);
    EXPECT_EQ(npc1->aiState, AIState::IDLE);

    std::cout << "✓ Combat: Flee check and exit combat work" << std::endl;
}

// ============================================================
// Test 6: Error Handling and Edge Cases
// ============================================================

TEST_F(NPCIntegrationTest, InvalidDialogueOption) {
    auto companion = std::make_shared<NPC>(1000, "Companion");
    dialogueSystem->startConversation(companion);

    // Try invalid option (out of range)
    bool selected = dialogueSystem->selectOption(1000, 999);
    EXPECT_FALSE(selected);

    std::cout << "✓ Error Handling: Invalid dialogue option rejected" << std::endl;
}

TEST_F(NPCIntegrationTest, ConversationWithoutTree) {
    auto unknownNPC = std::make_shared<NPC>(9999, "Unknown");

    // Try to start conversation without dialogue tree
    bool started = dialogueSystem->startConversation(unknownNPC);
    EXPECT_FALSE(started);

    std::cout << "✓ Error Handling: Conversation rejected for unknown NPC" << std::endl;
}

TEST_F(NPCIntegrationTest, MultipleNPCStates) {
    // Test that multiple NPCs maintain independent states
    auto npc3 = std::make_shared<NPC>(2001, "Enemy");

    stateMachine->processEvent(npc1, AIEvent::PLAYER_APPROACHED);
    stateMachine->processEvent(npc3, AIEvent::ENEMY_DETECTED);

    AIState state1 = stateMachine->getCurrentState(npc1);
    AIState state3 = stateMachine->getCurrentState(npc3);

    EXPECT_EQ(state1, AIState::WANDER);
    EXPECT_EQ(state3, AIState::COMBAT);

    std::cout << "✓ Multi-NPC: Independent state management works" << std::endl;
}

// ============================================================
// Integration Test Summary
// ============================================================

int main(int argc, char** argv) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║      NPC AI System Integration Test Suite                  ║\n";
    std::cout << "║      Testing: AIStateMachine + DialogueSystem + NPC        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
