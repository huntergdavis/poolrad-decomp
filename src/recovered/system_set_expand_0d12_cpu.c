/* START.EXE SYSTEM 0AF8:0D12, the Turbo Pascal set expansion (raw CPU); docs/re/RES_0709_0768_PURE_CALLEES_CPU.md. */
#include "poolrad/recovered/system_set_expand_0d12_cpu.h"

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

static uint8_t sub8(pr_system_leaf_cpu *cpu, uint8_t a, uint8_t b)
{
    uint8_t result = (uint8_t)(a - b);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & 0x80u) cpu->flags |= 0x0800u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    return result;
}

/* REP STOSB of AL with CX bytes (DF clear). */
static void stos(uint8_t *ram, pr_system_leaf_cpu *cpu)
{
    while (cpu->cx != 0u) {
        ram[physical(cpu->es, cpu->di)] = (uint8_t)cpu->ax;
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
}

pr_system_leaf_status pr_system_set_expand_0d12(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    size_t at;
    uint8_t bl, bh;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LEAF_RAM_SIZE)
        return PR_SYSTEM_LEAF_ARGUMENT;
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 0D12 CLD; MOV BX,SP; MOV DX,DS */
    cpu->bx = cpu->sp;
    cpu->dx = cpu->ds;
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 0x0Au));                  /* 0D17 LES DI,SS:[BX+0Ah]; LDS SI,SS:[BX+6] */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 6u));
    cpu->si = read16_at(ram, at);
    cpu->ds = read16_at(ram, (at + 2u) & MASK);
    cpu->bx = read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bx + 4u))); /* 0D1F MOV BX,SS:[BX+4] */
    bl = (uint8_t)cpu->bx;
    bh = (uint8_t)(cpu->bx >> 8);
    cpu->cx = bh;                                                         /* 0D23 MOV CL,BH; XOR CH,CH; XOR AL,AL */
    cpu->ax &= 0xFF00u;
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    stos(ram, cpu);                                                       /* 0D29 REP STOSB */
    cpu->cx = bl;                                                         /* 0D2B MOV CL,BL; REP MOVSB */
    while (cpu->cx != 0u) {
        ram[physical(cpu->es, cpu->di)] = ram[physical(cpu->ds, cpu->si)];
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->cx = sub8(cpu, sub8(cpu, 0x20u, bl), bh);                        /* 0D2F MOV CL,20h; SUB CL,BL; SUB CL,BH */
    stos(ram, cpu);                                                       /* 0D35 REP STOSB */
    cpu->ds = cpu->dx;                                                    /* 0D37 MOV DS,DX; RETF 6 */
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_SYSTEM_LEAF_RETURNED;
}
