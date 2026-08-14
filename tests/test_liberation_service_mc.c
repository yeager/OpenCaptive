/* Verifies the recovered generic-service mc-derivation (0xa738 + 3) against the
 * reverse-engineered model: real category bytes yield only the generic range
 * mc 0-9, and bank(13)/repair(16,18) are NOT producible here (they are named
 * NPCs) — the property that proved bank/repair are not building categories. */
#include "liberation_service_mc.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    /* Spot-check the decode paths. */
    assert(liberation_service_mc_from_category(0) == 3);   /* (c&7)==0 -> 0, +3 */
    assert(liberation_service_mc_from_category(7) == 0);   /* (c&7)==7 -> -3, +3 */
    assert(liberation_service_mc_from_category(1) == 4);   /* commercial: c=1, +3 */
    assert(liberation_service_mc_from_category(6) == 9);   /* commercial: c=6, +3 */
    assert(liberation_service_mc_from_category(8) == 1);   /* bit3, bit0 clear: -2+3 */
    assert(liberation_service_mc_from_category(9) == 2);   /* bit3, bit0 set:  -1+3 */

    /* Property: over all category bytes whose low nibble is a real category
     * (0-15, matching the building-record category field), mc stays in 0-9 and
     * never equals a named-NPC profession (bank=13, repair=16/18). */
    for (int c = 0; c < 16; c++) {
        int mc = liberation_service_mc_from_category((uint8_t)c);
        assert(mc >= 0 && mc <= 9);
        assert(mc != 13 && mc != 16 && mc != 18);
    }
    printf("All liberation_service_mc tests passed\n");
    return 0;
}
