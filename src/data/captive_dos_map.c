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

bool captive_dos_dispatch_record_read(const uint8_t *memory, size_t memory_size,
                                      uint16_t ds_segment, size_t ordinal,
                                      CaptiveDosDispatchRecord *out) {
    if (!memory || !out || ordinal >= CAPTIVE_DOS_DISPATCH_RECORD_COUNT ||
        memory_size < DOS_MEMORY_SIZE)
        return false;
    size_t offset = (size_t)ds_segment * 16U +
                    CAPTIVE_DOS_DISPATCH_RECORD_OFFSET +
                    ordinal * CAPTIVE_DOS_DISPATCH_RECORD_SIZE;
    if (!range_inside(offset, CAPTIVE_DOS_DISPATCH_RECORD_SIZE, memory_size))
        return false;
    const uint8_t *raw = memory + offset;
    out->word_at_0 = (uint16_t)(raw[0] | ((uint16_t)raw[1] << 8));
    out->word_at_2 = (uint16_t)(raw[2] | ((uint16_t)raw[3] << 8));
    out->byte_at_4 = raw[4];
    out->byte_at_5 = raw[5];
    out->byte_at_6 = raw[6];
    out->window_index = raw[7];
    return true;
}

bool captive_dos_dispatch_window_xy(uint8_t window_index, int *x, int *y) {
    if (!x || !y || window_index >= 40U ||
        (window_index % 8U) >= 5U)
        return false;
    *x = window_index % 8U;
    *y = window_index / 8U;
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

CaptiveDosCellRoute captive_dos_cell_route_normal(uint8_t raw) {
    switch (raw & 0x7FU) {
        case 0x00: return CAPTIVE_DOS_CELL_ROUTE_1C90;
        case 0x01: case 0x02: case 0x03: case 0x04:
        case 0x05: case 0x06: case 0x07: case 0x08:
        case 0x09: case 0x0A: case 0x0B: case 0x0C:
        case 0x0D: case 0x0E: case 0x0F:
            return CAPTIVE_DOS_CELL_ROUTE_1D17;
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
            return CAPTIVE_DOS_CELL_ROUTE_1E35;
        case 0x18: return CAPTIVE_DOS_CELL_ROUTE_1DFC;
        case 0x19: return CAPTIVE_DOS_CELL_ROUTE_1E13;
        case 0x1A: return CAPTIVE_DOS_CELL_ROUTE_1DC9;
        case 0x1B: return CAPTIVE_DOS_CELL_ROUTE_1D72;
        case 0x3E: return CAPTIVE_DOS_CELL_ROUTE_1DF2;
        default: return CAPTIVE_DOS_CELL_ROUTE_NONE;
    }
}

static uint16_t route_address(CaptiveDosCellRoute route) {
    switch (route) {
        case CAPTIVE_DOS_CELL_ROUTE_1C90: return 0x1C90;
        case CAPTIVE_DOS_CELL_ROUTE_1D17: return 0x1D17;
        case CAPTIVE_DOS_CELL_ROUTE_1B06: return 0x1B06;
        case CAPTIVE_DOS_CELL_ROUTE_1DF2: return 0x1DF2;
        case CAPTIVE_DOS_CELL_ROUTE_1E35: return 0x1E35;
        case CAPTIVE_DOS_CELL_ROUTE_1DFC: return 0x1DFC;
        case CAPTIVE_DOS_CELL_ROUTE_1E13: return 0x1E13;
        case CAPTIVE_DOS_CELL_ROUTE_1DC9: return 0x1DC9;
        case CAPTIVE_DOS_CELL_ROUTE_1D72: return 0x1D72;
        case CAPTIVE_DOS_CELL_ROUTE_212F: return 0x212F;
        case CAPTIVE_DOS_CELL_ROUTE_2701: return 0x2701;
        case CAPTIVE_DOS_CELL_ROUTE_2171: return 0x2171;
        case CAPTIVE_DOS_CELL_ROUTE_21A0: return 0x21A0;
        case CAPTIVE_DOS_CELL_ROUTE_26F8: return 0x26F8;
        case CAPTIVE_DOS_CELL_ROUTE_2103: return 0x2103;
        case CAPTIVE_DOS_CELL_ROUTE_206A: return 0x206A;
        case CAPTIVE_DOS_CELL_ROUTE_20C7: return 0x20C7;
        case CAPTIVE_DOS_CELL_ROUTE_201C: return 0x201C;
        case CAPTIVE_DOS_CELL_ROUTE_218C: return 0x218C;
        case CAPTIVE_DOS_CELL_ROUTE_445A: return 0x445A;
        case CAPTIVE_DOS_CELL_ROUTE_2065: return 0x2065;
        case CAPTIVE_DOS_CELL_ROUTE_21A9: return 0x21A9;
        case CAPTIVE_DOS_CELL_ROUTE_272E: return 0x272E;
        case CAPTIVE_DOS_CELL_ROUTE_NONE: return 0;
    }
    return 0;
}

uint16_t captive_dos_cell_route_address(uint8_t raw) {
    return route_address(captive_dos_cell_route(raw));
}

uint16_t captive_dos_cell_route_normal_address(uint8_t raw) {
    return route_address(captive_dos_cell_route_normal(raw));
}
