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
    PR_U25_0F53_CALL_ABORT,  /* hook entry: the hook declined */
    PR_U25_0F53_CODE,        /* composed entry: a reached call site is not LCALL xxxx:0B25 (9A 25 0B); stopped before it, IP at the site */
    PR_U25_0F53_TRANSFER,    /* composed entry: the string store's RETF 0Ah popped a CS:IP other than the call's next instruction */
    PR_U25_0F53_LIMIT        /* composed entry: host cap on an element loop's taken JNE loop-backs; IP at the loop's INC */
} pr_u25_0f53_status;

/* Entry at CS:0F53 after the far CALL 2B:005C. SS:SP holds return IP/CS,
 * the far source-string pointer and the address word. The near +06FB
 * classifier is executed in place on the same RAM and CPU. Hook entry
 * (provisional): the two string stores are the caller's hook. LES pairs and
 * the RETF frames are read physically contiguous in both entries, as the
 * Unicorn oracle reads them. */
pr_u25_0f53_status pr_u25_store_string_0f53(
    uint8_t *ram, size_t ram_size, pr_u25_0f53_cpu *cpu,
    pr_u25_0f53_external external, void *context);

/* Strict full-state entry (RAM exactly 1 MiB): the same body with both
 * string stores (+0F69, and +1010 on the class-1 address 6B00h) run as real
 * far calls. Each reached site is read from RAM (9A, offset 0B25h,
 * segment); CS and the next IP (relative to the entry IP) are pushed, and
 * pr_system_strstore_0b25 runs at that segment and must return there. The
 * +06FB classifier's near CALL pushes the entry-relative return IP (entry
 * + 22h); its only frame write is SS:[BP'-1], so it always returns there.
 * max_loopbacks caps the taken JNE loop-backs of the element loop that
 * runs (+0FD5, +1063, +10D0 or +1136); on the next one the model stops at
 * that loop's INC BYTE [BP-103h] (entry IP + 4Fh, DDh, 14Ah or 1B6h) with
 * every earlier effect kept (LIMIT), the original's state on reaching it
 * that time. Without aliasing a loop takes at most 255 loop-backs; an
 * element store into the loop's own index or bound can make the original
 * loop forever. *callee_status (may be NULL) receives the last store's
 * pr_system_0b25_status, or -1 when none ran. */
pr_u25_0f53_status pr_u25_store_string_0f53_composed(
    uint8_t *ram, size_t ram_size, pr_u25_0f53_cpu *cpu,
    unsigned max_loopbacks, int *callee_status);

#endif
