/* GAME.OVR unit 0x38 +2915, +2976, +2984, +2992 (raw CPU); docs/re/U38_ROLL_GATE_2915_CPU.md. */
#include "poolrad/recovered/overlay_u38_roll_gate_2915_cpu.h"
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
#include "poolrad/recovered/resident_00ba_class_value_26c2_cpu.h"

#include <string.h>

#define MASK (PR_U38R_RAM_SIZE - 1u)
#define STUB_OFFSET 0x48u
#define ENTRY8_IP 0x0DE3u
#define LOOKUP_ENTRY 0x26C2u
#define ARITH 0x08D5u

_Static_assert(sizeof(pr_u38r_cpu) == sizeof(pr_u38c_cpu), "u38 helper CPU layout differs");
_Static_assert(sizeof(pr_u38r_cpu) == sizeof(pr_u9e_entry8_compose_cpu), "U9E +0DE3 CPU layout differs");
_Static_assert(sizeof(pr_u38r_cpu) == sizeof(pr_res_26c2_cpu), "RES_00BA_26C2 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u38r_cpu *cpu, uint16_t value)
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

static void cmp8(pr_u38r_cpu *cpu, uint8_t left, uint8_t right)
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

/* AND AL,imm8; OR AL,AL: CF = OF = AF = 0, PF/ZF/SF from AL. */
static void and_or_al(pr_u38r_cpu *cpu, uint8_t mask)
{
    uint8_t al = (uint8_t)(cpu->ax & mask);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | al);
    cpu->flags &= (uint16_t)~ARITH;
    if (even_parity(al)) cpu->flags |= 0x0004u;
    if (al == 0u) cpu->flags |= 0x0040u;
    if (al & 0x80u) cpu->flags |= 0x0080u;
}

static void set_al(pr_u38r_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void note(int *callee_status, int status)
{
    if (callee_status != NULL)
        *callee_status = status;
}

static int valid(const uint8_t *ram, size_t ram_size, const pr_u38r_cpu *cpu)
{
    return ram != NULL && cpu != NULL && ram_size == PR_U38R_RAM_SIZE;
}

/* roll(count, sides) at unit offset `site` of a routine at `base`: PUSH AL twice; LCALL through the loaded stub. */
static pr_u38r_status roll(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, uint16_t entry, uint16_t base, uint16_t site,
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
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != STUB_OFFSET)
        return PR_U38R_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(at + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu || read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U38R_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = ENTRY8_IP;
    cpu->cs = read16(ram, stub_segment, STUB_OFFSET + 3u);
    memcpy(&chain, cpu, sizeof chain);
    status = pr_u9e_entry8_fullstate_compose_0de3(ram, ram_size, &chain, max_system_calls);
    memcpy(cpu, &chain, sizeof chain);
    note(callee_status, (int)status);
    if (status != PR_U9E_ENTRY8_COMPOSE_RETURNED)
        return PR_U38R_CALLEE;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38R_TRANSFER;
    return PR_U38R_OK;
}

/* MOV AL,key; PUSH AX; PUSH CS; CALL +0000 at unit offset `site` of a routine at `base`. */
static pr_u38r_status helper(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, uint16_t entry, uint16_t base, uint16_t site,
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
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38R_TRANSFER;
    return PR_U38R_OK;
}

/* +2915..+2973 on the live CPU, entered with the far frame and key word already pushed. */
static pr_u38r_status gate(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls, int *callee_status)
{
    uint16_t entry = cpu->ip, caller_cs, at, return_ip;
    uint8_t value;
    size_t frame;
    pr_res_26c2_cpu leaf;
    pr_u38r_status status;
    push16(ram, cpu, cpu->bp);                                            /* +2915 PUSH BP; MOV BP,SP; SUB SP,2 */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    set_al(cpu, read8(ram, cpu->ds, 0x681Bu));                            /* +291B MOV AL,[681B]; PUSH AX; LCALL 26C2 */
    push16(ram, cpu, cpu->ax);
    caller_cs = cpu->cs;
    at = (uint16_t)(entry + 0x0Au);
    return_ip = (uint16_t)(at + 5u);
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != LOOKUP_ENTRY)
        return PR_U38R_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = read16(ram, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = LOOKUP_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    note(callee_status, (int)pr_res_00ba_26c2_cpu(ram, ram_size, &leaf));
    memcpy(cpu, &leaf, sizeof leaf);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38R_TRANSFER;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);    /* +2924 MOV [BP-1],AL; MOV AL,[BP-1]; XOR AH,AH */
    cpu->ax = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u));
    cpu->dx = cpu->ax;                                                    /* +292C..+293B: DX := 5 * (11 - r) (flags folded) */
    cpu->ax = (uint16_t)(0x0Bu - cpu->dx);
    cpu->si = cpu->ax;
    cpu->ax = (uint16_t)(cpu->ax << 2);
    cpu->ax = (uint16_t)(cpu->ax + cpu->si);
    cpu->dx = cpu->ax;
    cpu->ax = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));              /* +293D MOV AL,[BP+6]; XOR AH,AH; SUB AX,DX */
    cpu->ax = (uint16_t)(cpu->ax - cpu->dx);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    value = read8(ram, cpu->ds, 0x6817u);                                 /* +2947 CMP BYTE [6817],0; JNE +2959 */
    cmp8(cpu, value, 0u);
    if (value == 0u) {
        set_al(cpu, read8(ram, cpu->ds, 0x6819u));                        /* +294E MOV AL,[6819]; AND AL,8; OR AL,AL */
        and_or_al(cpu, 0x08u);
        if ((uint8_t)cpu->ax == 0u)                                       /* +2955 JNE +2959; +2957 JMP +2970 */
            goto leave;
    }
    status = roll(ram, ram_size, cpu, entry, 0x2915u, 0x295Fu, 1u, 0x64u, max_system_calls, callee_status);
    if (status != PR_U38R_OK)
        return status;
    value = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));                /* +2964 CMP AL,[BP-2]; JA +2970 */
    cmp8(cpu, (uint8_t)cpu->ax, value);
    if (!((uint8_t)cpu->ax > value)) {
        status = helper(ram, ram_size, cpu, entry, 0x2915u, 0x296Du, 0u, callee_status);
        if (status != PR_U38R_OK)
            return status;
    }
