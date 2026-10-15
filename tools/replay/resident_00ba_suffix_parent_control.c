#include "resident_00ba_suffix_parent_control.h"

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

static uint16_t parity(uint8_t value) {
    unsigned shift;
    for (shift = 0u; shift < 4u; ++shift)
        value ^= (uint8_t)(value >> (1u << shift));
    return (value & 1u) ? 0u : 0x0004u;
}

static uint16_t add16_flags(uint16_t old, uint16_t lhs, uint16_t rhs) {
    uint32_t wide = (uint32_t)lhs + rhs;
    uint16_t result = (uint16_t)wide;
    uint16_t bits = parity((uint8_t)result);
    if (wide > 0xFFFFu) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x0010u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x8000u) bits |= 0x0080u;
    if (((~(lhs ^ rhs)) & (lhs ^ result) & 0x8000u) != 0u)
        bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

int pr_00ba_suffix_parent_control(unsigned site, uint8_t *ram,
                                   size_t ram_size, pr_1816_caller_cpu *cpu,
                                   pr_00ba_suffix_parent_report *report) {
    uint16_t off, seg, parent_ip, stop_ip, source_off, source_seg;
    uint8_t length;
    if (site > 1u || ram == NULL || cpu == NULL || report == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x00BAu || cpu->ip != 0x188Cu)
        return 1;
    parent_ip = site == 0u ? 0x11D1u : 0x2ABBu;
    stop_ip = site == 0u ? 0x11DDu : 0x2AC8u;
    if (rd16(ram, cpu->ss, (uint16_t)(cpu->bp + 2u)) != parent_ip)
        return 1;
    if (pr_188c_display_return(ram, ram_size, cpu) != 0 ||
        cpu->ip != parent_ip)
        return 2;

    source_seg = site == 0u ? cpu->ss : cpu->ds;
    source_off = site == 0u ? (uint16_t)(cpu->bp - 0x3Fu) : 0x5D92u;
    off = rd16(ram, source_seg, source_off);
    seg = rd16(ram, source_seg, (uint16_t)(source_off + 2u));
    cpu->di = off;
    cpu->es = seg;
    length = rd8(ram, cpu->es, cpu->di);
    cpu->ax = length; /* MOV AL,[ES:DI]; XOR AH,AH */
    cpu->flags = add16_flags(cpu->flags, cpu->ax, 4u);
    cpu->ax = (uint16_t)(cpu->ax + 4u);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, cpu->ax);
    cpu->ip = stop_ip;
    report->node_length = length;
    report->pushed_length = cpu->ax;
    report->parent_ip = parent_ip;
    return 0;
}
