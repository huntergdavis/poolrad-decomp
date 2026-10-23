/* START.EXE resident 0709:0DBE (raw CPU, composing the raw CRT Delay 0A40:029C); docs/re/RES_0709_DELAY_0DBE_CPU.md. */
#include "poolrad/recovered/resident_0709_delay_wrapper_0dbe_cpu.h"
#include "poolrad/recovered/resident_a40_delay_029c_cpu.h"

#define MASK (PR_R0709_DELAY_CPU_RAM_SIZE - 1u)
#define DELAY_IP 0x029Cu

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

static uint16_t rd16(const uint8_t *ram, uint16_t segment, uint16_t offset) { return read16_at(ram, physical(segment, offset)); }

static void push16(uint8_t *ram, pr_r0709_delay_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
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

pr_r0709_delay_cpu_status pr_r0709_delay_wrapper_0dbe_cpu(uint8_t *ram, size_t ram_size, pr_r0709_delay_cpu *cpu,
                                                          uint64_t max_loopbacks, int *callee_status)
{
    pr_a40_delay_cpu state;
    pr_a40_delay_cpu_status status;
    uint32_t product;
    uint16_t caller_cs;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_R0709_DELAY_CPU_RAM_SIZE || cpu->ip != 0x0DBEu)
        return PR_R0709_DELAY_CPU_ARGUMENT;
    caller_cs = cpu->cs;
    push16(ram, cpu, cpu->bp);                                            /* 0DBE PUSH BP; MOV BP,SP */
    cpu->bp = cpu->sp;
    cpu->ax = ram[physical(cpu->ds, 0x49E2u)];                            /* 0DC1 MOV AL,[49E2h]; XOR AH,AH */
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
    cpu->dx = 0x00E1u;                                                    /* 0DC6 MOV DX,0E1h; MUL DX */
    product = (uint32_t)cpu->ax * cpu->dx;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    {   /* pinned DOSBox-X MULW (instructions.h:605-619): SF/AF kept, ZF from AX alone,
         * PF = even parity of all four bytes of DX:AX, CF = OF = (DX != 0). */
        uint16_t w = (uint16_t)(cpu->ax ^ cpu->dx);
        cpu->flags &= (uint16_t)~0x0845u;
        if (cpu->dx != 0u) cpu->flags |= 0x0801u;
        if (cpu->ax == 0u) cpu->flags |= 0x0040u;
        if (even_parity((uint8_t)(w ^ (w >> 8)))) cpu->flags |= 0x0004u;
    }
    push16(ram, cpu, cpu->ax);                                            /* 0DCB PUSH AX */
    cpu->ip = 0x0DCCu;                                                    /* 0DCC LCALL 0A40:029C */
    if (ram[physical(cpu->cs, 0x0DCCu)] != 0x9Au || rd16(ram, cpu->cs, 0x0DCDu) != DELAY_IP)
        return PR_R0709_DELAY_CPU_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x0DD1u);
    cpu->cs = rd16(ram, caller_cs, 0x0DCFu);
    cpu->ip = DELAY_IP;
    COPY_CPU(&state, cpu);
    status = pr_a40_delay_029c_cpu(ram, ram_size, &state, max_loopbacks);
    COPY_CPU(cpu, &state);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (status != PR_A40_DELAY_CPU_RETURNED)
        return PR_R0709_DELAY_CPU_CALLEE;
    if (cpu->ip != 0x0DD1u || cpu->cs != caller_cs)
        return PR_R0709_DELAY_CPU_TRANSFER;
    at = physical(cpu->ss, cpu->sp);                                      /* 0DD1 POP BP; RETF */
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_R0709_DELAY_CPU_OK;
}
