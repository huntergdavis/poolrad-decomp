/* GAME.OVR U9E public entry 17, +110D..+115B (79 original bytes). */
#include "poolrad/recovered/overlay_u9e_entry17_110d.h"

#define RAM_MASK (PR_U9E_110D_RAM_SIZE - 1u)
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

/* LES consumes a contiguous four-byte operand, even across offset FFFF. */
static void les_arg(const uint8_t *ram, pr_u9e_110d_cpu *cpu,
                    uint16_t displacement)
{
    size_t a = physical(cpu->ss, (uint16_t)(cpu->bp + displacement));
    cpu->di = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    cpu->es = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                         ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
}

static uint16_t pop16(const uint8_t *ram, pr_u9e_110d_cpu *cpu)
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

static void dec_ax(pr_u9e_110d_cpu *cpu)
{
    uint16_t before = cpu->ax;
    uint16_t result = (uint16_t)(before - 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & (uint16_t)~1u);
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (before == 0x8000u) cpu->flags |= 0x800u;
    cpu->ax = result;
}

static void sub_ax_64(pr_u9e_110d_cpu *cpu)
{
    uint16_t before = cpu->ax;
    uint16_t result = (uint16_t)(before - 0x64u);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < 0x64u) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 0x64u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ 0x64u) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->ax = result;
}

pr_u9e_110d_status pr_u9e_entry17_110d(
    uint8_t *ram, size_t ram_size, pr_u9e_110d_cpu *cpu)
{
    uint8_t value;
    if (ram == NULL || cpu == NULL || ram_size < PR_U9E_110D_RAM_SIZE ||
        cpu->ip != 0x110Du)
        return PR_U9E_110D_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;

    les_arg(ram, cpu, 6u);
    write8(ram, cpu->es, cpu->di, 0u);
    les_arg(ram, cpu, 14u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->es, (uint16_t)(cpu->di + 3u)));
    cpu->ax &= 0xFF7Fu; /* AND AL,7F; AH is unchanged. */
    les_arg(ram, cpu, 10u);
    write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
    les_arg(ram, cpu, 10u);
    value = read8(ram, cpu->es, cpu->di);
    if (value <= 0x65u) {
        les_arg(ram, cpu, 10u);
        cpu->ax = (uint16_t)read8(ram, cpu->es, cpu->di); /* XOR AH,AH */
        cpu->flags &= (uint16_t)~1u; /* XOR clears CF; DEC preserves it. */
        dec_ax(cpu);
        les_arg(ram, cpu, 6u);
        write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
        les_arg(ram, cpu, 10u);
        write8(ram, cpu->es, cpu->di, 0x12u);
    } else {
        les_arg(ram, cpu, 10u);
        cpu->ax = (uint16_t)read8(ram, cpu->es, cpu->di); /* XOR AH,AH */
        sub_ax_64(cpu);
        les_arg(ram, cpu, 10u);
        write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
    }

    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 12u); /* RETF 12 */
    return PR_U9E_110D_OK;
}
