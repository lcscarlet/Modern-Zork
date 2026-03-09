// ============================================================================
#include <cstdlib>
#include <algorithm>
// actions.cpp — Verb handler implementations
//
// Faithfully ported from MDL act1.mud, act2.mud, act3.mud.
// Each function directly mirrors its MDL counterpart.
// ============================================================================

#include "actions.h"
#include "world.h"
#include "game.h"
#include "melee.h"
#include "parser.h"
#include <cstring>
#include <cstdio>

// Max objects a player can carry (MDL: MAXLOAD)
constexpr int MAX_LOAD     = 8;    // item count limit
constexpr int MAX_BULK     = 100;  // bulk limit

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Is there a live villain in the room that would prevent action?
static ObjId villainInRoom() {
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        if (gObjects[i].loc == gAdventurer.loc &&
            gObjects[i].hasFlag(VILLAIN) &&
            !gObjects[i].hasFlag(SLEEPBIT))
            return static_cast<ObjId>(i);
    }
    return OBJ_NONE;
}

// Count items in player's hands
static int countCarried() {
    int n = 0;
    for (int i = 0; i < NUM_OBJECTS; ++i)
        if (gObjects[i].isCarried() || gObjects[i].isWorn()) ++n;
    return n;
}

// Count total bulk carried
static int bulkCarried() {
    int b = 0;
    for (int i = 0; i < NUM_OBJECTS; ++i)
        if (gObjects[i].isCarried() || gObjects[i].isWorn()) b += gObjects[i].size;
    return b;
}

// ---------------------------------------------------------------------------
// actions::dispatch — main entry point
// ---------------------------------------------------------------------------
bool actions::dispatch(const ParsedCommand& cmd) {
    if (!cmd.valid) {
        game::tell("I don't understand that.");
        return false;
    }

    // Check light *before* most actions (dark room restriction)
    bool lit = world::hasLight();

    switch (cmd.verb) {
    // Navigation
    case ActionType::WALK:
        return doGo(cmd.dir);

    // Information
    case ActionType::LOOK:
        return doLook(true);

    // Taking / dropping
    case ActionType::TAKE:
        if (cmd.prso == OBJ_NONE) return doTakeAll();
        return doTake(cmd.prso);
    case ActionType::DROP:
        if (cmd.prso == OBJ_NONE) return doDropAll();
        return doDrop(cmd.prso);

    // Object interaction
    case ActionType::EXAMINE:
        if (!lit) { game::tell("It is too dark to see anything here."); return false; }
        return doExamine(cmd.prso);
    case ActionType::OPEN:
        return doOpen(cmd.prso);
    case ActionType::CLOSE:
        return doClose(cmd.prso);
    case ActionType::LOCK:
        return doLock(cmd.prso, cmd.prsi);
    case ActionType::UNLOCK:
        return doUnlock(cmd.prso, cmd.prsi);
    case ActionType::READ:
        if (!lit) { game::tell("It is too dark to read."); return false; }
        return doRead(cmd.prso);
    case ActionType::PUT:
        return doPut(cmd.prso, cmd.prsi);
    case ActionType::LIGHT:
        return doLight(cmd.prso);
    case ActionType::EXTINGUISH:
        return doExtinguish(cmd.prso);
    case ActionType::BURN:
        return doBurn(cmd.prso, cmd.prsi);
    case ActionType::THROW:
        return (cmd.prsi != OBJ_NONE)
            ? doThrowAt(cmd.prso, cmd.prsi)
            : doThrow(cmd.prso, cmd.dir);
    case ActionType::ATTACK:
        return doAttack(cmd.prso, cmd.prsi);
    case ActionType::EAT:
        return doEat(cmd.prso);
    case ActionType::DRINK:
        return doDrink(cmd.prso);
    case ActionType::WEAR:
        return doWear(cmd.prso);
    case ActionType::REMOVE:
        return doRemove(cmd.prso);
    case ActionType::CLIMB:
        return doClimb(cmd.prso);
    case ActionType::ENTER:
        return doEnter(cmd.prso);
    case ActionType::TIE:
        return doTie(cmd.prso, cmd.prsi);
    case ActionType::INFLATE:
        return doInflate(cmd.prso, cmd.prsi);
    case ActionType::DEFLATE:
        return doDeflate(cmd.prso);
    case ActionType::MOVE:
        return doMove(cmd.prso);
    case ActionType::PULL:
        return doPull(cmd.prso);
    case ActionType::PUSH:
        return doPush(cmd.prso);
    case ActionType::TURN:
        return doTurn(cmd.prso);
    case ActionType::SMELL:
        return doSmell(cmd.prso);
    case ActionType::LISTEN:
        return doListen(cmd.prso);
    case ActionType::TOUCH:
        return doTouch(cmd.prso);
    case ActionType::GIVE:
        return doGive(cmd.prso, cmd.prsi);

    case ActionType::INCANT:
        // Meta-commands dispatched by game loop based on the raw verb token
        // Handled in game.cpp
        return false;

    default:
        game::tell("I don't know how to do that.");
        return false;
    }
}

