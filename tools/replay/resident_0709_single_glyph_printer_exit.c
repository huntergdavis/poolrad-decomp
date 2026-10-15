#include "resident_0709_single_glyph_printer_exit.h"

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

static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value) {
    ram[addr(seg, off)] = value;
}

static uint16_t pop(const uint8_t *ram, pr_1816_caller_cpu *cpu) {
    uint16_t value = rd16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
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

int pr_0694_single_glyph_printer_exit(uint8_t *ram, size_t ram_size,
                                       pr_1816_caller_cpu *cpu) {
    uint8_t index, count, column;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x0694u) return 1;
    index = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x102u));
    count = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x103u));
    if (index != 1u || count != 1u) return 2;

    column = rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu),
        (uint8_t)(column + 1u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | index);
    cpu->flags = cmp8(cpu->flags, index, count);
    cpu->sp = cpu->bp;
    cpu->bp = pop(ram, cpu);
    cpu->ip = pop(ram, cpu);
    cpu->cs = pop(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 0x0Au);
    return 0;
}
