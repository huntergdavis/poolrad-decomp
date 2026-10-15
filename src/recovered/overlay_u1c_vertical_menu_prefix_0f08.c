/* GAME.OVR unit 0x1C:+0F08..+0F72, bounded VERTICAL MENU prefix. */
#include "poolrad/recovered/overlay_u1c_vertical_menu_prefix_0f08.h"

#define RAM_MASK (PR_U1C_0F08_RAM_SIZE - 1u)
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

static void push16(uint8_t *ram, pr_u1c_0f08_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void sub_sp_112(pr_u1c_0f08_cpu *cpu)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - 0x0112u);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < 0x0112u) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 0x0112u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ 0x0112u) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->sp = result;
}

static void dec_word(uint8_t *ram, pr_u1c_0f08_cpu *cpu, uint16_t off)
{
    uint16_t before = read16(ram, cpu->ds, off);
    uint16_t result = (uint16_t)(before - 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & ~1u); /* DEC preserves CF */
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 1u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (before == 0x8000u) cpu->flags |= 0x800u;
    write16(ram, cpu->ds, off, result);
}

static void set_al(pr_u1c_0f08_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static int external_call(uint8_t *ram, size_t size, pr_u1c_0f08_cpu *cpu,
                         pr_u1c_0f08_external external, void *context,
                         pr_u1c_0f08_call call, uint16_t site,
                         uint16_t next, uint16_t cleanup)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    cpu->ip = next;
    return 1;
}

pr_u1c_0f08_status pr_u1c_vertical_menu_prefix_0f08(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f08_external external, void *context)
{
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0F08_RAM_SIZE)
        return PR_U1C_0F08_ARGUMENT;
    push16(ram, cpu, cpu->bp);                            /* +0F08 */
    cpu->bp = cpu->sp;
    sub_sp_112(cpu);
    write8(ram, cpu->ds, 0x84E4u, 0u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x010Fu), 1u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x010Cu), 0u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);
    set_al(cpu, 3u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F08_LOADER, 0x0F25u, 0x0F2Au, 2u))
        return PR_U1C_0F08_CALL_ABORT;
    set_al(cpu, read8(ram, cpu->ds, 0x7086u));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, read8(ram, cpu->ds, 0x7046u));
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F08_WORD, 0x0F32u, 0x0F37u, 4u))
        return PR_U1C_0F08_CALL_ABORT;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0111u), cpu->ax);
    cpu->di = 0x70C8u;
    push16(ram, cpu, cpu->ds);
    push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x0102u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x00FFu;
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F08_COPY, 0x0F4Au, 0x0F4Fu, 10u))
        return PR_U1C_0F08_CALL_ABORT;
    set_al(cpu, 3u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F08_VALUE, 0x0F52u, 0x0F57u, 2u))
        return PR_U1C_0F08_CALL_ABORT;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    dec_word(ram, cpu, 0x49EDu);
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F08_LOADER, 0x0F62u, 0x0F67u, 2u))
        return PR_U1C_0F08_CALL_ABORT;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->ax &= 0x00FFu; /* XOR AH, AH */
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x0044u; /* zero, even parity */
    push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 0x0106u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ip = 0x0F73u;
    return PR_U1C_0F08_PREFIX_DONE;
}
