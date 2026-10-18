/* START.EXE SYSTEM Text Assign 0AF8:04F7 and Insert 0AF8:0C68, raw CPU; docs/re/SYSTEM_TEXTASSIGN_INSERT.md. */
#include "poolrad/recovered/system_textassign_insert_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_SYSTEM_LEAF_RAM_SIZE - 1u)

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

static void write16_at(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16_at(ram, physical(cpu->ss, cpu->sp), value);
}

static void stosw(uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t value)
{
    write16_at(ram, physical(cpu->es, cpu->di), value);
    cpu->di = (uint16_t)(cpu->di + 2u);
}

pr_system_leaf_status pr_system_text_assign_04f7(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    size_t at;
    uint16_t i;
    uint8_t length;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LEAF_RAM_SIZE)
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 04F7 MOV BX,SP; PUSH DS */
    push16(ram, cpu, cpu->ds);
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 8u));                    /* 04FA LES DI,SS:[BX+8]; LDS SI,SS:[BX+4]; CLD */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->si = read16_at(ram, at);
    cpu->ds = read16_at(ram, (at + 2u) & MASK);
    cpu->flags &= (uint16_t)~0x0400u;
    stosw(ram, cpu, 0u);                                                  /* 0503 handle 0, mode D7B0h, buffer size 80h, three 0s */
    stosw(ram, cpu, 0xD7B0u);
    stosw(ram, cpu, 0x0080u);
    stosw(ram, cpu, 0u);
    stosw(ram, cpu, 0u);
    stosw(ram, cpu, 0u);
    cpu->ax = (uint16_t)(cpu->di + 0x74u);                                /* 0513 LEA AX,[DI+74h]; STOSW; MOV AX,ES; STOSW */
    stosw(ram, cpu, cpu->ax);
    cpu->ax = cpu->es;
    stosw(ram, cpu, cpu->ax);
    cpu->ax = 0x060Eu;                                                    /* 051A MOV AX,060Eh; STOSW; MOV AX,CS; STOSW */
    stosw(ram, cpu, cpu->ax);
    cpu->ax = cpu->cs;
    stosw(ram, cpu, cpu->ax);
    cpu->ax = 0u;                                                         /* 0521 XOR AX,AX; MOV CX,0Eh; REP STOSW */
    for (i = 0; i < 0x0Eu; ++i)
        stosw(ram, cpu, 0u);
    length = ram[physical(cpu->ds, cpu->si)];                             /* 0528 LODSB; CMP AL,4Fh; JBE; MOV AL,4Fh (flags folded) */
    cpu->si = (uint16_t)(cpu->si + 1u);
    if (length > 0x4Fu)
        length = 0x4Fu;
    cpu->cx = length;                                                     /* 052F MOV CL,AL; XOR CH,CH; REP MOVSB */
    while (cpu->cx != 0u) {
        ram[physical(cpu->es, cpu->di)] = ram[physical(cpu->ds, cpu->si)];
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->ax = 0u;                                                         /* 0535 XOR AL,AL; STOSB */
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    ram[physical(cpu->es, cpu->di)] = 0u;
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->ds = read16_at(ram, physical(cpu->ss, cpu->sp));                 /* 0538 POP DS; RETF 8 */
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 8u);
    return PR_SYSTEM_LEAF_RETURNED;
}

/* PUSH CS; CALL target from `site` (the CALL's address); runs the leaf and checks its RETF came back. */
static int near_call(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu, uint16_t site, uint16_t target)
{
    uint16_t caller_cs = cpu->cs, back = (uint16_t)(site + 3u);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, back);
    cpu->ip = target;
    if (target == 0x0B57u) {
        (void)pr_system_strcopy_0b57(ram, ram_size, cpu);
    } else if (target == 0x0B98u) {
        (void)pr_system_strcat_0b98(ram, ram_size, cpu);
    } else {
        pr_system_0b25_cpu state;
        COPY_CPU(&state, cpu);
        (void)pr_system_strstore_0b25(ram, ram_size, &state);
        COPY_CPU(cpu, &state);
    }
    return cpu->ip == back && cpu->cs == caller_cs;
}

/* LES DI,[BP+displacement]; PUSH ES; PUSH DI. */
static void push_far(uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t displacement)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp + displacement));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    push16(ram, cpu, cpu->es);
    push16(ram, cpu, cpu->di);
}

/* LEA DI,[BP-displacement]; PUSH SS; PUSH DI. */
static void push_local(uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t displacement)
{
    cpu->di = (uint16_t)(cpu->bp - displacement);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
}

pr_system_insert_status pr_system_insert_0c68(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LEAF_RAM_SIZE || cpu->ip != 0x0C68u)
        return PR_SYSTEM_INSERT_ARGUMENT;
    push16(ram, cpu, cpu->bp);                                            /* 0C68 PUSH BP; MOV BP,SP; SUB SP,200h (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x200u);
    push_local(ram, cpu, 0x0100u);                                        /* 0C6F T1 := Copy(S, 1, Index - 1) */
    push_far(ram, cpu, 0x000Au);
    cpu->ax = 1u;
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)(read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u))) - 1u);  /* MOV AX,[BP+6]; DEC AX (flags folded) */
    push16(ram, cpu, cpu->ax);
    if (!near_call(ram, ram_size, cpu, 0x0C84u, 0x0B57u))
        return PR_SYSTEM_INSERT_TRANSFER;
    push_far(ram, cpu, 0x000Eu);                                          /* 0C87 T1 := T1 + Source */
    if (!near_call(ram, ram_size, cpu, 0x0C8Du, 0x0B98u))
        return PR_SYSTEM_INSERT_TRANSFER;
    push_local(ram, cpu, 0x0200u);                                        /* 0C90 T2 := Copy(S, Index, 255) */
    push_far(ram, cpu, 0x000Au);
    push16(ram, cpu, read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u))));
    cpu->ax = 0x00FFu;
    push16(ram, cpu, cpu->ax);
    if (!near_call(ram, ram_size, cpu, 0x0CA3u, 0x0B57u))
        return PR_SYSTEM_INSERT_TRANSFER;
    if (!near_call(ram, ram_size, cpu, 0x0CA7u, 0x0B98u))                 /* 0CA6 T1 := T1 + T2 */
        return PR_SYSTEM_INSERT_TRANSFER;
    push_far(ram, cpu, 0x000Au);                                          /* 0CAA S := T1 (StrStore with M) */
    push16(ram, cpu, read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 8u))));
    if (!near_call(ram, ram_size, cpu, 0x0CB3u, 0x0B25u))
        return PR_SYSTEM_INSERT_TRANSFER;
    cpu->sp = cpu->bp;                                                    /* 0CB6 MOV SP,BP; POP BP; RETF 0Ch */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 12u);
    return PR_SYSTEM_INSERT_RETURNED;
}
