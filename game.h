#pragma once
// ============================================================================
// game.h — Game loop, output utilities, and meta-commands
//
// Mirrors MDL functions: RDCOM (main loop), TELL (output), KILLED (death),
// and the save/restore system.
// ============================================================================

#include "types.h"

namespace game {

// ---------------------------------------------------------------------------
// Output primitives — mirror MDL TELL
// ---------------------------------------------------------------------------

// Print a string followed by newline
void tell(const char* msg);

// printf-style formatted output
void tellf(const char* fmt, ...);

// Print a blank line
void cr();

// ---------------------------------------------------------------------------
// Death handling — mirrors MDL KILLED
// Called when player health hits 0; handles respawn or game-over
// ---------------------------------------------------------------------------
[[noreturn]] void killed(const char* cause);

// ---------------------------------------------------------------------------
// Save / Restore — binary snapshot of all game state
// ---------------------------------------------------------------------------
bool save(const char* filename);
bool restore(const char* filename);

// ---------------------------------------------------------------------------
// Main game loop — mirrors MDL RDCOM
// Call this from main() after world::init()
// ---------------------------------------------------------------------------
[[noreturn]] void run();

// ---------------------------------------------------------------------------
// Startup banner — mirrors MDL START / SAVE-IT intro text
// ---------------------------------------------------------------------------
void printBanner();

} // namespace game
