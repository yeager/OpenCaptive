#ifndef CAPTIVE_VIEWPORT_DESCRIPTORS_H
#define CAPTIVE_VIEWPORT_DESCRIPTORS_H

#include "captive_dos_descriptor.h"

#define CAPTIVE_VIEWPORT_DESCRIPTOR_COUNT 112

/* Viewport descriptor table extracted from CAPPO.EXE v1.06 (unpacked)
 * at file offset 0x21698.  Each record is 8 bytes: source_offset(u16)
 * destination_offset(u16) width_bytes(u8) height(u8) flags(u8) source_bank(u8).
 *
 * The table encodes the complete panel draw list for the 19-cell trapezoid
 * viewport.  Entries come in mirrored pairs (flags bit 0 toggles horizontal
 * flip).  Source bank 0 is the wall sheet, bank 4 is the floor/ceiling sheet.
 *
 * Depth bands (by destination y and height):
 *   y= 0, h=44/112  — ceiling / full-height side walls
 *   y= 9, h= 98     — depth 1 (nearest) walls
 *   y=25, h= 70     — depth 2 walls
 *   y=37, h= 49     — depth 3 walls
 *   y=45, h= 35     — depth 4 (farthest) walls
 *   y=75..105        — floor perspective strips (bank 4)
 */

