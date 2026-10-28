/* GAME.OVR U38:16E6, explicit callee boundaries; no closure/credit claim. */
#include "poolrad/recovered/overlay_u38_roll_dispatch_16e6.h"

#define RAM_MASK (PR_U38_16E6_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void push16(uint8_t *ram, pr_u38_16e6_cpu *cpu, uint16_t value)
{
    size_t a;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    a = physical(cpu->ss, cpu->sp);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_16e6_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

pr_u38_16e6_status pr_u38_roll_dispatch_16e6(
    uint8_t *ram, size_t ram_size, pr_u38_16e6_cpu *cpu,
    uint16_t wrapper_cs, pr_u38_16e6_phase phase)
{
    uint16_t input, result, parity;
    uint16_t expected_ip;
    switch (phase) {
    case PR_U38_16E6_ENTER: expected_ip = 0x16E6u; break;
    case PR_U38_16E6_AFTER_ROLL: expected_ip = 0x16FAu; break;
    case PR_U38_16E6_AFTER_DISPATCH: expected_ip = 0x1707u; break;
    default: return PR_U38_16E6_ARGUMENT;
    }
    if (ram == NULL || cpu == NULL || ram_size != PR_U38_16E6_RAM_SIZE ||
        cpu->cs != wrapper_cs || cpu->ip != expected_ip)
        return PR_U38_16E6_ARGUMENT;

    if (phase == PR_U38_16E6_ENTER) {
        push16(ram, cpu, cpu->bp);
        cpu->bp = cpu->sp;
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0xEu)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0xCu)));
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
        push16(ram, cpu, cpu->ax);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 9u);
        push16(ram, cpu, cpu->ax);
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x16FAu);
        cpu->cs = read16(ram, wrapper_cs, 0x16F8u);
        cpu->ip = 0x0048u;
        return PR_U38_16E6_ROLL_STOP;
    }
    if (phase == PR_U38_16E6_AFTER_ROLL) {
        /* XOR AH,AH; ADD AX,0Ah. ADD defines all six arithmetic flags:
         * AL+10 is 10..265, so CF/OF/SF/ZF are always clear. */
        input = (uint16_t)(cpu->ax & 0x00FFu);
        result = (uint16_t)(input + 10u);
        parity = (uint16_t)(result & 0xFFu);
        parity ^= (uint16_t)(parity >> 4);
        parity ^= (uint16_t)(parity >> 2);
        parity ^= (uint16_t)(parity >> 1);
        cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) |
            ((parity & 1u) ? 0u : 0x0004u) |
            (((input & 0xFu) + 10u > 0xFu) ? 0x0010u : 0u));
        cpu->ax = result;
        push16(ram, cpu, cpu->ax);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0xFEu);
        push16(ram, cpu, cpu->ax);
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x1707u);
        cpu->ip = 0x15EBu;
        return PR_U38_16E6_DISPATCH_STOP;
    }
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_16E6_RETURNED;
}
