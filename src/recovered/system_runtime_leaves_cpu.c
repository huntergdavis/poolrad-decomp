/* Turbo Pascal SYSTEM runtime leaves FillChar, Move, set IN and UpCase (raw CPU); docs/re/SYSTEM_RUNTIME_LEAVES.md. */
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#define MASK (PR_SYSTEM_LEAF_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t stack16(const uint8_t *ram, const pr_system_leaf_cpu *cpu, uint16_t disp)
{
    return read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bx + disp)));
}

/* LES/LDS reg,SS:[BX+disp]: the segment word is the next two physical bytes. */
static void far_pointer(const uint8_t *ram, const pr_system_leaf_cpu *cpu, uint16_t disp, uint16_t *offset, uint16_t *segment)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bx + disp));
    *offset = read16_at(ram, at);
    *segment = read16_at(ram, (at + 2u) & MASK);
}

static void retf(const uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t args)
{
    size_t at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + args);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* CMP/SUB r8 flags. */
static void sub8_flags(pr_system_leaf_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

static int bad(const uint8_t *ram, size_t ram_size, const pr_system_leaf_cpu *cpu)
{
    return ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LEAF_RAM_SIZE;
}

pr_system_leaf_status pr_system_fillchar_16a8(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    if (bad(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 16A8 MOV BX,SP */
    far_pointer(ram, cpu, 8u, &cpu->di, &cpu->es);                        /* 16AA LES DI,SS:[BX+8] */
    cpu->cx = stack16(ram, cpu, 6u);                                      /* 16AE MOV CX,SS:[BX+6] */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical(cpu->ss, (uint16_t)(cpu->bx + 4u))]); /* 16B2 MOV AL,SS:[BX+4] */
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 16B6 CLD */
    while (cpu->cx != 0u) {                                               /* 16B7 REP STOSB */
        ram[physical(cpu->es, cpu->di)] = (uint8_t)cpu->ax;
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    retf(ram, cpu, 8u);                                                   /* 16B9 RETF 8 */
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_move_0a3f(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    if (bad(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 0A3F MOV BX,SP */
    cpu->dx = cpu->ds;                                                    /* 0A41 MOV DX,DS */
    far_pointer(ram, cpu, 0x0Au, &cpu->si, &cpu->ds);                     /* 0A43 LDS SI,SS:[BX+0Ah] */
    far_pointer(ram, cpu, 6u, &cpu->di, &cpu->es);                        /* 0A47 LES DI,SS:[BX+6] */
    cpu->cx = stack16(ram, cpu, 4u);                                      /* 0A4B MOV CX,SS:[BX+4] */
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 0A4F CLD */
    while (cpu->cx != 0u) {                                               /* 0A50 REP MOVSB */
        ram[physical(cpu->es, cpu->di)] = ram[physical(cpu->ds, cpu->si)];
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->ds = cpu->dx;                                                    /* 0A52 MOV DS,DX */
    retf(ram, cpu, 0x0Au);                                                /* 0A54 RETF 0Ah */
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_set_in_0dc0(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint8_t element, byte;
    if (bad(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 0DC0 MOV BX,SP */
    far_pointer(ram, cpu, 4u, &cpu->di, &cpu->es);                        /* 0DC2 LES DI,SS:[BX+4] */
    element = ram[physical(cpu->ss, (uint16_t)(cpu->bx + 8u))];           /* 0DC6 MOV AL,SS:[BX+8] */
    cpu->bx = (uint16_t)(element >> 3);                                   /* 0DCA MOV BL,AL; XOR BH,BH; MOV CL,3; SHR BX,CL */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (element & 7u));           /* 0DD2 MOV CL,AL; AND CL,7 */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (uint8_t)(1u << (element & 7u))); /* 0DD7 MOV AL,1; SHL AL,CL */
    byte = (uint8_t)(ram[physical(cpu->es, (uint16_t)(cpu->bx + cpu->di))] & (uint8_t)cpu->ax); /* 0DDB TEST ES:[BX+DI],AL */
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity(byte)) cpu->flags |= 0x0004u;
    if (byte == 0u) cpu->flags |= 0x0040u;
    if (byte & 0x80u) cpu->flags |= 0x0080u;
    retf(ram, cpu, 6u);                                                   /* 0DDE RETF 6 */
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_upcase_182a(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint8_t ch;
    if (bad(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 182A MOV BX,SP */
    ch = ram[physical(cpu->ss, (uint16_t)(cpu->bx + 4u))];                /* 182C MOV AL,SS:[BX+4] */
    sub8_flags(cpu, ch, 0x61u);                                           /* 1830 CMP AL,61h; JB 183A */
    if (ch >= 0x61u) {
        sub8_flags(cpu, ch, 0x7Au);                                       /* 1834 CMP AL,7Ah; JA 183A */
        if (ch <= 0x7Au) {
            sub8_flags(cpu, ch, 0x20u);                                   /* 1838 SUB AL,20h */
            ch = (uint8_t)(ch - 0x20u);
        }
    }
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ch);
    retf(ram, cpu, 2u);                                                   /* 183A RETF 2 */
    return PR_SYSTEM_LEAF_RETURNED;
}
