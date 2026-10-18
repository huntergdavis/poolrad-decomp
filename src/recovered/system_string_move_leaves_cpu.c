/* START.EXE SYSTEM leaves 0AF8:0B57 (Copy), 0C10 (string compare), 0C3B (char to string) and 1685 (Move), raw CPU;
 * docs/re/SYSTEM_STRING_MOVE_LEAVES.md. */
#include "poolrad/recovered/system_string_move_leaves_cpu.h"

#define MASK (PR_SYSTEM_LEAF_RAM_SIZE - 1u)
#define ARITHMETIC 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static int valid(const uint8_t *ram, size_t ram_size, const pr_system_leaf_cpu *cpu)
{
    return ram != NULL && cpu != NULL && ram_size == PR_SYSTEM_LEAF_RAM_SIZE;
}

/* CMP/SUB of `width`-bit values (sign bit `sign`): all six arithmetic flags. */
static void sub_flags(pr_system_leaf_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t mask = (uint16_t)((sign << 1) - 1u), result = (uint16_t)((a - b) & mask);
    cpu->flags &= (uint16_t)~ARITHMETIC;
    if (a < b) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
}

/* XOR of a register with itself: CF = OF = SF = AF = 0, ZF = PF = 1. */
static void xor_self_flags(pr_system_leaf_cpu *cpu)
{
    cpu->flags = (uint16_t)((cpu->flags & ~ARITHMETIC) | 0x0044u);
}

/* LES/LDS from SS:[BX+displacement], the pair read physically contiguous. */
static void pointer(const uint8_t *ram, const pr_system_leaf_cpu *cpu, uint16_t displacement, uint16_t *offset, uint16_t *segment)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bx + displacement));
    *offset = read16_at(ram, at);
    *segment = read16_at(ram, (at + 2u) & MASK);
}

static uint16_t frame_word(const uint8_t *ram, const pr_system_leaf_cpu *cpu, uint16_t displacement)
{
    return read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bx + displacement)));
}

/* REP MOVSB one byte at a time, forward when DF is clear, backward when set; CX ends 0. */
static void rep_movsb(uint8_t *ram, pr_system_leaf_cpu *cpu)
{
    uint16_t step = (cpu->flags & 0x0400u) ? 0xFFFFu : 1u;
    while (cpu->cx != 0u) {
        ram[physical(cpu->es, cpu->di)] = ram[physical(cpu->ds, cpu->si)];
        cpu->si = (uint16_t)(cpu->si + step);
        cpu->di = (uint16_t)(cpu->di + step);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
}

/* MOV DS,DX; RETF n: the frame read physically contiguous. */
static void retf(const uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t pop)
{
    size_t at = physical(cpu->ss, cpu->sp);
    cpu->ds = cpu->dx;
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + pop);
}