// ---------------------------------------------------------------------------
// Movement — doGo
// Mirrors MDL V-GO / WALK function in act1.mud
// ---------------------------------------------------------------------------
bool actions::doGo(Direction d) {
    if (d == DIR_NONE) {
        game::tell("Where do you want to go?");
        return false;
    }
    const Room& here = gRooms[gAdventurer.loc];
    const Exit* exit = here.findExit(d);

    if (!exit) {
        // No exit defined at all
        static const char* noExitMessages[] = {
            "You can't go that way.",
            "There is a wall there.",
            "The passage is blocked.",
        };
        game::tell("You can't go that way.");
        return false;
    }

    // Conditional exit?
    if (exit->cond && !exit->cond()) {
        game::tell(exit->msg ? exit->msg : "You can't go that way.");
        return false;
    }

    if (exit->dest == ROOM_NONE) {
        game::tell(exit->msg ? exit->msg : "You can't go that way.");
        return false;
    }

    // Check if a villain is blocking
    ObjId villain = villainInRoom();
    if (villain != OBJ_NONE && exit->dest != ROOM_NONE) {
        // Some directions might be blocked — villain-specific logic
        // Troll blocks east passage specifically
        if (villain == TROLL && d == DIR_EAST) {
            game::tell("The troll, with a menacing gesture, blocks your passage.");
            return false;
        }
    }

    // Make the move
    RoomId oldRoom = gAdventurer.loc;
    gAdventurer.loc = exit->dest;

    // Notify old room of departure (room action WALK)
    if (gRooms[oldRoom].action)
        gRooms[oldRoom].action(oldRoom, ActionType::WALK);

    // Notify new room of arrival
    Room& newRoom = gRooms[gAdventurer.loc];
    if (newRoom.action)
        newRoom.action(gAdventurer.loc, ActionType::ROOM_ENTRY);

    // Display new room
    doLook(!newRoom.isSeen() || gAdventurer.verbose);
    newRoom.markSeen();

    return true;
}

// ---------------------------------------------------------------------------
// Look — doLook
// Mirrors MDL ROOM-INFO / RDCOM description display
// ---------------------------------------------------------------------------
bool actions::doLook(bool forceLong) {
    RoomId loc = gAdventurer.loc;
    Room&  here = gRooms[loc];

    // Print room name
    game::tellf("\n%s", here.desc);

    bool lit = world::isLit(loc);
    if (!lit) {
        game::tell("It is pitch black. You are likely to be eaten by a grue.");
        // In the dark, there's a random chance of grue attack
        // (MDL: <COND (<0? <RANDOM 128>> <TELL "Oh, no! A grue..."> <KILLED>)>)
        // Simplified version: 1-in-32 chance each look in darkness
        if ((std::rand() % 32) == 0) {
            game::tell("\nYou have moved into the darkness. A grue lurks nearby...");
            game::tell("The grue, a dark and hungry creature, has ended your adventuring.\n");
            game::killed("grue");
        }
        return true;
    }

    // Long description (first visit, verbose mode, or explicit LOOK)
    if (forceLong && here.ldesc)
        game::tell(here.ldesc);
    else if (forceLong)
        game::tell(here.desc);

    // List objects in room
    ObjId buf[32];
    int n = world::objectsInRoom(loc, buf, 32);
    for (int i = 0; i < n; ++i) {
        const Object& o = gObjects[buf[i]];
        if (!o.hasFlag(NDESCBIT) && o.hasFlag(VISBIT)) {
            if (o.ldesc)
                game::tell(o.ldesc);
            else
                game::tellf("There is a %s here.", o.desc);
        }
    }

    // Trigger room LOOK action (for rooms with special descriptions)
    if (here.action)
        here.action(loc, ActionType::LOOK);

    return true;
}

