/* DOS v1.3 GAME.OVR U3B0 +0880: 26 8A 05. */
#include "poolrad/recovered/overlay_u3b0_0880_es_al_read.h"

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

pr_u3b0_0880_es_al_read_status pr_u3b0_0880_es_al_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0880u || (cpu->flags & 0x0001u) == 0u)
        return PR_U3B0_0880_ES_AL_READ_ARGUMENT;
    if (ram[physical(cpu->cs, 0x0880u)] != 0x26u ||
        ram[physical(cpu->cs, 0x0881u)] != 0x8Au ||
        ram[physical(cpu->cs, 0x0882u)] != 0x05u)
        return PR_U3B0_0880_ES_AL_READ_ARGUMENT;

    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical(cpu->es, cpu->di)]);
    cpu->ip = 0x0883u;
    return PR_U3B0_0880_ES_AL_READ_OK;
}