pr_system_leaf_status pr_system_strcopy_0b57(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint16_t index, count;
    if (!valid(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 0B57 CLD; MOV BX,SP; MOV DX,DS */
    cpu->bx = cpu->sp;
    cpu->dx = cpu->ds;
    pointer(ram, cpu, 0x000Cu, &cpu->di, &cpu->es);                       /* 0B5C LES DI,SS:[BX+0Ch]; LDS SI,SS:[BX+8] */
    pointer(ram, cpu, 0x0008u, &cpu->si, &cpu->ds);
    cpu->ax = ram[physical(cpu->ds, cpu->si)];                            /* 0B64 MOV AL,[SI]; XOR AH,AH (flags folded) */
    index = frame_word(ram, cpu, 0x0006u);                                /* 0B68 MOV CX,SS:[BX+6]; OR CX,CX; JG 0B73 */
    if (index == 0u || (index & 0x8000u))
        index = 1u;                                                       /* 0B70 MOV CX,1 */
    cpu->cx = index;
    cpu->si = (uint16_t)(cpu->si + index);                                /* 0B73 ADD SI,CX; SUB AX,CX; JB 0B8C */
    if (cpu->ax < index) {
        cpu->ax = 0u;                                                     /* 0B8C XOR AX,AX */
        xor_self_flags(cpu);
    } else {
        cpu->ax = (uint16_t)(cpu->ax - index + 1u);                       /* 0B79 INC AX; MOV CX,SS:[BX+4]; OR CX,CX; JGE 0B84 */
        count = frame_word(ram, cpu, 0x0004u);
        if (count & 0x8000u)
            count = 0u;                                                   /* 0B82 XOR CX,CX */
        cpu->cx = count;
        sub_flags(cpu, cpu->ax, cpu->cx, 0x8000u);                        /* 0B84 CMP AX,CX; JBE 0B8E; MOV AX,CX */
        if (cpu->ax > cpu->cx)
            cpu->ax = cpu->cx;
    }
    ram[physical(cpu->es, cpu->di)] = (uint8_t)cpu->ax;                   /* 0B8E STOSB; MOV CX,AX; REP MOVSB */
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->cx = cpu->ax;
    rep_movsb(ram, cpu);
    retf(ram, cpu, 8u);                                                   /* 0B93 MOV DS,DX; RETF 8 */
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_strcmp_0c10(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint8_t left, right, shorter;
    if (!valid(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 0C10 CLD; MOV BX,SP; MOV DX,DS */
    cpu->bx = cpu->sp;
    cpu->dx = cpu->ds;
    pointer(ram, cpu, 0x0008u, &cpu->si, &cpu->ds);                       /* 0C15 LDS SI,SS:[BX+8]; LES DI,SS:[BX+4] */
    pointer(ram, cpu, 0x0004u, &cpu->di, &cpu->es);
    left = ram[physical(cpu->ds, cpu->si)];                               /* 0C1D LODSB; MOV AH,ES:[DI]; INC DI */
    cpu->si = (uint16_t)(cpu->si + 1u);
    right = ram[physical(cpu->es, cpu->di)];
    cpu->ax = (uint16_t)(((uint16_t)right << 8) | left);
    cpu->di = (uint16_t)(cpu->di + 1u);
    shorter = left <= right ? left : right;                               /* 0C22 MOV CL,AL; CMP CL,AH; JBE; MOV CL,AH */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | shorter);
    if (shorter != 0u) {                                                  /* 0C2A OR CL,CL; JE 0C34 */
        cpu->cx = shorter;                                                /* 0C2E XOR CH,CH; REPE CMPSB */
        do {
            sub_flags(cpu, ram[physical(cpu->ds, cpu->si)], ram[physical(cpu->es, cpu->di)], 0x0080u);
            cpu->si = (uint16_t)(cpu->si + 1u);
            cpu->di = (uint16_t)(cpu->di + 1u);
            cpu->cx = (uint16_t)(cpu->cx - 1u);
        } while (cpu->cx != 0u && (cpu->flags & 0x0040u));
    }
    if (shorter == 0u || (cpu->flags & 0x0040u))                          /* 0C32 JNE 0C36 */
        sub_flags(cpu, left, right, 0x0080u);                             /* 0C34 CMP AL,AH */
    retf(ram, cpu, 8u);                                                   /* 0C36 MOV DS,DX; RETF 8 */
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_charstr_0c3b(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    if (!valid(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 0C3B CLD; MOV BX,SP; LES DI,SS:[BX+6] */
    cpu->bx = cpu->sp;
    pointer(ram, cpu, 0x0006u, &cpu->di, &cpu->es);
    ram[physical(cpu->es, cpu->di)] = 1u;                                 /* 0C42 MOV AL,1; STOSB */
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical(cpu->ss, (uint16_t)(cpu->bx + 4u))]);  /* 0C45 MOV AL,SS:[BX+4] */
    ram[physical(cpu->es, cpu->di)] = (uint8_t)cpu->ax;                   /* 0C49 STOSB */
    cpu->di = (uint16_t)(cpu->di + 1u);
    {
        size_t at = physical(cpu->ss, cpu->sp);                           /* 0C4A RETF 2 */
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
        cpu->sp = (uint16_t)(cpu->sp + 4u + 2u);
    }
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_move_1685(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint32_t wide;
    if (!valid(ram, ram_size, cpu))
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 1685 MOV BX,SP; MOV DX,DS */
    cpu->dx = cpu->ds;
    pointer(ram, cpu, 0x000Au, &cpu->si, &cpu->ds);                       /* 1689 LDS SI,SS:[BX+0Ah]; LES DI,SS:[BX+6] */
    pointer(ram, cpu, 0x0006u, &cpu->di, &cpu->es);
    cpu->cx = frame_word(ram, cpu, 0x0004u);                              /* 1691 MOV CX,SS:[BX+4]; CLD */
    cpu->flags &= (uint16_t)~0x0400u;
    sub_flags(cpu, cpu->si, cpu->di, 0x8000u);                            /* 1696 CMP SI,DI; JAE 16A1 */
    if (cpu->si < cpu->di) {
        cpu->si = (uint16_t)(cpu->si + cpu->cx);                          /* 169A ADD SI,CX; ADD DI,CX (its CF stays) */
        wide = (uint32_t)cpu->di + cpu->cx;
        cpu->di = (uint16_t)wide;
        cpu->si = (uint16_t)(cpu->si - 1u);                               /* 169E DEC SI; DEC DI: the other flags */
        cpu->di = (uint16_t)(cpu->di - 1u);
        cpu->flags &= (uint16_t)~ARITHMETIC;
        if (wide > 0xFFFFu) cpu->flags |= 0x0001u;
        if (even_parity((uint8_t)cpu->di)) cpu->flags |= 0x0004u;
        if ((cpu->di & 0x000Fu) == 0x000Fu) cpu->flags |= 0x0010u;
        if (cpu->di == 0u) cpu->flags |= 0x0040u;
        if (cpu->di & 0x8000u) cpu->flags |= 0x0080u;
        if (cpu->di == 0x7FFFu) cpu->flags |= 0x0800u;
        cpu->flags |= 0x0400u;                                            /* 16A0 STD */
    }
    rep_movsb(ram, cpu);                                                  /* 16A1 REP MOVSB */
    retf(ram, cpu, 10u);                                                  /* 16A3 MOV DS,DX; RETF 0Ah */
    return PR_SYSTEM_LEAF_RETURNED;
}
