/* DOS v1.3 GAME.OVR U3B0 +0067..+00AA, 68 original bytes. */
#include "poolrad/recovered/overlay_u3b0_leaf_0067.h"

#define MASK (PR_U3B0_0067_RAM_SIZE - 1u)
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

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void logical_flags(pr_u3b0_0067_cpu *cpu, uint16_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) cpu->flags |= 0x0080u;
}

static void inc16(pr_u3b0_0067_cpu *cpu, uint16_t before)
{
    uint16_t result = (uint16_t)(before + 1u);
    uint16_t carry = cpu->flags & 0x0001u;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= carry;
    if (parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (((before ^ result) & 0x0010u) != 0u)
        cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) cpu->flags |= 0x0080u;
    if (before == 0x7FFFu) cpu->flags |= 0x0800u;
}

static uint16_t pop16(const uint8_t *ram, pr_u3b0_0067_cpu *cpu)
{
    uint16_t result = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return result;
}

pr_u3b0_0067_status pr_u3b0_leaf_0067(
    uint8_t *ram, size_t ram_size, pr_u3b0_0067_cpu *cpu)
{
    uint16_t offset, segment, count;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0067_RAM_SIZE ||
        cpu->ip != 0x0067u)
        return PR_U3B0_0067_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u); /* PUSH BP */
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    cpu->ax = 0u;                    /* XOR AX,AX */
    logical_flags(cpu, cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->dx);

    for (;;) {
        offset = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
        cpu->ax = offset;
        segment = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        cpu->ax = (uint16_t)(cpu->ax | segment);
        logical_flags(cpu, cpu->ax);
        if (cpu->ax == 0u) break;
        count = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
        inc16(cpu, count);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u),
                (uint16_t)(count + 1u));
        cpu->di = offset;           /* LES DI,[BP-6] */
        cpu->es = segment;
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->dx);
    }

    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->ax);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U3B0_0067_OK;
}
