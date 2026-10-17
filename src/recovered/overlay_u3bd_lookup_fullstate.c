/* GAME.OVR unit 0x3BD +047B..+0579 and +0350..+047A as full-state models;
 * docs/re/U33_CELL_LOOKUP_012B.md records the folding and its parity. */
#include "poolrad/recovered/overlay_u3bd_lookup_fullstate.h"
#include "poolrad/recovered/overlay_u3bd_in_grid_031f.h"

#include <string.h>

#define MASK (PR_U3BD_LOOKUP_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define MAP_ID 0x84DCu
#define BUFFER_POINTER 0x6A5Cu

_Static_assert(sizeof(pr_u3bd_lookup_cpu) == sizeof(pr_u3bd_031f_cpu),
               "u3BD +031F CPU state layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u3bd_lookup_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u3bd_lookup_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u3bd_lookup_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t arg(const uint8_t *ram, const pr_u3bd_lookup_cpu *cpu, uint16_t forward)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp + forward));
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u3bd_lookup_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

/* AND/OR on AL: CF/OF clear, undefined AF represented as zero. */
static void logic8(pr_u3bd_lookup_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
}

/* SHR AX,CL with CL in 2..6: CF is the last bit shifted out, OF follows
 * the penultimate and final sign bits (zero here), AF undefined as zero. */
static void shr16(pr_u3bd_lookup_cpu *cpu, unsigned count)
{
    uint16_t before = (uint16_t)(cpu->ax >> (count - 1u));
    uint16_t result = (uint16_t)(before >> 1);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before & 1u) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((before ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->ax = result;
}

/* PUSH CS; CALL +031F with the pushed x and y words; the merged leaf runs
 * on the same RAM and its RETF frame must return to the call site. */
static pr_u3bd_lookup_status call_031f(uint8_t *ram, size_t ram_size, pr_u3bd_lookup_cpu *cpu,
                                       uint16_t return_ip)
{
    pr_u3bd_031f_cpu leaf;
    uint16_t caller_cs = cpu->cs;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u3bd_in_grid_031f(ram, ram_size, &leaf) != PR_U3BD_031F_OK)
        return PR_U3BD_LOOKUP_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U3BD_LOOKUP_TRANSFER;
    return PR_U3BD_LOOKUP_OK;
}

/* +0491..+049D / +0366..+0372: a zero range result returns 0 at once on
 * map 0 or 10; the two reads of DS:84DC are kept separate. */
static int map_gate_returns_zero(uint8_t *ram, pr_u3bd_lookup_cpu *cpu)
{
    uint8_t map = read8(ram, cpu->ds, MAP_ID);
    cmp8(cpu, map, 0u);
    if (map == 0u)
        return 1;
    map = read8(ram, cpu->ds, MAP_ID);
    cmp8(cpu, map, 10u);
    return map == 10u;
}

/* +04A6..+04CD / +037B..+03A2: wrap the live x ([BP+A]) and y ([BP+8])
 * argument bytes into 0..15 with signed compares. */
static void wrap_arguments(uint8_t *ram, pr_u3bd_lookup_cpu *cpu)
{
    static const uint16_t forward[2] = {0x0Au, 0x08u};
    unsigned i;
    for (i = 0u; i < 2u; ++i) {
        uint8_t value = arg(ram, cpu, forward[i]);
        cmp8(cpu, value, 0x0Fu);
        if ((int8_t)value > 15)
            write8(ram, cpu->ss, (uint16_t)(cpu->bp + forward[i]), 0u);
        value = arg(ram, cpu, forward[i]);
        cmp8(cpu, value, 0u);
        if ((int8_t)value < 0)
            write8(ram, cpu->ss, (uint16_t)(cpu->bp + forward[i]), 0x0Fu);
    }
}

/* DX := sign(x); AX := sign(y) << 4 with CL = 4; ES:DI := DS:6A5C + AX + DX. */
static void cell_address(uint8_t *ram, pr_u3bd_lookup_cpu *cpu)
{
    size_t at;
    set_al(cpu, arg(ram, cpu, 0x0Au));
    cbw(cpu);
    cpu->dx = cpu->ax;
    set_al(cpu, arg(ram, cpu, 0x08u));
    cbw(cpu);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    cpu->ax = (uint16_t)(cpu->ax << 4);
    at = physical(cpu->ds, BUFFER_POINTER);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
}

static void frame_enter(uint8_t *ram, pr_u3bd_lookup_cpu *cpu)
{
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
}

/* MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 6. */
static void frame_leave(uint8_t *ram, pr_u3bd_lookup_cpu *cpu)
{
    size_t at;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
}

pr_u3bd_lookup_status pr_u3bd_nibble_047b_fullstate(
    uint8_t *ram, size_t ram_size, pr_u3bd_lookup_cpu *cpu)
{
    uint16_t entry;
    pr_u3bd_lookup_status status;
    uint8_t selector;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3BD_LOOKUP_RAM_SIZE)
        return PR_U3BD_LOOKUP_ARGUMENT;
    entry = cpu->ip;
    frame_enter(ram, cpu);
    /* +0481..+048A: range leaf on the live x and y bytes. */
    set_al(cpu, arg(ram, cpu, 0x0Au));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x08u));
    push16(ram, cpu, cpu->ax);
    status = call_031f(ram, ram_size, cpu, (uint16_t)(entry + 0x12u));
    if (status != PR_U3BD_LOOKUP_OK)
        return status;
    /* +048D..+049D */
    logic8(cpu, (uint8_t)cpu->ax);
    if ((uint8_t)cpu->ax == 0u && map_gate_returns_zero(ram, cpu)) {
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);   /* +049F */
        frame_leave(ram, cpu);
        return PR_U3BD_LOOKUP_OK;
    }
    wrap_arguments(ram, cpu);
    /* +04CE..+056E: the selector chain; an unmatched selector writes nothing. */
    selector = arg(ram, cpu, 0x06u);
    set_al(cpu, selector);
    cmp8(cpu, selector, 0u);
    if (selector == 0u) {
        cell_address(ram, cpu);
        set_al(cpu, read8(ram, cpu->es, cpu->di));
        logic8(cpu, (uint8_t)(cpu->ax & 0xF0u));
        set_al(cpu, (uint8_t)(cpu->ax & 0xF0u));
        cpu->ax &= 0x00FFu;
        cpu->cx = 4u;
        shr16(cpu, 4u);
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    } else {
        cmp8(cpu, selector, 2u);
        if (selector == 2u) {
            cell_address(ram, cpu);
            set_al(cpu, read8(ram, cpu->es, cpu->di));
            set_al(cpu, (uint8_t)(cpu->ax & 0x0Fu));
            logic8(cpu, (uint8_t)cpu->ax);
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
        } else {
            cmp8(cpu, selector, 4u);
            if (selector == 4u) {
                cell_address(ram, cpu);
                set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x100u)));
                logic8(cpu, (uint8_t)(cpu->ax & 0xF0u));
                set_al(cpu, (uint8_t)(cpu->ax & 0xF0u));
                cpu->ax &= 0x00FFu;
                cpu->cx = 4u;
                shr16(cpu, 4u);
                write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
            } else {
                cmp8(cpu, selector, 6u);
                if (selector == 6u) {
                    cell_address(ram, cpu);
                    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x100u)));
                    set_al(cpu, (uint8_t)(cpu->ax & 0x0Fu));
                    logic8(cpu, (uint8_t)cpu->ax);
                    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
                }
            }
        }
    }
    frame_leave(ram, cpu);   /* +0571 */
    return PR_U3BD_LOOKUP_OK;
}

