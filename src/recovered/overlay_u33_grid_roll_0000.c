/* GAME.OVR unit 0x33:+0000..+012A; docs/re/U33_GRID_ROLL_0000.md. */
#include "poolrad/recovered/overlay_u33_grid_roll_0000.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U33_0000_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define GRID_POINTER 0x6716u
#define TABLE 0x2747u
#define MAP_ID 0x84DCu
#define STUB_OFFSET 0x48u   /* unit 0x9E entry 8 */
#define ENTRY8_IP 0x0DE3u
#define SITE_FIRST 0x4Bu
#define SITE_SECOND 0xB7u
#define SITE_THIRD 0xC6u

_Static_assert(sizeof(pr_u33_0000_cpu) == sizeof(pr_u9e_entry8_compose_cpu),
               "u9E entry-8 CPU state layout differs");

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

static void push16(uint8_t *ram, pr_u33_0000_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_0000_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_0000_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t local(const uint8_t *ram, const pr_u33_0000_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u33_0000_cpu *cpu, uint16_t back, uint8_t value)
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

static void cmp8(pr_u33_0000_cpu *cpu, uint8_t left, uint8_t right)
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

/* CX := sign(column [BP-1]); AX := sign(row [BP-2]) * 32h; ES:DI := grid + AX + CX. */
static void cell_address(uint8_t *ram, pr_u33_0000_cpu *cpu)
{
    uint32_t product;
    size_t at;
    set_al(cpu, local(ram, cpu, 1u));
    cbw(cpu);
    cpu->cx = cpu->ax;
    set_al(cpu, local(ram, cpu, 2u));
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

/* LCALL 00B0:0048 at a given site through the loaded stub into entry 8. */
static pr_u33_0000_status roll(uint8_t *ram, size_t ram_size, pr_u33_0000_cpu *cpu,
                               uint16_t entry, uint16_t site, int *chain_status)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + site + 5u);
    uint16_t stub_segment;
    pr_u9e_entry8_compose_cpu chain;
    pr_u9e_entry8_compose_status status;
    if (read8(ram, cpu->cs, (uint16_t)(entry + site)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + site + 1u)) != STUB_OFFSET)
        return PR_U33_0000_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + site + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U33_0000_CODE;
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
        return PR_U33_0000_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_0000_TRANSFER;
    return PR_U33_0000_OK;
}

/* MOV AL,count; PUSH AX; MOV AL,range; PUSH AX; LCALL. */
static pr_u33_0000_status roll_with(uint8_t *ram, size_t ram_size, pr_u33_0000_cpu *cpu,
                                    uint16_t entry, uint16_t site, uint8_t count, uint8_t range,
                                    int *chain_status)
{
    set_al(cpu, count);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, range);
    push16(ram, cpu, cpu->ax);
    return roll(ram, ram_size, cpu, entry, site, chain_status);
}

pr_u33_0000_status pr_u33_grid_roll_0000(
    uint8_t *ram, size_t ram_size, pr_u33_0000_cpu *cpu, int *chain_status)
{
    uint16_t entry;
    int first_column = 1;
    if (chain_status != NULL)
        *chain_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_0000_RAM_SIZE)
        return PR_U33_0000_ARGUMENT;
    entry = cpu->ip;
    /* +0000..+0006: frame, four local bytes, column := 0. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    set_local(ram, cpu, 1u, 0u);
    for (;;) {
        int first_row = 1;
        if (!first_column)
            set_local(ram, cpu, 1u, (uint8_t)(local(ram, cpu, 1u) + 1u));   /* +000C */
        first_column = 0;
        set_local(ram, cpu, 2u, 0u);                                          /* +000F */
        for (;;) {
            uint8_t cell, roll_value;
            pr_u33_0000_status status;
            if (!first_row)
                set_local(ram, cpu, 2u, (uint8_t)(local(ram, cpu, 2u) + 1u)); /* +0015 */
            first_row = 0;
            /* +0018..+0042: the cell byte indexes the DS table by four. */
            cell_address(ram, cpu);
            set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 7u)));
            cpu->ax &= 0x00FFu;
            cpu->di = cpu->ax;
            cpu->di = (uint16_t)(cpu->di << 1);
            cpu->di = (uint16_t)(cpu->di << 1);
            cell = read8(ram, cpu->ds, (uint16_t)(cpu->di + TABLE));
            cmp8(cpu, cell, 0x16u);
            if (cell == 0x16u) {
                /* +0045..+005E: roll(1, 64h); 1..61h does nothing. */
                status = roll_with(ram, ram_size, cpu, entry, SITE_FIRST, 1u, 0x64u, chain_status);
                if (status != PR_U33_0000_OK)
                    return status;
                set_local(ram, cpu, 3u, (uint8_t)cpu->ax);
                roll_value = local(ram, cpu, 3u);
                set_al(cpu, roll_value);
                cmp8(cpu, roll_value, 1u);
                if (roll_value >= 1u) {
                    cmp8(cpu, roll_value, 0x61u);
                    if (roll_value <= 0x61u)
                        goto next_row;
                }
                /* +0061..+00A4: 62h and 63h write 1Ah and 1Bh. */
                cmp8(cpu, roll_value, 0x62u);
                if (roll_value == 0x62u) {
                    cell_address(ram, cpu);
                    write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x1Au);
                    goto next_row;
                }
                cmp8(cpu, roll_value, 0x63u);
                if (roll_value == 0x63u) {
                    cell_address(ram, cpu);
                    write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x1Bu);
                    goto next_row;
                }
                /* +00A6..+0113: 64h off map 0Ah rolls (1, 64h) == 1, then (1, 0Ah). */
                cmp8(cpu, roll_value, 0x64u);
                if (roll_value != 0x64u)
                    goto next_row;
                {
                    uint8_t map = read8(ram, cpu->ds, MAP_ID);
                    cmp8(cpu, map, 0x0Au);
                    if (map == 0x0Au)
                        goto next_row;
                }
                status = roll_with(ram, ram_size, cpu, entry, SITE_SECOND, 1u, 0x64u, chain_status);
                if (status != PR_U33_0000_OK)
                    return status;
                cmp8(cpu, (uint8_t)cpu->ax, 1u);
                if ((uint8_t)cpu->ax != 1u)
                    goto next_row;
                status = roll_with(ram, ram_size, cpu, entry, SITE_THIRD, 1u, 0x0Au, chain_status);
                if (status != PR_U33_0000_OK)
                    return status;
                roll_value = (uint8_t)cpu->ax;
                cmp8(cpu, roll_value, 1u);
                if (roll_value >= 1u) {
                    cmp8(cpu, roll_value, 6u);
                    if (roll_value <= 6u) {
                        cell_address(ram, cpu);
                        write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x1Cu);
                        goto next_row;
                    }
                }
                cmp8(cpu, roll_value, 7u);
                if (roll_value < 7u)
                    goto next_row;
                cmp8(cpu, roll_value, 0x0Au);
                if (roll_value > 0x0Au)
                    goto next_row;
                cell_address(ram, cpu);
                write8(ram, cpu->es, (uint16_t)(cpu->di + 7u), 0x1Du);
            }
next_row:
            /* +0115..+011B: row 18h ends the inner loop. */
            if (local(ram, cpu, 2u) == 0x18u)
                break;
        }
        /* +011E..+0124: column 31h ends the outer loop with the compare's flags. */
        {
            uint8_t column = local(ram, cpu, 1u);
            cmp8(cpu, column, 0x31u);
            if (column == 0x31u)
                break;
        }
    }
    /* +0127..+012A: MOV SP,BP; POP BP; RETF. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U33_0000_OK;
}
