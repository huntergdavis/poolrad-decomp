/* START.EXE SYSTEM IOResult 0AF8:04A2 and Assign 0AF8:13E2, raw CPU; docs/re/SYSTEM_IORESULT_ASSIGN.md. */
#include "poolrad/recovered/system_ioresult_assign_cpu.h"

#define MASK (PR_SYSTEM_LEAF_RAM_SIZE - 1u)

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

static int valid(const uint8_t *ram, size_t ram_size, const pr_system_leaf_cpu *cpu)
{
    return ram != NULL && cpu != NULL && ram_size == PR_SYSTEM_LEAF_RAM_SIZE;
}

/* XOR of a register with itself: CF = OF = SF = AF = 0, ZF = PF = 1. */
static void xor_self_flags(pr_system_leaf_cpu *cpu)
{
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
}

static void retf(const uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t pop)
{
    size_t at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + pop);
}

/* STOSW, forward. */
static void stosw(uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t value)
{
    write16_at(ram, physical(cpu->es, cpu->di), value);
    cpu->di = (uint16_t)(cpu->di + 2u);
}

pr_system_leaf_status pr_system_ioresult_04a2(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    size_t at;
    if (!valid(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    xor_self_flags(cpu);                                                  /* 04A2 XOR AX,AX; XCHG [4426h],AX */
    at = physical(cpu->ds, 0x4426u);
    cpu->ax = read16_at(ram, at);
    write16_at(ram, at, 0u);
    retf(ram, cpu, 0u);                                                   /* 04A8 RETF */
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_assign_13e2(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    size_t at;
    uint16_t i;
    uint8_t length;
    if (!valid(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 13E2 MOV BX,SP; PUSH DS */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16_at(ram, physical(cpu->ss, cpu->sp), cpu->ds);
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 8u));                    /* 13E5 LES DI,SS:[BX+8]; LDS SI,SS:[BX+4] */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->si = read16_at(ram, at);
    cpu->ds = read16_at(ram, (at + 2u) & MASK);
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 13ED CLD; XOR AX,AX; STOSW; MOV AX,0D7B0h; STOSW */
    stosw(ram, cpu, 0u);
    stosw(ram, cpu, 0xD7B0u);
    cpu->ax = 0u;                                                         /* 13F5 XOR AX,AX; MOV CX,16h; REP STOSW */
    for (i = 0; i < 0x16u; ++i)
        stosw(ram, cpu, 0u);
    cpu->cx = 0u;
    length = ram[physical(cpu->ds, cpu->si)];                             /* 13FC LODSB; CMP AL,4Fh; JBE; MOV AL,4Fh (flags folded) */
    cpu->si = (uint16_t)(cpu->si + 1u);
    if (length > 0x4Fu)
        length = 0x4Fu;
    cpu->cx = length;                                                     /* 1403 MOV CL,AL; XOR CH,CH; REP MOVSB */
    while (cpu->cx != 0u) {
        ram[physical(cpu->es, cpu->di)] = ram[physical(cpu->ds, cpu->si)];
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->ax = 0u;                                                         /* 1409 XOR AL,AL; STOSB */
    xor_self_flags(cpu);
    ram[physical(cpu->es, cpu->di)] = 0u;
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->ds = read16_at(ram, physical(cpu->ss, cpu->sp));                 /* 140C POP DS; RETF 8 */
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    retf(ram, cpu, 8u);
    return PR_SYSTEM_LEAF_RETURNED;
}
