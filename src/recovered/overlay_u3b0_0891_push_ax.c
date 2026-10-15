/* DOS v1.3 GAME.OVR U3B0 +0891: 50 (PUSH AX). */
#include "poolrad/recovered/overlay_u3b0_0891_push_ax.h"

pr_u3b0_0891_push_ax_status pr_u3b0_0891_push_ax(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t new_sp, high_offset;
    uint32_t code_base, stack_base, low_physical, high_physical;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0891u)
        return PR_U3B0_0891_PUSH_AX_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0891u) & 0xFFFFFu] != 0x50u)
        return PR_U3B0_0891_PUSH_AX_ARGUMENT;

    new_sp = (uint16_t)(cpu->sp - 2u);
    high_offset = (uint16_t)(new_sp + 1u);
    stack_base = (uint32_t)cpu->ss << 4;
    low_physical = (stack_base + new_sp) & 0xFFFFFu;
    high_physical = (stack_base + high_offset) & 0xFFFFFu;
    ram[low_physical] = (uint8_t)cpu->ax;
    ram[high_physical] = (uint8_t)(cpu->ax >> 8);
    cpu->sp = new_sp;
    cpu->ip = 0x0892u;
    return PR_U3B0_0891_PUSH_AX_OK;
}