leave:
    cpu->sp = cpu->bp;                                                    /* +2970 MOV SP,BP; POP BP; RETF 2 */
    frame = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, frame);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    frame = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, frame);
    cpu->cs = read16_at(ram, (frame + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_U38R_OK;
}

pr_u38r_status pr_u38_roll_gate_2915_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                         int *callee_status)
{
    note(callee_status, -1);
    if (!valid(ram, ram_size, cpu))
        return PR_U38R_ARGUMENT;
    return gate(ram, ram_size, cpu, max_system_calls, callee_status);
}

/* POP BP; RETF 0Ah. */
static pr_u38r_status leave_entry(const uint8_t *ram, pr_u38r_cpu *cpu)
{
    size_t frame = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, frame);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    frame = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, frame);
    cpu->cs = read16_at(ram, (frame + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_U38R_OK;
}

/* Entries 89/90: PUSH BP; MOV BP,SP; MOV AL,key; PUSH AX; PUSH CS; CALL +2915; POP BP; RETF 0Ah. */
static pr_u38r_status gate_entry(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, uint16_t base, uint8_t key,
                                 unsigned max_system_calls, int *callee_status)
{
    uint16_t entry, caller_cs, return_ip;
    pr_u38r_status status;
    note(callee_status, -1);
    if (!valid(ram, ram_size, cpu))
        return PR_U38R_ARGUMENT;
    entry = cpu->ip;
    caller_cs = cpu->cs;
    return_ip = (uint16_t)(entry + 0x0Au);
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    set_al(cpu, key);
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = (uint16_t)(entry - (uint16_t)(base - 0x2915u));
    status = gate(ram, ram_size, cpu, max_system_calls, callee_status);
    if (status != PR_U38R_OK)
        return status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38R_TRANSFER;
    return leave_entry(ram, cpu);
}

pr_u38r_status pr_u38_entry89_2976_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                       int *callee_status)
{
    return gate_entry(ram, ram_size, cpu, 0x2976u, 0x32u, max_system_calls, callee_status);
}

pr_u38r_status pr_u38_entry90_2984_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                       int *callee_status)
{
    return gate_entry(ram, ram_size, cpu, 0x2984u, 0x0Fu, max_system_calls, callee_status);
}

pr_u38r_status pr_u38_entry91_2992_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                       int *callee_status)
{
    uint16_t entry;
    pr_u38r_status status;
    note(callee_status, -1);
    if (!valid(ram, ram_size, cpu))
        return PR_U38R_ARGUMENT;
    entry = cpu->ip;
    push16(ram, cpu, cpu->bp);                                            /* +2992 PUSH BP; MOV BP,SP */
    cpu->bp = cpu->sp;
    status = roll(ram, ram_size, cpu, entry, 0x2992u, 0x299Bu, 1u, 0x64u, max_system_calls, callee_status);
    if (status != PR_U38R_OK)
        return status;
    cmp8(cpu, (uint8_t)cpu->ax, 0x5Au);                                   /* +29A0 CMP AL,5Ah; JA +29B2 */
    if (!((uint8_t)cpu->ax > 0x5Au)) {
        status = helper(ram, ram_size, cpu, entry, 0x2992u, 0x29A8u, 0x35u, callee_status);
        if (status == PR_U38R_OK)
            status = helper(ram, ram_size, cpu, entry, 0x2992u, 0x29AFu, 0x0Bu, callee_status);
        if (status != PR_U38R_OK)
            return status;
    }
    return leave_entry(ram, cpu);                                         /* +29B2 POP BP; RETF 0Ah */
}
