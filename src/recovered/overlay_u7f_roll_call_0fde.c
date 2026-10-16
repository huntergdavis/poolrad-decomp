/* GAME.OVR U7F +0FDE LCALL B0:0048; docs/re/U7F_ROLL_CALL_0FDE.md. */
#include "poolrad/recovered/overlay_u7f_roll_call_0fde.h"

#define MASK (PR_U7F_0FDE_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void push(uint8_t *ram, pr_u7f_0fde_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

pr_u7f_0fde_status pr_u7f_roll_call_0fde(
    uint8_t *ram, size_t ram_size, pr_u7f_0fde_cpu *cpu,
    pr_u7f_0fde_roll roll, void *context)
{
    uint16_t source_cs;
    if (ram == NULL || cpu == NULL || roll == NULL ||
        ram_size != PR_U7F_0FDE_RAM_SIZE || cpu->ip != 0x0FDEu)
        return PR_U7F_0FDE_ARGUMENT;
    source_cs = cpu->cs;
    push(ram, cpu, source_cs);
    push(ram, cpu, 0x0FE3u);
    cpu->cs = 0x00B0u;
    cpu->ip = 0x0048u;
    if (!roll(context, ram, ram_size, cpu))
        return PR_U7F_0FDE_CALL_ABORT;
    if (cpu->cs != source_cs || cpu->ip != 0x0FE3u)
        return PR_U7F_0FDE_TRANSFER;
    return PR_U7F_0FDE_RETURNED;
}
