#include "poolrad/recovered/overlay_u1c_script_les_di_3196.h"

int pr_rec_u1c_3196_les_script_pointer(pr_rec_u1c_33bd_cpu *cpu,
                                        const uint8_t *ram, size_t ram_size)
{
    uint32_t code_address;
    uint32_t address[4];
    uint32_t i;
    uint16_t di, es;

    if (cpu == NULL || ram == NULL ||
        (ram_size != 655360u && ram_size != 1048576u) ||
        cpu->cs != 0x3582u || cpu->ip != 0x3196u ||
        (cpu->flags & 0x0040u) == 0u)
        return 0;

    code_address = (((uint32_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (code_address >= ram_size || code_address + 1u >= ram_size ||
        code_address + 2u >= ram_size || code_address + 3u >= ram_size ||
        ram[code_address] != 0xc4u || ram[code_address + 1u] != 0x3eu ||
        ram[code_address + 2u] != 0xdeu || ram[code_address + 3u] != 0x49u)
        return 0;

    for (i = 0u; i < 4u; i++) {
        address[i] = (((uint32_t)cpu->ds << 4) + 0x49deu + i) & 0xfffffu;
        if (address[i] >= ram_size) return 0;
    }

    di = (uint16_t)(ram[address[0]] | ((uint16_t)ram[address[1]] << 8));
    es = (uint16_t)(ram[address[2]] | ((uint16_t)ram[address[3]] << 8));
    cpu->di = di;
    cpu->es = es;
    cpu->ip = 0x319au;
    return 1;
}
