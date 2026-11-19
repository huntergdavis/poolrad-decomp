#ifndef POOLRAD_REPLAY_SLOT_A_ENTRY1_TO_0742_H
#define POOLRAD_REPLAY_SLOT_A_ENTRY1_TO_0742_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u68_entry1_prefix.h"

/* Slice 1 of the t4492 entry-1 composition: unit 0x68 entry 1 from its public
 * entry +0x0D6D (before the prologue) to the far call at +0x0742 inside the
 * coordinate writer +0x073B, over a 1 MiB-or-smaller conventional RAM image.
 * Evidence: docs/re/SLOT_A_ENTRY1_TO_0742.md.
 *
 * It composes the merged supplied-state projection pr_u68_e1_prefix (selector,
 * window word and unit 0x3BD entry 3) with the stack stores that the original
 * bytes make between +0x0D6D and +0x0742:
 *   entry 1   push bp; mov bp,sp; sub sp,0x12C; [bp-1]:=0 ... [bp-1]:=1
 *   commit    cmp [bp-1],0; push cs; call near +0x073B  (return IP 0x1031)
 *   +0x073B   push bp; mov bp,sp; push word [DS:0x2628]
 * and stops BEFORE the far call 0x8AE:0x00DC at +0x0742. It does not execute
 * that call, the rest of +0x073B, the commit tail, 0xBA:0x290B, or any callee.
 *
 * Not modeled, deliberately (explicit stops, never silently skipped):
 *   - the stack bytes below the final SP that the unit 0x3BD entry 3 call chain
 *     (and any interrupt frame) leaves behind: dead-stack residue, see the doc
 *   - everything after +0x0742
 * The model never writes outside the stores listed above. */

#define PR_SLOT_A_E1_RETURN_IP_073B 0x1031u  /* +0x102E call (3 bytes) -> next IP */
#define PR_SLOT_A_E1_LOCALS 0x012Cu          /* sub sp, 0x12C at +0x0D70 */
#define PR_SLOT_A_E1_STOP_IP 0x0742u         /* far call 0x8AE:0x00DC */
#define PR_SLOT_A_E1_FRAME_STORES 6u

/* Registers at the moment +0x0D6D is about to execute (the caller's far call
 * has already pushed its 4-byte return address). */
typedef struct {
    uint16_t ds, ss, cs;   /* DS, SS, and CS (the loaded unit 0x68 segment) */
    uint16_t sp, bp;       /* SP at +0x0D6D, and the caller's BP */
} pr_slot_a_e1_regs;

typedef enum {
    PR_SLOT_A_E1_ARGUMENT = 0,   /* null pointer or an unusable RAM size; nothing written */
    PR_SLOT_A_E1_MISSING_INPUT,  /* a reached input or store lies outside RAM; nothing written */
    PR_SLOT_A_E1_NOT_REACHED,    /* the projection stops elsewhere; nothing written */
    PR_SLOT_A_E1_STOP_0742       /* the six frame stores were written; next PC = +0x0742 */
} pr_slot_a_e1_status;

typedef struct {
    pr_u68_e1_prefix_stop prefix_stop;  /* what pr_u68_e1_prefix reported */
    uint8_t entry3_result;
    unsigned entry3_valid;
    uint16_t sp, bp, ip, cs;            /* registers at the stop (valid on STOP_0742) */
    unsigned frame_bytes_written;       /* 11 on STOP_0742, else 0 */
} pr_slot_a_e1_report;

/* Reads the entry-1 inputs from `ram` (DS offsets 0x49FA, 0x6AAD..0x6AAF,
 * 0x84DC, 0x2628, far 0x49D6 window 1 word 0x592, far 0x6A5C 1,024-byte
 * buffer), runs pr_u68_e1_prefix, and on PR_U68_PREFIX_BEFORE_073B writes the
 * six frame stores into `ram`. All reached addresses are checked before any
 * write. `report` is required and is initialized on every return. */
pr_slot_a_e1_status pr_slot_a_entry1_to_0742(uint8_t *ram, size_t ram_size,
                                             const pr_slot_a_e1_regs *regs,
                                             pr_slot_a_e1_report *report);

#endif
