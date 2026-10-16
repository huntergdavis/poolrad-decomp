#include "poolrad/recovered/overlay_u1c_script_store_al_3190.h"

int pr_rec_u1c_3190_store_latch_al(pr_rec_u1c_33bd_cpu *cpu,
                                   uint8_t *ram, size_t ram_size)
{
    uint32_t code_address;
    uint32_t address;

    if (cpu == NULL || ram == NULL ||
        (ram_size != 655360u && ram_size != 1048576u) ||
        cpu->cs != 0x3582u || cpu->ip != 0x3190u ||
        (cpu->flags & 0x0040u) == 0u)
        return 0;

    code_address = (((uint32_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (code_address >= ram_size || code_address + 1u >= ram_size ||
        code_address + 2u >= ram_size || ram[code_address] != 0xa2u ||
        ram[code_address + 1u] != 0x7eu || ram[code_address + 2u] != 0x6fu)
        return 0;

    address = (((uint32_t)cpu->ds << 4) + 0x6f7eu) & 0xfffffu;
    if (address >= ram_size) return 0;

    ram[address] = (uint8_t)cpu->ax;
    cpu->ip = 0x3193u;
    return 1;
}
