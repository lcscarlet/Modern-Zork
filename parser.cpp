// ============================================================================
// parser.cpp — Vocabulary table and input parsing
//
// Mirrors the MDL parser (np.92/93) and syntax dispatch (syntax.mud).
// The vocabulary is a compile-time sorted table searched with bsearch().
// ============================================================================

#include "parser.h"
#include "world.h"
#include "game.h"
#include <cstring>
#include <cctype>
#include <curses.h>
#include <cstdlib>   // bsearch, qsort
#include <algorithm>

// ---------------------------------------------------------------------------
// Vocabulary table  (must be kept sorted by .word for binary search)
// ---------------------------------------------------------------------------
// Format: { "word", WordType::X, id }
//   id for VERB:      cast of ActionType to int16_t
//   id for DIRECTION: cast of Direction to int16_t
//   id for NOUN:      ObjId hint (OBJ_NONE = ambiguous/generic)
//   id for PREP:      PrepType

#define V(at)  static_cast<int16_t>(ActionType::at)
#define D(d)   static_cast<int16_t>(Direction::DIR_##d)
#define P(p)   static_cast<int16_t>(PrepType::PREP_##p)
#define N(id)  static_cast<int16_t>(ObjID::id)

static VocabEntry sVocab[] = {
    // --- Articles ---
    { "a",          WordType::ARTICLE,     0 },
    { "an",         WordType::ARTICLE,     0 },
    { "the",        WordType::ARTICLE,     0 },

    // --- Pronouns ---
    { "him",        WordType::PRONOUN,     0 },
    { "her",        WordType::PRONOUN,     0 },
    { "it",         WordType::PRONOUN,     0 },
    { "them",       WordType::PRONOUN,     0 },

    // --- Conjunctions / specials ---
    { "all",        WordType::SPECIAL,     0 },
    { "and",        WordType::CONJUNCTION, 0 },
    { "but",        WordType::PREPOSITION, P(EXCEPT) },
    { "everything", WordType::SPECIAL,     0 },
    { "except",     WordType::PREPOSITION, P(EXCEPT) },
    { "then",       WordType::CONJUNCTION, 0 },

    // --- Prepositions ---
    { "at",         WordType::PREPOSITION, P(AT)     },
    { "behind",     WordType::PREPOSITION, P(BEHIND) },
    { "from",       WordType::PREPOSITION, P(FROM)   },
    { "in",         WordType::PREPOSITION, P(IN)     },
    { "inside",     WordType::PREPOSITION, P(INTO)   },
    { "into",       WordType::PREPOSITION, P(INTO)   },
    { "on",         WordType::PREPOSITION, P(ON)     },
    { "onto",       WordType::PREPOSITION, P(ONTO)   },
    { "over",       WordType::PREPOSITION, P(OVER)   },
    { "through",    WordType::PREPOSITION, P(THROUGH)},
    { "to",         WordType::PREPOSITION, P(TO)     },
    { "under",      WordType::PREPOSITION, P(UNDER)  },
    { "with",       WordType::PREPOSITION, P(WITH)   },

    // --- Directions (also valid as verbs for "go north") ---
    { "d",          WordType::DIRECTION,   D(DOWN)  },
    { "down",       WordType::DIRECTION,   D(DOWN)  },
    { "e",          WordType::DIRECTION,   D(EAST)  },
    { "east",       WordType::DIRECTION,   D(EAST)  },
    { "in",         WordType::DIRECTION,   D(IN)    },
    { "land",       WordType::DIRECTION,   D(LAND)  },
    { "launch",     WordType::DIRECTION,   D(LAUNCH)},
    { "n",          WordType::DIRECTION,   D(NORTH) },
    { "ne",         WordType::DIRECTION,   D(NE)    },
    { "north",      WordType::DIRECTION,   D(NORTH) },
    { "northeast",  WordType::DIRECTION,   D(NE)    },
    { "northwest",  WordType::DIRECTION,   D(NW)    },
    { "nw",         WordType::DIRECTION,   D(NW)    },
    { "out",        WordType::DIRECTION,   D(OUT)   },
    { "s",          WordType::DIRECTION,   D(SOUTH) },
    { "se",         WordType::DIRECTION,   D(SE)    },
    { "south",      WordType::DIRECTION,   D(SOUTH) },
    { "southeast",  WordType::DIRECTION,   D(SE)    },
    { "southwest",  WordType::DIRECTION,   D(SW)    },
    { "sw",         WordType::DIRECTION,   D(SW)    },
    { "u",          WordType::DIRECTION,   D(UP)    },
    { "up",         WordType::DIRECTION,   D(UP)    },
    { "w",          WordType::DIRECTION,   D(WEST)  },
    { "west",       WordType::DIRECTION,   D(WEST)  },

    // --- Verbs ---
    { "attack",     WordType::VERB, V(ATTACK)     },
    { "blow",       WordType::VERB, V(INFLATE)    },
    { "board",      WordType::VERB, V(BOARD)      },
    { "break",      WordType::VERB, V(MUNG)       },
    { "burn",       WordType::VERB, V(BURN)       },
    { "carry",      WordType::VERB, V(TAKE)       },
    { "climb",      WordType::VERB, V(CLIMB)      },
    { "close",      WordType::VERB, V(CLOSE)      },
    { "deflate",    WordType::VERB, V(DEFLATE)    },
    { "destroy",    WordType::VERB, V(MUNG)       },
    { "dig",        WordType::VERB, V(PUSH)       },
    { "disembark",  WordType::VERB, V(DISEMBARK)  },
    { "drink",      WordType::VERB, V(DRINK)      },
    { "drop",       WordType::VERB, V(DROP)       },
    { "eat",        WordType::VERB, V(EAT)        },
    { "enter",      WordType::VERB, V(ENTER)      },
    { "examine",    WordType::VERB, V(EXAMINE)    },
    { "exit",       WordType::VERB, V(EXIT)       },
    { "extinguish", WordType::VERB, V(EXTINGUISH) },
    { "get",        WordType::VERB, V(TAKE)       },
    { "give",       WordType::VERB, V(GIVE)       },
    { "go",         WordType::VERB, V(WALK)       },
    { "grab",       WordType::VERB, V(TAKE)       },
    { "hello",      WordType::VERB, V(INCANT)     },
    { "i",          WordType::VERB, V(TAKE)       }, // "i" = inventory in some parsers
    { "inflate",    WordType::VERB, V(INFLATE)    },
    { "insert",     WordType::VERB, V(PUT)        },
    { "kill",       WordType::VERB, V(ATTACK)     },
    { "l",          WordType::VERB, V(LOOK)       },
    { "light",      WordType::VERB, V(LIGHT)      },
    { "listen",     WordType::VERB, V(LISTEN)     },
    { "lock",       WordType::VERB, V(LOCK)       },
    { "look",       WordType::VERB, V(LOOK)       },
    { "move",       WordType::VERB, V(MOVE)       },
    { "open",       WordType::VERB, V(OPEN)       },
    { "pick",       WordType::VERB, V(TAKE)       },   // "pick up"
    { "pull",       WordType::VERB, V(PULL)       },
    { "push",       WordType::VERB, V(PUSH)       },
    { "put",        WordType::VERB, V(PUT)        },
    { "quit",       WordType::VERB, V(INCANT)     },
    { "read",       WordType::VERB, V(READ)       },
    { "remove",     WordType::VERB, V(REMOVE)     },
    { "restore",    WordType::VERB, V(INCANT)     },
    { "rub",        WordType::VERB, V(TOUCH)      },
    { "run",        WordType::VERB, V(WALK)       },
    { "save",       WordType::VERB, V(INCANT)     },
    { "score",      WordType::VERB, V(INCANT)     },
    { "search",     WordType::VERB, V(EXAMINE)    },
    { "shake",      WordType::VERB, V(MOVE)       },
    { "slam",       WordType::VERB, V(CLOSE)      },
    { "slash",      WordType::VERB, V(ATTACK)     },
    { "smell",      WordType::VERB, V(SMELL)      },
    { "stab",       WordType::VERB, V(ATTACK)     },
    { "strike",     WordType::VERB, V(ATTACK)     },
    { "take",       WordType::VERB, V(TAKE)       },
    { "throw",      WordType::VERB, V(THROW)      },
    { "tie",        WordType::VERB, V(TIE)        },
    { "touch",      WordType::VERB, V(TOUCH)      },
    { "turn",       WordType::VERB, V(TURN)       },
    { "unlock",     WordType::VERB, V(UNLOCK)     },
    { "untie",      WordType::VERB, V(UNTIE)      },
    { "verbose",    WordType::VERB, V(INCANT)     },
    { "walk",       WordType::VERB, V(WALK)       },
    { "wear",       WordType::VERB, V(WEAR)       },
    { "x",          WordType::VERB, V(EXAMINE)    },

    // --- Nouns (key items; others resolve by substring match in world::findObject) ---
    { "axe",        WordType::NOUN, N(AXE)    },
    { "bottle",     WordType::NOUN, N(BOTTL)  },
    { "candles",    WordType::NOUN, N(CANDL)  },
    { "case",       WordType::NOUN, N(TCASE)  },
    { "coffin",     WordType::NOUN, N(COFFIN) },
    { "cyclops",    WordType::NOUN, N(CYCLOPS)},
    { "door",       WordType::NOUN, N(FDOOR)  },
    { "eggs",       WordType::NOUN, N(EGGS)   },
    { "key",        WordType::NOUN, N(LANTER) },
    { "keys",       WordType::NOUN, N(KEYS)   },
    { "knife",      WordType::NOUN, N(KNIFE)  },
    { "lamp",       WordType::NOUN, N(LAMP)   },
    { "lantern",    WordType::NOUN, N(LAMP)   },
    { "leaflet",    WordType::NOUN, N(LEAFL)  },
    { "mailbox",    WordType::NOUN, N(MBOX)   },
    { "matches",    WordType::NOUN, N(MATCH)  },
    { "rope",       WordType::NOUN, N(ROPE)   },
    { "rug",        WordType::NOUN, N(RUG)    },
    { "sack",       WordType::NOUN, N(SACK)   },
    { "sword",      WordType::NOUN, N(SWORD)  },
    { "thief",      WordType::NOUN, N(THIEF)  },
    { "torch",      WordType::NOUN, N(TORCH)  },
    { "trapdoor",   WordType::NOUN, N(TRAPDOOR) },
    { "troll",      WordType::NOUN, N(TROLL)  },
    { "water",      WordType::NOUN, N(WATER)  },
    { "window",     WordType::NOUN, N(WNDOW)  },

    // --- Adjectives ---
    { "bloody",     WordType::ADJECTIVE, 0 },
    { "brass",      WordType::ADJECTIVE, 0 },
    { "bronze",     WordType::ADJECTIVE, 0 },
    { "brown",      WordType::ADJECTIVE, 0 },
    { "crystal",    WordType::ADJECTIVE, 0 },
    { "elvish",     WordType::ADJECTIVE, 0 },
    { "glass",      WordType::ADJECTIVE, 0 },
    { "golden",     WordType::ADJECTIVE, 0 },
    { "ivory",      WordType::ADJECTIVE, 0 },
    { "jeweled",    WordType::ADJECTIVE, 0 },
    { "large",      WordType::ADJECTIVE, 0 },
    { "leather",    WordType::ADJECTIVE, 0 },
    { "nasty",      WordType::ADJECTIVE, 0 },
    { "oriental",   WordType::ADJECTIVE, 0 },
    { "rusty",      WordType::ADJECTIVE, 0 },
    { "seedy",      WordType::ADJECTIVE, 0 },
    { "silver",     WordType::ADJECTIVE, 0 },
    { "skeleton",   WordType::ADJECTIVE, 0 },
    { "small",      WordType::ADJECTIVE, 0 },
    { "trap",       WordType::ADJECTIVE, 0 },  // "trap door"
    { "two",        WordType::ADJECTIVE, 0 },
};

