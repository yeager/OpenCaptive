#include "liberation_service_mc.h"

/* FUN_0xa738 main path (no plot-flag override), reverse-engineered from the
 * CD32 game binary and cross-checked with the Ghidra decompiler:
 *   - category bit 3 set        -> -1 if bit 0 set, else -2  (no normal service)
 *   - (category & 7) == 0       -> 0
 *   - (category & 7) == 7       -> -3
 *   - (category & 7) in 1..6    -> the full category byte (commercial service)
 * The interpreter then reads mc as this value + 3.  The plot-flag override
 * branch (mc 20-23) and the per-building PRNG refinement are runtime state and
 * are intentionally not modelled here; this is the deterministic base map. */
int liberation_service_mc_from_category(uint8_t category_byte) {
    int c = (int)category_byte;
    int r;
    if (c & 8) {
        r = (c & 1) ? -1 : -2;
    } else if ((c & 7) == 0) {
        r = 0;
    } else if ((c & 7) == 7) {
        r = -3;
    } else {
        r = c;
    }
    return r + 3;
}
