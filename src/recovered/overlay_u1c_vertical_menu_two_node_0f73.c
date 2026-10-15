/* GAME.OVR U1C:+0F73, exactly two linked menu nodes. */
#include "poolrad/recovered/overlay_u1c_vertical_menu_two_node_0f73.h"

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

static void logic16(pr_u1c_0f08_cpu *cpu, uint16_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x8000u) cpu->flags |= 0x80u;
}

static void add_di(pr_u1c_0f08_cpu *cpu, uint16_t value)
{
    uint16_t before = cpu->di;
    uint16_t result = (uint16_t)(before + value);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if ((uint32_t)before + value > 0xFFFFu) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ value ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((~(before ^ value)) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->di = result;
}

static void inc_byte(uint8_t *ram, pr_u1c_0f08_cpu *cpu, uint16_t off)
{
    uint8_t before = read8(ram, cpu->ss, off);
    uint8_t result = (uint8_t)(before + 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & ~1u);
    if (parity(result)) cpu->flags |= 4u;
    if ((before ^ 1u ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if (before == 0x7Fu) cpu->flags |= 0x800u;
    write8(ram, cpu->ss, off, result);
}

static void les_di(const uint8_t *ram, pr_u1c_0f08_cpu *cpu,
                   uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    cpu->di = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    cpu->es = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                         ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
}

pr_u1c_0f73_status pr_u1c_vertical_menu_two_node_0f73(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context)
{
    pr_u1c_0f73_status status;
    uint16_t link_off, link_seg;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0F08_RAM_SIZE || cpu->ip != 0x0F73u)
        return PR_U1C_0F73_ARGUMENT;
    status = pr_u1c_vertical_menu_one_node_0f73(ram, ram_size, cpu,
                                                 external, context);
    if (status != PR_U1C_0F73_BRANCH_STOP) return status;
    link_off = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    link_seg = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u));
    if ((link_off | link_seg) == 0u) return PR_U1C_0F73_BRANCH_STOP;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    clear_ah(cpu);
    cpu->di = cpu->ax;
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 8u);
    cpu->di = (uint16_t)(cpu->di << 8);
    add_di(cpu, 0x6FC8u);
    push16(ram, cpu, cpu->ds); push16(ram, cpu, cpu->di);
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    push16(ram, cpu, cpu->es); push16(ram, cpu, cpu->di);
    cpu->ax = 0x0028u; push16(ram, cpu, cpu->ax);
    cpu->ip = 0x0FD4u;
    if (!external(context, ram, ram_size, cpu, PR_U1C_0F73_COPY))
        return PR_U1C_0F73_CALL_ABORT;
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    cpu->ip = 0x0FD9u;
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x29u), 0u);
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u), cpu->dx);
    inc_byte(ram, cpu, (uint16_t)(cpu->bp - 2u));
    cpu->ax = (uint16_t)(cpu->ax | cpu->dx);
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        cpu->ip = 0x0FB7u;
        return PR_U1C_0F73_BRANCH_STOP;
    }
    cpu->ip = 0x0FFBu;
    return pr_u1c_vertical_menu_after_list_0ffb(ram, ram_size,
                                                 cpu, external, context);
}
