#ifndef LIBERATION_CTE_H
#define LIBERATION_CTE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Parser for the Liberation CITY_TEXT (CTE) interaction table — the original
 * shop/bank/police/clue conversation script.  Unlike the DTE "location
 * descriptions" table (ASCII `|id[...]` sections), CTE uses a binary section
 * frame: 0xD7 <id-hi> <id-lo> 0x00 <len> [ content ].  The id is a 16-bit
 * big-endian value — the decimal label (e.g. 0x4268 == 17000) that the
 * script's ^XG/^XS jumps reference — so calls resolve to other CTE sections
 * within this same table.  <len> is a one-byte hint that overflows for long
 * sections, so content is delimited by bracket matching, not by <len>.
 *
 * The section parser makes the authentic sections addressable; the read-only
 * bytecode interpreter below expands a section's content, evaluating its ^X
 * opcodes against supplied game state.  It is complete for the self-contained,
 * text-producing opcodes (^O random, ^XI conditional, ^XC case, ^^ newline,
 * ^; comment), resolves ^XS/^XG calls to other sections (with cycle
 * protection), and safely skips side-effect and state-dependent substitution
 * opcodes.  Branches gated on game flags that the caller has not set read as
 * their initial state, so a flag-heavy section expands to only its
 * unconditional text — never invented text.  The interpreter is not yet wired
 * to on-screen dialogue: correct branch selection needs the game-state field
 * model (mc, v, g, s, H, E, …), so no partial/wrong-context output is shown. */

#define CTE_MAX_SECTIONS 256

typedef struct {
    uint16_t id;            /* section token (the byte before the 0x00 frame) */
    const uint8_t *content; /* points into the source buffer */
    size_t length;          /* content length in bytes */
} CteSection;

typedef struct {
    const uint8_t *raw;
    size_t raw_size;
    CteSection sections[CTE_MAX_SECTIONS];
    unsigned section_count;
} CteTable;

/* Parse the raw CTE buffer into addressable sections.  The buffer must outlive
 * the table (sections point into it).  Returns true if at least one section was
 * found. */
bool cte_table_parse(CteTable *table, const uint8_t *data, size_t size);

/* Find a section by its id token, or NULL. */
const CteSection *cte_section_find(const CteTable *table, uint16_t id);

/* --- Bytecode interpreter (decoder step 2) ---------------------------------
 *
 * CTE section content is a bytecode script over the same `^X` language as DTE,
 * plus conditionals on game state and calls between sections.  Expanding a
 * section to text requires evaluating those conditionals, so the caller
 * supplies the script variables' current values.  The variable names are the
 * short tokens used in the script conditions (e.g. "mc", "E", "g", "H", "u").
 * Unset variables read as 0 (the initial-interaction state for most flags).
 *
 * This interpreter is deliberately read-only: it emits the authentic text for
 * a given state but performs none of the script's side effects (^X=, ^X+,
 * ^Xf..., ^FS...) — those mutate the live game and are the runtime's job. */

#define CTE_MAX_VARS 64

typedef struct {
    struct { char name[4]; int value; } vars[CTE_MAX_VARS];
    unsigned count;
    uint32_t prng_state;
} CteState;

void cte_state_init(CteState *state, uint32_t seed);
void cte_state_set(CteState *state, const char *name, int value);
int  cte_state_get(const CteState *state, const char *name);

/* Expand a section to authentic text using the supplied state.  Returns true
 * and NUL-terminates out on success. */
bool cte_expand(const CteTable *table, const CteSection *section,
                CteState *state, char *out, size_t out_size);

#endif
