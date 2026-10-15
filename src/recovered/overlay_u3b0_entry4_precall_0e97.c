/* DOS v1.3 GAME.OVR U3B0 entry4 +0E97..+0ED1, 59 original bytes. */
#include "poolrad/recovered/overlay_u3b0_entry4_precall_0e97.h"

#define MASK (PR_U3B0_0DEA_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint8_t even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void les(pr_u3b0_0dea_cpu *cpu, const uint8_t *ram, uint16_t offset)
{
    cpu->di = read16(ram, cpu->ss, offset);
    cpu->es = read16(ram, cpu->ss, (uint16_t)(offset + 2u));
}

static uint8_t cmp16_le_flags(pr_u3b0_0dea_cpu *cpu, uint16_t a,
                              uint16_t b)
{
    uint16_t result = (uint16_t)(a - b);
    uint16_t flags = cpu->flags & (uint16_t)~ARITH_FLAGS;
    uint8_t overflow = (uint8_t)(((a ^ b) & (a ^ result) & 0x8000u) != 0u);
    if (a < b) flags |= 0x0001u;
    if (even_parity((uint8_t)result)) flags |= 0x0004u;
    if (((a ^ b ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (overflow) flags |= 0x0800u;
    cpu->flags = flags;
    return (uint8_t)(result == 0u ||
                     (((result & 0x8000u) != 0u) != (overflow != 0u)));
}

static void inc16_flags(pr_u3b0_0dea_cpu *cpu, uint16_t before,
                        uint16_t result)
{
    uint16_t flags = cpu->flags &
                     (uint16_t)~(ARITH_FLAGS & (uint16_t)~0x0001u);
    if (even_parity((uint8_t)result)) flags |= 0x0004u;
    if (((before ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (before == 0x7FFFu) flags |= 0x0800u;
    cpu->flags = flags;
}

pr_u3b0_0e97_status pr_u3b0_entry4_precall_0e97(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu)
{
    uint16_t before, after;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0DEA_RAM_SIZE ||
        cpu->ip != 0x0E97u)
        return PR_U3B0_0E97_ARGUMENT;

    cpu->ax = read16(ram, cpu->ds, 0x6B29u);
    les(cpu, ram, (uint16_t)(cpu->bp + 0x0Au));
    if (!cmp16_le_flags(cpu, cpu->ax, read16(ram, cpu->es, cpu->di))) {
        les(cpu, ram, (uint16_t)(cpu->bp + 0x0Au));
        cpu->ax = read16(ram, cpu->es, cpu->di);
        write16(ram, cpu->ds, 0x6B29u, cpu->ax);
        les(cpu, ram, (uint16_t)(cpu->bp + 0x0Eu));
        ram[physical(cpu->es, cpu->di)] = 1u;
    }

    cpu->ax = read16(ram, cpu->ds, 0x6B29u);
    if (!cmp16_le_flags(cpu, cpu->ax,
                        read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8Du)))) {
        cpu->ax = 0u;
        cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x0044u);
        write16(ram, cpu->ds, 0x6B29u, cpu->ax);
        les(cpu, ram, (uint16_t)(cpu->bp + 0x0Eu));
        ram[physical(cpu->es, cpu->di)] = 1u;
    }

    les(cpu, ram, (uint16_t)(cpu->bp + 0x0Au));
    before = read16(ram, cpu->es, cpu->di);
    after = (uint16_t)(before + 1u);
    write16(ram, cpu->es, cpu->di, after);
    inc16_flags(cpu, before, after);
    cpu->ax = (uint16_t)(cpu->ax & 0xFF00u); /* MOV AL,0 retains AH. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->ax);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->cs);
    cpu->ip = 0x0ED2u;
    return PR_U3B0_0E97_OK;
}
