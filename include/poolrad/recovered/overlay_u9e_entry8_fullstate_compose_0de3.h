#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_COMPOSE_0DE3_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_COMPOSE_0DE3_H

#include "poolrad/recovered/overlay_u9e_entry8_fullstate_prefix_0de3.h"

typedef pr_u9e_entry8_prefix_cpu pr_u9e_entry8_compose_cpu;

typedef enum {
    PR_U9E_ENTRY8_COMPOSE_RETURNED = 0,
    PR_U9E_ENTRY8_COMPOSE_ARGUMENT,
    PR_U9E_ENTRY8_COMPOSE_PREFIX_REJECTED,
    PR_U9E_ENTRY8_COMPOSE_CALL_ARGUMENT,
    PR_U9E_ENTRY8_COMPOSE_SYSTEM_ARGUMENT,
    PR_U9E_ENTRY8_COMPOSE_SYSTEM_FAILED,
    PR_U9E_ENTRY8_COMPOSE_SYSTEM_TRANSFER,
    PR_U9E_ENTRY8_COMPOSE_RETURN_TRANSFER,
    PR_U9E_ENTRY8_COMPOSE_POSTCALL_REJECTED,
    PR_U9E_ENTRY8_COMPOSE_LOOP_REJECTED,
    PR_U9E_ENTRY8_COMPOSE_TAIL_REJECTED,
    PR_U9E_ENTRY8_COMPOSE_LIMIT
} pr_u9e_entry8_compose_status;

/* Start at loaded U9E CS:+0DE3 with the original far-call frame and
 * 1 MiB A20-off RAM. The host supplies a maximum number of SYSTEM call
 * slices. A zero limit permits the zero-count path; a nonzero count stops
 * at +0E0E with LIMIT after the prefix. Each component's live CPU14/RAM
 * effects survive later failures. The loaded U9E and SYSTEM instruction
 * bodies must be original. B0:0048 dispatch is outside this API. */
pr_u9e_entry8_compose_status pr_u9e_entry8_fullstate_compose_0de3(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_compose_cpu *cpu,
    unsigned max_system_calls);

#endif
