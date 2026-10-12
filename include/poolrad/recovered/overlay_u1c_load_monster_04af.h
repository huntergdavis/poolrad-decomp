#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_MONSTER_04AF_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_MONSTER_04AF_H

/* Bounded model of unit 0x1c +0x04AF..+0x0536 of ECL opcode 0x0B (LOAD MONSTER): the slice after the unit 0x25 entry-5 wrapper
 * returns, through the call to unit 0x3ce entry 4 (0x3E9:0x34, +0x0532). File 0x11F5..0x127C (unit base 0x0D46), 136 bytes, 49
 * instructions, SHA-256 1f65be4a3e667f96a5a670e960c2b51836b49b07ad06b83f7fcca0545dda222e (of +0x04AF..+0x0536; the routine's own hash is in
 * overlay_u1c_load_monster_prefix_0450.h). It continues from the prefix model's outputs. Evidence: docs/re/U1C_LOAD_MONSTER_04AF.md.
 *
 * In the original's order:
 *   Move(far A, local record copy, 0x11D)  (0xAF8:0x0A3F, `rep movsb`)                         (+0x04AF..+0x04BE)
 *   saved1 := B; saved2 := C   (the two list heads the wrapper returned)                          (+0x04C3..+0x04DF)
 *   count := AL of slot value(2); if count = 0 then count := 1  (unsigned `ja`)                   (+0x04E3..+0x04F8)
 *   p3 := AL of slot value(3)                                                                    (+0x04F8..+0x0500)
 *   cur := far [0x5D96]:[0x5D98]  (the party list head)                                          (+0x0503..+0x050D)
 *   local string := cs:0x044B  (StrStore, maximum 12)                                            (+0x0510..+0x051F)
 *   0x3E9:0x34 (unit 0x3ce entry 4, +0x01C8) with (far local string, p3, [0x6F83])               (+0x0524..+0x0532)
 *   the model STOPS at the return of that call (+0x0537): the tail walk and everything after it are not modelled
 *
 * The record copy is made from the object the wrapper left in variable A: the object bytes are host heap reached through the same
 * explicit object table as the entry-5 model (an unmapped pointer is UNMAPPED, a view shorter than 0x11D bytes from its offset is VIEW; both
 * stop before any other effect). A class-1 load-hook HOOK at slot value(2) (the count) or slot value(3) stops there (CORE/HOOK, later steps
 * not run). The code constant cs:0x044B is host data (a Pascal string of length 4; its text stays out of the repository). The callee
 * 0x3E9:0x34 is unresolved and an explicit fallible hook.
 *
 * Classification: bounded model of a slice. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"
#include "poolrad/recovered/overlay_u25_entry5_0411.h"

#define PR_U1C_LM04AF_RECORD_SIZE 0x11Du     /* mov ax, 0x11D : the Move count */
#define PR_U1C_LM04AF_STR_MAX 12u            /* mov ax, 0xC : StrStore maximum */

typedef struct pr_u1c_lm04af_env pr_u1c_lm04af_env;

typedef struct {
    /* 0x3E9:0x34 (unit 0x3ce entry 4): (far Pascal string, p3 = slot 3's low byte, b6f83 = [0x6F83]). Nonzero return stops with
       PR_U1C_LM04AF_CALLBACK. */
    int (*unit3ce_01c8)(void *ctx, const pr_u1c_lm04af_env *env, const uint8_t *pascal_string, uint8_t p3, uint8_t b6f83);
} pr_u1c_lm04af_hooks;

struct pr_u1c_lm04af_env {
    const pr_u25_e5_object *objects;         /* the host object table the entry-5 model used */
    size_t object_count;
    const uint8_t *str_044b;                 /* the code constant cs:0x044B (a Pascal string; length byte first) */
    uint16_t w5d96, w5d98;                   /* the party list head words [0x5D96], [0x5D98] (read only here) */
    uint8_t b6f83;                           /* DS byte [0x6F83] (read only here) */
    pr_u1c_lm04af_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_LM04AF_OK = 0,        /* the 0x3E9:0x34 hook returned OK; the model stops at +0x0537 */
    PR_U1C_LM04AF_ARGUMENT,      /* null state, env, report input or table inconsistency: rejected before any effect */
    PR_U1C_LM04AF_UNMAPPED,      /* the object pointer in A is not in the table */
    PR_U1C_LM04AF_VIEW,          /* the object's view has fewer than 0x11D bytes from its offset */
    PR_U1C_LM04AF_CORE,          /* slot value(2)/(3) failed or reported a class-1 HOOK (report->core_status) */
    PR_U1C_LM04AF_STRING,        /* the code constant pointer is NULL */
    PR_U1C_LM04AF_HOOK,          /* the 0x3E9:0x34 hook is NULL */
    PR_U1C_LM04AF_CALLBACK       /* the hook returned nonzero (report->callback_status) */
} pr_u1c_lm04af_status;

typedef struct {
    uint8_t record_copy[PR_U1C_LM04AF_RECORD_SIZE];   /* the 0x11D bytes moved from the object (valid when the Move happened) */
    unsigned moved;
    pr_far16 saved_b, saved_c;                       /* the two list heads saved at +0x04C3..+0x04DF */
    uint8_t count;                                   /* slot value(2) low byte, 0 replaced by 1 */
    uint8_t count_raw;
    uint8_t p3;                                      /* slot value(3) low byte */
    pr_far16 party_head;                             /* [0x5D96]:[0x5D98] (offset, segment) */
    uint8_t local_string[1 + PR_U1C_LM04AF_STR_MAX]; /* the StrStore result, length byte first */
    unsigned hook_called;
    int callback_status;
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status[2];
} pr_u1c_lm04af_report;

/* Continues from the prefix model: `var_a` is the object pointer, `var_b`/`var_c` the list heads the entry-5 wrapper returned. `report` may be NULL. */
pr_u1c_lm04af_status pr_u1c_load_monster_04af(pr_rec_ecl_loader_state *state, const pr_u1c_lm04af_env *env,
                                              pr_far16 var_a, pr_far16 var_b, pr_far16 var_c, pr_u1c_lm04af_report *report);

#endif
