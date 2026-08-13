#include "captive_navigation.h"

CaptiveNavigationAction captive_navigation_action_at(int x, int y) {
    /* CAPPO's 18x18 control hitboxes in the native GAME SCRN bank. */
    if (x >= 193 && x < 211 && y >= 65 && y < 83)
        return CAPTIVE_NAV_ACTION_ORBIT;
    if (x >= 242 && x < 260 && y >= 65 && y < 83)
        return CAPTIVE_NAV_ACTION_LAND;
    if (x >= 193 && x < 211 && y >= 105 && y < 123)
        return CAPTIVE_NAV_ACTION_ZOOM_OUT;
    if (x >= 242 && x < 260 && y >= 105 && y < 123)
        return CAPTIVE_NAV_ACTION_ZOOM_IN;
    if (x >= 218 && x < 236 && y >= 85 && y < 103)
        return CAPTIVE_NAV_ACTION_PYRAMID;
    if (x >= 218 && x < 236 && y >= 65 && y < 83)
        return CAPTIVE_NAV_ACTION_UP;
    if (x >= 193 && x < 211 && y >= 85 && y < 103)
        return CAPTIVE_NAV_ACTION_LEFT;
    if (x >= 242 && x < 260 && y >= 85 && y < 103)
        return CAPTIVE_NAV_ACTION_RIGHT;
    if (x >= 218 && x < 236 && y >= 105 && y < 123)
        return CAPTIVE_NAV_ACTION_DOWN;
    return CAPTIVE_NAV_ACTION_NONE;
}

CaptiveNavigationDirection captive_navigation_direction_at(int x, int y) {
    switch (captive_navigation_action_at(x, y)) {
        case CAPTIVE_NAV_ACTION_UP:    return CAPTIVE_NAV_UP;
        case CAPTIVE_NAV_ACTION_DOWN:  return CAPTIVE_NAV_DOWN;
        case CAPTIVE_NAV_ACTION_LEFT:  return CAPTIVE_NAV_LEFT;
        case CAPTIVE_NAV_ACTION_RIGHT: return CAPTIVE_NAV_RIGHT;
        default: return CAPTIVE_NAV_NONE;
    }
}

int captive_navigation_quantize_motion(float *remainder, float delta,
                                       float threshold,
                                       CaptiveNavigationAction negative,
                                       CaptiveNavigationAction positive,
                                       CaptiveNavigationAction *actions,
                                       int action_capacity) {
    int count = 0;
    if (!remainder || !actions || action_capacity <= 0 || threshold <= 0.0f)
        return 0;
    *remainder += delta;
    while (*remainder <= -threshold) {
        if (count < action_capacity) actions[count] = negative;
        count++;
        *remainder += threshold;
    }
    while (*remainder >= threshold) {
        if (count < action_capacity) actions[count] = positive;
        count++;
        *remainder -= threshold;
    }
    return count < action_capacity ? count : action_capacity;
}
