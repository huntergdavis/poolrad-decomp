#include "overlay_u86_entry1_postdc0_jz_01ed.h"

int pr_u86_entry1_01ed_jz(pr_1816_caller_cpu *cpu, const uint8_t *ram,
                          size_t ram_size, uint16_t supplied_u86_cs)
{
    size_t address;

    if (cpu == NULL || ram == NULL ||
        ram_size != PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != supplied_u86_cs || cpu->ip != 0x01edu)
        return 1;

    address = (((size_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (ram[address] != 0x74u || ram[(address + 1u) & 0xfffffu] != 0xadu)
        return 1;

    /* Next IP 01EFh plus signed ADh (-83) reaches 019Ch. The preceding
     * far callee supplies ZF; this instruction does not change flags. */
    cpu->ip = (cpu->flags & 0x0040u) != 0u ? 0x019cu : 0x01efu;
    return 0;
}
