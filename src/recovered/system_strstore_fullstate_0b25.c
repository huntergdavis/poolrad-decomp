/* Turbo Pascal SYSTEM 0AF8:0B25 string store (raw CPU); docs/re/SYSTEM_STRSTORE_0B25.md. */
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_SYSTEM_0B25_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

pr_system_0b25_status pr_system_strstore_0b25(uint8_t *ram, size_t ram_size, pr_system_0b25_cpu *cpu)
{
    size_t at;
    uint8_t length;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_0B25_RAM_SIZE)
        return PR_SYSTEM_0B25_ARGUMENT;
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 0B25 CLD */
    cpu->bx = cpu->sp;                                                    /* 0B26 MOV BX,SP */
    cpu->dx = cpu->ds;                                                    /* 0B28 MOV DX,DS */
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 0x0Au));                  /* 0B2A LDS SI,SS:[BX+0Ah] */
    cpu->si = read16_at(ram, at);
    cpu->ds = read16_at(ram, (at + 2u) & MASK);
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 6u));                     /* 0B2E LES DI,SS:[BX+6] */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->cx = read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bx + 4u))); /* 0B32 MOV CX,SS:[BX+4] */
    length = ram[physical(cpu->ds, cpu->si)];                             /* 0B36 LODSB */
    cpu->si = (uint16_t)(cpu->si + 1u);
    if (length > (uint8_t)cpu->cx)                                        /* 0B37 CMP AL,CL; JBE; MOV AL,CL (flags folded) */
        length = (uint8_t)cpu->cx;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | length);
    ram[physical(cpu->es, cpu->di)] = length;                             /* 0B3D STOSB */
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->cx = length;                                                     /* 0B3E MOV CL,AL; 0B40 XOR CH,CH */
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    while (cpu->cx != 0u) {                                               /* 0B42 REP MOVSB */
        ram[physical(cpu->es, cpu->di)] = ram[physical(cpu->ds, cpu->si)];
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->ds = cpu->dx;                                                    /* 0B44 MOV DS,DX */
    at = physical(cpu->ss, cpu->sp);                                      /* 0B46 RETF 0Ah */
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_SYSTEM_0B25_RETURNED;
}
