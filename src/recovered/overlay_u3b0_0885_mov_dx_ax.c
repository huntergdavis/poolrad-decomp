/* DOS v1.3 GAME.OVR U3B0 +0885: 8B D0. */
#include "poolrad/recovered/overlay_u3b0_0885_mov_dx_ax.h"

#define DEFINED_XOR_FLAGS 0x08C5u /* OF SF ZF PF CF; AF undefined */

pr_u3b0_0885_mov_dx_ax_status pr_u3b0_0885_mov_dx_ax(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint32_t code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0885u ||
        (cpu->ax & 0xFF00u) != 0u ||
        (cpu->flags & DEFINED_XOR_FLAGS) != 0x0044u)
        return PR_U3B0_0885_MOV_DX_AX_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0885u) & 0xFFFFFu] != 0x8Bu ||
        ram[(code_base + 0x0886u) & 0xFFFFFu] != 0xD0u)
        return PR_U3B0_0885_MOV_DX_AX_ARGUMENT;

    cpu->dx = cpu->ax;
    cpu->ip = 0x0887u;
    return PR_U3B0_0885_MOV_DX_AX_OK;
}
