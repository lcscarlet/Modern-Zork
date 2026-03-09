// ============================================================================
#include <algorithm>
#include <cctype>// game.cpp — Main game loop and utility functions
//
// Mirrors MDL: RDCOM (main loop), TELL (output), KILLED, save/restore.
// ============================================================================

#include "game.h"
#include "world.h"
#include "actions.h"
#include "parser.h"
#include "melee.h"
#include <curses.h>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------
void game::tell(const char* msg) {
    if (msg) printw("%s\n", msg);
    refresh();
}

void game::tellf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vwprintw(stdscr, fmt, ap);
    printw("\n");
    va_end(ap);
    refresh();
}

void game::cr() {
    printw("\n");
    refresh();
}

// ---------------------------------------------------------------------------
// Death handling
// ---------------------------------------------------------------------------
[[noreturn]] void game::killed(const char* cause) {
    ++gAdventurer.deaths;
    tellf("\n***  You have been %s!  ***\n", cause[0] == 'k' ? "killed" : "killed by a");
    tell("As you take your last breath, you find yourself inside the ");
    tell("Adventurer's Salvation Office, where a cheerful bureaucrat");
    tell("restores you to life (less any possessions you dropped in your death throes).\n");

    // Drop held objects in the room where death occurred
    RoomId deathRoom = gAdventurer.loc;
    ObjId buf[32]; int n = world::objectsCarried(buf, 32);
    for (int i = 0; i < n; ++i) {
        // Drop most things; keep lamp (MDL behavior: keep wielded weapon)
        if (buf[i] != LAMP)
            world::moveTo(buf[i], deathRoom);
    }

    // Restore player to starting room with full health
    gAdventurer.loc    = WHOUS;
    gAdventurer.health = gAdventurer.maxHealth;
    gAdventurer.score  = std::max(0, gAdventurer.score - 10);  // death penalty

    tellf("(Your score is now %d.)\n", gAdventurer.score);

    // Re-enter game loop
    run();
}

// ---------------------------------------------------------------------------
// Save / Restore
// Binary dump of all mutable state: gAdventurer + gObjects[].flags + locs
// + gRooms[].flags.  Strings and function pointers are static — not saved.
// ---------------------------------------------------------------------------

struct SaveState {
    Adventurer   adventurer;
    // For each object: current location and mutable flags
    struct ObjState { RoomId loc; uint32_t flags; } objects[NUM_OBJECTS];
    // For each room: mutable flags
    struct RoomState { uint16_t flags; } rooms[NUM_ROOMS];
};

bool game::save(const char* filename) {
    SaveState ss;
    ss.adventurer = gAdventurer;
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        ss.objects[i].loc   = gObjects[i].loc;
        ss.objects[i].flags = gObjects[i].flags;
    }
    for (int i = 0; i < NUM_ROOMS; ++i)
        ss.rooms[i].flags = gRooms[i].flags;

    std::FILE* f = std::fopen(filename, "wb");
    if (!f) { tell("Cannot open save file."); return false; }
    bool ok = (std::fwrite(&ss, sizeof(ss), 1, f) == 1);
    std::fclose(f);
    if (ok) tell("Saved.");
    return ok;
}

bool game::restore(const char* filename) {
    SaveState ss;
    std::FILE* f = std::fopen(filename, "rb");
    if (!f) { tell("Cannot open save file."); return false; }
    bool ok = (std::fread(&ss, sizeof(ss), 1, f) == 1);
    std::fclose(f);
    if (!ok) { tell("Save file is corrupt."); return false; }

    gAdventurer = ss.adventurer;
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        gObjects[i].loc   = ss.objects[i].loc;
        gObjects[i].flags = ss.objects[i].flags;
    }
    for (int i = 0; i < NUM_ROOMS; ++i)
        gRooms[i].flags = ss.rooms[i].flags;

    tell("Restored.");
    actions::doLook(true);
    return true;
}

// ---------------------------------------------------------------------------
// Banner — mirrors MDL dung.mud header comment / welcome text
// ---------------------------------------------------------------------------
void game::printBanner() {
    tell(
        "\n"
        "ZORK  (c) 1977 MIT\n"
        "Running: Zork (December 1977 version)\n"
        "\n"
        "ZORK is a game of adventure, danger, and low cunning.\n"
        "In it you will explore some of the most amazing territory\n"
        "ever seen by mortals. No computer should be without one!\n"
        "\n"
        "Type HELP for instructions, QUIT to end the game.\n"
    );
}

