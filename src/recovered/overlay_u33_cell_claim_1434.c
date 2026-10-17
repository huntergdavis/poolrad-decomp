/* GAME.OVR unit 0x33:+1434..+1576; docs/re/U33_CELL_CLAIM_1434.md. */
#include "poolrad/recovered/overlay_u33_cell_claim_1434.h"
#include "poolrad/recovered/overlay_u3c6_0cb9_cpu.h"

#include <string.h>

#define MASK (PR_U33_1434_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define CALL_SITE 0xDAu      /* +150E: LCALL 03E0:0070 */
#define STUB_OFFSET 0x70u
#define SCAN_ENTRY 0x0CB9u

_Static_assert(sizeof(pr_u33_1434_cpu) == sizeof(pr_u3c6_0cb9_cpu), "u3C6 +0CB9 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u33_1434_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u33_1434_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void set_al(pr_u33_1434_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_1434_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t frame8(const uint8_t *ram, const pr_u33_1434_cpu *cpu, int16_t displacement)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp + (uint16_t)displacement));
}

/* MUL r16; the flags it writes are always overwritten before they are observed. */
static void mul16(pr_u33_1434_cpu *cpu, uint16_t source)
{
    uint32_t product = (uint32_t)cpu->ax * source;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u33_1434_cpu *cpu, uint8_t left, uint8_t right)
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

static uint16_t add16(pr_u33_1434_cpu *cpu, uint16_t left, uint16_t right)
{
    uint32_t wide = (uint32_t)left + right;
    uint16_t result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (wide > 0xFFFFu) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (~(left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    return result;
}

/* +1452..+147F and +1535..+1562: DI := 108h*sign(DS:4658) + 42h*r + 0Bh*sign(y) + sign(x),
 * with x pushed and popped around the products; returns with the last ADD's flags. */
static void availability_index(uint8_t *ram, pr_u33_1434_cpu *cpu)
{
    set_al(cpu, frame8(ram, cpu, 0x0E));
    cbw(cpu);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, frame8(ram, cpu, 0x0C));
    cbw(cpu);
    cpu->dx = 0x000Bu;
    mul16(cpu, cpu->dx);
    cpu->bx = cpu->ax;
    set_al(cpu, frame8(ram, cpu, 0x06));
    cpu->ax &= 0x00FFu;
    cpu->dx = 0x0042u;
    mul16(cpu, cpu->dx);
    cpu->cx = cpu->ax;
    set_al(cpu, read8(ram, cpu->ds, 0x4658u));
    cbw(cpu);
    cpu->dx = 0x0108u;
    mul16(cpu, cpu->dx);
    cpu->di = cpu->ax;
    cpu->di = add16(cpu, cpu->di, cpu->cx);
    cpu->di = add16(cpu, cpu->di, cpu->bx);
    cpu->ax = pop16(ram, cpu);
    cpu->di = add16(cpu, cpu->di, cpu->ax);
}

/* AX := 5*sign(byte at BP+displacement) with SI = the byte's sign extension. */
static void times_five(uint8_t *ram, pr_u33_1434_cpu *cpu, int16_t displacement)
{
    set_al(cpu, frame8(ram, cpu, displacement));
    cbw(cpu);
    cpu->si = cpu->ax;
    cpu->ax = (uint16_t)(cpu->ax << 1);
    cpu->ax = (uint16_t)(cpu->ax << 1);
    cpu->ax = (uint16_t)(cpu->ax + cpu->si);
}

/* DI := 4 * slot byte at BP+10h. */
static void slot_index(uint8_t *ram, pr_u33_1434_cpu *cpu)
{
    set_al(cpu, frame8(ram, cpu, 0x10));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di << 1);
}

static pr_u33_1434_status scan_call(uint8_t *ram, size_t ram_size, pr_u33_1434_cpu *cpu, uint16_t entry,
                                    int *callee_status)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + CALL_SITE + 5u);
    uint16_t stub_segment;
    pr_u3c6_0cb9_cpu leaf;
    pr_u3c6_0cb9_cpu_status status;
    if (read8(ram, cpu->cs, (uint16_t)(entry + CALL_SITE)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + CALL_SITE + 1u)) != STUB_OFFSET)
        return PR_U33_1434_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + CALL_SITE + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, (uint16_t)(STUB_OFFSET + 1u)) != SCAN_ENTRY)
        return PR_U33_1434_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = SCAN_ENTRY;
    cpu->cs = read16(ram, stub_segment, (uint16_t)(STUB_OFFSET + 3u));
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_u3c6_0cb9_cpu_run(ram, ram_size, &leaf, PR_U3C6_0CB9_RETF_CONTIGUOUS_PHYSICAL);
    memcpy(cpu, &leaf, sizeof leaf);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (status != PR_U3C6_0CB9_CPU_RETURNED)
        return PR_U33_1434_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_1434_TRANSFER;
    return PR_U33_1434_OK;
}

