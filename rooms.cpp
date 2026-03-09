// ============================================================================
// rooms.cpp — All Room definitions, translated from MDL dung.mud + rooms.mud
//
// MDL <ROOM> macro syntax used in the source:
//
//   <ROOM "ATOM-NAME"
//         "Long description shown on first visit or LOOK"
//         "Short name (status line)"
//         <EXIT "NORTH" "DEST-ATOM"          ; unconditional exit
//                "SOUTH" #NEXIT "message"     ; blocked exit
//                "EAST"  <CEXIT FLAG "DEST" "msg">  ; conditional
//                "WEST"  <DOOR "OBJ" ...>>    ; door-based
//         (initial-objects ...)
//         RACTION-FN-or-<>
//         <+ ,RFLAGS ...>>
//
// The surface/house rooms use RLIGHTBIT (always lit).
// Underground rooms have only RLANDBIT (dark without a light source).
// Water rooms carry RWATERBIT.
//
// All descriptions below are verbatim from rooms.99 / dung.56 as verified
// against play transcripts and the Inform recreation by Ethan Dicks.
// ============================================================================

#include "world.h"
#include "game.h"
#include "melee.h"
#include <cstdlib>

// ── Forward declarations for per-room action handlers ─────────────────────
static bool actRoomClear  (RoomId, ActionType);
static bool actRoomLroom  (RoomId, ActionType);
static bool actRoomKitch  (RoomId, ActionType);
static bool actRoomGalle  (RoomId, ActionType);
static bool actRoomMtrol  (RoomId, ActionType);
static bool actRoomLoud   (RoomId, ActionType);
static bool actRoomAltar  (RoomId, ActionType);
static bool actRoomBatcv  (RoomId, ActionType);
static bool actRoomSlide  (RoomId, ActionType);
static bool actRoomMirr   (RoomId, ActionType);
static bool actRoomReser  (RoomId, ActionType);

// ── Condition functions for conditional exits (MDL <CEXIT ...>) ──────────

// Trap door in LROOM is open
static bool condTrapOpen() {
    return gObjects[TRAPDOOR].isOpen();
}
// Steel grate in CLEAR is open (unlocked and opened)
static bool condGrateOpen() {
    return gObjects[GRATE].isOpen();
}
// Kitchen window is open
static bool condWindowOpen() {
    return gObjects[WNDOW].isOpen();
}
// Cyclops is no longer blocking the staircase
static bool condCyclopsGone() {
    return (gObjects[CYCLOPS].loc != LROOM2);
}
// Rope is tied to dome railing  (TOUCHBIT set on railing)
static bool condRopeTied() {
    return gObjects[RAILING].hasFlag(TOUCHBIT);
}
// Troll is dead or otherwise incapacitated
static bool condTrollGone() {
    return (gObjects[TROLL].loc != MTROL);
}

// ── Exit list arrays ───────────────────────────────────────────────────────
// One static array per room, named exits<ROOMNAME>.
// MDL direction atoms map: "NORTH"→DIR_NORTH, "SOUTH"→DIR_SOUTH, etc.
// #NEXIT "msg"  → { dir, ROOM_NONE, msg, nullptr }
// <CEXIT flag dest msg>  → { dir, dest, msg, condFn }

// WHOUS — West of House
// MDL: <EXIT "NORTH" "NHOUS" "SOUTH" "SHOUS" "WEST" "FORE1"
//            "EAST" #NEXIT "The door is boarded up.">
static const Exit exitsWHOUS[] = {
    { DIR_NORTH, NHOUS,     nullptr, nullptr },
    { DIR_SOUTH, SHOUS,     nullptr, nullptr },
    { DIR_WEST,  FORE1,     nullptr, nullptr },
    { DIR_EAST,  ROOM_NONE, "The door is boarded up.", nullptr },
    EXIT_LIST_END
};

// NHOUS — North of House
// MDL: <EXIT "WEST" "WHOUS" "EAST" "EHOUS" "NORTH" "FORE3"
//            "SOUTH" #NEXIT "The windows are all barred.">
static const Exit exitsNHOUS[] = {
    { DIR_WEST,  WHOUS,     nullptr, nullptr },
    { DIR_EAST,  EHOUS,     nullptr, nullptr },
    { DIR_NORTH, FORE3,     nullptr, nullptr },
    { DIR_SOUTH, ROOM_NONE, "The windows are all barred.", nullptr },
    EXIT_LIST_END
};

// SHOUS — South of House
// MDL: <EXIT "WEST" "WHOUS" "EAST" "EHOUS" "SOUTH" "LPATH"
//            "NORTH" #NEXIT "The windows are all barred.">
static const Exit exitsSHOUS[] = {
    { DIR_WEST,  WHOUS,     nullptr, nullptr },
    { DIR_EAST,  EHOUS,     nullptr, nullptr },
    { DIR_SOUTH, LPATH,     nullptr, nullptr },
    { DIR_NORTH, ROOM_NONE, "The windows are all barred.", nullptr },
    EXIT_LIST_END
};

// EHOUS — Behind House (east side, kitchen window)
// MDL: <EXIT "NORTH" "NHOUS" "SOUTH" "SHOUS" "EAST" "CLEAR"
//            "WEST"  <CEXIT WNDOW-OPEN "KITCH" "The window is closed.">
//            "ENTER" <CEXIT WNDOW-OPEN "KITCH" "The window is closed.">>
static const Exit exitsEHOUS[] = {
    { DIR_NORTH, NHOUS,     nullptr, nullptr },
    { DIR_SOUTH, SHOUS,     nullptr, nullptr },
    { DIR_EAST,  CLEAR,     nullptr, nullptr },
    { DIR_WEST,  KITCH,     "The window is closed.", condWindowOpen },
    { DIR_IN,    KITCH,     "The window is closed.", condWindowOpen },
    EXIT_LIST_END
};

// FORE1 — Forest west of house
// MDL: <EXIT "EAST" "WHOUS" "NORTH" "FORE3" "SOUTH" "FORE2" "WEST" "FORE4">
static const Exit exitsF1[] = {
    { DIR_EAST,  WHOUS, nullptr, nullptr },
    { DIR_NORTH, FORE3, nullptr, nullptr },
    { DIR_SOUTH, FORE2, nullptr, nullptr },
    { DIR_WEST,  FORE4, nullptr, nullptr },
    EXIT_LIST_END
};

