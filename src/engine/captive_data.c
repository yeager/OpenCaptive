#include "captive_data.h"
#include <string.h>

const char *const captive_material_names[CAPTIVE_MATERIAL_COUNT] = {
    "SHIT", "TIN", "BRASS", "IRON", "STEEL",
    "CHROME", "SILVER", "GOLD", "PLATINUM", "TITANIUX",
};

const char *const captive_skill_names[CAPTIVE_SKILL_COUNT] = {
    "BRAWLING", "CLOSE COMBAT", "PROJECTILE", "FIREARMS",
    "LIGHT ARMS", "HEAVY ARMS", "AUTO ARMS", "HEAVY AUTO",
    "MECH. WEAPON", "ENERGY WEAPON",
};

const char *const captive_device_names[CAPTIVE_DEVICE_COUNT] = {
    "HEAD", "TORSO", "LEFT ARM", "RIGHT ARM",
    "LEFT HAND", "RIGHT HAND", "LEFT LEG", "RIGHT LEG",
    "LEFT FOOT", "RIGHT FOOT", "BACKPACK", "POWER UNIT",
};

const char *const captive_name_syllables[CAPTIVE_SYLLABLE_COUNT] = {
    "BA", "BE", "BI", "BO", "BU", "BY",
    "DA", "DE", "DI", "DO", "DU", "DY",
    "KA", "KE", "KI", "KO", "KU", "KY",
    "LA", "LE", "LI", "LO", "LU", "LY",
    "MA", "ME", "MI", "MO", "MU", "MY",
    "RA", "RE", "RI", "RO", "RU", "RY",
    "SA", "SE", "SI", "SO", "SU", "SY",
    "TA", "TE", "TI", "TO", "TU", "TY",
};

uint32_t captive_prng(uint32_t *state) {
    uint32_t s = *state;
    s = s * 0x5e5 + 0x29;
    s = (s >> 4) | (s << 28);
    s ^= 0x0800;
    *state = s;
    return s;
}

void captive_generate_name(uint32_t *prng_state, char *buf, int bufsize) {
    if (!buf || bufsize < 1) return;
    buf[0] = '\0';

    int syllable_count = 2 + (captive_prng(prng_state) % 2);
    int pos = 0;

    for (int i = 0; i < syllable_count; i++) {
        int idx = captive_prng(prng_state) % CAPTIVE_SYLLABLE_COUNT;
        const char *syl = captive_name_syllables[idx];
        int len = (int)strlen(syl);
        if (pos + len >= bufsize - 1) break;
        memcpy(buf + pos, syl, len);
        pos += len;
    }
    buf[pos] = '\0';
}
