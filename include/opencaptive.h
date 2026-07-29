#ifndef OPENCAPTIVE_H
#define OPENCAPTIVE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OPENCAPTIVE_VERSION_MAJOR 1
#define OPENCAPTIVE_VERSION_MINOR 1
#define OPENCAPTIVE_VERSION_PATCH 11

// Original Captive resolution: 320x200 (DOS/Atari ST), 320x256 (Amiga)
#define CAPTIVE_ORIGINAL_WIDTH  320
#define CAPTIVE_ORIGINAL_HEIGHT 200

/* Liberation's CD32 presentation is a PAL 320x256 screen. */
#define LIBERATION_SCREEN_WIDTH  320
#define LIBERATION_SCREEN_HEIGHT 256

/* Captive's original GAME SCRN shell reserves a 144x112 dungeon window at
 * these coordinates.  The earlier 256x136 rectangle covered the status
 * monitors, controls and message panel in the hash-verified source frame. */
#define CAPTIVE_VIEWPORT_WIDTH  144
#define CAPTIVE_VIEWPORT_HEIGHT 112
#define CAPTIVE_VIEWPORT_X      32
#define CAPTIVE_VIEWPORT_Y      55

typedef enum {
    CAPTIVE_PLATFORM_DOS,
    CAPTIVE_PLATFORM_ATARI_ST,
    CAPTIVE_PLATFORM_AMIGA,
} CaptivePlatform;

typedef enum {
    CAPTIVE_RENDER_ORIGINAL,
    CAPTIVE_RENDER_ENHANCED,
} CaptiveRenderMode;

typedef struct {
    CaptivePlatform platform;
    CaptiveRenderMode render_mode;
    const char *data_path;
    int scale_factor;
    bool fullscreen;
    bool vsync;
    bool scanlines;
    bool crt_curvature;
    bool bilinear;
    bool integer_scaling;
    int fps_limit;       // 0 = unlimited, 30, 60, 120
    int brightness;      // 0-100
    int contrast;        // 0-100
} OpenCaptiveConfig;

#endif
