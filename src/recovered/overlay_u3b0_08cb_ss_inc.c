/* DOS v1.3 GAME.OVR U3B0 +08CB: FE 46 F7. */
#include "poolrad/recovered/overlay_u3b0_08cb_ss_inc.h"

#define INC_DEFINED_FLAGS 0x08D4u /* OF SF ZF AF PF; CF preserved */

pr_u3b0_08cb_ss_inc_status pr_u3b0_08cb_ss_inc(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset, flags;
    uint32_t code_base, physical;
    uint8_t old_value, result;
    unsigned parity, bit;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x08CBu || (cpu->flags & 0x0001u) != 0u)
        return PR_U3B0_08CB_SS_INC_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x08CBu) & 0xFFFFFu] != 0xFEu ||
        ram[(code_base + 0x08CCu) & 0xFFFFFu] != 0x46u ||
        ram[(code_base + 0x08CDu) & 0xFFFFFu] != 0xF7u)
        return PR_U3B0_08CB_SS_INC_ARGUMENT;

    offset = (uint16_t)(cpu->bp - 9u);
    physical = (((uint32_t)cpu->ss << 4) + offset) & 0xFFFFFu;
    old_value = ram[physical];
    result = (uint8_t)(old_value + 1u);
    flags = (uint16_t)(cpu->flags & (uint16_t)~INC_DEFINED_FLAGS);
    if (old_value == 0x7Fu)
        flags |= 0x0800u;
    if ((result & 0x80u) != 0u)
        flags |= 0x0080u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((old_value & 0x0Fu) == 0x0Fu)
        flags |= 0x0010u;
    parity = 0u;
    for (bit = 0u; bit < 8u; ++bit)
        parity ^= ((unsigned)result >> bit) & 1u;
    if (parity == 0u)
        flags |= 0x0004u;

    ram[physical] = result;
    cpu->flags = flags;
    cpu->ip = 0x08CEu;
    return PR_U3B0_08CB_SS_INC_OK;
}
