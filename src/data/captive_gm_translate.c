#include "captive_gm_translate.h"

/*
 * Byte-for-byte transcription of GM_UNP.EXE 0x129..0x2F1 (Captive DOS level
 * generator's per-cell code translator).  The original is a flat compare chain
 * where `bl` holds the candidate output, is (re)loaded by each `mov bl,imm`,
 * and is returned (with the 0x80 marker OR'd in at 0x2ED) as soon as a `cmp
 * dl,code / je` matches.  The ordering is preserved exactly because `bl` carries
 * across blocks: the ax-based early exits (0x169/0x17D/0x185) return whatever bl
 * was last set to (0), and the 0x2C/0x15/0x22 cases derive bl from dh.
 */
uint8_t captive_gm_translate_cell(uint8_t src, uint8_t aux, uint16_t ax) {
    uint8_t dl = src;
    uint8_t dh = aux;
    uint8_t flag = 0u;
    uint8_t bl;

    /* 0x129/0x12E: the 0x20 case returns 0xA0 *before* the marker split, so no
     * 0x80 flag is ever OR'd for it. */
    if (dl == 0x20u)
        return 0xA0u;

    /* 0x136: high bit is a marker; strip it and remember to OR it back at end. */
    if (dl & 0x80u) {
        flag = 0x80u;
        dl = (uint8_t)(dl & 0x7Fu);
    }

    bl = 0x18u; if (dl == 0x1Eu) return (uint8_t)(bl | flag);            /* 0x142 */
    bl = 0x28u; if (dl == 0x09u) return (uint8_t)(bl | flag);            /* 0x14C */
    if (dl == 0x22u) {                                                   /* 0x156 */
        bl = (uint8_t)(0x10u + ((dh >> 1) & 3u));
        return (uint8_t)(bl | flag);
    }
    bl = 0x00u; if (ax == 0xFFFFu) return (uint8_t)(bl | flag);          /* 0x167/0x169 */
    if (dl == 0x2Cu) { bl = dh; return (uint8_t)(bl | flag); }           /* 0x171 */
    if (ax == 0x0000u) return (uint8_t)(bl | flag);                      /* 0x17B (bl==0) */
    if (ax == 0xFFFDu) return (uint8_t)(bl | flag);                      /* 0x182 (bl==0) */
    if (dl == 0x15u) { bl = (uint8_t)(dh + 0x40u); return (uint8_t)(bl | flag); } /* 0x18A */
    bl = 0x1Bu; if (dl == 0x16u) return (uint8_t)(bl | flag);            /* 0x197 */
    bl = 0x61u; if (dl == 0x34u) return (uint8_t)(bl | flag);            /* 0x1A1 */
    bl = 0x60u; if (dl == 0x33u) return (uint8_t)(bl | flag);            /* 0x1AB */
    bl = 0x20u; if (dl == 0x1Fu || dl == 0x11u || dl == 0x7Fu || dl == 0x0Fu)
                    return (uint8_t)(bl | flag);                         /* 0x1B5 */
    bl = 0x2Au; if (dl == 0x21u) return (uint8_t)(bl | flag);            /* 0x1D7 */
    bl = 0x1Eu; if (dl == 0x14u) return (uint8_t)(bl | flag);            /* 0x1E1 */
    bl = 0x26u; if (dl == 0x23u || dl == 0x27u) return (uint8_t)(bl | flag); /* 0x1EB */
    bl = 0x2Cu; if (dl == 0x12u) return (uint8_t)(bl | flag);            /* 0x1FD */
    bl = 0x2Eu; if (dl == 0x13u) return (uint8_t)(bl | flag);            /* 0x207 */
    bl = 0x1Au; if (dl == 0x0Eu) return (uint8_t)(bl | flag);            /* 0x211 */
    bl = 0x00u; if (dl == 0x00u) return (uint8_t)(bl | flag);            /* 0x21B */
    bl = 0x20u; if (dl == 0x10u) return (uint8_t)(bl | flag);            /* 0x225 */
    bl = 0x21u; if (dl == 0x0Du) return (uint8_t)(bl | flag);            /* 0x22F */
    bl = 0x3Eu; if (dl == 0x32u) return (uint8_t)(bl | flag);            /* 0x239 */
    bl = 0x3Cu; if (dl == 0x30u) return (uint8_t)(bl | flag);            /* 0x243 */
    bl = 0x3Du; if (dl == 0x31u) return (uint8_t)(bl | flag);            /* 0x24D */
    bl = 0x20u; if (dl == 0x0Cu) return (uint8_t)(bl | flag);            /* 0x257 */
    bl = 0x21u; if (dl == 0x28u) return (uint8_t)(bl | flag);            /* 0x261 */
    bl = 0x1Fu; if (dl == 0x29u) return (uint8_t)(bl | flag);            /* 0x26B */
    bl = 0x23u; if (dl == 0x2Au) return (uint8_t)(bl | flag);            /* 0x272 */
    bl = 0x37u; if (dl == 0x2Bu) return (uint8_t)(bl | flag);            /* 0x279 */
    bl = 0x00u; if (dl == 0x18u) return (uint8_t)(bl | flag);            /* 0x280 */
    bl = 0x22u; if (dl == 0x24u) return (uint8_t)(bl | flag);            /* 0x287 */
    bl = 0x1Cu; if (dl == 0x26u) return (uint8_t)(bl | flag);            /* 0x28E */
    bl = 0x36u; if (dl == 0x25u) return (uint8_t)(bl | flag);            /* 0x295 */
    bl = 0x36u; if (dl == 0x1Au || dl == 0x1Bu || dl == 0x1Cu || dl == 0x1Du)
                    return (uint8_t)(bl | flag);                         /* 0x29C */
    bl = 0x33u; if (dl == 0x2Eu) return (uint8_t)(bl | flag);            /* 0x2B2 */
    bl = 0x30u; if (dl == 0x2Fu) return (uint8_t)(bl | flag);            /* 0x2B9 */
    bl = 0x34u; if (dl == 0x04u || dl == 0x35u) return (uint8_t)(bl | flag); /* 0x2C0 */
    bl = 0x32u; if (dl == 0x06u || dl == 0x37u) return (uint8_t)(bl | flag); /* 0x2CC */
    bl = 0x31u; if (dl == 0x05u || dl == 0x36u) return (uint8_t)(bl | flag); /* 0x2D8 */
    bl = 0x26u; if (dl == 0x08u) return (uint8_t)(bl | flag);            /* 0x2E4 */
    bl = 0x20u; return (uint8_t)(bl | flag);                             /* 0x2EB default */
}