// ---------------------------------------------------------------------------
// Handle meta-commands (SAVE, RESTORE, QUIT, SCORE, VERBOSE, BRIEF, HELP)
// These are parsed separately because the verb token itself carries the intent.
// ---------------------------------------------------------------------------
static bool handleMetaCommand(const char* rawVerb) {
    if (!std::strcmp(rawVerb, "quit")    || !std::strcmp(rawVerb, "q"))
        actions::doQuit();
    if (!std::strcmp(rawVerb, "save"))
        return game::save("zork.sav");
    if (!std::strcmp(rawVerb, "restore") || !std::strcmp(rawVerb, "load"))
        return game::restore("zork.sav");
    if (!std::strcmp(rawVerb, "score"))
        return actions::doScore();
    if (!std::strcmp(rawVerb, "verbose"))
        return actions::doVerbose();
    if (!std::strcmp(rawVerb, "brief"))
        return actions::doBrief();
    if (!std::strcmp(rawVerb, "inventory") || !std::strcmp(rawVerb, "i"))
        return actions::doInventory();
    if (!std::strcmp(rawVerb, "help")) {
        game::tell(
            "Some useful commands:\n"
            "  GO <direction>   or just the direction: N, S, E, W, NE, UP, etc.\n"
            "  TAKE <object>    pick something up\n"
            "  DROP <object>    put something down\n"
            "  EXAMINE <object> look closely at something\n"
            "  OPEN / CLOSE     open or close a door or container\n"
            "  READ <object>    read something\n"
            "  INVENTORY (I)    list what you're carrying\n"
            "  LOOK (L)         describe your surroundings\n"
            "  SCORE            show your current score\n"
            "  SAVE / RESTORE   save or restore the game\n"
            "  QUIT             end the game\n"
        );
        return false;
    }
    if (!std::strcmp(rawVerb, "xyzzy") || !std::strcmp(rawVerb, "plugh"))
        return actions::doXyzzy();
    return false;
}

// ---------------------------------------------------------------------------
// Per-turn room events — call all villain room-each handlers
// Mirrors MDL: after each successful command, each actor in the room acts
// ---------------------------------------------------------------------------
static void roomEachTurn() {
    RoomId here = gAdventurer.loc;
    if (gRooms[here].action)
        gRooms[here].action(here, ActionType::ROOM_EACH);
}

// ---------------------------------------------------------------------------
// Thief roaming — mirrors MDL thief behaviour (moves rooms periodically)
// ---------------------------------------------------------------------------
static void roamThief() {
    static int thiefCounter = 0;
    if (++thiefCounter < 5) return;
    thiefCounter = 0;

    // Simple roaming: move thief to an adjacent room with probability 1/3
    Object& thief = gObjects[THIEF];
    if (thief.loc < 0) return;  // thief is dead or in limbo

    // Don't move if thief is in same room as player (thief attacks separately)
    if (thief.loc == gAdventurer.loc) return;

    const Room& r = gRooms[thief.loc];
    if (!r.exits) return;
    // Count exits
    int nexits = 0;
    for (const Exit* e = r.exits; e->dir != DIR_NONE; ++e)
        if (e->dest != ROOM_NONE) ++nexits;
    if (nexits == 0) return;
    // Pick a random one
    int pick = std::rand() % nexits;
    int j = 0;
    for (const Exit* e = r.exits; e->dir != DIR_NONE; ++e) {
        if (e->dest != ROOM_NONE) {
            if (j++ == pick) {
                thief.loc = e->dest;
                // If thief arrives in player's room, announce it
                if (thief.loc == gAdventurer.loc) {
                    game::tell("A seedy-looking individual wanders in.");
                }
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Main game loop
// ---------------------------------------------------------------------------
[[noreturn]] void game::run() {
    // Show first room
    actions::doLook(true);
    gRooms[gAdventurer.loc].markSeen();

    static char inputBuf[INPUT_BUF_SIZE];
    static ParserState pstate;

    while (true) {
        // Prompt and read input
        if (!parser::readLine(inputBuf, sizeof(inputBuf))) {
            // EOF — treat as QUIT
            actions::doQuit();
        }

        // Check for bare meta-commands before full parse
        // (needed because "i" alone is inventory, etc.)
        {
            // Trim leading whitespace, lowercase first token
            const char* p = inputBuf;
            while (*p == ' ' || *p == '\t') ++p;
            char verb[32]; int vl = 0;
            while (vl < 31 && *p && *p != ' ' && *p != '\t')
                verb[vl++] = static_cast<char>(std::tolower((unsigned char)*p++));
            verb[vl] = '\0';
            if (handleMetaCommand(verb)) {
                ++gAdventurer.moves;
                roomEachTurn();
                roamThief();
                continue;
            }
        }

        // Full parse
        parser::parse(inputBuf, &pstate);

        if (!pstate.valid) {
            tell("I beg your pardon?");
            continue;
        }

        // Disambiguation
        if (pstate.nAmbiguous > 1) {
            if (!parser::disambiguate(&pstate)) continue;
        }

        ParsedCommand cmd = parser::toCommand(pstate);

        // Execute
        bool consumedMove = actions::dispatch(cmd);

        if (consumedMove) {
            ++gAdventurer.moves;
            roomEachTurn();
            roamThief();
        }
    }
}
