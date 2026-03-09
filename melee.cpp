// ============================================================================
#include <algorithm>
// melee.cpp — Combat system implementation
//
// Faithfully ported from MDL melee.mud.
//
// The MDL combat system uses a probability table for each weapon, with entries
// for: probable outcome, kill chance, stagger chance, etc.
// We replicate this with a WeaponStats table indexed by ObjId.
// ============================================================================

#include "melee.h"
#include "world.h"
#include "game.h"
#include "actions.h"
#include <cstdlib>
#include <ctime>

// ---------------------------------------------------------------------------
// Weapon statistics
// Mirrors the MDL WEAPONS global in melee.mud
//   { hitChance (0-100), damage (1-N), killChance (0-100) }
// ---------------------------------------------------------------------------
struct WeaponStats {
    const char* name;
    int hitChance;   // % chance of any hit at all
    int damage;      // damage range 1..damage (uniform)
    int killChance;  // % chance of instant kill (rare)
};

// Hit messages (indexed by MeleeResult)
static const char* sPlayerHitMsgs[] = {
    "Your %s misses the %s by a mile.",
    "The %s gives a mighty swing but misses.",
    "The %s staggers but recovers.",
    "The %s receives a light wound.",
    "The %s is seriously wounded!",
    "The %s is killed!",
    "The %s is knocked unconscious!",
};

static const char* sVillainHitMsgs[] = {
    "The %s's %s misses you.",
    "The %s nearly takes your head off, but misses.",
    "You stagger back from the blow.",
    "The %s gives you a light wound.",
    "You are seriously wounded!",
    "The %s has killed you with a mighty blow!",
    "You are knocked unconscious!",
};

// Death messages (player is killed)
static const char* sDeathMessages[] = {
    "It appears that last blow was too much for you. I'm afraid you are dead.",
    "You have died. Better luck next time.",
    "Well, it seems that the adventure is over. "
    "As you take your last breath, you reflect on the wonderful "
    "places you've been and the (not-so-wonderful) fate you have met.",
};

// ---------------------------------------------------------------------------
// Random utility (mirrors MDL RANDOM function)
// ---------------------------------------------------------------------------
static int randRange(int lo, int hi) {
    // Returns integer in [lo, hi] inclusive
    return lo + (std::rand() % (hi - lo + 1));
}

static bool prob(int percent) {
    return (std::rand() % 100) < percent;
}

// ---------------------------------------------------------------------------
// Get the weapon stats for the player's wielded weapon
// ---------------------------------------------------------------------------
static WeaponStats getWeaponStats(ObjId weapon) {
    switch (weapon) {
    case SWORD:  return { "elvish sword",  65, 3, 2 };
    case KNIFE:  return { "nasty knife",   50, 2, 1 };
    case AXE:    return { "bloody axe",    60, 4, 3 };
    case STILET: return { "stiletto",      70, 2, 1 };
    case OBJ_NONE:
    default:     return { "your hands",    30, 1, 0 };
    }
}

// ---------------------------------------------------------------------------
// MeleeResult rollAttack
// ---------------------------------------------------------------------------
melee::MeleeResult melee::rollAttack(int atkStr, int defStr, bool atkHasBonus) {
    int hitBase = 40 + atkStr * 10 - defStr * 5;
    if (atkHasBonus) hitBase += 20;
    hitBase = std::max(5, std::min(95, hitBase));

    if (!prob(hitBase)) {
        // Miss
        return (prob(20)) ? MeleeResult::NEAR_MISS : MeleeResult::MISS;
    }
    // Hit — determine severity
    int severity = randRange(1, 10) + atkStr - defStr;
    if (severity <= 0)  return MeleeResult::MISS;
    if (severity <= 3)  return MeleeResult::LIGHT_WOUND;
    if (severity <= 6)  return MeleeResult::SERIOUS_WOUND;
    if (severity <= 8)  return MeleeResult::STAGGER;
    if (prob(15))       return MeleeResult::KILL;
    if (prob(30))       return MeleeResult::UNCONSCIOUS;
    return MeleeResult::SERIOUS_WOUND;
}

