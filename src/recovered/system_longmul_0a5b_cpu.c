/* Turbo Pascal SYSTEM 0AF8:0A5B LongInt multiply (raw CPU); docs/re/SYSTEM_LONGMUL_0A5B.md. */
#include "poolrad/recovered/system_longmul_0a5b_cpu.h"

#define MASK (PR_SYSTEM_LONGMUL_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void push16(uint8_t *ram, pr_system_longmul_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop16(const uint8_t *ram, pr_system_longmul_cpu *cpu)
{
    uint16_t value = read16_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* ADD left,right on words: the six arithmetic flags; the other flags are kept. */
static uint16_t add16(pr_system_longmul_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left + right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (result < left) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ result) & (right ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    return result;
}

pr_system_longmul_status pr_system_longmul_0a5b(uint8_t *ram, size_t ram_size, pr_system_longmul_cpu *cpu)
{
    uint32_t product;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LONGMUL_RAM_SIZE)
        return PR_SYSTEM_LONGMUL_ARGUMENT;
    cpu->si = cpu->ax;                                                    /* 0A5B MOV SI,AX; MOV DI,DX */
    cpu->di = cpu->dx;
    product = (uint32_t)cpu->ax * cpu->cx;                                /* 0A5F MUL CX (flags folded); PUSH AX; PUSH DX */
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->dx);
    product = (uint32_t)cpu->si * cpu->bx;                                /* 0A63 MOV AX,SI; MUL BX; MOV BX,AX */
    cpu->bx = (uint16_t)product;
    product = (uint32_t)cpu->di * cpu->cx;                                /* 0A69 MOV AX,DI; MUL CX; MOV CX,AX */
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    cpu->cx = cpu->ax;
    cpu->dx = pop16(ram, cpu);                                            /* 0A6F POP DX; POP AX */
    cpu->ax = pop16(ram, cpu);
    cpu->dx = (uint16_t)(cpu->dx + cpu->bx);                              /* 0A71 ADD DX,BX (flags folded); ADD DX,CX */
    cpu->dx = add16(cpu, cpu->dx, cpu->cx);
    at = physical(cpu->ss, cpu->sp);                                      /* 0A75 RETF */
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_SYSTEM_LONGMUL_RETURNED;
}
