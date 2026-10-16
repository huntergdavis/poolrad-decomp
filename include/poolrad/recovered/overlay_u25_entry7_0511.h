#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY7_0511_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY7_0511_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_0511_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_0511_cpu;

typedef enum {
    PR_U25_0511_U3B9_6 = 0, /* 3D0:003E = U3B9 +05D9 entry 6, RETF 4 */
    PR_U25_0511_U3B9_5      /* 3D0:0039 = U3B9 +0697 entry 5, RETF 6 */
} pr_u25_0511_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all callee RAM/CPU effects; this wrapper performs the confirmed
 * argument cleanup after a successful return. */
typedef int (*pr_u25_0511_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_0511_cpu *cpu,
                                    pr_u25_0511_call call);

typedef enum {
    PR_U25_0511_OK = 0,
    PR_U25_0511_ARGUMENT,
    PR_U25_0511_CALL_ABORT
} pr_u25_0511_status;

/* Entry at CS:0511 after the far CALL 2B:0043 (or the near PUSH CS; CALL
 * from +054E). SS:SP holds return IP/CS and two byte words: [BP+6] and
 * [BP+8]. */
pr_u25_0511_status pr_u25_entry7_0511(
    uint8_t *ram, size_t ram_size, pr_u25_0511_cpu *cpu,
    pr_u25_0511_external external, void *context);

#endif
