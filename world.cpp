// ============================================================================
// world.cpp — Game objects, player state, and world utility functions
//
// Rooms are defined in rooms.cpp.  This file owns:
//   • gObjects[NUM_OBJECTS]  — every item, NPC, and scenery piece
//   • gAdventurer            — the player (MDL ADV vector)
//   • world::* functions     — world queries used by actions/game/melee
//
// Location encoding (see also object.h):
//   loc >= 0              → in room[loc]
//   ROOM_CARRIED (-2)     → player carries it
//   ROOM_WORN    (-3)     → player wears it
//   loc <= -10            → inside container: containerID = -(loc+10)
// ============================================================================

#include "world.h"
#include "game.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

// ── Container location helpers ─────────────────────────────────────────────
static inline RoomId locOfContainer(ObjId c)    { return static_cast<RoomId>(-(c + 10)); }
static inline ObjId  containerOfLoc(RoomId l)   { return static_cast<ObjId>(-(l) - 10); }
static inline bool   isInsideCont  (RoomId l)   { return l <= -10; }

// ── Player state ───────────────────────────────────────────────────────────
Adventurer gAdventurer = {
    /* loc       */ WHOUS,
    /* score     */ 0,
    /* moves     */ 0,
    /* health    */ 10,
    /* maxHealth */ 10,
    /* deaths    */ 0,
    /* verbose   */ false,
    /* brief     */ false,
    /* weapon    */ OBJ_NONE,
};

// ── Object action handler forward declarations ─────────────────────────────
static bool actMailbox    (ObjId, ActionType);
static bool actLeaflet    (ObjId, ActionType);
static bool actLantern    (ObjId, ActionType);
static bool actTorch      (ObjId, ActionType);
static bool actCandles    (ObjId, ActionType);
static bool actSword      (ObjId, ActionType);
static bool actLeaves     (ObjId, ActionType);
static bool actGrate      (ObjId, ActionType);
static bool actTrapDoor   (ObjId, ActionType);
static bool actRug        (ObjId, ActionType);
static bool actTrophyCase (ObjId, ActionType);
static bool actBottle     (ObjId, ActionType);
static bool actSack       (ObjId, ActionType);
static bool actRope       (ObjId, ActionType);
static bool actRailing    (ObjId, ActionType);
static bool actCoffin     (ObjId, ActionType);
static bool actPainting   (ObjId, ActionType);
static bool actWindow     (ObjId, ActionType);
static bool actBoat       (ObjId, ActionType);
static bool actBoatI      (ObjId, ActionType);
static bool actBasket     (ObjId, ActionType);
static bool actLunch      (ObjId, ActionType);
static bool actBell       (ObjId, ActionType);
static bool actBook       (ObjId, ActionType);
static bool actCyclops    (ObjId, ActionType);
static bool actTrolll     (ObjId, ActionType);
static bool actThief      (ObjId, ActionType);
static bool actMachine    (ObjId, ActionType);
static bool actBar        (ObjId, ActionType);
static bool actEggs       (ObjId, ActionType);

// ── Shorthand macros ───────────────────────────────────────────────────────
// INS(container) — place an object inside a container at definition time
#define INS(c) static_cast<RoomId>(locOfContainer(static_cast<ObjId>(c)))

