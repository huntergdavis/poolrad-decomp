#ifndef POOLRAD_REPLAY_SLOT_A_OPEN_025D_ASSIGN_ABI_H
#define POOLRAD_REPLAY_SLOT_A_OPEN_025D_ASSIGN_ABI_H
#include "poolrad/replay/slot_a_file_probe_02eb_abi.h"
typedef enum {
    PR_SLOT_A_OPEN_025D_STEPPED = 0,
    PR_SLOT_A_OPEN_025D_BLOCKED_RESET,
    PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY,
    PR_SLOT_A_OPEN_025D_BLOCKED_PC,
    PR_SLOT_A_OPEN_025D_ARGUMENT
} pr_slot_a_open_025d_status;
typedef pr_slot_a_string_0356_report pr_slot_a_open_025d_report;
/* One original instruction from actual 2A8E:0C26 -> resident 1026:025D,
 * two B25 copies, nested near CALL :02EB and reached SYSTEM Assign :13E2.
 * The existing probe API keeps its Assign gate; this separate composition
 * executes that call and the real RETF 8. At :031F block unchanged before
 * original AF8:1410 Reset, return :0324, including its far-frame push.
 * No DOS/file-open result, record read, returned AL or snapshot bridge.
 * Requires audited original/loaded code and nonoverlapping state/report/RAM.
 * RAM <= 1 MiB and spans within 64 KiB. Individual pointers wrap to 16 bits.
 * REP is atomic; DF-set string instructions outside reached CLD block.
 * Failed steps preserve CPU/RAM; NULL preserves report. Logic AF=0 follows
 * existing helper/native behavior. No full-C, identity or count credit. */
pr_slot_a_open_025d_status pr_slot_a_open_025d_step(
    pr_slot_a_directory_state *state, pr_slot_a_open_025d_report *report);
#endif
