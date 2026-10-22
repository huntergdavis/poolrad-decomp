/* GAME.OVR unit 0x33:+0D13..+0E9D; docs/re/U33_GRID_PAIR_0D13.md. */
#include "poolrad/recovered/overlay_u33_grid_pair_0d13.h"
#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U33_0D13_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define GRID_POINTER 0x6716u
#define TABLE 0x2747u
#define FLAGS_BACK 0x046Fu   /* +0D13 - +08A4 */
#define STUB_OFFSET 0x48u
#define ENTRY8_IP 0x0DE3u

_Static_assert(sizeof(pr_u33_0d13_cpu) == sizeof(pr_u33_08a4_cpu), "u33 +08A4 CPU layout differs");
_Static_assert(sizeof(pr_u33_0d13_cpu) == sizeof(pr_u9e_entry8_compose_cpu), "u9E entry-8 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u33_0d13_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_0d13_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_0d13_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t local(const uint8_t *ram, const pr_u33_0d13_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u33_0d13_cpu *cpu, uint16_t back, uint8_t value)
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

static void cmp8(pr_u33_0d13_cpu *cpu, uint8_t left, uint8_t right)
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
static void logic8(pr_u33_0d13_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
}

static void add16(pr_u33_0d13_cpu *cpu, uint16_t source)
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

/* INC AX (CF preserved) then SHR AX,1: CF the bit shifted out, OF the sign change. */
static void inc_then_shr(pr_u33_0d13_cpu *cpu)
{
    uint16_t before;
    cpu->ax = (uint16_t)(cpu->ax + 1u);
    before = cpu->ax;
    cpu->ax = (uint16_t)(before >> 1);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before & 1u) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
    if (cpu->ax == 0u) cpu->flags |= 0x0040u;
    if (cpu->ax & 0x8000u) cpu->flags |= 0x0080u;
    if ((before ^ cpu->ax) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->flags |= 0x0010u; /* pinned: SHR sets AF for a non-zero count */
}

/* CX := sign(column); AX := (sign(row) - delta) * 32h; ES:DI := grid + AX + CX. */
static void cell_address(uint8_t *ram, pr_u33_0d13_cpu *cpu, int above)
{
    uint32_t product;
    size_t at;
    set_al(cpu, local(ram, cpu, 4u));
    cbw(cpu);
    cpu->cx = cpu->ax;
    set_al(cpu, local(ram, cpu, 5u));
    cbw(cpu);
    if (above)
        cpu->ax = (uint16_t)(cpu->ax - 1u);
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

/* The cell byte +7 indexes the DS table by four; CMP with 16h. */
static int cell_eligible(uint8_t *ram, pr_u33_0d13_cpu *cpu, int above)
{
    uint8_t entry;
    cell_address(ram, cpu, above);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 7u)));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di << 1);
    entry = read8(ram, cpu->ds, (uint16_t)(cpu->di + TABLE));
    cmp8(cpu, entry, 0x16u);
    return entry == 0x16u;
}

/* PUSH CS; CALL +08A4: the merged byte-flags model returns through its own RETF. */
static pr_u33_0d13_status call_08a4(uint8_t *ram, size_t ram_size, pr_u33_0d13_cpu *cpu,
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
        return PR_U33_0D13_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0D13_TRANSFER;
    return PR_U33_0D13_OK;
}

/* AND AL,mask; OR AL,AL: the class bit decides. */
static int class_bit(pr_u33_0d13_cpu *cpu, uint8_t mask)
{
    set_al(cpu, (uint8_t)(cpu->ax & mask));
    logic8(cpu, (uint8_t)cpu->ax);
    logic8(cpu, (uint8_t)cpu->ax);
    return (uint8_t)cpu->ax != 0u;
}

/* MOV AL,count; PUSH AX; MOV AL,range; PUSH AX; LCALL 00B0:0048 through the loaded stub. */
static pr_u33_0d13_status roll(uint8_t *ram, size_t ram_size, pr_u33_0d13_cpu *cpu, uint16_t entry,
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
        return PR_U33_0D13_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + site + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U33_0D13_CODE;
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
        return PR_U33_0D13_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0D13_TRANSFER;
    return PR_U33_0D13_OK;
}

/* AL := local; XOR AH,AH; ADD AX,a; ADD AX,b; BL := AL; the cell is written with BL. */
static void write_cell(uint8_t *ram, pr_u33_0d13_cpu *cpu, uint16_t a, uint16_t b, int above)
{
    set_al(cpu, local(ram, cpu, 3u));
    cpu->ax &= 0x00FFu;
    add16(cpu, a);
    if (b != 0u)
        add16(cpu, b);
    cpu->bx = (uint16_t)((cpu->bx & 0xFF00u) | (uint8_t)cpu->ax);
    cell_address(ram, cpu, above);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), (uint8_t)cpu->bx);
}

