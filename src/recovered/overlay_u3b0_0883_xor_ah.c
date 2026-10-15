/* DOS v1.3 GAME.OVR U3B0 +0883: 30 E4. AF is undefined. */
#include "poolrad/recovered/overlay_u3b0_0883_xor_ah.h"

#define DEFINED_LOGIC_FLAGS 0x08C5u /* OF SF ZF PF CF; excludes undefined AF */

pr_u3b0_0883_xor_ah_status pr_u3b0_0883_xor_ah(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint32_t code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0883u || (cpu->flags & 0x0001u) == 0u)
        return PR_U3B0_0883_XOR_AH_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0883u) & 0xFFFFFu] != 0x30u ||
        ram[(code_base + 0x0884u) & 0xFFFFFu] != 0xE4u)
        return PR_U3B0_0883_XOR_AH_ARGUMENT;

    cpu->ax &= 0x00FFu;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~DEFINED_LOGIC_FLAGS) |
                            0x0044u); /* ZF/PF; preserve undefined AF as placeholder */
    cpu->ip = 0x0885u;
    return PR_U3B0_0883_XOR_AH_OK;
}
