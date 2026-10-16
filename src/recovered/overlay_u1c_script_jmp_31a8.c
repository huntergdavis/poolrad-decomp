#include "poolrad/recovered/overlay_u1c_script_jmp_31a8.h"

int pr_rec_u1c_31a8_jmp_loop(pr_rec_u1c_33bd_cpu *cpu,
                             const uint8_t *ram, size_t ram_size)
{
    uint32_t address;

    if (cpu == NULL || ram == NULL ||
        (ram_size != 655360u && ram_size != 1048576u) ||
        cpu->cs != 0x3582u || cpu->ip != 0x31a8u)
        return 0;

    address = (((uint32_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (address >= ram_size || address + 1u >= ram_size ||
        ram[address] != 0xebu || ram[address + 1u] != 0xd5u)
        return 0;

    /* 31AAh + signed D5h (-43) = 317Fh. Neither JMP nor this gate
     * reads the returned stack words or changes any flag or RAM byte. */
    cpu->ip = 0x317fu;
    return 1;
}
