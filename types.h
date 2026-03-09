#pragma once
// ============================================================================
// types.h — Core type definitions for Zork C++ port
//
// Design principles (PDP-10 memory budget ~512 KB):
//   - All objects/rooms live in static arrays; no heap allocation for world data
//   - Strings are const char* pointing into .rodata; zero heap for text
//   - Flags packed into uint32_t (objects) and uint16_t (rooms)
//   - IDs are int16_t — supports up to 32767 entities, fits in a register pair
//   - Function pointers instead of virtual dispatch (saves vtable overhead)
// ============================================================================

#include <cstdint>
#include <cstddef>

// ---------------------------------------------------------------------------
// Null/invalid sentinels
// ---------------------------------------------------------------------------
using ObjId  = int16_t;
using RoomId = int16_t;

constexpr ObjId  OBJ_NONE  = -1;
constexpr RoomId ROOM_NONE = -1;

// Special pseudo-room IDs used as "location" for objects being worn or in limbo
constexpr RoomId ROOM_CARRIED = -2;   // object is in adventurer's hands
constexpr RoomId ROOM_WORN    = -3;   // object is being worn

// ---------------------------------------------------------------------------
// Object flag bits  (from MDL b.mud / defs.mud)
// ---------------------------------------------------------------------------
enum ObjFlags : uint32_t {
    VISBIT    = 1u << 0,   // object is visible (default state)
    READBIT   = 1u << 1,   // object is readable (has TEXT property)
    TAKEBIT   = 1u << 2,   // player may take this object
    DOORBIT   = 1u << 3,   // object is a door
    TRANSBIT  = 1u << 4,   // container is transparent
    FOODBIT   = 1u << 5,   // object is edible
    NDESCBIT  = 1u << 6,   // don't auto-describe in room listings
    DRINKBIT  = 1u << 7,   // object is drinkable
    CONTBIT   = 1u << 8,   // object is a container
    LIGHTBIT  = 1u << 9,   // object is a light source (but may be off)
    VICBIT    = 1u << 10,  // can be attacked as victim
    BURNBIT   = 1u << 11,  // object is flammable
    FLAMEBIT  = 1u << 12,  // object is currently on fire
    TOOLBIT   = 1u << 13,  // is a tool/instrument
    TURNBIT   = 1u << 14,  // can be turned / rotated
    VEHBIT    = 1u << 15,  // is a vehicle (boat, etc.)
    FINDMEBIT = 1u << 16,  // can be reached while in a vehicle
    SLEEPBIT  = 1u << 17,  // NPC is currently asleep
    SEARCHBIT = 1u << 18,  // object can be searched
    SACREDBIT = 1u << 19,  // cannot be taken/dropped (e.g. Zork trophy case)
    TIEBIT    = 1u << 20,  // can be tied to things
    CLIMBBIT  = 1u << 21,  // can be climbed
    ACTORBIT  = 1u << 22,  // is an NPC actor
    WEAPONBIT = 1u << 23,  // is a weapon
    FIGHTBIT  = 1u << 24,  // currently engaged in combat
    VILLAIN   = 1u << 25,  // is a villain (will initiate combat)
    STAGGERED = 1u << 26,  // villain is staggered (in melee)
    TRYTAKEBIT= 1u << 27,  // NPC is trying to take object
    LOCKBIT   = 1u << 27,
    NOCHKBIT  = 1u << 28,  // skip normal reachability checks
    OPENBIT   = 1u << 29,  // door/container is currently open
    TOUCHBIT  = 1u << 30,  // object has been touched/examined
    ONBIT     = 1u << 31,  // light source is switched on
};

// ---------------------------------------------------------------------------
// Room flag bits  (from MDL defs)
// ---------------------------------------------------------------------------
enum RoomFlags : uint16_t {
    RLANDBIT   = 1u << 0,  // room is on land (not water/air)
    RLIGHTBIT  = 1u << 1,  // room is always lit (no lantern needed)
    RAIRBIT    = 1u << 2,  // room is in the air
    RWATERBIT  = 1u << 3,  // room contains water / is a waterway
    RSACREDBIT = 1u << 4,  // sacred room (limits certain actions)
    RSEENBIT   = 1u << 5,  // player has visited this room before
    RONBIT     = 1u << 6,  // room "on" bit (used by specific rooms)
    RMUNGBIT   = 1u << 7,  // room state has been changed
};

// ---------------------------------------------------------------------------
// Compass directions + special movement
// ---------------------------------------------------------------------------
enum Direction : uint8_t {
    DIR_NORTH = 0,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST,
    DIR_NE,
    DIR_NW,
    DIR_SE,
    DIR_SW,
    DIR_UP,
    DIR_DOWN,
    DIR_IN,
    DIR_OUT,
    DIR_LAND,   // special: land the magic boat
    DIR_LAUNCH, // special: launch the magic boat
    DIR_COUNT,
    DIR_NONE = 0xFF
};

// ---------------------------------------------------------------------------
// Action type passed to object/room action handlers
// Mirrors the MDL verb atoms dispatched via APPLY .ACTION
// ---------------------------------------------------------------------------
enum class ActionType : uint8_t {
    // Object-directed verbs
    TAKE,
    DROP,
    OPEN,
    CLOSE,
    LOCK,
    UNLOCK,
    EXAMINE,
    READ,
    LIGHT,
    EXTINGUISH,
    BURN,
    INFLATE,
    DEFLATE,
    EAT,
    DRINK,
    WEAR,
    REMOVE,
    ENTER,
    EXIT,
    CLIMB,
    TIE,
    UNTIE,
    TURN,
    PUSH,
    PULL,
    MOVE,
    THROW,
    PUT,        // put X in Y  (Y receives this action)
    ATTACK,
    GIVE,
    KISS,
    SMELL,
    LISTEN,
    TOUCH,
    MUNG,       // generic "destroy"
    BOARD,      // board a vehicle
    DISEMBARK,
    COUNT,      // count contained objects
    INCANT,     // speak a magic word to an object
    WALK,       // go in a direction (room exit handler)
    // Room actions
    ROOM_ENTRY, // called when player enters room
    ROOM_EACH,  // called each turn player is in room
    LOOK,       // called when describing room (for special rooms)
    // Internal
    PICK_UP,    // NPC trying to pick up
    M_FATAL,    // fatal result for NPC in combat
    M_HIT,      // hit result in melee
    M_MISS,     // miss result in melee
};

// ---------------------------------------------------------------------------
// Parsed command structure (output of parser)
// Mirrors MDL PRSVEC: PRSA (verb), PRSO (direct obj), PRSI (indirect obj)
// ---------------------------------------------------------------------------
struct ParsedCommand {
    ActionType  verb;
    ObjId       prso;       // direct object   (OBJ_NONE if none)
    ObjId       prsi;       // indirect object (OBJ_NONE if none)
    Direction   dir;        // direction, if verb is GO  (DIR_NONE otherwise)
    bool        valid;      // false if parse failed
};

// ---------------------------------------------------------------------------
// Weapon / combat descriptor  (attached to weapon objects)
// ---------------------------------------------------------------------------
struct WeaponDesc {
    const char* name;       // e.g. "elvish sword"
    int8_t      damage;     // base damage roll 1..N
    int8_t      hitChance;  // hit probability modifier
    bool        isMelee;    // true = melee, false = thrown
};

// ---------------------------------------------------------------------------
// Scoring categories (from MDL SCORE function)
// ---------------------------------------------------------------------------
enum class ScoreReason : uint8_t {
    TREASURE_DEPOSITED,
    PUZZLE_SOLVED,
    VILLAIN_KILLED,
    FOUND_ITEM,
};
