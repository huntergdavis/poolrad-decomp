#ifndef POOLRAD_RECOVERED_OVERLAY_U3CE_ENTRY4_01C8_H
#define POOLRAD_RECOVERED_OVERLAY_U3CE_ENTRY4_01C8_H

/* Bounded model of unit 0x3ce entry 4 (+0x01C8, file 0x31C34, run-time stub 0x3E9:0x34), the callee of LOAD MONSTER +0x0532.
 * 820 bytes (+0x01C8..+0x04FB), 353 instructions, SHA-256 3046c9df720e9e30e4db427198b37ced3932f8a6cbdca6e96a59f11e6568095d, `retf 8`.
 * Arguments: far Pascal string [bp+0xA], byte [bp+8] (p3), byte [bp+6] (the slot index, the caller's [0x6F83]). Evidence:
 * docs/re/U3CE_ENTRY4_01C8.md.
 *
 * The routine is name driven. s := StrStore(arg, 255); with the four unit constants (cs:0x1B0, 0x1B6 of length 5, 0x1BC of length 6,
 * 0x1C3 of length 4; their text stays out of the repository):
 *   A: Copy(s,1,5) equals cs:0x1B0 or cs:0x1B6: if UpCase(last char of s) = 'T' then p3 += 0x40 (byte wrap); s := s without its last char;
 *      suffix := ''; load(s + suffix, p3, 1, 0, slot.first); load(s + suffix, p3 + 0x80, 1, 0, slot.second)
 *   B: else s equals cs:0x1BC or cs:0x1C3: suffix := ''; the same two loads; if s equals cs:0x1C3, then palette(...) on each loaded slot word (both loads first)
 *   C: else suffix := Str([0x5376], max length 1) (0xAF8:0x1366, unresolved: an explicit hook); load, palette, load, palette (interleaved, table 0xCE2 instead of 0xCF2)
 * and every path ends with 0x899:0x0104 (a key-flush loop: while keypressed do read key).
 * The loader 0x464:0x111 (releases the previous slot object, composes a file name, calls the unresolved block loader 0x802:0x716) and
 * the palette routine 0x464:0x1E5C are explicit fallible hooks, so are 0xAF8:0x1366 and 0x899:0x0104. The string helpers Copy (0xAF8:0x0B57,
 * decoded), UpCase (0x182A, decoded), StrStore/StrLoad/StrCat (0x0B25/0x0B0B/0x0B98, from earlier notes) are implemented here.
 *
 * Classification: bounded model of the whole routine with explicit hooks at every effect. Not a strict translation. No game-function credit. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u25_entry5_0411.h"

#define PR_U3CE_E4_STR_MAX 255u
#define PR_U3CE_E4_MAX_CALLS 6u

typedef struct { pr_far16 first, second; } pr_u3ce_slot;    /* the 8-byte DS entry at 0x5E5A + 8*index: words at +0..+3 and +4..+7 */

typedef struct pr_u3ce_e4_env pr_u3ce_e4_env;

typedef struct {
    /* 0x464:0x0111(far name, p3 byte, 1, 0, far dest slot word pair). `which` 0 = DS 0x5E5A+8*i, 1 = 0x5E5E+8*i. Nonzero return stops. */
    int (*load_464_0111)(void *ctx, const pr_u3ce_e4_env *env, const uint8_t *pascal_name, uint8_t p3, unsigned which, pr_far16 *dest);
    /* 0x464:0x1E5C(ds:0xCD2, ds:table, 0, 0, far object read from the slot). `table_offset` 0xCF2 or 0xCE2. Nonzero return stops. */
    int (*palette_464_1e5c)(void *ctx, const pr_u3ce_e4_env *env, uint16_t table_offset, pr_far16 object);
    /* 0xAF8:0x1366: Str(value, 0, dest, 1). Writes a Pascal string of length 0 or 1 into out[0..1]. Nonzero return stops. */
    int (*str_af8_1366)(void *ctx, const pr_u3ce_e4_env *env, uint32_t value, uint8_t out[2]);
    /* 0x899:0x0104: the trailing key-flush loop. Nonzero return stops. */
    int (*flush_899_0104)(void *ctx, const pr_u3ce_e4_env *env);
} pr_u3ce_e4_hooks;

struct pr_u3ce_e4_env {
    const uint8_t *c1b0, *c1b6, *c1bc, *c1c3;   /* the four unit constants (Pascal strings) */
    uint8_t b5376;                               /* DS byte [0x5376] (read only) */
    pr_u3ce_slot *slots;                         /* the DS table at 0x5E5A, 256 entries; the load hook fills the destinations */
    pr_u3ce_e4_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U3CE_E4_OK = 0,
    PR_U3CE_E4_ARGUMENT,   /* null env/name/constant/table: rejected before any effect */
    PR_U3CE_E4_HOOK,       /* a reached hook is NULL (report->failed_call); earlier effects stand */
    PR_U3CE_E4_CALLBACK    /* a reached hook returned nonzero (report->callback_status); earlier effects stand */
} pr_u3ce_e4_status;

typedef enum { PR_U3CE_E4_PATH_NONE = 0, PR_U3CE_E4_PATH_A, PR_U3CE_E4_PATH_B6, PR_U3CE_E4_PATH_B4, PR_U3CE_E4_PATH_C } pr_u3ce_e4_path;
typedef enum { PR_U3CE_E4_CALL_LOAD = 1, PR_U3CE_E4_CALL_PALETTE, PR_U3CE_E4_CALL_STR, PR_U3CE_E4_CALL_FLUSH } pr_u3ce_e4_call;

typedef struct {
    pr_u3ce_e4_path path;
    uint8_t s[1 + PR_U3CE_E4_STR_MAX];     /* the working string after the path's edit (Pascal) */
    uint8_t name[1 + PR_U3CE_E4_STR_MAX];  /* the composed name passed to the loader (s + suffix) */
    uint8_t p3_used;                       /* p3 after the optional + 0x40 */
    unsigned t_suffix;                     /* path A: the last character upcased to 'T' */
    unsigned n_calls;
    pr_u3ce_e4_call calls[PR_U3CE_E4_MAX_CALLS];
    pr_u3ce_e4_call failed_call;
    int callback_status;
} pr_u3ce_e4_report;

/* `name` is the far Pascal string, `p3` the byte at [bp+8], `slot` the byte at [bp+6]. `report` may be NULL. */
pr_u3ce_e4_status pr_u3ce_entry4(const pr_u3ce_e4_env *env, const uint8_t *name, uint8_t p3, uint8_t slot, pr_u3ce_e4_report *report);

#endif
