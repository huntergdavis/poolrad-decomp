#ifndef POOLRAD_RESIDENT_00BA_SUFFIX_PARENT_CONTROL_H
#define POOLRAD_RESIDENT_00BA_SUFFIX_PARENT_CONTROL_H

#include "resident_00ba_second_printer_return.h"

typedef struct {
    uint8_t node_length;
    uint16_t pushed_length;
    uint16_t parent_ip;
} pr_00ba_suffix_parent_report;

/* Site zero returns to unit 077:+11D1; site one to +2ABB. Enter at
 * RES_00BA:188C after the second printer and stop just after the first
 * parent's length-plus-four push, before +11DD or +2AC8 respectively. */
int pr_00ba_suffix_parent_control(unsigned site, uint8_t *ram,
                                   size_t ram_size, pr_1816_caller_cpu *cpu,
                                   pr_00ba_suffix_parent_report *report);

#endif
