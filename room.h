#pragma once
// ============================================================================
// room.h — Room (location) definition
//
// In MDL, rooms are defined with the ROOM macro in defs.mud and populated in
// rooms.mud and dung.mud.  A room has:
//   RDESC1  → desc          (short name, e.g. "West of House")
//   RDESC2  → ldesc         (long description; null → always use desc)
//   REXITS  → exits[]       (array of Exit, terminated by DIR_NONE entry)
//   RACTION → action        (per-room action handler; null = none)
//   RFLAGS  → flags         (RoomFlags bitfield)
//   RGLOBAL → globals[]     (objects always visible regardless of location;
//                            terminated by OBJ_NONE.  e.g. sky, ground, water)
//   RSTUFF  → things[]      (scenery pseudo-objects described in ldesc;
//                            terminated by OBJ_NONE)
//
// Exit encoding:
//   dest != ROOM_NONE  →  unconditional or conditional move
//   dest == ROOM_NONE  →  movement in this dir prints msg and stays put
//   cond != nullptr    →  only valid if cond() returns true; else msg is shown
//
// Light rules (mirroring MDL LIT?):
//   A room is lit if:  (a) RLIGHTBIT set, OR
//                      (b) a carried/worn/room object has LIGHTBIT+ONBIT
// ============================================================================

#include "types.h"

// Forward declaration for room action handler type
// 'self' is the RoomId; 'act' specifies why the handler was called
using RoomActionFn = bool (*)(RoomId self, ActionType act);

// ---------------------------------------------------------------------------
// A single exit from a room
// ---------------------------------------------------------------------------
struct Exit {
    Direction   dir;        // compass direction (DIR_NONE = end of list)
    RoomId      dest;       // destination room; ROOM_NONE = impassable
    const char* msg;        // message to print when blocked/conditional fails
                            // (may be nullptr if dest is valid and unconditional)
    bool      (*cond)();    // optional condition; if non-null, called each time
                            // the player tries to move this direction.
                            // Returns true = allow move, false = print msg+block
};

// Sentinel to terminate an exit list
#define EXIT_LIST_END  { DIR_NONE, ROOM_NONE, nullptr, nullptr }

// ---------------------------------------------------------------------------
// Room descriptor
// ---------------------------------------------------------------------------
struct Room {
    // --- Text ---
    const char*   desc;     // short location name (e.g. "Forest")
    const char*   ldesc;    // long description; nullptr → use desc only
    RoomActionFn  action;   // per-room handler; nullptr = default behavior

    // --- Exits ---
    // Pointer to a static array terminated by EXIT_LIST_END.
    // Using a pointer (rather than embedding) keeps Room size small and lets
    // multiple rooms share exit tables if needed (e.g. featureless mazes).
    const Exit*   exits;

    // --- Object lists ---
    // Globally visible objects (e.g. sky, river) — terminated by OBJ_NONE
    const ObjId*  globals;
    // Scenery pseudo-objects mentioned in ldesc — terminated by OBJ_NONE
    const ObjId*  things;

    // --- State ---
    uint16_t      flags;    // RoomFlags bitfield

    // --- Helpers ---
    bool hasFlag(RoomFlags f) const { return (flags & f) != 0; }
    void setFlag(RoomFlags f)       { flags |= static_cast<uint16_t>(f); }
    void clrFlag(RoomFlags f)       { flags &= static_cast<uint16_t>(~f); }

    bool isLit()     const { return hasFlag(RLIGHTBIT); }
    bool isLand()    const { return hasFlag(RLANDBIT); }
    bool isWater()   const { return hasFlag(RWATERBIT); }
    bool isSeen()    const { return hasFlag(RSEENBIT); }
    void markSeen()        { setFlag(RSEENBIT); }

    // Find the exit for a given direction; returns nullptr if none
    const Exit* findExit(Direction d) const {
        if (!exits) return nullptr;
        for (const Exit* e = exits; e->dir != DIR_NONE; ++e)
            if (e->dir == d) return e;
        return nullptr;
    }
};
