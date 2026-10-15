#include "resident_0709_printer_0768_copy_gate.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t at = addr(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void push16(uint8_t *ram, pr_1816_caller_cpu *cpu, uint16_t value) {
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = addr(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t sub16_flags(uint16_t old, uint16_t lhs, uint16_t rhs,
                            uint16_t result) {
    uint16_t bits = 0;
    uint8_t parity = (uint8_t)result;
    unsigned shift;
    if (lhs < rhs) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x0010u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if ((result & 0x8000u) != 0u) bits |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x8000u) != 0u)
        bits |= 0x0800u;
    for (shift = 0u; shift < 4u; ++shift)
        parity ^= (uint8_t)(parity >> (1u << shift));
    if ((parity & 1u) == 0u) bits |= 0x0004u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

int pr_0709_0768_first_copy_gate(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu) {
    uint16_t before, source_off, source_seg;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x0768u) return 1;

    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    before = cpu->sp;
    cpu->sp = (uint16_t)(before - 0x0126u);
    cpu->flags = sub16_flags(cpu->flags, before, 0x0126u, cpu->sp);

    source_off = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    source_seg = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->di = source_off;
    cpu->es = source_seg;
    push16(ram, cpu, cpu->es);
    push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x0100u);
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x00FFu;
    push16(ram, cpu, cpu->ax);

    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x0783u);
    cpu->cs = 0x0AF8u;
    cpu->ip = 0x0B25u;
    return 0;
}
