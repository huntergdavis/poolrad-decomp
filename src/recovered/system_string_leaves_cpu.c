/* START.EXE SYSTEM string leaves 0AF8:0B0B (load) and 0AF8:0B98 (concatenation), raw CPU; docs/re/SYSTEM_STRING_LEAVES.md. */
#include "poolrad/recovered/system_string_leaves_cpu.h"

#define MASK (PR_SYSTEM_LEAF_RAM_SIZE - 1u)

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

/* CLD; MOV BX,SP; MOV DX,DS; LES DI,SS:[BX+8]; LDS SI,SS:[BX+4]: the pairs read physically contiguous. */
static void open_frame(const uint8_t *ram, pr_system_leaf_cpu *cpu)
{
    size_t at;
    cpu->flags &= (uint16_t)~0x0400u;
    cpu->bx = cpu->sp;
    cpu->dx = cpu->ds;
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->si = read16_at(ram, at);
    cpu->ds = read16_at(ram, (at + 2u) & MASK);
}

/* REP MOVSB, forward (DF clear), one byte at a time; CX ends 0. */
static void rep_movsb(uint8_t *ram, pr_system_leaf_cpu *cpu)
{
    while (cpu->cx != 0u) {
        ram[physical(cpu->es, cpu->di)] = ram[physical(cpu->ds, cpu->si)];
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
}

/* MOV DS,DX; RETF 4: the frame read physically contiguous. */
static void close_frame(const uint8_t *ram, pr_system_leaf_cpu *cpu)
{
    size_t at = physical(cpu->ss, cpu->sp);
    cpu->ds = cpu->dx;
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
}

pr_system_leaf_status pr_system_strload_0b0b(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint8_t length;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LEAF_RAM_SIZE)
        return PR_SYSTEM_LEAF_ARGUMENT;
    open_frame(ram, cpu);                                                 /* 0B0B..0B14 */
    length = ram[physical(cpu->ds, cpu->si)];                             /* 0B18 LODSB; STOSB */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | length);
    cpu->si = (uint16_t)(cpu->si + 1u);
    ram[physical(cpu->es, cpu->di)] = length;
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->cx = length;                                                     /* 0B1A MOV CL,AL; XOR CH,CH */
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    rep_movsb(ram, cpu);                                                  /* 0B1E REP MOVSB */
    close_frame(ram, cpu);                                                /* 0B20 MOV DS,DX; RETF 4 */
    return PR_SYSTEM_LEAF_RETURNED;
}

pr_system_leaf_status pr_system_strcat_0b98(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint8_t old_length, added, sum;
    uint32_t wide;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LEAF_RAM_SIZE)
        return PR_SYSTEM_LEAF_ARGUMENT;
    open_frame(ram, cpu);                                                 /* 0B98..0BA1 */
    old_length = ram[physical(cpu->es, cpu->di)];                         /* 0BA5 MOV CL,ES:[DI]; XOR CH,CH */
    cpu->cx = old_length;
    added = ram[physical(cpu->ds, cpu->si)];                              /* 0BAA LODSB */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | added);
    cpu->si = (uint16_t)(cpu->si + 1u);
    sum = (uint8_t)(ram[physical(cpu->es, cpu->di)] + added);             /* 0BAB ADD ES:[DI],AL; JAE 0BB8 */
    if ((unsigned)ram[physical(cpu->es, cpu->di)] + added > 0xFFu) {
        ram[physical(cpu->es, cpu->di)] = sum;
        ram[physical(cpu->es, cpu->di)] = 0xFFu;                          /* 0BB0 MOV BYTE ES:[DI],0FFh; MOV AL,CL; NOT AL */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (uint8_t)~(uint8_t)cpu->cx);
    } else {
        ram[physical(cpu->es, cpu->di)] = sum;
    }
    wide = (uint32_t)cpu->di + cpu->cx;                                   /* 0BB8 ADD DI,CX; INC DI */
    cpu->di = (uint16_t)(wide + 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (wide > 0xFFFFu) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)cpu->di)) cpu->flags |= 0x0004u;
    if ((cpu->di & 0x000Fu) == 0u) cpu->flags |= 0x0010u;
    if (cpu->di == 0u) cpu->flags |= 0x0040u;
    if (cpu->di & 0x8000u) cpu->flags |= 0x0080u;
    if (cpu->di == 0x8000u) cpu->flags |= 0x0800u;
    cpu->cx = (uint16_t)(cpu->ax & 0x00FFu);                              /* 0BBB MOV CL,AL (CH stays 0) */
    rep_movsb(ram, cpu);                                                  /* 0BBD REP MOVSB */
    close_frame(ram, cpu);                                                /* 0BBF MOV DS,DX; RETF 4 */
    return PR_SYSTEM_LEAF_RETURNED;
}