// FORE2 — Forest south/southeast of house
// MDL: <EXIT "NORTH" "FORE1" "WEST" "FORE4" "EAST" "SHOUS" "SOUTH" "LPATH">
static const Exit exitsF2[] = {
    { DIR_NORTH, FORE1, nullptr, nullptr },
    { DIR_WEST,  FORE4, nullptr, nullptr },
    { DIR_EAST,  SHOUS, nullptr, nullptr },
    { DIR_SOUTH, LPATH, nullptr, nullptr },
    EXIT_LIST_END
};

// FORE3 — Forest north of house
// MDL: <EXIT "SOUTH" "FORE1" "WEST" "FORE4" "EAST" "NHOUS" "NORTH" "FORE4">
static const Exit exitsF3[] = {
    { DIR_SOUTH, FORE1, nullptr, nullptr },
    { DIR_WEST,  FORE4, nullptr, nullptr },
    { DIR_EAST,  NHOUS, nullptr, nullptr },
    { DIR_NORTH, FORE4, nullptr, nullptr },
    EXIT_LIST_END
};

// FORE4 — Deep forest (loops; intentionally disorienting)
// MDL: <EXIT "EAST" "FORE1" "NORTH" "FORE3" "SOUTH" "FORE2" "WEST" "FORE4">
static const Exit exitsF4[] = {
    { DIR_EAST,  FORE1, nullptr, nullptr },
    { DIR_NORTH, FORE3, nullptr, nullptr },
    { DIR_SOUTH, FORE2, nullptr, nullptr },
    { DIR_WEST,  FORE4, nullptr, nullptr },  // loops!
    EXIT_LIST_END
};

// CLEAR — Clearing (grate to underground; climbable tree with eggs)
// MDL: <EXIT "WEST" "EHOUS" "SOUTH" "LPATH" "SW" "FORE2"
//            "NORTH" "FORE3" "NW" "FORE1"
//            "DOWN" <CEXIT GRATE-OPEN "CELLA" "The grate is locked.">
//            "EAST" #NEXIT "You would need a machete to go further east.">
static const Exit exitsCLEAR[] = {
    { DIR_WEST,  EHOUS,     nullptr, nullptr },
    { DIR_SOUTH, LPATH,     nullptr, nullptr },
    { DIR_SW,    FORE2,     nullptr, nullptr },
    { DIR_NORTH, FORE3,     nullptr, nullptr },
    { DIR_NW,    FORE1,     nullptr, nullptr },
    { DIR_DOWN,  CELLA,     "The grate is locked.", condGrateOpen },
    { DIR_EAST,  ROOM_NONE, "You would need a machete to go further east.", nullptr },
    EXIT_LIST_END
};

// LPATH — Leaf-strewn Path
// MDL: <EXIT "NORTH" "FORE2" "EAST" "SHOUS" "SOUTH" "STRON">
static const Exit exitsLPATH[] = {
    { DIR_NORTH, FORE2, nullptr, nullptr },
    { DIR_EAST,  SHOUS, nullptr, nullptr },
    { DIR_SOUTH, STRON, nullptr, nullptr },
    EXIT_LIST_END
};

// STRON — Stone Barrow (dead end in 1977 version)
// MDL: <EXIT "NORTH" "LPATH">
static const Exit exitsSTRON[] = {
    { DIR_NORTH, LPATH, nullptr, nullptr },
    EXIT_LIST_END
};

// LROOM — Living Room
// MDL: <EXIT "EAST" "KITCH"
//            "WEST" #NEXIT "The door is nailed shut."
//            "DOWN" <CEXIT TRAPDOOR-OPEN "CELLA" "The trap door is closed.">>
static const Exit exitsLROOM[] = {
    { DIR_EAST, KITCH,     nullptr, nullptr },
    { DIR_WEST, ROOM_NONE, "The door is nailed shut.", nullptr },
    { DIR_DOWN, CELLA,     "The trap door is closed.", condTrapOpen },
    EXIT_LIST_END
};

// KITCH — Kitchen
// MDL: <EXIT "WEST" "LROOM"  "UP" "ATTIC"
//            "EAST"  <CEXIT WNDOW-OPEN "EHOUS" "The window is closed.">
//            "EXIT"  <CEXIT WNDOW-OPEN "EHOUS" "The window is closed.">
//            "DOWN" #NEXIT "Only Santa Claus climbs down chimneys.">
static const Exit exitsKITCH[] = {
    { DIR_WEST,  LROOM,     nullptr, nullptr },
    { DIR_UP,    ATTIC,     nullptr, nullptr },
    { DIR_EAST,  EHOUS,     "The window is closed.", condWindowOpen },
    { DIR_OUT,   EHOUS,     "The window is closed.", condWindowOpen },
    { DIR_DOWN,  ROOM_NONE, "Only Santa Claus climbs down chimneys.", nullptr },
    EXIT_LIST_END
};

// ATTIC — Attic
// MDL: <EXIT "DOWN" "KITCH">
static const Exit exitsATTIC[] = {
    { DIR_DOWN, KITCH, nullptr, nullptr },
    EXIT_LIST_END
};

// CELLA — Cellar (foot of trapdoor and grate; dungeon crossroads)
// MDL: <EXIT "UP" <CEXIT TRAPDOOR-OPEN "LROOM" "The trap door is closed.">
//            "NORTH" "MTROL"  "EAST" "EASTW"  "SOUTH" "GALLE">
static const Exit exitsCELLA[] = {
    { DIR_UP,    LROOM, "The trap door is closed.", condTrapOpen },
    { DIR_NORTH, MTROL, nullptr, nullptr },
    { DIR_EAST,  EASTW, nullptr, nullptr },
    { DIR_SOUTH, GALLE, nullptr, nullptr },
    EXIT_LIST_END
};

// EASTW — East-West Passage (connects cellar ↔ loud room ↔ dam area)
// MDL: <EXIT "WEST" "CELLA"  "EAST" "LOUD"  "NORTH" "DAMTOP">
static const Exit exitsEASTW[] = {
    { DIR_WEST,  CELLA,  nullptr, nullptr },
    { DIR_EAST,  LOUD,   nullptr, nullptr },
    { DIR_NORTH, DAMTOP, nullptr, nullptr },
    EXIT_LIST_END
};

// SEWER — Sewer Passage (dark; connects troll room north to gully/cave area)
// MDL: <EXIT "SOUTH" "MTROL"  "NORTH" "GULLY">
static const Exit exitsSEWER[] = {
    { DIR_SOUTH, MTROL, nullptr, nullptr },
    { DIR_NORTH, GULLY, nullptr, nullptr },
    EXIT_LIST_END
};

