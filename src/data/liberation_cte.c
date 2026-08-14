#include "liberation_cte.h"
#include <string.h>

bool cte_table_parse(CteTable *table, const uint8_t *data, size_t size) {
    if (!table || !data || !size) return false;
    memset(table, 0, sizeof(*table));
    table->raw = data;
    table->raw_size = size;

    /* Walk the buffer tracking bracket depth.  A '[' at depth 0 opens a
     * section; its content runs to the matching ']'.  The section's id token
     * sits three bytes before the '[' when the middle byte is the 0x00 frame
     * separator (id, 0x00, len, '['), which is how every section after the
     * leading preamble is framed. */
    int depth = 0;
    size_t i = 0;
    while (i < size) {
        if (data[i] == '[') {
            if (depth == 0) {
                /* find matching ']' */
                int d = 1;
                size_t j = i + 1;
                while (j < size && d > 0) {
                    if (data[j] == '[') d++;
                    else if (data[j] == ']') d--;
                    if (d > 0) j++;
                }
                if (d != 0) break; /* unbalanced: stop */
                if (i >= 3 && data[i - 2] == 0x00 &&
                    table->section_count < CTE_MAX_SECTIONS) {
                    CteSection *s = &table->sections[table->section_count++];
                    s->id = data[i - 3];
                    s->content = data + i + 1;
                    s->length = j - (i + 1);
                }
                i = j + 1;
                depth = 0;
                continue;
            }
            depth++;
        } else if (data[i] == ']') {
            if (depth > 0) depth--;
        }
        i++;
    }
    return table->section_count > 0;
}

const CteSection *cte_section_find(const CteTable *table, uint16_t id) {
    if (!table) return NULL;
    for (unsigned i = 0; i < table->section_count; i++)
        if (table->sections[i].id == id)
            return &table->sections[i];
    return NULL;
}
