/* GAME.OVR unit 0x2F:+0131..+01F5; docs/re/U2F_PARTY_PICK_0131.md. */
#include "poolrad/recovered/overlay_u2f_party_pick_0131.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U2F_0131_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define HEAD_OFFSET 0x5D96u
#define HEAD_SEGMENT 0x5D98u
#define CALL_SITE 0x2Cu     /* +015D - +0131 */
#define STUB_OFFSET 0x48u   /* unit 0x9E entry 8 */
#define ENTRY8_IP 0x0DE3u

_Static_assert(sizeof(pr_u2f_0131_cpu) == sizeof(pr_u9e_entry8_compose_cpu),
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

static void push16(uint8_t *ram, pr_u2f_0131_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u2f_0131_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u2f_0131_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static uint8_t local(const uint8_t *ram, const pr_u2f_0131_cpu *cpu, uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

/* LES DI,[offset] in the SS (stack local) or ES (node) segment. */
static void les(const uint8_t *ram, pr_u2f_0131_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u2f_0131_cpu *cpu, uint8_t left, uint8_t right)
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

static void cmp16(pr_u2f_0131_cpu *cpu, uint16_t left, uint16_t right)
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

/* OR/XOR: CF/OF clear, undefined AF represented as zero. */
static void logic16(pr_u2f_0131_cpu *cpu, uint16_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

/* DX := zero-extended best byte; AL := signed byte +3 of the record named
 * by the node's far pointer +108; CMP AX,DX. */
static void compare_candidate(uint8_t *ram, pr_u2f_0131_cpu *cpu)
{
    set_al(cpu, local(ram, cpu, 1u));
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 7u));
    les(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 3u)));
    cbw(cpu);
    cmp16(cpu, cpu->ax, cpu->dx);
}

/* +015D: LCALL 00B0:0048 through the loaded stub into entry 8 (+0DE3). */
static pr_u2f_0131_status roll(uint8_t *ram, size_t ram_size, pr_u2f_0131_cpu *cpu,
                               uint16_t entry, int *chain_status)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + CALL_SITE + 5u);
    uint16_t stub_segment;
    pr_u9e_entry8_compose_cpu chain;
    pr_u9e_entry8_compose_status status;
    if (read8(ram, cpu->cs, (uint16_t)(entry + CALL_SITE)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + CALL_SITE + 1u)) != STUB_OFFSET)
        return PR_U2F_0131_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + CALL_SITE + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U2F_0131_CODE;
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
        return PR_U2F_0131_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U2F_0131_TRANSFER;
    return PR_U2F_0131_OK;
}

pr_u2f_0131_status pr_u2f_party_pick_0131(
    uint8_t *ram, size_t ram_size, pr_u2f_0131_cpu *cpu, int *chain_status)
{
    uint16_t entry;
    if (chain_status != NULL)
        *chain_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U2F_0131_RAM_SIZE)
        return PR_U2F_0131_ARGUMENT;
    entry = cpu->ip;
    /* +0131..+013B: frame, eight local bytes, best byte and best roll cleared. */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);
    /* +013F..+0149: current := DS:5D96/5D98. */
    cpu->ax = read16(ram, cpu->ds, HEAD_OFFSET);
    cpu->dx = read16(ram, cpu->ds, HEAD_SEGMENT);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 7u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->dx);
    for (;;) {
        pr_u2f_0131_status status;
        /* +014C..+0154: both pointer words zero ends the walk. */
        cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 7u));
        cpu->ax = (uint16_t)(cpu->ax | read16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u)));
        logic16(cpu, cpu->ax);
        if (cpu->ax == 0u)
            break;
        /* +0157..+0162: roll(1, 64h) with the live AH in both pushed words. */
        set_al(cpu, 1u);
        push16(ram, cpu, cpu->ax);
        set_al(cpu, 0x64u);
        push16(ram, cpu, cpu->ax);
        status = roll(ram, ram_size, cpu, entry, chain_status);
        if (status != PR_U2F_0131_OK)
            return status;
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), (uint8_t)cpu->ax);
        /* +0165..+0180: a byte above the best so far takes this roll as the best roll. */
        compare_candidate(ram, cpu);
        if ((int16_t)cpu->ax > (int16_t)cpu->dx) {
            set_al(cpu, local(ram, cpu, 3u));
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
        }
        /* +0183..+01A1: a byte at least the best, with a roll at least the best roll, is kept. */
        compare_candidate(ram, cpu);
        if ((int16_t)cpu->ax >= (int16_t)cpu->dx) {
            uint8_t this_roll = local(ram, cpu, 3u);
            set_al(cpu, this_roll);
            cmp8(cpu, this_roll, local(ram, cpu, 2u));
            if (this_roll >= local(ram, cpu, 2u)) {
                /* +01A3..+01C4: best roll, best byte and the result pointer follow this node. */
                set_al(cpu, local(ram, cpu, 3u));
                write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
                les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 7u));
                les(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
                set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 3u)));
                write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
                cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 7u));
                cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u));
                les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
                write16(ram, cpu->es, cpu->di, cpu->ax);
                write16(ram, cpu->es, (uint16_t)(cpu->di + 2u), cpu->dx);
            }
        }
        /* +01C8..+01DB: current := current^.link at +104/+106. */
        les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 7u));
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 7u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->dx);
    }
    /* +01DE..+01EC: no byte above zero clears the result to 0:0. */
    {
        uint8_t best = local(ram, cpu, 1u);
        cmp8(cpu, best, 0u);
        if (best == 0u) {
            les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
            cpu->ax = 0u;
            logic16(cpu, 0u);
            write16(ram, cpu->es, cpu->di, cpu->ax);
            write16(ram, cpu->es, (uint16_t)(cpu->di + 2u), cpu->ax);
        }
    }
    /* +01F0..+01F3: MOV SP,BP; POP BP; RETF 4. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U2F_0131_OK;
}
