#include "captive_navigation.h"

CaptiveNavigationDirection captive_navigation_direction_at(int x, int y) {
    /* CAPPO's 18x18 arrow hitboxes in the native GAME SCRN control bank. */
    if (x >= 218 && x < 236 && y >= 65 && y < 83)
        return CAPTIVE_NAV_UP;
    if (x >= 193 && x < 211 && y >= 85 && y < 103)
        return CAPTIVE_NAV_LEFT;
    if (x >= 242 && x < 260 && y >= 85 && y < 103)
        return CAPTIVE_NAV_RIGHT;
    if (x >= 218 && x < 236 && y >= 105 && y < 123)
        return CAPTIVE_NAV_DOWN;
    return CAPTIVE_NAV_NONE;
}
