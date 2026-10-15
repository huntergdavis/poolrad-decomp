/* GAME.OVR U38 public entry 35, complete +1086..+10C1 (59 bytes). */
#include "poolrad/recovered/overlay_u38_target_dispatch_1086.h"

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

pr_u38_1086_status pr_u38_target_dispatch_1086(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, pr_u38_1086_dispatch dispatch,
    void *context)
{
    uint8_t marker;
    if (ram == NULL || cpu == NULL || select_target == NULL ||
        dispatch == NULL || ram_size < PR_U38_101D_RAM_SIZE ||
        cpu->ip != 0x1086u)
        return PR_U38_1086_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D94u));
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D92u));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x1095u);
    cpu->ip = 0x101Du;
    if (pr_u38_target_select_101d(ram, ram_size, cpu,
                                    select_target, context) != PR_U38_101D_OK)
        return PR_U38_1086_SELECT_ABORT;
    cpu->ax |= cpu->dx;
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        push16(ram, cpu, read16(ram, cpu->ds, 0x5D94u));
        push16(ram, cpu, read16(ram, cpu->ds, 0x5D92u));
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x10A5u);
        cpu->ip = 0x101Du;
        if (pr_u38_target_select_101d(ram, ram_size, cpu,
                                        select_target, context) != PR_U38_101D_OK)
            return PR_U38_1086_SELECT_ABORT;
        cpu->di = cpu->ax;
        cpu->es = cpu->dx;
        marker = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x32u));
        compare8_zero(cpu, marker);
        if (marker == 0u) {
            push16(ram, cpu, read16(ram, cpu->ss,
                                    (uint16_t)(cpu->bp + 0xEu)));
            push16(ram, cpu, read16(ram, cpu->ss,
                                    (uint16_t)(cpu->bp + 0xCu)));
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x64u);
            push16(ram, cpu, cpu->ax);
            push16(ram, cpu, cpu->cs);
            push16(ram, cpu, 0x10BDu);
            cpu->ip = 0x0FACu;
            if (!dispatch(context, ram, ram_size, cpu))
                return PR_U38_1086_DISPATCH_ABORT;
            cpu->ip = pop16(ram, cpu);
            cpu->cs = pop16(ram, cpu);
            cpu->sp = (uint16_t)(cpu->sp + 6u);
        }
    }
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_1086_OK;
}
