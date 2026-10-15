/* GAME.OVR U38 public entry 104: 73 reachable bytes, +2BC8..+2C11. */
#include "poolrad/recovered/overlay_u38_link_message_2bc8.h"

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

static void logic16(pr_u38_101d_cpu *cpu, uint16_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x8000u) cpu->flags |= 0x80u;
}

static void compare8(pr_u38_101d_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 1u;
    if (parity(result)) cpu->flags |= 4u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x800u;
}

pr_u38_2bc8_status pr_u38_link_message_2bc8(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_2bc8_dispatch dispatch, void *context)
{
    uint8_t marker;
    if (ram == NULL || cpu == NULL || dispatch == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x2BC8u)
        return PR_U38_2BC8_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 4u);
    cpu->di = read16(ram, cpu->ds, 0x5D92u);
    cpu->es = read16(ram, cpu->ds, 0x5D94u);
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCCu));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCEu));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->ax |= read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        marker = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Eu));
        compare8(cpu, marker, 0x57u);
        if (marker != 0x57u) {
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
            cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
            compare8(cpu, marker, 0x58u);
        }
        if (marker == 0x57u || marker == 0x58u) {
            push16(ram, cpu, read16(ram, cpu->ss,
                                    (uint16_t)(cpu->bp + 0xEu)));
            push16(ram, cpu, read16(ram, cpu->ss,
                                    (uint16_t)(cpu->bp + 0xCu)));
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x32u);
            push16(ram, cpu, cpu->ax);
            push16(ram, cpu, cpu->cs);
            push16(ram, cpu, 0x2C0Bu);
            cpu->ip = 0x0FACu;
            if (!dispatch(context, ram, ram_size, cpu))
                return PR_U38_2BC8_CALL_ABORT;
            cpu->ip = pop16(ram, cpu);
            cpu->cs = pop16(ram, cpu);
            cpu->sp = (uint16_t)(cpu->sp + 6u);
        }
    }
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_2BC8_OK;
}