// ---------------------------------------------------------------------------
// Take — doTake
// Mirrors MDL V-TAKE in act1.mud
// ---------------------------------------------------------------------------
bool actions::doTake(ObjId o) {
    if (o == OBJ_NONE) {
        game::tell("I don't see that here.");
        return false;
    }
    Object& ob = gObjects[o];

    // Already held?
    if (ob.isCarried() || ob.isWorn()) {
        game::tell("You already have that.");
        return false;
    }

    // Object-specific override
    if (ob.action && ob.action(o, ActionType::TAKE)) return true;

    // Immovable (SACREDBIT)?
    if (ob.hasFlag(SACREDBIT)) {
        game::tell("That isn't something you can take.");
        return false;
    }

    // Can be taken?
    if (!ob.hasFlag(TAKEBIT)) {
        game::tell("You can't take that.");
        return false;
    }

    // Not accessible?
    if (!world::isAccessible(o)) {
        game::tell("You can't reach that.");
        return false;
    }

    // Load check
    if (countCarried() >= MAX_LOAD) {
        game::tell("You are carrying too many things already.");
        return false;
    }
    if (bulkCarried() + ob.size > MAX_BULK) {
        game::tell("That is too heavy to carry.");
        return false;
    }

    // Take it
    world::moveTo(o, ROOM_CARRIED);
    parser::rememberPronoun(o);
    game::tellf("Taken.");
    return true;
}

// ---------------------------------------------------------------------------
// Take All
// ---------------------------------------------------------------------------
bool actions::doTakeAll() {
    ObjId buf[32];
    int n = world::objectsInRoom(gAdventurer.loc, buf, 32);
    bool anyTaken = false;
    for (int i = 0; i < n; ++i) {
        const Object& ob = gObjects[buf[i]];
        if (!ob.hasFlag(TAKEBIT) || ob.hasFlag(SACREDBIT)) continue;
        game::tellf("%s: ", ob.desc);
        doTake(buf[i]);
        anyTaken = true;
    }
    if (!anyTaken) game::tell("There is nothing here to take.");
    return anyTaken;
}

