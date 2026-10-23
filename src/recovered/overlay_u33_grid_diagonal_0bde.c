/* GAME.OVR unit 0x33:+0BDE..+0D12 with +0B97..+0BDD folded; docs/re/U33_GRID_DIAGONAL_0BDE.md. */
#include "poolrad/recovered/overlay_u33_grid_diagonal_0bde.h"
#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U33_0BDE_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define GRID_POINTER 0x6716u
#define FLAGS_BACK 0x033Au   /* +0BDE - +08A4 */
#define PAIR_BACK 0x0047u    /* +0BDE - +0B97 */
#define STUB_OFFSET 0x48u
#define ENTRY8_IP 0x0DE3u

_Static_assert(sizeof(pr_u33_0bde_cpu) == sizeof(pr_u33_08a4_cpu), "u33 +08A4 CPU layout differs");
_Static_assert(sizeof(pr_u33_0bde_cpu) == sizeof(pr_u9e_entry8_compose_cpu), "u9E entry-8 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u33_0bde_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_0bde_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_0bde_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t local(const uint8_t *ram, const pr_u33_0bde_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u33_0bde_cpu *cpu, uint16_t back, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - back), value);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u33_0bde_cpu *cpu, uint8_t left, uint8_t right)
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

static void logic(pr_u33_0bde_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void add16(pr_u33_0bde_cpu *cpu, uint16_t source)
{
    uint16_t left = cpu->ax;
    uint32_t wide = (uint32_t)left + source;
    uint16_t result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (wide & 0x10000u) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ source ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ result) & (source ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->ax = result;
}

static void sub16(pr_u33_0bde_cpu *cpu, uint16_t source)
{
    uint16_t left = cpu->ax;
    uint16_t result = (uint16_t)(left - source);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < source) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ source ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ source) & (left ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->ax = result;
}

/* INC/DEC of a byte in RAM: CF preserved. */
static void inc8_memory(uint8_t *ram, pr_u33_0bde_cpu *cpu, uint16_t segment, uint16_t offset, int decrement)
{
    uint8_t before = read8(ram, segment, offset);
    uint8_t result = (uint8_t)(decrement ? before - 1u : before + 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & ~0x0001u);
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (decrement ? ((before & 0x0Fu) == 0u) : ((before & 0x0Fu) == 0x0Fu)) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (decrement ? (before == 0x80u) : (before == 0x7Fu)) cpu->flags |= 0x0800u;
    write8(ram, segment, offset, result);
}

/* ES:DI := grid + sign(row) * 32h + sign(column), with column in CX and the
 * product in AX/DX, exactly as every cell address in this unit forms it. */
static void cell_address(uint8_t *ram, pr_u33_0bde_cpu *cpu, uint8_t column, uint8_t row, int column_plus_one)
{
    uint32_t product;
    size_t at;
    set_al(cpu, column);
    cbw(cpu);
    if (column_plus_one)
        cpu->ax = (uint16_t)(cpu->ax + 1u);
    cpu->cx = cpu->ax;
    set_al(cpu, row);
    cbw(cpu);
    cpu->dx = 0x32u;
    product = (uint32_t)cpu->ax * cpu->dx;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    at = physical(cpu->ds, GRID_POINTER);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
    cpu->di = (uint16_t)(cpu->di + cpu->cx);
}

/* +0B97..+0BDD, the pair writer, entered with its own frame: [BP+6] the
 * caller's BP, [BP+8] column, [BP+A] row; writes 34h at (column, row) and
 * 35h at (column + 1, row), increments the caller's [BP-2] through the
 * pushed BP, and returns with RETF 6. */
static pr_u33_0bde_status pair_writer(uint8_t *ram, pr_u33_0bde_cpu *cpu, uint16_t return_ip)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t link;
    size_t at;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cell_address(ram, cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)), read8(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)), 0);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x34u);
    cell_address(ram, cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)), read8(ram, cpu->ss, (uint16_t)(cpu->bp + 10u)), 1);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x35u);
    link = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = link;
    inc8_memory(ram, cpu, cpu->ss, (uint16_t)(cpu->di - 2u), 0);
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0BDE_TRANSFER;
    return PR_U33_0BDE_OK;
}

