/* DOS v1.3 GAME.OVR U3B0 entry4 +0DEA..+0DFE, 21 original bytes. */
#include "poolrad/recovered/overlay_u3b0_entry4_nonnull_0dea.h"

#define MASK (PR_U3B0_0DEA_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint8_t even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void set_al(pr_u3b0_0dea_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t sign_extend(uint8_t value)
{
    return (value & 0x80u) != 0u ? (uint16_t)(0xFF00u | value) : value;
}

static void sub16_flags(pr_u3b0_0dea_cpu *cpu, uint16_t left,
                        uint16_t right, uint16_t result)
{
    uint16_t flags = cpu->flags & (uint16_t)~ARITH_FLAGS;
    if (left < right) flags |= 0x0001u;
    if (even_parity((uint8_t)result)) flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

static void inc16_flags(pr_u3b0_0dea_cpu *cpu, uint16_t before,
                        uint16_t result)
{
    uint16_t flags = cpu->flags & (uint16_t)~(ARITH_FLAGS & ~0x0001u);
    if (even_parity((uint8_t)result)) flags |= 0x0004u;
    if (((before ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (before == 0x7FFFu) flags |= 0x0800u;
    cpu->flags = flags;
}

pr_u3b0_0dea_status pr_u3b0_entry4_nonnull_0dea(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu)
{
    uint16_t before;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0DEA_RAM_SIZE ||
        cpu->ip != 0x0DEAu)
        return PR_U3B0_0DEA_ARGUMENT;

    write8(ram, cpu->ds, 0x6B2Bu, 1u); /* +0DEA MOV byte [6B2B],1 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x1Eu)));
    cpu->ax = sign_extend((uint8_t)cpu->ax);
    cpu->dx = cpu->ax;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x1Au)));
    cpu->ax = sign_extend((uint8_t)cpu->ax);
    before = cpu->ax;
    cpu->ax = (uint16_t)(before - cpu->dx);
    sub16_flags(cpu, before, cpu->dx, cpu->ax);
    before = cpu->ax;
    cpu->ax = (uint16_t)(before + 1u);
    inc16_flags(cpu, before, cpu->ax);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x54u),
           (uint8_t)cpu->ax);
    cpu->ip = 0x0DFFu;
    return PR_U3B0_0DEA_OK;
}
