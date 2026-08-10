#include "captive_dos_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    uint8_t *memory = calloc(1, 0x100000u);
    assert(memory);
    const uint16_t ds = 0x2942;
    const size_t base = (size_t)ds * 16U;
    memory[base + CAPTIVE_DOS_POSITION_X_OFFSET] = 4;
    memory[base + CAPTIVE_DOS_POSITION_Y_OFFSET] = 5;
    memory[base + CAPTIVE_DOS_FACING_OFFSET] = 0x07;
    memory[base + CAPTIVE_DOS_MAP_OFFSET + 5 * 64 + 4] = 0xB6;

    CaptiveDosMapState state;
    assert(captive_dos_map_decode(memory, 0x100000u, ds, &state));
    assert(state.player_x == 4 && state.player_y == 5);
    assert(state.facing == 3);
    uint8_t raw = 0;
    assert(captive_dos_map_cell(&state, 4, 5, &raw));
    assert(raw == 0xB6);
    assert(!captive_dos_map_cell(&state, -1, 0, &raw));
    for (int y = 0; y < CAPTIVE_DOS_MAP_HEIGHT; ++y)
        for (int x = 0; x < CAPTIVE_DOS_MAP_WIDTH; ++x)
            memory[base + CAPTIVE_DOS_MAP_OFFSET + y * 64 + x] =
                (uint8_t)(y * 64 + x);
    assert(captive_dos_map_decode(memory, 0x100000u, ds, &state));
    CaptiveDosViewWindow window;
    assert(captive_dos_view_window_build(&state, &window));
    assert(window.facing == 3);
    assert(window.raw[0][0] == (uint8_t)(3 * 64 + 8));
    assert(window.raw[0][4] == (uint8_t)(7 * 64 + 8));
    assert(window.raw[4][0] == (uint8_t)(3 * 64 + 4));
    assert(window.raw[4][4] == (uint8_t)(7 * 64 + 4));

    state.player_x = 0;
    state.player_y = 0;
    assert(captive_dos_view_window_build(&state, &window));
    assert(window.outside[0][0]);
    assert(captive_dos_cell_route(0x10) == CAPTIVE_DOS_CELL_ROUTE_1E35);
    assert(captive_dos_cell_route_address(0x10) == 0x1E35);
    assert(captive_dos_cell_route(0x90) == CAPTIVE_DOS_CELL_ROUTE_1E35);
    assert(captive_dos_cell_route(0x34) == CAPTIVE_DOS_CELL_ROUTE_201C);
    assert(captive_dos_cell_route_address(0x34) == 0x201C);
    assert(captive_dos_cell_route(0x5F) == CAPTIVE_DOS_CELL_ROUTE_21A9);
    assert(captive_dos_cell_route_address(0x5F) == 0x21A9);
    assert(captive_dos_cell_route(0x3F) == CAPTIVE_DOS_CELL_ROUTE_21A9);
    assert(captive_dos_cell_route_address(0x3F) == 0x21A9);
    assert(captive_dos_cell_route_normal(0x00) == CAPTIVE_DOS_CELL_ROUTE_1C90);
    assert(captive_dos_cell_route_normal_address(0x00) == 0x1C90);
    assert(captive_dos_cell_route_normal(0x0F) == CAPTIVE_DOS_CELL_ROUTE_1D17);
    assert(captive_dos_cell_route_normal_address(0x0F) == 0x1D17);
    assert(captive_dos_cell_route_normal(0x3E) == CAPTIVE_DOS_CELL_ROUTE_1DF2);
    assert(captive_dos_cell_route_normal_address(0x3E) == 0x1DF2);
    assert(CAPTIVE_DOS_DISPATCH_RECORD_COUNT == 38);
    int x = 0, y = 0;
    assert(captive_dos_dispatch_window_xy(0, &x, &y));
    assert(x == 0 && y == 0);
    assert(captive_dos_dispatch_window_xy(12, &x, &y));
    assert(x == 4 && y == 1);
    assert(!captive_dos_dispatch_window_xy(5, &x, &y));
    assert(captive_dos_cell_route(0x00) == CAPTIVE_DOS_CELL_ROUTE_NONE);
    CaptiveDosDispatchRecord record = {
        .word_at_0 = 0x03,
        .byte_at_5 = 0x03,
        .byte_at_6 = 0x03,
    };
    memory[base + 0x5CC2] = 0x03;
    memory[base + 0x5CC3] = 0x04;
    memory[base + CAPTIVE_DOS_FACING_OFFSET] = 1;
    memory[base + 0x5CC2 + 4] = 0x03;
    memory[base + 0x5CC2 + 5] = 0x04;
    memory[base + 0x1276 + 3] = 0x4A;
    memory[base + 0x1296 + 3] = 0x15;
    memory[base + 0x144E] = 0xFF;
    memory[base + 0x144E + 2] = 0xFF;
    CaptiveDosDescriptorOperands operands;
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x03, &operands));
    assert(operands.handler_address == 0x1D17);
    assert(operands.descriptor_count == 1);
    assert(operands.descriptor_id[0] == 0x0406);
    record.byte_at_5 = 0x03;
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x10, &operands));
    assert(operands.handler_address == 0x1E35);
    assert(operands.descriptor_id[0] == 0x01DC);
    assert(operands.descriptor_id[1] == 0x01E3);
    record.byte_at_4 = 3;
    record.byte_at_5 = 0x08;
    memory[base + 0x8CFD] = 3;
    memory[base + 0x8CFE] = 0;
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x28, &operands));
    assert(operands.handler_address == 0x2103);
    assert(operands.descriptor_count == 1);
    assert(operands.descriptor_id[0] == 0x02F2);
    record.byte_at_4 = 3;
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x22, &operands));
    assert(operands.handler_address == 0x2171);
    assert(operands.descriptor_count == 2);
    assert(operands.descriptor_id[0] == 0x034C);
    assert(operands.descriptor_id[1] == 0x0355);
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x36, &operands));
    assert(operands.handler_address == 0x218C);
    assert(operands.descriptor_count == 2);
    assert(operands.descriptor_id[0] == 0x035E);
    assert(operands.descriptor_id[1] == 0x036A);
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x2C, &operands));
    assert(operands.handler_address == 0x206A);
    assert(operands.descriptor_count == 2);
    assert(operands.descriptor_id[0] == 0x006E);
    assert(operands.descriptor_id[1] == 0x0077);
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x2E, &operands));
    assert(operands.handler_address == 0x20C7);
    assert(operands.descriptor_count == 2);
    assert(operands.descriptor_id[0] == 0x0089);
    assert(operands.descriptor_id[1] == 0x0080);
    memory[0x0E3FU * 16U + 0x5E95U] = 0x34;
    memory[0x0E3FU * 16U + 0x5E96U] = 0x12;
    memory[0x0E3FU * 16U + 0x93AEU] = 0x34;
    memory[0x0E3FU * 16U + 0x93AFU] = 0x12;
    memory[0x0E3FU * 16U + 0x93B0U] = 0x02;
    record.byte_at_4 = 3;
    record.byte_at_5 = 9; /* CAPPO overlay dispatch: DL bit 3 is set. */
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x3E, &operands));
    assert(operands.handler_address == 0x2065);
    assert(operands.descriptor_count == 2);
    assert(operands.descriptor_id[0] == 0x030D);
    assert(operands.descriptor_id[1] == 0x02FB);
    memory[(size_t)ds * 16U + 0x5E8CU] = 4;
    memory[(size_t)ds * 16U + 0x5E8DU] = 0;
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x26, &operands));
    assert(operands.handler_address == 0x26F8);
    assert(operands.descriptor_count == 3);
    assert(operands.descriptor_id[0] == 0x0057);
    assert(operands.descriptor_id[1] == 0x030D);
    assert(operands.descriptor_id[2] == 0x02FB);
    memory[(size_t)ds * 16U + 0x5EF4U] = 1;
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x1C, &operands));
    assert(operands.handler_address == 0x212F);
    assert(operands.descriptor_count == 5);
    assert(operands.descriptor_id[0] == 0x003B);
    assert(operands.descriptor_id[1] == 0x0328);
    assert(operands.descriptor_id[2] == 0x0331);
    assert(operands.descriptor_id[3] == 0x033A);
    assert(operands.descriptor_id[4] == 0x0343);
    memory[(size_t)ds * 16U + CAPTIVE_DOS_FACING_OFFSET] = 0;
    record.byte_at_4 = 0x0C;
    record.byte_at_5 = 0x08;
    assert(captive_dos_dispatch_descriptor_operands(
        memory, 0x100000u, ds, &record, 0x3F, &operands));
    assert(operands.handler_address == 0x21A9);
    assert(operands.descriptor_count == 2);
    assert(operands.descriptor_id[0] == 0x02AD);
    assert(operands.descriptor_id[1] == 0x02AE);
    window.outside[0][0] = false;
    window.outside[0][1] = false;
    window.raw[0][0] = 0x1B;
    record.window_index = 0;
    record.byte_at_4 = 1;
    record.byte_at_5 = 3;
    assert(captive_dos_1c90_evaluate(
               &window, &record, 0x10) == CAPTIVE_DOS_1C90_FAIL);
    window.raw[0][1] = 0x1B;
    assert(captive_dos_1c90_evaluate(
               &window, &record, 0x10) == CAPTIVE_DOS_1C90_PASS);
    record.window_index = 4;
    assert(captive_dos_1c90_evaluate(
               &window, &record, 0x10) == CAPTIVE_DOS_1C90_UNKNOWN);
    assert(!captive_dos_map_decode(memory, 0xFFFFFu, ds, &state));
    free(memory);
    return 0;
}
