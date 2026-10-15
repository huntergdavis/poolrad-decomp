#include "overlay_u86_entry1_postdc0_joined_push_ss.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)
#define DEFINED_LOGIC_FLAGS 0x08C5u

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return (uint16_t)(ram[addr(seg, off)] |
        ((uint16_t)ram[addr(seg, (uint16_t)(off + 1u))] << 8));
}

int pr_u86_entry1_postdc0_joined_push_ss(
    uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_u86_entry1_postdc0_joined_push_ss_report *report) {
    pr_u86_entry1_postdc0_joined_push_ss_report next;
    if (ram == NULL || cpu == NULL || report == NULL ||
        ram_size != PR_1816_CALLER_RAM_SIZE || cpu->ip != 0x019Fu ||
        cpu->ax != 0u || cpu->di != (uint16_t)(cpu->bp - 0x32u) ||
        (cpu->flags & DEFINED_LOGIC_FLAGS) != 0x0044u ||
        rd16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u)) != 0u ||
        rd16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u)) != 0u)
        return 1;

    next.before_sp = cpu->sp;
    next.pushed_ss = cpu->ss;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    ram[addr(cpu->ss, cpu->sp)] = (uint8_t)cpu->ss;
    ram[addr(cpu->ss, (uint16_t)(cpu->sp + 1u))] =
        (uint8_t)(cpu->ss >> 8);
    cpu->ip = 0x01A0u;
    next.stop_sp = cpu->sp;
    next.stop_ip = cpu->ip;
    *report = next;
    return 0;
}
