/* GAME.OVR U38 public entry 109, complete +2E08..+2E37 (47 bytes). */
#include "poolrad/recovered/overlay_u38_clear_cycle_2e08.h"

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

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_2e08_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_2e08_cpu *cpu)
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

static void compare8(pr_u38_2e08_cpu *cpu, uint8_t left, uint8_t right)
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

/* Execute original U38 private +0000, including PUSH CS/CALL and RETF 2.
 * Its behavior was independently bounded in
 * docs/re/OVERLAY_UNIT38_HELPER_0000.md. */
static void helper_0000(uint8_t *ram, pr_u38_2e08_cpu *cpu,
                        uint16_t return_ip)
{
    uint8_t key, latch;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = 0u;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    key = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    compare8(cpu, key, 0u);
    if (key == 0u) {
        write8(ram, cpu->ds, 0x6818u, 0u);
        write8(ram, cpu->ds, 0x6817u, 0u);
    } else {
        latch = read8(ram, cpu->ds, 0x6817u);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | latch);
        compare8(cpu, latch, key);
        if (latch == key) {
            write8(ram, cpu->ds, 0x6818u, 0u);
            write8(ram, cpu->ds, 0x6817u, 0u);
        }
    }
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
}

pr_u38_2e08_status pr_u38_clear_cycle_2e08(
    uint8_t *ram, size_t ram_size, pr_u38_2e08_cpu *cpu)
{
    static const uint8_t keys[4] = {0x0Bu, 0x35u, 0x34u, 0x37u};
    static const uint16_t returns[4] = {0x2E12u, 0x2E19u, 0x2E20u, 0x2E27u};
    if (ram == NULL || cpu == NULL || ram_size < PR_U38_101D_RAM_SIZE ||
        cpu->ip != 0x2E08u)
        return PR_U38_2E08_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    for (unsigned i = 0u; i < 4u; ++i) {
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | keys[i]);
        push16(ram, cpu, cpu->ax);
        helper_0000(ram, cpu, returns[i]);
    }
    compare8(cpu, read8(ram, cpu->ds, 0x682Au), 0u);
    if (read8(ram, cpu->ds, 0x682Au) == 0u)
        write8(ram, cpu->ds, 0x6816u, 0x64u);
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_2E08_OK;
}
