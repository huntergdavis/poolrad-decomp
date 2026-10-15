/* DOS v1.3 GAME.OVR U3B0 +088F: 03 C2 (ADD AX,DX). */
#include "poolrad/recovered/overlay_u3b0_088f_add_ax_dx.h"

#define ADD_DEFINED_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define XOR_DEFINED_FLAGS 0x08C5u /* +0883 XOR; AF undefined */

pr_u3b0_088f_add_ax_dx_status pr_u3b0_088f_add_ax_dx(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t ax, dx, result, flags;
    uint32_t code_base, sum;
    unsigned parity, bit;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x088Fu || (cpu->flags & XOR_DEFINED_FLAGS) != 0x0044u ||
        (cpu->ax & 0xFF00u) != ((cpu->ax & 0x0080u) != 0u ? 0xFF00u : 0u))
        return PR_U3B0_088F_ADD_AX_DX_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x088Fu) & 0xFFFFFu] != 0x03u ||
        ram[(code_base + 0x0890u) & 0xFFFFFu] != 0xC2u)
        return PR_U3B0_088F_ADD_AX_DX_ARGUMENT;

    ax = cpu->ax;
    dx = cpu->dx;
    sum = (uint32_t)ax + dx;
    result = (uint16_t)sum;
    flags = (uint16_t)(cpu->flags & (uint16_t)~ADD_DEFINED_FLAGS);
    if (sum > 0xFFFFu)
        flags |= 0x0001u;
    if (((~(ax ^ dx)) & (ax ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    if ((result & 0x8000u) != 0u)
        flags |= 0x0080u;
    if (result == 0u)
        flags |= 0x0040u;
    if (((ax ^ dx ^ result) & 0x0010u) != 0u)
        flags |= 0x0010u;
    parity = 0u;
    for (bit = 0u; bit < 8u; ++bit)
        parity ^= ((unsigned)result >> bit) & 1u;
    if (parity == 0u)
        flags |= 0x0004u;

    cpu->ax = result;
    cpu->flags = flags;
    cpu->ip = 0x0891u;
    return PR_U3B0_088F_ADD_AX_DX_OK;
}
