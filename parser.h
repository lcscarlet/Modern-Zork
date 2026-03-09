#pragma once
// ============================================================================
// parser.h — Vocabulary and command parsing
//
// The original MDL parser (np.92/93) is one of the most sophisticated parts
// of Zork — it handles full noun phrases, adjectives, "all but X" syntax,
// pronoun resolution ("it", "him"), and disambiguation.
//
// We translate its architecture faithfully:
//   1. TOKENIZE:  split raw input into Word tokens
//   2. CLASSIFY:  look each word up in vocabulary table → WordType + semantic ID
//   3. SYNTAX:    match the classified tokens against syntax patterns
//                 (from syntax.mud: SYNTAX TAKE OBJECT = V-TAKE, etc.)
//   4. RESOLVE:   turn object names into ObjIds (with disambiguation)
//   5. EMIT:      return ParsedCommand
//
// Vocabulary is a sorted static array of VocabEntry; binary search gives
// O(log N) lookup.  Total vocabulary size ~300 words × 12 bytes = ~3.6 KB.
// ============================================================================

#include "types.h"

// ---------------------------------------------------------------------------
// Word classification
// ---------------------------------------------------------------------------
enum class WordType : uint8_t {
    UNKNOWN,
    VERB,        // action word: take, put, go, ...
    DIRECTION,   // north, south, n, s, up, down, in, out, ...
    NOUN,        // object name: sword, lantern, mailbox, ...
    ADJECTIVE,   // descriptor: brass, small, large, elvish, ...
    PREPOSITION, // in, on, with, at, through, under, ...
    ARTICLE,     // the, a, an
    PRONOUN,     // it, him, her, them
    SPECIAL,     // all, all but, except, everything, ...
    CONJUNCTION, // and, then
};

struct VocabEntry {
    const char* word;   // lowercase, null-terminated
    WordType    type;
    int16_t     id;     // semantics: for VERB → ActionType, for DIRECTION → Direction,
                        // for NOUN/ADJ → ObjId hint (may be OBJ_NONE = generic),
                        // for PREP → PrepType
};

// ---------------------------------------------------------------------------
// Preposition IDs (used to determine syntax pattern)
// ---------------------------------------------------------------------------
enum PrepType : int8_t {
    PREP_NONE = 0,
    PREP_IN,    PREP_INTO,
    PREP_ON,    PREP_ONTO,
    PREP_WITH,
    PREP_AT,
    PREP_TO,
    PREP_FROM,
    PREP_UNDER,
    PREP_OVER,
    PREP_THROUGH,
    PREP_BEHIND,
    PREP_EXCEPT, // "but", "except", "but not"
};

// ---------------------------------------------------------------------------
// A tokenized word from the player's input
// ---------------------------------------------------------------------------
struct Token {
    char     word[32];  // lowercase copy of the input word
    WordType type;
    int16_t  id;
};

constexpr int MAX_TOKENS = 16;

// ---------------------------------------------------------------------------
// Input buffer (mirrors MDL INBUF)
// ---------------------------------------------------------------------------
constexpr int INPUT_BUF_SIZE = 128;

// ---------------------------------------------------------------------------
// Parser state (mirrors MDL PRSVEC)
// ---------------------------------------------------------------------------
struct ParserState {
    Token    tokens[MAX_TOKENS];
    int      ntokens;
    // Resolved parse result
    ActionType verb;
    ObjId      prso;    // direct object
    ObjId      prsi;    // indirect object
    PrepType   prep;    // preposition (if any)
    Direction  dir;     // if verb == GO
    bool       valid;
    // Disambiguation state
    ObjId   ambiguous[8];  // candidates if ambiguous noun
    int     nAmbiguous;
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
namespace parser {

// Initialize vocabulary (sort the table; call once at startup)
void init();

// Read a line of input from stdin; returns false on EOF
bool readLine(char* buf, int maxLen);

// Tokenize buf into state->tokens / state->ntokens
void tokenize(const char* buf, ParserState* state);

// Full parse: tokenize + classify + resolve → fills state; state->valid on success
void parse(const char* buf, ParserState* state);

// Turn ParserState into a ParsedCommand (convenience wrapper)
ParsedCommand toCommand(const ParserState& state);

// Handle disambiguation (call when nAmbiguous > 1):
// prints "Which do you mean…" and reads a clarification
bool disambiguate(ParserState* state);

// Expand "all" / "all but X" into a list of ObjIds accessible to the player
int expandAll(PrepType except_prep, ObjId except_id, ObjId* outBuf, int maxN);

// Remember "it" / "him" / "her" for pronoun resolution
void rememberPronoun(ObjId o);
ObjId lastPronoun();

} // namespace parser
