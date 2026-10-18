/* GAME.OVR U38 +2DE4; docs/re/U38_ROLL_CLEAR_2DE4_CPU.md.
 * Raw composition adapted from the merged U38 +2992 implementation. */
#include "poolrad/recovered/overlay_u38_roll_clear_2de4_cpu.h"
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U38RC_RAM_SIZE - 1u)
#define STUB_OFFSET 0x48u
#define ENTRY8_IP 0x0DE3u
#define ARITH 0x08D5u

_Static_assert(sizeof(pr_u38rc_cpu) == sizeof(pr_u38c_cpu), "u38 helper CPU layout differs");
_Static_assert(sizeof(pr_u38rc_cpu) == sizeof(pr_u9e_entry8_compose_cpu), "U9E +0DE3 CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(uint8_t *ram, pr_u38rc_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u38rc_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

static void set_al(pr_u38rc_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void note(int *callee_status, int status)
{
    if (callee_status != NULL)
        *callee_status = status;
}

static int valid(const uint8_t *ram, size_t ram_size, const pr_u38rc_cpu *cpu)
{
    return ram != NULL && cpu != NULL && ram_size == PR_U38RC_RAM_SIZE
        && cpu->ip == 0x2DE4u && cpu->sp >= 0x0040u && cpu->sp <= 0xFFFCu;
}

/* roll(count, sides) at unit offset `site` of a routine at `base`: PUSH AL twice; LCALL through the loaded stub. */
static pr_u38rc_status roll(uint8_t *ram, size_t ram_size, pr_u38rc_cpu *cpu, uint16_t entry, uint16_t base, uint16_t site,
                           uint8_t count, uint8_t sides, unsigned max_system_calls, int *callee_status)
{
    uint16_t caller_cs = cpu->cs, at = (uint16_t)(entry + (uint16_t)(site - base)), return_ip = (uint16_t)(at + 5u);
    uint16_t stub_segment;
    pr_u9e_entry8_compose_cpu chain;
    pr_u9e_entry8_compose_status status;
    set_al(cpu, count);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, sides);
    push16(ram, cpu, cpu->ax);
    cpu->ip = at;
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != STUB_OFFSET)
        return PR_U38RC_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(at + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu || read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U38RC_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = ENTRY8_IP;
    cpu->cs = read16(ram, stub_segment, STUB_OFFSET + 3u);
    memcpy(&chain, cpu, sizeof chain);
    status = pr_u9e_entry8_fullstate_compose_0de3(ram, ram_size, &chain, max_system_calls);
    memcpy(cpu, &chain, sizeof chain);
    note(callee_status, (int)status);
    if (status != PR_U9E_ENTRY8_COMPOSE_RETURNED)
        return PR_U38RC_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38RC_TRANSFER;
    return PR_U38RC_OK;
}

/* MOV AL,key; PUSH AX; PUSH CS; CALL +0000 at unit offset `site` of a routine at `base`. */
static pr_u38rc_status helper(uint8_t *ram, size_t ram_size, pr_u38rc_cpu *cpu, uint16_t entry, uint16_t base, uint16_t site,
                             uint8_t key, int *callee_status)
{
    uint16_t caller_cs = cpu->cs, return_ip = (uint16_t)(entry + (uint16_t)(site + 3u - base));
    pr_u38c_cpu leaf;
    pr_u38c_status status;
    set_al(cpu, key);
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = (uint16_t)(entry - base);
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_u38_helper_0000_cpu(ram, ram_size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    note(callee_status, (int)status);
    if (status != PR_U38C_OK)
        return PR_U38RC_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38RC_TRANSFER;
    return PR_U38RC_OK;
}

/* POP BP; RETF 0Ah. */
static pr_u38rc_status leave_entry(const uint8_t *ram, pr_u38rc_cpu *cpu)
{
    size_t frame = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, frame);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    frame = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, frame);
    cpu->cs = read16_at(ram, (frame + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_U38RC_OK;
}

pr_u38rc_status pr_u38_roll_clear_2de4_cpu(uint8_t *ram, size_t ram_size, pr_u38rc_cpu *cpu, unsigned max_system_calls,
                                       int *callee_status)
{
    uint16_t entry;
    pr_u38rc_status status;
    note(callee_status, -1);
    if (!valid(ram, ram_size, cpu))
        return PR_U38RC_ARGUMENT;
    entry = cpu->ip;
    push16(ram, cpu, cpu->bp);                                            /* +2DE4 PUSH BP; MOV BP,SP */
    cpu->bp = cpu->sp;
    status = roll(ram, ram_size, cpu, entry, 0x2DE4u, 0x2DEDu, 1u, 0x64u, max_system_calls, callee_status);
    if (status != PR_U38RC_OK)
        return status;
    cmp8(cpu, (uint8_t)cpu->ax, 0x1Eu);                                   /* +2DF2 CMP AL,1Eh; JA +2E04 */
    if (!((uint8_t)cpu->ax > 0x1Eu)) {
        status = helper(ram, ram_size, cpu, entry, 0x2DE4u, 0x2DFAu, 0x0Bu, callee_status);
        if (status == PR_U38RC_OK)
            status = helper(ram, ram_size, cpu, entry, 0x2DE4u, 0x2E01u, 0x35u, callee_status);
        if (status != PR_U38RC_OK)
            return status;
    }
    return leave_entry(ram, cpu);                                         /* +2E04 POP BP; RETF 0Ah */
}
