#pragma once
// ============================================================================
// world.h — All game entity declarations
//
// Room and object IDs match the original MDL atom names from dung.mud/rooms.mud
// as closely as possible.  Where the 1977 source uses atoms like "CELLA",
// "MTROL", "CAROU" etc., we use those same names verbatim.
// ============================================================================

#include "object.h"
#include "room.h"

// ---------------------------------------------------------------------------
// Room IDs — indices into gRooms[]
// ---------------------------------------------------------------------------
enum RoomID : RoomId {

    // ── Surface (RLIGHTBIT | RLANDBIT) ───────────────────────────────────
    WHOUS = 0,   // West of House          (mailbox, front door)
    NHOUS,       // North of House
    SHOUS,       // South of House
    EHOUS,       // Behind House           (kitchen window)
    FORE1,       // Forest  (W of house)
    FORE2,       // Forest  (SE of house)
    FORE3,       // Forest  (N of house)
    FORE4,       // Forest  (deep W; looping)
    CLEAR,       // Clearing               (climbable tree; grate to underground)
    LPATH,       // Leaf-strewn Path
    STRON,       // Stone Barrow           (game-ending site)

    // ── House (RLIGHTBIT | RLANDBIT) ─────────────────────────────────────
    LROOM,       // Living Room            (lamp, sword, rug, trophy case, trapdoor)
    KITCH,       // Kitchen                (sack, bottle; window to EHOUS; chimney)
    ATTIC,       // Attic                  (rope, nasty knife)
    BLROO,       // Strange Passage        (W of LROOM via gothic door; magic only)

    // ── Upper Dungeon ─────────────────────────────────────────────────────
    CELLA,       // Cellar                 (foot of trapdoor; dungeon crossroads)
    EASTW,       // East-West Passage      (E of cellar; connects loud room, gallery)
    RAVI1,       // Deep Ravine            (part of E-W passage; S branch)
    GALLE,       // Gallery                (oil painting; N→Studio chimney)
    STUDIO,      // Studio                 (UP the chimney → Kitchen; N of Gallery)
    SEWER,       // Sewer Passage          (dark connector; S of troll, N of gully area)
    MTROL,       // Troll Room             (troll blocks east exit; N→Sewer, S→Cellar)

    // ── The Maze ─────────────────────────────────────────────────────────
    MAZE1,       // Maze  (entry from W of Troll Room)
    MAZE2,       // Maze  (W of MAZE1)
    MAZE3,       // Maze  (W of MAZE2; UP→coin room)
    MAZE4,       // Maze  (UP from MAZE3; bag of coins + skeleton key)
    MAZE5,       // Maze  (dead-end branch)
    MAZEE,       // Maze  (E branch; connects back to Round Room area)

    // ── Mid-Dungeon ───────────────────────────────────────────────────────
    CAROU,       // Round Room             (8 exits; disorienting hub)
    DPASS,       // Damp Passage           (W of Round Room → Engravings)
    ENGRA,       // Engravings Cave        (NE of Damp Passage; old-man puzzle)
    LROOM2,      // Large Low Room         (Cyclops guards UP→Treasure Room)
    TREAS,       // Treasure Room          (Thief's lair; chalice; above Cyclops Room)
    DOME,        // Dome Room              (SE of Round Room; rope-descend to Torch Room)
    TROOM,       // Torch Room             (below dome via rope; ivory torch)
    TEMPL,       // Temple                 (S of Torch Room; candles, bell, book)
    ALTAR,       // Altar                  (S of Temple; pray→teleport Forest)
    EGYPT,       // Egyptian Room          (E of Torch Room; crystal coffin)
    LOUD,        // Loud Room              (E of East-West Passage; "echo" puzzle)

