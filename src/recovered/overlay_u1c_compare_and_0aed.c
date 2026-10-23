/* GAME.OVR unit 0x1C:+0AED..+0B51, complete 101-byte COMPARE AND body. */
#include "poolrad/recovered/overlay_u1c_compare_and_0aed.h"

#define RAM_MASK (PR_U1C_0AED_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

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

static void push16(uint8_t *ram, pr_u1c_0aed_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u1c_0aed_cpu *cpu)
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

static void result_flags(pr_u1c_0aed_cpu *cpu, uint16_t result, unsigned width)
{
    uint16_t sign = width == 8u ? 0x80u : 0x8000u;
    if (parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void sub_flags(pr_u1c_0aed_cpu *cpu, uint16_t a,
                      uint16_t b, unsigned width, int keep_carry)
{
    uint16_t mask = width == 8u ? 0x00FFu : 0xFFFFu;
    uint16_t sign = width == 8u ? 0x0080u : 0x8000u;
    uint16_t result = (uint16_t)((a - b) & mask);
    uint16_t carry = cpu->flags & 1u;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if ((a & mask) < (b & mask)) cpu->flags |= 1u;
    if ((a ^ b ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (((a ^ b) & (a ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    result_flags(cpu, result, width);
    if (keep_carry) cpu->flags = (uint16_t)((cpu->flags & ~1u) | carry);
}

static void xor_ah_zero(pr_u1c_0aed_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    /* XOR clears CF/OF; pinned FillFlags also clears AF. */
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    result_flags(cpu, 0u, 8u);
}

static void shift_di_one(pr_u1c_0aed_cpu *cpu)
{
    uint16_t old = cpu->di;
    cpu->di = (uint16_t)(old << 1);
    /* pinned DOSBox-X: SHL sets AF for a non-zero count. */
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x10u;
    if (old & 0x8000u) cpu->flags |= 1u;
    if (((cpu->di >> 15) ^ (old >> 15)) & 1u) cpu->flags |= 0x800u;
    result_flags(cpu, cpu->di, 16u);
}

static void inc_byte_flags(pr_u1c_0aed_cpu *cpu, uint8_t old)
{
    uint16_t carry = cpu->flags & 1u;
    uint8_t result = (uint8_t)(old + 1u);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= carry;
    if ((old & 0x0Fu) == 0x0Fu) cpu->flags |= 0x10u;
    if (old == 0x7Fu) cpu->flags |= 0x800u;
    result_flags(cpu, result, 8u);
}

static void set_al(pr_u1c_0aed_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static int external_call(uint8_t *ram, size_t size, pr_u1c_0aed_cpu *cpu,
                         pr_u1c_0aed_external external, void *context,
                         pr_u1c_0aed_call call, uint16_t site,
                         uint16_t next, uint16_t cleanup)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    cpu->ip = next;
    return 1;
}

pr_u1c_0aed_status pr_u1c_compare_and_0aed(
    uint8_t *ram, size_t ram_size, pr_u1c_0aed_cpu *cpu,
    pr_u1c_0aed_external external, void *context)
{
    unsigned iterations;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0AED_RAM_SIZE)
        return PR_U1C_0AED_ARGUMENT;

    push16(ram, cpu, cpu->bp);                             /* +0AED */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 10u, 16u, 0);
    cpu->sp = (uint16_t)(cpu->sp - 10u);
    cpu->di = 0x6F78u;
    push16(ram, cpu, cpu->ds);
    push16(ram, cpu, cpu->di);
    cpu->ax = 6u;
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 0u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0AED_FILL, 0x0AFFu, 0x0B04u, 8u))
        return PR_U1C_0AED_CALL_ABORT;
    set_al(cpu, 4u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0AED_LOADER, 0x0B07u, 0x0B0Cu, 2u))
        return PR_U1C_0AED_CALL_ABORT;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u), 1u);
    for (iterations = 0u; iterations < 256u; ++iterations) {
        uint8_t index;
        uint16_t at;
        if (iterations != 0u) {                            /* +0B12 */
            at = (uint16_t)(cpu->bp - 9u);
            index = read8(ram, cpu->ss, at);
            inc_byte_flags(cpu, index);
            write8(ram, cpu->ss, at, (uint8_t)(index + 1u));
        }
        index = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u));
        set_al(cpu, index);
        push16(ram, cpu, cpu->ax);
        if (!external_call(ram, ram_size, cpu, external, context,
                           PR_U1C_0AED_VALUE, 0x0B19u, 0x0B1Eu, 2u))
            return PR_U1C_0AED_CALL_ABORT;
        cpu->dx = cpu->ax;
        set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u)));
        xor_ah_zero(cpu);
        cpu->di = cpu->ax;
        shift_di_one(cpu);
        at = (uint16_t)(cpu->bp + cpu->di - 10u);
        write16(ram, cpu->ss, at, cpu->dx);
        index = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u));
        sub_flags(cpu, index, 4u, 8u, 0);
        if (index == 4u) break;
    }
    if (iterations == 256u) return PR_U1C_0AED_LOOP_LIMIT;
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
    sub_flags(cpu, cpu->ax,
              read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u)), 16u, 0);
    if ((cpu->flags & 0x40u) == 0u) {
        write8(ram, cpu->ds, 0x6F79u, 1u);
    } else {
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        sub_flags(cpu, cpu->ax,
                  read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)), 16u, 0);
        write8(ram, cpu->ds,
               (uint16_t)((cpu->flags & 0x40u) ? 0x6F78u : 0x6F79u), 1u);
    }
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    return PR_U1C_0AED_OK;
}
