#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PREFIX_0482_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PREFIX_0482_H
#include <stddef.h>
#include <stdint.h>
#define PR_U6F_0482_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6f_0482_cpu;
typedef struct {
    uint16_t call_ip, continuation_ip, target_cs, target_ip;
} pr_u6f_0482_pending_call;
typedef enum {
    PR_U6F_0482_CALLBACK = 1,
    PR_U6F_0482_CONTROL_STOP,
    PR_U6F_0482_ARGUMENT
} pr_u6f_0482_status;
/* Bounded +0482..+04D8 prefix, not the complete +0482 function.
 * Flat A20-disabled RAM; models pinned immutable caller/helper instructions.
 * Loaded caller call operands/literals must be in RAM. Entry IP is 0482.
 * Host CPU and pending-output storage must not overlap RAM or each other.
 * Closed calls: 0802:094A, SYSTEM 0B0B/0C3B/0B98, using loaded segment operands.
 * CALLBACK stops BEFORE the unresolved far CALL at 04D8: no return words pushed,
 * no host callback invoked. CPU and RAM include all preceding effects; pending
 * gives the loaded target. Arguments remain on SS:SP. No continuation is run.
 * A changed closed-helper far return yields CONTROL_STOP with CPU at that return
 * target; pending is untouched. Undefined logical AF follows a cleared-AF
 * convention on such early stops; final callback flags are all defined.
 * Invalid buffers, output pointers or entry IP return ARGUMENT without mutation.
 */
pr_u6f_0482_status pr_u6f_prefix_0482(uint8_t *ram, size_t ram_size,
                                    pr_u6f_0482_cpu *cpu,
                                    pr_u6f_0482_pending_call *pending);
#endif
