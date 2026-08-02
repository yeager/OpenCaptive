#ifndef CAPTIVE_DATA_H
#define CAPTIVE_DATA_H

#include <stdint.h>

/* Data recovered from the verified DOS executable CAPPO.EXE v1.06 (Oct 7 1992).
 * Unpacked SHA-256: fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4
 *
 * All tables below are direct transcriptions from the binary's string and data
 * sections. No values are invented or interpolated. */

/* Droid material grades, ascending quality. The original spells these
 * exactly as shown (CROMIZE not Chrome, TITANIUX not Titanium). */
#define CAPTIVE_MATERIAL_COUNT 10
extern const char *const captive_material_names[CAPTIVE_MATERIAL_COUNT];

/* Combat skills. Each droid trains these independently, 1-8 points per tier. */
#define CAPTIVE_SKILL_COUNT 10
extern const char *const captive_skill_names[CAPTIVE_SKILL_COUNT];

/* Droid body part / device slot names (12 categories). */
#define CAPTIVE_DEVICE_COUNT 12
extern const char *const captive_device_names[CAPTIVE_DEVICE_COUNT];

/* Name generation syllable table: 8 consonant groups x 6 vowels = 48. */
#define CAPTIVE_SYLLABLE_COUNT 48
extern const char *const captive_name_syllables[CAPTIVE_SYLLABLE_COUNT];

/* Original PRNG recovered from code offset 0x44a0 in the unpacked executable.
 * multiply by 0x5e5, add 0x29, rotate right 4, XOR 0x0800. */
uint32_t captive_prng(uint32_t *state);

/* Generate a droid name from the syllable table using the original algorithm.
 * Writes a NUL-terminated string of 2-3 syllables into buf (max bufsize). */
void captive_generate_name(uint32_t *prng_state, char *buf, int bufsize);

#endif
