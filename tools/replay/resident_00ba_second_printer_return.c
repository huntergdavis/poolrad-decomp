#include "resident_00ba_second_printer_return.h"

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

static uint16_t pop(const uint8_t *ram, pr_1816_caller_cpu *cpu) {
    uint16_t value = rd16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

int pr_1887_second_printer_entry(uint8_t *ram, size_t ram_size,
                                 pr_1816_caller_cpu *cpu,
                                 pr_1887_second_printer_frame *frame) {
    if (ram == NULL || cpu == NULL || frame == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x00BAu || cpu->ip != 0x1887u ||
        cpu->sp != (uint16_t)(cpu->bp - 14u) ||
        rd16(ram, cpu->ss, cpu->sp) != (uint16_t)(cpu->bp - 4u) ||
        rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) != cpu->ss)
        return 1;
    frame->destination_offset = rd16(ram, cpu->ss, cpu->sp);
    frame->destination_segment = rd16(ram, cpu->ss,
                                      (uint16_t)(cpu->sp + 2u));
    frame->color = rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    frame->row = rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    frame->column = rd16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u));
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x188Cu);
    cpu->cs = 0x0709u;
    cpu->ip = 0x0626u;
    return 0;
}

int pr_188c_display_return(uint8_t *ram, size_t ram_size,
                            pr_1816_caller_cpu *cpu) {
    if (ram == NULL || cpu == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x00BAu || cpu->ip != 0x188Cu)
        return 1;
    cpu->sp = cpu->bp;
    cpu->bp = pop(ram, cpu);
    cpu->ip = pop(ram, cpu);
    cpu->cs = pop(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 0x0Au);
    return 0;
}
