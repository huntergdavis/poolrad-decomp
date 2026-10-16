#ifndef POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_JGE_FALL_MOV_AL0_AFTER_FOURTH_PUSH_H
#define POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_JGE_FALL_MOV_AL0_AFTER_FOURTH_PUSH_H

#include "resident_1816_caller_routes.h"

typedef struct {
    uint16_t before_ax, stop_ax, before_flags, stop_ip;
} pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push_report;

/* Enter U86:+015E after the fourth PUSH AX with its four-word SS frame.
 * Set AL=0 and stop at +0160 before the next PUSH AX. */
int pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push(
    uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push_report *report);

#endif
