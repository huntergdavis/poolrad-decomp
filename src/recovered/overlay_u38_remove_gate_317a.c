/* GAME.OVR U38 public entry 115, complete +317A..+319B (33 bytes). */
#include "poolrad/recovered/overlay_u38_remove_gate_317a.h"

#define RAM_MASK (PR_U38_101D_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
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

static void push16(uint8_t *ram, pr_u38_317a_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_317a_cpu *cpu)
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

static void compare8(pr_u38_317a_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 1u;
    if (parity(result)) cpu->flags |= 4u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x800u;
}

static void logic16(pr_u38_317a_cpu *cpu, uint16_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x8000u) cpu->flags |= 0x80u;
}

pr_u38_317a_status pr_u38_remove_gate_317a(
    uint8_t *ram, size_t ram_size, pr_u38_317a_cpu *cpu,
    pr_u38_317a_remove remove_item, void *context)
{
    uint16_t caller_cs, loaded_segment;
    uint8_t gate;
    if (ram == NULL || cpu == NULL || remove_item == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x317Au)
        return PR_U38_317A_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    gate = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    compare8(cpu, gate, 0u);
    if (gate != 0u) {
        push16(ram, cpu, read16(ram, cpu->ss,
                                (uint16_t)(cpu->bp + 0xEu)));
        push16(ram, cpu, read16(ram, cpu->ss,
                                (uint16_t)(cpu->bp + 0xCu)));
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x17u);
        push16(ram, cpu, cpu->ax);
        cpu->ax = 0u; /* XOR AX,AX */
        logic16(cpu, 0u);
        cpu->dx = 0u; /* XOR DX,DX */
        logic16(cpu, 0u);
        push16(ram, cpu, cpu->dx);
        push16(ram, cpu, cpu->ax);
        caller_cs = cpu->cs;
        loaded_segment = read16(ram, caller_cs, 0x3195u);
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x3197u);
        cpu->cs = loaded_segment;
        cpu->ip = 0x002Au;
        if (!remove_item(context, ram, ram_size, cpu) ||
            cpu->cs != caller_cs || cpu->ip != 0x3197u)
            return PR_U38_317A_CALL_ABORT;
    }
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_317A_OK;
}
