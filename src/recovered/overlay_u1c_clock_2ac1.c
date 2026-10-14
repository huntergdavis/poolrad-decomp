/* GAME.OVR unit 0x1C:+2AC1..+2AEA, complete 42-byte body. */
#include "poolrad/recovered/overlay_u1c_clock_2ac1.h"

#define RAM_MASK (PR_U1C_2AC1_RAM_SIZE - 1u)
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

static void push16(uint8_t *ram, pr_u1c_2ac1_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u1c_2ac1_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void sub_sp_two(pr_u1c_2ac1_cpu *cpu)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - 2u);
    uint8_t bits = (uint8_t)result;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < 2u) cpu->flags |= 0x0001u;
    bits ^= (uint8_t)(bits >> 4);
    bits ^= (uint8_t)(bits >> 2);
    bits ^= (uint8_t)(bits >> 1);
    if (!(bits & 1u)) cpu->flags |= 0x0004u;
    if (((before ^ 2u ^ result) & 0x10u) != 0u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (((before ^ 2u) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
    cpu->sp = result;
}

static int external_call(uint8_t *ram, size_t ram_size,
                         pr_u1c_2ac1_cpu *cpu,
                         pr_u1c_2ac1_external external, void *context,
                         pr_u1c_2ac1_call call, uint16_t site,
                         uint16_t next, uint16_t cleanup)
{
    cpu->ip = site;
    if (!external(context, ram, ram_size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    cpu->ip = next;
    return 1;
}

pr_u1c_2ac1_status pr_u1c_clock_2ac1(
    uint8_t *ram, size_t ram_size, pr_u1c_2ac1_cpu *cpu,
    pr_u1c_2ac1_external external, void *context)
{
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_2AC1_RAM_SIZE)
        return PR_U1C_2AC1_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub_sp_two(cpu);
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_2AC1_LOADER, 0x2ACAu, 0x2ACFu, 2u))
        return PR_U1C_2AC1_CALL_ABORT;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_2AC1_VALUE, 0x2AD3u, 0x2AD8u, 2u))
        return PR_U1C_2AC1_CALL_ABORT;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_2AC1_CLOCK, 0x2AE2u, 0x2AE7u, 4u))
        return PR_U1C_2AC1_CALL_ABORT;
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U1C_2AC1_OK;
}
