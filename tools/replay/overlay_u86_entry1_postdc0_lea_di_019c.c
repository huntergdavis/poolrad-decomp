#include "overlay_u86_entry1_postdc0_lea_di_019c.h"

int pr_u86_entry1_019c_lea_di(pr_1816_caller_cpu *cpu, const uint8_t *ram,
                              size_t ram_size, uint16_t supplied_u86_cs)
{
    size_t address;

    if (cpu == NULL || ram == NULL ||
        ram_size != PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != supplied_u86_cs || cpu->ip != 0x019cu)
        return 1;

    address = (((size_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (ram[address] != 0x8du ||
        ram[(address + 1u) & 0xfffffu] != 0x7eu ||
        ram[(address + 2u) & 0xfffffu] != 0xceu)
        return 1;

    /* LEA computes an offset; it neither reads [BP-32h] nor changes flags. */
    cpu->di = (uint16_t)(cpu->bp - 0x32u);
    cpu->ip = 0x019fu;
    return 0;
}
