#include "amos_sprite.h"
#include <assert.h>
#include <string.h>
int main(void) {
    uint8_t bank[84] = {0}; memcpy(bank, "AmSp", 4); bank[5] = 1; bank[7] = 1; bank[9] = 1; bank[11] = 1;
    bank[16] = 0x80; bank[18] = 0x80; bank[22] = 0x0f; AmosSprite sprite; uint32_t pixels[16];
    assert(amos_sprite_get(bank, sizeof bank, 0, &sprite)); assert(amos_sprite_decode_argb(&sprite, pixels, 16));
    assert(pixels[0] == 0xffff0000u && pixels[1] == 0);
    uint8_t odd[82] = {0}; memcpy(odd, "AmSp", 4); odd[5] = 1; odd[6] = 0x80; odd[7] = 1; odd[9] = 1; odd[11] = 1;
    odd[16] = 0x80; odd[17] = 0x80; odd[20] = 0x0f;
    uint32_t odd_pixels[8]; assert(amos_sprite_get(odd, sizeof odd, 0, &sprite));
    assert(sprite.width == 8 && sprite.bytes_per_row == 1);
    assert(amos_sprite_decode_argb(&sprite, odd_pixels, 8)); assert(odd_pixels[0] == 0xffff0000u);
    return 0;
}
