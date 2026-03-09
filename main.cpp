// ============================================================================
// main.cpp — Entry point
// ============================================================================

#include "world.h"
#include "parser.h"
#include "melee.h"
#include "game.h"
#include <curses.h>

int main() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Initialize all subsystems
    world::init();    // set up world state, fix object flags
    parser::init();   // sort vocabulary table
    melee::init();    // seed RNG

    // Print intro banner
    game::printBanner();

    // Hand off to main game loop (never returns)
    game::run();

    // Cleanup (though run() never returns)
    endwin();
}
