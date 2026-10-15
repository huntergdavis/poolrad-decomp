/* GAME.OVR U38 public entry 40, +1276..+12B2; SYSTEM AF8:0C10 composed. */
#include "poolrad/recovered/overlay_u38_character_compare_1276.h"

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

static void compare8(pr_u38_101d_cpu *cpu, uint8_t left, uint8_t right)
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

static void subtract8(pr_u38_101d_cpu *cpu, uint8_t *ram,
                      uint16_t seg, uint16_t off, uint8_t right)
{
    uint8_t left = read8(ram, seg, off);
    compare8(cpu, left, right);
    write8(ram, seg, off, (uint8_t)(left - right));
}

/* The 43 original SYSTEM bytes at AF8:0C10, SHA-256
 * 11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58.
 * The caller has already entered the loaded :0C10 far frame. */
static void runtime_compare(uint8_t *ram, pr_u38_101d_cpu *cpu)
{
    uint8_t first_length, second_length, count, left, right;
    uint16_t saved_ds;
    cpu->flags &= (uint16_t)~0x400u; /* CLD */
    cpu->bx = cpu->sp;
    saved_ds = cpu->ds;
    cpu->dx = saved_ds;
    cpu->si = read16(ram, cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->ds = read16(ram, cpu->ss, (uint16_t)(cpu->bx + 10u));
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bx + 6u));
    first_length = read8(ram, cpu->ds, cpu->si);
    cpu->si = (uint16_t)(cpu->si + 1u);
    second_length = read8(ram, cpu->es, cpu->di);
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->ax = (uint16_t)(((uint16_t)second_length << 8) | first_length);
    count = first_length < second_length ? first_length : second_length;
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | count);
    if (count != 0u) {
        cpu->cx = count; /* XOR CH,CH before REPE CMPSB. */
        do {
            left = read8(ram, cpu->ds, cpu->si);
            right = read8(ram, cpu->es, cpu->di);
            cpu->si = (uint16_t)(cpu->si + 1u);
            cpu->di = (uint16_t)(cpu->di + 1u);
            cpu->cx = (uint16_t)(cpu->cx - 1u);
            compare8(cpu, left, right);
        } while (cpu->cx != 0u && left == right);
        if (left != right) goto returned;
    }
    compare8(cpu, first_length, second_length);
returned:
    cpu->ds = saved_ds;
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
}

static void compare_call(uint8_t *ram, pr_u38_101d_cpu *cpu,
                         uint16_t constant, uint16_t fixup, uint16_t ret_ip)
{
    cpu->di = read16(ram, cpu->ds, 0x5D92u);
    cpu->es = read16(ram, cpu->ds, 0x5D94u);
    push16(ram, cpu, cpu->es);
    push16(ram, cpu, cpu->di);
    cpu->di = constant;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, cpu->di);
    uint16_t loaded_segment = read16(ram, cpu->cs, fixup);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, ret_ip);
    cpu->cs = loaded_segment;
    cpu->ip = 0x0C10u;
    runtime_compare(ram, cpu);
}

pr_u38_1276_status pr_u38_character_compare_1276(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu)
{
    uint8_t marker;
    if (ram == NULL || cpu == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x1276u)
        return PR_U38_1276_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->di = read16(ram, cpu->ds, 0x5D92u);
    cpu->es = read16(ram, cpu->ds, 0x5D94u);
    marker = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x9Fu));
    compare8(cpu, marker, 1u);
    if (marker == 1u) {
        compare_call(ram, cpu, 0x1268u, 0x1293u, 0x1295u);
        if ((cpu->flags & 0x40u) == 0u) {
            compare_call(ram, cpu, 0x1270u, 0x12A5u, 0x12A7u);
        }
        if ((cpu->flags & 0x40u) != 0u)
            subtract8(cpu, ram, cpu->ds, 0x6822u, 4u);
    }
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_1276_OK;
}
