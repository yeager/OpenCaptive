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

bool captive_dos_view_window_build(const CaptiveDosMapState *state,
                                   CaptiveDosViewWindow *out) {
    if (!state || !out || state->facing > 3) return false;
    memset(out, 0, sizeof(*out));
    out->facing = state->facing;

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            int x = state->player_x;
            int y = state->player_y;
            /* The four branches in CAPPO 0x1818 write the same 5x5
             * neighbourhood in different traversal orders. */
            switch (state->facing) {
                case 0:
                    x -= 2 + row;
                    y -= 4 - col;
                    break;
                case 1:
                    x -= 4 - row;
                    y += 2 - col;
                    break;
                case 2:
                    x += 2 - row;
                    y += 4 - col;
                    break;
                case 3:
                    x += 4 - row;
                    y -= 2 - col;
                    break;
                default:
                    return false;
            }
            if (x < 0 || x >= CAPTIVE_DOS_MAP_WIDTH ||
                y < 0 || y >= CAPTIVE_DOS_MAP_HEIGHT) {
                out->outside[row][col] = true;
                /* Keep an outside cell distinguishable from a real 0x00
                 * CAPPO cell.  No guessed map value is emitted. */
                continue;
            }
            out->raw[row][col] = state->cells[y * CAPTIVE_DOS_MAP_WIDTH + x];
        }
    }
    return true;
}

CaptiveDosCellRoute captive_dos_cell_route(uint8_t raw) {
    switch (raw & 0x7FU) {
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
            return CAPTIVE_DOS_CELL_ROUTE_1E35;
        case 0x18: return CAPTIVE_DOS_CELL_ROUTE_1DFC;
        case 0x19: return CAPTIVE_DOS_CELL_ROUTE_1E13;
        case 0x1A: return CAPTIVE_DOS_CELL_ROUTE_1DC9;
        case 0x1B: return CAPTIVE_DOS_CELL_ROUTE_1D72;
        case 0x1C: return CAPTIVE_DOS_CELL_ROUTE_212F;
        case 0x1E: return CAPTIVE_DOS_CELL_ROUTE_2701;
        case 0x22: return CAPTIVE_DOS_CELL_ROUTE_2171;
        case 0x24: return CAPTIVE_DOS_CELL_ROUTE_21A0;
        case 0x26: return CAPTIVE_DOS_CELL_ROUTE_26F8;
        case 0x28: return CAPTIVE_DOS_CELL_ROUTE_2103;
        case 0x2C: return CAPTIVE_DOS_CELL_ROUTE_206A;
        case 0x2E: return CAPTIVE_DOS_CELL_ROUTE_20C7;
        case 0x2F: case 0x30: case 0x31: case 0x32:
        case 0x33: case 0x34:
            return CAPTIVE_DOS_CELL_ROUTE_201C;
        case 0x36: return CAPTIVE_DOS_CELL_ROUTE_218C;
        case 0x37: case 0x38: case 0x39: case 0x3A:
            return CAPTIVE_DOS_CELL_ROUTE_445A;
        case 0x3C: case 0x3E:
            return CAPTIVE_DOS_CELL_ROUTE_2065;
        case 0x40: case 0x41: case 0x42: case 0x43:
        case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4A: case 0x4B:
        case 0x4C: case 0x4D: case 0x4E: case 0x4F:
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5A: case 0x5B:
        case 0x5C: case 0x5D: case 0x5E: case 0x5F:
            return CAPTIVE_DOS_CELL_ROUTE_21A9;
        case 0x60: return CAPTIVE_DOS_CELL_ROUTE_272E;
        default: return CAPTIVE_DOS_CELL_ROUTE_NONE;
    }
}
