#ifndef LIBERATION_TEXT_TABLE_H
#define LIBERATION_TEXT_TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define LIB_TEXT_MAX_OUTPUT 4096
#define LIB_TEXT_MAX_SECTIONS 64
#define LIB_TEXT_MAX_VARS 32

typedef struct {
    unsigned id;
    const char *start;
    size_t length;
} LibTextSection;

typedef struct {
    const uint8_t *raw;
    size_t raw_size;
    LibTextSection sections[LIB_TEXT_MAX_SECTIONS];
    unsigned section_count;
} LibTextTable;

typedef struct {
    int16_t flags[LIB_TEXT_MAX_VARS];
    int16_t vars[LIB_TEXT_MAX_VARS];
    uint32_t prng_state;
    const char *city_name;
    const char *victim_name;
    const char *person_name;
} LibTextState;

bool lib_text_table_parse(LibTextTable *table, const uint8_t *data, size_t size);
const LibTextSection *lib_text_table_find(const LibTextTable *table, unsigned id);

void lib_text_state_init(LibTextState *state, uint32_t seed);

bool lib_text_expand(const LibTextTable *table, const LibTextSection *section,
                     LibTextState *state, int case_value,
                     char *out, size_t out_size);

bool lib_text_expand_id(const LibTextTable *table, unsigned section_id,
                        LibTextState *state, int case_value,
                        char *out, size_t out_size);

#endif
