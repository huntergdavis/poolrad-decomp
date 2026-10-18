/* GAME.OVR unit 0x7D +0565 two-digit string (raw CPU, composing resident 00BA:1037 and the SYSTEM string leaves);
 * docs/re/U7D_TWO_DIGITS_0565_CPU.md. */
#include "poolrad/recovered/overlay_u7d_two_digits_0565_cpu.h"
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_U7D_0565_RAM_SIZE - 1u)

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u7d_0565_cpu *cpu;
    pr_u7d_0565_callee *callee;
    int *callee_status;
    uint16_t caller_cs;
    pr_u7d_0565_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(machine *m, uint16_t value)
{
    size_t at;
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    at = physical(m->cpu->ss, m->cpu->sp);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

/* LEA DI,[BP-displacement]; PUSH SS; PUSH DI. */
static void push_local(machine *m, uint16_t displacement)
{
    m->cpu->di = (uint16_t)(m->cpu->bp - displacement);
    push16(m, m->cpu->ss);
    push16(m, m->cpu->di);
}

/* MOV AX,word; PUSH AX. */
static void push_ax(machine *m, uint16_t value)
{
    m->cpu->ax = value;
    push16(m, value);
}

static void note(machine *m, pr_u7d_0565_callee which, int status)
{
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
}

/* LCALL xxxx:target at `site`: checks 9A and the offset, reads the segment from RAM, pushes CS and site + 5, and runs
 * the callee; returns 1 when it came back to site + 5 in this CS. */
static int far_call(machine *m, uint16_t site, uint16_t target, pr_u7d_0565_callee which)
{
    pr_u7d_0565_cpu *cpu = m->cpu;
    uint16_t segment;
    int status, returned;
    cpu->ip = site;
    if (m->ram[physical(cpu->cs, site)] != 0x9Au || read16(m->ram, cpu->cs, (uint16_t)(site + 1u)) != target) {
        m->stop = PR_U7D_0565_CODE;
        return 0;
    }
    segment = read16(m->ram, cpu->cs, (uint16_t)(site + 3u));
    push16(m, cpu->cs);
    push16(m, (uint16_t)(site + 5u));
    cpu->cs = segment;
    cpu->ip = target;
    if (which == PR_U7D_0565_BYTE_STRING) {
        pr_res_1037_cpu state;
        COPY_CPU(&state, cpu);
        status = (int)pr_res_00ba_1037_cpu(m->ram, m->size, &state, NULL, NULL);
        COPY_CPU(cpu, &state);
        returned = status == (int)PR_RES_1037_OK;
    } else if (which == PR_U7D_0565_STRSTORE) {
        pr_system_0b25_cpu state;
        COPY_CPU(&state, cpu);
        status = (int)pr_system_strstore_0b25(m->ram, m->size, &state);
        COPY_CPU(cpu, &state);
        returned = status == (int)PR_SYSTEM_0B25_RETURNED;
    } else {
        pr_system_leaf_cpu state;
        COPY_CPU(&state, cpu);
        if (which == PR_U7D_0565_STRLOAD)
            status = (int)pr_system_strload_0b0b(m->ram, m->size, &state);
        else if (which == PR_U7D_0565_STRCAT)
            status = (int)pr_system_strcat_0b98(m->ram, m->size, &state);
        else
            status = (int)pr_system_strcopy_0b57(m->ram, m->size, &state);
        COPY_CPU(cpu, &state);
        returned = status == (int)PR_SYSTEM_LEAF_RETURNED;
    }
    note(m, which, status);
    if (!returned) {
        m->stop = PR_U7D_0565_CALLEE;
        return 0;
    }
    if (cpu->ip != (uint16_t)(site + 5u) || cpu->cs != m->caller_cs) {
        m->stop = PR_U7D_0565_TRANSFER;
        return 0;
    }
    return 1;
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* SUB/CMP of `sign`-bit values: the six arithmetic flags; the other flags are kept. */
static void sub_flags(pr_u7d_0565_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t result = (uint16_t)((a - b) & (uint16_t)((sign << 1) - 1u));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
}

pr_u7d_0565_status pr_u7d_two_digits_0565_cpu(uint8_t *ram, size_t ram_size, pr_u7d_0565_cpu *cpu, pr_u7d_0565_callee *callee,
                                              int *callee_status)
{
    machine m;
    uint8_t b;
    size_t at;
    if (callee != NULL)
        *callee = PR_U7D_0565_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7D_0565_RAM_SIZE || cpu->ip != 0x0565u)
        return PR_U7D_0565_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.callee = callee; m.callee_status = callee_status;
    m.caller_cs = cpu->cs; m.stop = PR_U7D_0565_OK;
    push16(&m, cpu->bp);                                                  /* 0565 PUSH BP; MOV BP,SP; SUB SP,12Ah */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 0x012Au, 0x8000u);
    cpu->sp = (uint16_t)(cpu->sp - 0x012Au);
    push_local(&m, 0x0129u);                                              /* 056C LEA DI,[BP-129h]; PUSH SS; PUSH DI; */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))]);  /* MOV AL,[BP+6]; PUSH AX */
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x0576u, 0x1037u, PR_U7D_0565_BYTE_STRING))         /* 0576 LCALL 00BA:1037 */
        return m.stop;
    push_local(&m, 0x0029u);                                              /* 057B s := T (StrStore, 40) */
    push_ax(&m, 0x0028u);
    if (!far_call(&m, 0x0584u, 0x0B25u, PR_U7D_0565_STRSTORE))
        return m.stop;
    b = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];                 /* 0589 CMP BYTE [BP+6],0Ah; JAE 05B7 */
    sub_flags(cpu, b, 0x0Au, 0x0080u);
    if (b < 0x0Au) {
        push_local(&m, 0x0129u);                                          /* 058F T := '0' (StrLoad from CS:0563) */
        cpu->di = 0x0563u;
        push16(&m, cpu->cs);
        push16(&m, cpu->di);
        if (!far_call(&m, 0x059Au, 0x0B0Bu, PR_U7D_0565_STRLOAD))
            return m.stop;
        push_local(&m, 0x0029u);                                          /* 059F T := T + s (StrConcat) */
        if (!far_call(&m, 0x05A4u, 0x0B98u, PR_U7D_0565_STRCAT))
            return m.stop;
        push_local(&m, 0x0029u);                                          /* 05A9 s := T (StrStore, 40) */
        push_ax(&m, 0x0028u);
        if (!far_call(&m, 0x05B2u, 0x0B25u, PR_U7D_0565_STRSTORE))
            return m.stop;
    }
    push_local(&m, 0x0129u);                                              /* 05B7 T := Copy(s, 1, 2) */
    push_local(&m, 0x0029u);
    push_ax(&m, 0x0001u);
    push_ax(&m, 0x0002u);
    if (!far_call(&m, 0x05CAu, 0x0B57u, PR_U7D_0565_COPY))
        return m.stop;
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));                    /* 05CF LES DI,[BP+8]; R := T (StrStore, 40) */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    push16(&m, cpu->es);
    push16(&m, cpu->di);
    push_ax(&m, 0x0028u);
    if (!far_call(&m, 0x05D8u, 0x0B25u, PR_U7D_0565_STRSTORE))
        return m.stop;
    cpu->sp = cpu->bp;                                                    /* 05DD MOV SP,BP; POP BP; RETF 2 */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_U7D_0565_OK;
}
