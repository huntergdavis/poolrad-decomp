#include "resident_00ba_1858_continuation.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return ram[addr(seg, off)];
}

static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t at = addr(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value) {
    size_t at = addr(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push(uint8_t *ram, pr_1816_caller_cpu *cpu, uint16_t value) {
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop(const uint8_t *ram, pr_1816_caller_cpu *cpu) {
    uint16_t value = rd16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void set_al(pr_1816_caller_cpu *cpu, uint8_t value) {
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t parity8(uint8_t value) {
    unsigned i;
    for (i = 0u; i < 4u; ++i) value ^= (uint8_t)(value >> (1u << i));
    return (value & 1u) ? 0u : 0x0004u;
}

static uint16_t cmp8(uint16_t old, uint8_t lhs, uint8_t rhs) {
    uint8_t result = (uint8_t)(lhs - rhs);
    uint16_t bits = parity8(result);
    if (lhs < rhs) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x80u) bits |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u) bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

static uint16_t add16(uint16_t old, uint16_t lhs, uint16_t rhs) {
    uint32_t wide = (uint32_t)lhs + rhs;
    uint16_t result = (uint16_t)wide;
    uint16_t bits = parity8((uint8_t)result);
    if (wide > 0xFFFFu) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x0010u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x8000u) bits |= 0x0080u;
    if (((~(lhs ^ rhs)) & (lhs ^ result) & 0x8000u) != 0u)
        bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

int pr_1858_display_continuation(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu) {
    uint8_t flag, length, column;
    uint16_t base, sum;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x00BAu || cpu->ip != 0x1858u) return 1;
    base = cpu->bp;
    flag = rd8(ram, cpu->ss, (uint16_t)(base + 6u));
    cpu->flags = cmp8(cpu->flags, flag, 0u);
    if (flag == 0u) {
        cpu->sp = cpu->bp;
        cpu->bp = pop(ram, cpu);
        cpu->ip = pop(ram, cpu);
        cpu->cs = pop(ram, cpu);
        cpu->sp = (uint16_t)(cpu->sp + 0x0Au);
        return 0;
    }

    /* 185E..1887: late node-length read and arguments for the private
     * suffix copy. Stop at the call entry before the copy changes RAM. */
    cpu->di = rd16(ram, cpu->ss, (uint16_t)(base + 0x0Cu));
    cpu->es = rd16(ram, cpu->ss, (uint16_t)(base + 0x0Eu));
    length = rd8(ram, cpu->es, cpu->di);
    set_al(cpu, length);
    cpu->ax &= 0x00FFu;
    cpu->dx = cpu->ax;
    column = rd8(ram, cpu->ss, (uint16_t)(base + 0x0Au));
    set_al(cpu, column);
    cpu->ax &= 0x00FFu;
    sum = (uint16_t)(cpu->ax + cpu->dx);
    cpu->flags = add16(cpu->flags, cpu->ax, cpu->dx);
    cpu->ax = sum;
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(base + 8u)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(base - 1u)));
    push(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(base - 4u);
    push(ram, cpu, cpu->ss);
    push(ram, cpu, cpu->di);
    cpu->di = 0x1813u;
    push(ram, cpu, cpu->cs);
    push(ram, cpu, cpu->di);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x1887u);
    cpu->cs = 0x0AF8u;
    cpu->ip = 0x0B0Bu;
    return 0;
}
