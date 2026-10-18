/* START.EXE resident 0899:0104, whose reachable code is a frame and a return (raw CPU); docs/re/RES_0709_0768_PURE_CALLEES_CPU.md. */
#include "poolrad/recovered/resident_0899_idle_0104_cpu.h"

#define MASK (PR_R0899_IDLE_RAM_SIZE - 1u)

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

pr_r0899_idle_status pr_r0899_idle_0104_cpu(uint8_t *ram, size_t ram_size, pr_r0899_idle_cpu *cpu)
{
    uint16_t a, result;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_R0899_IDLE_RAM_SIZE || cpu->ip != 0x0104u)
        return PR_R0899_IDLE_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);                                   /* 0104 PUSH BP; MOV BP,SP */
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->bp >> 8);
    cpu->bp = cpu->sp;
    a = cpu->sp;                                                          /* 0107 SUB SP,2; JMP 011E */
    result = (uint16_t)(a - 2u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < 2u) cpu->flags |= 0x0001u;
    if ((a ^ 2u ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if ((a ^ 2u) & (a ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    cpu->sp = cpu->bp;                                                    /* 011E MOV SP,BP; POP BP; RETF */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_R0899_IDLE_OK;
}
