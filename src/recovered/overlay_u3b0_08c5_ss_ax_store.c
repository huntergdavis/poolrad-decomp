/* DOS v1.3 GAME.OVR U3B0 +08C5: 89 46 FC. */
#include "poolrad/recovered/overlay_u3b0_08c5_ss_ax_store.h"

pr_u3b0_08c5_ss_ax_store_status pr_u3b0_08c5_ss_ax_store(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset, high_offset;
    uint32_t code_base, base, low_physical, high_physical;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x08C5u || (cpu->flags & 0x0001u) != 0u)
        return PR_U3B0_08C5_SS_AX_STORE_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x08C5u) & 0xFFFFFu] != 0x89u ||
        ram[(code_base + 0x08C6u) & 0xFFFFFu] != 0x46u ||
        ram[(code_base + 0x08C7u) & 0xFFFFFu] != 0xFCu)
        return PR_U3B0_08C5_SS_AX_STORE_ARGUMENT;

    offset = (uint16_t)(cpu->bp - 4u);
    high_offset = (uint16_t)(offset + 1u);
    base = (uint32_t)cpu->ss << 4;
    low_physical = (base + offset) & 0xFFFFFu;
    high_physical = (base + high_offset) & 0xFFFFFu;
    ram[low_physical] = (uint8_t)cpu->ax;
    ram[high_physical] = (uint8_t)(cpu->ax >> 8);
    cpu->ip = 0x08C8u;
    return PR_U3B0_08C5_SS_AX_STORE_OK;
}
