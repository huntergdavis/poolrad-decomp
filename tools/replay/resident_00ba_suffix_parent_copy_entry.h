#ifndef POOLRAD_RESIDENT_00BA_SUFFIX_PARENT_COPY_ENTRY_H
#define POOLRAD_RESIDENT_00BA_SUFFIX_PARENT_COPY_ENTRY_H

#include "resident_00ba_suffix_parent_control.h"

typedef struct {
    uint16_t return_ip, return_cs;
    uint16_t source_offset, source_segment;
    uint16_t destination_offset, destination_segment;
    uint16_t color, row, column;
} pr_00ba_suffix_parent_copy_frame;

/* Site zero enters at unit 077:+11DD, site one at +2AC8. Stop at the
 * SYSTEM 0AF8:0B0B copy entry, before executing the helper body. */
int pr_00ba_suffix_parent_copy_entry(
    unsigned site, uint8_t *ram, size_t ram_size,
    pr_1816_caller_cpu *cpu, pr_00ba_suffix_parent_copy_frame *frame);

#endif