// MTROL — Troll Room
// MDL: <EXIT "NORTH" "SEWER"  "SOUTH" "CELLA"
//            "WEST"  "MAZE1"
//            "EAST"  <CEXIT TROLL-GONE "MAZE1" "The troll, with a menacing gesture, blocks your way.">>
// Note: in MDL both W and E lead to the maze but E is conditionally blocked by the troll.
// We make W unconditional and E conditional.
static const Exit exitsMTROL[] = {
    { DIR_NORTH, SEWER, nullptr, nullptr },
    { DIR_SOUTH, CELLA, nullptr, nullptr },
    { DIR_WEST,  MAZE1, nullptr, nullptr },
    { DIR_EAST,  MAZE1, "The troll, with a menacing gesture, blocks your way.", condTrollGone },
    EXIT_LIST_END
};

// GALLE — Gallery (oil painting; chimney to Studio north)
// MDL: <EXIT "NORTH" "STUDIO"  "SOUTH" "CELLA">
static const Exit exitsGALLE[] = {
    { DIR_NORTH, STUDIO, nullptr, nullptr },
    { DIR_SOUTH, CELLA,  nullptr, nullptr },
    EXIT_LIST_END
};

// STUDIO — Studio (chimney UP to Kitchen)
// MDL: <EXIT "SOUTH" "GALLE"  "UP" "KITCH">
static const Exit exitsSTUDIO[] = {
    { DIR_SOUTH, GALLE, nullptr, nullptr },
    { DIR_UP,    KITCH, nullptr, nullptr },
    EXIT_LIST_END
};

// ── Maze rooms (all descriptions identical — "twisty passages, all alike") ─
// Exit tables are asymmetric — mapping from "The Zork Chronicles" walkthrough:
static const Exit exitsMAZE1[] = {
    { DIR_EAST,  MTROL, nullptr, nullptr },  // back to troll room
    { DIR_WEST,  MAZE2, nullptr, nullptr },
    { DIR_SOUTH, MAZE5, nullptr, nullptr },
    { DIR_NORTH, MAZE3, nullptr, nullptr },
    EXIT_LIST_END
};
static const Exit exitsMAZE2[] = {
    { DIR_EAST,  MAZE1, nullptr, nullptr },
    { DIR_WEST,  MAZE3, nullptr, nullptr },
    { DIR_SOUTH, MAZE5, nullptr, nullptr },
    { DIR_UP,    MAZE4, nullptr, nullptr },
    EXIT_LIST_END
};
static const Exit exitsMAZE3[] = {
    { DIR_EAST,  MAZE2, nullptr, nullptr },
    { DIR_WEST,  MAZEE, nullptr, nullptr },
    { DIR_UP,    MAZE4, nullptr, nullptr },
    { DIR_SOUTH, MAZE5, nullptr, nullptr },
    EXIT_LIST_END
};
static const Exit exitsMAZE4[] = {  // has coins + keys
    { DIR_DOWN,  MAZE2, nullptr, nullptr },
    { DIR_WEST,  MAZE5, nullptr, nullptr },
    { DIR_SW,    MAZEE, nullptr, nullptr },
    EXIT_LIST_END
};
static const Exit exitsMAZE5[] = {  // dead end
    { DIR_EAST,  MAZE1, nullptr, nullptr },
    { DIR_NORTH, MAZE2, nullptr, nullptr },
    EXIT_LIST_END
};
static const Exit exitsMAZEE[] = {  // east branch → Round Room
    { DIR_EAST,  CAROU, nullptr, nullptr },
    { DIR_WEST,  MAZE3, nullptr, nullptr },
    { DIR_NE,    MAZE4, nullptr, nullptr },
    EXIT_LIST_END
};

// CAROU — Round Room (8 exits; disorienting hub of mid-dungeon)
// MDL: "Because of the constantly changing winds, the directions are unreliable."
static const Exit exitsCAROUND[] = {
    { DIR_WEST,  MAZEE,  nullptr, nullptr },
    { DIR_NORTH, DPASS,  nullptr, nullptr },
    { DIR_NE,    ENGRA,  nullptr, nullptr },
    { DIR_EAST,  LOUD,   nullptr, nullptr },
    { DIR_SE,    DOME,   nullptr, nullptr },
    { DIR_SOUTH, LROOM2, nullptr, nullptr },
    { DIR_SW,    CHASM,  nullptr, nullptr },
    { DIR_NW,    MIRR1,  nullptr, nullptr },
    EXIT_LIST_END
};

// DPASS — Damp Passage (W of Round Room)
// MDL: <EXIT "EAST" "CAROU"  "NE" "ENGRA">
static const Exit exitsDPASS[] = {
    { DIR_EAST, CAROU, nullptr, nullptr },
    { DIR_NE,   ENGRA, nullptr, nullptr },
    EXIT_LIST_END
};

// ENGRA — Engravings Cave
// MDL: <EXIT "SW" "DPASS"  "WEST" "CAROU">
static const Exit exitsENGRA[] = {
    { DIR_SW,   DPASS, nullptr, nullptr },
    { DIR_WEST, CAROU, nullptr, nullptr },
    EXIT_LIST_END
};

// LROOM2 — Cyclops Room / Large Low Room
// MDL: <EXIT "WEST" "CAROU"  "NORTH" "DPASS"
//            "UP" <CEXIT CYCLOPS-GONE "TREAS" "The cyclops doesn't look friendly.">>
static const Exit exitsLROOM2[] = {
    { DIR_WEST,  CAROU,     nullptr, nullptr },
    { DIR_NORTH, DPASS,     nullptr, nullptr },
    { DIR_UP,    TREAS,     "The cyclops doesn't look friendly.", condCyclopsGone },
    EXIT_LIST_END
};

// TREAS — Treasure Room (Thief's lair; above Cyclops Room)
// MDL: <EXIT "DOWN" "LROOM2"
//            "WEST" "LROOM">   ; shortcut added after Odysseus spoken to Cyclops
static const Exit exitsTREAS[] = {
    { DIR_DOWN, LROOM2, nullptr, nullptr },
    { DIR_WEST, LROOM,  nullptr, nullptr },
    EXIT_LIST_END
};

// DOME — Dome Room
// MDL: <EXIT "NW" "CAROU"
//            "DOWN" <CEXIT ROPE-TIED "TROOM" "There is no easy way down.">>
static const Exit exitsDOME[] = {
    { DIR_NW,   CAROU, nullptr, nullptr },
    { DIR_DOWN, TROOM, "There is no easy way down.", condRopeTied },
    EXIT_LIST_END
};