#define TRY(call) do { pr_u33_0d13_status status_ = (call); if (status_ != PR_U33_0D13_OK) return status_; } while (0)

pr_u33_0d13_status pr_u33_grid_pair_0d13(
    uint8_t *ram, size_t ram_size, pr_u33_0d13_cpu *cpu, int *chain_status)
{
    uint16_t entry;
    int first_column = 1;
    if (chain_status != NULL)
        *chain_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_0D13_RAM_SIZE)
        return PR_U33_0D13_ARGUMENT;
    entry = cpu->ip;
    /* +0D13..+0D19: frame, six local bytes, threshold := 1. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 6u);
    set_local(ram, cpu, 2u, 1u);
    /* +0D1D..+0D5F: five class checks, the last matching bit wins. */
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x0Eu));
    if (class_bit(cpu, 0x02u)) set_local(ram, cpu, 2u, 1u);
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x1Cu));
    if (class_bit(cpu, 0x04u)) set_local(ram, cpu, 2u, 3u);
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x2Au));
    if (class_bit(cpu, 0x40u)) set_local(ram, cpu, 2u, 4u);
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x38u));
    if (class_bit(cpu, 0x08u)) set_local(ram, cpu, 2u, 7u);
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x46u));
    if (class_bit(cpu, 0x80u)) set_local(ram, cpu, 2u, 0u);
    /* +0D63..+0E97: columns 0..31h, rows 1..18h. */
    set_local(ram, cpu, 4u, 0u);
    for (;;) {
        int first_row = 1;
        if (!first_column)
            set_local(ram, cpu, 4u, (uint8_t)(local(ram, cpu, 4u) + 1u));   /* +0D69 */
        first_column = 0;
        set_local(ram, cpu, 5u, 1u);                                          /* +0D6C */
        for (;;) {
            if (!first_row)
                set_local(ram, cpu, 5u, (uint8_t)(local(ram, cpu, 5u) + 1u)); /* +0D72 */
            first_row = 0;
            if (cell_eligible(ram, cpu, 0) && cell_eligible(ram, cpu, 1)) {
                uint8_t value;
                /* +0DD0..+0DE0: a (1, 64h) roll above the threshold skips the cell. */
                TRY(roll(ram, ram_size, cpu, entry, 0xC3u, 1u, 0x64u, chain_status));
                cmp8(cpu, (uint8_t)cpu->ax, local(ram, cpu, 2u));
                if ((uint8_t)cpu->ax <= local(ram, cpu, 2u)) {
                    /* +0DE3..+0E14: (1, 0Ah); above 8 a (1, 2) roll plus 4, else (value + 1) / 2. */
                    TRY(roll(ram, ram_size, cpu, entry, 0xD6u, 1u, 0x0Au, chain_status));
                    set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
                    value = local(ram, cpu, 3u);
                    cmp8(cpu, value, 8u);
                    if (value > 8u) {
                        TRY(roll(ram, ram_size, cpu, entry, 0xEAu, 1u, 2u, chain_status));
                        cpu->ax &= 0x00FFu;
                        add16(cpu, 4u);
                        set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
                    } else {
                        set_al(cpu, local(ram, cpu, 3u));
                        cpu->ax &= 0x00FFu;
                        inc_then_shr(cpu);
                        set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
                    }
                    /* +0E17..+0E31: class bit 40h replaces the value with a (1, 3) roll plus 3. */
                    TRY(call_08a4(ram, ram_size, cpu, entry, 0x108u));
                    if (class_bit(cpu, 0x40u)) {
                        TRY(roll(ram, ram_size, cpu, entry, 0x114u, 1u, 3u, chain_status));
                        cpu->ax &= 0x00FFu;
                        add16(cpu, 3u);
                        set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
                    }
                    /* +0E34..+0E84: this cell gets value + 1Fh + 4; below 5 the cell above gets value + 1Fh. */
                    write_cell(ram, cpu, 0x1Fu, 4u, 0);
                    value = local(ram, cpu, 3u);
                    cmp8(cpu, value, 5u);
                    if (value < 5u)
                        write_cell(ram, cpu, 0x1Fu, 0u, 1);
                }
            }
            /* +0E88..+0E8E: row 18h ends the inner loop. */
            if (local(ram, cpu, 5u) == 0x18u)
                break;
        }
        /* +0E91..+0E97: column 31h ends the outer loop with the compare's flags. */
        {
            uint8_t column = local(ram, cpu, 4u);
            cmp8(cpu, column, 0x31u);
            if (column == 0x31u)
                break;
        }
    }
    /* +0E9A..+0E9D: MOV SP,BP; POP BP; RETF. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U33_0D13_OK;
}
