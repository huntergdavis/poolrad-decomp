#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY9_ROLL_LATCH_0E2E_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY9_ROLL_LATCH_0E2E_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_0E2E_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_0e2e_cpu;

/* Called at CS:+0DE3 after PUSH CS / CALL NEAR. SS:SP holds return IP
 * 0E46, return CS, range WORD, then count WORD. The callback supplies
 * the complete callee effects and leaves SP on this frame for RETF 4.
 * The merged pr_u9e_entry8_roll supplies its AL and seed effects. */
typedef int (*pr_u9e_0e2e_roll)(void *context, uint8_t *ram,
                                 size_t ram_size, pr_u9e_0e2e_cpu *cpu);

typedef enum {
    PR_U9E_0E2E_OK = 0,
    PR_U9E_0E2E_ARGUMENT,
    PR_U9E_0E2E_CALL_ABORT
} pr_u9e_0e2e_status;

pr_u9e_0e2e_status pr_u9e_entry9_roll_latch_0e2e(
    uint8_t *ram, size_t ram_size, pr_u9e_0e2e_cpu *cpu,
    pr_u9e_0e2e_roll roll, void *context);


#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
typedef pr_u9e_entry8_compose_cpu pr_roll_chain_cpu;
typedef enum {
    PR_ROLL_CHAIN_RETURNED=0, PR_ROLL_CHAIN_ARGUMENT, PR_ROLL_CHAIN_CODE,
    PR_ROLL_CHAIN_CALLEE, PR_ROLL_CHAIN_TRANSFER
} pr_roll_chain_status;
typedef struct { int latch_status, dice_status; } pr_roll_chain_diagnostic;

/* Complete U9E [0E2E,0E52), RETF4; compose raw +0DE3/SYSTEM.
 * Exact 1 MiB A20-off RAM, entry IP0E2E, SP0020..FFFC. Original immutable
 * loaded code, SYSTEM at 0AF8 with its original multiplier at +1245;
 * separate host CPU/diagnostic/RAM storage, no MMIO/IRQ/fault/SMC.
 * Every reached far-return frame must fit within SS:0000..FFFF, including
 * after alias-induced BP changes. Effective offsets wrap at 16 bits;
 * words read physically contiguous bytes with 20-bit wrapping.
 * max_system_calls bounds the dice dependency. Partial CPU/RAM state and
 * its status survive CALLEE, except LIMIT explicitly clears undefined AF
 * after XOR AH,AH (+0E0B), selecting the measured Unicorn stop profile.
 * The legacy dice API retains AF; no native budget-stop claim is made.
 * Changed return CS:IP survives TRANSFER.
 * Guard near CALL before its push. Invalid host arguments reject atomically.
 * Valid entry initializes both diagnostic fields to -1; dice_status records
 * the dependency result, latch_status remains -1 for direct latch entry.
 * The DS:681C store precedes a fresh read of the live count argument.
 * See docs/re/U38_LINK_ROLL_2B20_CPU.md for measured coverage and limits. */
pr_roll_chain_status pr_u9e_roll_latch_0e2e_cpu(uint8_t *ram,size_t size,
    pr_roll_chain_cpu *cpu,unsigned max_system_calls,
    pr_roll_chain_diagnostic *diagnostic);

#endif
