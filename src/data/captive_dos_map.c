#include "captive_dos_map.h"

#include <string.h>

#define DOS_MEMORY_SIZE 0x100000u

static bool range_inside(size_t offset, size_t length, size_t size) {
    return offset <= size && length <= size - offset;
}

bool captive_dos_map_decode(const uint8_t *memory, size_t memory_size,
                            uint16_t ds_segment, CaptiveDosMapState *out) {
    if (!memory || !out || memory_size < DOS_MEMORY_SIZE) return false;

    size_t base = (size_t)ds_segment * 16U;
    size_t map_offset = base + CAPTIVE_DOS_MAP_OFFSET;
    size_t x_offset = base + CAPTIVE_DOS_POSITION_X_OFFSET;
    size_t y_offset = base + CAPTIVE_DOS_POSITION_Y_OFFSET;
    size_t facing_offset = base + CAPTIVE_DOS_FACING_OFFSET;
    if (!range_inside(map_offset, CAPTIVE_DOS_MAP_SIZE, memory_size) ||
        !range_inside(x_offset, 1, memory_size) ||
        !range_inside(y_offset, 1, memory_size) ||
        !range_inside(facing_offset, 1, memory_size))
        return false;

    memset(out, 0, sizeof(*out));
    out->ds_segment = ds_segment;
    memcpy(out->cells, memory + map_offset, CAPTIVE_DOS_MAP_SIZE);
    out->player_x = memory[x_offset];
    out->player_y = memory[y_offset];
    out->facing = (uint8_t)(memory[facing_offset] & 0x03U);
    return out->player_x < CAPTIVE_DOS_MAP_WIDTH &&
           out->player_y < CAPTIVE_DOS_MAP_HEIGHT;
}

bool captive_dos_map_cell(const CaptiveDosMapState *state, int x, int y,
                          uint8_t *out_raw) {
    if (!state || !out_raw || x < 0 || x >= CAPTIVE_DOS_MAP_WIDTH ||
        y < 0 || y >= CAPTIVE_DOS_MAP_HEIGHT)
        return false;
    *out_raw = state->cells[y * CAPTIVE_DOS_MAP_WIDTH + x];
    return true;
}