// ── gObjects[NUM_OBJECTS] — order MUST match ObjID enum in world.h ─────────
Object gObjects[NUM_OBJECTS] = {

// ─── Light sources ─────────────────────────────────────────────────────────

// [LAMP] Brass lantern — battery powered; starts in Living Room
// MDL: <OBJECT LAMP (LOC LROOM)(DESC "brass lantern")(LIGHTBIT)(TAKEBIT)>
{ "brass lantern",
  "There is a brass lantern (battery-powered) here.",
  nullptr, actLantern,
  LROOM, 15, 0, 0, 0,
  VISBIT|TAKEBIT|LIGHTBIT
},
// [TORCH] Ivory torch — always-lit; a treasure AND a light source; in Torch Room
{ "ivory torch",
  "A shiny ivory torch is here, burning with a clear flame.",
  nullptr, actTorch,
  TROOM, 5, 0, 0, 20,
  VISBIT|TAKEBIT|LIGHTBIT|ONBIT
},
// [CANDL] Two candles — in Temple; need matches to light
{ "pair of candles",
  "A pair of candles are here.",
  nullptr, actCandles,
  TEMPL, 3, 0, 0, 0,
  VISBIT|TAKEBIT|LIGHTBIT|BURNBIT
},
// [MATCH] Book of matches — in Maintenance Lobby; a flame source
{ "book of matches",
  "There is a book of matches here.",
  nullptr, nullptr,
  LOBBY, 1, 0, 0, 0,
  VISBIT|TAKEBIT|TOOLBIT|FLAMEBIT
},

// ─── Weapons ───────────────────────────────────────────────────────────────

// [SWORD] Elvish sword — in Living Room; glows blue near danger
{ "elvish sword",
  "There is a beautiful elvish sword of great antiquity here.",
  nullptr, actSword,
  LROOM, 20, 0, 3, 0,
  VISBIT|TAKEBIT|WEAPONBIT
},
// [KNIFE] Nasty knife — in Attic
{ "nasty knife",
  "There is a nasty-looking knife here.",
  nullptr, nullptr,
  ATTIC, 5, 0, 2, 0,
  VISBIT|TAKEBIT|WEAPONBIT
},

// ─── Tools / key items ─────────────────────────────────────────────────────

// [ROPE] Rope — in Attic; tie to DOME railing to descend
{ "rope",
  "A coil of rope is lying here.",
  nullptr, actRope,
  ATTIC, 10, 0, 0, 0,
  VISBIT|TAKEBIT|TIEBIT
},
// [KEYS] Set of keys — in Maze4; unlocks grate in Clearing
{ "set of keys",
  "There is a set of keys here.",
  nullptr, nullptr,
  MAZE4, 3, 0, 0, 0,
  VISBIT|TAKEBIT|TOOLBIT
},
// [LANTER] Skeleton key — in Maze4 (yes, both keys are in the same room)
{ "skeleton key",
  "There is a skeleton key here.",
  nullptr, nullptr,
  MAZE4, 1, 0, 0, 0,
  VISBIT|TAKEBIT|TOOLBIT
},
// [BOTTL] Glass bottle — in Kitchen; starts full of water
{ "glass bottle",
  "There is a glass bottle here.",
  nullptr, actBottle,
  KITCH, 5, 5, 0, 0,
  VISBIT|TAKEBIT|CONTBIT|OPENBIT|TRANSBIT
},
// [WATER] Quantity of water — inside the bottle
{ "quantity of water",
  nullptr,
  nullptr, nullptr,
  INS(BOTTL), 1, 0, 0, 0,
  VISBIT|TAKEBIT|DRINKBIT
},
// [SACK] Brown sack — in Kitchen; container with garlic + lunch inside
{ "brown sack",
  "A brown sack, smelling of hot peppers, is here.",
  nullptr, actSack,
  KITCH, 5, 20, 0, 0,
  VISBIT|TAKEBIT|CONTBIT|OPENBIT
},
// [GARLIC] Clove of garlic — inside sack; repels bat
{ "clove of garlic",
  nullptr,
  nullptr, nullptr,
  INS(SACK), 1, 0, 0, 0,
  VISBIT|TAKEBIT|FOODBIT
},
// [LUNCH] Lunch — inside sack; edible; can be fed to Cyclops
{ "lunch",
  nullptr,
  nullptr, actLunch,
  INS(SACK), 3, 0, 0, 0,
  VISBIT|TAKEBIT|FOODBIT
},
// [PUMP] Air pump — in Reservoir North; inflate the rubber boat
{ "air pump",
  "There is an air pump here.",
  nullptr, nullptr,
  RESN, 8, 0, 0, 0,
  VISBIT|TAKEBIT|TOOLBIT
},
// [WRENCH] Wrench — in Maintenance Room; turn dam sluice bolt
{ "wrench",
  "There is a large wrench here.",
  nullptr, nullptr,
  MAINT, 10, 0, 0, 0,
  VISBIT|TAKEBIT|TOOLBIT|WEAPONBIT
},
// [SCRDVR] Screwdriver — in Maintenance Room; operate machine switch
{ "screwdriver",
  "There is a screwdriver here.",
  nullptr, nullptr,
  MAINT, 5, 0, 0, 0,
  VISBIT|TAKEBIT|TOOLBIT
},
// [BELL] Brass bell — in Temple; ring at Hades entrance (with candles and book)
{ "brass bell",
  "There is a small brass bell here.",
  nullptr, actBell,
  TEMPL, 3, 0, 0, 0,
  VISBIT|TAKEBIT|TOOLBIT
},
// [BOOK] Black book — in Temple; read at Hades; contains "ODYSSEUS" hint
{ "black book",
  "A black, leather-bound book is here.",
  "The book is titled 'Rites for the Entrance into Hades.'\n\n"
  "It explains that to enter the Land of the Dead, one must:\n"
  "  1. Ring a bell three times\n"
  "  2. Light the candles\n"
  "  3. Read this book\n\n"
  "Furthermore, it notes that the Cyclops can be dispelled by uttering "
  "his true name: ODYSSEUS (also known as ULYSSES).",
  actBook,
  TEMPL, 5, 0, 0, 0,
  VISBIT|TAKEBIT|READBIT
},
// [PAPER] Piece of paper — in Living Room; pinned to front door
{ "piece of paper",
  nullptr,
  "The paper reads:\n\n"
  "'This house has been abandoned for many years. It is rumoured to "
  "rest upon the entrance to the Great Underground Empire, where "
  "adventurers have sought the fabled treasures of Zork.'",
  nullptr,
  LROOM, 1, 0, 0, 0,
  VISBIT|TAKEBIT|READBIT
},
// [LEAFL] Leaflet — inside mailbox
{ "leaflet",
  nullptr,
  "WELCOME TO ZORK!\n\n"
  "ZORK is a game of adventure, danger, and low cunning. In it you "
  "will explore some of the most amazing territory ever seen by mortals. "
  "No computer should be without one!\n\n"
  "In your quest to recover the legendary treasures of Zork, you will "
  "need to find and use many objects.",
  actLeaflet,
  INS(MBOX), 1, 0, 0, 0,
  VISBIT|TAKEBIT|READBIT
},

// ─── Fixed / scenery ───────────────────────────────────────────────────────

// [MBOX] Small mailbox — in WHOUS; closed at start; contains leaflet
{ "small mailbox",
  "There is a small mailbox here.",
  nullptr, actMailbox,
  WHOUS, 0, 10, 0, 0,
  VISBIT|CONTBIT|DOORBIT|SACREDBIT|NDESCBIT
  // starts CLOSED — no OPENBIT
},
// [MAT] Welcome mat — in WHOUS
{ "welcome mat",
  nullptr, nullptr, nullptr,
  WHOUS, 0, 0, 0, 0,
  VISBIT|SACREDBIT|NDESCBIT
},
// [FDOOR] Front door — WHOUS; boarded shut
{ "front door",
  nullptr, nullptr, nullptr,
  WHOUS, 0, 0, 0, 0,
  VISBIT|DOORBIT|SACREDBIT|NDESCBIT
},
// [WNDOW] Kitchen window — spans EHOUS & KITCH; starts OPEN (OPENBIT)
{ "window",
  nullptr, nullptr, actWindow,
  EHOUS, 0, 0, 0, 0,
  VISBIT|DOORBIT|SACREDBIT|NDESCBIT|OPENBIT
},
// [WDOOR] Gothic wood door — LROOM west wall; nailed shut
{ "wooden door",
  "The wooden door has strange gothic lettering: 'This space intentionally left blank.'",
  nullptr, nullptr,
  LROOM, 0, 0, 0, 0,
  VISBIT|DOORBIT|SACREDBIT|NDESCBIT
},
// [LEAVES] Pile of leaves — in CLEAR; grate hidden underneath
{ "pile of leaves",
  "There is a pile of leaves on the ground here.",
  nullptr, actLeaves,
  CLEAR, 0, 0, 0, 0,
  VISBIT|SACREDBIT|NDESCBIT|TURNBIT
},
// [GRATE] Steel grate — in CLEAR; locked; KEYS open it; DOWN → CELLA
{ "steel grate",
  nullptr, nullptr, actGrate,
  CLEAR, 0, 0, 0, 0,
  DOORBIT|SACREDBIT|NDESCBIT|LOCKBIT
  // starts not VISBIT (hidden under leaves) and not OPENBIT (closed+locked)
},
// [TRAPDOOR] Trap door — in LROOM floor; closed at start
{ "trap door",
  nullptr, nullptr, actTrapDoor,
  LROOM, 0, 0, 0, 0,
  VISBIT|DOORBIT|SACREDBIT|NDESCBIT
  // no OPENBIT at start
},
// [RUG] Large oriental rug — in LROOM; MOVE → reveals trapdoor
{ "large oriental rug",
  "There is a large oriental rug in the center of the room.",
  nullptr, actRug,
  LROOM, 30, 0, 0, 0,
  VISBIT|SACREDBIT|TURNBIT
},
// [TCASE] Trophy case — in LROOM; open; deposit treasures here for points
{ "trophy case",
  "There is a trophy case here.",
  nullptr, actTrophyCase,
  LROOM, 0, 200, 0, 0,
  VISBIT|CONTBIT|SACREDBIT|OPENBIT
},
// [RAILING] Railing — in DOME; tie rope here to descend
{ "railing",
  "There is a strong railing bolted to the rock at the edge of the dome.",
  nullptr, actRailing,
  DOME, 0, 0, 0, 0,
  VISBIT|SACREDBIT|NDESCBIT|TIEBIT
},
// [BASKET] Basket — in SHAFT; raise/lower with mechanism
{ "large basket",
  "There is a large basket attached to a rope and pulley here.",
  nullptr, actBasket,
  SHAFT, 0, 50, 0, 0,
  VISBIT|CONTBIT|SACREDBIT|OPENBIT|NDESCBIT
},
// [BOAT] Rubber boat deflated/folded — at Dam Base; INFLATE with pump
{ "pile of plastic",
  "There is a small pile of folded plastic here.",
  nullptr, actBoat,
  DAMB, 5, 0, 0, 0,
  VISBIT|TAKEBIT
},
// [BOATI] Rubber boat inflated — starts in limbo; created by INFLATE
{ "rubber boat",
  "There is an inflated rubber boat here.",
  nullptr, actBoatI,
  ROOM_NONE, 30, 40, 0, 0,
  VISBIT|VEHBIT|SACREDBIT|CONTBIT|OPENBIT
},

// ─── Treasures ─────────────────────────────────────────────────────────────

// [PAINTING] Oil painting — Gallery; value 11
{ "oil painting",
  "A large oil painting hangs on the wall here.",
  nullptr, actPainting,
  GALLE, 30, 0, 0, 11,
  VISBIT|TAKEBIT|SACREDBIT  // SACREDBIT cleared by actPainting on first TAKE
},
// [COINS] Bag of zorkmid coins — Maze4; value 15
{ "bag of zorkmid coins",
  "There is a bag of zorkmid coins here.",
  nullptr, nullptr,
  MAZE4, 5, 0, 0, 15,
  VISBIT|TAKEBIT
},
// [TRIDEN] Crystal trident — Atlantis Room; value 15
{ "crystal trident",
  "There is a magnificent crystal trident here.",
  nullptr, nullptr,
  ATLAN, 10, 0, 0, 15,
  VISBIT|TAKEBIT
},
// [EGGS] Jeweled eggs — in Clearing (in tree); value 12
{ "jeweled eggs",
  "There are some exquisite jeweled eggs here.",
  nullptr, actEggs,
  CLEAR, 5, 0, 0, 12,
  VISBIT|TAKEBIT|NDESCBIT
},
// [TRUNK] Trunk of jewels — starts in limbo; appears in Reservoir when drained
{ "trunk of jewels",
  "There is an enormous trunk here, positively overflowing with jewels.",
  nullptr, nullptr,
  ROOM_NONE, 50, 0, 0, 23,
  VISBIT|TAKEBIT|SACREDBIT
},
// [COFFIN] Crystal coffin — Egyptian Room; closed; contains sceptre
{ "crystal coffin",
  "A closed crystal coffin rests upon a stone bier.",
  nullptr, actCoffin,
  EGYPT, 50, 30, 0, 0,
  VISBIT|CONTBIT|SACREDBIT|NDESCBIT
},
// [SCEPTR] Jeweled sceptre — inside the coffin; value 14
{ "jeweled sceptre",
  nullptr, nullptr, nullptr,
  INS(COFFIN), 10, 0, 0, 14,
  VISBIT|TAKEBIT
},
// [EMERALD] Large emerald — Sandy Beach; value 15
{ "large emerald",
  "There is a large emerald here.",
  nullptr, nullptr,
  BEACH, 5, 0, 0, 15,
  VISBIT|TAKEBIT
},
// [BAR] Platinum bar — in Loud Room; appears after ECHO command; value 22
{ "platinum bar",
  nullptr, nullptr, actBar,
  LOUD, 20, 0, 0, 22,
  TAKEBIT|NDESCBIT  // not VISBIT — appears only after ECHO
},
// [SCARAB] Sapphire scarab — Timber Room crack; value 14
{ "sapphire scarab",
  "There is a sapphire scarab here.",
  nullptr, nullptr,
  TIMB, 3, 0, 0, 14,
  VISBIT|TAKEBIT
},
// [DIAMOND] Large diamond — output of coal machine; starts in limbo; value 16
{ "large diamond",
  "There is a large diamond here, sparkling brilliantly.",
  nullptr, nullptr,
  ROOM_NONE, 3, 0, 0, 16,
  VISBIT|TAKEBIT
},
// [RUBY] Large ruby — Gas Room; value 23
{ "large ruby",
  "There is a large ruby here.",
  nullptr, nullptr,
  GAS, 3, 0, 0, 23,
  VISBIT|TAKEBIT
},
// [CHALICE] Crystal chalice — Treasure Room; value 20
{ "crystal chalice",
  "There is an elegant crystal chalice here.",
  nullptr, nullptr,
  TREAS, 5, 0, 0, 20,
  VISBIT|TAKEBIT
},
// [JADE] Jade figurine — Bat Cave; value 10
{ "jade figurine",
  "There is a small jade figurine here.",
  nullptr, nullptr,
  BATCV, 3, 0, 0, 10,
  VISBIT|TAKEBIT
},
// [BRACELET] Jeweled bracelet — Bat Cave; value 8
{ "jeweled bracelet",
  "There is a jeweled bracelet here.",
  nullptr, nullptr,
  BATCV, 3, 0, 0, 8,
  VISBIT|TAKEBIT
},
// [SPHERE] Crystal sphere — Ledge; value 12
{ "crystal sphere",
  "There is a beautiful crystal sphere here.",
  nullptr, nullptr,
  LEDGE, 5, 0, 0, 12,
  VISBIT|TAKEBIT
},
// [SKULL] Crystal skull — Gully; value 10
{ "crystal skull",
  "There is a crystal skull here.",
  nullptr, nullptr,
  GULLY, 5, 0, 0, 10,
  VISBIT|TAKEBIT
},

// ─── Villains ──────────────────────────────────────────────────────────────

// [TROLL] Troll — in Troll Room; wields axe; aggressive
{ "troll",
  "A nasty troll, wielding a bloody axe, blocks your passage.",
  nullptr, actTrolll,
  MTROL, 0, 0, 4, 0,
  VISBIT|VILLAIN|ACTORBIT|VICBIT|NDESCBIT|FIGHTBIT
},
// [CYCLOPS] Cyclops — in Large Low Room; blocks stairs to Treasure Room
{ "cyclops",
  "A cyclops, who looks like he could eat a horse -- uncooked -- is "
  "blocking the staircase.",
  nullptr, actCyclops,
  LROOM2, 0, 0, 6, 0,
  VISBIT|VILLAIN|ACTORBIT|VICBIT|NDESCBIT
},
// [THIEF] Master Thief — starts in Treasure Room; roams dungeon
{ "seedy-looking thief",
  "A seedy-looking individual with a large bag is here.",
  nullptr, actThief,
  TREAS, 0, 0, 3, 0,
  VISBIT|VILLAIN|ACTORBIT|VICBIT|NDESCBIT|SLEEPBIT
},

// ─── Villain weapons (inside their owners) ─────────────────────────────────

// [AXE] Troll's bloody axe
{ "bloody axe",
  nullptr, nullptr, nullptr,
  INS(TROLL), 10, 0, 5, 0,
  VISBIT|TAKEBIT|WEAPONBIT
},
// [STILET] Thief's stiletto
{ "stiletto",
  nullptr, nullptr, nullptr,
  INS(THIEF), 3, 0, 4, 0,
  VISBIT|TAKEBIT|WEAPONBIT
},
// [TKNIFE] Thief's throwing knife
{ "knife",
  nullptr, nullptr, nullptr,
  INS(THIEF), 2, 0, 2, 0,
  VISBIT|TAKEBIT|WEAPONBIT
},

}; // end gObjects[]

