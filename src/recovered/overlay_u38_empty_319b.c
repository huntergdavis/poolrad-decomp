/* GAME.OVR U38 public entry 116: PUSH BP; MOV BP,SP; POP BP; RETF 10. */
#include "poolrad/recovered/overlay_u38_empty_319b.h"

#define RAM_MASK (PR_U38_101D_RAM_SIZE - 1u)

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

pr_u38_319b_status pr_u38_empty_319b(
    uint8_t *ram, size_t ram_size, pr_u38_319b_cpu *cpu)
{
    size_t at;
    if (ram == NULL || cpu == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x319Bu)
        return PR_U38_319B_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    write16(ram, at, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->bp = read16(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16(ram, at);
    cpu->cs = read16(ram, (at + 2u) & RAM_MASK);
    cpu->sp = (uint16_t)(cpu->sp + 14u);
    return PR_U38_319B_OK;
}
