#include "resident_00ba_suffix_parent_copy_entry.h"

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

int pr_00ba_suffix_parent_copy_entry(
    unsigned site, uint8_t *ram, size_t ram_size,
    pr_1816_caller_cpu *cpu, pr_00ba_suffix_parent_copy_frame *frame) {
    uint16_t entry_ip, return_ip, dest, source;
    unsigned i;
    uint16_t words[9];
    if (site > 1u || ram == NULL || cpu == NULL || frame == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE)
        return 1;
    entry_ip = site == 0u ? 0x11DDu : 0x2AC8u;
    return_ip = site == 0u ? 0x11F2u : 0x2ADEu;
    dest = site == 0u ? (uint16_t)(cpu->bp - 0x47u) :
                        (uint16_t)(cpu->bp - 0x12Au);
    source = site == 0u ? 0x0EB2u : 0x2994u;
    if (cpu->ip != entry_ip) return 1;

    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    push(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x0Au);
    push(ram, cpu, cpu->ax);
    cpu->di = dest;
    push(ram, cpu, cpu->ss);
    push(ram, cpu, cpu->di);
    cpu->di = source;
    push(ram, cpu, cpu->cs);
    push(ram, cpu, cpu->di);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, return_ip);
    cpu->cs = 0x0AF8u;
    cpu->ip = 0x0B0Bu;
    for (i = 0u; i < 9u; ++i)
        words[i] = rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u * i));
    frame->return_ip = words[0];
    frame->return_cs = words[1];
    frame->source_offset = words[2];
    frame->source_segment = words[3];
    frame->destination_offset = words[4];
    frame->destination_segment = words[5];
    frame->color = words[6];
    frame->row = words[7];
    frame->column = words[8];
    return 0;
}