pr_u33_1434_status pr_u33_cell_claim_1434(
    uint8_t *ram, size_t ram_size, pr_u33_1434_cpu *cpu, int *callee_status)
{
    uint16_t entry;
    uint8_t value;
    pr_u33_1434_status status;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_1434_RAM_SIZE)
        return PR_U33_1434_ARGUMENT;
    entry = cpu->ip;
    /* +1434..+1437 */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    /* +143A..+1450: x in 0..0Ah and y in 0..5, signed. */
    value = frame8(ram, cpu, 0x0E);
    cmp8(cpu, value, 0u);
    if ((int8_t)value < 0) goto fail_1487;
    value = frame8(ram, cpu, 0x0C);
    cmp8(cpu, value, 0u);
    if ((int8_t)value < 0) goto fail_1487;
    value = frame8(ram, cpu, 0x0E);
    cmp8(cpu, value, 0x0Au);
    if ((int8_t)value > 0x0A) goto fail_1487;
    value = frame8(ram, cpu, 0x0C);
    cmp8(cpu, value, 0x05u);
    if ((int8_t)value > 0x05) goto fail_1487;
    /* +1452..+1485: the availability byte. */
    availability_index(ram, cpu);
    value = read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x4440u));
    cmp8(cpu, value, 0u);
    if (value != 0u) goto origin_148e;
fail_1487:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);
    goto leave_156e;
origin_148e:
    /* +148E..+14C2: slot q's column := x + 6*b10 + 5*b8 + 16h. */
    times_five(ram, cpu, 0x08);
    cpu->cx = cpu->ax;
    set_al(cpu, frame8(ram, cpu, 0x0A));
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax << 1);
    cpu->si = cpu->ax;
    cpu->ax = (uint16_t)(cpu->ax << 1);
    cpu->ax = (uint16_t)(cpu->ax + cpu->si);
    cpu->dx = cpu->ax;
    set_al(cpu, frame8(ram, cpu, 0x0E));
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
    cpu->ax = (uint16_t)(cpu->ax + cpu->cx);
    cpu->ax = (uint16_t)(cpu->ax + 0x16u);
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)cpu->ax);
    slot_index(ram, cpu);
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 0x5F27u), (uint8_t)cpu->dx);
    /* +14C6..+14EA: slot q's row := y + 5*b8 + 0Ah. */
    times_five(ram, cpu, 0x08);
    cpu->dx = cpu->ax;
    set_al(cpu, frame8(ram, cpu, 0x0C));
    cbw(cpu);
    cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
    cpu->ax = (uint16_t)(cpu->ax + 0x0Au);
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)cpu->ax);
    slot_index(ram, cpu);
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 0x5F28u), (uint8_t)cpu->dx);
    /* +14EE..+150E: +0CB9(P6 = SS:BP-3, P10 = SS:BP-2, k = 8, Q = DS:[65B9+4q]). */
    slot_index(ram, cpu);
    push16(ram, cpu, read16(ram, cpu->ds, (uint16_t)(cpu->di + 0x65BBu)));
    push16(ram, cpu, read16(ram, cpu->ds, (uint16_t)(cpu->di + 0x65B9u)));
    set_al(cpu, 8u);
    push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 2u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 3u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    status = scan_call(ram, ram_size, cpu, entry, callee_status);
    if (status != PR_U33_1434_OK)
        return status;
    /* +1513..+152F: no occupant, a nonzero class and a class weight below FFh. */
    value = frame8(ram, cpu, -2);
    cmp8(cpu, value, 0u);
    if (value != 0u) goto fail_156a;
    value = frame8(ram, cpu, -3);
    cmp8(cpu, value, 0u);
    if (value == 0u) goto fail_156a;
    set_al(cpu, frame8(ram, cpu, -3));
    cpu->ax &= 0x00FFu;
    cpu->di = cpu->ax;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di << 1);
    value = read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2744u));
    cmp8(cpu, value, 0xFFu);
    if (value >= 0xFFu) goto fail_156a;
    /* +1531..+1568: claim the cell. */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);
    availability_index(ram, cpu);
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 0x4440u), 0u);
    goto leave_156e;
fail_156a:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);
leave_156e:
    /* +156E..+1574: AL := [BP-1]; MOV SP,BP; POP BP; RETF 0Ch. */
    set_al(cpu, frame8(ram, cpu, -1));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Cu);
    return PR_U33_1434_OK;
}
