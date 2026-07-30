#include "dos_vga_reference.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    uint8_t *memory = calloc(DOS_VGA_MEMORY_SIZE, 1);
    uint32_t pixels[DOS_VGA_FRAME_SIZE];
    assert(memory);

    memory[DOS_VGA_FRAME_OFFSET] = 17;
    memory[DOS_VGA_FRAME_OFFSET + DOS_VGA_FRAME_SIZE - 1] = 20;
    assert(dos_vga_reference_decode(memory, DOS_VGA_MEMORY_SIZE, pixels,
                                    DOS_VGA_FRAME_SIZE));
    assert(pixels[0] == 0xFFFFFFFF);
    assert(pixels[DOS_VGA_FRAME_SIZE - 1] == 0xFF825130);
    assert(!dos_vga_reference_decode(memory, DOS_VGA_MEMORY_SIZE - 1, pixels,
                                     DOS_VGA_FRAME_SIZE));
    assert(!dos_vga_reference_decode(memory, DOS_VGA_MEMORY_SIZE, pixels,
                                     DOS_VGA_FRAME_SIZE - 1));
    free(memory);
    return 0;
}