/* PUSH row; PUSH column; PUSH BP; PUSH CS; CALL +0B97 at a given site. */
static pr_u33_0bde_status place_pair(uint8_t *ram, pr_u33_0bde_cpu *cpu, uint16_t entry, uint16_t site,
                                     int row_plus_one, int column_plus_one)
{
    set_al(cpu, local(ram, cpu, 5u));
    if (row_plus_one) {
        cbw(cpu);
        cpu->ax = (uint16_t)(cpu->ax + 1u);
    }
    push16(ram, cpu, cpu->ax);
    set_al(cpu, local(ram, cpu, 4u));
    if (column_plus_one) {
        cbw(cpu);
        cpu->ax = (uint16_t)(cpu->ax + 1u);
    }
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->bp);
    return pair_writer(ram, cpu, (uint16_t)(entry + site + 3u));
}

static pr_u33_0bde_status call_08a4(uint8_t *ram, size_t ram_size, pr_u33_0bde_cpu *cpu,
                                    uint16_t entry, uint16_t return_delta)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + return_delta);
    pr_u33_08a4_cpu leaf;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = (uint16_t)(entry - FLAGS_BACK);
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u33_byte_flags_08a4(ram, ram_size, &leaf) != PR_U33_08A4_OK)
        return PR_U33_0BDE_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0BDE_TRANSFER;
    return PR_U33_0BDE_OK;
}

static int class_bit(pr_u33_0bde_cpu *cpu, uint8_t mask)
{
    set_al(cpu, (uint8_t)(cpu->ax & mask));
    logic(cpu, (uint8_t)cpu->ax, 0x80u);
    logic(cpu, (uint8_t)cpu->ax, 0x80u);
    return (uint8_t)cpu->ax != 0u;
}

static pr_u33_0bde_status roll(uint8_t *ram, size_t ram_size, pr_u33_0bde_cpu *cpu, uint16_t entry,
                               uint16_t site, uint8_t count, uint8_t range, int *chain_status)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + site + 5u);
    uint16_t stub_segment;
    pr_u9e_entry8_compose_cpu chain;
    pr_u9e_entry8_compose_status status;
    set_al(cpu, count);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, range);
    push16(ram, cpu, cpu->ax);
    if (read8(ram, cpu->cs, (uint16_t)(entry + site)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + site + 1u)) != STUB_OFFSET)
        return PR_U33_0BDE_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + site + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U33_0BDE_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = ENTRY8_IP;
    cpu->cs = read16(ram, stub_segment, STUB_OFFSET + 3u);
    memcpy(&chain, cpu, sizeof chain);
    status = pr_u9e_entry8_fullstate_compose_0de3(ram, ram_size, &chain, 256u);
    memcpy(cpu, &chain, sizeof chain);
    if (chain_status != NULL)
        *chain_status = (int)status;
    if (status != PR_U9E_ENTRY8_COMPOSE_RETURNED)
        return PR_U33_0BDE_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0BDE_TRANSFER;
    return PR_U33_0BDE_OK;
}

#define TRY(call) do { pr_u33_0bde_status status_ = (call); if (status_ != PR_U33_0BDE_OK) return status_; } while (0)

static void leave(uint8_t *ram, pr_u33_0bde_cpu *cpu)
{
    size_t at;
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
}

