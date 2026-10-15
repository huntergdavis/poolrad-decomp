/* DOS v1.3 GAME.OVR U3B0 +08BD: 26 8B 45 2A. */
#include "poolrad/recovered/overlay_u3b0_08bd_es_ax_read.h"

pr_u3b0_08bd_es_ax_read_status pr_u3b0_08bd_es_ax_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset, high_offset;
    uint32_t code_base, base, low_physical, high_physical;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x08BDu || (cpu->flags & 0x0001u) != 0u)
        return PR_U3B0_08BD_ES_AX_READ_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x08BDu) & 0xFFFFFu] != 0x26u ||
        ram[(code_base + 0x08BEu) & 0xFFFFFu] != 0x8Bu ||
        ram[(code_base + 0x08BFu) & 0xFFFFFu] != 0x45u ||
        ram[(code_base + 0x08C0u) & 0xFFFFFu] != 0x2Au)
        return PR_U3B0_08BD_ES_AX_READ_ARGUMENT;

    offset = (uint16_t)(cpu->di + 0x2Au);
    high_offset = (uint16_t)(offset + 1u);
    base = (uint32_t)cpu->es << 4;
    low_physical = (base + offset) & 0xFFFFFu;
    high_physical = (base + high_offset) & 0xFFFFFu;
    cpu->ax = (uint16_t)((uint16_t)ram[low_physical] |
                         ((uint16_t)ram[high_physical] << 8));
    cpu->ip = 0x08C1u;
    return PR_U3B0_08BD_ES_AX_READ_OK;
}
