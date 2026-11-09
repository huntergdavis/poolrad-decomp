/* Resident 00BA:1073 word to string, composing SYSTEM Str and StrStore.
 * Adapted from the 1037 byte formatter; see docs/re/RES_00BA_WORD_STRING_1073.md. */
#include "poolrad/recovered/resident_00ba_word_string_1073_cpu.h"
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

static int ordinary(uint16_t s) { return s < 0x9000u || s >= 0xB000u; }

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(uint8_t *ram, pr_res_1073_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void note(pr_res_1073_callee *callee, int *callee_status, pr_res_1073_callee which, int status)
{
    if (callee != NULL)
        *callee = which;
    if (callee_status != NULL)
        *callee_status = status;
}

/* LCALL xxxx:target at `site` (operand bytes checked, segment read from RAM), pushes, and the CS:IP for the callee;
 * returns 0 at a CODE stop. */
static int far_call(uint8_t *ram, pr_res_1073_cpu *cpu, uint16_t site, uint16_t target)
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

pr_res_1073_status pr_res_00ba_1073_cpu(pr_bus *bus, size_t ram_size, pr_res_1073_cpu *cpu, pr_res_1073_callee *callee,
                                        int *callee_status)
{
    uint8_t *ram;
    pr_system_leaf_cpu str;
    pr_system_0b25_cpu store;
    pr_system_leaf_status str_status;
    pr_system_0b25_status store_status;
    uint16_t caller_cs;
    size_t at;
    note(callee, callee_status, PR_RES_1037_NO_CALLEE, -1);
    if (bus == NULL || bus->ram == NULL || cpu == NULL || ram_size != PR_RES_1037_RAM_SIZE || cpu->ip != 0x1073u ||
        cpu->sp < 0x0200u || cpu->sp > 0xFFF0u || !ordinary(cpu->cs) || !ordinary(cpu->ss))
        return PR_RES_1073_ARGUMENT;
    ram = bus->ram;
    caller_cs = cpu->cs;
    push16(ram, cpu, cpu->bp);                                            /* 1073 PUSH BP; MOV BP,SP; SUB SP,100h (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x100u);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));          /* 107A MOV AX,[BP+6]; XOR DX,DX; PUSH DX; PUSH AX */
    cpu->dx = 0u;
    push16(ram, cpu, cpu->dx);
    push16(ram, cpu, cpu->ax);
    cpu->ax = 0u;                                                         /* 1081 XOR AX,AX; PUSH AX */
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    push16(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 0x100u);                               /* 1084 LEA DI,[BP-100h]; PUSH SS; PUSH DI; MOV AX,0FFh; PUSH AX */
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x00FFu;
    push16(ram, cpu, cpu->ax);
    if (!far_call(ram, cpu, 0x108Eu, 0x1366u))                            /* 108E LCALL xxxx:1366 */
        return PR_RES_1073_CODE;
    if (bus->ega != NULL && !ordinary(cpu->cs))
        return PR_RES_1073_PROFILE;
    COPY_CPU(&str, cpu);
    str_status = pr_system_str_1366(ram, ram_size, &str);
    COPY_CPU(cpu, &str);
    note(callee, callee_status, PR_RES_1037_STR, (int)str_status);
    if (str_status != PR_SYSTEM_LEAF_RETURNED)
        return PR_RES_1073_CALLEE;
    if (cpu->ip != 0x1093u || cpu->cs != caller_cs)
        return PR_RES_1073_TRANSFER;
    cpu->di = (uint16_t)(cpu->bp - 0x100u);                               /* 1093 LEA DI,[BP-100h]; PUSH SS; PUSH DI; LES DI,[BP+8]; */
    push16(ram, cpu, cpu->ss);                                            /*      PUSH ES; PUSH DI; MOV AX,0FFh; PUSH AX */
    push16(ram, cpu, cpu->di);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    push16(ram, cpu, cpu->es);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x00FFu;
    push16(ram, cpu, cpu->ax);
    if (!far_call(ram, cpu, 0x10A2u, 0x0B25u))                            /* 10A2 LCALL xxxx:0B25 */
        return PR_RES_1073_CODE;
    if (bus->ega != NULL && (!ordinary(cpu->cs) ||
        !ordinary(read16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)))))
        return PR_RES_1073_PROFILE;
    COPY_CPU(&store, cpu);
    store_status = pr_system_strstore_0b25(ram, ram_size, &store);
    COPY_CPU(cpu, &store);
    note(callee, callee_status, PR_RES_1037_STRSTORE, (int)store_status);
    if (store_status != PR_SYSTEM_0B25_RETURNED)
        return PR_RES_1073_CALLEE;
    if (cpu->ip != 0x10A7u || cpu->cs != caller_cs)
        return PR_RES_1073_TRANSFER;
    cpu->sp = cpu->bp;                                                    /* 10A7 MOV SP,BP; POP BP; RETF 2 */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    return PR_RES_1073_OK;
}
