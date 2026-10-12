#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_MONSTER_PREFIX_0450_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_MONSTER_PREFIX_0450_H

/* Bounded model of ONLY the entry prefix of DOS v1.3 GAME.OVR unit 0x1c +0x0450 (ECL opcode 0x0B, LOAD MONSTER; the
 * whole routine is +0x0450..+0x0816, file 0x1196..0x155C, 967 bytes, SHA-256
 * bf1903765b12e98cfc5749630f31e2bedcabc607346255cdfae6b641759716cd) up to and including the first call to unit 0x25
 * entry 5 (+0x04AA, 0x2B:0x39, modelled by overlay_u25_entry5_0411.h). Evidence: docs/re/U1C_LOAD_MONSTER_0450_MAP.md.
 *
 * In the original's order:
 *   saved := far [0x5D92]:[0x5D94] (the active-character pointer; restored only at +0x0806, not modelled)   (+0x0457..+0x0461)
 *   loader(3) (0x2B:0x2A)                                                                                  (+0x0468..+0x046B)
 *   [0x4435] >= 0x3F: return at once (+0x0813); the loader's PC advance stands, nothing else is touched     (+0x0470..+0x0477)
 *   id := AL of slot value(1) (0x2B:0x25)  (a class-1 load-hook HOOK stops here, before the wrapper)       (+0x048A..+0x0492)
 *   0x2B:0x39 (unit 0x25 entry 5) with the id word (`mov al,[bp-1]; push ax`: the wrapper reads only AL) and the far
 *       variables A (the object pointer), B (list head 1, the +0xC8 chain) and C (list head 2, the +0x7F chain) (+0x0495..+0x04AA)
 *   the model STOPS at the return of that call (+0x04AF): everything after it is not modelled
 *
 * Slot values 2 and 3 are read AFTER that call in the original (+0x04E6, +0x04FB), so they are not part of the prefix. The
 * wrapper's unresolved pieces (GetMem, the unit 0x6f record initializer 0x7C:0x3E) are its explicit hooks; the model
 * invents no object contents, no count, no party links and no final PC. A, B and C start as explicit host stack residue (the
 * wrapper zeroes A first; B and C are written by it only on success).
 *
 * Classification: bounded model of a prefix. Not a strict translation of the routine. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"
#include "poolrad/recovered/overlay_u25_entry5_0411.h"

#define PR_U1C_LMON_OPERANDS 3u
#define PR_U1C_LMON_CAP 0x3Fu               /* cmp byte [0x4435], 0x3F ; jb */

typedef struct {
    uint8_t b4435;                          /* DS byte [0x4435] (read only here) */
    uint16_t w5d92, w5d94;                  /* the active-character far pointer (read only here) */
    pr_far16 stack_a, stack_b, stack_c;     /* the caller's locals [bp-0x10], [bp-0x142], [bp-0x14E] before the call (host residue) */
    pr_u25_e5_env *wrapper;                 /* the entry-5 wrapper's environment (hooks and object table) */
} pr_u1c_lmon_env;

typedef enum {
    PR_U1C_LMON_PREFIX_DONE = 0,    /* the wrapper returned OK; the model stops at +0x04AF (the rest of the routine is not modelled) */
    PR_U1C_LMON_ARGUMENT,           /* null state/env/wrapper env */
    PR_U1C_LMON_CAP_RETURN,         /* [0x4435] >= 0x3F: the routine returns at +0x0813 (a complete path of the original) */
    PR_U1C_LMON_CORE,               /* a lane-A core status ended the model (report->core_status); a HOOK from slot value(1) stops before the wrapper */
    PR_U1C_LMON_WRAPPER             /* the wrapper reported a non-OK status (report->wrapper_status/report); A, B, C hold what it left */
} pr_u1c_lmon_status;

typedef struct {
    uint8_t id;                     /* AL of slot value(1) (valid when read) */
    pr_far16 var_a, var_b, var_c;   /* the three far variables after the wrapper call (the host residue when it was not reached) */
    uint16_t saved_w5d92, saved_w5d94;
    unsigned wrapper_called;
    pr_u25_e5_status wrapper_status;
    pr_u25_e5_report wrapper_report;
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status;
} pr_u1c_lmon_report;

/* The prefix on lane A's loader state. `report` may be NULL. */
pr_u1c_lmon_status pr_u1c_op_load_monster_prefix(pr_rec_ecl_loader_state *state, pr_u1c_lmon_env *env,
                                                 pr_u1c_lmon_report *report);

#endif