// ---------------------------------------------------------------------------
// melee::attack — player attacks villain
// ---------------------------------------------------------------------------
bool melee::attack(ObjId villain, ObjId weapon) {
    Object& v = gObjects[villain];

    if (!world::isAccessible(villain)) {
        game::tell("I don't see that here.");
        return false;
    }
    if (!v.hasFlag(VICBIT) && !v.hasFlag(VILLAIN)) {
        game::tellf("Attacking the %s accomplishes nothing.", v.desc);
        return false;
    }
    if (v.hasFlag(SLEEPBIT)) {
        // Attacking a sleeping villain wakes it
        v.clrFlag(SLEEPBIT);
        game::tellf("You attack the sleeping %s!", v.desc);
    }

    // Resolve weapon
    WeaponStats ws;
    if (weapon != OBJ_NONE) {
        if (!gObjects[weapon].isCarried()) {
            game::tell("You aren't holding that weapon.");
            return false;
        }
        ws = getWeaponStats(weapon);
        gAdventurer.weapon = weapon;
    } else if (gAdventurer.weapon != OBJ_NONE) {
        ws = getWeaponStats(gAdventurer.weapon);
        weapon = gAdventurer.weapon;
    } else {
        ws = getWeaponStats(OBJ_NONE);  // bare hands
    }

    bool playerHasBonus = v.hasFlag(STAGGERED);
    MeleeResult result  = rollAttack(ws.damage, v.strength, playerHasBonus);

    // Apply result to villain
    switch (result) {
    case MeleeResult::MISS:
        game::tellf("Your %s misses the %s.", ws.name, v.desc);
        break;
    case MeleeResult::NEAR_MISS:
        game::tellf("Your %s barely misses the %s.", ws.name, v.desc);
        break;
    case MeleeResult::STAGGER:
        v.setFlag(STAGGERED);
        game::tellf("Your %s staggers the %s.", ws.name, v.desc);
        break;
    case MeleeResult::LIGHT_WOUND:
        v.clrFlag(STAGGERED);
        v.strength -= 1;
        game::tellf("The %s receives a light wound.", v.desc);
        if (v.strength <= 0) goto villainDead;
        break;
    case MeleeResult::SERIOUS_WOUND:
        v.clrFlag(STAGGERED);
        v.strength -= 2;
        game::tellf("The %s is seriously wounded!", v.desc);
        if (v.strength <= 0) goto villainDead;
        break;
    case MeleeResult::KILL:
        v.strength = 0;
        goto villainDead;
    case MeleeResult::UNCONSCIOUS:
        v.setFlag(SLEEPBIT);
        game::tellf("The %s is knocked unconscious!", v.desc);
        break;
    }

    // Now the villain attacks back (if still alive and not unconscious)
    if (!v.hasFlag(SLEEPBIT) && v.strength > 0) {
        villainAction(villain);
    }

    return true;

villainDead:
    {
        // Villain is dead
        game::tellf("The %s is dead!", v.desc);
        // Drop villain's carried weapons/treasure
        ObjId drops[8]; int nd = world::objectsIn(villain, drops, 8);
        for (int i = 0; i < nd; ++i) {
            world::moveTo(drops[i], gAdventurer.loc);
            game::tellf("The %s drops a %s.", v.desc, gObjects[drops[i]].desc);
        }
        // Remove villain from game (move to limbo)
        world::moveTo(villain, ROOM_NONE);

        // Score bonus for killing a villain
        gAdventurer.score += 10;
        return true;
    }
}

// ---------------------------------------------------------------------------
// melee::villainAction — villain attacks player
// Called from room-each handler while villain is alive
// ---------------------------------------------------------------------------
void melee::villainAction(ObjId villain) {
    Object& v = gObjects[villain];
    if (v.hasFlag(SLEEPBIT) || v.strength <= 0) return;
    if (v.loc != gAdventurer.loc) return;

    // Find villain's weapon
    ObjId vWeapon = OBJ_NONE;
    ObjId buf[8]; int n = world::objectsIn(villain, buf, 8);
    for (int i = 0; i < n; ++i) {
        if (gObjects[buf[i]].hasFlag(WEAPONBIT)) { vWeapon = buf[i]; break; }
    }

    WeaponStats vs = getWeaponStats(vWeapon);
    MeleeResult result = rollAttack(v.strength, 3 /* player's base defense */, false);

    switch (result) {
    case MeleeResult::MISS:
    case MeleeResult::NEAR_MISS:
        game::tellf("The %s's %s misses you.", v.desc, vs.name);
        break;
    case MeleeResult::STAGGER:
        game::tellf("The %s staggers you with a glancing blow.", v.desc);
        gAdventurer.health -= 1;
        break;
    case MeleeResult::LIGHT_WOUND:
        game::tellf("The %s gives you a light wound with the %s.", v.desc, vs.name);
        gAdventurer.health -= 1;
        break;
    case MeleeResult::SERIOUS_WOUND:
        game::tellf("The %s seriously wounds you with the %s!", v.desc, vs.name);
        gAdventurer.health -= 2;
        break;
    case MeleeResult::KILL:
        gAdventurer.health = 0;
        break;
    case MeleeResult::UNCONSCIOUS:
        gAdventurer.health -= 3;
        game::tell("You are knocked unconscious!");
        break;
    }

    // Troll special: if player tries to flee, troll gets extra attack
    // (handled by checking if player moved in GO)

    if (gAdventurer.health <= 0) {
        game::tell(sDeathMessages[std::rand() % 3]);
        game::killed(v.desc);
    }
}

// ---------------------------------------------------------------------------
// melee::throwWeapon
// ---------------------------------------------------------------------------
bool melee::throwWeapon(ObjId weapon, ObjId target) {
    if (!gObjects[weapon].isCarried()) {
        game::tell("You aren't holding that.");
        return false;
    }
    if (!gObjects[weapon].hasFlag(WEAPONBIT)) {
        game::tellf("The %s is not a suitable throwing weapon.", gObjects[weapon].desc);
        world::moveTo(weapon, gAdventurer.loc);
        return true;
    }

    WeaponStats ws = getWeaponStats(weapon);
    Object& t = gObjects[target];

    // The throw removes the weapon from inventory regardless of outcome
    world::moveTo(weapon, gAdventurer.loc);

    if (!prob(50)) {
        game::tellf("The %s misses the %s and clatters to the floor.", ws.name, t.desc);
        return true;
    }

    // Hit!
    game::tellf("The %s strikes the %s!", ws.name, t.desc);
    t.strength -= ws.damage;
    if (t.strength <= 0) {
        game::tellf("The %s is killed!", t.desc);
        world::moveTo(target, ROOM_NONE);
        gAdventurer.score += 10;
    } else {
        game::tellf("The %s is wounded but still standing.", t.desc);
    }
    return true;
}

// ---------------------------------------------------------------------------
// melee::init
// ---------------------------------------------------------------------------
void melee::init() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}
