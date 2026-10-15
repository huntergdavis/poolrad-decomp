/* GAME.OVR U1C:+0F73 null-head branch and shared +0FFB menu tail. */
#include "poolrad/recovered/overlay_u1c_vertical_menu_null_head_0f73.h"

#define RAM_MASK (PR_U1C_0F08_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

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

static void push16(uint8_t *ram, pr_u1c_0f08_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u1c_0f08_cpu *cpu)
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

static void inc_ax(pr_u1c_0f08_cpu *cpu)
{
    uint16_t before = cpu->ax;
    uint16_t result = (uint16_t)(before + 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & ~1u);
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ 1u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (before == 0x7FFFu) cpu->flags |= 0x800u;
    cpu->ax = result;
}

static void set_al(pr_u1c_0f08_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void clear_ah(pr_u1c_0f08_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x44u;
}

static int external_call(uint8_t *ram, size_t size, pr_u1c_0f08_cpu *cpu,
                         pr_u1c_0f73_external external, void *context,
                         pr_u1c_0f73_call event, uint16_t site,
                         uint16_t next, uint16_t cleanup)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, event)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    cpu->ip = next;
    return 1;
}

pr_u1c_0f73_status pr_u1c_vertical_menu_after_list_0ffb(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context)
{
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0F08_RAM_SIZE || cpu->ip != 0x0FFBu)
        return PR_U1C_0F73_ARGUMENT;
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x010Au));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0108u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u), cpu->dx);
    cpu->ax = 0u;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x44u;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x010Eu), cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 0x010Cu);
    push16(ram, cpu, cpu->ss); push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x010Cu);
    push16(ram, cpu, cpu->ss); push16(ram, cpu, cpu->di);
    set_al(cpu, 0x0Du); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x0Au); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x0Fu); push16(ram, cpu, cpu->ax);
    set_al(cpu, 1u); push16(ram, cpu, cpu->ax);
    set_al(cpu, read8(ram, cpu->ds, 0x5E25u));
    clear_ah(cpu); inc_ax(cpu); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x26u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x16u); push16(ram, cpu, cpu->ax);
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u)));
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u)));
    set_al(cpu, 0u); push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 0x010Fu);
    push16(ram, cpu, cpu->ss); push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x010Eu);
    push16(ram, cpu, cpu->ss); push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x0106u);
    push16(ram, cpu, cpu->ss); push16(ram, cpu, cpu->di);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F73_MENU, 0x1053u, 0x1058u, 40u))
        return PR_U1C_0F73_CALL_ABORT;
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0111u)));
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x010Eu)));
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F73_STORE, 0x1060u, 0x1065u, 4u))
        return PR_U1C_0F73_CALL_ABORT;
    cpu->di = (uint16_t)(cpu->bp - 0x0106u);
    push16(ram, cpu, cpu->ss); push16(ram, cpu, cpu->di);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F73_RELEASE, 0x106Bu, 0x1070u, 4u))
        return PR_U1C_0F73_CALL_ABORT;
    set_al(cpu, 1u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x11u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x26u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x16u); push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F73_CLEANUP, 0x107Cu, 0x1081u, 8u))
        return PR_U1C_0F73_CALL_ABORT;
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    return PR_U1C_0F73_OK;
}

pr_u1c_0f73_status pr_u1c_vertical_menu_null_head_0f73(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context)
{
    pr_u1c_0f73_status status;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0F08_RAM_SIZE || cpu->ip != 0x0F73u)
        return PR_U1C_0F73_ARGUMENT;
    status = pr_u1c_vertical_menu_one_node_0f73(ram, ram_size, cpu,
                                                 external, context);
    if (status != PR_U1C_0F73_BRANCH_STOP) return status;
    if (read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u)) != 0u ||
        read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u)) != 0u)
        return PR_U1C_0F73_BRANCH_STOP;
    cpu->ip = 0x0FFBu; /* original JE at +0FB7 is taken */
    return pr_u1c_vertical_menu_after_list_0ffb(ram, ram_size,
                                                 cpu, external, context);
}
