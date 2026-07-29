#ifndef AMIGA_HUNK_H
#define AMIGA_HUNK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t hunk_count;
    size_t code_count;
    size_t data_count;
    size_t bss_count;
    size_t reloc32_count;
    size_t first_code_offset;
    size_t first_code_size;
} AmigaHunkInfo;

/* Validates an Amiga loadseg HUNK stream without executing its contents. */
bool amiga_hunk_parse(const uint8_t *data, size_t size, AmigaHunkInfo *info);

#endif
