#include "captive_dos_map_load.h"

#include <string.h>

void captive_dos_map_to_level(DungeonLevel *out, const uint8_t *bytes) {
    if (!out || !bytes)
        return;

    memset(out, 0, sizeof(*out));

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            uint8_t cell = bytes[y * MAP_WIDTH + x];
            MapCell *mc = &out->cells[y][x];
            /* Confirmed holomap semantic: (cell & 0x7F) > 0x1A == drawn/solid. */
            mc->type = captive_dos_cell_is_wall(cell) ? CELL_WALL : CELL_FLOOR;
            /* Preserve the raw byte's marker bit in flags so later, finer
             * cell-typing RE can refine the mapping without re-capturing. */
            mc->flags = (uint8_t)(cell & 0x80u);
        }
    }
}
