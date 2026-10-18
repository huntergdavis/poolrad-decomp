#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_CALLER_32E6_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_CALLER_32E6_H
#include <stddef.h>
#include <stdint.h>
#define PR_U5E_32E6_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u5e_32e6_cpu;
typedef struct { uint16_t slots_cs,chain_cs,system_cs; } pr_u5e_32e6_layout;
typedef struct {
    size_t slot_iterations,chain_iterations,line_iterations;
    unsigned sort_loopbacks;
    size_t copy_iterations;
} pr_u5e_32e6_limits;
typedef enum {
    PR_U5E_32E6_OK=0,PR_U5E_32E6_ARGUMENT,PR_U5E_32E6_SLOT_LIMIT,
    PR_U5E_32E6_CHAIN_LIMIT,PR_U5E_32E6_LINE_LIMIT,PR_U5E_32E6_SORT_LIMIT,
    PR_U5E_32E6_COPY_LIMIT,PR_U5E_32E6_CALLEE,PR_U5E_32E6_TRANSFER
} pr_u5e_32e6_status;
/* Whole U5E [32E6,3389), RETF6: stack arguments are parent-frame offset,
 * slot-pointer offset, slot-pointer segment. Output is SS:parent-DA count
 * and SS:parent-DB+3*i records, i=1..count, copied forward from DS:6718+3*i.
 * Exact 1MiB A20-off RAM; distinct CPU/layout/limits; immutable code and
 * bindings, synchronous access, all writes must avoid modeled code.
 * Initial IP32E6 and SP[00A4,FFF6]. slots_cs binds U3C6 +0000 and
 * +0BE7/+0C0F/+0C37/+0C5F; chain_cs binds U3C2 +0909 and dependencies;
 * system_cs binds Move +0A3F. Nested +002E reads its Move bindings live.
 * All budgets positive. Slot budget resets per getter at +0C71; chain,
 * line, sort budgets follow +0909; copy budget counts visits to +3349.
 * Stops preserve exact partial RAM/CPU. CALLEE retains dependency rejection
 * or unexpected return state; TRANSFER retains direct changed-return state.
 * Nonwrapping return frames only; writes may alias data/stack but not code.
 * See docs/re/U5E_CALLER_32E6.md for measured coverage and limitations. */
pr_u5e_32e6_status pr_u5e_caller_32e6(uint8_t *ram,size_t size,
    pr_u5e_32e6_cpu *cpu,const pr_u5e_32e6_layout *layout,
    const pr_u5e_32e6_limits *limits);
#endif
