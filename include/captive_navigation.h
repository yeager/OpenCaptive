#ifndef CAPTIVE_NAVIGATION_H
#define CAPTIVE_NAVIGATION_H

/* Native GAME SCRN coordinates for Captive's on-screen navigation controls.
 * These are the original 320x200 control-bank hit areas verified against the
 * DOSBox-X CAPPO runtime; callers perform window-to-canvas scaling separately.
 */
typedef enum {
    CAPTIVE_NAV_ACTION_NONE = 0,
    CAPTIVE_NAV_ACTION_ORBIT,
    CAPTIVE_NAV_ACTION_LAND,
    CAPTIVE_NAV_ACTION_ZOOM_OUT,
    CAPTIVE_NAV_ACTION_ZOOM_IN,
    CAPTIVE_NAV_ACTION_PYRAMID,
    CAPTIVE_NAV_ACTION_UP,
    CAPTIVE_NAV_ACTION_DOWN,
    CAPTIVE_NAV_ACTION_LEFT,
    CAPTIVE_NAV_ACTION_RIGHT,
} CaptiveNavigationAction;

typedef enum {
    CAPTIVE_NAV_NONE = 0,
    CAPTIVE_NAV_UP,
    CAPTIVE_NAV_DOWN,
    CAPTIVE_NAV_LEFT,
    CAPTIVE_NAV_RIGHT,
} CaptiveNavigationDirection;

CaptiveNavigationDirection captive_navigation_direction_at(int x, int y);
CaptiveNavigationAction captive_navigation_action_at(int x, int y);

#endif
