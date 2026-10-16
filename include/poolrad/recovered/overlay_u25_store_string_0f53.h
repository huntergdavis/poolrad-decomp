#ifndef POOLRAD_RECOVERED_OVERLAY_U25_STORE_STRING_0F53_H
#define POOLRAD_RECOVERED_OVERLAY_U25_STORE_STRING_0F53_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_0F53_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_0f53_cpu;

/* The only external callee: SYSTEM AF8:0B25 StrStore, RETF 0xA. */
typedef enum {
    PR_U25_0F53_STRSTORE = 0
} pr_u25_0f53_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all runtime-routine RAM/CPU effects; this wrapper performs the
 * confirmed argument cleanup after a successful return. */
typedef int (*pr_u25_0f53_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_0f53_cpu *cpu,
                                    pr_u25_0f53_call call);

typedef enum {
    PR_U25_0F53_OK = 0,
    PR_U25_0F53_ARGUMENT,
    PR_U25_0F53_CALL_ABORT
} pr_u25_0f53_status;

/* Entry at CS:0F53 after the far CALL 2B:005C. SS:SP holds return IP/CS,
 * the address word and the far source-string pointer. The near +06FB
 * classifier is executed in place on the same RAM and CPU. */
pr_u25_0f53_status pr_u25_store_string_0f53(
    uint8_t *ram, size_t ram_size, pr_u25_0f53_cpu *cpu,
    pr_u25_0f53_external external, void *context);

#endif
