/* DOS v1.3 GAME.OVR U3B0 +089F: 30 E4. AF is undefined. */
#include "poolrad/recovered/overlay_u3b0_089f_xor_ah.h"

#define DEFINED_LOGIC_FLAGS 0x08D5u /* OF SF ZF AF PF CF: pinned XOR clears AF (flags.cpp:684-691) */

pr_u3b0_089f_xor_ah_status pr_u3b0_089f_xor_ah(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint32_t code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x089Fu)
        return PR_U3B0_089F_XOR_AH_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x089Fu) & 0xFFFFFu] != 0x30u ||
        ram[(code_base + 0x08A0u) & 0xFFFFFu] != 0xE4u)
        return PR_U3B0_089F_XOR_AH_ARGUMENT;

    cpu->ax &= 0x00FFu;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~DEFINED_LOGIC_FLAGS) |
                            0x0044u); /* ZF/PF; preserve undefined AF as placeholder */
    cpu->ip = 0x08A1u;
    return PR_U3B0_089F_XOR_AH_OK;
}
