#include "resident_00ba_suffix_post_b0b_parent.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
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

int pr_00ba_suffix_post_b0b_parent(
    unsigned site, uint8_t *ram, size_t ram_size,
    pr_1816_caller_cpu *cpu, pr_00ba_suffix_post_b0b_report *report) {
    uint16_t expected_ip, retained, next, return_ip, callee_ip, callee_cs;
    if (site > 1u || ram == NULL || cpu == NULL || report == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE) return 1;
    expected_ip = site == 0u ? 0x11F2u : 0x2ADEu;
    retained = site == 0u ? (uint16_t)(cpu->bp - 0x47u) :
                            (uint16_t)(cpu->bp - 0x12Au);
    if (cpu->ip != expected_ip ||
        rd16(ram, cpu->ss, cpu->sp) != retained ||
        rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) != cpu->ss)
        return 1;
    next = site == 0u ? retained : (uint16_t)(cpu->bp - 0x2Au);
    return_ip = site == 0u ? 0x11F7u : 0x2AE8u;
    callee_ip = site == 0u ? 0x0626u : 0x0B98u;
    callee_cs = site == 0u ? 0x0709u : 0x0AF8u;
    report->retained_offset = retained;
    report->retained_segment = cpu->ss;
    report->next_offset = next;
    report->next_segment = cpu->ss;
    report->return_ip = return_ip;
    report->callee_ip = callee_ip;
    report->callee_cs = callee_cs;
    if (site == 1u) {
        cpu->di = next;
        push(ram, cpu, cpu->ss);
        push(ram, cpu, cpu->di);
    }
    push(ram, cpu, cpu->cs);
    push(ram, cpu, return_ip);
    cpu->cs = callee_cs;
    cpu->ip = callee_ip;
    return 0;
}
