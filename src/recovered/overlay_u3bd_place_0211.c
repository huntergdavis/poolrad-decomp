/* GAME.OVR U3BD:+0211..+031E. See docs/re/U3BD_PLACE_0211.md. */
#include "poolrad/recovered/overlay_u3bd_place_0211.h"

#define RAM_MASK (PR_U3BD_0211_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define LOGIC_FLAGS 0x08C5u /* AF is undefined */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at & RAM_MASK] |
                      ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t local16(const uint8_t *ram, const pr_u3bd_0211_cpu *cpu,
                        uint16_t distance)
{
    return read16(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local16(uint8_t *ram, const pr_u3bd_0211_cpu *cpu,
                        uint16_t distance, uint16_t value)
{
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static uint8_t local8(const uint8_t *ram, const pr_u3bd_0211_cpu *cpu,
                      uint16_t distance)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local8(uint8_t *ram, const pr_u3bd_0211_cpu *cpu,
                       uint16_t distance, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static uint16_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u ? 4u : 0u;
}

static void result_flags(pr_u3bd_0211_cpu *cpu, uint16_t result,
                         uint16_t sign, uint16_t mask)
{
    cpu->flags &= (uint16_t)~mask;
    cpu->flags |= parity((uint8_t)result);
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static uint16_t add16(pr_u3bd_0211_cpu *cpu, uint16_t left, uint16_t right)
{
    uint32_t wide = (uint32_t)left + right;
    uint16_t result = (uint16_t)wide;
    result_flags(cpu, result, 0x8000u, ARITH_FLAGS);
    if (wide > 0xFFFFu) cpu->flags |= 1u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if ((~(left ^ right) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
    return result;
}

static uint16_t sub16(pr_u3bd_0211_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    result_flags(cpu, result, 0x8000u, ARITH_FLAGS);
    if (left < right) cpu->flags |= 1u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
    return result;
}

static uint8_t sub8(pr_u3bd_0211_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    result_flags(cpu, result, 0x80u, ARITH_FLAGS);
    if (left < right) cpu->flags |= 1u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x0800u;
    return result;
}

static uint16_t inc16(pr_u3bd_0211_cpu *cpu, uint16_t value)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = add16(cpu, value, 1u);
    cpu->flags = (uint16_t)((cpu->flags & ~1u) | carry);
    return result;
}

static uint8_t inc8(pr_u3bd_0211_cpu *cpu, uint8_t value)
{
    uint16_t carry = cpu->flags & 1u;
    uint8_t result = (uint8_t)(value + 1u);
    result_flags(cpu, result, 0x80u, ARITH_FLAGS);
    if (((value ^ 1u ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (value == 0x7Fu) cpu->flags |= 0x0800u;
    cpu->flags = (uint16_t)((cpu->flags & ~1u) | carry);
    return result;
}

static uint16_t dec16(pr_u3bd_0211_cpu *cpu, uint16_t value)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = sub16(cpu, value, 1u);
    cpu->flags = (uint16_t)((cpu->flags & ~1u) | carry);
    return result;
}

static void xor_ah(pr_u3bd_0211_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    result_flags(cpu, 0u, 0x80u, LOGIC_FLAGS | 0x10u); /* pinned: XOR clears AF */
}

static void cbw(pr_u3bd_0211_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static void mul_dx(pr_u3bd_0211_cpu *cpu)
{
    uint32_t result = (uint32_t)cpu->ax * cpu->dx;
    cpu->ax = (uint16_t)result;
    cpu->dx = (uint16_t)(result >> 16);
    /* Pinned MULW: SF/AF kept, ZF from AX, PF is the even parity of all four bytes of DX:AX. */
    {
        uint32_t fold = result;
        fold ^= fold >> 16; fold ^= fold >> 8; fold ^= fold >> 4; fold ^= fold >> 2; fold ^= fold >> 1;
        cpu->flags = (uint16_t)((cpu->flags & ~0x0845u) | (cpu->dx != 0u ? 0x0801u : 0u) |
                                (cpu->ax == 0u ? 0x0040u : 0u) | ((fold & 1u) ? 0u : 0x0004u));
    }
}

static void push16(uint8_t *ram, pr_u3bd_0211_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void les_pieces(const uint8_t *ram, pr_u3bd_0211_cpu *cpu)
{
    cpu->di = read16(ram, cpu->ds, 0x6A58u);
    cpu->es = read16(ram, cpu->ds, 0x6A5Au);
}

pr_u3bd_0211_status pr_u3bd_place_0211(
    uint8_t *ram, size_t ram_size, pr_u3bd_0211_cpu *cpu,
    pr_u3bd_0211_tile_call tile_call, void *context)
{
    uint16_t value, source_cs, target_ip, target_cs;
    uint8_t item;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3BD_0211_RAM_SIZE)
        return PR_U3BD_0211_ARGUMENT;

    /* +0211..+0254: prologue, table selection, signed low-byte origins. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = sub16(cpu, cpu->sp, 14u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    xor_ah(cpu);
    cpu->di = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x0C8Cu)));
    set_local8(ram, cpu, 9u, (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 12u)));
    cbw(cpu);
    cpu->dx = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    xor_ah(cpu);
    cpu->di = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x0C96u)));
    xor_ah(cpu);
    cpu->ax = add16(cpu, cpu->ax, cpu->dx);
    cpu->ax = dec16(cpu, cpu->ax);
    set_local16(ram, cpu, 2u, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)));
    cbw(cpu);
    cpu->dx = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    xor_ah(cpu);
    cpu->di = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x0CA0u)));
    xor_ah(cpu);
    cpu->ax = add16(cpu, cpu->ax, cpu->dx);
    cpu->ax = dec16(cpu, cpu->ax);
    set_local16(ram, cpu, 4u, cpu->ax);

    /* +0257..+0318: signed outer y / inner x loops. The cursor at BP-9
     * increments after every cell, even on clipping and zero items. */
    cpu->ax = local16(ram, cpu, 4u);
    set_local16(ram, cpu, 11u, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)));
    cbw(cpu);
    value = local16(ram, cpu, 11u);
    (void)sub16(cpu, cpu->ax, value);
    if ((int16_t)cpu->ax > (int16_t)value) goto epilogue;
    set_local16(ram, cpu, 8u, cpu->ax);
    goto outer_body;
outer_next:
    value = local16(ram, cpu, 8u);
    set_local16(ram, cpu, 8u, inc16(cpu, value));
outer_body:
    cpu->ax = local16(ram, cpu, 2u);
    set_local16(ram, cpu, 13u, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 12u)));
    cbw(cpu);
    value = local16(ram, cpu, 13u);
    (void)sub16(cpu, cpu->ax, value);
    if ((int16_t)cpu->ax > (int16_t)value) goto outer_tail;
    set_local16(ram, cpu, 6u, cpu->ax);
    goto inner_body;
inner_next:
    value = local16(ram, cpu, 6u);
    set_local16(ram, cpu, 6u, inc16(cpu, value));
inner_body:
    value = local16(ram, cpu, 8u);
    (void)sub16(cpu, value, 0u);
    if ((int16_t)value < 0) goto cell_done;
    (void)sub16(cpu, value, 10u);
    if ((int16_t)value > 10) goto cell_done;
    value = local16(ram, cpu, 6u);
    (void)sub16(cpu, value, 0u);
    if ((int16_t)value < 0) goto cell_done;
    (void)sub16(cpu, value, 10u);
    if ((int16_t)value > 10) goto cell_done;

    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | local8(ram, cpu, 9u));
    xor_ah(cpu);
    cpu->cx = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    xor_ah(cpu);
    cpu->dx = 156u;
    mul_dx(cpu);
    les_pieces(ram, cpu);
    cpu->di = add16(cpu, cpu->di, cpu->ax);
    cpu->di = add16(cpu, cpu->di, cpu->cx);
    item = read8(ram, cpu->es, (uint16_t)(cpu->di - 156u));
    (void)sub8(cpu, item, 0u);
    if (item == 0u) goto cell_done;

    cpu->ax = local16(ram, cpu, 6u);
    cpu->ax = add16(cpu, cpu->ax, 3u);
    cpu->ax = dec16(cpu, cpu->ax);
    push16(ram, cpu, cpu->ax);
    cpu->ax = local16(ram, cpu, 8u);
    cpu->ax = add16(cpu, cpu->ax, 3u);
    cpu->ax = dec16(cpu, cpu->ax);
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | local8(ram, cpu, 9u));
    xor_ah(cpu);
    cpu->cx = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    xor_ah(cpu);
    cpu->dx = 156u;
    mul_dx(cpu);
    les_pieces(ram, cpu);
    cpu->di = add16(cpu, cpu->di, cpu->ax);
    cpu->di = add16(cpu, cpu->di, cpu->cx);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->es, (uint16_t)(cpu->di - 156u)));
    xor_ah(cpu);
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    push16(ram, cpu, cpu->ax);
    source_cs = cpu->cs;
    target_ip = read16(ram, source_cs, 0x02FCu);
    target_cs = read16(ram, source_cs, 0x02FEu);
    push16(ram, cpu, source_cs);
    push16(ram, cpu, 0x0300u);
    cpu->ip = target_ip;
    cpu->cs = target_cs;
    if (tile_call == NULL) return PR_U3BD_0211_CALLEE_REQUIRED;
    if (tile_call(ram, ram_size, cpu, context) != 0)
        return PR_U3BD_0211_CALLEE_FAILED;
    if (cpu->cs != source_cs || cpu->ip != 0x0300u)
        return PR_U3BD_0211_CALLEE_TRANSFER;

cell_done:
    value = local8(ram, cpu, 9u);
    set_local8(ram, cpu, 9u, inc8(cpu, (uint8_t)value));
    cpu->ax = local16(ram, cpu, 6u);
    value = local16(ram, cpu, 13u);
    (void)sub16(cpu, cpu->ax, value);
    if (cpu->ax != value) goto inner_next;
outer_tail:
    cpu->ax = local16(ram, cpu, 8u);
    value = local16(ram, cpu, 11u);
    (void)sub16(cpu, cpu->ax, value);
    if (cpu->ax != value) goto outer_next;

epilogue:
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t return_at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, return_at);
        cpu->cs = read16_at(ram, return_at + 2u);
    }
    cpu->sp = (uint16_t)(cpu->sp + 12u);
    return PR_U3BD_0211_OK;
}
