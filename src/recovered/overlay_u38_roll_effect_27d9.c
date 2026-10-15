/* GAME.OVR U38 public entry 84, complete +27D9..+2814 (59 bytes). */
#include "poolrad/recovered/overlay_u38_roll_effect_27d9.h"

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

static void logic8(pr_u38_101d_cpu *cpu, uint8_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity(value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x80u) cpu->flags |= 0x80u;
}

pr_u38_27d9_status pr_u38_roll_effect_27d9(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_27d9_call roll, pr_u38_27d9_call effect, void *context)
{
    uint8_t masked;
    uint16_t loaded_segment;
    if (ram == NULL || cpu == NULL || roll == NULL || effect == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x27D9u)
        return PR_U38_27D9_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ds, 0x6819u));
    masked = (uint8_t)cpu->ax & 1u;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | masked);
    logic8(cpu, masked); /* AND AL,1; OR AL,AL */
    if (masked == 0u) {
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             read8(ram, cpu->ds, 0x6819u));
        masked = (uint8_t)cpu->ax & 0x10u;
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | masked);
        logic8(cpu, masked); /* AND AL,10h; OR AL,AL */
        if (masked == 0u) {
            push16(ram, cpu, read16(ram, cpu->ss,
                                    (uint16_t)(cpu->bp + 0xEu)));
            push16(ram, cpu, read16(ram, cpu->ss,
                                    (uint16_t)(cpu->bp + 0xCu)));
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x66u);
            push16(ram, cpu, cpu->ax);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 3u);
            push16(ram, cpu, cpu->ax);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 6u);
            push16(ram, cpu, cpu->ax);
            loaded_segment = read16(ram, cpu->cs, 0x2800u);
            push16(ram, cpu, cpu->cs);
            push16(ram, cpu, 0x2802u);
            cpu->cs = loaded_segment;
            cpu->ip = 0x0048u;
            if (!roll(context, ram, ram_size, cpu))
                return PR_U38_27D9_ROLL_ABORT;
            cpu->ip = pop16(ram, cpu);
            cpu->cs = pop16(ram, cpu);
            cpu->sp = (uint16_t)(cpu->sp + 4u);
            cpu->ax &= 0x00FFu; /* XOR AH,AH */
            logic8(cpu, 0u);
            push16(ram, cpu, cpu->ax);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0xFFu);
            push16(ram, cpu, cpu->ax);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
            push16(ram, cpu, cpu->ax);
            loaded_segment = read16(ram, cpu->cs, 0x280Eu);
            push16(ram, cpu, cpu->cs);
            push16(ram, cpu, 0x2810u);
            cpu->cs = loaded_segment;
            cpu->ip = 0x0052u;
            if (!effect(context, ram, ram_size, cpu))
                return PR_U38_27D9_EFFECT_ABORT;
            cpu->ip = pop16(ram, cpu);
            cpu->cs = pop16(ram, cpu);
            cpu->sp = (uint16_t)(cpu->sp + 12u);
        }
    }
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_27D9_OK;
}
