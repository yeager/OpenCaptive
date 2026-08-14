#include "captive_dos_generator.h"

/* Cardinal-step deltas, from DS:0x5E18 (DX) and DS:0x5E20 (DY). */
const int8_t captive_gen_dx[4] = { 0, -1, 0, 1 };
const int8_t captive_gen_dy[4] = { -1, 0, 1, 0 };

size_t captive_gen_postprocess(uint8_t cells[CAPTIVE_GEN_MAP_SIZE]) {
    /* CAPPO 0x4661:
     *     mov cx, 0x800
     * .loop:
     *     lodsb ; and al,0x7f ; cmp al,0x1b ; je .add5
     *     cmp al,0x48 ; je .or10 ; cmp al,0x4b ; je .or10
     *     cmp al,0x40 ; je .or10 ; cmp al,0x43 ; jne .next
     * .or10: or  byte[si-1], 0x10 ; jmp .next
     * .add5: add byte[si-1], 5
     * .next: loop .loop
     */
    size_t changed = 0;
    for (size_t i = 0; i < CAPTIVE_GEN_MAP_SIZE; ++i) {
        uint8_t al = (uint8_t)(cells[i] & 0x7Fu);
        if (al == 0x1Bu) {
            cells[i] = (uint8_t)(cells[i] + 5u);
            ++changed;
        } else if (al == 0x40u || al == 0x43u || al == 0x48u || al == 0x4Bu) {
            cells[i] = (uint8_t)(cells[i] | 0x10u);
            ++changed;
        }
    }
    return changed;
}

void captive_gen_carve_cell(uint8_t cells[CAPTIVE_GEN_MAP_SIZE], int x, int y) {
    /* CAPPO 0x4736: cmp cl,0x3f/ja; cmp dl,0x1f/ja; al=[bx+di]; and al,0x80;
     * or al,0x26; mov [bx+di],al. */
    if (!captive_gen_in_bounds(x, y))
        return;
    int i = captive_gen_index(x, y);
    cells[i] = (uint8_t)((cells[i] & 0x80u) | CAPTIVE_GEN_WALK_STAMP);
}

void captive_gen_carve_plus(uint8_t cells[CAPTIVE_GEN_MAP_SIZE], int x, int y) {
    /* CAPPO 0x4706..0x472A: centre, then W (x-1), E (x+1), N (y-1), S (y+1),
     * each stamped through the same bounds-checked writer. */
    captive_gen_carve_cell(cells, x, y);
    captive_gen_carve_cell(cells, x - 1, y);
    captive_gen_carve_cell(cells, x + 1, y);
    captive_gen_carve_cell(cells, x, y - 1);
    captive_gen_carve_cell(cells, x, y + 1);
}
