#ifndef POOLRAD_RECOVERED_OVERLAY_U25_STEP_PROBE_0482_H
#define POOLRAD_RECOVERED_OVERLAY_U25_STEP_PROBE_0482_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_0482_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_0482_cpu;

/* The only external callee: 3D5:002A, unit 3BD entry 2, RETF 6. */
typedef enum {
    PR_U25_0482_U3BD_2 = 0
} pr_u25_0482_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all callee RAM/CPU effects; this wrapper performs the confirmed
 * argument cleanup after a successful return. */
typedef int (*pr_u25_0482_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_0482_cpu *cpu,
                                    pr_u25_0482_call call);

typedef enum {
    PR_U25_0482_OK = 0,
    PR_U25_0482_ARGUMENT,
    PR_U25_0482_CALL_ABORT,
    PR_U25_0482_CODE,       /* reached loaded call/stub metadata differs */
    PR_U25_0482_TRANSFER,   /* actual nested return changed CS:IP */
    PR_U25_0482_CODE_ALIAS, /* reserved caller stack overlaps caller code */
    PR_U25_0482_CALLEE      /* reached dependency rejected its entry */
} pr_u25_0482_status;

/* Entry at CS:0482 after the far CALL 2B:003E. SS:SP holds return IP/CS
 * and the direction, y and x byte words, which the routine may rewrite in
 * place. Returns the result byte in AL. */
pr_u25_0482_status pr_u25_step_probe_0482(
    uint8_t *ram, size_t ram_size, pr_u25_0482_cpu *cpu,
    pr_u25_0482_external external, void *context);

/* Complete caller plus real U3BD +047B/+031F, without callbacks. Exactly
 * 1 MiB synchronous A20-off RAM, distinct stable CPU/RAM, IP=0482 and
 * SP in [0080,FFFA]. The 128-byte reserved stack cannot overlap caller
 * code. All loaded code/stubs must remain original and immutable; other
 * data/code aliases are outside the contract. The reached 9A call and EA
 * loaded stub are validated; fixed target offset 047B, relocated segments.
 * Data/frame/window aliases and actual RETF6 are preserved. Argument and
 * reserved-code checks reject atomically; later CODE/CALLEE/TRANSFER retain
 * reached state. Logical AF follows the callee's zero profile.
 * See docs/re/U25_STEP_PROBE_0482_COMPOSED.md.
 */
pr_u25_0482_status pr_u25_step_probe_0482_fullstate(
    uint8_t *ram, size_t ram_size, pr_u25_0482_cpu *cpu);

#endif