pr_u33_0bde_status pr_u33_grid_diagonal_0bde(
    uint8_t *ram, size_t ram_size, pr_u33_0bde_cpu *cpu, int *chain_status)
{
    uint16_t entry;
    int first_row = 1;
    if (chain_status != NULL)
        *chain_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_0BDE_RAM_SIZE)
        return PR_U33_0BDE_ARGUMENT;
    entry = cpu->ip;
    /* +0BDE..+0BE8: frame, six local bytes, threshold and pair count cleared. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 6u);
    set_local(ram, cpu, 1u, 0u);
    set_local(ram, cpu, 2u, 0u);
    /* +0BEC..+0C04: class bits 20h and 10h raise the threshold. */
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x12u));
    if (class_bit(cpu, 0x20u)) set_local(ram, cpu, 1u, 0x23u);
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x20u));
    if (class_bit(cpu, 0x10u)) set_local(ram, cpu, 1u, 0x4Bu);
    /* +0C08..+0C18: a (1, 64h) roll above the threshold returns. */
    TRY(roll(ram, ram_size, cpu, entry, 0x30u, 1u, 0x64u, chain_status));
    cmp8(cpu, (uint8_t)cpu->ax, local(ram, cpu, 1u));
    if ((uint8_t)cpu->ax > local(ram, cpu, 1u)) {
        leave(ram, cpu);
        return PR_U33_0BDE_OK;
    }
    /* +0C1B..+0C2F: x := 22h - (5, 4). */
    TRY(roll(ram, ram_size, cpu, entry, 0x43u, 5u, 4u, chain_status));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    cpu->ax = 0x22u;
    sub16(cpu, cpu->dx);
    set_local(ram, cpu, 4u, (uint8_t)cpu->ax);
    /* +0C32..+0C46: lower x until (x + 2) divided by 7 leaves no positive remainder. */
    for (;;) {
        int32_t dividend;
        set_al(cpu, local(ram, cpu, 4u));
        cbw(cpu);
        cpu->ax = (uint16_t)(cpu->ax + 2u);
        cpu->dx = (cpu->ax & 0x8000u) ? 0xFFFFu : 0u;
        cpu->cx = 7u;
        dividend = (int32_t)(((uint32_t)cpu->dx << 16) | cpu->ax);
        cpu->ax = (uint16_t)(dividend / 7);
        cpu->dx = (uint16_t)(dividend % 7);
        {
            uint16_t swap = cpu->ax;
            cpu->ax = cpu->dx;
            cpu->dx = swap;
        }
        logic(cpu, cpu->ax, 0x8000u);
        if ((int16_t)cpu->ax <= 0)
            break;
        inc8_memory(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u), 1);
    }
    /* +0C48..+0C4E: save x; row := 0. */
    set_al(cpu, local(ram, cpu, 4u));
    set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
    set_local(ram, cpu, 5u, 0u);
    for (;;) {
        uint8_t x;
        if (!first_row)
            inc8_memory(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 5u), 0);   /* +0C54 */
        first_row = 0;
        /* +0C57..+0C5B: x beyond 31h (signed) skips the row. */
        x = local(ram, cpu, 4u);
        cmp8(cpu, x, 0x31u);
        if ((int8_t)x <= 0x31) {
            /* +0C5D..+0C91: 32h at (x, row), 33h at (x + 1, row). */
            cell_address(ram, cpu, local(ram, cpu, 4u), local(ram, cpu, 5u), 0);
            write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x32u);
            cell_address(ram, cpu, local(ram, cpu, 4u), local(ram, cpu, 5u), 1);
            write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x33u);
            /* +0C96..+0CB6: a (1, 14h) roll of 1, or an odd pair count, places a pair. */
            TRY(roll(ram, ram_size, cpu, entry, 0xBEu, 1u, 0x14u, chain_status));
            cmp8(cpu, (uint8_t)cpu->ax, 1u);
            if ((uint8_t)cpu->ax != 1u) {
                uint8_t count = local(ram, cpu, 2u);
                set_al(cpu, (uint8_t)(count >> 1));
                cpu->flags &= (uint16_t)~ARITH_FLAGS;
                if (count & 1u) cpu->flags |= 0x0001u;
                cpu->flags |= 0x0010u; /* pinned DOSBox-X: AF set after SHR AL,1 */
                if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
                if ((uint8_t)cpu->ax == 0u) cpu->flags |= 0x0040u;
                if (count & 0x80u) cpu->flags |= 0x0800u;
                if ((count & 1u) == 0u)
                    goto advance;
            }
            TRY(place_pair(ram, cpu, entry, 0xD8u, 0, 0));
advance:
            inc8_memory(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u), 0);    /* +0CB9 */
        }
        /* +0CBC..+0CC0: row 18h ends the loop. */
        if (local(ram, cpu, 5u) == 0x18u)
            break;
    }
    /* +0CC2..+0CC6: a placed pair returns. */
    {
        uint8_t count = local(ram, cpu, 2u);
        cmp8(cpu, count, 0u);
        if (count != 0u) {
            leave(ram, cpu);
            return PR_U33_0BDE_OK;
        }
    }
    /* +0CC8..+0D0C: row := 0Ch - (1, 9) + 4; x := saved x + row; two pairs. */
    TRY(roll(ram, ram_size, cpu, entry, 0xF0u, 1u, 9u, chain_status));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    cpu->ax = 0x0Cu;
    sub16(cpu, cpu->dx);
    add16(cpu, 4u);
    set_local(ram, cpu, 5u, (uint8_t)cpu->ax);
    set_al(cpu, local(ram, cpu, 5u));
    cbw(cpu);
    cpu->dx = cpu->ax;
    set_al(cpu, local(ram, cpu, 3u));
    cbw(cpu);
    add16(cpu, cpu->dx);
    set_local(ram, cpu, 4u, (uint8_t)cpu->ax);
    TRY(place_pair(ram, cpu, entry, 0x11Du, 0, 0));
    TRY(place_pair(ram, cpu, entry, 0x12Eu, 1, 1));
    leave(ram, cpu);
    return PR_U33_0BDE_OK;
}