// ── Object action handlers ─────────────────────────────────────────────────

static bool actMailbox(ObjId self, ActionType act) {
    switch (act) {
    case ActionType::OPEN:
        if (obj(self).isOpen()) { game::tell("The small mailbox is already open."); return true; }
        obj(self).setFlag(OPENBIT);
        game::tell("Opening the small mailbox reveals a leaflet.");
        return true;
    case ActionType::CLOSE:
        if (!obj(self).isOpen()) { game::tell("The mailbox is already closed."); return true; }
        obj(self).clrFlag(OPENBIT);
        game::tell("The small mailbox is now closed.");
        return true;
    case ActionType::EXAMINE: {
        game::tellf("The small mailbox is %s.", obj(self).isOpen() ? "open" : "closed");
        if (obj(self).isOpen()) {
            ObjId buf[4]; int n = world::objectsIn(self, buf, 4);
            for (int i = 0; i < n; ++i)
                game::tellf("  A %s.", obj(buf[i]).desc);
        }
        return true;
    }
    default: return false;
    }
}

static bool actLeaflet(ObjId self, ActionType act) {
    if (act == ActionType::READ) { game::tell(obj(self).text); return true; }
    return false;
}

static bool actLantern(ObjId self, ActionType act) {
    switch (act) {
    case ActionType::LIGHT:
    case ActionType::TURN:
        if (!obj(self).isCarried()) { game::tell("You need to be holding the lantern."); return true; }
        if (obj(self).isOn()) { game::tell("The brass lantern is already on."); }
        else { obj(self).setFlag(ONBIT); game::tell("The brass lantern is now on."); }
        return true;
    case ActionType::EXTINGUISH:
        if (!obj(self).isOn()) { game::tell("The brass lantern is already off."); }
        else { obj(self).clrFlag(ONBIT); game::tell("The brass lantern is now off."); }
        return true;
    case ActionType::EXAMINE:
        game::tellf("The brass lantern is battery-powered and is currently %s.",
                    obj(self).isOn() ? "on" : "off");
        return true;
    default: return false;
    }
}

