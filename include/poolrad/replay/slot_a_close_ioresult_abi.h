#ifndef POOLRAD_REPLAY_SLOT_A_CLOSE_IORESULT_ABI_H
#define POOLRAD_REPLAY_SLOT_A_CLOSE_IORESULT_ABI_H
#include "poolrad/replay/slot_a_file_probe_02eb_abi.h"
typedef enum {
    PR_SLOT_A_CLOSE_IORESULT_STEPPED=0, PR_SLOT_A_CLOSE_IORESULT_BLOCKED_DOS,
    PR_SLOT_A_CLOSE_IORESULT_BLOCKED_CALLER, PR_SLOT_A_CLOSE_IORESULT_BLOCKED_MEMORY,
    PR_SLOT_A_CLOSE_IORESULT_BLOCKED_PC, PR_SLOT_A_CLOSE_IORESULT_ARGUMENT
} pr_slot_a_close_ioresult_status;
typedef struct {
    unsigned valid;
    uint16_t original_segment, call_pc, return_pc, ax, bx;
} pr_slot_a_close_ioresult_report;
/* Actual resident 0324 -> Close1491 CPU prefix, stopping before INT21/14A6.
 * A separately seeded native14A8 state audits Close's RETF4, IOResult04A2,
 * DS:49CE writer and selector RETF4 to caller0292. No DOS callback, returned
 * flags/registers, RAM journal or continuous execution across INT21 supplied.
 * Reuses unchanged resident CPU steps; original probe's runtime gates remain.
 * Requires audited original/loaded code and disjoint state/report/RAM. Every
 * accessed span must fit one 64KiB segment and RAM<=1MiB. Unsupported PCs/spans
 * and typed gates preserve CPU/RAM; NULL state/report preserves report.
 * Undefined XOR AF follows observed zero. No full-C or playable credit. */
pr_slot_a_close_ioresult_status pr_slot_a_close_ioresult_step(
    pr_slot_a_directory_state *state,pr_slot_a_close_ioresult_report *report);
#endif
