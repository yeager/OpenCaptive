#ifndef CAPTIVE_NAVIGATION_H
#define CAPTIVE_NAVIGATION_H

/* Native GAME SCRN coordinates for Captive's four on-screen navigation arrows.
 * These are the original 320x200 control-bank hit areas verified against the
 * DOSBox-X CAPPO runtime; callers perform window-to-canvas scaling separately.
 */
typedef enum {
    CAPTIVE_NAV_NONE = 0,
    CAPTIVE_NAV_UP,
    CAPTIVE_NAV_DOWN,
    CAPTIVE_NAV_LEFT,
    CAPTIVE_NAV_RIGHT,
} CaptiveNavigationDirection;

CaptiveNavigationDirection captive_navigation_direction_at(int x, int y);

#endif
