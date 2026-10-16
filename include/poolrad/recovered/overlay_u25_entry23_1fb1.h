#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY23_1FB1_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY23_1FB1_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_1FB1_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_1fb1_cpu;

/* The only external callee: resident BA:2199 sidecar lookup, RETF 0xA,
 * called at +2004 (key 27h) and +2027 (key 2Ah). */
typedef enum {
    PR_U25_1FB1_LOOKUP = 0
} pr_u25_1fb1_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all callee RAM/CPU effects (AL = found); this wrapper performs
 * the confirmed argument cleanup after a successful return. */
typedef int (*pr_u25_1fb1_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_1fb1_cpu *cpu,
                                    pr_u25_1fb1_call call);

typedef enum {
    PR_U25_1FB1_OK = 0,
    PR_U25_1FB1_ARGUMENT,
    PR_U25_1FB1_CALL_ABORT
} pr_u25_1fb1_status;

/* Entry at CS:1FB1 after the far CALL 2B:009B. SS:SP holds return IP/CS
 * and two far byte pointers: [BP+6] (minimum) and [BP+0xA] (maximum). A
 * cyclic list does not return, as in DOS. */
pr_u25_1fb1_status pr_u25_entry23_1fb1(
    uint8_t *ram, size_t ram_size, pr_u25_1fb1_cpu *cpu,
    pr_u25_1fb1_external external, void *context);

#endif
