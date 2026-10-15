/* GAME.OVR U38 public entry 43, +133F..+1355 (22 original bytes). */
#include "poolrad/recovered/overlay_u38_target_reset_call_133f.h"

#define RAM_MASK (PR_U38_101D_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_101d_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_101d_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

pr_u38_133f_status pr_u38_target_reset_call_133f(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_133f_reset reset, void *context)
{
    uint16_t loaded_segment;
    uint8_t al;
    if (ram == NULL || cpu == NULL || reset == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x133Fu)
        return PR_U38_133F_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0xEu)));
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0xCu)));
    loaded_segment = read16(ram, cpu->cs, 0x134Bu);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x134Du);
    cpu->cs = loaded_segment;
    cpu->ip = 0x2637u;
    if (!reset(context, ram, ram_size, cpu))
        return PR_U38_133F_CALL_ABORT;
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    al = (uint8_t)cpu->ax;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity(al)) cpu->flags |= 4u;
    if (al == 0u) cpu->flags |= 0x40u;
    if (al & 0x80u) cpu->flags |= 0x80u;
    /* JE +1351 and fall-through both reach the same POP BP. */
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_133F_OK;
}
