#include "custom_features.h"
#include <math.h>

uint32_t lighting_shade_pixel(uint32_t color, float intensity) {
    if (isnan(intensity)) intensity = 0.0f;
    if (intensity >= 1.0f) return color;
    if (intensity <= 0.0f) return (color & 0xFF000000);

    uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * intensity);
    uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * intensity);
    uint8_t b = (uint8_t)((color & 0xFF) * intensity);
    return (color & 0xFF000000) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

float lighting_compute_intensity(int distance, int normal_dot) {
    float base = 1.0f - (float)distance * 0.15f;
    if (base < 0.15f) base = 0.15f;

    float normal_factor = 1.0f;
    float normal = (float)normal_dot;
    if (normal_dot > 0) {
        normal_factor = 0.7f + 0.3f * (normal / 127.0f);
    } else if (normal_dot < 0) {
        normal_factor = 0.7f + 0.15f * (normal / 127.0f);
    }

    float result = base * normal_factor;
    if (result > 1.0f) result = 1.0f;
    if (result < 0.1f) result = 0.1f;
    return result;
}
