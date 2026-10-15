#ifndef POOLRAD_REPLAY_SLOT_A_CBF_POINTER_STORE_H
#define POOLRAD_REPLAY_SLOT_A_CBF_POINTER_STORE_H
#include "poolrad/recovered/slot_a_directory_fullstate.h"
typedef enum {
 PR_CBF_STORE_STEPPED=0,
 PR_CBF_STORE_BLOCKED_CONTINUATION,
 PR_CBF_STORE_BLOCKED_MEMORY,
 PR_CBF_STORE_BLOCKED_PC,
 PR_CBF_STORE_ARGUMENT
} pr_cbf_store_status;
typedef struct {
 unsigned valid;
 uint16_t continuation_segment;
 uint16_t continuation_pc;
} pr_cbf_store_report;
/* One original instruction: actual loaded unit6F CBF LESDI,[BP+A],
 * CC2 ES:[DI+C8]=AX, or CC7 ES:[DI+CA]=DX. CCC stops before reload/use.
 * Requires audited original/loaded identity and independently captured actual
 * GetMem-return CPU/RAM. No allocator execution/output or snapshot bridge.
 * RAM <=1 MiB; multi-byte spans fit one segment and host image. Effective
 * offsets wrap16bits; unsupported spans block atomically. State/report/RAM
 * must not overlap. Blocked CPU/RAM preserved; NULL also preserves report.
 * No new function identity/count or continuous replay claim. */
pr_cbf_store_status pr_slot_a_cbf_pointer_store_step(
 pr_slot_a_directory_state *state,pr_cbf_store_report *report);
#endif