constexpr int VOCAB_SIZE = static_cast<int>(sizeof(sVocab) / sizeof(sVocab[0]));

// Pronoun memory
static ObjId sLastIt = OBJ_NONE;

// ---------------------------------------------------------------------------
// Comparison for bsearch / qsort
// ---------------------------------------------------------------------------
static int vocabCmp(const void* a, const void* b) {
    return std::strcmp(
        static_cast<const VocabEntry*>(a)->word,
        static_cast<const VocabEntry*>(b)->word
    );
}

// ---------------------------------------------------------------------------
// parser::init — sort vocabulary table
// ---------------------------------------------------------------------------
void parser::init() {
    std::qsort(sVocab, VOCAB_SIZE, sizeof(VocabEntry), vocabCmp);
}

// ---------------------------------------------------------------------------
// parser::readLine
// ---------------------------------------------------------------------------
bool parser::readLine(char* buf, int maxLen) {
    printw("> ");
    refresh();
    getstr(buf);
    // Strip trailing newline if any
    char* nl = std::strchr(buf, '\n');
    if (nl) *nl = '\0';
    return true; // ncurses getstr doesn't return false on EOF easily
}

// ---------------------------------------------------------------------------
// parser::tokenize — split input, convert to lowercase, look up vocabulary
// ---------------------------------------------------------------------------
void parser::tokenize(const char* buf, ParserState* state) {
    state->ntokens = 0;
    const char* p = buf;
    while (*p && state->ntokens < MAX_TOKENS) {
        // Skip whitespace and punctuation
        while (*p && (std::isspace((unsigned char)*p) || *p == ',' || *p == '.'))
            ++p;
        if (!*p) break;

        // Gather a word
        Token& tok = state->tokens[state->ntokens];
        int wlen = 0;
        while (*p && !std::isspace((unsigned char)*p) && *p != ',' && *p != '.') {
            if (wlen < 31)
                tok.word[wlen++] = static_cast<char>(std::tolower((unsigned char)*p));
            ++p;
        }
        tok.word[wlen] = '\0';
        if (wlen == 0) continue;

        // Look up in vocabulary
        VocabEntry key; key.word = tok.word;
        const VocabEntry* entry = static_cast<const VocabEntry*>(
            std::bsearch(&key, sVocab, VOCAB_SIZE, sizeof(VocabEntry), vocabCmp)
        );
        if (entry) {
            tok.type = entry->type;
            tok.id   = entry->id;
        } else {
            tok.type = WordType::UNKNOWN;
            tok.id   = OBJ_NONE;
        }
        ++state->ntokens;
    }
}

