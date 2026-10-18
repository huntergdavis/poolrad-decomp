/* GAME.OVR U9E entry 16, +10E4..+110C (41 original bytes). */
#include "poolrad/recovered/overlay_u9e_entry16_10e4.h"

#define RAM_MASK (PR_U9E_10E4_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop16(const uint8_t *ram, pr_u9e_10e4_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void cmp8_12(pr_u9e_10e4_cpu *cpu, uint8_t value)
{
    uint8_t result = (uint8_t)(value - 0x12u);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (value < 0x12u) cpu->flags |= 1u;
    if (parity(result)) cpu->flags |= 4u;
    if ((value ^ 0x12u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if (((value ^ 0x12u) & (value ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x800u;
}

static void inc16_ax(pr_u9e_10e4_cpu *cpu)
{
    uint16_t before = cpu->ax;
    uint16_t result = (uint16_t)(before + 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & (uint16_t)~1u);
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (before == 0x7FFFu) cpu->flags |= 0x800u;
    cpu->ax = result;
}

pr_u9e_10e4_status pr_u9e_entry16_10e4(
    uint8_t *ram, size_t ram_size, pr_u9e_10e4_cpu *cpu)
{
    uint8_t second;
    if (ram == NULL || cpu == NULL || ram_size < PR_U9E_10E4_RAM_SIZE ||
        cpu->ip != 0x10E4u)
        return PR_U9E_10E4_ARGUMENT;

    /* PUSH BP; MOV BP,SP; SUB SP,2. Their interim flags are overwritten. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);

    second = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->ax = (uint16_t)((uint16_t)second + 0x64u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    cmp8_12(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    if (second == 0x12u) {
        cpu->ax = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
        inc16_ax(cpu);
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    }
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u); /* RETF 4 */
    return PR_U9E_10E4_OK;
}
