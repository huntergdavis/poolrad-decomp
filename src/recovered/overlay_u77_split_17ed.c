/* GAME.OVR unit 0x77:+17ED..+189C. */
#include "poolrad/recovered/overlay_u77_split_17ed.h"

#define RAM_MASK (PR_U77_17ED_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t read16_at(const uint8_t *ram, size_t address)
{
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return read16_at(ram, physical(seg, off));
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

static void push16(uint8_t *ram, pr_u77_17ed_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u77_17ed_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t local8(const uint8_t *ram, const pr_u77_17ed_cpu *cpu,
                      uint16_t distance)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local8(uint8_t *ram, const pr_u77_17ed_cpu *cpu,
                       uint16_t distance, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static uint16_t local16(const uint8_t *ram, const pr_u77_17ed_cpu *cpu,
                        uint16_t distance)
{
    return read16(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local16(uint8_t *ram, const pr_u77_17ed_cpu *cpu,
                        uint16_t distance, uint16_t value)
{
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static void les_far_argument(const uint8_t *ram, pr_u77_17ed_cpu *cpu)
{
    size_t a = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = read16_at(ram, a);
    cpu->es = read16_at(ram, (a + 2u) & RAM_MASK);
}

static void les_local(const uint8_t *ram, pr_u77_17ed_cpu *cpu)
{
    cpu->di = local16(ram, cpu, 4u);
    cpu->es = local16(ram, cpu, 2u);
}

static void set_al(pr_u77_17ed_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void parity(pr_u77_17ed_cpu *cpu, uint8_t result)
{
    result ^= (uint8_t)(result >> 4);
    result ^= (uint8_t)(result >> 2);
    result ^= (uint8_t)(result >> 1);
    if (!(result & 1u)) cpu->flags |= 0x0004u;
}

static void cmp8(pr_u77_17ed_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    parity(cpu, result);
    if (((left ^ right ^ result) & 0x10u) != 0u)
        cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x0800u;
}

static void sub16(pr_u77_17ed_cpu *cpu, uint16_t right)
{
    uint16_t left = cpu->ax;
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    parity(cpu, (uint8_t)result);
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
    cpu->ax = result;
}

static int external_call(uint8_t *ram, size_t ram_size,
                         pr_u77_17ed_cpu *cpu, pr_u77_17ed_external external,
                         void *context, pr_u77_17ed_call call,
                         uint16_t cleanup)
{
    if (!external(context, ram, ram_size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    return 1;
}

pr_u77_17ed_status pr_u77_split_17ed(
    uint8_t *ram, size_t ram_size, pr_u77_17ed_cpu *cpu,
    pr_u77_17ed_external external, void *context)
{
    uint16_t half;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U77_17ED_RAM_SIZE)
        return PR_U77_17ED_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 24u);
    les_far_argument(ram, cpu);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u)));
    cpu->ax &= 0x00FFu;
    cpu->ax >>= 1;
    half = cpu->ax;
    set_local8(ram, cpu, 5u, (uint8_t)half);
    cmp8(cpu, local8(ram, cpu, 5u), 0u);

    if (half != 0u) {
        set_al(cpu, local8(ram, cpu, 5u));
        cpu->ax &= 0x00FFu;
        cpu->dx = cpu->ax;
        les_far_argument(ram, cpu);
        set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u)));
        cpu->ax &= 0x00FFu;
        sub16(cpu, cpu->dx);
        set_local8(ram, cpu, 6u, (uint8_t)cpu->ax);

        cpu->ax = 0x003Fu;
        push16(ram, cpu, cpu->ax);
        if (!external_call(ram, ram_size, cpu, external, context,
                           PR_U77_17ED_ALLOC, 2u))
            return PR_U77_17ED_CALL_ABORT;
        set_local16(ram, cpu, 4u, cpu->ax);
        set_local16(ram, cpu, 2u, cpu->dx);

        les_far_argument(ram, cpu);
        push16(ram, cpu, cpu->es);
        push16(ram, cpu, cpu->di);
        les_local(ram, cpu);
        push16(ram, cpu, cpu->es);
        push16(ram, cpu, cpu->di);
        cpu->ax = 0x003Fu;
        push16(ram, cpu, cpu->ax);
        if (!external_call(ram, ram_size, cpu, external, context,
                           PR_U77_17ED_COPY, 10u))
            return PR_U77_17ED_CALL_ABORT;

        set_al(cpu, local8(ram, cpu, 5u));
        les_local(ram, cpu);
        write8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u), (uint8_t)cpu->ax);
        les_local(ram, cpu);
        write8(ram, cpu->es, (uint16_t)(cpu->di + 0x34u), 0u);
        les_far_argument(ram, cpu);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        les_local(ram, cpu);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au), cpu->ax);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), cpu->dx);
        set_al(cpu, local8(ram, cpu, 6u));
        les_far_argument(ram, cpu);
        write8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u), (uint8_t)cpu->ax);
        cpu->ax = local16(ram, cpu, 4u);
        cpu->dx = local16(ram, cpu, 2u);
        les_far_argument(ram, cpu);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au), cpu->ax);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), cpu->dx);
    } else {
        cpu->di = (uint16_t)(cpu->bp - 0x17u);
        push16(ram, cpu, cpu->ss);
        push16(ram, cpu, cpu->di);
        cpu->di = 0x17DCu;
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, cpu->di);
        if (!external_call(ram, ram_size, cpu, external, context,
                           PR_U77_17ED_STRING, 8u) ||
            !external_call(ram, ram_size, cpu, external, context,
                           PR_U77_17ED_REPORT, 0u))
            return PR_U77_17ED_CALL_ABORT;
    }

    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U77_17ED_OK;
}
