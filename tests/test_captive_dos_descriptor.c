#include "captive_dos_descriptor.h"

#include <assert.h>
#include <string.h>

int main(void) {
    uint8_t memory[1024 * 1024];
    memset(memory, 0, sizeof(memory));
    const uint16_t descriptor_segment = 0x2942;
    const uint16_t code_segment = 0x0824;
    const uint16_t graphic_id = 4;
    size_t descriptor = ((size_t)descriptor_segment << 4) + 0xc0 + graphic_id * 8;
    memory[descriptor + 0] = 0x62;
    memory[descriptor + 1] = 0x66;
    memory[descriptor + 2] = 0x20;
    memory[descriptor + 3] = 0x17;
    memory[descriptor + 4] = 6;
    memory[descriptor + 5] = 49;
    memory[descriptor + 6] = 6;
    memory[descriptor + 7] = 2;
    size_t bank = ((size_t)code_segment << 4) + 0x423c + 2 * 2;
    memory[bank] = 0xcf;
    memory[bank + 1] = 0x55;

    CaptiveDosDescriptor result = {0};
    assert(captive_dos_descriptor_read(memory, sizeof(memory), descriptor_segment,
                                       code_segment, graphic_id, &result));
    assert(result.source_offset == 0x6662 && result.destination_offset == 0x1720);
    assert(result.width_bytes == 6 && result.height == 49 && result.flags == 6);
    assert(result.source_bank == 2 && result.source_segment == 0x55cf);
    assert(!captive_dos_descriptor_read(memory, sizeof(memory) - 1,
                                        descriptor_segment, code_segment,
                                        graphic_id, &result));
    return 0;
}
