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

static void test_control_bank_boundaries(void) {
    /* The live window click path uses these native rectangles before sending
     * CAPPO's original scan byte. Keep every button's exclusive edge covered
     * so adjacent controls cannot steal a click. */
    const struct {
        int x, y;
        CaptiveNavigationAction action;
    } buttons[] = {
        {193, 65, CAPTIVE_NAV_ACTION_ORBIT},
        {210, 82, CAPTIVE_NAV_ACTION_ORBIT},
        {242, 65, CAPTIVE_NAV_ACTION_LAND},
        {259, 82, CAPTIVE_NAV_ACTION_LAND},
        {193, 105, CAPTIVE_NAV_ACTION_ZOOM_OUT},
        {210, 122, CAPTIVE_NAV_ACTION_ZOOM_OUT},
        {242, 105, CAPTIVE_NAV_ACTION_ZOOM_IN},
        {259, 122, CAPTIVE_NAV_ACTION_ZOOM_IN},
        {218, 85, CAPTIVE_NAV_ACTION_PYRAMID},
        {235, 102, CAPTIVE_NAV_ACTION_PYRAMID},
    };
    for (size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i)
        assert(captive_navigation_action_at(buttons[i].x, buttons[i].y) ==
               buttons[i].action);
    assert(captive_navigation_action_at(192, 74) == CAPTIVE_NAV_ACTION_NONE);
    assert(captive_navigation_action_at(211, 74) == CAPTIVE_NAV_ACTION_NONE);
    assert(captive_navigation_action_at(241, 74) == CAPTIVE_NAV_ACTION_NONE);
    assert(captive_navigation_action_at(260, 74) == CAPTIVE_NAV_ACTION_NONE);
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

static void test_mouse_motion_quantization(void) {
    CaptiveNavigationAction actions[4];
    float remainder = 0.0f;
    assert(captive_navigation_quantize_motion(
               &remainder, 5.0f, 12.0f, CAPTIVE_NAV_ACTION_LEFT,
               CAPTIVE_NAV_ACTION_RIGHT, actions, 4) == 0);
    assert(captive_navigation_quantize_motion(
               &remainder, 7.0f, 12.0f, CAPTIVE_NAV_ACTION_LEFT,
               CAPTIVE_NAV_ACTION_RIGHT, actions, 4) == 1);
    assert(actions[0] == CAPTIVE_NAV_ACTION_RIGHT);
    assert(remainder == 0.0f);
    assert(captive_navigation_quantize_motion(
               &remainder, -25.0f, 12.0f, CAPTIVE_NAV_ACTION_LEFT,
               CAPTIVE_NAV_ACTION_RIGHT, actions, 4) == 2);
    assert(actions[0] == CAPTIVE_NAV_ACTION_LEFT);
    assert(actions[1] == CAPTIVE_NAV_ACTION_LEFT);
    assert(remainder == -1.0f);
}

int main(void) {
    test_arrow_centres();
    test_original_action_centres();
    test_edges_and_gaps();
    test_control_bank_boundaries();
    test_original_visual_scans();
    test_mouse_motion_quantization();
    puts("All Captive navigation tests passed.");
    return 0;
}
