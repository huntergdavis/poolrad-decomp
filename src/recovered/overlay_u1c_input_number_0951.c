/* GAME.OVR unit 0x1C:+0951..+0991, complete 65-byte INPUT NUMBER. */
#include "poolrad/recovered/overlay_u1c_input_number_0951.h"

#define RAM_MASK (PR_U1C_0951_RAM_SIZE - 1u)
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

static void push16(uint8_t *ram, pr_u1c_0951_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u1c_0951_cpu *cpu)
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

static void sub_sp_six(pr_u1c_0951_cpu *cpu)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - 6u);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < 6u) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 6u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ 6u) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->sp = result;
}

static void set_al(pr_u1c_0951_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static int external_call(uint8_t *ram, size_t size, pr_u1c_0951_cpu *cpu,
                         pr_u1c_0951_external external, void *context,
                         pr_u1c_0951_call call, uint16_t site,
                         uint16_t next, uint16_t cleanup)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    cpu->ip = next;
    return 1;
}

pr_u1c_0951_status pr_u1c_input_number_0951(
    uint8_t *ram, size_t ram_size, pr_u1c_0951_cpu *cpu,
    pr_u1c_0951_external external, void *context)
{
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0951_RAM_SIZE)
        return PR_U1C_0951_ARGUMENT;
    push16(ram, cpu, cpu->bp);                           /* +0951 */
    cpu->bp = cpu->sp;
    sub_sp_six(cpu);
    set_al(cpu, 2u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0951_LOADER, 0x095Au, 0x095Fu, 2u))
        return PR_U1C_0951_CALL_ABORT;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);
    set_al(cpu, read8(ram, cpu->ds, 0x7087u));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, read8(ram, cpu->ds, 0x7047u));
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0951_WORD, 0x096Bu, 0x0970u, 4u))
        return PR_U1C_0951_CALL_ABORT;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 2u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    set_al(cpu, 10u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0951_INPUT, 0x097Bu, 0x0980u, 6u))
        return PR_U1C_0951_CALL_ABORT;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u)));
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u)));
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0951_STORE, 0x0989u, 0x098Eu, 4u))
        return PR_U1C_0951_CALL_ABORT;
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    return PR_U1C_0951_OK;
}
