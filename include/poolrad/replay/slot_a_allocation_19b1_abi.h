#ifndef POOLRAD_REPLAY_SLOT_A_ALLOCATION_19B1_ABI_H
#define POOLRAD_REPLAY_SLOT_A_ALLOCATION_19B1_ABI_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
    PR_SLOT_A_ALLOCATION_STEPPED = 0,
    PR_SLOT_A_ALLOCATION_BLOCKED_GETMEM,
    PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY,
    PR_SLOT_A_ALLOCATION_BLOCKED_PC,
    PR_SLOT_A_ALLOCATION_ARGUMENT
} pr_slot_a_allocation_status;
/* One original instruction at 2A8E:19B1 (MOV AX,011D) or :19B4 (PUSH AX).
 * At :19B5 block before SYSTEM AF8:023F GetMem and its far-frame push;
 * report the actual caller/dependency/return addresses. No allocator or
 * file-I/O effects/results are supplied. Existing +19AD gate is unchanged.
 * Requires audited code identity, live CPU/RAM and nonoverlapping state,
 * report and RAM. ram_size <= 1 MiB; a word must fit its 64 KiB segment
 * and the host image. Unsupported states preserve CPU/RAM atomically;
 * NULL arguments also preserve report. No route/identity/count credit. */
pr_slot_a_allocation_status pr_slot_a_allocation_19b1_step(
    pr_slot_a_directory_state *state, pr_slot_a_string_0356_report *report);
#endif
