#ifndef POOLRAD_REPLAY_U3B0_POLL_0566_H
#define POOLRAD_REPLAY_U3B0_POLL_0566_H

#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include <stddef.h>
#include <stdint.h>

/* Source interval [0566,059c), stopping before 06cf or 059c. Caller owns
 * CPU, 1 MiB A20-off RAM and all service contexts. The caller supplies a live
 * pre-0566 frame only after preceding prompt/draw/timer effects; this API
 * cannot prove that provenance. Loaded code and all callback-accessed state
 * must remain valid. Never sets entry2's provided flag or executes RETF 16h. */
typedef struct {
    uint16_t crt_segment;
    uint16_t key_segment;
    const pr_r0899_key_env *key_env;
} pr_u3b0_poll_env;

typedef enum {
    PR_U3B0_POLL_NO_EVENT = 0,
    PR_U3B0_POLL_NON_EXTENDED,
    PR_U3B0_POLL_EXTENDED_DISABLED,
    PR_U3B0_POLL_EXTENDED,
    PR_U3B0_POLL_ARGUMENT,
    PR_U3B0_POLL_CALL_TARGET,
    PR_U3B0_POLL_TRANSFER,
    PR_U3B0_POLL_CRT_STOP,
    PR_U3B0_POLL_KEY_STOP
} pr_u3b0_poll_status;

typedef struct {
    uint16_t site;
    pr_crt_status crt_status;
    pr_r0899_key_status key_status;
    pr_r0899_key_diagnostic key;
} pr_u3b0_poll_diagnostic;

/* Nested failures retain all earlier effects; no status authorizes restarting
 * the step. FAR target bytes are checked at each reached call and must name
 * the caller-supplied loaded segments. This is a diagnostic projection, not
 * a validator of prior effects, loaded non-call instructions or native parity. */
pr_u3b0_poll_status pr_u3b0_poll_0566(uint8_t *ram, size_t ram_size,
    pr_r0899_cpu *cpu, const pr_u3b0_poll_env *env,
    pr_u3b0_poll_diagnostic *diagnostic);
#endif
