/* START.EXE resident 0709:00A0, the EGA page select (raw CPU); docs/re/RES_0709_PAGE_SELECT_00A0_CPU.md. */
#include "poolrad/recovered/resident_0709_page_select_00a0_cpu.h"

#define MASK (PR_R0709_PAGE_RAM_SIZE - 1u)

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

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

pr_r0709_page_status pr_r0709_page_select_00a0_cpu(uint8_t *ram, size_t ram_size, pr_r0709_page_cpu *cpu)
{
    uint32_t wide;
    uint16_t shifted, result;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_R0709_PAGE_RAM_SIZE || cpu->ip != 0x00A0u)
        return PR_R0709_PAGE_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);                                   /* 00A0 PUSH BP; MOV BP,SP */
    write16_at(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;
    cpu->ax = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];           /* 00A3 MOV AL,[BP+6]; XOR AH,AH */
    cpu->cx = 9u;                                                         /* 00A8 MOV CX,9; SHL AX,CL */
    shifted = (uint16_t)(cpu->ax << 9);
    wide = (uint32_t)shifted + 0xA000u;                                   /* 00AD ADD AX,0A000h */
    result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (wide > 0xFFFFu) cpu->flags |= 0x0001u;
    if ((shifted ^ 0xA000u ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (~(shifted ^ 0xA000u) & (shifted ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    cpu->ax = result;
    write16_at(ram, physical(cpu->ds, 0x4A18u), cpu->ax);                 /* 00B0 MOV [4A18h],AX */
    at = physical(cpu->ss, cpu->sp);                                      /* 00B3 POP BP; RETF 2 */
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_R0709_PAGE_OK;
}