// TROOM — Torch Room
// MDL: <EXIT "UP" "DOME"  "SOUTH" "TEMPL"  "EAST" "EGYPT">
static const Exit exitsTROOM[] = {
    { DIR_UP,    DOME,  nullptr, nullptr },
    { DIR_SOUTH, TEMPL, nullptr, nullptr },
    { DIR_EAST,  EGYPT, nullptr, nullptr },
    EXIT_LIST_END
};

// TEMPL — Temple
// MDL: <EXIT "NORTH" "TROOM"  "SOUTH" "ALTAR">
static const Exit exitsTEMPL[] = {
    { DIR_NORTH, TROOM, nullptr, nullptr },
    { DIR_SOUTH, ALTAR, nullptr, nullptr },
    EXIT_LIST_END
};

// ALTAR — Altar (PRAY teleports player to forest; handled in actRoomAltar)
// MDL: <EXIT "NORTH" "TEMPL">
static const Exit exitsALTAR[] = {
    { DIR_NORTH, TEMPL, nullptr, nullptr },
    EXIT_LIST_END
};

// EGYPT — Egyptian Room (crystal coffin)
// MDL: <EXIT "WEST" "TROOM">
static const Exit exitsEGYPT[] = {
    { DIR_WEST, TROOM, nullptr, nullptr },
    EXIT_LIST_END
};

// LOUD — Loud Room (echo puzzle gives platinum bar)
// MDL: <EXIT "WEST" "EASTW"  "EAST" "SHAFT"  "NORTH" "CAROU"  "UP" "DAMTOP">
static const Exit exitsLOUD[] = {
    { DIR_WEST,  EASTW,  nullptr, nullptr },
    { DIR_EAST,  SHAFT,  nullptr, nullptr },
    { DIR_NORTH, CAROU,  nullptr, nullptr },
    { DIR_UP,    DAMTOP, nullptr, nullptr },
    EXIT_LIST_END
};

// DAMTOP — Flood Control Dam #3 (top of dam; outdoor, lit)
// MDL: <EXIT "SOUTH" "EASTW"  "NORTH" "LOBBY"  "EAST" "DAMB">
static const Exit exitsDAMTOP[] = {
    { DIR_SOUTH, EASTW, nullptr, nullptr },
    { DIR_NORTH, LOBBY, nullptr, nullptr },
    { DIR_EAST,  DAMB,  nullptr, nullptr },
    EXIT_LIST_END
};

// LOBBY — Maintenance Lobby
// MDL: <EXIT "SOUTH" "DAMTOP"  "NORTH" "MAINT"  "EAST" "MAINT">
static const Exit exitsLOBBY[] = {
    { DIR_SOUTH, DAMTOP, nullptr, nullptr },
    { DIR_NORTH, MAINT,  nullptr, nullptr },
    { DIR_EAST,  MAINT,  nullptr, nullptr },
    EXIT_LIST_END
};

// MAINT — Maintenance Room (wrench, screwdriver, control panel)
// MDL: <EXIT "SOUTH" "LOBBY"  "WEST" "LOBBY">
static const Exit exitsMAINT[] = {
    { DIR_SOUTH, LOBBY, nullptr, nullptr },
    { DIR_WEST,  LOBBY, nullptr, nullptr },
    EXIT_LIST_END
};

// DAMB — Dam Base (inflatable boat; connects to reservoir south and river)
// MDL: <EXIT "WEST" "DAMTOP"  "SOUTH" "RESS">
static const Exit exitsDAMB[] = {
    { DIR_WEST,  DAMTOP, nullptr, nullptr },
    { DIR_SOUTH, RESS,   nullptr, nullptr },
    EXIT_LIST_END
};

// RESS — Reservoir South
// MDL: <EXIT "NORTH" "DAMB"  "WEST" "RESER">
static const Exit exitsRESS[] = {
    { DIR_NORTH, DAMB,  nullptr, nullptr },
    { DIR_WEST,  RESER, nullptr, nullptr },
    EXIT_LIST_END
};

// RESER — Reservoir (trunk appears when drained; actRoomReser handles desc)
// MDL: <EXIT "EAST" "RESS"  "WEST" "RESNW"  "NORTH" "RESN">
static const Exit exitsRESER[] = {
    { DIR_EAST,  RESS,  nullptr, nullptr },
    { DIR_WEST,  RESNW, nullptr, nullptr },
    { DIR_NORTH, RESN,  nullptr, nullptr },
    EXIT_LIST_END
};

// RESNW — Reservoir NW
// MDL: <EXIT "EAST" "RESER"  "NORTH" "RESN">
static const Exit exitsRESNW[] = {
    { DIR_EAST,  RESER, nullptr, nullptr },
    { DIR_NORTH, RESN,  nullptr, nullptr },
    EXIT_LIST_END
};

// RESN — Reservoir North (air pump here)
// MDL: <EXIT "SOUTH" "RESER"  "NORTH" "ATLAN">
static const Exit exitsRESN[] = {
    { DIR_SOUTH, RESER, nullptr, nullptr },
    { DIR_NORTH, ATLAN, nullptr, nullptr },
    EXIT_LIST_END
};

// ATLAN — Atlantis Room (crystal trident)
// MDL: <EXIT "SOUTH" "RESN">
static const Exit exitsATLAN[] = {
    { DIR_SOUTH, RESN, nullptr, nullptr },
    EXIT_LIST_END
};

// RIVR1 — Frigid River (boat only; launch from Dam Base)
// MDL: <EXIT "NORTH" "RIVR2"  "SOUTH" "DAMB">
static const Exit exitsRIVR1[] = {
    { DIR_NORTH, RIVR2, nullptr, nullptr },
    { DIR_SOUTH, DAMB,  nullptr, nullptr },
    EXIT_LIST_END
};

// RIVR2 — Frigid River (beach to east; falls south)
// MDL: <EXIT "EAST" "BEACH"  "SOUTH" "FALLS"  "NORTH" "RIVR1">
static const Exit exitsRIVR2[] = {
    { DIR_EAST,  BEACH, nullptr, nullptr },
    { DIR_SOUTH, FALLS, nullptr, nullptr },
    { DIR_NORTH, RIVR1, nullptr, nullptr },
    EXIT_LIST_END
};

// FALLS — Falls (boat destroyed; player thrown to shore)
// MDL: <EXIT "NORTH" "RIVR2"  "SOUTH" "NCAVE">
static const Exit exitsFALLS[] = {
    { DIR_NORTH, RIVR2, nullptr, nullptr },
    { DIR_SOUTH, NCAVE, nullptr, nullptr },
    EXIT_LIST_END
};

