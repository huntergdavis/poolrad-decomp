/* GAME.OVR U38 entry 41, +12B2..+12FB, composed with U38 +0C06. */
#include "poolrad/recovered/overlay_u38_effect_match_12b2.h"

#define RAM_MASK (PR_U38_101D_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define INC_DEC_FLAGS 0x08D4u /* INC/DEC preserve CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_101d_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_101d_cpu *cpu)
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

static void result_flags(pr_u38_101d_cpu *cpu, uint8_t result)
{
    if (parity(result)) cpu->flags |= 4u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
}

static void compare8(pr_u38_101d_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 1u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x800u;
    result_flags(cpu, result);
}

static void increment8(uint8_t *ram, pr_u38_101d_cpu *cpu, uint16_t off)
{
    uint8_t before = read8(ram, cpu->ds, off);
    uint8_t result = (uint8_t)(before + 1u);
    cpu->flags &= (uint16_t)~INC_DEC_FLAGS;
    if ((before & 0x0Fu) == 0x0Fu) cpu->flags |= 0x10u;
    if (before == 0x7Fu) cpu->flags |= 0x800u;
    result_flags(cpu, result);
    write8(ram, cpu->ds, off, result);
}

static void decrement8(uint8_t *ram, pr_u38_101d_cpu *cpu, uint16_t off)
{
    uint8_t before = read8(ram, cpu->ds, off);
    uint8_t result = (uint8_t)(before - 1u);
    cpu->flags &= (uint16_t)~INC_DEC_FLAGS;
    if ((before & 0x0Fu) == 0u) cpu->flags |= 0x10u;
    if (before == 0x80u) cpu->flags |= 0x800u;
    result_flags(cpu, result);
    write8(ram, cpu->ds, off, result);
}

/* The complete original 15-byte U38 +0C06 leaf, SHA-256
 * 435bd9fac807d24f064b6bf223817b188e737a5b6ddb9a8b37d5635842a52fe8. */
static void effect_leaf_0c06(uint8_t *ram, pr_u38_101d_cpu *cpu)
{
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    increment8(ram, cpu, 0x6816u);
    increment8(ram, cpu, 0x6822u);
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
}

pr_u38_12b2_status pr_u38_effect_match_12b2(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu)
{
    uint8_t b3, a10e, local;
    if (ram == NULL || cpu == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x12B2u)
        return PR_U38_12B2_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 10u));
    b3 = read8(ram, cpu->es, (uint16_t)(cpu->di + 3u));
    cpu->ax = (uint16_t)((b3 & 0x10u) >> 4);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 12u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 14u));
    a10e = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x10Eu));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | a10e);
    local = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u));
    compare8(cpu, a10e, local);
    if (a10e == local) {
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 14u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 12u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
        push16(ram, cpu, cpu->ax);
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x12EBu);
        cpu->ip = 0x0C06u;
        effect_leaf_0c06(ram, cpu);
    } else {
        decrement8(ram, cpu, 0x6822u);
        decrement8(ram, cpu, 0x6816u);
    }
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_12B2_OK;
}
