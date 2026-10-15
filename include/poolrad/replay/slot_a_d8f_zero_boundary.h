#ifndef POOLRAD_REPLAY_SLOT_A_D8F_ZERO_BOUNDARY_H
#define POOLRAD_REPLAY_SLOT_A_D8F_ZERO_BOUNDARY_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_D8F_ZERO_STEPPED=0,
 PR_D8F_ZERO_BLOCKED_COPY,
 PR_D8F_ZERO_BLOCKED_MEMORY,
 PR_D8F_ZERO_BLOCKED_PC,
 PR_D8F_ZERO_ARGUMENT
} pr_d8f_zero_status;
/* One instruction from actual zero-AL D8F entry through argument setup.
 * D9B blocks BEFORE B0B far CALL/frame/copy, reports original AF8:B0B,
 * returnDA0. Zero-path provenance is measured with a controlled original
 * sidecar-file-absent fixture; this is not an untouched baseline claim.
 * Requires audited original/loaded code and independent live CPU/RAM.
 * RAM <=1MiB; each word must fit one segment and host image. Effective
 * offsets wrap16bits; unsupported spans block atomically. State/report/RAM
 * must not overlap. Blocked CPU/RAM preserved; NULL also preserves report.
 * No runtime/allocator outcome injection, snapshot bridge or function credit. */
pr_d8f_zero_status pr_slot_a_d8f_zero_step(
 pr_slot_a_directory_state *state,pr_slot_a_string_0356_report *report);
#endif
