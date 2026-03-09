#pragma once
// ============================================================================
// actions.h — Verb dispatch system
//
// Mirrors the MDL action dispatch:  verb → handler function.
// Each handler receives the ParsedCommand and performs the action.
// Returns true if the action consumed a move (i.e., triggers room-each events).
//
// File organisation maps to MDL source files:
//   act1.mud → general actions (take, drop, look, go, examine, open, …)
//   act2.mud → extended actions (put, light, extinguish, read, lock, …)
//   act3.mud → combat actions (attack, throw) + special verbs
// ============================================================================

#include "types.h"
#include "parser.h"

namespace actions {

// Main dispatch entry point — call this with the parsed command each turn
// Returns true if the action consumed a move
bool dispatch(const ParsedCommand& cmd);

// Declared individually so melee.cpp / other modules can call them directly
bool doGo       (Direction d);
bool doLook     (bool forceLong = false);
bool doTake     (ObjId o);
bool doTakeAll  ();
bool doDrop     (ObjId o);
bool doDropAll  ();
bool doExamine  (ObjId o);
bool doOpen     (ObjId o);
bool doClose    (ObjId o);
bool doLock     (ObjId o, ObjId key);
bool doUnlock   (ObjId o, ObjId key);
bool doRead     (ObjId o);
bool doPut      (ObjId o, ObjId container);
bool doLight    (ObjId o);
bool doExtinguish(ObjId o);
bool doBurn     (ObjId o, ObjId with);
bool doThrow    (ObjId o, Direction d);
bool doThrowAt  (ObjId o, ObjId target);
bool doAttack   (ObjId target, ObjId weapon);
bool doEat      (ObjId o);
bool doDrink    (ObjId o);
bool doWear     (ObjId o);
bool doRemove   (ObjId o);
bool doInventory();
bool doScore    ();
bool doQuit     ();
bool doVerbose  ();
bool doBrief    ();
bool doSave     ();
bool doRestore  ();
bool doClimb    (ObjId o);
bool doEnter    (ObjId o);
bool doTie      (ObjId rope, ObjId anchor);
bool doInflate  (ObjId o, ObjId pump);
bool doDeflate  (ObjId o);
bool doMove     (ObjId o);
bool doPull     (ObjId o);
bool doPush     (ObjId o);
bool doTurn     (ObjId o);
bool doBoard    (ObjId vehicle);
bool doDisembark();
bool doSmell    (ObjId o);
bool doListen   (ObjId o);
bool doTouch    (ObjId o);
bool doGive     (ObjId o, ObjId recipient);

// Special meta-verbs handled inline in game loop
bool doXyzzy    ();  // Easter egg — nod to Colossal Cave
bool doBriefVerbose();

} // namespace actions
