/* DOS v1.3 GAME.OVR U3B0 +0877: 36 3A 45 AC. */
#include "poolrad/recovered/overlay_u3b0_0877_cmp_al.h"

pr_u3b0_0877_cmp_al_status pr_u3b0_0877_cmp_al(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset, new_flags;
    uint32_t physical, code_base;
    uint8_t al, source, result, parity;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0877u)
        return PR_U3B0_0877_CMP_AL_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0877u) & 0xFFFFFu] != 0x36u ||
        ram[(code_base + 0x0878u) & 0xFFFFFu] != 0x3Au ||
        ram[(code_base + 0x0879u) & 0xFFFFFu] != 0x45u ||
        ram[(code_base + 0x087Au) & 0xFFFFFu] != 0xACu)
        return PR_U3B0_0877_CMP_AL_ARGUMENT;

    offset = (uint16_t)(cpu->di - 0x54u);
    physical = (((uint32_t)cpu->ss << 4) + offset) & 0xFFFFFu;
    al = (uint8_t)cpu->ax;
    source = ram[physical];
    result = (uint8_t)(al - source);
    parity = result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    new_flags = (uint16_t)(cpu->flags & (uint16_t)~0x08D5u);
    if (al < source)
        new_flags |= 0x0001u; /* CF */
    if ((parity & 1u) == 0u)
        new_flags |= 0x0004u; /* PF */
    if (((al ^ source ^ result) & 0x10u) != 0u)
        new_flags |= 0x0010u; /* AF */
    if (result == 0u)
        new_flags |= 0x0040u; /* ZF */
    if ((result & 0x80u) != 0u)
        new_flags |= 0x0080u; /* SF */
    if (((al ^ source) & (al ^ result) & 0x80u) != 0u)
        new_flags |= 0x0800u; /* OF */
    cpu->flags = new_flags;
    cpu->ip = 0x087Bu;
    return PR_U3B0_0877_CMP_AL_OK;
}