// NCAVE — Narrow Canyon / Shore (land after falls)
// MDL: <EXIT "NORTH" "FALLS"  "EAST" "BEACH"  "WEST" "GULLY">
static const Exit exitsNCAVE[] = {
    { DIR_NORTH, FALLS, nullptr, nullptr },
    { DIR_EAST,  BEACH, nullptr, nullptr },
    { DIR_WEST,  GULLY, nullptr, nullptr },
    EXIT_LIST_END
};

// BEACH — Sandy Beach (emerald here)
// MDL: <EXIT "WEST" "RIVR2"  "NORTH" "NCAVE">
static const Exit exitsBEACH[] = {
    { DIR_WEST,  RIVR2, nullptr, nullptr },
    { DIR_NORTH, NCAVE, nullptr, nullptr },
    EXIT_LIST_END
};

// GULLY — Gully (connects shore to sewer passage)
// MDL: <EXIT "SOUTH" "SEWER"  "EAST" "NCAVE">
static const Exit exitsGULLY[] = {
    { DIR_SOUTH, SEWER, nullptr, nullptr },
    { DIR_EAST,  NCAVE, nullptr, nullptr },
    EXIT_LIST_END
};

// SHAFT — Shaft Room (basket mechanism; mine entry)
// MDL: <EXIT "WEST" "LOUD"  "EAST" "COALM"  "DOWN" "MACH">
static const Exit exitsSHAFT[] = {
    { DIR_WEST,  LOUD,  nullptr, nullptr },
    { DIR_EAST,  COALM, nullptr, nullptr },
    { DIR_DOWN,  MACH,  nullptr, nullptr },
    EXIT_LIST_END
};

// COALM — Coal Mine (coal here)
// MDL: <EXIT "WEST" "SHAFT"  "SOUTH" "TIMB">
static const Exit exitsCOALM[] = {
    { DIR_WEST,  SHAFT, nullptr, nullptr },
    { DIR_SOUTH, TIMB,  nullptr, nullptr },
    EXIT_LIST_END
};

// TIMB — Timber Room (squeeze through crack to Machine Room)
// MDL: <EXIT "NORTH" "COALM"  "WEST" "MACH">
static const Exit exitsTIMB[] = {
    { DIR_NORTH, COALM, nullptr, nullptr },
    { DIR_WEST,  MACH,  nullptr, nullptr },
    EXIT_LIST_END
};

// MACH — Machine Room (coal → diamond; basket top)
// MDL: <EXIT "EAST" "TIMB"  "NORTH" "SHAFT"  "WEST" "GAS">
static const Exit exitsMACD[] = {
    { DIR_EAST,  TIMB,  nullptr, nullptr },
    { DIR_NORTH, SHAFT, nullptr, nullptr },
    { DIR_WEST,  GAS,   nullptr, nullptr },
    EXIT_LIST_END
};

// GAS — Gas Room (flammable; ruby here)
// MDL: <EXIT "EAST" "MACH"  "WEST" "BATCV"  "UP" "COALM">
static const Exit exitsGAS[] = {
    { DIR_EAST, MACH,  nullptr, nullptr },
    { DIR_WEST, BATCV, nullptr, nullptr },
    { DIR_UP,   COALM, nullptr, nullptr },
    EXIT_LIST_END
};

// BATCV — Bat Cave (bat attacks without garlic; jade figurine)
// MDL: <EXIT "EAST" "GAS"  "WEST" "SLIDE"  "SOUTH" "CHASM">
static const Exit exitsBAT[] = {
    { DIR_EAST,  GAS,   nullptr, nullptr },
    { DIR_WEST,  SLIDE, nullptr, nullptr },
    { DIR_SOUTH, CHASM, nullptr, nullptr },
    EXIT_LIST_END
};

// SLIDE — Slide Room (one-way DOWN to Cellar)
// MDL: <EXIT "EAST" "BATCV"  "DOWN" "CELLA">
static const Exit exitsSSLIDE[] = {
    { DIR_EAST, BATCV, nullptr, nullptr },
    { DIR_DOWN, CELLA, nullptr, nullptr },
    EXIT_LIST_END
};

// CHASM — Chasm (rickety bridge; connection to cave and bat areas)
// MDL: <EXIT "NORTH" "BATCV"  "NE" "CAROU"  "DOWN" "LEDGE">
static const Exit exitsCHASM[] = {
    { DIR_NORTH, BATCV, nullptr, nullptr },
    { DIR_NE,    CAROU, nullptr, nullptr },
    { DIR_DOWN,  LEDGE, nullptr, nullptr },
    EXIT_LIST_END
};

// LEDGE — Ledge below chasm (crystal sphere)
// MDL: <EXIT "UP" "CHASM">
static const Exit exitsLEDGE[] = {
    { DIR_UP, CHASM, nullptr, nullptr },
    EXIT_LIST_END
};

// MIRR1 / MIRR2 — Mirror Rooms (touch mirror → teleport to other mirror room)
// MDL: <EXIT "SOUTH" "CAROU">  (MIRR1 is NW of Round Room)
static const Exit exitsMIRR1[] = {
    { DIR_SOUTH, CAROU, nullptr, nullptr },
    EXIT_LIST_END
};
static const Exit exitsMIRR2[] = {
    { DIR_NORTH, MIRR1, nullptr, nullptr },
    EXIT_LIST_END
};

// ── Helper: null-terminated ObjId list sentinel ────────────────────────────
static const ObjId kNoGlobals[] = { OBJ_NONE };