static bool actTorch(ObjId /*self*/, ActionType act) {
    if (act == ActionType::EXTINGUISH) {
        game::tell("The torch cannot be extinguished -- it is magical."); return true;
    }
    if (act == ActionType::EXAMINE) {
        game::tell("The ivory torch burns with a clear, steady flame. It "
                   "is beautifully crafted from a single piece of ivory.");
        return true;
    }
    return false;
}

static bool actCandles(ObjId self, ActionType act) {
    if (act == ActionType::LIGHT || act == ActionType::BURN) {
        bool hasFlame = false;
        for (int i = 0; i < NUM_OBJECTS; ++i)
            if (gObjects[i].isCarried() && gObjects[i].hasFlag(FLAMEBIT))
                hasFlame = true;
        if (!hasFlame) { game::tell("You have nothing with which to light the candles."); return true; }
        obj(self).setFlag(static_cast<ObjFlags>(ONBIT|FLAMEBIT));
        game::tell("The candles are now lit."); return true;
    }
    if (act == ActionType::EXTINGUISH) {
        obj(self).clrFlag(static_cast<ObjFlags>(ONBIT|FLAMEBIT));
        game::tell("The candles are extinguished."); return true;
    }
    return false;
}

static bool actSword(ObjId /*self*/, ActionType act) {
    if (act == ActionType::EXAMINE) {
        bool danger = false;
        for (int i = 0; i < NUM_OBJECTS; ++i)
            if (gObjects[i].hasFlag(VILLAIN) && gObjects[i].loc == gAdventurer.loc)
                { danger = true; break; }
        game::tell(danger
            ? "The elvish sword is glowing with a faint blue light."
            : "The elvish sword bears an inscription in Elvish script.");
        return true;
    }
    return false;
}

