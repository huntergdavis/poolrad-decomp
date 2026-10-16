#include "poolrad/recovered/overlay_u1c_key_jmp_33ea.h"

int pr_rec_u1c_33ea_jmp_342c(pr_rec_u1c_33bd_cpu *cpu,
                              const uint8_t *ram, size_t ram_size)
{
    uint32_t code_address;

    if (cpu == NULL || ram == NULL ||
        (ram_size != 655360u && ram_size != 1048576u) ||
        cpu->cs != 0x3582u || cpu->ip != 0x33eau)
        return 0;

    code_address = (((uint32_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (code_address >= ram_size || code_address + 1u >= ram_size ||
        ram[code_address] != 0xebu || ram[code_address + 1u] != 0x40u)
        return 0;

    cpu->ip = 0x342cu;
    return 1;
}
