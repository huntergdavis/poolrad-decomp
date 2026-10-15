#include "resident_00ba_suffix_first_glyph.h"

int pr_00ba_suffix_first_glyph(uint8_t *ram, size_t ram_size,
                               pr_1816_caller_cpu *cpu,
                               pr_064f_ega_report *report) {
    int status;
    if (ram == NULL || cpu == NULL || report == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x064Fu) return 1;
    status = pr_064f_first_glyph_entry(ram, ram_size, cpu);
    if (status != 0) return 2;
    status = pr_064f_first_ega_change(ram, ram_size, cpu, report);
    return status == 0 && cpu->cs == 0x0709u && cpu->ip == 0x0415u ? 0 : 2;
}
