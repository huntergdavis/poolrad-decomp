#ifndef POOLRAD_RESIDENT_00BA_SUFFIX_POST_B0B_PARENT_H
#define POOLRAD_RESIDENT_00BA_SUFFIX_POST_B0B_PARENT_H

#include "resident_00ba_suffix_parent_b0b.h"

typedef struct {
    uint16_t retained_offset, retained_segment;
    uint16_t next_offset, next_segment;
    uint16_t return_ip, callee_ip, callee_cs;
} pr_00ba_suffix_post_b0b_report;

/* Site zero: unit 077:+11F2 to printer 0709:0626.
 * Site one: unit 077:+2ADE to SYSTEM concat 0AF8:0B98.
 * Enter only after the measured B0B RETF 4; stop before either callee. */
int pr_00ba_suffix_post_b0b_parent(
    unsigned site, uint8_t *ram, size_t ram_size,
    pr_1816_caller_cpu *cpu, pr_00ba_suffix_post_b0b_report *report);

#endif