/* PUSH CS; CALL +047B from +0350 with x, y and the selector pushed. */
static pr_u3bd_lookup_status call_047b(uint8_t *ram, size_t ram_size, pr_u3bd_lookup_cpu *cpu,
                                       uint16_t return_ip, uint16_t callee_ip)
{
    uint16_t caller_cs = cpu->cs;
    pr_u3bd_lookup_status status;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = callee_ip;
    status = pr_u3bd_nibble_047b_fullstate(ram, ram_size, cpu);
    if (status != PR_U3BD_LOOKUP_OK)
        return status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U3BD_LOOKUP_TRANSFER;
    return PR_U3BD_LOOKUP_OK;
}

/* AL := ES:[DI+300] & mask, AH := 0, CX := count, AX >>= count. */
static void field_bits(uint8_t *ram, pr_u3bd_lookup_cpu *cpu, uint8_t mask, unsigned count)
{
    cell_address(ram, cpu);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x300u)));
    logic8(cpu, (uint8_t)(cpu->ax & mask));
    set_al(cpu, (uint8_t)(cpu->ax & mask));
    cpu->ax &= 0x00FFu;
    cpu->cx = (uint16_t)count;
    shr16(cpu, count);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
}

pr_u3bd_lookup_status pr_u3bd_field_0350_fullstate(
    uint8_t *ram, size_t ram_size, pr_u3bd_lookup_cpu *cpu)
{
    uint16_t entry;
    pr_u3bd_lookup_status status;
    uint8_t selector;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3BD_LOOKUP_RAM_SIZE)
        return PR_U3BD_LOOKUP_ARGUMENT;
    entry = cpu->ip;
    frame_enter(ram, cpu);
    /* +0356..+035F: range leaf on the live x and y bytes. */
    set_al(cpu, arg(ram, cpu, 0x0Au));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x08u));
    push16(ram, cpu, cpu->ax);
    status = call_031f(ram, ram_size, cpu, (uint16_t)(entry + 0x12u));
    if (status != PR_U3BD_LOOKUP_OK)
        return status;
    /* +0362..+0372 */
    logic8(cpu, (uint8_t)cpu->ax);
    if ((uint8_t)cpu->ax == 0u && map_gate_returns_zero(ram, cpu)) {
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);   /* +0374 */
        frame_leave(ram, cpu);
        return PR_U3BD_LOOKUP_OK;
    }
    wrap_arguments(ram, cpu);
    /* +03A3..+03B4: default 1, then the nibble entry on the wrapped bytes. */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);
    set_al(cpu, arg(ram, cpu, 0x0Au));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x08u));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x06u));
    push16(ram, cpu, cpu->ax);
    status = call_047b(ram, ram_size, cpu, (uint16_t)(entry + 0x67u), (uint16_t)(entry + 0x12Bu));
    if (status != PR_U3BD_LOOKUP_OK)
        return status;
    /* +03B7..+03BB: OR AL,AL; JA keeps a nonzero nibble on the field chain. */
    logic8(cpu, (uint8_t)cpu->ax);
    if ((uint8_t)cpu->ax != 0u) {
        selector = arg(ram, cpu, 0x06u);
        set_al(cpu, selector);
        cmp8(cpu, selector, 6u);
        if (selector == 6u) {
            field_bits(ram, cpu, 0xC0u, 6u);
        } else {
            cmp8(cpu, selector, 4u);
            if (selector == 4u) {
                field_bits(ram, cpu, 0x30u, 4u);
            } else {
                cmp8(cpu, selector, 2u);
                if (selector == 2u) {
                    field_bits(ram, cpu, 0x0Cu, 2u);
                } else {
                    cmp8(cpu, selector, 0u);
                    if (selector == 0u) {
                        cell_address(ram, cpu);
                        set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x300u)));
                        set_al(cpu, (uint8_t)(cpu->ax & 0x03u));
                        logic8(cpu, (uint8_t)cpu->ax);
                        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
                    }
                }
            }
        }
    }
    /* +046C..+046F: the result passes through [BP-2] into [BP-1]. */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    frame_leave(ram, cpu);   /* +0472 */
    return PR_U3BD_LOOKUP_OK;
}
