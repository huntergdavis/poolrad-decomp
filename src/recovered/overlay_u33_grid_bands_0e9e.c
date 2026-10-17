/* GAME.OVR unit 0x33:+0E9E..+11B3; docs/re/U33_GRID_BANDS_0E9E.md. */
#include "poolrad/recovered/overlay_u33_grid_bands_0e9e.h"
#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U33_0E9E_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define GRID_POINTER 0x6716u
#define TABLE 0x2747u
#define FLAGS_BACK 0x05FAu   /* +0E9E - +08A4 */
#define STUB_OFFSET 0x48u
#define ENTRY8_IP 0x0DE3u

_Static_assert(sizeof(pr_u33_0e9e_cpu) == sizeof(pr_u33_08a4_cpu), "u33 +08A4 CPU layout differs");
_Static_assert(sizeof(pr_u33_0e9e_cpu) == sizeof(pr_u9e_entry8_compose_cpu), "u9E entry-8 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u33_0e9e_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_0e9e_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_0e9e_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t local(const uint8_t *ram, const pr_u33_0e9e_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u33_0e9e_cpu *cpu, uint16_t back, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - back), value);
}

/* MOV AL,[BP-back]; XOR AH,AH as a zero-extended word. */
static uint16_t zx_local(uint8_t *ram, pr_u33_0e9e_cpu *cpu, uint16_t back)
{
    set_al(cpu, local(ram, cpu, back));
    cpu->ax &= 0x00FFu;
    return cpu->ax;
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u33_0e9e_cpu *cpu, uint8_t left, uint8_t right)
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

static void cmp16(pr_u33_0e9e_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x0800u;
}

static void logic8(pr_u33_0e9e_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
}

static void add16(pr_u33_0e9e_cpu *cpu, uint16_t source)
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

/* CX := sign(column [BP-7]); AX := (sign(row [BP-8]) - delta) * 32h; ES:DI := grid + AX + CX. */
static void cell_address(uint8_t *ram, pr_u33_0e9e_cpu *cpu, int above)
{
    uint32_t product;
    size_t at;
    set_al(cpu, local(ram, cpu, 7u));
    cbw(cpu);
    cpu->cx = cpu->ax;
    set_al(cpu, local(ram, cpu, 8u));
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

static int cell_eligible(uint8_t *ram, pr_u33_0e9e_cpu *cpu, int above)
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

static pr_u33_0e9e_status call_08a4(uint8_t *ram, size_t ram_size, pr_u33_0e9e_cpu *cpu,
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
        return PR_U33_0E9E_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0E9E_TRANSFER;
    return PR_U33_0E9E_OK;
}

static int class_bit(pr_u33_0e9e_cpu *cpu, uint8_t mask)
{
    set_al(cpu, (uint8_t)(cpu->ax & mask));
    logic8(cpu, (uint8_t)cpu->ax);
    logic8(cpu, (uint8_t)cpu->ax);
    return (uint8_t)cpu->ax != 0u;
}

static pr_u33_0e9e_status roll(uint8_t *ram, size_t ram_size, pr_u33_0e9e_cpu *cpu, uint16_t entry,
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
        return PR_U33_0E9E_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + site + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U33_0E9E_CODE;
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
        return PR_U33_0E9E_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0E9E_TRANSFER;
    return PR_U33_0E9E_OK;
}

/* XOR AH,AH; ADD AX,1Fh; ADD AX,b; BL := AL; this cell := BL. */
static void write_added(uint8_t *ram, pr_u33_0e9e_cpu *cpu, uint16_t b)
{
    cpu->ax &= 0x00FFu;
    add16(cpu, 0x1Fu);
    add16(cpu, b);
    cpu->bx = (uint16_t)((cpu->bx & 0xFF00u) | (uint8_t)cpu->ax);
    cell_address(ram, cpu, 0);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), (uint8_t)cpu->bx);
}

#define TRY(call) do { pr_u33_0e9e_status status_ = (call); if (status_ != PR_U33_0E9E_OK) return status_; } while (0)

