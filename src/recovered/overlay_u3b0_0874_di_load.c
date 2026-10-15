/* DOS v1.3 GAME.OVR U3B0 +0874: 8B 7E 06. */
#include "poolrad/recovered/overlay_u3b0_0874_di_load.h"

pr_u3b0_0874_di_load_status pr_u3b0_0874_di_load(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset, high_offset;
    uint32_t base, low_physical, high_physical, code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0874u)
        return PR_U3B0_0874_DI_LOAD_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0874u) & 0xFFFFFu] != 0x8Bu ||
        ram[(code_base + 0x0875u) & 0xFFFFFu] != 0x7Eu ||
        ram[(code_base + 0x0876u) & 0xFFFFFu] != 0x06u)
        return PR_U3B0_0874_DI_LOAD_ARGUMENT;

    offset = (uint16_t)(cpu->bp + 6u);
    high_offset = (uint16_t)(offset + 1u);
    base = (uint32_t)cpu->ss << 4;
    low_physical = (base + offset) & 0xFFFFFu;
    high_physical = (base + high_offset) & 0xFFFFFu;
    cpu->di = (uint16_t)((uint16_t)ram[low_physical] |
                         ((uint16_t)ram[high_physical] << 8));
    cpu->ip = 0x0877u;
    return PR_U3B0_0874_DI_LOAD_OK;
}