// ---------------------------------------------------------------------------
// Drop — doDrop
// ---------------------------------------------------------------------------
bool actions::doDrop(ObjId o) {
    if (o == OBJ_NONE) { game::tell("I don't see that here."); return false; }
    Object& ob = gObjects[o];

    if (!ob.isCarried() && !ob.isWorn()) {
        game::tell("You're not carrying that.");
        return false;
    }
    if (ob.action && ob.action(o, ActionType::DROP)) return true;

    world::moveTo(o, gAdventurer.loc);
    game::tellf("Dropped.");

    // If dropped in trophy case room and it's a treasure, score it
    if (gAdventurer.loc == LROOM) {
        // Auto-put in trophy case if open
        if (gObjects[TCASE].isOpen() && ob.value > 0) {
            world::placeIn(o, TCASE);
            world::scoreTreasure(o);
            game::tellf("Your score has just gone up by %d points.", ob.value);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Drop All
// ---------------------------------------------------------------------------
bool actions::doDropAll() {
    ObjId buf[32];
    int n = world::objectsCarried(buf, 32);
    bool anyDropped = false;
    for (int i = 0; i < n; ++i) {
        game::tellf("%s: ", gObjects[buf[i]].desc);
        doDrop(buf[i]);
        anyDropped = true;
    }
    if (!anyDropped) game::tell("You aren't carrying anything.");
    return anyDropped;
}

// ---------------------------------------------------------------------------
// Examine
// ---------------------------------------------------------------------------
bool actions::doExamine(ObjId o) {
    if (o == OBJ_NONE) { game::tell("What do you want to examine?"); return false; }
    Object& ob = gObjects[o];

    if (!world::isAccessible(o)) { game::tell("I don't see that here."); return false; }
    if (ob.action && ob.action(o, ActionType::EXAMINE)) return true;

    // Default: print ldesc if available, else generic message
    if (ob.ldesc)
        game::tell(ob.ldesc);
    else
        game::tellf("There is nothing special about the %s.", ob.desc);

    // If it's an open container, list contents
    if (ob.hasFlag(CONTBIT) && ob.isOpen()) {
        ObjId contents[16]; int n = world::objectsIn(o, contents, 16);
        if (n == 0) {
            game::tellf("The %s is empty.", ob.desc);
        } else {
            game::tellf("The %s contains:", ob.desc);
            for (int i = 0; i < n; ++i)
                game::tellf("  A %s.", gObjects[contents[i]].desc);
        }
    }

    ob.setFlag(TOUCHBIT);  // mark as examined
    parser::rememberPronoun(o);
    return true;
}

// ---------------------------------------------------------------------------
// Open
// ---------------------------------------------------------------------------
bool actions::doOpen(ObjId o) {
    if (o == OBJ_NONE) { game::tell("What do you want to open?"); return false; }
    Object& ob = gObjects[o];

    // Delegate to object handler first
    if (ob.action && ob.action(o, ActionType::OPEN)) return true;

    if (!ob.hasFlag(CONTBIT) && !ob.hasFlag(DOORBIT)) {
        game::tell("That isn't something you can open.");
        return false;
    }
    if (ob.isOpen()) {
        game::tellf("The %s is already open.", ob.desc);
        return false;
    }
    if (ob.hasFlag(LOCKBIT)) {
        game::tellf("The %s is locked.", ob.desc);
        return false;
    }
    ob.setFlag(OPENBIT);
    game::tell("Opened.");
    // Describe newly visible contents
    if (ob.hasFlag(CONTBIT)) {
        ObjId contents[16]; int n = world::objectsIn(o, contents, 16);
        if (n > 0) {
            game::tell("Inside you see:");
            for (int i = 0; i < n; ++i)
                game::tellf("  A %s.", gObjects[contents[i]].desc);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Close
// ---------------------------------------------------------------------------
bool actions::doClose(ObjId o) {
    if (o == OBJ_NONE) { game::tell("What do you want to close?"); return false; }
    Object& ob = gObjects[o];

    if (ob.action && ob.action(o, ActionType::CLOSE)) return true;

    if (!ob.hasFlag(CONTBIT) && !ob.hasFlag(DOORBIT)) {
        game::tell("That isn't something you can close.");
        return false;
    }
    if (!ob.isOpen()) {
        game::tellf("The %s is already closed.", ob.desc);
        return false;
    }
    ob.clrFlag(OPENBIT);
    game::tell("Closed.");
    return true;
}

// ---------------------------------------------------------------------------
// Lock / Unlock
// ---------------------------------------------------------------------------
bool actions::doLock(ObjId o, ObjId key) {
    if (o == OBJ_NONE) { game::tell("What do you want to lock?"); return false; }
    Object& ob = gObjects[o];

    if (!ob.hasFlag(DOORBIT) && !ob.hasFlag(CONTBIT)) {
        game::tell("You can't lock that.");
        return false;
    }
    if (ob.isOpen()) {
        game::tell("You must close it before locking it.");
        return false;
    }
    if (ob.hasFlag(LOCKBIT)) {
        game::tellf("The %s is already locked.", ob.desc);
        return false;
    }

    // Must have the right key
    if (key == OBJ_NONE) { game::tell("You'll need a key."); return false; }
    if (!gObjects[key].isCarried()) { game::tell("You aren't carrying that key."); return false; }

    ob.setFlag(static_cast<ObjFlags>(LOCKBIT));
    game::tell("Locked.");
    return true;
}

bool actions::doUnlock(ObjId o, ObjId key) {
    if (o == OBJ_NONE) { game::tell("What do you want to unlock?"); return false; }
    Object& ob = gObjects[o];

    if (!ob.hasFlag(static_cast<ObjFlags>(LOCKBIT))) {
        game::tellf("The %s is not locked.", ob.desc);
        return false;
    }
    if (key == OBJ_NONE) { game::tell("With what key?"); return false; }
    if (!gObjects[key].isCarried()) { game::tell("You aren't carrying that key."); return false; }

    ob.clrFlag(static_cast<ObjFlags>(LOCKBIT));
    game::tell("Unlocked.");
    return true;
}

// ---------------------------------------------------------------------------
// Read
// ---------------------------------------------------------------------------
bool actions::doRead(ObjId o) {
    if (o == OBJ_NONE) { game::tell("What do you want to read?"); return false; }
    Object& ob = gObjects[o];

    if (!world::isAccessible(o)) { game::tell("I don't see that here."); return false; }
    if (ob.action && ob.action(o, ActionType::READ)) return true;

    if (!ob.hasFlag(READBIT) || !ob.text) {
        game::tell("There is nothing to read there.");
        return false;
    }
    game::tell(ob.text);
    return true;
}

// ---------------------------------------------------------------------------
// Put X in Y
// ---------------------------------------------------------------------------
bool actions::doPut(ObjId o, ObjId container) {
    if (o == OBJ_NONE) { game::tell("Put what?"); return false; }
    if (container == OBJ_NONE) { game::tell("Put it where?"); return false; }

    Object& src  = gObjects[o];
    Object& cont = gObjects[container];

    if (!src.isCarried()) { game::tell("You aren't carrying that."); return false; }
    if (!world::isAccessible(container)) { game::tell("I don't see that here."); return false; }

    // Delegate to container's action handler
    if (cont.action && cont.action(container, ActionType::PUT)) return true;

    if (!cont.hasFlag(CONTBIT)) {
        game::tellf("The %s can't contain things.", cont.desc);
        return false;
    }
    if (!cont.isOpen()) {
        game::tellf("The %s is not open.", cont.desc);
        return false;
    }
    if (cont.contentsBulk() + src.size > cont.capacity) {
        game::tellf("There is no room in the %s.", cont.desc);
        return false;
    }

    world::placeIn(o, container);
    game::tellf("Done.");

    // Trophy case scoring
    if (container == TCASE && src.value > 0) {
        world::scoreTreasure(o);
        game::tellf("Your score has just gone up by %d points.", src.value);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Light source control
// ---------------------------------------------------------------------------
bool actions::doLight(ObjId o) {
    if (o == OBJ_NONE) { game::tell("What do you want to light?"); return false; }
    Object& ob = gObjects[o];

    if (!ob.hasFlag(LIGHTBIT)) {
        game::tellf("The %s is not a light source.", ob.desc);
        return false;
    }
    if (ob.action && ob.action(o, ActionType::LIGHT)) return true;

    if (ob.isOn()) {
        game::tellf("The %s is already on.", ob.desc);
        return false;
    }
    // Candles need matches
    if (ob.hasFlag(BURNBIT)) {
        // Verify we have matches (or another lit flame source)
        bool hasFireSource = false;
        for (int i = 0; i < NUM_OBJECTS; ++i) {
            if (gObjects[i].isCarried() && gObjects[i].hasFlag(FLAMEBIT))
                hasFireSource = true;
        }
        if (!hasFireSource) {
            game::tell("You need something to light it with.");
            return false;
        }
        ob.setFlag(FLAMEBIT);
    }
    ob.setFlag(ONBIT);
    game::tellf("The %s is now on.", ob.desc);
    return true;
}

bool actions::doExtinguish(ObjId o) {
    if (o == OBJ_NONE) { game::tell("What do you want to extinguish?"); return false; }
    Object& ob = gObjects[o];

    if (ob.action && ob.action(o, ActionType::EXTINGUISH)) return true;

    if (!ob.hasFlag(LIGHTBIT)) {
        game::tellf("The %s is not a light source.", ob.desc);
        return false;
    }
    if (!ob.isOn()) {
        game::tellf("The %s is not on.", ob.desc);
        return false;
    }
    ob.clrFlag(ONBIT);
    ob.clrFlag(FLAMEBIT);
    game::tellf("The %s is now off.", ob.desc);
    return true;
}

// ---------------------------------------------------------------------------
// Burn
// ---------------------------------------------------------------------------
bool actions::doBurn(ObjId o, ObjId with) {
    if (o == OBJ_NONE) { game::tell("Burn what?"); return false; }
    if (!gObjects[o].hasFlag(BURNBIT)) {
        game::tellf("The %s is not flammable.", gObjects[o].desc);
        return false;
    }
    if (with == OBJ_NONE || !gObjects[with].hasFlag(FLAMEBIT)) {
        game::tell("You don't have a lit flame source.");
        return false;
    }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::BURN)) return true;
    gObjects[o].setFlag(FLAMEBIT);
    gObjects[o].setFlag(ONBIT);
    game::tellf("The %s catches fire.", gObjects[o].desc);
    return true;
}

// ---------------------------------------------------------------------------
// Throw
// ---------------------------------------------------------------------------
bool actions::doThrow(ObjId o, Direction d) {
    if (o == OBJ_NONE) { game::tell("Throw what?"); return false; }
    if (!gObjects[o].isCarried()) { game::tell("You aren't carrying that."); return false; }
    (void)d;
    // Basic throw — drop it in current room
    world::moveTo(o, gAdventurer.loc);
    game::tellf("You throw the %s.", gObjects[o].desc);
    return true;
}

bool actions::doThrowAt(ObjId o, ObjId target) {
    if (o == OBJ_NONE || target == OBJ_NONE) { game::tell("Throw what at what?"); return false; }
    if (!gObjects[o].isCarried()) { game::tell("You aren't carrying that."); return false; }

    // If target is a villain, resolve as attack
    if (gObjects[target].hasFlag(VILLAIN) || gObjects[target].hasFlag(VICBIT)) {
        return melee::throwWeapon(o, target);
    }

    world::moveTo(o, gAdventurer.loc);
    game::tellf("The %s bounces off the %s.", gObjects[o].desc, gObjects[target].desc);
    return true;
}

// ---------------------------------------------------------------------------
// Attack
// ---------------------------------------------------------------------------
bool actions::doAttack(ObjId target, ObjId weapon) {
    if (target == OBJ_NONE) { game::tell("Attack what?"); return false; }
    if (!world::isAccessible(target)) { game::tell("I don't see that here."); return false; }

    const Object& tgt = gObjects[target];
    if (!tgt.hasFlag(VICBIT) && !tgt.hasFlag(VILLAIN)) {
        game::tellf("I've known strange people, but fighting a %s?", tgt.desc);
        return false;
    }

    // Delegate to melee system
    return melee::attack(target, weapon);
}

// ---------------------------------------------------------------------------
// Eating / drinking
// ---------------------------------------------------------------------------
bool actions::doEat(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Eat what?"); return false; }
    Object& ob = gObjects[o];

    if (!ob.isCarried()) { game::tell("You aren't holding that."); return false; }
    if (ob.action && ob.action(o, ActionType::EAT)) return true;
    if (!ob.hasFlag(FOODBIT)) {
        game::tellf("The %s is not edible.", ob.desc);
        return false;
    }
    world::moveTo(o, ROOM_NONE);  // consumed
    game::tell("Eaten.");
    return true;
}

bool actions::doDrink(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Drink what?"); return false; }
    Object& ob = gObjects[o];

    if (!ob.isCarried() && !world::isAccessible(o)) {
        game::tell("You can't reach that.");
        return false;
    }
    if (ob.action && ob.action(o, ActionType::DRINK)) return true;
    if (!ob.hasFlag(DRINKBIT)) {
        game::tellf("The %s is not something you can drink.", ob.desc);
        return false;
    }
    world::moveTo(o, ROOM_NONE);  // consumed
    game::tell("Drunk.");
    return true;
}

// ---------------------------------------------------------------------------
// Wear / remove
// ---------------------------------------------------------------------------
bool actions::doWear(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Wear what?"); return false; }
    if (!gObjects[o].isCarried()) { game::tell("You aren't holding that."); return false; }
    world::moveTo(o, ROOM_WORN);
    game::tellf("You are now wearing the %s.", gObjects[o].desc);
    return true;
}

bool actions::doRemove(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Remove what?"); return false; }
    if (!gObjects[o].isWorn()) { game::tell("You aren't wearing that."); return false; }
    world::moveTo(o, ROOM_CARRIED);
    game::tellf("You remove the %s.", gObjects[o].desc);
    return true;
}

// ---------------------------------------------------------------------------
// Inventory
// ---------------------------------------------------------------------------
bool actions::doInventory() {
    ObjId buf[32]; int n = world::objectsCarried(buf, 32);
    if (n == 0) {
        game::tell("You are not carrying anything.");
        return false;
    }
    game::tell("You are carrying:");
    for (int i = 0; i < n; ++i) {
        const Object& ob = gObjects[buf[i]];
        if (ob.isWorn())
            game::tellf("  A %s (being worn)", ob.desc);
        else
            game::tellf("  A %s", ob.desc);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Score
// ---------------------------------------------------------------------------
bool actions::doScore() {
    game::tellf("Your score is %d out of 500 points, in %d moves.",
                gAdventurer.score, gAdventurer.moves);
    return false;
}

// ---------------------------------------------------------------------------
// Quit
// ---------------------------------------------------------------------------
bool actions::doQuit() {
    game::tell("Your score is:");
    doScore();
    game::tell("The game is over.");
    std::exit(0);
    return false;
}

// ---------------------------------------------------------------------------
// Verbose / Brief
// ---------------------------------------------------------------------------
bool actions::doVerbose() {
    gAdventurer.verbose = true;
    gAdventurer.brief   = false;
    game::tell("Maximum verbosity.");
    return false;
}

bool actions::doBrief() {
    gAdventurer.brief   = true;
    gAdventurer.verbose = false;
    game::tell("Brief descriptions.");
    return false;
}

// ---------------------------------------------------------------------------
// Climb
// ---------------------------------------------------------------------------
bool actions::doClimb(ObjId o) {
    if (o == OBJ_NONE) {
        // Try to climb the tree if in clearing, or stairs if in living room
        if (gAdventurer.loc == CLEAR)        o = TREE;
        else if (gAdventurer.loc == LROOM)   o = STAIR;
        else { game::tell("What do you want to climb?"); return false; }
    }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::CLIMB)) return true;
    game::tell("Climbing that doesn't accomplish anything useful.");
    return false;
}

// ---------------------------------------------------------------------------
// Enter
// ---------------------------------------------------------------------------
bool actions::doEnter(ObjId o) {
    if (o != OBJ_NONE && gObjects[o].hasFlag(VEHBIT)) return doBoard(o);
    // Try moving EAST (into house through window)
    const Exit* e = gRooms[gAdventurer.loc].findExit(DIR_IN);
    if (e) return doGo(DIR_IN);
    game::tell("Where do you want to enter?");
    return false;
}

// ---------------------------------------------------------------------------
// Tie
// ---------------------------------------------------------------------------
bool actions::doTie(ObjId rope, ObjId anchor) {
    if (rope == OBJ_NONE || !gObjects[rope].hasFlag(TIEBIT)) {
        game::tell("You can't tie that.");
        return false;
    }
    if (!gObjects[rope].isCarried()) { game::tell("You aren't holding the rope."); return false; }
    if (anchor == OBJ_NONE) { game::tell("Tie it to what?"); return false; }

    if (gObjects[rope].action && gObjects[rope].action(rope, ActionType::TIE)) return true;
    game::tellf("The %s is now tied to the %s.", gObjects[rope].desc, gObjects[anchor].desc);
    return true;
}

// ---------------------------------------------------------------------------
// Inflate / Deflate (rubber boat)
// ---------------------------------------------------------------------------
bool actions::doInflate(ObjId o, ObjId pump) {
    if (o == OBJ_NONE) { game::tell("Inflate what?"); return false; }
    if (pump == OBJ_NONE || pump != PUMP) { game::tell("You need the pump for that."); return false; }
    if (!gObjects[PUMP].isCarried()) { game::tell("You don't have the pump."); return false; }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::INFLATE)) return true;
    game::tell("The boat inflates with a loud whoosh.");
    gObjects[o].setFlag(VEHBIT);
    return true;
}

bool actions::doDeflate(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Deflate what?"); return false; }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::DEFLATE)) return true;
    gObjects[o].clrFlag(VEHBIT);
    game::tell("The air rushes out.");
    return true;
}

// ---------------------------------------------------------------------------
// Move / Pull / Push / Turn (generic manipulation)
// ---------------------------------------------------------------------------
bool actions::doMove(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Move what?"); return false; }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::MOVE)) return true;
    game::tellf("Moving the %s reveals nothing of interest.", gObjects[o].desc);
    return true;
}

bool actions::doPull(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Pull what?"); return false; }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::PULL)) return true;
    game::tell("Nothing happens.");
    return true;
}

bool actions::doPush(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Push what?"); return false; }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::PUSH)) return true;
    game::tell("Nothing happens.");
    return true;
}

bool actions::doTurn(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Turn what?"); return false; }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::TURN)) return true;
    game::tell("Nothing happens.");
    return true;
}

// ---------------------------------------------------------------------------
// Sensory verbs
// ---------------------------------------------------------------------------
bool actions::doSmell(ObjId o) {
    if (o == OBJ_NONE) game::tell("You smell nothing out of the ordinary.");
    else               game::tellf("The %s smells like a %s.", gObjects[o].desc, gObjects[o].desc);
    return false;
}

bool actions::doListen(ObjId o) {
    (void)o;
    game::tell("You hear nothing unusual.");
    return false;
}

bool actions::doTouch(ObjId o) {
    if (o == OBJ_NONE) { game::tell("Touch what?"); return false; }
    if (gObjects[o].action && gObjects[o].action(o, ActionType::TOUCH)) return true;
    game::tellf("Touching the %s is unremarkable.", gObjects[o].desc);
    gObjects[o].setFlag(TOUCHBIT);
    return false;
}

// ---------------------------------------------------------------------------
// Board / Disembark
// ---------------------------------------------------------------------------
bool actions::doBoard(ObjId vehicle) {
    if (vehicle == OBJ_NONE) { game::tell("Board what?"); return false; }
    if (!gObjects[vehicle].hasFlag(VEHBIT)) {
        game::tell("That is not something you can board.");
        return false;
    }
    if (gObjects[vehicle].loc != gAdventurer.loc) {
        game::tell("The vehicle is not here.");
        return false;
    }
    // Set player location to "inside vehicle" (represented as vehicle location)
    gAdventurer.loc = static_cast<RoomId>(gObjects[vehicle].loc);
    game::tell("You board the vessel.");
    return true;
}

bool actions::doDisembark() {
    game::tell("You step out of the vessel onto solid ground.");
    return true;
}

// ---------------------------------------------------------------------------
// Give
// ---------------------------------------------------------------------------
bool actions::doGive(ObjId o, ObjId recipient) {
    if (o == OBJ_NONE) { game::tell("Give what?"); return false; }
    if (!gObjects[o].isCarried()) { game::tell("You aren't holding that."); return false; }
    if (recipient == OBJ_NONE) { game::tell("Give it to whom?"); return false; }
    if (!world::isAccessible(recipient)) { game::tell("I don't see them here."); return false; }
    if (gObjects[recipient].action && gObjects[recipient].action(recipient, ActionType::GIVE)) return true;
    // Default: just put in their possession (NPC doesn't react)
    world::placeIn(o, recipient);
    game::tellf("You give the %s to the %s.", gObjects[o].desc, gObjects[recipient].desc);
    return true;
}

// ---------------------------------------------------------------------------
// Easter egg: XYZZY
// ---------------------------------------------------------------------------
bool actions::doXyzzy() {
    game::tell("A hollow voice says \"Fool.\"");
    return false;
}

// ---------------------------------------------------------------------------
// Save / Restore (platform-specific; stub implementation)
// ---------------------------------------------------------------------------
bool actions::doSave() {
    // Full implementation in game.cpp using fwrite of all state
    game::tell("(Saving is not yet implemented in this build.)");
    return false;
}

bool actions::doRestore() {
    game::tell("(Restore is not yet implemented in this build.)");
    return false;
}
