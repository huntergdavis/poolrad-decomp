/* DOS v1.3 GAME.OVR U3B0 +088E: 98 (CBW). */
#include "poolrad/recovered/overlay_u3b0_088e_cbw.h"

#define DEFINED_XOR_FLAGS 0x08C5u /* OF SF ZF PF CF; AF undefined */

pr_u3b0_088e_cbw_status pr_u3b0_088e_cbw(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint32_t code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x088Eu || (cpu->ax & 0xFF00u) != 0u ||
        (cpu->flags & DEFINED_XOR_FLAGS) != 0x0044u)
        return PR_U3B0_088E_CBW_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x088Eu) & 0xFFFFFu] != 0x98u)
        return PR_U3B0_088E_CBW_ARGUMENT;

    cpu->ax = (uint16_t)((cpu->ax & 0x00FFu) |
                         ((cpu->ax & 0x0080u) != 0u ? 0xFF00u : 0u));
    cpu->ip = 0x088Fu;
    return PR_U3B0_088E_CBW_OK;
}
