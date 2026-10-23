#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_CALLER_1010_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_CALLER_1010_H
#include <stddef.h>
#include <stdint.h>
#define PR_U7F_1010_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u7f_1010_cpu;
typedef enum {
    PR_U7F_1010_OK=0, PR_U7F_1010_ARGUMENT, PR_U7F_1010_CALLER_LIMIT,
    PR_U7F_1010_DICE_LIMIT, PR_U7F_1010_CALLEE, PR_U7F_1010_TRANSFER
} pr_u7f_1010_status;
/* Complete U7F entry13 [1010,18E0), after far CALL; RETF6. Arguments at
 * initial SS:SP+4 output offset, +6 segment, +8 item type byte in a word.
 * Exact 1MiB A20-off RAM; separate CPU object; immutable loaded code and
 * bindings, synchronous access; writes must avoid modeled code. Wrapping far-return frames (including
 * alias-modified frames) are outside the contract. Initial
 * IP1010 and SP[0040,FFF6]. The three FillChar LCALLs (1022, 1044, 1088) read their SYSTEM segment from
 * the loader-relocated word at 1025, 1047 and 108B in the caller's code, live, not a constant. dice_cs is a
 * DECLARED PARAMETER (the host binds U9E +0DE3), not a constant: it is correct by design and would be wrong only
 * if a caller hard-coded it. +0FD2 reads its loaded stub binding, which must resolve to dice_cs:0DE3.
 * Positive caller budget is shared across taken loop-back entries 1608/181A/18AA.
 * Each dice call receives a fresh SYSTEM-call budget (zero is permitted).
 * LIMIT/CALLEE/TRANSFER preserve actual partial CPU14/RAM; ARGUMENT is
 * atomic. CALLEE reports a dependency rejection or changed inner return.
 * See docs/re/U7F_CALLER_1010.md for profile and evidence boundaries. */
/* Required loaded overlay bindings are checked before CPU/RAM mutation.
 * A cold/malformed required stub returns ARGUMENT with CPU14/RAM unchanged.
 * Call words and loaded EA jumps must remain immutable during execution.
 * See docs/LOADED_STUB_GUARDS.md for the exact direct/inherited scope. */
pr_u7f_1010_status pr_u7f_caller_1010(uint8_t *ram,size_t size,
    pr_u7f_1010_cpu *cpu,uint16_t dice_cs,size_t max_caller_iterations,
    unsigned max_system_calls);
#endif
