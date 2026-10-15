/* GAME.OVR U9E public entry 0, +1932..+1936 (5 original bytes). */
#include "poolrad/recovered/overlay_u9e_entry0_1932.h"

#define RAM_MASK (PR_U9E_1932_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop16(const uint8_t *ram, pr_u9e_1932_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

pr_u9e_1932_status pr_u9e_entry0_1932(
    uint8_t *ram, size_t ram_size, pr_u9e_1932_cpu *cpu)
{
    if (ram == NULL || cpu == NULL || ram_size < PR_U9E_1932_RAM_SIZE ||
        cpu->ip != 0x1932u)
        return PR_U9E_1932_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp); /* PUSH BP */
    cpu->bp = cpu->sp;                        /* MOV BP,SP */
    cpu->bp = pop16(ram, cpu);                /* POP BP */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);                /* RETF, 16-bit SP wraps */
    return PR_U9E_1932_OK;
}