static bool actLeaves(ObjId self, ActionType act) {
    if (act == ActionType::MOVE || act == ActionType::TURN || act == ActionType::PULL) {
        if (!obj(self).hasFlag(TOUCHBIT)) {
            obj(self).setFlag(TOUCHBIT);
            obj(GRATE).setFlag(VISBIT);  // grate becomes visible
            game::tell("Moving the leaves aside reveals a steel grate set into the earth.");
        } else {
            game::tell("The leaves are already cleared away.");
        }
        return true;
    }
    return false;
}

static bool actGrate(ObjId self, ActionType act) {
    switch (act) {
    case ActionType::OPEN:
        if (obj(self).isOpen()) { game::tell("The grate is already open."); return true; }
        if (obj(self).hasFlag(LOCKBIT)) {
            if (obj(KEYS).isCarried()) {
                obj(self).clrFlag(LOCKBIT); obj(self).setFlag(OPENBIT);
                game::tell("The grate is now unlocked and open.");
            } else {
                game::tell("The grate is locked. You'll need a key.");
            }
        } else {
            obj(self).setFlag(OPENBIT);
            game::tell("The grate is now open.");
        }
        return true;
    case ActionType::CLOSE:
        if (!obj(self).isOpen()) { game::tell("The grate is already closed."); return true; }
        obj(self).clrFlag(OPENBIT); game::tell("The grate is now closed."); return true;
    case ActionType::UNLOCK:
        if (!obj(self).hasFlag(LOCKBIT)) { game::tell("The grate is already unlocked."); return true; }
        if (!obj(KEYS).isCarried()) { game::tell("You don't have the right key."); return true; }
        obj(self).clrFlag(LOCKBIT); game::tell("The grate is now unlocked."); return true;
    default: return false;
    }
}

