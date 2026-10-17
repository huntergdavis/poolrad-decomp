#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY1_0136_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY1_0136_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_0136_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_0136_cpu;

typedef enum {
    PR_U25_0136_CALL_0E0C = 0,
    PR_U25_0136_CALL_06AF
} pr_u25_0136_call_kind;

typedef enum {
    PR_U25_0136_RETURNED = 0,
    PR_U25_0136_ARGUMENT,
    PR_U25_0136_CALLEE_REQUIRED,
    PR_U25_0136_CALLEE_FAILED,
    PR_U25_0136_TRANSFER
} pr_u25_0136_status;

/* Both local CALLs are preceded by PUSH CS, so the callee sees a far
 * return frame even though the CALL instruction itself is near. The
 * callback owns the full callee, its return, cleanup, and RAM effects. */
typedef int (*pr_u25_0136_call)(void *context, pr_u25_0136_call_kind kind,
                                uint8_t *ram, size_t ram_size,
                                pr_u25_0136_cpu *cpu);

typedef struct {
    pr_u25_0136_call call;
    void *context;
} pr_u25_0136_io;

/* Complete original GAME.OVR U25 entry 1, +0136..+01B5. The body
 * assumes immutable loaded U25 code; it does not infer external callee
 * effects. The unmatched type path deliberately returns live BP-local RAM.
 * RETF uses the contiguous four-byte frame profile before SP wraps, matching
 * the load-word composition/Unicorn audit; native boundary parity is separate. */
pr_u25_0136_status pr_u25_entry1_0136(
    uint8_t *ram, size_t ram_size, pr_u25_0136_cpu *cpu,
    const pr_u25_0136_io *io, pr_u25_0136_call_kind *stopped_at);

#endif
