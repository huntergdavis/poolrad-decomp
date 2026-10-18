#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_CLEAR_2DE4_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_CLEAR_2DE4_CPU_H
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"

#define PR_U38RC_RAM_SIZE (1u << 20)
typedef pr_u38c_cpu pr_u38rc_cpu;
typedef enum {
    PR_U38RC_OK = 0,
    PR_U38RC_ARGUMENT,
    PR_U38RC_CODE,
    PR_U38RC_CALLEE,
    PR_U38RC_TRANSFER
} pr_u38rc_status;

/* Complete U38 entry 108 (+2DE4..+2E08): roll(1,100); when AL <= 30,
 * helper(0Bh), then helper(35h); RETF 0Ah. Incoming AH survives MOV AL.
 * Composes the original U9E +0DE3/SYSTEM chain and U38 +0000 raw APIs.
 *
 * Requires exactly 1 MiB A20-off RAM, IP=2DE4, SP in [0040,FFFC],
 * original immutable loaded bodies and valid non-overlapping code mappings.
 * U38's far-call operand names a loaded EA 0DE3 stub; SYSTEM remains 0AF8.
 * Stack/data aliases are live; operands are reread in instruction order.
 * Multi-byte accesses are physically contiguous (A20 wraps).
 * max_system_calls bounds the dependency, whose live stop state survives.
 * ARGUMENT changes nothing; CODE stops at the unsupported far-call site;
 * CALLEE preserves the dependency stop; TRANSFER preserves an unexpected
 * callee return CS:IP. No rollback. callee_status (optional) receives -1
 * before any callee ran, otherwise the last raw dependency status.
 * Every reached U9E/SYSTEM far-return frame must fit within SS:0000..FFFF
 * (including after alias-induced BP changes); wrapped dependency RETF
 * frames, self-modifying code, interrupts and faults are outside contract.
 */
pr_u38rc_status pr_u38_roll_clear_2de4_cpu(
    uint8_t *ram, size_t ram_size, pr_u38rc_cpu *cpu,
    unsigned max_system_calls, int *callee_status);
#endif
