#include "poolrad/recovered/overlay_u1c_script_load_al_318d.h"

int pr_rec_u1c_318d_load_latch_al(pr_rec_u1c_33bd_cpu *cpu,
                                  const uint8_t *ram, size_t ram_size)
{
    uint32_t address;
    uint32_t code_address;

    if (cpu == NULL || ram == NULL ||
        (ram_size != 655360u && ram_size != 1048576u) ||
        cpu->cs != 0x3582u || cpu->ip != 0x318du ||
        (cpu->flags & 0x0040u) == 0u)
        return 0;

    code_address = (((uint32_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (code_address >= ram_size || code_address + 1u >= ram_size ||
        code_address + 2u >= ram_size || ram[code_address] != 0xa0u ||
        ram[code_address + 1u] != 0x7fu || ram[code_address + 2u] != 0x6fu)
        return 0;

    address = (((uint32_t)cpu->ds << 4) + 0x6f7fu) & 0xfffffu;
    if (address >= ram_size) return 0;

    cpu->ax = (uint16_t)((cpu->ax & 0xff00u) | ram[address]);
    cpu->ip = 0x3190u;
    return 1;
}