    // ── Dam & Reservoir ───────────────────────────────────────────────────
    DAMTOP,      // Flood Control Dam #3   (top of dam; NE from Loud Room)
    LOBBY,       // Maintenance Lobby      (N of dam top; matchbook)
    MAINT,       // Maintenance Room       (N of Lobby; wrench, screwdriver, buttons)
    RESS,        // Reservoir South        (SW of dam; connects to reservoir)
    RESER,       // Reservoir              (trunk of jewels when dam is drained)
    RESNW,       // Reservoir NW
    RESN,        // Reservoir North        (air pump)
    ATLAN,       // Atlantis Room          (N of Reservoir North; crystal trident)
    DAMB,        // Dam Base               (E of dam; folded rubber boat)

    // ── River (boat travel) ───────────────────────────────────────────────
    RIVR1,       // Frigid River  (1 — launch from Dam Base)
    RIVR2,       // Frigid River  (2 — Sandy Beach to E)
    FALLS,       // Falls                  (boat destroyed if taken over)
    NCAVE,       // Narrow Canyon / Shore  (land after falls; N of Beach)
    BEACH,       // Sandy Beach            (E of River 2; emerald here)
    GULLY,       // Gully                  (NW of beach; leads to lower passages)

    // ── Coal Mine ─────────────────────────────────────────────────────────
    SHAFT,       // Shaft Room             (basket for lowering items; E of Loud)
    COALM,       // Coal Mine              (coal here)
    TIMB,        // Timber Room            (squeeze W through crack; scarab here)
    MACH,        // Machine Room           (coal → diamond via machine)
    GAS,         // Gas Room               (flammable; ruby here)
    BATCV,       // Bat Cave               (bat; garlic repels)
    SLIDE,       // Slide Room             (slide DOWN → Cellar)

    // ── Chasm ─────────────────────────────────────────────────────────────
    CHASM,       // Chasm                  (rickety bridge)
    LEDGE,       // Ledge                  (crystal sphere)

    // ── Mirror Rooms ──────────────────────────────────────────────────────
    MIRR1,       // Mirror Room  (N dungeon half)
    MIRR2,       // Mirror Room  (S dungeon half; touch → teleport to MIRR1)

    NUM_ROOMS
};

// ---------------------------------------------------------------------------
// Object IDs — indices into gObjects[]
// ---------------------------------------------------------------------------
enum ObjID : ObjId {

    // ── Light sources ─────────────────────────────────────────────────────
    LAMP   = 0,  // Brass lantern       (LROOM; battery-powered)
    TORCH,       // Ivory torch         (TROOM; always lit; treasure)
    CANDL,       // Two candles         (TEMPL; need matches)
    MATCH,       // Book of matches     (LOBBY)

    // ── Weapons ───────────────────────────────────────────────────────────
    SWORD,       // Elvish sword        (LROOM; glows blue near danger)
    KNIFE,       // Nasty knife         (ATTIC)

    // ── Tools / key items ─────────────────────────────────────────────────
    ROPE,        // Rope                (ATTIC; tie to dome railing)
    KEYS,        // Set of keys         (MAZE4; opens grate in Clearing)
    LANTER,      // Skeleton key        (MAZE4; opens certain locks)
    BOTTL,       // Glass bottle        (KITCH; contains WATER)
    WATER,       // Quantity of water   (inside BOTTL)
    SACK,        // Brown sack          (KITCH; container; has garlic + lunch)
    GARLIC,      // Clove of garlic     (inside SACK; repels bat)
    LUNCH,       // Lunch               (inside SACK; edible; appeases Cyclops)
    PUMP,        // Air pump            (RESN; inflates boat)
    WRENCH,      // Wrench              (MAINT; turn dam sluice bolt)
    SCRDVR,      // Screwdriver         (MAINT; operate machine switch)
    BELL,        // Brass bell          (TEMPL; ring at Hades entrance)
    BOOK,        // Black book          (TEMPL; read at Hades; contains "ODYSSEUS")
    PAPER,       // Piece of paper      (LROOM; historical note)
    LEAFL,       // Leaflet             (inside MBOX)