// ---------------------------------------------------------------------------
// Resolve a noun token to an ObjId (with accessibility check)
// ---------------------------------------------------------------------------
static ObjId resolveNoun(const Token& tok, ParserState* state) {
    // If the vocabulary gave us a specific ObjId hint, verify it's accessible
    if (tok.id != OBJ_NONE) {
        if (world::isAccessible(static_cast<ObjId>(tok.id)))
            return static_cast<ObjId>(tok.id);
    }

    // Fallback: substring search through accessible objects
    // Build candidate list
    state->nAmbiguous = 0;
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        ObjId id = static_cast<ObjId>(i);
        if (!world::isAccessible(id)) continue;
        if (std::strstr(gObjects[i].desc, tok.word) != nullptr) {
            if (state->nAmbiguous < 8)
                state->ambiguous[state->nAmbiguous++] = id;
        }
    }
    if (state->nAmbiguous == 1)  return state->ambiguous[0];
    if (state->nAmbiguous == 0)  return OBJ_NONE;
    return OBJ_NONE;  // ambiguous — caller must handle
}

// ---------------------------------------------------------------------------
// parser::parse — full parse pass
// ---------------------------------------------------------------------------
void parser::parse(const char* buf, ParserState* state) {
    tokenize(buf, state);
    state->verb   = ActionType::LOOK;  // default if empty
    state->prso   = OBJ_NONE;
    state->prsi   = OBJ_NONE;
    state->prep   = PrepType::PREP_NONE;
    state->dir    = DIR_NONE;
    state->valid  = false;
    state->nAmbiguous = 0;

    if (state->ntokens == 0) {
        state->valid = false;
        return;
    }

    int i = 0;
    const Token* toks = state->tokens;
    const int   n    = state->ntokens;

    // Skip leading article/conjunction
    while (i < n && (toks[i].type == WordType::ARTICLE ||
                     toks[i].type == WordType::CONJUNCTION)) ++i;

    if (i >= n) { state->valid = false; return; }

    // --- DIRECTION as bare word (e.g. "north", "n", "up") ---
    if (toks[i].type == WordType::DIRECTION) {
        state->verb  = ActionType::WALK;
        state->dir   = static_cast<Direction>(toks[i].id);
        state->valid = true;
        return;
    }

    // --- Must be a VERB ---
    if (toks[i].type != WordType::VERB) {
        // Unknown word at start
        game::tellf("I don't know the word \"%s\".", toks[i].word);
        return;
    }
    state->verb = static_cast<ActionType>(toks[i].id);
    ++i;

    // Handle special single-word commands
    if (state->verb == ActionType::INCANT) {
        // save/restore/quit/score/verbose — passed through directly
        state->valid = true;
        return;
    }

    // Skip optional "up" after "pick" (pick up X)
    if (state->verb == ActionType::TAKE && i < n &&
        toks[i].type == WordType::DIRECTION &&
        static_cast<Direction>(toks[i].id) == DIR_UP) ++i;

    // Look for a direct object
    while (i < n && toks[i].type == WordType::ARTICLE) ++i;

    // "go <direction>"
    if (state->verb == ActionType::WALK && i < n &&
        toks[i].type == WordType::DIRECTION) {
        state->dir   = static_cast<Direction>(toks[i].id);
        state->valid = true;
        return;
    }

    // "all" / "everything"
    if (i < n && toks[i].type == WordType::SPECIAL) {
        state->prso  = OBJ_NONE;   // caller expands "all"
        state->valid = true;
        ++i;
        // Check for "all but X"
        if (i < n && toks[i].type == WordType::PREPOSITION &&
            toks[i].id == PrepType::PREP_EXCEPT) {
            ++i;
            while (i < n && toks[i].type == WordType::ARTICLE) ++i;
            if (i < n)
                state->prsi = resolveNoun(toks[i++], state);
        }
        return;
    }

    // Pronoun
    if (i < n && toks[i].type == WordType::PRONOUN) {
        state->prso = sLastIt;
        ++i;
    } else if (i < n && (toks[i].type == WordType::NOUN ||
                          toks[i].type == WordType::ADJECTIVE ||
                          toks[i].type == WordType::UNKNOWN)) {
        // Possible adjective(s) followed by noun
        while (i < n && toks[i].type == WordType::ADJECTIVE) ++i;
        if (i < n && (toks[i].type == WordType::NOUN ||
                       toks[i].type == WordType::UNKNOWN)) {
            state->prso = resolveNoun(toks[i++], state);
        }
    }

    // Look for preposition + indirect object
    while (i < n && toks[i].type == WordType::ARTICLE) ++i;
    if (i < n && toks[i].type == WordType::PREPOSITION) {
        state->prep = static_cast<PrepType>(toks[i].id);
        ++i;
        while (i < n && toks[i].type == WordType::ARTICLE) ++i;
        if (i < n && (toks[i].type == WordType::NOUN ||
                       toks[i].type == WordType::UNKNOWN)) {
            state->prsi = resolveNoun(toks[i++], state);
        }
    }

    state->valid = true;
}

