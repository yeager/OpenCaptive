#include "liberation_vgm.h"
#include <string.h>

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static size_t amsp_bank_size(const uint8_t *data, size_t avail) {
    if (avail < 6 || memcmp(data, "AmSp", 4) != 0) return 0;
    unsigned count = be16(data + 4);
    size_t pos = 6;
    for (unsigned i = 0; i < count; i++) {
        if (pos + 10 > avail) return 0;
        unsigned w_raw = be16(data + pos);
        unsigned h = be16(data + pos + 2);
        unsigned depth = be16(data + pos + 4);
        bool has_mask = (be16(data + pos + 6) & 0x8000u) != 0;
        unsigned word_count = w_raw & 0x7FFFu;
        if (word_count == 0) return 0;
        unsigned bpr = word_count * 2 - (w_raw >> 15);
        /* AmosSprite exposes a 32-colour palette and rejects depths above
           five; reject those banks here instead of accepting an unusable VGM. */
        if (!bpr || bpr > UINT16_MAX / 8U || !h || !depth || depth > 5)
            return 0;
        size_t plane_count = depth + (has_mask ? 1U : 0U);
        if ((size_t)bpr > SIZE_MAX / h ||
            (size_t)bpr * h > SIZE_MAX / plane_count)
            return 0;
        size_t bytes = (size_t)bpr * h * plane_count;
        if (bytes > avail - pos - 10U) return 0;
        size_t record = (10U + bytes + 1U) & ~(size_t)1;
        if (record > avail - pos) return 0;
        pos += record;
    }
    return pos;
}

bool vgm_open(VgmFile *vgm, const uint8_t *data, size_t size) {
    if (!vgm) return false;
    memset(vgm, 0, sizeof(*vgm));
    if (!data) return false;

    size_t pos = 0;
    while (pos + 6 <= size && vgm->num_banks < VGM_MAX_BANKS) {
        if (memcmp(data + pos, "AmSp", 4) != 0) break;

        size_t bank_sz = amsp_bank_size(data + pos, size - pos);
        if (bank_sz == 0) break;

        unsigned b = vgm->num_banks;
        unsigned bank_count = be16(data + pos + 4);
        if (bank_count == 0 || bank_count > VGM_MAX_SPRITES ||
            vgm->total_sprites > VGM_MAX_SPRITES - bank_count) {
            memset(vgm, 0, sizeof(*vgm));
            return false;
        }
        vgm->bank_data[b] = data + pos;
        vgm->bank_size[b] = bank_sz;
        vgm->bank_count[b] = (uint16_t)bank_count;
        vgm->bank_first_index[b] = (uint16_t)vgm->total_sprites;
        vgm->total_sprites += bank_count;
        vgm->num_banks++;
        pos += bank_sz;
    }

    return vgm->num_banks > 0;
}

bool vgm_get_sprite(const VgmFile *vgm, unsigned index, AmosSprite *out) {
    if (!vgm || index >= vgm->total_sprites) return false;

    for (unsigned b = 0; b < vgm->num_banks; b++) {
        if (index < vgm->bank_first_index[b] + vgm->bank_count[b]) {
            unsigned local = index - vgm->bank_first_index[b];
            return amos_sprite_get(vgm->bank_data[b], vgm->bank_size[b],
                                   local, out);
        }
    }
    return false;
}
