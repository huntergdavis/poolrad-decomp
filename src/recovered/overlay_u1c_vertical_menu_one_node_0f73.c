/* GAME.OVR unit 0x1C:+0F73..+1084, one-node VERTICAL MENU continuation. */
#include "poolrad/recovered/overlay_u1c_vertical_menu_one_node_0f73.h"

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

static void logic16(pr_u1c_0f08_cpu *cpu, uint16_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x8000u) cpu->flags |= 0x80u;
}

static void set_al(pr_u1c_0f08_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void clear_ah(pr_u1c_0f08_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x44u; /* XOR AH,AH: byte zero */
}

static void xor_ax_ax(pr_u1c_0f08_cpu *cpu)
{
    cpu->ax = 0u;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    cpu->flags |= 0x44u;
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

static void les_di(const uint8_t *ram, pr_u1c_0f08_cpu *cpu,
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

static int external_call(uint8_t *ram, size_t size, pr_u1c_0f08_cpu *cpu,
                         pr_u1c_0f73_external external, void *context,
                         pr_u1c_0f73_call call, uint16_t site,
                         uint16_t next, uint16_t cleanup)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    cpu->ip = next;
    return 1;
}

pr_u1c_0f73_status pr_u1c_vertical_menu_one_node_0f73(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context)
{
    uint16_t link_off, link_seg;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0F08_RAM_SIZE || cpu->ip != 0x0F73u)
        return PR_U1C_0F73_ARGUMENT;
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F73_NODE, 0x0F73u, 0x0F78u, 6u))
        return PR_U1C_0F73_CALL_ABORT;
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x010Au), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0108u), cpu->dx);
    write8(ram, cpu->ds, 0x5E24u, 1u);
    write8(ram, cpu->ds, 0x5E25u, 0x11u);
    set_al(cpu, 1u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x11u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x26u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x16u); push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x0Au); push16(ram, cpu, cpu->ax);
    set_al(cpu, 1u); push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 0x0102u);
    push16(ram, cpu, cpu->ss); push16(ram, cpu, cpu->di);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F73_DRAW, 0x0FAAu, 0x0FAFu, 16u))
        return PR_U1C_0F73_CALL_ABORT;
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    cpu->ax |= read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u));
    logic16(cpu, cpu->ax);
    if (cpu->ax == 0u) {
        cpu->ip = 0x0FB7u;
        return PR_U1C_0F73_BRANCH_STOP;
    }
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
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0F73_COPY, 0x0FD4u, 0x0FD9u, 10u))
        return PR_U1C_0F73_CALL_ABORT;
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x29u), 0u);
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u), cpu->dx);
    inc_byte(ram, cpu, (uint16_t)(cpu->bp - 2u));
    link_off = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u));
    link_seg = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u));
    cpu->ax = (uint16_t)(link_off | link_seg);
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        cpu->ip = 0x0FB7u;
        return PR_U1C_0F73_BRANCH_STOP;
    }
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x010Au));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0108u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0106u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0104u), cpu->dx);
    xor_ax_ax(cpu);
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
