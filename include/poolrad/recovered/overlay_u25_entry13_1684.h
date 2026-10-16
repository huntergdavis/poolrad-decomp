#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY13_1684_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY13_1684_H

#include "poolrad/recovered/overlay_u25_tilde_caps_156d.h"

#define PR_U25_1684_RAM_SIZE PR_U25_156D_RAM_SIZE

/* Shared register ABI with the composed +156D callee. */
typedef pr_u25_156d_cpu pr_u25_1684_cpu;

typedef enum {
    PR_U25_1684_STRSTORE = 0, /* AF8:0B25 at +1699/+16AD, RETF 0xA */
    PR_U25_1684_U3B0_2,       /* 3C5:002A = U3B0 +03BD entry 2, RETF 0x16 */
    PR_U25_1684_U77_10,       /* 85:0052 = U77 +27DF entry 10, RETF 2 */
    PR_U25_1684_LOAD_PIECES,  /* resident BA:0767, RETF 4 */
    PR_U25_1684_IN_SET,       /* AF8:0DC0 set test (ZF), RETF 6 */
    /* The composed +156D's runtime calls, offset by this base: */
    PR_U25_1684_156D_BASE     /* + pr_u25_156d_call */
} pr_u25_1684_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all callee RAM/CPU effects; the wrappers perform the confirmed
 * argument cleanup. Calls inside +156D arrive as BASE + its call code. */
typedef int (*pr_u25_1684_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_1684_cpu *cpu,
                                    unsigned call);

typedef enum {
    PR_U25_1684_OK = 0,
    PR_U25_1684_ARGUMENT,
    PR_U25_1684_CALL_ABORT,
    /* +156D's RETF 8 popped a return other than CS:16C2 from live RAM;
     * the CPU holds that transfer and +1684 does not continue. */
    PR_U25_1684_TRANSFER
} pr_u25_1684_status;

/* Entry at CS:1684 after the far CALL 2B:0061. SS:SP holds return IP/CS
 * and nine argument words ([BP+6]..[BP+0x17]). Returns the result in AL.
 * The input loop ends only when the hooks deliver a terminating key, as in
 * DOS; there is no host cap. */
pr_u25_1684_status pr_u25_entry13_1684(
    uint8_t *ram, size_t ram_size, pr_u25_1684_cpu *cpu,
    pr_u25_1684_external external, void *context);

#endif
