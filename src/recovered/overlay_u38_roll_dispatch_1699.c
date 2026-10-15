/* GAME.OVR U38 public entry 55, +1699..+16BB (34 original bytes). */
#include "poolrad/recovered/overlay_u38_roll_dispatch_1699.h"

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

static void write16(uint8_t *ram, uint16_t seg, uint16_t off,
                    uint16_t value)
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

pr_u38_1699_status pr_u38_roll_dispatch_1699(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_1699_call call, void *context)
{
    if (ram == NULL || cpu == NULL || call == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x1699u)
        return PR_U38_1699_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    push16(ram, cpu, read16(ram, cpu->ss,
                            (uint16_t)(cpu->bp + 0xEu)));
    push16(ram, cpu, read16(ram, cpu->ss,
                            (uint16_t)(cpu->bp + 0xCu)));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 2u);
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 8u);
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x16ADu);
    cpu->cs = read16(ram, cpu->cs, 0x16ABu);
    cpu->ip = 0x0048u;
    if (!call(context, ram, ram_size, cpu))
        return PR_U38_1699_ROLL_ABORT;
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u); /* U9E roll RETF 4. */
    cpu->ax &= 0x00FFu; /* XOR AH,AH. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) |
                            0x0044u); /* ZF and PF; zero AH. */
    push16(ram, cpu, cpu->ax);
    cpu->ax &= 0xFF00u; /* MOV AL,0. */
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x16B7u);
    cpu->ip = 0x15EBu;
    if (!call(context, ram, ram_size, cpu))
        return PR_U38_1699_DISPATCH_ABORT;
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 8u); /* U38 +15EB RETF 8. */
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_1699_OK;
}
