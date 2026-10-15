/* DOS v1.3 GAME.OVR U3B0 +0896: B0 00 (MOV AL,0). */
#include "poolrad/recovered/overlay_u3b0_0896_mov_al_zero.h"

pr_u3b0_0896_mov_al_zero_status pr_u3b0_0896_mov_al_zero(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint32_t code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0896u)
        return PR_U3B0_0896_MOV_AL_ZERO_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0896u) & 0xFFFFFu] != 0xB0u ||
        ram[(code_base + 0x0897u) & 0xFFFFFu] != 0x00u)
        return PR_U3B0_0896_MOV_AL_ZERO_ARGUMENT;

    cpu->ax &= 0xFF00u;
    cpu->ip = 0x0898u;
    return PR_U3B0_0896_MOV_AL_ZERO_OK;
}
