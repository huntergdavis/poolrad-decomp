/* Resident 00BA:1037 byte to string (raw CPU, composing SYSTEM Str 1366 and StrStore 0B25);
 * docs/re/SYSTEM_STR_1366_RES_1037.md. */
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"
#include "poolrad/recovered/system_str_1366_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_RES_1037_RAM_SIZE - 1u)

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

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

static void push16(uint8_t *ram, pr_res_1037_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void note(pr_res_1037_callee *callee, int *callee_status, pr_res_1037_callee which, int status)
{
    if (callee != NULL)
        *callee = which;
    if (callee_status != NULL)
        *callee_status = status;
}

/* LCALL xxxx:target at `site` (operand bytes checked, segment read from RAM), pushes, and the CS:IP for the callee;
 * returns 0 at a CODE stop. */
static int far_call(uint8_t *ram, pr_res_1037_cpu *cpu, uint16_t site, uint16_t target)
{
    uint16_t segment;
    cpu->ip = site;
    if (ram[physical(cpu->cs, site)] != 0x9Au || read16(ram, cpu->cs, (uint16_t)(site + 1u)) != target)
        return 0;
    segment = read16(ram, cpu->cs, (uint16_t)(site + 3u));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, (uint16_t)(site + 5u));
    cpu->cs = segment;
    cpu->ip = target;
    return 1;
}

pr_res_1037_status pr_res_00ba_1037_cpu(uint8_t *ram, size_t ram_size, pr_res_1037_cpu *cpu, pr_res_1037_callee *callee,
                                        int *callee_status)
{
    pr_system_leaf_cpu str;
    pr_system_0b25_cpu store;
    pr_system_leaf_status str_status;
    pr_system_0b25_status store_status;
    uint16_t caller_cs;
    size_t at;
    note(callee, callee_status, PR_RES_1037_NO_CALLEE, -1);
    if (ram == NULL || cpu == NULL || ram_size != PR_RES_1037_RAM_SIZE || cpu->ip != 0x1037u)
        return PR_RES_1037_ARGUMENT;
    caller_cs = cpu->cs;
    push16(ram, cpu, cpu->bp);                                            /* 1037 PUSH BP; MOV BP,SP; SUB SP,100h (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x100u);
    cpu->ax = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];          /* 103E MOV AL,[BP+6]; XOR AH,AH; XOR DX,DX; PUSH DX; PUSH AX */
    cpu->dx = 0u;
    push16(ram, cpu, cpu->dx);
    push16(ram, cpu, cpu->ax);
    cpu->ax = 0u;                                                         /* 1047 XOR AX,AX; PUSH AX */
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 0x100u);                               /* 104A LEA DI,[BP-100h]; PUSH SS; PUSH DI; MOV AX,0FFh; PUSH AX */
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x00FFu;
    push16(ram, cpu, cpu->ax);
    if (!far_call(ram, cpu, 0x1054u, 0x1366u))                            /* 1054 LCALL xxxx:1366 */
        return PR_RES_1037_CODE;
    COPY_CPU(&str, cpu);
    str_status = pr_system_str_1366(ram, ram_size, &str);
    COPY_CPU(cpu, &str);
    note(callee, callee_status, PR_RES_1037_STR, (int)str_status);
    if (str_status != PR_SYSTEM_LEAF_RETURNED)
        return PR_RES_1037_CALLEE;
    if (cpu->ip != 0x1059u || cpu->cs != caller_cs)
        return PR_RES_1037_TRANSFER;
    cpu->di = (uint16_t)(cpu->bp - 0x100u);                               /* 1059 LEA DI,[BP-100h]; PUSH SS; PUSH DI; LES DI,[BP+8]; */
    push16(ram, cpu, cpu->ss);                                            /*      PUSH ES; PUSH DI; MOV AX,0FFh; PUSH AX */
    push16(ram, cpu, cpu->di);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    push16(ram, cpu, cpu->es);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x00FFu;
    push16(ram, cpu, cpu->ax);
    if (!far_call(ram, cpu, 0x1068u, 0x0B25u))                            /* 1068 LCALL xxxx:0B25 */
        return PR_RES_1037_CODE;
    COPY_CPU(&store, cpu);
    store_status = pr_system_strstore_0b25(ram, ram_size, &store);
    COPY_CPU(cpu, &store);
    note(callee, callee_status, PR_RES_1037_STRSTORE, (int)store_status);
    if (store_status != PR_SYSTEM_0B25_RETURNED)
        return PR_RES_1037_CALLEE;
    if (cpu->ip != 0x106Du || cpu->cs != caller_cs)
        return PR_RES_1037_TRANSFER;
    cpu->sp = cpu->bp;                                                    /* 106D MOV SP,BP; POP BP; RETF 2 */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_RES_1037_OK;
}
