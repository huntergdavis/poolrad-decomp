#include "resident_00ba_suffix_glyph_control.h"

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

static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value) {
    size_t at = addr(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t parity(uint8_t value) {
    unsigned shift;
    for (shift = 0u; shift < 4u; ++shift)
        value ^= (uint8_t)(value >> (1u << shift));
    return (value & 1u) ? 0u : 0x0004u;
}

static uint16_t inc8_flags(uint16_t old, uint8_t before) {
    uint8_t result = (uint8_t)(before + 1u);
    uint16_t bits = parity(result);
    if ((before & 0x0Fu) == 0x0Fu) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x80u) bits |= 0x0080u;
    if (before == 0x7Fu) bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D4u) | bits);
}

static uint16_t cmp8_flags(uint16_t old, uint8_t lhs, uint8_t rhs) {
    uint8_t result = (uint8_t)(lhs - rhs);
    uint16_t bits = parity(result);
    if (lhs < rhs) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x80u) bits |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u) bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

static void set_al(pr_1816_caller_cpu *cpu, uint8_t value) {
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
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

int pr_00ba_suffix_glyph_control(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu,
                                  pr_00ba_suffix_control_report *report) {
    uint8_t before, after, total, column;
    if (ram == NULL || cpu == NULL || report == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x0694u) return 1;
    *report = (pr_00ba_suffix_control_report){0};
    column = rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    column = (uint8_t)(column + 1u);
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu), column);
    cpu->flags = inc8_flags(cpu->flags, (uint8_t)(column - 1u));
    before = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x102u));
    total = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x103u));
    set_al(cpu, before);
    cpu->flags = cmp8_flags(cpu->flags, before, total);
    report->glyph_index_before = before;
    report->total = total;
    report->column_after = column;
    if (before == total) {
        cpu->sp = cpu->bp;
        cpu->bp = pop(ram, cpu);
        cpu->ip = pop(ram, cpu);
        cpu->cs = pop(ram, cpu);
        cpu->sp = (uint16_t)(cpu->sp + 0x0Au);
        report->stop = PR_00BA_SUFFIX_PRINTER_RETURN;
        report->glyph_index_after = before;
        return 0;
    }

    after = (uint8_t)(before + 1u);
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x102u), after);
    cpu->flags = inc8_flags(cpu->flags, before);
    report->glyph_index_after = after;
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push(ram, cpu, cpu->ax);
    set_al(cpu, after);
    cpu->ax &= 0x00FFu;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08C5u) | 0x0044u);
    cpu->di = cpu->ax;
    set_al(cpu, rd8(ram, cpu->ss,
                    (uint16_t)(cpu->bp + cpu->di - 0x100u)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push(ram, cpu, cpu->ax);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x0694u);
    cpu->ip = 0x01DFu;
    report->stop = PR_00BA_SUFFIX_NEXT_GLYPH;
    return 0;
}
