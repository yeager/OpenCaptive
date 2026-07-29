#ifndef OPENCAPTIVE_H
#define OPENCAPTIVE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OPENCAPTIVE_VERSION_MAJOR 1
#define OPENCAPTIVE_VERSION_MINOR 1
#define OPENCAPTIVE_VERSION_PATCH 3

// Original Captive resolution: 320x200 (DOS/Atari ST), 320x256 (Amiga)
#define CAPTIVE_ORIGINAL_WIDTH  320
#define CAPTIVE_ORIGINAL_HEIGHT 200

// Viewport area in original game (upper portion of screen)
#define CAPTIVE_VIEWPORT_WIDTH  256
#define CAPTIVE_VIEWPORT_HEIGHT 136
#define CAPTIVE_VIEWPORT_X      32
#define CAPTIVE_VIEWPORT_Y       8

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
