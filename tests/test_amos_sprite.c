#include "amos_sprite.h"
#include <assert.h>
#include <string.h>
int main(void) {
    uint8_t bank[84] = {0}; memcpy(bank, "AmSp", 4); bank[5] = 1; bank[7] = 1; bank[9] = 1; bank[11] = 1;
    bank[16] = 0x80; bank[18] = 0x80; bank[22] = 0x0f; AmosSprite sprite; uint32_t pixels[16];
    assert(amos_sprite_get(bank, sizeof bank, 0, &sprite)); assert(amos_sprite_decode_argb(&sprite, pixels, 16));
    assert(pixels[0] == 0xffff0000u && pixels[1] == 0); return 0;
}
