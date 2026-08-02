#ifndef CAPTIVE_DATA_H
#define CAPTIVE_DATA_H

#include <stdint.h>

/* Data recovered from the verified DOS executable CAPPO.EXE v1.06 (Oct 7 1992).
 * Unpacked SHA-256: fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4
 *
 * All tables below are direct transcriptions from the binary's string and data
 * sections. No values are invented or interpolated. */

/* Droid material grades, ascending quality. Indices 1-3 are blank in the
 * original executable (reserved/unused material tiers). */
#define CAPTIVE_MATERIAL_COUNT 10
extern const char *const captive_material_names[CAPTIVE_MATERIAL_COUNT];

/* Combat skills from the original skill table. */
#define CAPTIVE_SKILL_COUNT 10
extern const char *const captive_skill_names[CAPTIVE_SKILL_COUNT];

/* Shop device names (12 purchasable devices). */
#define CAPTIVE_DEVICE_COUNT 12
extern const char *const captive_device_names[CAPTIVE_DEVICE_COUNT];

/* Body part names (6 slots). */
#define CAPTIVE_BODYPART_COUNT 6
extern const char *const captive_bodypart_names[CAPTIVE_BODYPART_COUNT];

/* Name generation syllable table: 48 syllables in 4-char slots. */
#define CAPTIVE_SYLLABLE_COUNT 48
extern const char *const captive_name_syllables[CAPTIVE_SYLLABLE_COUNT];

/* Original PRNG recovered from code offset 0x44a0 in the unpacked executable.
 * multiply by 0x5e5, add 0x29, rotate right 4, XOR 0x0800. */
uint32_t captive_prng(uint32_t *state);

/* Generate a droid name from the syllable table using the original algorithm.
 * Writes a NUL-terminated string of 2-3 syllables into buf (max bufsize). */
void captive_generate_name(uint32_t *prng_state, char *buf, int bufsize);

#endif
