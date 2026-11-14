#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_RUNNER_NEXT_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_RUNNER_NEXT_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
typedef enum { PR_COMPARE_RUNNER_FETCHED,PR_COMPARE_RUNNER_ARGUMENT,PR_COMPARE_RUNNER_JUMP_REFUSED,PR_COMPARE_RUNNER_LOOP_REFUSED,PR_COMPARE_RUNNER_EXIT,PR_COMPARE_RUNNER_PREFIX_STOP,PR_COMPARE_RUNNER_FETCH_REFUSED } pr_compare_runner_status;
/* Code references and host-object disjointness are caller-authenticated;
 * reached state is retained on any later refusal or exit. */
pr_compare_runner_status pr_slot_a_compare_runner_next(uint8_t *ram,size_t size,const uint8_t *loop,const uint8_t *fetch,pr_u1c_3171_cpu *cpu);
#endif
