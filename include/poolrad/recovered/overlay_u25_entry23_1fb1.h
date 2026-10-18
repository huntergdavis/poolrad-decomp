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
    PR_U25_1FB1_CALL_ABORT,  /* hook entry: the hook declined */
    PR_U25_1FB1_CODE,        /* composed entry: a reached call site is not LCALL xxxx:2199 (9A 99 21); stopped before it */
    PR_U25_1FB1_CALLEE,      /* composed entry: the RES_00BA_2199 model stopped; *callee_status holds its status */
    PR_U25_1FB1_TRANSFER,    /* composed entry: the model returned to a CS:IP other than the call's next instruction */
    PR_U25_1FB1_LIMIT        /* composed entry: host cap on the node walk; CS:IP = CS:(entry IP + 2Fh) */
} pr_u25_1fb1_status;

/* Entry at CS:1FB1 after the far CALL 2B:009B. SS:SP holds return IP/CS
 * and two far byte pointers: [BP+6] (minimum) and [BP+0xA] (maximum).
 * Hook entry (provisional): the lookup is the caller's hook and a cyclic
 * list does not return, as in DOS. LES pairs and the RETF frame are read
 * physically contiguous in both entries, as the Unicorn oracle reads them. */
pr_u25_1fb1_status pr_u25_entry23_1fb1(
    uint8_t *ram, size_t ram_size, pr_u25_1fb1_cpu *cpu,
    pr_u25_1fb1_external external, void *context);

/* Strict full-state entry (RAM exactly 1 MiB): the same body with both
 * lookups run as real far calls. Each reached site is read from RAM
 * (9A, offset 2199h, segment); CS and the next IP (relative to the entry
 * IP) are pushed, and pr_res_00ba_2199_cpu runs at that segment with
 * lookup_max_loopbacks and must return there. max_loopbacks caps the taken
 * JMP +2075 -> +1FE0 loop-backs (one per node visited); on the next one the
 * model stops at the loop head with IP = entry IP + 2Fh, SP = BP - 0Ah and
 * every earlier effect kept (LIMIT), the original's state on reaching
 * +1FE0 that time. A list of n nodes needs n loop-backs; a cyclic list
 * makes the original loop forever. *callee_status (may be NULL) receives
 * the last lookup's pr_res_2199_status, or -1 when none ran. */
pr_u25_1fb1_status pr_u25_entry23_1fb1_composed(
    uint8_t *ram, size_t ram_size, pr_u25_1fb1_cpu *cpu,
    unsigned max_loopbacks, unsigned lookup_max_loopbacks, int *callee_status);

#endif