static bool actTrapDoor(ObjId self, ActionType act) {
    switch (act) {
    case ActionType::OPEN:
        if (obj(self).isOpen()) { game::tell("The trap door is already open."); return true; }
        if (!obj(RUG).hasFlag(TOUCHBIT)) {
            game::tell("The large rug is in the way. You should move it first."); return true;
        }
        obj(self).setFlag(OPENBIT);
        game::tell("The trap door opens, revealing a rickety staircase descending into darkness.");
        return true;
    case ActionType::CLOSE:
        if (!obj(self).isOpen()) { game::tell("The trap door is already closed."); return true; }
        obj(self).clrFlag(OPENBIT); game::tell("The trap door closes."); return true;
    default: return false;
    }
}

static bool actRug(ObjId self, ActionType act) {
    if (act == ActionType::MOVE || act == ActionType::TURN || act == ActionType::PULL) {
        if (!obj(self).hasFlag(TOUCHBIT)) {
            obj(self).setFlag(TOUCHBIT);
            game::tell("With a great effort, the rug is moved to one side of the room, "
                       "revealing a closed trap door beneath it.");
        } else {
            game::tell("The rug has already been moved aside.");
        }
        return true;
    }
    if (act == ActionType::TAKE) {
        game::tell("The rug is far too heavy to take."); return true;
    }
    return false;
}

static bool actTrophyCase(ObjId self, ActionType act) {
    if (act == ActionType::EXAMINE) {
        ObjId buf[32]; int n = world::objectsIn(self, buf, 32);
        if (n == 0) { game::tell("The trophy case is empty."); return true; }
        game::tell("The trophy case contains:");
        for (int i = 0; i < n; ++i) game::tellf("  A %s.", obj(buf[i]).desc);
        return true;
    }
    return false;
}

static bool actBottle(ObjId self, ActionType act) {
    if (act == ActionType::EXAMINE) {
        ObjId buf[4]; int n = world::objectsIn(self, buf, 4);
        if (n == 0) game::tell("The glass bottle is empty.");
        else game::tellf("The glass bottle contains a %s.", obj(buf[0]).desc);
        return true;
    }
    return false;
}

static bool actSack(ObjId self, ActionType act) {
    if (act == ActionType::EXAMINE) {
        ObjId buf[8]; int n = world::objectsIn(self, buf, 8);
        if (n == 0) game::tell("The brown sack is empty.");
        else { game::tell("The brown sack contains:"); for (int i=0;i<n;++i) game::tellf("  A %s.", obj(buf[i]).desc); }
        return true;
    }
    return false;
}

static bool actRope(ObjId /*self*/, ActionType act) {
    if (act == ActionType::TIE && gAdventurer.loc == DOME) {
        // Tie to railing automatically
        return actRailing(RAILING, ActionType::TIE);
    }
    return false;
}

static bool actRailing(ObjId self, ActionType act) {
    if (act == ActionType::TIE) {
        if (!obj(ROPE).isCarried()) { game::tell("You don't have any rope to tie."); return true; }
        obj(self).setFlag(TOUCHBIT);  // TOUCHBIT = rope is tied here
        world::moveTo(ROPE, static_cast<RoomId>(DOME));
        game::tell("The rope is now tied securely to the railing. It hangs "
                   "down into the Torch Room below.");
        return true;
    }
    return false;
}

static bool actCoffin(ObjId self, ActionType act) {
    switch (act) {
    case ActionType::OPEN:
        if (obj(self).isOpen()) { game::tell("The coffin is already open."); return true; }
        obj(self).setFlag(OPENBIT);
        game::tell("The lid of the crystal coffin swings open to reveal the jeweled sceptre.");
        return true;
    case ActionType::EXAMINE:
        game::tell(obj(self).isOpen()
            ? "The crystal coffin is open."
            : "The crystal coffin is closed. It is an exquisite work of art.");
        return true;
    default: return false;
    }
}

static bool actPainting(ObjId self, ActionType act) {
    if (act == ActionType::TAKE) {
        // First time: clear SACREDBIT so the default TAKE code can proceed
        obj(self).clrFlag(SACREDBIT);
        return false;  // let default TAKE handle it
    }
    if (act == ActionType::EXAMINE) {
        game::tell("The painting is a Velázquez -- a portrait of a man who "
                   "looks remarkably like a burglar.");
        return true;
    }
    if (act == ActionType::BURN) {
        world::moveTo(self, ROOM_NONE);
        game::tell("The painting is consumed by fire, leaving only a charred frame.");
        gAdventurer.score -= obj(self).value;
        return true;
    }
    return false;
}

