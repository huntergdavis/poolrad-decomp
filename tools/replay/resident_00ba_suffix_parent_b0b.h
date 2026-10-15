#ifndef POOLRAD_RESIDENT_00BA_SUFFIX_PARENT_B0B_H
#define POOLRAD_RESIDENT_00BA_SUFFIX_PARENT_B0B_H

#include "resident_00ba_suffix_parent_copy_entry.h"

typedef struct {
    uint16_t before_ip, after_ip;
    uint8_t source_length;
    uint8_t returned_to_parent;
} pr_00ba_suffix_parent_b0b_report;

/* Step one reached SYSTEM 0AF8:0B0B..0B22 instruction on either parent
 * frame. Return 2 at the first unsupported PC after RETF 4, preserving
 * CPU/RAM. The source and destination bytes come from caller RAM. */
int pr_00ba_suffix_parent_b0b_step(
    unsigned site, uint8_t *ram, size_t ram_size,
    pr_1816_caller_cpu *cpu, pr_00ba_suffix_parent_b0b_report *report);

#endif
