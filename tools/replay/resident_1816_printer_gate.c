#include "resident_1816_printer_gate.h"

#define RAM_MASK (PR_1816_GATE_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return ram[addr(seg, off)];
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t a = addr(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off,
                    uint16_t value) {
    size_t a = addr(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push(uint8_t *ram, pr_1816_gate_cpu *cpu, uint16_t value) {
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t cmp8_flags(uint16_t old, uint8_t lhs, uint8_t rhs) {
    uint8_t result = (uint8_t)(lhs - rhs);
    uint8_t parity = result;
    uint16_t bits = 0;
    unsigned i;
    if (lhs < rhs) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if ((result & 0x80u) != 0u) bits |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u) bits |= 0x0800u;
    for (i = 0; i < 4u; ++i) parity ^= (uint8_t)(parity >> (1u << i));
    if ((parity & 1u) == 0u) bits |= 0x0004u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

int pr_1816_first_printer_gate(uint8_t *ram, size_t ram_size,
                               pr_1816_gate_cpu *cpu) {
    uint16_t node_off, node_seg;
    uint8_t color, first;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_GATE_RAM_SIZE ||
        cpu->cs != 0x00BAu || cpu->ip != 0x1816u) return 1;

    /* 1816..181C: Pascal prologue and four-byte local frame. */
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);

    /* Both LES instructions reload this far pointer, including ES:DI. */
    node_off = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));
    node_seg = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    cpu->di = node_off;
    cpu->es = node_seg;
    first = read8(ram, node_seg, (uint16_t)(node_off + 0x10Du));
    cpu->flags = cmp8_flags(cpu->flags, first, 0u);
    if (first == 0u) {
        color = 0x0Cu;
    } else {
        uint8_t second = read8(ram, node_seg,
                               (uint16_t)(node_off + 0x10Eu));
        cpu->flags = cmp8_flags(cpu->flags, second, 1u);
        color = second == 1u ? 0x0Eu : 0x0Bu;
    }
    ram[addr(cpu->ss, (uint16_t)(cpu->bp - 1u))] = color;

    /* 1842..1853: low-byte scalar loads retain AH; order is the ABI. */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au)));
    push(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    push(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | color);
    push(ram, cpu, cpu->ax);
    cpu->di = node_off;
    cpu->es = node_seg;
    push(ram, cpu, cpu->es);
    push(ram, cpu, cpu->di);

    /* CALL FAR pushes return CS, then return IP. Stop before printer code. */
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x1858u);
    cpu->cs = 0x0709u;
    cpu->ip = 0x0626u;
    return 0;
}