static bool actWindow(ObjId self, ActionType act) {
    switch (act) {
    case ActionType::OPEN:
        if (obj(self).isOpen()) { game::tell("The window is already open."); return true; }
        obj(self).setFlag(OPENBIT); game::tell("The window is now open."); return true;
    case ActionType::CLOSE:
        if (!obj(self).isOpen()) { game::tell("The window is already closed."); return true; }
        obj(self).clrFlag(OPENBIT); game::tell("The window is now closed."); return true;
    default: return false;
    }
}

static bool actBoat(ObjId self, ActionType act) {
    if (act == ActionType::INFLATE) {
        if (!obj(PUMP).isCarried()) { game::tell("You need the air pump to inflate it."); return true; }
        world::moveTo(BOATI, gAdventurer.loc);
        world::moveTo(self,  ROOM_NONE);
        game::tell("You inflate the rubber boat. It expands to its full size, "
                   "easily big enough to hold a person and some cargo.");
        return true;
    }
    if (act == ActionType::EXAMINE) {
        game::tell("The pile of plastic is a deflated rubber boat, rolled into a compact bundle.");
        return true;
    }
    return false;
}

static bool actBoatI(ObjId self, ActionType act) {
    if (act == ActionType::DEFLATE) {
        world::moveTo(BOAT, gAdventurer.loc);
        world::moveTo(self, ROOM_NONE);
        game::tell("The boat deflates rapidly."); return true;
    }
    if (act == ActionType::EXAMINE) {
        game::tell("The rubber boat is inflated and ready for the water."); return true;
    }
    return false;
}

static bool actBasket(ObjId self, ActionType act) {
    if (act == ActionType::EXAMINE) {
        ObjId buf[8]; int n = world::objectsIn(self, buf, 8);
        if (n == 0) game::tell("The basket is empty.");
        else { game::tell("The basket contains:"); for (int i=0;i<n;++i) game::tellf("  A %s.", obj(buf[i]).desc); }
        return true;
    }
    return false;
}

static bool actLunch(ObjId self, ActionType act) {
    if (act == ActionType::EAT) {
        world::moveTo(self, ROOM_NONE);
        gAdventurer.health = std::min<int16_t>(gAdventurer.health + 2, gAdventurer.maxHealth);
        game::tell("The lunch is delicious. You feel much better."); return true;
    }
    if (act == ActionType::GIVE && gAdventurer.loc == LROOM2 && gObjects[CYCLOPS].loc == LROOM2) {
        world::moveTo(self, ROOM_NONE);
        gObjects[CYCLOPS].setFlag(SLEEPBIT);
        game::tell("The cyclops snatches the lunch from your hand and devours it greedily. "
                   "He then yawns enormously and falls into a deep sleep."); return true;
    }
    return false;
}

static bool actBell(ObjId /*self*/, ActionType act) {
    if (act == ActionType::TURN || act == ActionType::PUSH) {
        game::tell("The bell rings with a clear, piercing tone that echoes strangely.");
        return true;
    }
    return false;
}

static bool actBook(ObjId self, ActionType act) {
    if (act == ActionType::READ) { game::tell(obj(self).text); return true; }
    return false;
}

static bool actCyclops(ObjId /*self*/, ActionType act) {
    switch (act) {
    case ActionType::ATTACK:
        game::tell("The cyclops ignores your feeble attack."); return true;
    case ActionType::EXAMINE:
        game::tell("The cyclops is enormous. His single eye watches you with "
                   "a gleam of hungry anticipation."); return true;
    default: return false;
    }
}

static bool actTrolll(ObjId /*self*/, ActionType /*act*/) {
    return false;  // handled by melee + room action
}

static bool actThief(ObjId /*self*/, ActionType /*act*/) {
    return false;  // handled by game loop roaming logic
}

static bool actMachine(ObjId self, ActionType act) {
    if (act == ActionType::OPEN) {
        if (obj(self).isOpen()) { game::tell("The machine lid is already open."); return true; }
        obj(self).setFlag(OPENBIT); game::tell("You open the machine lid."); return true;
    }
    if (act == ActionType::CLOSE) {
        if (!obj(self).isOpen()) { game::tell("The machine lid is already closed."); return true; }
        obj(self).clrFlag(OPENBIT); game::tell("You close the machine lid."); return true;
    }
    if (act == ActionType::TURN || act == ActionType::PUSH) {
        // Turn/push the control switch: if coal inside, produce diamond
        bool hasCoal = false;
        ObjId contents[4]; int n = world::objectsIn(self, contents, 4);
        ObjId coalObj = OBJ_NONE;
        for (int i = 0; i < n; ++i) {
            if (std::strstr(gObjects[contents[i]].desc, "coal")) {
                hasCoal = true; coalObj = contents[i]; break;
            }
        }
        if (hasCoal) {
            world::moveTo(coalObj, ROOM_NONE);
            world::moveTo(DIAMOND, MACH);
            game::tell("The machine grinds and shudders. There is a flash of "
                       "light. A large diamond drops out of the machine.");
        } else {
            game::tell("The machine rattles and groans, but nothing happens.");
        }
        return true;
    }
    return false;
}

