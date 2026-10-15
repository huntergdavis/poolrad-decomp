#include "poolrad/replay/slot_a_allocation_19b1_abi.h"

pr_slot_a_allocation_status pr_slot_a_allocation_19b1_step(
    pr_slot_a_directory_state *s, pr_slot_a_string_0356_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint16_t offset;
    size_t start;
    if (!s || !report) return PR_SLOT_A_ALLOCATION_ARGUMENT;
    *report = (pr_slot_a_string_0356_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs != 0x2A8Eu) return PR_SLOT_A_ALLOCATION_BLOCKED_PC;
    switch (c->ip) {
    case 0x19B1u:
        c->ax = 0x011Du; c->ip = 0x19B4u;
        return PR_SLOT_A_ALLOCATION_STEPPED;
    case 0x19B4u:
        offset = (uint16_t)(c->sp - 2u); start = ((size_t)c->ss << 4) + offset;
        if ((size_t)offset + 2u > 0x10000u || start > s->ram_size ||
            2u > s->ram_size - start) return PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY;
        s->ram[start] = (uint8_t)c->ax; s->ram[start+1u] = (uint8_t)(c->ax >> 8);
        c->sp = offset; c->ip = 0x19B5u;
        return PR_SLOT_A_ALLOCATION_STEPPED;
    case 0x19B5u:
        *report = (pr_slot_a_string_0356_report){1u, 0x6Fu, 0x19B5u, 0xAF8u, 0x023Fu, 0x19BAu, 1u};
        return PR_SLOT_A_ALLOCATION_BLOCKED_GETMEM;
    default: return PR_SLOT_A_ALLOCATION_BLOCKED_PC;
    }
}
