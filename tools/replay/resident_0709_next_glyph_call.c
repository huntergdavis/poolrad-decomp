#include "resident_0709_next_glyph_call.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return ram[addr(seg, off)];
}

static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value) {
    ram[addr(seg, off)] = value;
}

static void push(uint8_t *ram, pr_1816_caller_cpu *cpu, uint16_t value) {
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = addr(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void set_al(pr_1816_caller_cpu *cpu, uint8_t value) {
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

int pr_0694_next_glyph_entry(uint8_t *ram, size_t ram_size,
                             pr_1816_caller_cpu *cpu) {
    uint8_t index, count, column;
    uint16_t base;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x0694u) return 1;
    base = cpu->bp;
    index = rd8(ram, cpu->ss, (uint16_t)(base - 0x102u));
    count = rd8(ram, cpu->ss, (uint16_t)(base - 0x103u));
    if (index == 0u || index >= count) return 2;

    /* 0694..069F increments the low column byte and tests the copied
     * Pascal length. 066D advances the local one-based glyph index. */
    column = (uint8_t)(rd8(ram, cpu->ss, (uint16_t)(base + 0x0Eu)) + 1u);
    wr8(ram, cpu->ss, (uint16_t)(base + 0x0Eu), column);
    set_al(cpu, index);
    index = (uint8_t)(index + 1u);
    wr8(ram, cpu->ss, (uint16_t)(base - 0x102u), index);

    /* 0671..0691 pushes column, row, color, one cell, indexed byte,
     * upcase flag, CS, and the near-call return offset in that order. */
    set_al(cpu, column);
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(base + 0x0Cu)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(base + 0x0Au)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push(ram, cpu, cpu->ax);
    set_al(cpu, index);
    cpu->ax &= 0x00FFu; /* XOR AH,AH */
    cpu->di = cpu->ax;
    set_al(cpu, rd8(ram, cpu->ss,
                    (uint16_t)(base - 0x100u + cpu->di)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push(ram, cpu, cpu->ax);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x0694u);
    /* XOR AH,AH clears CF/OF/SF, sets ZF/PF; AF is undefined. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08C5u) | 0x0044u);
    cpu->ip = 0x01DFu;
    return 0;
}
