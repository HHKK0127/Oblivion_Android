#pragma once

namespace game {

// Single source of truth for the player's carrying capacity, in kilograms.
//
// Two legacy paths disagreed: game::Inventory capped at 100 kg while
// inventory::InventoryGrid (the grid the inventory UI actually enforces) capped at
// 150 kg, so the HUD reported a limit the player could exceed. 150 kg is the value
// the live UI has always used, so unifying on it changes no existing gameplay.
//
// RE note: the retail formula is still unconfirmed
// (docs/RE_ANALYSIS_ROADMAP.md: max-encumbrance-calc). Oblivion scales the limit with
// Strength (base 150 + 5 x Strength); the Strength term is deliberately not modelled
// here because it would rebalance encumbrance beyond this fix.
constexpr float MAX_CARRY_WEIGHT_KG = 150.0f;

}  // namespace game