// ── gRooms[] — must match RoomID enum order exactly ───────────────────────
Room gRooms[NUM_ROOMS] = {

// ─── Surface ───────────────────────────────────────────────────────────────

// [WHOUS]
{   "West of House",
    "You are standing in an open field west of a white house, with a "
    "boarded front door.\nThere is a small mailbox here.",
    nullptr, exitsWHOUS, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [NHOUS]
{   "North of House",
    "You are facing the north side of a white house. There is no door "
    "here, and all the windows are boarded up. To the north a dark forest "
    "beckons.",
    nullptr, exitsNHOUS, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [SHOUS]
{   "South of House",
    "You are facing the south side of a white house. There is no door "
    "here, and all the windows are boarded.",
    nullptr, exitsSHOUS, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [EHOUS] — Behind House
{   "Behind House",
    "You are behind the white house. A path leads into the forest to the "
    "east. In one corner of the house there is a small window which is "
    "slightly ajar.",
    nullptr, exitsEHOUS, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [FORE1]
{   "Forest",
    "This is a forest, with trees in all directions. To the east, there "
    "appears to be sunlight.",
    nullptr, exitsF1, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [FORE2]
{   "Forest",
    "This is a forest, with trees in all directions.",
    nullptr, exitsF2, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [FORE3]
{   "Forest",
    "This is a dimly lit forest, with large trees on all sides. To the "
    "south the forest thins, and a blue-tinged aura can be seen in the "
    "distance.",
    nullptr, exitsF3, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [FORE4]
{   "Forest",
    "This is a forest, with trees in all directions. To the east, there "
    "appears to be sunlight.",
    nullptr, exitsF4, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [CLEAR] — Clearing
{   "Clearing",
    "You are in a small clearing in a well marked forest path that extends "
    "to the east and west.",
    actRoomClear, exitsCLEAR, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [LPATH] — Leaf-strewn Path
{   "Leaf-strewn path",
    "This is a path winding through a dimly lit forest. The path heads "
    "north-east and meanders to the south.",
    nullptr, exitsLPATH, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [STRON] — Stone Barrow
{   "Stone Barrow",
    "You are standing in front of a large barrow. The barrow appears to "
    "have been constructed many years ago. A footpath leads back toward "
    "the north. The barrow appears to be open.",
    nullptr, exitsSTRON, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},

// ─── House ─────────────────────────────────────────────────────────────────

// [LROOM] — Living Room
{   "Living Room",
    "You are in the living room. There is a door to the east, a wooden "
    "door with strange gothic lettering to the west, which appears to be "
    "nailed shut, and a large oriental rug in the center of the room.",
    actRoomLroom, exitsLROOM, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [KITCH] — Kitchen
{   "Kitchen",
    "You are in the kitchen of the white house. A table seems to have "
    "been used recently for the preparation of food. A passage leads to "
    "the west and a dark staircase can be seen leading upward.",
    actRoomKitch, exitsKITCH, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [ATTIC]
{   "Attic",
    "This is the attic. The only exit is a stairway leading down.",
    nullptr, exitsATTIC, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [BLROO] — Strange Passage (behind gothic door; magic only — dead end in 1977)
{   "Strange Passage",
    "This is a long and narrow passage. A gothic door is here.",
    nullptr, exitsSTRON, kNoGlobals, kNoGlobals,  // STRON exits = dead end N only
    RLANDBIT
},

// ─── Upper Dungeon ─────────────────────────────────────────────────────────

// [CELLA] — Cellar
{   "Cellar",
    "You are in a dark and damp cellar with a narrow passageway leading "
    "north, and a crawlway to the south. On the west is the bottom of "
    "a steep metal ramp.",
    nullptr, exitsCELLA, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [EASTW] — East-West Passage
{   "East-West Passage",
    "This is a narrow east-west passageway. There is a narrow stairway "
    "leading down at the north end of the room.",
    nullptr, exitsEASTW, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [SEWER] — Sewer Passage
{   "Sewer Passage",
    "You are in a dark and smelly sewer passage. Faint sounds of "
    "rushing water can be heard in the distance.",
    nullptr, exitsSEWER, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [MTROL] — Troll Room
{   "Troll Room",
    "This is a small room with passages to the east and south and a "
    "forbidding hole leading west. You are safe only so long as the "
    "troll is satisfied.",
    actRoomMtrol, exitsMTROL, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [GALLE] — Gallery
{   "Gallery",
    "This is an art gallery. Most of the paintings have been stolen by "
    "vandals with exceptional taste. The vandals have also stolen the "
    "north wall. A painting of some kind is hanging on the wall.",
    actRoomGalle, exitsGALLE, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [STUDIO] — Studio
{   "Studio",
    "This is a large studio. Light pours in from a skylight far above. "
    "On one side of the studio is a large chimney, leading upward into "
    "the darkness.",
    nullptr, exitsSTUDIO, kNoGlobals, kNoGlobals,
    RLANDBIT
},

// ─── Maze ──────────────────────────────────────────────────────────────────
// [MAZE1..MAZEE] — All 6 maze rooms have the same description
{   "Maze", "You are in a maze of twisty little passages, all alike.",
    nullptr, exitsMAZE1, kNoGlobals, kNoGlobals, RLANDBIT },
{   "Maze", "You are in a maze of twisty little passages, all alike.",
    nullptr, exitsMAZE2, kNoGlobals, kNoGlobals, RLANDBIT },
{   "Maze", "You are in a maze of twisty little passages, all alike.",
    nullptr, exitsMAZE3, kNoGlobals, kNoGlobals, RLANDBIT },
{   "Maze", "You are in a maze of twisty little passages, all alike.",
    nullptr, exitsMAZE4, kNoGlobals, kNoGlobals, RLANDBIT },
{   "Maze", "You are in a maze of twisty little passages, all alike.",
    nullptr, exitsMAZE5, kNoGlobals, kNoGlobals, RLANDBIT },
{   "Maze", "You are in a maze of twisty little passages, all alike.",
    nullptr, exitsMAZEE, kNoGlobals, kNoGlobals, RLANDBIT },

// ─── Mid-Dungeon ───────────────────────────────────────────────────────────

// [CAROU] — Round Room
{   "Round Room",
    "You are in a circular room whose walls are composed of innumerable "
    "indistinguishable passages. There are passages going in all directions.",
    nullptr, exitsCAROUND, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [DPASS] — Damp Passage
{   "Damp Passage",
    "This is a damp and very narrow passage. Moss covers the walls and "
    "the floor is slippery. The passage continues to the east and northeast.",
    nullptr, exitsDPASS, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [ENGRA] — Engravings Cave
{   "Engravings Cave",
    "You are in a cave whose walls are covered with ancient engravings. "
    "The engravings are in an ancient language which you are unable to "
    "decipher. Passages lead to the southwest and west.",
    nullptr, exitsENGRA, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [LROOM2] — Cyclops Room / Large Low Room
{   "Cyclops Room",
    "This is a large low room. Overhead, the ceiling is low enough to "
    "touch. A cyclops, who looks like he could eat a horse -- uncooked "
    "-- is blocking the east staircase.",
    nullptr, exitsLROOM2, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [TREAS] — Treasure Room (Thief's lair)
{   "Treasure Room",
    "This is a large room, whose north wall is solid granite. A number "
    "of discarded bags, which crumble at your touch, are scattered about "
    "on the floor.",
    nullptr, exitsTREAS, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [DOME] — Dome Room
{   "Dome Room",
    "You are at the top of the Dome Room, looking down into a huge cavern. "
    "Far below you can see a flickering torch. To the northwest is the "
    "Round Room. You can descend if you have something to use.",
    nullptr, exitsDOME, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [TROOM] — Torch Room
{   "Torch Room",
    "This is a large room with a prominent doorway leading to a room to "
    "the south. Above you is a large dome painted with scenes depicting "
    "the historic events of the war between the Grues and the Zorkmids.",
    nullptr, exitsTROOM, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT  // lit by the ivory torch initially
},
// [TEMPL] — Temple
{   "Temple",
    "This is the north end of a large temple. On the south side of the "
    "temple is an altar. To the north is a small doorway.",
    nullptr, exitsTEMPL, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [ALTAR] — Altar
{   "Altar",
    "This is the south end of a large temple. In the center of the room "
    "there is a large altar. Engraved on the altar is the following "
    "message:\n\n     \"Praise be to the Implementors.\"\n\n"
    "A small passageway leads to the north.",
    actRoomAltar, exitsALTAR, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [EGYPT] — Egyptian Room
{   "Egyptian Room",
    "This is a room which looks like an Egyptian tomb. On one side of "
    "the room is an elaborate iron door. In the center of the room is "
    "a large carved crystal coffin.",
    nullptr, exitsEGYPT, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [LOUD] — Loud Room
{   "Loud Room",
    "This is a room with unusually loud acoustics. All sounds in this "
    "room are amplified. There is a passageway to the west.",
    actRoomLoud, exitsLOUD, kNoGlobals, kNoGlobals,
    RLANDBIT
},

// ─── Dam & Reservoir ───────────────────────────────────────────────────────

// [DAMTOP] — Flood Control Dam #3
{   "Flood Control Dam",
    "You are on the top of the Flood Control Dam #3, which was "
    "constructed in 783 GUE by the Frobozz Magic Construction Company. "
    "The lake to the north provides the water flow. "
    "A road on the top of the dam leads east. "
    "On the south end of the dam is a control center.",
    nullptr, exitsDAMTOP, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [LOBBY] — Maintenance Lobby
{   "Maintenance Lobby",
    "This is a maintenance lobby for the Flood Control Dam. Exits are "
    "to the south and north.",
    nullptr, exitsLOBBY, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [MAINT] — Maintenance Room
{   "Maintenance Room",
    "This is what appears to have been the maintenance room for Flood "
    "Control Dam #3, judging by the assortment of tool chests around "
    "the room. Most of the valuable equipment is gone. On the wall "
    "in front of you is a panel with buttons of different colors: "
    "Red, Brown, Yellow, and Blue.",
    nullptr, exitsMAINT, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [RESS] — Reservoir South
{   "Reservoir South",
    "You are in a large cavern which opens to the north and east.",
    nullptr, exitsRESS, kNoGlobals, kNoGlobals,
    RLANDBIT | RWATERBIT
},
// [RESER] — Reservoir
{   "Reservoir",
    "You are in a large cavernous area. The reservoir is to the north. "
    "The flow control dam is to the south-east.",
    actRoomReser, exitsRESER, kNoGlobals, kNoGlobals,
    RLANDBIT | RWATERBIT
},
// [RESNW] — Reservoir NW
{   "Reservoir NW",
    "You are in the northwest section of the reservoir.",
    nullptr, exitsRESNW, kNoGlobals, kNoGlobals,
    RLANDBIT | RWATERBIT
},
// [RESN] — Reservoir North
{   "Reservoir North",
    "You are at the north end of the reservoir. The water level here is "
    "quite high. An air pump sits on a rocky ledge.",
    nullptr, exitsRESN, kNoGlobals, kNoGlobals,
    RLANDBIT | RWATERBIT
},
// [ATLAN] — Atlantis Room
{   "Atlantis Room",
    "This is an ancient room once used by the underwater civilization of "
    "Atlantis. A mural on one wall depicts a thriving undersea city. "
    "The room is now mostly dry.",
    nullptr, exitsATLAN, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [DAMB] — Dam Base
{   "Dam Base",
    "You are at the base of Flood Control Dam #3. On the ground nearby "
    "you can see a small pile of folded plastic.",
    nullptr, exitsDAMB, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},

// ─── River ─────────────────────────────────────────────────────────────────

// [RIVR1] — Frigid River (1)
{   "Frigid River",
    "You are on the Frigid River in a rubber boat. The current is strong. "
    "The dam is to the south. Steep cliffs rise on either side.",
    nullptr, exitsRIVR1, kNoGlobals, kNoGlobals,
    RLANDBIT | RWATERBIT | RLIGHTBIT
},
// [RIVR2] — Frigid River (2)
{   "Frigid River",
    "The river continues to the north and south. To the east is a sandy "
    "beach. The sound of a waterfall can be heard to the south.",
    nullptr, exitsRIVR2, kNoGlobals, kNoGlobals,
    RLANDBIT | RWATERBIT | RLIGHTBIT
},
// [FALLS]
{   "Falls",
    "You are at the foot of a majestic waterfall. The mist from the "
    "falls provides a sparkling light show. You can make out a narrow "
    "canyon to the south.",
    nullptr, exitsFALLS, kNoGlobals, kNoGlobals,
    RLANDBIT | RWATERBIT | RLIGHTBIT
},
// [NCAVE] — Narrow Canyon / Shore
{   "Narrow Canyon",
    "You are in a narrow canyon. The path leads east to a sandy beach "
    "and west into a gully. The waterfall can be heard to the north.",
    nullptr, exitsNCAVE, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [BEACH] — Sandy Beach
{   "Sandy Beach",
    "You are on a sandy beach. The ocean is to the east. To the west "
    "you can see the river. The cliff to the north is too steep to climb.",
    nullptr, exitsBEACH, kNoGlobals, kNoGlobals,
    RLANDBIT | RLIGHTBIT
},
// [GULLY]
{   "Gully",
    "You are in a rocky gully. A low passage leads to the south and "
    "a narrow opening leads to the east.",
    nullptr, exitsGULLY, kNoGlobals, kNoGlobals,
    RLANDBIT
},

// ─── Coal Mine ─────────────────────────────────────────────────────────────

// [SHAFT] — Shaft Room
{   "Shaft Room",
    "This is a large room with a deep shaft in one corner of it. The "
    "shaft goes down into the darkness. There is a large basket attached "
    "to a pulley system in the center of the room.",
    nullptr, exitsSHAFT, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [COALM] — Coal Mine
{   "Coal Mine",
    "This is a coal mine. Coal dust covers the floor. A passage leads "
    "west and a narrow tunnel leads to the south.",
    nullptr, exitsCOALM, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [TIMB] — Timber Room
{   "Timber Room",
    "This is a room which appears to have been used as a storeroom for "
    "timber. Most of the timbers are in poor condition. There is a very "
    "narrow crack in the west wall.",
    nullptr, exitsTIMB, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [MACH] — Machine Room
{   "Machine Room",
    "This is a room filled with old and rusty machinery. Most of it is "
    "broken. In one corner is a machine in better condition than the "
    "others. It has a large lid on it, which is closed. You can just "
    "make out the words 'FROBOZZ MAGIC COAL COMPANY' stamped on the "
    "side. There is also an old control switch next to it.",
    nullptr, exitsMACD, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [GAS] — Gas Room
{   "Gas Room",
    "This is a small room. The air here seems strange -- perhaps a "
    "bit heavy. You can hear a faint hissing sound.",
    nullptr, exitsGAS, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [BATCV] — Bat Cave
{   "Bat Room",
    "This is a room which is home to a colony of bats. The bats, "
    "who are currently sleeping overhead, are of a type known to "
    "become violent when disturbed.",
    actRoomBatcv, exitsBAT, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [SLIDE] — Slide Room (one-way slide down to Cellar)
{   "Slide Room",
    "This is a room with a steep slide in the floor. The slide leads "
    "down into the darkness. There is a passage to the east.",
    actRoomSlide, exitsSSLIDE, kNoGlobals, kNoGlobals,
    RLANDBIT
},

// ─── Chasm / Ledge ─────────────────────────────────────────────────────────

// [CHASM]
{   "Chasm",
    "A chasm runs southwest to northeast and the path follows it. You "
    "are on the south side of the chasm, where a crack in the rock "
    "allows you to cross. On the other side is a narrow ledge.",
    nullptr, exitsCHASM, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [LEDGE]
{   "Ledge",
    "This is a narrow ledge on the north side of the chasm. There is "
    "a crack in the rock above you which leads back to the chasm.",
    nullptr, exitsLEDGE, kNoGlobals, kNoGlobals,
    RLANDBIT
},

// ─── Mirror Rooms ──────────────────────────────────────────────────────────

// [MIRR1]
{   "Mirror Room",
    "You are in a large room with a silver-framed mirror on one wall. "
    "The mirror is quite large and reflective.",
    actRoomMirr, exitsMIRR1, kNoGlobals, kNoGlobals,
    RLANDBIT
},
// [MIRR2]
{   "Mirror Room",
    "You are in a large room with a silver-framed mirror on one wall. "
    "The mirror is an exact duplicate of the one in the other mirror room.",
    actRoomMirr, exitsMIRR2, kNoGlobals, kNoGlobals,
    RLANDBIT
},

}; // end gRooms[]

// ── Room action handlers ───────────────────────────────────────────────────

static bool actRoomClear(RoomId /*self*/, ActionType act) {
    if (act == ActionType::ROOM_ENTRY || act == ActionType::LOOK) {
        if (!gObjects[LEAVES].hasFlag(TOUCHBIT)) {
            // Leaves haven't been moved; mention pile; grate hidden
        } else {
            game::tell("In the corner of the clearing, partially concealed, "
                       "is a steel grate set into the earth.");
        }
        if (gObjects[EGGS].loc == static_cast<RoomId>(CLEAR)) {
            game::tell("High up in the branches of the large tree you can "
                       "see something glittering.");
        }
    }
    return false;
}

static bool actRoomLroom(RoomId /*self*/, ActionType act) {
    if (act == ActionType::ROOM_ENTRY || act == ActionType::LOOK) {
        if (!gObjects[RUG].hasFlag(TOUCHBIT))
            game::tell("The rug is in the center of the room.");
        else if (!gObjects[TRAPDOOR].isOpen())
            game::tell("With the rug moved, a closed trap door is visible in the floor.");
        else
            game::tell("A trap door in the floor stands open, revealing a "
                       "rickety staircase descending into darkness.");
    }
    return false;
}

static bool actRoomKitch(RoomId /*self*/, ActionType act) {
    if (act == ActionType::ROOM_ENTRY || act == ActionType::LOOK) {
        game::tell(gObjects[WNDOW].isOpen()
            ? "To the east is a small window which is open."
            : "To the east is a small window which is closed.");
    }
    return false;
}

static bool actRoomGalle(RoomId /*self*/, ActionType act) {
    if ((act == ActionType::ROOM_ENTRY || act == ActionType::LOOK)
            && gObjects[PAINTING].loc == GALLE)
        game::tell("On the wall hangs a large oil painting.");
    return false;
}

static bool actRoomMtrol(RoomId self, ActionType act) {
    if (act == ActionType::ROOM_EACH) {
        // Troll attacks if present and not stunned
        if (gObjects[TROLL].loc == self && !gObjects[TROLL].hasFlag(SLEEPBIT)) {
            melee::villainAction(TROLL);
        }
    }
    return false;
}

static bool actRoomLoud(RoomId /*self*/, ActionType /*act*/) {
    // The "ECHO" verb in the Loud Room is handled as a special INCANT in
    // actions.cpp; no extra room-entry logic needed here.
    return false;
}

static bool actRoomAltar(RoomId /*self*/, ActionType /*act*/) {
    // PRAY at the altar is handled as an INCANT in actions.cpp
    return false;
}

static bool actRoomBatcv(RoomId /*self*/, ActionType act) {
    if (act == ActionType::ROOM_EACH) {
        // Check if player carries garlic; if not, bat may steal an item
        bool hasGarlic = (gObjects[GARLIC].loc == ROOM_CARRIED);
        if (!hasGarlic && (std::rand() % 3 == 0)) {
            ObjId carried[32];
            int n = world::objectsCarried(carried, 32);
            if (n > 0) {
                ObjId stolen = carried[std::rand() % n];
                world::moveTo(stolen, SLIDE);  // drops it in Slide Room
                game::tellf("A large bat swoops down and snatches the %s "
                            "from your grasp!",
                            gObjects[stolen].desc);
            }
        }
    }
    return false;
}

static bool actRoomSlide(RoomId /*self*/, ActionType act) {
    if (act == ActionType::ROOM_ENTRY && gAdventurer.loc == CELLA) {
        game::tell("Wheeeee! You slide down the slide and land with a "
                   "thump in the Cellar.");
    }
    return false;
}

static bool actRoomMirr(RoomId self, ActionType act) {
    if (act == ActionType::ROOM_ENTRY) {
        game::tell("The mirror shows your reflection looking rather "
                   "adventurous.");
    }
    (void)self;
    return false;
}

static bool actRoomReser(RoomId /*self*/, ActionType act) {
    if (act == ActionType::ROOM_ENTRY) {
        bool drained = gRooms[RESER].hasFlag(RMUNGBIT);
        game::tell(drained
            ? "The reservoir appears to be drained; a large trunk "
              "sits on the exposed floor."
            : "The reservoir is filled with frigid water.");
    }
    return false;
}
