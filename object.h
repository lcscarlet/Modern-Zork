#pragma once
// ============================================================================
// object.h — Game object definition
//
// In MDL, every game entity (items, NPCs, the player) is an OBJECT with a
// fixed set of properties defined by the OBJECT macro in defs.mud.  We
// translate that macro into a plain struct.  All objects live in a static
// array in world.cpp; their ObjId is their array index.
//
// MDL OBJECT properties mapped to C++ struct fields:
//   ODESC1   → desc        (short noun phrase, e.g. "brass lantern")
//   ODESC2   → ldesc       (long description shown on LOOK)
//   OACTIONS → action      (function pointer, called for verb dispatch)
//   OLOC     → loc         (where is this object right now; see note below)
//   OSIZE    → size        (bulk of object; used for container capacity)
//   OCAPACITY→ capacity    (how much this container holds)
//   OSTRENGTH→ strength    (for weapons: damage; for NPC: combat strength)
//   OSCENERY → scenery     (pointer to scenery objects visible in this room)
//   OTEXT    → text        (text to print when READ; nullptr if not readable)
//   OFLAGS   → flags       (ObjFlags bitfield; see types.h)
//   OVAL     → value       (score value when deposited in trophy case)
//   OVTYPE   → vtype       (vehicle type identifier; 0 if not a vehicle)
//
// Location encoding:
//   loc >= 0           → object is in that RoomId
//   loc == ROOM_CARRIED → object is being carried by the player
//   loc == ROOM_WORN    → object is being worn by the player
//   special: the player's loc is always a valid RoomId (their current room)
// ============================================================================

#include "types.h"

// Forward declaration — action handler signature
// Returns true if the handler consumed the action (prevents default handling)
// 'self' is this object's ObjId; 'act' is the action being applied
using ActionFn = bool (*)(ObjId self, ActionType act);

struct Object {
    // --- Identity ---
    const char*  desc;      // short name, e.g. "brass lantern" (never null)
    const char*  ldesc;     // long room description, null → use desc
    const char*  text;      // READ text; null → not readable
    ActionFn     action;    // verb handler; null → use default behavior

    // --- Containment ---
    // For normal objects:  loc is a RoomId (≥0), ROOM_CARRIED, or ROOM_WORN
    // For room "objects" (which are not really in the object array):  n/a
    RoomId       loc;

    // --- Physical properties ---
    uint16_t     size;      // bulk units consumed in a container
    uint16_t     capacity;  // how many bulk units this container holds (0 if not CONTBIT)
    int8_t       strength;  // NPC combat strength (0 for non-fighters)
    int8_t       value;     // score value when deposited (0 if not a treasure)

    // --- State flags (ObjFlags) ---
    uint32_t     flags;

    // --- Helpers ---
    bool hasFlag(ObjFlags f) const { return (flags & f) != 0; }
    void setFlag(ObjFlags f)       { flags |= f; }
    void clrFlag(ObjFlags f)       { flags &= ~f; }
    bool isOpen()    const { return hasFlag(OPENBIT); }
    bool isOn()      const { return hasFlag(ONBIT); }
    bool isTaken()   const { return loc == ROOM_CARRIED || loc == ROOM_WORN; }
    bool isCarried() const { return loc == ROOM_CARRIED; }
    bool isWorn()    const { return loc == ROOM_WORN; }
    bool isLight()   const { return hasFlag(LIGHTBIT) && hasFlag(ONBIT); }

    // Total bulk of contents (must call world::objectsIn to compute)
    // Declared here; defined in world.cpp
    uint16_t contentsBulk() const;

    // Can obj fit inside this container?
    bool canContain(const Object& obj) const {
        return hasFlag(CONTBIT) &&
               (hasFlag(TRANSBIT) || isOpen()) &&
               (contentsBulk() + obj.size <= capacity);
    }
};

// ---------------------------------------------------------------------------
// The Adventurer (player) is not a regular Object but a small POD struct
// that the MDL code calls ADV.  In MDL it is a CHTYPE'd vector.
// ---------------------------------------------------------------------------
struct Adventurer {
    RoomId   loc;       // current room (= HERE in MDL)
    int16_t  score;     // current score
    int16_t  moves;     // move counter
    int16_t  health;    // hit points (starts at 10)
    int16_t  maxHealth; // maximum HP
    int16_t  deaths;    // number of times killed
    bool     verbose;   // VERBOSE mode (always print long room desc)
    bool     brief;     // BRIEF mode (never print long desc after first visit)
    // Wielded weapon (-1 if bare hands)
    ObjId    weapon;
};