static bool actBar(ObjId self, ActionType /*act*/) {
    // The bar becomes visible after ECHO in the Loud Room
    // (handled in actions.cpp INCANT handler; this handler is rarely called directly)
    (void)self;
    return false;
}

static bool actEggs(ObjId self, ActionType act) {
    if (act == ActionType::EXAMINE) {
        game::tell("The jeweled eggs are warm to the touch and emit a faint glow.");
        return true;
    }
    return false;
}

// ── world:: namespace implementations ─────────────────────────────────────
namespace world {

bool isLit(RoomId r) {
    if (gRooms[r].hasFlag(RLIGHTBIT)) return true;
    // Check light sources carried by player or in this room
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        if (!gObjects[i].isLight()) continue;
        RoomId loc = gObjects[i].loc;
        if (loc == ROOM_CARRIED || loc == ROOM_WORN) {
            if (r == gAdventurer.loc) return true;
        } else if (loc == r) {
            return true;
        } else if (isInsideCont(loc)) {
            ObjId cont = containerOfLoc(loc);
            if (gObjects[cont].isCarried() &&
                (gObjects[cont].hasFlag(TRANSBIT) || gObjects[cont].isOpen()))
                if (r == gAdventurer.loc) return true;
        }
    }
    return false;
}

bool isHeld(ObjId o) {
    RoomId loc = gObjects[o].loc;
    return loc == ROOM_CARRIED || loc == ROOM_WORN;
}

bool isInRoom(ObjId o, RoomId r) { return gObjects[o].loc == r; }

bool isAccessible(ObjId o) {
    RoomId loc = gObjects[o].loc;
    if (loc == ROOM_CARRIED || loc == ROOM_WORN) return true;
    if (loc == gAdventurer.loc) return true;
    if (isInsideCont(loc)) {
        ObjId cont = containerOfLoc(loc);
        if ((gObjects[cont].isOpen() || gObjects[cont].hasFlag(TRANSBIT)) &&
            isAccessible(cont)) return true;
    }
    return false;
}

int objectsInRoom(RoomId r, ObjId* buf, int maxN) {
    int n = 0;
    for (int i = 0; i < NUM_OBJECTS && n < maxN; ++i)
        if (gObjects[i].loc == r) buf[n++] = static_cast<ObjId>(i);
    return n;
}

int objectsCarried(ObjId* buf, int maxN) {
    int n = 0;
    for (int i = 0; i < NUM_OBJECTS && n < maxN; ++i)
        if (gObjects[i].loc == ROOM_CARRIED || gObjects[i].loc == ROOM_WORN)
            buf[n++] = static_cast<ObjId>(i);
    return n;
}

int objectsIn(ObjId container, ObjId* buf, int maxN) {
    RoomId cl = locOfContainer(container);
    int n = 0;
    for (int i = 0; i < NUM_OBJECTS && n < maxN; ++i)
        if (gObjects[i].loc == cl) buf[n++] = static_cast<ObjId>(i);
    return n;
}

uint16_t containerBulk(ObjId container) {
    ObjId buf[32]; int n = objectsIn(container, buf, 32);
    uint16_t total = 0;
    for (int i = 0; i < n; ++i) total += gObjects[buf[i]].size;
    return total;
}

void moveTo(ObjId o, RoomId r)       { gObjects[o].loc = r; }
void placeIn(ObjId o, ObjId container){ gObjects[o].loc = locOfContainer(container); }
bool hasLight()                       { return isLit(gAdventurer.loc); }

ObjId findObject(const char* name, bool inRoom, bool inInventory) {
    ObjId result = OBJ_NONE; int matches = 0;
    auto check = [&](ObjId id) {
        if (std::strstr(gObjects[id].desc, name)) { result = id; ++matches; }
    };
    if (inRoom)
        for (int i = 0; i < NUM_OBJECTS; ++i)
            if (gObjects[i].loc == gAdventurer.loc) check(static_cast<ObjId>(i));
    if (inInventory)
        for (int i = 0; i < NUM_OBJECTS; ++i)
            if (gObjects[i].isCarried() || gObjects[i].isWorn()) check(static_cast<ObjId>(i));
    return (matches == 1) ? result : OBJ_NONE;
}

void scoreTreasure(ObjId o) {
    if (gObjects[o].value > 0) {
        gAdventurer.score += gObjects[o].value;
        game::tellf("[Your score has just gone up by %d points.]", gObjects[o].value);
    }
}

void init() {
    // Grate is hidden until leaves are moved
    gObjects[GRATE].clrFlag(VISBIT);
    // Inflated boat starts in limbo (created when BOAT is inflated)
    gObjects[BOATI].loc = ROOM_NONE;
    // Diamond starts in limbo (produced by machine puzzle)
    gObjects[DIAMOND].loc = ROOM_NONE;
    // Trunk starts in limbo (appears in Reservoir when drained)
    gObjects[TRUNK].loc = ROOM_NONE;
}

} // namespace world

// ── Object::contentsBulk() defined here (needs world context) ─────────────
uint16_t Object::contentsBulk() const {
    ObjId myId = static_cast<ObjId>(this - gObjects);
    return world::containerBulk(myId);
}
