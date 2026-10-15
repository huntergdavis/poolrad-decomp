/* DOS v1.3 GAME.OVR U3B0 +08C1: 26 8B 55 2C. */
#include "poolrad/recovered/overlay_u3b0_08c1_es_dx_read.h"

pr_u3b0_08c1_es_dx_read_status pr_u3b0_08c1_es_dx_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset, high_offset;
    uint32_t code_base, base, low_physical, high_physical;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x08C1u || (cpu->flags & 0x0001u) != 0u)
        return PR_U3B0_08C1_ES_DX_READ_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x08C1u) & 0xFFFFFu] != 0x26u ||
        ram[(code_base + 0x08C2u) & 0xFFFFFu] != 0x8Bu ||
        ram[(code_base + 0x08C3u) & 0xFFFFFu] != 0x55u ||
        ram[(code_base + 0x08C4u) & 0xFFFFFu] != 0x2Cu)
        return PR_U3B0_08C1_ES_DX_READ_ARGUMENT;

    offset = (uint16_t)(cpu->di + 0x2Cu);
    high_offset = (uint16_t)(offset + 1u);
    base = (uint32_t)cpu->es << 4;
    low_physical = (base + offset) & 0xFFFFFu;
    high_physical = (base + high_offset) & 0xFFFFFu;
    cpu->dx = (uint16_t)((uint16_t)ram[low_physical] |
                         ((uint16_t)ram[high_physical] << 8));
    cpu->ip = 0x08C5u;
    return PR_U3B0_08C1_ES_DX_READ_OK;
}
