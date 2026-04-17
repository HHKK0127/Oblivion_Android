# Oblivion Android - Gameplay Guide

## Quick Start

### Title Screen
1. App launches with Oblivion logo (3 seconds)
2. Menu appears: "Start Game"
3. Tap anywhere to begin

### Main Game World

**Visual**: Oblivion world rendered in 3D with NPCs and game systems  
**Controls**: Touch-based camera and interaction controls

## Basic Controls

### Camera Movement
- **Drag Screen**: Move camera (look around)
- **Two-Finger Pinch**: Zoom in/out (future version)
- **Tap NPC**: Interact with nearby character

### Current Limitations
- Text-based menu system
- No direct movement controls (yet)
- Camera-only touchscreen interaction

## Game Systems

### 1. Combat System

**How to Fight**:
1. Encounter enemy NPC or hostile creature
2. System auto-initiates combat when nearby
3. Attacks happen automatically based on equipped weapon
4. Damage calculated from:
   - Attacker's Strength + Weapon Damage
   - Defender's Armor Rating
   - Combat Skill level

**Combat Status**:
- Watch NPC health decrease with each attack
- NPC defeated when health reaches 0
- Loot dropped from defeated enemies (future)

**Combat Skills** (Oblivion schools):
- Blade (one-handed weapons)
- Blunt Weapons (maces, axes, hammers)
- Archery (bows)
- Destruction Magic (offensive spells)
- Restoration Magic (healing)
- Mysticism (utility magic)

### 2. Quest System

**Finding Quests**:
1. Interact with NPC (tap to select)
2. Check available quests offered by that NPC
3. View quest objective and rewards

**Quest Types**:
- **Kill Monsters**: Defeat specific enemy
- **Collect Items**: Gather resource for NPC
- **Explore Locations**: Visit area (future)
- **Deliver Items**: Carry item to destination

**Completing Quests**:
1. Achieve all quest objectives
2. System auto-completes when done
3. Receive rewards:
   - Gold
   - Experience points
   - Unique items

**Quest Log**:
- View all active quests in Quest UI
- Check progress on objectives
- See rewards for completion

### 3. Magic System

**Magic Schools** (6 types):

| School | Effect | Use Case |
|--------|--------|----------|
| **Destruction** | Damage enemy | Combat |
| **Restoration** | Heal character | Support |
| **Conjuration** | Summon creatures | Combat (future) |
| **Alteration** | Change environment | Utility (future) |
| **Illusion** | Invisibility, charm | Utility (future) |
| **Mysticism** | Mana recovery, utility | Support |

**Casting Spells**:
1. NPC equips spell from known spells
2. Cast during combat (auto or manual future)
3. Consumes Mana
4. Deals damage or healing based on spell

**Mana System**:
- Characters have Mana Pool (blue bar)
- Spells cost Mana to cast
- Mana recovers over time
- Intelligence + Willpower = Mana recovery rate

**Example Spells**:
- **Fireball**: 50 mana, 30 damage, Destruction school
- **Heal**: 40 mana, 50 healing, Restoration school
- **Restore Mana**: 30 mana, 40 mana recovery, Mysticism school

### 4. Character Status System

**Core Stats**:
- **Health**: Current/Maximum HP (0 = death)
- **Mana**: Current/Maximum Mana for spells
- **Stamina**: Endurance for sustained actions

**Attributes** (8 Oblivion attributes):
- Strength (melee damage)
- Intelligence (spell learning)
- Willpower (mana recovery)
- Agility (weapon accuracy)
- Speed (movement speed)
- Endurance (health, stamina)
- Personality (NPC reactions)
- Luck (critical chance)

**Skills** (Oblivion skill list):
- Combat: Blade, Blunt, Archery
- Magic: Destruction, Restoration, Mysticism
- Stealth: Sneaking, Security, Acrobatics
- Others: Armor, Speechcraft, etc.

### 5. Save/Load System (Phase 7.1 NEW)

**Game Progress Persistence**:
- Save and load your game progress anytime
- Multiple save slots available
- Preserves all game state:
  - Player position and status
  - NPC locations and health
  - Quest progress and objectives
  - World state

**How to Save**:
1. During gameplay, use Quick Save (future: Ctrl+S)
2. Game state snapshot created
3. Saved to device storage
4. Show confirmation message

**How to Load**:
1. From title screen, select Load Game
2. Choose save slot from list
3. Game restores to saved state
4. Resume from saved position

**Save Slots**:
- Unlimited save slots available
- Each save includes timestamp
- Delete old saves to free space
- Save size: ~500 KB per slot

### 6. NPC System

**NPC Behaviors**:
- **IDLE**: Standing still (default)
- **WANDER**: Moving around world
- **PATROL**: Following patrol route
- **COMBAT**: Fighting enemy/player
- **FOLLOW**: Trailing player

**NPC Interaction**:
- Tap nearby NPC to select
- View name, health, quests offered
- Initiate dialogue (future expanded)
- Accept quests
- Trade items (future)

**Named NPCs in Demo**:
- **Izar**: Warrior NPC, offers "Kill the Monster" quest
- **Hellas**: Mage NPC, offers "Collect Items" quest

### 6. Inventory System (Future)

Currently limited text-based view. Full system coming with:
- Equip/unequip weapons
- Manage spells
- Consume potions
- Drop items
- Weight/carry capacity

## Tips for Playing

### Combat Strategy
1. **Use Healing Spells**: Keep health above 50% for safety
2. **Position Awareness**: Stay away from environmental hazards (future)
3. **Upgrade Skills**: Gain experience, improve combat abilities
4. **Equip Best Gear**: Better weapons deal more damage

### Resource Management
1. **Mana Conservation**: Don't cast expensive spells recklessly
2. **Potion Usage**: Save healing potions for critical moments
3. **Gold Economy**: Save gold for future upgrades (coming)

### Quest Progression
1. **Start with Simple Quests**: Build experience before hard ones
2. **Complete Objectives**: Don't abandon quests mid-way
3. **Track Progress**: Use Quest UI to monitor goals
4. **Collect Rewards**: Receive gold and experience points

## Localization

**Language Support**:
- **English**: Default language
- **Japanese (日本語)**: Tap menu option to switch

**Supported Text** (100+ translations):
- UI menus
- NPC names
- Quest titles and descriptions
- Combat messages
- Magic spell names

## Performance Tips

### For Better FPS
- Close background apps before playing
- Reduce screen brightness (saves battery)
- Play in cool environment (prevents throttling)

### For Longer Play Sessions
- Play on charger if possible
- Monitor device temperature
- Take breaks every 30 minutes
- Expected battery drain: 1-3% per hour

## Known Limitations (See KNOWN_ISSUES.md)

- No save/load system yet
- No inventory management UI
- Limited NPC interactions
- Text-based menus only
- Single-player only
- No quest markers on map (yet)

## Future Features

Coming in Phase 7+:
- Enhanced UI with graphics
- Save game system
- Full inventory management
- Expanded NPC dialogue
- Multiple quest rewards
- Skill advancement UI
- Map system with markers
- Fast travel (Waystones)
- Character creation
- Expanded world areas

---

**Game Version**: Phase 5 Complete (M5-3 Magic System)  
**Last Updated**: 2026-04-17  
**Tested On**: Android 9, Android 16 (60 FPS stable)
