#include "poolrad/recovered/overlay_u1c_script_load_ax_3193.h"

int pr_rec_u1c_3193_load_script_ax(pr_rec_u1c_33bd_cpu *cpu,
                                    const uint8_t *ram, size_t ram_size)
{
    uint32_t code_address;
    uint32_t low_address;
    uint32_t high_address;
    uint16_t word;

    if (cpu == NULL || ram == NULL ||
        (ram_size != 655360u && ram_size != 1048576u) ||
        cpu->cs != 0x3582u || cpu->ip != 0x3193u ||
        (cpu->flags & 0x0040u) == 0u)
        return 0;

    code_address = (((uint32_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (code_address >= ram_size || code_address + 1u >= ram_size ||
        code_address + 2u >= ram_size || ram[code_address] != 0xa1u ||
        ram[code_address + 1u] != 0xedu || ram[code_address + 2u] != 0x49u)
        return 0;

    low_address = (((uint32_t)cpu->ds << 4) + 0x49edu) & 0xfffffu;
    high_address = (((uint32_t)cpu->ds << 4) + 0x49eeu) & 0xfffffu;
    if (low_address >= ram_size || high_address >= ram_size) return 0;

    word = (uint16_t)(ram[low_address] | ((uint16_t)ram[high_address] << 8));
    cpu->ax = word;
    cpu->ip = 0x3196u;
    return 1;
}
