#include "captive_navigation.h"

#include <assert.h>
#include <stdio.h>

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

int main(void) {
    test_arrow_centres();
    test_original_action_centres();
    test_edges_and_gaps();
    puts("All Captive navigation tests passed.");
    return 0;
}