// ---------------------------------------------------------------------------
// parser::toCommand
// ---------------------------------------------------------------------------
ParsedCommand parser::toCommand(const ParserState& s) {
    ParsedCommand cmd;
    cmd.verb  = s.verb;
    cmd.prso  = s.prso;
    cmd.prsi  = s.prsi;
    cmd.dir   = s.dir;
    cmd.valid = s.valid;
    return cmd;
}

// ---------------------------------------------------------------------------
// parser::disambiguate
// ---------------------------------------------------------------------------
bool parser::disambiguate(ParserState* state) {
    if (state->nAmbiguous <= 1) return true;
    game::tell("Which do you mean:");
    for (int i = 0; i < state->nAmbiguous; ++i)
        game::tellf("  (%d) The %s", i+1, obj(state->ambiguous[i]).desc);
    char buf[32];
    if (!readLine(buf, sizeof(buf))) return false;
    int choice = std::atoi(buf);
    if (choice < 1 || choice > state->nAmbiguous) {
        game::tell("That isn't one of the choices.");
        return false;
    }
    state->prso = state->ambiguous[choice - 1];
    state->nAmbiguous = 0;
    return true;
}

// ---------------------------------------------------------------------------
// parser::expandAll
// ---------------------------------------------------------------------------
int parser::expandAll(PrepType except_prep, ObjId except_id, ObjId* outBuf, int maxN) {
    (void)except_prep;
    int n = 0;
    for (int i = 0; i < NUM_OBJECTS && n < maxN; ++i) {
        ObjId id = static_cast<ObjId>(i);
        if (i == except_id) continue;
        if (!gObjects[i].hasFlag(VISBIT)) continue;
        if (gObjects[i].hasFlag(SACREDBIT)) continue;
        if (world::isAccessible(id))
            outBuf[n++] = id;
    }
    return n;
}

// ---------------------------------------------------------------------------
// parser::rememberPronoun / lastPronoun
// ---------------------------------------------------------------------------
void parser::rememberPronoun(ObjId o) { sLastIt = o; }
ObjId parser::lastPronoun()           { return sLastIt; }
