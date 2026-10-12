#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_GETTABLE_0E5E_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_GETTABLE_0E5E_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x0E5E..+0x0EBB (file 0x13A4..0x1401, exactly 94
 * bytes, `retf` last byte, ten bytes of locals, no arguments, no result): the handler of ECL opcode
 * 0x2A (GETTABLE, reference name). The next routine (+0x0EBC) is separate. Evidence:
 * docs/re/U1C_GETTABLE_0E5E.md.
 *
 * In the original's order:
 *   loader(3) (0x2B:0x2A)                                              (+0x0E64..+0x0E67)
 *   base := word([0x7086], [0x7046]) via 0x2B:0x4D (slot 1)            (+0x0E6C..+0x0E79)
 *   idx := low BYTE of slot value(2) (0x2B:0x25)                       (+0x0E7C..+0x0E84)
 *   dest := word([0x7088], [0x7048]) via 0x2B:0x4D (slot 3)            (+0x0E87..+0x0E94)
 *   addr := idx (zero-extended) + base  (16-bit, wraps)                (+0x0E97..+0x0E9F)
 *   value := load word (0x2B:0x57, unit 0x25 +0x0E0C) at addr          (+0x0EA2..+0x0EAA)
 *   store word 0x2B:0x52 (dest, value)                                 (+0x0EAD..+0x0EB3)
 *   return
 *
 * Slot 1 is used only as an address WORD (its two value bytes, whatever its tag), not as a slot
 * value. The table is not carried by the handler: the element is whatever the lane-A load word
 * returns for `addr` (the window or script word, a class-4 pseudo-address, or a class-1 address
 * through the engine's load hook). Statuses of the load word are reported, not hidden: a
 * class-4 address the model cannot resolve (the engine returns an uninitialized local there)
 * stops the handler before the store.
 *
 * Classification: bounded model composed of the lane-A loader, operand word, slot value, load
 * word and store word. No new hooks. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_GETTABLE_OPERANDS 3u

typedef enum {
    PR_U1C_GETTABLE_OK = 0,
    PR_U1C_GETTABLE_ARGUMENT,    /* null state */
    PR_U1C_GETTABLE_CORE         /* a lane-A core status ended the handler (report->core_status); on HOOK the handler
                                    has completed and a class-1 read (slot or element) or the store ran the engine's
                                    hook model */
} pr_u1c_gettable_status;

typedef struct {
    uint16_t base;                   /* word(slot 1) */
    uint8_t index;                   /* low byte of slot value(2) */
    uint16_t addr;                   /* index + base, 16-bit */
    uint16_t value;                  /* the loaded element (valid when the load did not stop) */
    uint16_t dest;                   /* word(slot 3) when reached */
    unsigned loaded;                 /* the load word call was made */
    unsigned stored;                 /* the store word call was made */
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status;
    pr_rec_ecl_window_status load_status;
    pr_rec_ecl_window_status store_status;
} pr_u1c_gettable_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_gettable_status pr_u1c_op_gettable(pr_rec_ecl_loader_state *state, pr_u1c_gettable_report *report);

#endif
