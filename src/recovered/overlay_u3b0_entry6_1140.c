/* GAME.OVR unit 0x3B0:+1140..+11F2, complete 179-byte entry 6 body. */
#include "poolrad/recovered/overlay_u3b0_entry6_1140.h"

#define RAM_MASK (PR_U3B0_1140_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
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

static void push16(uint8_t *ram, pr_u3b0_1140_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u3b0_1140_cpu *cpu)
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

static void sub_sp_eight(pr_u3b0_1140_cpu *cpu)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - 8u);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < 8u) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 8u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ 8u) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->sp = result;
}

static void xor_ax_ax(pr_u3b0_1140_cpu *cpu)
{
    cpu->ax = 0u;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x0044u; /* ZF, PF */
}

static void cmp16(pr_u3b0_1140_cpu *cpu, uint16_t a, uint16_t b)
{
    uint16_t result = (uint16_t)(a - b);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (a < b) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((a ^ b ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((a ^ b) & (a ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
}

static void inc_local(uint8_t *ram, pr_u3b0_1140_cpu *cpu, uint16_t off)
{
    uint16_t before = read16(ram, cpu->ss, off);
    uint16_t result = (uint16_t)(before + 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & ~1u); /* INC preserves CF */
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 1u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (before == 0x7FFFu) cpu->flags |= 0x800u;
    write16(ram, cpu->ss, off, result);
}

static void les_di(const uint8_t *ram, pr_u3b0_1140_cpu *cpu,
                   uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    uint16_t value_off = (uint16_t)(ram[a] |
        ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    uint16_t value_seg = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
        ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
    cpu->di = value_off;
    cpu->es = value_seg;
}

static int allocate(uint8_t *ram, size_t size, pr_u3b0_1140_cpu *cpu,
                    pr_u3b0_1140_alloc alloc, void *context,
                    uint16_t site, uint16_t next)
{
    cpu->ax = 0x002Eu;
    push16(ram, cpu, cpu->ax);
    cpu->ip = site;
    if (!alloc(context, ram, size, cpu)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = next;
    return 1;
}

pr_u3b0_1140_status pr_u3b0_entry6_1140(
    uint8_t *ram, size_t ram_size, pr_u3b0_1140_cpu *cpu,
    pr_u3b0_1140_alloc alloc, void *context)
{
    uint16_t count;
    if (ram == NULL || cpu == NULL || alloc == NULL ||
        ram_size < PR_U3B0_1140_RAM_SIZE)
        return PR_U3B0_1140_ARGUMENT;
    push16(ram, cpu, cpu->bp);                    /* +1140 */
    cpu->bp = cpu->sp;
    sub_sp_eight(cpu);
    if (!allocate(ram, ram_size, cpu, alloc, context, 0x114Au, 0x114Fu))
        return PR_U3B0_1140_ALLOC_ABORT;
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    write16(ram, cpu->es, cpu->di, cpu->ax);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 2u), cpu->dx);
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    les_di(ram, cpu, cpu->es, cpu->di);
    xor_ax_ax(cpu);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au), cpu->ax);
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), cpu->ax);
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    les_di(ram, cpu, cpu->es, cpu->di);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x29u), 0u);
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    les_di(ram, cpu, cpu->es, cpu->di);
    write8(ram, cpu->es, cpu->di, 0u);
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->ax = read16(ram, cpu->es, cpu->di);
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 2u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0xAu));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    cpu->ax = 2u;
    count = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
    cmp16(cpu, cpu->ax, count);
    if ((int16_t)cpu->ax <= (int16_t)count) {
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
        for (;;) {
            if (!allocate(ram, ram_size, cpu, alloc, context,
                          0x11A8u, 0x11ADu))
                return PR_U3B0_1140_ALLOC_ABORT;
            les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u));
            write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au), cpu->ax);
            write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), cpu->dx);
            les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u));
            cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
            cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
            write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
            write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
            les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u));
            xor_ax_ax(cpu);
            write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au), cpu->ax);
            write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), cpu->ax);
            les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u));
            write8(ram, cpu->es, (uint16_t)(cpu->di + 0x29u), 0u);
            les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u));
            write8(ram, cpu->es, cpu->di, 0u);
            cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
            count = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
            cmp16(cpu, cpu->ax, count);
            if (cpu->ax == count) break;
            inc_local(ram, cpu, (uint16_t)(cpu->bp - 6u));
        }
    }
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_U3B0_1140_OK;
}
