#include "resident_00ba_suffix_next_glyph_ega.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t at = (((size_t)seg << 4) + off) & RAM_MASK;
    return ram[at];
}

int pr_00ba_suffix_next_glyph_ega(uint8_t *ram, size_t ram_size,
                                    pr_1816_caller_cpu *cpu,
                                    pr_064f_ega_report *report) {
    int status;
    if (ram == NULL || cpu == NULL || report == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x01DFu ||
        rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x102u)) != 2u ||
        rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x103u)) != 2u)
        return 1;
    status = pr_064f_first_ega_change(ram, ram_size, cpu, report);
    return status == 0 && cpu->cs == 0x0709u && cpu->ip == 0x0415u ? 0 : 2;
}
