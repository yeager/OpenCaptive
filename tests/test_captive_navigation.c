#include "captive_navigation.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t scan_for_visual_action(CaptiveNavigationAction action) {
    switch (action) {
        case CAPTIVE_NAV_ACTION_UP: return 0x50;
        case CAPTIVE_NAV_ACTION_DOWN: return 0x48;
        case CAPTIVE_NAV_ACTION_LEFT: return 0x4D;
        case CAPTIVE_NAV_ACTION_RIGHT: return 0x4B;
        case CAPTIVE_NAV_ACTION_ORBIT: return 0x47;
        case CAPTIVE_NAV_ACTION_LAND: return 0x49;
        case CAPTIVE_NAV_ACTION_PYRAMID: return 0x1C;
        default: return 0;
    }
}

static void test_arrow_centres(void) {
    assert(captive_navigation_direction_at(226, 74) == CAPTIVE_NAV_UP);
    assert(captive_navigation_direction_at(226, 114) == CAPTIVE_NAV_DOWN);
    assert(captive_navigation_direction_at(201, 94) == CAPTIVE_NAV_LEFT);
    assert(captive_navigation_direction_at(251, 94) == CAPTIVE_NAV_RIGHT);
}

static void test_original_action_centres(void) {
    assert(captive_navigation_action_at(201, 74) == CAPTIVE_NAV_ACTION_ORBIT);
    assert(captive_navigation_action_at(251, 74) == CAPTIVE_NAV_ACTION_LAND);
    assert(captive_navigation_action_at(201, 114) == CAPTIVE_NAV_ACTION_ZOOM_OUT);
    assert(captive_navigation_action_at(251, 114) == CAPTIVE_NAV_ACTION_ZOOM_IN);
    assert(captive_navigation_action_at(226, 94) == CAPTIVE_NAV_ACTION_PYRAMID);
}

static void test_edges_and_gaps(void) {
    assert(captive_navigation_direction_at(218, 65) == CAPTIVE_NAV_UP);
    assert(captive_navigation_direction_at(235, 82) == CAPTIVE_NAV_UP);
    assert(captive_navigation_direction_at(217, 74) == CAPTIVE_NAV_NONE);
    assert(captive_navigation_direction_at(236, 74) == CAPTIVE_NAV_NONE);
    assert(captive_navigation_direction_at(226, 84) == CAPTIVE_NAV_NONE);
    assert(captive_navigation_direction_at(0, 0) == CAPTIVE_NAV_NONE);
    assert(captive_navigation_direction_at(320, 200) == CAPTIVE_NAV_NONE);
}

static void test_original_visual_scans(void) {
    /* CAPPO's original IRQ1 dispatcher: the screen arrows use the same
     * direction as their visual labels, while keypad 7/9 remain Orbit/Land. */
    assert(scan_for_visual_action(CAPTIVE_NAV_ACTION_UP) == 0x50);
    assert(scan_for_visual_action(CAPTIVE_NAV_ACTION_DOWN) == 0x48);
    assert(scan_for_visual_action(CAPTIVE_NAV_ACTION_LEFT) == 0x4D);
    assert(scan_for_visual_action(CAPTIVE_NAV_ACTION_RIGHT) == 0x4B);
    assert(scan_for_visual_action(CAPTIVE_NAV_ACTION_ORBIT) == 0x47);
    assert(scan_for_visual_action(CAPTIVE_NAV_ACTION_LAND) == 0x49);
    /* CAPPO HELP: ENTER/Pyramid moves the cursor to the ship in space. */
    assert(scan_for_visual_action(CAPTIVE_NAV_ACTION_PYRAMID) == 0x1C);
}

int main(void) {
    test_arrow_centres();
    test_original_action_centres();
    test_edges_and_gaps();
    test_original_visual_scans();
    puts("All Captive navigation tests passed.");
    return 0;
}
