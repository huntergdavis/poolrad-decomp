/* GAME.OVR U7F +0FD2..+0FDD; docs/re/U7F_ROLL_PREFIX_0FD2.md. */
#include "poolrad/recovered/overlay_u7f_roll_prefix_0fd2.h"

#define MASK (PR_U7F_0FD2_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void push(uint8_t *ram, pr_u7f_0fd2_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t sub2_flags(uint16_t incoming, uint16_t left)
{
    uint16_t result = (uint16_t)(left - 2u);
    uint16_t flags = (uint16_t)(incoming & (uint16_t)~0x08D5u);
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (left < 2u) flags |= 0x0001u;
    if (!(parity & 1u)) flags |= 0x0004u;
    if ((left ^ 2u ^ result) & 0x0010u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x8000u) flags |= 0x0080u;
    if ((left ^ 2u) & (left ^ result) & 0x8000u) flags |= 0x0800u;
    return flags;
}

pr_u7f_0fd2_status pr_u7f_roll_prefix_0fd2(
    uint8_t *ram, size_t ram_size, pr_u7f_0fd2_cpu *cpu)
{
    if (ram == NULL || cpu == NULL || ram_size != PR_U7F_0FD2_RAM_SIZE ||
        cpu->ip != 0x0FD2u)
        return PR_U7F_0FD2_ARGUMENT;
    push(ram, cpu, cpu->bp);                  /* +0FD2 PUSH BP */
    cpu->bp = cpu->sp;                        /* +0FD3 MOV BP,SP */
    cpu->flags = sub2_flags(cpu->flags, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp - 2u);     /* +0FD5 SUB SP,2 */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    push(ram, cpu, cpu->ax);                  /* +0FDA PUSH AX */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x14u);
    push(ram, cpu, cpu->ax);                  /* +0FDD PUSH AX */
    cpu->ip = 0x0FDEu;
    return PR_U7F_0FD2_AT_CALL;
}
