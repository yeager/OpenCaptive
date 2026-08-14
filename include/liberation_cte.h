#ifndef LIBERATION_CTE_H
#define LIBERATION_CTE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Parser for the Liberation CITY_TEXT (CTE) interaction table — the original
 * shop/bank/police/clue conversation script.  Unlike the DTE "location
 * descriptions" table (ASCII `|id[...]` sections), CTE uses a binary section
 * frame: <id byte> 0x00 <len byte> [ content ], where len = content length + 3
 * and the id byte is the token that the script's ^XG/^XS jumps reference.
 *
 * This is step one of decoding the CTE: it makes the authentic sections
 * addressable.  The bytecode interpreter that expands a section's content
 * (evaluating its ^X opcodes against live game state) is built on top of this
 * and is not part of this file yet — nothing here is wired to on-screen text,
 * so no partial/invented output is shown while the interpreter is incomplete. */

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

#endif
