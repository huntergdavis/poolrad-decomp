#include "overlay_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return (uint16_t)(ram[addr(seg, off)] |
        ((uint16_t)ram[addr(seg, (uint16_t)(off + 1u))] << 8));
}

int pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push(
    uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push_report *report) {
    pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push_report next;
    uint16_t high_ax;
    unsigned sf, of;
    if (ram == NULL || cpu == NULL || report == NULL ||
        ram_size != PR_1816_CALLER_RAM_SIZE || cpu->ip != 0x015Eu)
        return 1;
    sf = (cpu->flags & 0x0080u) != 0u;
    of = (cpu->flags & 0x0800u) != 0u;
    if (sf == of)
        return 1;
    high_ax = (uint16_t)(cpu->ax & 0xFF00u);
    if (rd16(ram, cpu->ss, cpu->sp) != cpu->ax ||
        rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) !=
            (uint16_t)(high_ax | 0x26u) ||
        rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) !=
            (uint16_t)(high_ax | 3u) ||
        rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) !=
            (uint16_t)(high_ax | 1u))
        return 1;

    next.before_ax = cpu->ax;
    next.before_flags = cpu->flags;
    cpu->ax = high_ax;
    cpu->ip = 0x0160u;
    next.stop_ax = cpu->ax;
    next.stop_ip = cpu->ip;
    *report = next;
    return 0;
}
