#include "captive_data.h"
#include <string.h>

const char *const captive_material_names[CAPTIVE_MATERIAL_COUNT] = {
    "SHIT", "", "", "", "HUMAN",
    "TINDRON", "COPPATOR", "BRONZITE", "IRONIDE", "CROMIZE",
};

const char *const captive_skill_names[CAPTIVE_SKILL_COUNT] = {
    "ROBOTICS", "BRAWLING", "SWORDS", "HANDGUNS", "RIFLES",
    "AUTOMATICS", "LASERS", "CANNONS", "SPAYGUNS", "EXPERIENCE",
};

const char *const captive_device_names[CAPTIVE_DEVICE_COUNT] = {
    "AG-SCAN", "ROOT-FINDER", "MAPPER", "RADAR",
    "MAGNA-SCAN", "BODY-SCAN", "VISION-CORRECTOR", "VISOR",
    "ANTI-GRAV", "SHIELD", "FIRE-SHIELD", "GREASER",
};

const char *const captive_bodypart_names[CAPTIVE_BODYPART_COUNT] = {
    "HEAD", "CHEST", "ARM", "LEG", "FOOT", "HAND",
};

const char *const captive_name_syllables[CAPTIVE_SYLLABLE_COUNT] = {
    "VI",   "RUP",  "YUL",  "SCO",  "PHY",  "RAT",
    "QUE",  "CHA",  "SY",   "POC",  "E",    "EX",
    "DE",   "LAP",  "EL",   "MID",  "SO",   "SI",
    "LE",   "NE",   "SIC",  "THA",  "ENE",  "INS",
    "OO",   "ES",   "GIN",  "CEP",  "LTE",  "PE",
    "DER",  "S",    "DON",  "S",    "ING",  "ST",
    "Y",    "ED",   "BERY", "SY",   "LUME", "TON",
    "FAR",  "HAM",  "KAL",  "APE",  "BEE",  "INK",
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
