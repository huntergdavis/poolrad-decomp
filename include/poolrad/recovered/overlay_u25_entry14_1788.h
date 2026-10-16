#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY14_1788_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY14_1788_H

#include "poolrad/recovered/overlay_u25_tilde_caps_156d.h"

#define PR_U25_1788_RAM_SIZE PR_U25_156D_RAM_SIZE

/* Shared register ABI with the composed +156D callee. */
typedef pr_u25_156d_cpu pr_u25_1788_cpu;

typedef enum {
    PR_U25_1788_STRSTORE = 0, /* AF8:0B25 at +179D/+17B1, RETF 0xA */
    PR_U25_1788_U3B0_4,       /* 3C5:0034 = U3B0 +0D9A entry 4, RETF 0x28 */
    /* The composed +156D's runtime calls, offset by this base: */
    PR_U25_1788_156D_BASE     /* + pr_u25_156d_call */
} pr_u25_1788_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all callee RAM/CPU effects; the wrappers perform the confirmed
 * argument cleanup. Calls inside +156D arrive as BASE + its call code. */
typedef int (*pr_u25_1788_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_1788_cpu *cpu,
                                    unsigned call);

typedef enum {
    PR_U25_1788_OK = 0,
    PR_U25_1788_ARGUMENT,
    PR_U25_1788_CALL_ABORT,
    /* +156D's RETF 8 popped a return other than CS:17C6 from live RAM;
     * the CPU holds that transfer and +1788 does not continue. */
    PR_U25_1788_TRANSFER
} pr_u25_1788_status;

/* Entry at CS:1788 after the far CALL 2B:0066. SS:SP holds return IP/CS
 * and twenty argument words ([BP+6]..[BP+0x2D]). */
pr_u25_1788_status pr_u25_entry14_1788(
    uint8_t *ram, size_t ram_size, pr_u25_1788_cpu *cpu,
    pr_u25_1788_external external, void *context);

#endif