static const CaptiveDosDescriptor captive_viewport_descriptors[CAPTIVE_VIEWPORT_DESCRIPTOR_COUNT] = {
    /* [  0] decorative:     56x  2 at ( 39,  3) */ {0x0207, 0x0207, 7, 2, 0x07, 2, 0},
    /* [  1] ceiling full: 144x 33 at (  0, 79) */ {0x0000, 0x3160, 18, 33, 0x02, 0, 0},
    /* [  2] ceiling flip: 144x 33 at (  0, 79) */ {0x0000, 0x3160, 18, 33, 0x03, 0, 0},
    /* [  3] floor full:  144x 44 at (  0,  0) */ {0x79e0, 0x0000, 18, 44, 0x02, 4, 0},
    /* [  4] floor flip:  144x 44 at (  0,  0) */ {0x79e0, 0x0000, 18, 44, 0x03, 4, 0},
    /* [  5] d3 left:      48x 49 at (  0, 37) */ {0x6662, 0x1720, 6, 49, 0x06, 0, 0},
    /* [  6] d3 left M:    48x 49 at (  0, 37) */ {0x6658, 0x1720, 6, 49, 0x07, 0, 0},
    /* [  7] d3 center:    64x 49 at ( 40, 37) */ {0x6658, 0x1748, 8, 49, 0x04, 0, 0},
    /* [  8] d3 center M:  64x 49 at ( 40, 37) */ {0x6658, 0x1748, 8, 49, 0x05, 0, 0},
    /* [  9] d3 right:     48x 49 at ( 96, 37) */ {0x6658, 0x1780, 6, 49, 0x06, 0, 0},
    /* [ 10] d3 right M:   48x 49 at ( 96, 37) */ {0x6662, 0x1780, 6, 49, 0x07, 0, 0},
    /* [ 11] d2 left:      32x 70 at (  0, 25) */ {0x0078, 0x0fa0, 4, 70, 0x02, 0, 0},
    /* [ 12] d2 left M:    32x 70 at (  0, 25) */ {0x005a, 0x0fa0, 4, 70, 0x03, 0, 0},
    /* [ 13] d2 center:    80x 70 at ( 32, 25) */ {0x005a, 0x0fc0, 10, 70, 0x02, 0, 0},
    /* [ 14] d2 center M:  80x 70 at ( 32, 25) */ {0x005a, 0x0fc0, 10, 70, 0x03, 0, 0},
    /* [ 15] d2 right:     32x 70 at (112, 25) */ {0x005a, 0x1010, 4, 70, 0x02, 0, 0},
    /* [ 16] d2 right M:   32x 70 at (112, 25) */ {0x0078, 0x1010, 4, 70, 0x03, 0, 0},
    /* [ 17] d1 left:      16x 98 at (  0,  9) */ {0x1a04, 0x05a0, 2, 98, 0x02, 0, 0},
    /* [ 18] d1 left M:    16x 98 at (  0,  9) */ {0x19c8, 0x05a0, 2, 98, 0x03, 0, 0},
    /* [ 19] d1 center:   112x 98 at ( 16,  9) */ {0x19c8, 0x05b0, 14, 98, 0x02, 0, 0},
    /* [ 20] d1 center M: 112x 98 at ( 16,  9) */ {0x19c8, 0x05b0, 14, 98, 0x03, 0, 0},
    /* [ 21] d1 right:     16x 98 at (128,  9) */ {0x19c8, 0x0620, 2, 98, 0x02, 0, 0},
    /* [ 22] d1 right M:   16x 98 at (128,  9) */ {0x1a04, 0x0620, 2, 98, 0x03, 0, 0},
    /* [ 23] d3 orn left:  16x 49 at ( 40, 37) */ {0x6680, 0x1748, 2, 49, 0x05, 0, 0},
    /* [ 24] d3 orn left2: 16x 49 at ( 40, 37) */ {0x668a, 0x1748, 2, 49, 0x05, 0, 0},
    /* [ 25] d2 orn left:  16x 70 at ( 32, 25) */ {0x371e, 0x0fc0, 2, 70, 0x07, 0, 0},
    /* [ 26] d2 orn left2: 16x 70 at ( 32, 25) */ {0x3728, 0x0fc0, 2, 70, 0x07, 0, 0},
    /* [ 27] d1 orn left:  16x 98 at ( 16,  9) */ {0x1a0e, 0x05b0, 2, 98, 0x07, 0, 0},
    /* [ 28] d1 orn left2: 16x 98 at ( 16,  9) */ {0x1a18, 0x05b0, 2, 98, 0x07, 0, 0},
    /* [ 29] full left:    16x112 at (  0,  0) */ {0x3732, 0x0000, 2, 112, 0x07, 0, 0},
    /* [ 30] full left2:   16x112 at (  0,  0) */ {0x373c, 0x0000, 2, 112, 0x07, 0, 0},
    /* [ 31] d3 orn right: 16x 49 at ( 88, 37) */ {0x6680, 0x1778, 2, 49, 0x04, 0, 0},
    /* [ 32] d3 orn right2:16x 49 at ( 88, 37) */ {0x668a, 0x1778, 2, 49, 0x04, 0, 0},
    /* [ 33] d2 orn right: 16x 70 at ( 96, 25) */ {0x371e, 0x1000, 2, 70, 0x06, 0, 0},
    /* [ 34] d2 orn right2:16x 70 at ( 96, 25) */ {0x3728, 0x1000, 2, 70, 0x06, 0, 0},
    /* [ 35] d1 orn right: 16x 98 at (112,  9) */ {0x1a0e, 0x0610, 2, 98, 0x06, 0, 0},
    /* [ 36] d1 orn right2:16x 98 at (112,  9) */ {0x1a18, 0x0610, 2, 98, 0x06, 0, 0},
    /* [ 37] full right:   16x112 at (128,  0) */ {0x3732, 0x0080, 2, 112, 0x06, 0, 0},
    /* [ 38] full right2:  16x112 at (128,  0) */ {0x373c, 0x0080, 2, 112, 0x06, 0, 0},
    /* [ 39] pillar L:     16x 41 at (  0, 41) */ {0x008c, 0x19a0, 2, 41, 0x07, 0, 0},
    /* [ 40] pillar L dup: 16x 41 at (  0, 41) */ {0x008c, 0x19a0, 2, 41, 0x07, 0, 0},
    /* [ 41] pillar R:     16x 41 at (128, 41) */ {0x008c, 0x1a20, 2, 41, 0x06, 0, 0},
    /* [ 42] pillar R dup: 16x 41 at (128, 41) */ {0x008c, 0x1a20, 2, 41, 0x06, 0, 0},
    /* [ 43] d4 far left:  16x 35 at (  0, 45) */ {0x66a8, 0x1c20, 2, 35, 0x06, 0, 0},
    /* [ 44] d4 far left M:16x 35 at (  0, 45) */ {0x6694, 0x1c20, 2, 35, 0x07, 0, 0},
    /* [ 45] d4 left:      48x 35 at (  8, 45) */ {0x6694, 0x1c28, 6, 35, 0x04, 0, 0},
    /* [ 46] d4 left M:    48x 35 at (  8, 45) */ {0x6694, 0x1c28, 6, 35, 0x05, 0, 0},
    /* [ 47] d4 center L:  48x 35 at ( 48, 45) */ {0x6694, 0x1c50, 6, 35, 0x06, 0, 0},
    /* [ 48] d4 center LM: 48x 35 at ( 48, 45) */ {0x6694, 0x1c50, 6, 35, 0x07, 0, 0},
    /* [ 49] d4 center R:  48x 35 at ( 88, 45) */ {0x6694, 0x1c78, 6, 35, 0x04, 0, 0},
    /* [ 50] d4 center RM: 48x 35 at ( 88, 45) */ {0x6694, 0x1c78, 6, 35, 0x05, 0, 0},
    /* [ 51] d4 right:     16x 35 at (128, 45) */ {0x6694, 0x1ca0, 2, 35, 0x06, 0, 0},
    /* [ 52] d4 far right: 16x 35 at (128, 45) */ {0x66a8, 0x1ca0, 2, 35, 0x07, 0, 0},
    /* [ 53] flr d4 L:     16x  3 at (  0, 79) */ {0x64be, 0x3160, 2, 3, 0x06, 4, 0},
    /* [ 54] flr d4 L M:   16x  3 at (  0, 79) */ {0x6716, 0x3160, 2, 3, 0x07, 4, 0},
    /* [ 55] flr d4 CL:    56x  7 at (  0, 79) */ {0x4d12, 0x3160, 7, 7, 0x04, 4, 0},
    /* [ 56] flr d4 CL M:  56x  7 at (  0, 79) */ {0x4d35, 0x3160, 7, 7, 0x05, 4, 0},
    /* [ 57] flr d4 C:     64x  7 at ( 40, 79) */ {0x4cea, 0x3188, 8, 7, 0x04, 4, 0},
    /* [ 58] flr d4 C M:   64x  7 at ( 40, 79) */ {0x4cea, 0x3188, 8, 7, 0x05, 4, 0},
    /* [ 59] flr d4 CR:    56x  7 at ( 88, 79) */ {0x4d35, 0x31b8, 7, 7, 0x04, 4, 0},
    /* [ 60] flr d4 CR M:  56x  7 at ( 88, 79) */ {0x4d12, 0x31b8, 7, 7, 0x05, 4, 0},
    /* [ 61] flr d4 R:     16x  3 at (128, 79) */ {0x6716, 0x31e0, 2, 3, 0x06, 4, 0},
    /* [ 62] flr d4 R M:   16x  3 at (128, 79) */ {0x64be, 0x31e0, 2, 3, 0x07, 4, 0},
    /* [ 63] flr d3 L:     48x 11 at (  0, 84) */ {0x51cc, 0x3480, 6, 11, 0x06, 4, 0},
    /* [ 64] flr d3 L M:   48x 11 at (  0, 84) */ {0x51ea, 0x3480, 6, 11, 0x07, 4, 0},
    /* [ 65] flr d3 C:     80x 11 at ( 32, 84) */ {0x519a, 0x34a0, 10, 11, 0x06, 4, 0},
    /* [ 66] flr d3 C M:   80x 11 at ( 32, 84) */ {0x519a, 0x34a0, 10, 11, 0x07, 4, 0},
    /* [ 67] flr d3 R:     48x 11 at ( 96, 84) */ {0x51ea, 0x34e0, 6, 11, 0x06, 4, 0},
    /* [ 68] flr d3 R M:   48x 11 at ( 96, 84) */ {0x51cc, 0x34e0, 6, 11, 0x07, 4, 0},
    /* [ 69] flr d2 L:     32x 14 at (  0, 93) */ {0x59b0, 0x3a20, 4, 14, 0x06, 4, 0},
    /* [ 70] flr d2 L M:   32x 14 at (  0, 93) */ {0x59c4, 0x3a20, 4, 14, 0x07, 4, 0},
    /* [ 71] flr d2 C:    112x 14 at ( 16, 93) */ {0x596a, 0x3a30, 14, 14, 0x06, 4, 0},
    /* [ 72] flr d2 C M:  112x 14 at ( 16, 93) */ {0x596a, 0x3a30, 14, 14, 0x07, 4, 0},
    /* [ 73] flr d2 R:     32x 14 at (112, 93) */ {0x59c4, 0x3a90, 4, 14, 0x06, 4, 0},
    /* [ 74] flr d2 R M:   32x 14 at (112, 93) */ {0x59b0, 0x3a90, 4, 14, 0x07, 4, 0},
    /* [ 75] flr d1 L:     16x  7 at (  0,105) */ {0x63e2, 0x41a0, 2, 7, 0x06, 4, 0},
    /* [ 76] flr d1 L M:   16x  7 at (  0,105) */ {0x63ec, 0x41a0, 2, 7, 0x07, 4, 0},
    /* [ 77] flr d1 C:    128x  7 at (  8,105) */ {0x6392, 0x41a8, 16, 7, 0x04, 4, 0},
    /* [ 78] flr d1 C M:  128x  7 at (  8,105) */ {0x6392, 0x41a8, 16, 7, 0x05, 4, 0},
    /* [ 79] flr d1 R:     16x  7 at (128,105) */ {0x63ec, 0x4220, 2, 7, 0x06, 4, 0},
    /* [ 80] flr d1 R M:   16x  7 at (128,105) */ {0x63e2, 0x4220, 2, 7, 0x07, 4, 0},
    /* [ 81] ceil d4 L:    16x  5 at (  0, 77) */ {0x9466, 0x3020, 2, 5, 0x06, 4, 0},
    /* [ 82] ceil d4 L M:  16x  5 at (  0, 77) */ {0x984e, 0x3020, 2, 5, 0x07, 4, 0},
    /* [ 83] ceil d4 CL:   56x 10 at (  0, 75) */ {0x6932, 0x2ee0, 7, 10, 0x04, 4, 0},
    /* [ 84] ceil d4 CLM:  56x 10 at (  0, 75) */ {0x6955, 0x2ee0, 7, 10, 0x05, 4, 0},
    /* [ 85] ceil d4 C:    64x 10 at ( 40, 75) */ {0x690a, 0x2f08, 8, 10, 0x04, 4, 0},
    /* [ 86] ceil d4 C M:  64x 10 at ( 40, 75) */ {0x690a, 0x2f08, 8, 10, 0x05, 4, 0},
    /* [ 87] ceil d4 CR:   56x 10 at ( 88, 75) */ {0x6955, 0x2f38, 7, 10, 0x04, 4, 0},
    /* [ 88] ceil d4 CRM:  56x 10 at ( 88, 75) */ {0x6932, 0x2f38, 7, 10, 0x05, 4, 0},
    /* [ 89] ceil d4 R:    16x  5 at (128, 77) */ {0x984e, 0x30a0, 2, 5, 0x06, 4, 0},
    /* [ 90] ceil d4 RM:   16x  5 at (128, 77) */ {0x9466, 0x30a0, 2, 5, 0x07, 4, 0},
    /* [ 91] ceil d3 L:    48x 17 at (  0, 77) */ {0x710c, 0x3020, 6, 17, 0x06, 4, 0},
    /* [ 92] ceil d3 LM:   48x 17 at (  0, 77) */ {0x712a, 0x3020, 6, 17, 0x07, 4, 0},
    /* [ 93] ceil d3 C:    80x 16 at ( 32, 78) */ {0x71a2, 0x30e0, 10, 16, 0x06, 4, 0},
    /* [ 94] ceil d3 CM:   80x 16 at ( 32, 78) */ {0x71a2, 0x30e0, 10, 16, 0x07, 4, 0},
    /* [ 95] ceil d3 R:    48x 17 at ( 96, 77) */ {0x712a, 0x3080, 6, 17, 0x06, 4, 0},
    /* [ 96] ceil d3 RM:   48x 17 at ( 96, 77) */ {0x710c, 0x3080, 6, 17, 0x07, 4, 0},
    /* [ 97] ceil d2 L:    32x 26 at (  0, 80) */ {0x7ff8, 0x3200, 4, 26, 0x06, 4, 0},
    /* [ 98] ceil d2 LM:   32x 26 at (  0, 80) */ {0x800c, 0x3200, 4, 26, 0x07, 4, 0},
    /* [ 99] ceil d2 C:   112x 28 at ( 16, 78) */ {0x7e22, 0x30d0, 14, 28, 0x06, 4, 0},
    /* [100] ceil d2 CM:  112x 28 at ( 16, 78) */ {0x7e22, 0x30d0, 14, 28, 0x07, 4, 0},
    /* [101] ceil d2 R:    32x 26 at (112, 80) */ {0x800c, 0x3270, 4, 26, 0x06, 4, 0},
    /* [102] ceil d2 RM:   32x 26 at (112, 80) */ {0x7ff8, 0x3270, 4, 26, 0x07, 4, 0},
    /* [103] ceil d1 L:    16x 10 at (  0,102) */ {0x951a, 0x3fc0, 2, 10, 0x06, 4, 0},
    /* [104] ceil d1 LM:   16x 10 at (  0,102) */ {0x9524, 0x3fc0, 2, 10, 0x07, 4, 0},
    /* [105] ceil d1 C:   128x 11 at (  8,101) */ {0x9402, 0x3f28, 16, 11, 0x04, 4, 0},
    /* [106] ceil d1 CM:  128x 11 at (  8,101) */ {0x9402, 0x3f28, 16, 11, 0x05, 4, 0},
    /* [107] ceil d1 R:    16x 10 at (128,102) */ {0x9524, 0x4040, 2, 10, 0x06, 4, 0},
    /* [108] ceil d1 RM:   16x 10 at (128,102) */ {0x951a, 0x4040, 2, 10, 0x07, 4, 0},
    /* [109] door d3 L:    24x 45 at (  0, 40) */ {0x29c2, 0x1900, 3, 45, 0x04, 4, 0},
    /* [110] door d3 C:    24x 45 at ( 48, 40) */ {0x29c2, 0x1930, 3, 45, 0x04, 4, 0},
    /* [111] door d3 R:    24x 45 at ( 96, 40) */ {0x29c2, 0x1960, 3, 45, 0x04, 4, 0},
};

/* Draw order groups for the 19-cell viewport.
 * The original compositor draws back-to-front: farthest depth band first,
 * ceiling+floor first at each depth, then walls, then ornaments. */

/* Depth band 4 (farthest): entries 43-52 (walls), 53-62 (floor), 81-90 (ceiling) */
/* Depth band 3: entries 5-10 (walls), 63-68 (floor), 91-96 (ceiling) */
/* Depth band 2: entries 11-16 (walls), 69-74 (floor), 97-102 (ceiling) */
/* Depth band 1 (nearest): entries 17-22 (walls), 75-80 (floor), 103-108 (ceiling) */
/* Full-height: entries 1-4 (full ceiling+floor), 29-30/37-38 (side columns) */
/* Ornaments: entries 23-28 (wall ornament overlays) */
/* Doors: entries 109-111 */

typedef enum {
    CAPTIVE_DESC_FLAG_MIRROR_H = 0x01,
    CAPTIVE_DESC_FLAG_MASK_ZERO = 0x04,
} CaptiveDescriptorFlag;

#endif
