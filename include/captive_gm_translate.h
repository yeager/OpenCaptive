#ifndef CAPTIVE_GM_TRANSLATE_H
#define CAPTIVE_GM_TRANSLATE_H

#include <stdint.h>

/*
 * Captive DOS level generation: GM.EXE, and its cell-code translator.
 *
 * MAJOR RE RESULT (see docs/CAPTIVE_DOS_DUNGEON_RE.md, "GM.EXE" section):
 * CAPPO.EXE is only the front-end/menu.  The actual dungeon LEVEL is generated
 * by a child program, GM.EXE, which CAPPO launches via INT 21h AH=4Bh (EXEC)
 * from its level-setup path (0xC9D9 -> 0x0DBC).  GM.EXE receives the mission
 * parameters at fixed low memory (0000:04F4..04FA), generates a 64x32 base map
 * in its own work segment, then copies the result back into CAPPO's DGROUP.
 * This is why CAPPO alone never produced a level: with EXEC stubbed, GM.EXE
 * never ran.  GM.EXE is real game data (ships in the Captive DOS tree); nothing
 * here is synthetic.
 *
 * GM.EXE's generation pipeline (GM_UNP.EXE offsets, verified by emulation):
 *   - 0x2F2  seed/init from the mission parameter,
 *   - 0xEE   the 2048-cell generate+translate loop, and
 *   - 0x129  the per-cell CODE TRANSLATOR transcribed below.
 * The translator maps GM's internal/source cell codes (as produced by the
 * core placement machine at 0x1F00 and read by 0xEE) to the final display/tile
 * codes written into the map.  This function is a byte-for-byte transcription
 * of GM_UNP.EXE 0x129..0x2F1 -- every constant and branch is copied from the
 * original code, not invented.
 *
 * Inputs mirror the original registers at the call site (0xEE, 0x103):
 *   src  = dl : the source cell code (bit 7 is a marker preserved into bit 7
 *              of the result via the 0x80 flag at DS:0x351C).
 *   aux  = dh : an auxiliary byte used by the door (0x22), 0x2C and 0x15 cases.
 *   ax        : a 16-bit selector; the values 0x0000, 0xFFFF and 0xFFFD force
 *              an empty (0) result for most codes (GM 0x169/0x17D/0x185).
 */
uint8_t captive_gm_translate_cell(uint8_t src, uint8_t aux, uint16_t ax);

/*
 * Wall test for GM.EXE *output* (display) codes, matching CAPPO's holomap
 * semantics on the copied-back map: a cell is solid/drawn when (code & 0x7F)
 * > 0x1A.  0xA0 (the translation of source 0x20) is thus a wall; 0x18/0x00 are
 * open.  Provided so callers can classify a GM-translated map without repeating
 * the mask.
 */
static inline int captive_gm_cell_is_wall(uint8_t code) {
    return (uint8_t)(code & 0x7Fu) > 0x1Au;
}

#endif /* CAPTIVE_GM_TRANSLATE_H */
