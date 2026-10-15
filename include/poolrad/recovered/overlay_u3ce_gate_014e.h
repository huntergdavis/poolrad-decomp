#ifndef POOLRAD_RECOVERED_OVERLAY_U3CE_GATE_014E_H
#define POOLRAD_RECOVERED_OVERLAY_U3CE_GATE_014E_H
#include <stddef.h>
#include <stdint.h>
#define PR_U3CE_014E_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u3ce_014e_cpu;
typedef struct {
    uint16_t call_ip,continuation_ip,target_cs,target_ip;
} pr_u3ce_014e_call;
typedef enum {
    PR_U3CE_014E_RETURNED = 0,
    PR_U3CE_014E_CALLBACK,
    PR_U3CE_014E_ARGUMENT
} pr_u3ce_014e_status;
/* Bounded zero/first-external gate, not a complete nonzero cleanup model.
 * IP 014E enters the procedure; IP 017F independently enters its post-first-call
 * continuation using supplied CPU/RAM. No prior heap operation is implied.
 * CALLBACK stops before CALL 017A or 01A5, with its far pointer-to-pointer
 * argument on SS:SP; no return words are pushed and no external call is run.
 * RETURNED executes the both-zero/second-zero RETF 2 and leaves pending intact.
 * Flat A20-disabled RAM, immutable modeled code, loaded call operands in RAM,
 * separate host CPU/output storage. Undefined logical AF on zero returns uses
 * this oracle's cleared-AF convention; callback arithmetic flags are defined.
 * Invalid arguments/entry IP leave all state and pending output unchanged.
 */
pr_u3ce_014e_status pr_u3ce_gate_014e(uint8_t *ram,size_t size,
                                    pr_u3ce_014e_cpu *cpu,
                                    pr_u3ce_014e_call *pending);
#endif
