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
        unsigned bpr = (w_raw & 0x7FFF) * 2 - (w_raw >> 15);
        if (!bpr || !h || !depth || depth > 8) return 0;
        size_t bytes = (size_t)bpr * h * (depth + (has_mask ? 1 : 0));
        pos += (10 + bytes + 1) & ~(size_t)1;
    }
    return pos;
}

bool vgm_open(VgmFile *vgm, const uint8_t *data, size_t size) {
    if (!vgm || !data) return false;
    memset(vgm, 0, sizeof(*vgm));

    size_t pos = 0;
    while (pos + 6 <= size && vgm->num_banks < VGM_MAX_BANKS) {
        if (memcmp(data + pos, "AmSp", 4) != 0) break;

        size_t bank_sz = amsp_bank_size(data + pos, size - pos);
        if (bank_sz == 0) break;

        unsigned b = vgm->num_banks;
        vgm->bank_data[b] = data + pos;
        vgm->bank_size[b] = bank_sz;
        vgm->bank_count[b] = be16(data + pos + 4);
        vgm->bank_first_index[b] = (uint16_t)vgm->total_sprites;
        vgm->total_sprites += vgm->bank_count[b];
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
