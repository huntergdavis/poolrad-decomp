/* GAME.OVR U38 near-called helper, +101D..+1085 (105 original bytes). */
#include "poolrad/recovered/overlay_u38_target_select_101d.h"

#define RAM_MASK (PR_U38_101D_RAM_SIZE - 1u)
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

static void write16(uint8_t *ram, uint16_t seg, uint16_t off,
                    uint16_t value)
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

static void logic16(pr_u38_101d_cpu *cpu, uint16_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x8000u) cpu->flags |= 0x80u;
}

static void compare8_zero(pr_u38_101d_cpu *cpu, uint8_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity(value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x80u) cpu->flags |= 0x80u;
}

static void sub16_sp(pr_u38_101d_cpu *cpu, uint16_t value)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - value);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < value) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ value ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ value) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->sp = result;
}

pr_u38_101d_status pr_u38_target_select_101d(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context)
{
    uint16_t off, seg;
    if (ram == NULL || cpu == NULL || select_target == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x101Du)
        return PR_U38_101D_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 10u);
    cpu->ax = 0u; /* XOR AX,AX */
    logic16(cpu, 0u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCCu));
    cpu->ax |= read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCEu));
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
        cpu->di = (uint16_t)(cpu->bp - 8u);
        push16(ram, cpu, cpu->ss);
        push16(ram, cpu, cpu->di);
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x104Au);
        cpu->cs = read16(ram, cpu->cs, 0x1048u);
        cpu->ip = 0x2E8Bu;
        if (!select_target(context, ram, ram_size, cpu))
            return PR_U38_101D_CALL_ABORT;
        cpu->ip = pop16(ram, cpu);
        cpu->cs = pop16(ram, cpu);
        cpu->sp = (uint16_t)(cpu->sp + 8u); /* BA:2E8B RETF 8 */
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u),
               (uint8_t)cpu->ax);
        compare8_zero(cpu, read8(ram, cpu->ss,
                                 (uint16_t)(cpu->bp - 9u)));
        if (read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u)) != 0u) {
            cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
            cpu->ax |= read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
            logic16(cpu, cpu->ax);
            if (cpu->ax != 0u) goto result;
        }
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
        cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
        off = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCCu));
        seg = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCEu));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), off);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), seg);
    }
result:
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U38_101D_OK;
}