    // ── Fixed / scenery ───────────────────────────────────────────────────
    MBOX,        // Small mailbox       (WHOUS; contains LEAFL)
    MAT,         // Welcome mat         (WHOUS)
    FDOOR,       // Front door          (WHOUS; boarded shut)
    WNDOW,       // Kitchen window      (EHOUS & KITCH; starts ajar)
    WDOOR,       // Gothic wood door    (LROOM W wall; nailed shut)
    LEAVES,      // Pile of leaves      (CLEAR; grate hidden under)
    GRATE,       // Steel grate         (CLEAR; locked; KEYS unlock; DOWN→CELLA)
    TRAPDOOR,    // Trap door           (LROOM floor; opened after rug moved)
    RUG,         // Large oriental rug  (LROOM; MOVE→reveals trapdoor)
    TCASE,       // Trophy case         (LROOM; deposit treasures for score)
    RAILING,     // Railing             (DOME; tie ROPE here to descend)
    BASKET,      // Basket              (SHAFT; raise/lower with mechanism)

    // ── Boat ──────────────────────────────────────────────────────────────
    BOAT,        // Rubber boat (deflated/folded)   (DAMB)
    BOATI,       // Rubber boat (inflated/vehicle)  — after INFLATE with PUMP

    // ── Treasures ─────────────────────────────────────────────────────────
    PAINTING,    // Oil painting        (GALLE; value=11)
    COINS,       // Bag of zorkmid coins(MAZE4; value=15)
    TRIDEN,      // Crystal trident     (ATLAN; value=15)
    EGGS,        // Jeweled eggs        (CLEAR tree branch; value=12)
    TRUNK,       // Trunk of jewels     (RESER when drained; value=23)
    COFFIN,      // Crystal coffin      (EGYPT; contains SCEPTR)
    SCEPTR,      // Jeweled sceptre     (inside COFFIN; value=14)
    EMERALD,     // Emerald             (BEACH; value=15)
    BAR,         // Platinum bar        (LOUD; "echo" puzzle; value=22)
    SCARAB,      // Sapphire scarab     (TIMB; value=14)
    DIAMOND,     // Diamond             (MACH output; value=16)
    RUBY,        // Ruby                (GAS room; value=23)
    CHALICE,     // Crystal chalice     (TREAS; thief's item; value=20)
    JADE,        // Jade figurine       (BATCV room; value=10)
    BRACELET,    // Jeweled bracelet    (deep mine; value=8)
    SPHERE,      // Crystal sphere      (LEDGE; value=12)
    SKULL,       // Crystal skull       (GULLY area; value=10)

    // ── Villains ──────────────────────────────────────────────────────────
    TROLL,       // Troll               (MTROL)
    CYCLOPS,     // Cyclops             (LROOM2)
    THIEF,       // Master Thief        (TREAS; roams dungeon)

    // ── Villain weapons ───────────────────────────────────────────────────
    AXE,         // Bloody axe          (Troll carries this)
    STILET,      // Stiletto            (Thief carries this)
    TKNIFE,      // Knife               (Thief can throw)

    NUM_OBJECTS
};

// ---------------------------------------------------------------------------
// Static world arrays (defined in world.cpp)
// ---------------------------------------------------------------------------
extern Room        gRooms  [NUM_ROOMS];
extern Object      gObjects[NUM_OBJECTS];
extern Adventurer  gAdventurer;

inline Room&   room(RoomId id) { return gRooms[id];   }
inline Object& obj (ObjId  id) { return gObjects[id]; }

// ---------------------------------------------------------------------------
// World utilities
// ---------------------------------------------------------------------------
namespace world {
    bool     isLit        (RoomId r);
    bool     isHeld       (ObjId o);
    bool     isAccessible (ObjId o);
    bool     isInRoom     (ObjId o, RoomId r);
    int      objectsInRoom(RoomId r, ObjId* buf, int maxN);
    int      objectsCarried(ObjId* buf, int maxN);
    int      objectsIn    (ObjId container, ObjId* buf, int maxN);
    uint16_t containerBulk(ObjId container);
    void     moveTo       (ObjId o, RoomId r);
    void     placeIn      (ObjId o, ObjId container);
    bool     hasLight     ();
    ObjId    findObject   (const char* name, bool inRoom=true, bool inInventory=true);
    void     scoreTreasure(ObjId o);
    void     init         ();
} // namespace world