pr_u33_0e9e_status pr_u33_grid_bands_0e9e(
    uint8_t *ram, size_t ram_size, pr_u33_0e9e_cpu *cpu, int *chain_status)
{
    uint16_t entry;
    int first_column = 1;
    if (chain_status != NULL)
        *chain_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_0E9E_RAM_SIZE)
        return PR_U33_0E9E_ARGUMENT;
    entry = cpu->ip;
    /* +0E9E..+0EB4: frame, eight local bytes, the five band defaults. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    set_local(ram, cpu, 1u, 0u);
    set_local(ram, cpu, 2u, 6u);
    set_local(ram, cpu, 3u, 0x0Fu);
    set_local(ram, cpu, 4u, 0x28u);
    set_local(ram, cpu, 5u, 0u);
    /* +0EB8..+0EFC: four class checks. */
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x1Eu));
    if (class_bit(cpu, 0x02u)) { set_local(ram, cpu, 3u, 0x2Du); set_local(ram, cpu, 4u, 0x0Au); }
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x30u));
    if (class_bit(cpu, 0x04u)) set_local(ram, cpu, 3u, 0x19u);
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x3Eu));
    if (class_bit(cpu, 0x40u)) { set_local(ram, cpu, 4u, 5u); set_local(ram, cpu, 5u, 0x1Eu); }
    TRY(call_08a4(ram, ram_size, cpu, entry, 0x50u));
    if (class_bit(cpu, 0x80u)) { set_local(ram, cpu, 1u, 0x0Fu); set_local(ram, cpu, 4u, 0x19u); set_local(ram, cpu, 3u, 0x0Au); }
    /* +0F00..+11AD: columns 0..31h, rows 0..18h. */
    set_local(ram, cpu, 7u, 0u);
    for (;;) {
        int first_row = 1;
        if (!first_column)
            set_local(ram, cpu, 7u, (uint8_t)(local(ram, cpu, 7u) + 1u));   /* +0F06 */
        first_column = 0;
        set_local(ram, cpu, 8u, 0u);                                          /* +0F09 */
        for (;;) {
            if (!first_row)
                set_local(ram, cpu, 8u, (uint8_t)(local(ram, cpu, 8u) + 1u)); /* +0F0F */
            first_row = 0;
            if (cell_eligible(ram, cpu, 0)) {
                uint8_t value;
                /* +0F3F..+0F55: (1, 0FFh); at or below the first band width stays here. */
                TRY(roll(ram, ram_size, cpu, entry, 0xA7u, 1u, 0xFFu, chain_status));
                set_local(ram, cpu, 6u, (uint8_t)cpu->ax);
                value = local(ram, cpu, 6u);
                set_al(cpu, value);
                cmp8(cpu, value, local(ram, cpu, 1u));
                if (value <= local(ram, cpu, 1u)) {
                    /* +0F58..+1004: (1, 4); a 4 with an eligible cell above writes 41h/40h; below 4 writes roll + 3Ch. */
                    TRY(roll(ram, ram_size, cpu, entry, 0xC0u, 1u, 4u, chain_status));
                    set_local(ram, cpu, 6u, (uint8_t)cpu->ax);
                    value = local(ram, cpu, 6u);
                    cmp8(cpu, value, 4u);
                    if (value == 4u) {
                        uint8_t row = local(ram, cpu, 8u);
                        cmp8(cpu, row, 0u);
                        if ((int8_t)row > 0 && cell_eligible(ram, cpu, 1)) {
                            cell_address(ram, cpu, 1);
                            write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x41u);
                            cell_address(ram, cpu, 0);
                            write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x40u);
                        }
                    }
                    value = local(ram, cpu, 6u);
                    cmp8(cpu, value, 4u);
                    if (value < 4u) {
                        zx_local(ram, cpu, 6u);
                        write_added(ram, cpu, 0x1Du);
                    }
                    goto next_row;
                }
                /* +1007..+1050: second band: (1, 3) + 36h. */
                cpu->dx = zx_local(ram, cpu, 1u);
                zx_local(ram, cpu, 2u);
                add16(cpu, cpu->dx);
                cpu->dx = cpu->ax;
                zx_local(ram, cpu, 6u);
                cmp16(cpu, cpu->ax, cpu->dx);
                if ((int16_t)cpu->ax <= (int16_t)cpu->dx) {
                    TRY(roll(ram, ram_size, cpu, entry, 0x188u, 1u, 3u, chain_status));
                    write_added(ram, cpu, 0x17u);
                    goto next_row;
                }
                /* +1053..+10A5: third band: (1, 4) + 29h. */
                cpu->cx = zx_local(ram, cpu, 2u);
                cpu->dx = zx_local(ram, cpu, 1u);
                zx_local(ram, cpu, 3u);
                add16(cpu, cpu->dx);
                add16(cpu, cpu->cx);
                cpu->dx = cpu->ax;
                zx_local(ram, cpu, 6u);
                cmp16(cpu, cpu->ax, cpu->dx);
                if ((int16_t)cpu->ax <= (int16_t)cpu->dx) {
                    TRY(roll(ram, ram_size, cpu, entry, 0x1DDu, 1u, 4u, chain_status));
                    write_added(ram, cpu, 0x0Au);
                    goto next_row;
                }
                /* +10A8..+1113: fourth band: (1, 0Ah) folded by three + 38h. */
                cpu->bx = zx_local(ram, cpu, 2u);
                cpu->cx = zx_local(ram, cpu, 1u);
                cpu->dx = zx_local(ram, cpu, 3u);
                zx_local(ram, cpu, 5u);
                add16(cpu, cpu->dx);
                add16(cpu, cpu->cx);
                add16(cpu, cpu->bx);
                cpu->dx = cpu->ax;
                zx_local(ram, cpu, 6u);
                cmp16(cpu, cpu->ax, cpu->dx);
                if ((int16_t)cpu->ax <= (int16_t)cpu->dx) {
                    uint32_t dividend;
                    TRY(roll(ram, ram_size, cpu, entry, 0x23Bu, 1u, 0x0Au, chain_status));
                    cpu->ax &= 0x00FFu;
                    cpu->ax = (uint16_t)(cpu->ax - 1u);
                    cpu->dx = (cpu->ax & 0x8000u) ? 0xFFFFu : 0u;
                    cpu->cx = 3u;
                    dividend = ((uint32_t)cpu->dx << 16) | cpu->ax;
                    cpu->ax = (uint16_t)(dividend / 3u);
                    cpu->dx = (uint16_t)(dividend % 3u);
                    cpu->ax = (uint16_t)(cpu->ax + 1u);
                    set_local(ram, cpu, 6u, (uint8_t)cpu->ax);
                    zx_local(ram, cpu, 6u);
                    write_added(ram, cpu, 0x19u);
                    goto next_row;
                }
                /* +1116..+119A: fifth band: (1, 4), a 4 rerolled by (1, 3) unless class bit 40h, + 2Dh. */
                cpu->bx = zx_local(ram, cpu, 1u);
                cpu->cx = zx_local(ram, cpu, 3u);
                cpu->dx = zx_local(ram, cpu, 5u);
                zx_local(ram, cpu, 4u);
                add16(cpu, cpu->dx);
                add16(cpu, cpu->cx);
                add16(cpu, cpu->bx);
                cpu->dx = cpu->ax;
                zx_local(ram, cpu, 2u);
                add16(cpu, cpu->dx);
                cpu->dx = cpu->ax;
                zx_local(ram, cpu, 6u);
                cmp16(cpu, cpu->ax, cpu->dx);
                if ((int16_t)cpu->ax <= (int16_t)cpu->dx) {
                    TRY(roll(ram, ram_size, cpu, entry, 0x2B2u, 1u, 4u, chain_status));
                    set_local(ram, cpu, 6u, (uint8_t)cpu->ax);
                    value = local(ram, cpu, 6u);
                    cmp8(cpu, value, 4u);
                    if (value == 4u) {
                        TRY(call_08a4(ram, ram_size, cpu, entry, 0x2C4u));
                        if (!class_bit(cpu, 0x40u)) {
                            TRY(roll(ram, ram_size, cpu, entry, 0x2D0u, 1u, 3u, chain_status));
                            set_local(ram, cpu, 6u, (uint8_t)cpu->ax);
                        }
                    }
                    zx_local(ram, cpu, 6u);
                    write_added(ram, cpu, 0x0Eu);
                }
            }
next_row:
            /* +119E..+11A4: row 18h ends the inner loop. */
            if (local(ram, cpu, 8u) == 0x18u)
                break;
        }
        /* +11A7..+11AD: column 31h ends the outer loop with the compare's flags. */
        {
            uint8_t column = local(ram, cpu, 7u);
            cmp8(cpu, column, 0x31u);
            if (column == 0x31u)
                break;
        }
    }
    /* +11B0..+11B3: MOV SP,BP; POP BP; RETF. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U33_0E9E_OK;
}
