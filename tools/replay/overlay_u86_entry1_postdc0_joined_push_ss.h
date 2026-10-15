#ifndef POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_JOINED_PUSH_SS_H
#define POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_JOINED_PUSH_SS_H

#include "resident_1816_caller_routes.h"

typedef struct {
    uint16_t before_sp, stop_sp, pushed_ss, stop_ip;
} pr_u86_entry1_postdc0_joined_push_ss_report;

/* Enter U86:+019F after LEA DI with the joined SS frame. PUSH SS and
 * stop at +01A0 before PUSH DI. */
int pr_u86_entry1_postdc0_joined_push_ss(
    uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_u86_entry1_postdc0_joined_push_ss_report *report);

#endif
