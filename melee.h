#pragma once
// ============================================================================
// melee.h — Combat system
//
// Direct port of MDL melee.mud.  The original MDL combat works as follows:
//
//   Each combat round:
//     1. Player attacks the villain with their wielded weapon
//        → roll dice to determine hit/miss/unconscious/killed
//     2. If villain is alive, villain attacks player
//        → similar roll
//     3. If player health <= 0, KILLED
//     4. If villain strength <= 0, it is defeated
//
// Weapon effectiveness table (from MDL WEAPONS vector):
//   Each weapon has: { name, prob-of-hit, damage-when-hit, alternates... }
//
// The MDL PROB function: <PROB X Y> rolls a random chance X out of Y
// ============================================================================

#include "types.h"

namespace melee {

// Initialize melee system (called once from world::init)
void init();

// Main combat round: player attacks villain
// Called from doAttack() in actions.cpp
// Returns true if combat ended (villain dead or player fled)
bool attack(ObjId villain, ObjId weapon);

// Villain's turn to attack player (called from room-each handler)
// Called each turn the villain is alive in the player's room
void villainAction(ObjId villain);

// Throw a weapon at a target (called from doThrowAt)
bool throwWeapon(ObjId weapon, ObjId target);

// Outcome of a combat round
enum class MeleeResult {
    MISS,
    NEAR_MISS,      // "The axe barely misses your head."
    STAGGER,        // villain staggers; gives player a bonus
    LIGHT_WOUND,    // 1 hp damage
    SERIOUS_WOUND,  // 2 hp damage
    KILL,           // instant death
    UNCONSCIOUS,    // villain goes unconscious
};

// Get result of a strike with a weapon of given strength
MeleeResult rollAttack(int attackerStrength, int defenderStrength, bool attackerHasBonus);

} // namespace melee
