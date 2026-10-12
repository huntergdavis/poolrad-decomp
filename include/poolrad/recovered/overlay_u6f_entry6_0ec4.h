#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_ENTRY6_0EC4_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_ENTRY6_0EC4_H

/* Bounded model of unit 0x6f entry 6 (+0x0EC4, file 0x1EEAA, run-time stub 0x7C:0x3E), the monster-record initializer the unit 0x25
 * entry-5 wrapper calls (LOAD MONSTER). 926 bytes (+0x0EC4..+0x1261), 341 instructions, SHA-256
 * 1160665fef6397a88b922ce115836e5cf4b55f09b5c4db04e765e8a02f08f747, `retf 6`: far object [bp+6], id word [bp+0xA] (only AL is read).
 * Evidence: docs/re/U6F_ENTRY6_0EC4_MAP.md (the earlier static map) and docs/re/U6F_ENTRY6_0EC4.md (this model).
 *
 * The routine loads three blocks through the resident block reader 0x802:0x0716 (a bounded model of it exists: resident_ecl_dax_block.h)
 * with the same id and three file names built from five unit constants and the digit string of DS byte [0x5376], then:
 *   block 1: name = c0E98 + Str([0x5376]) + c0E9C, ext = c0EA0; size 0 -> the error path (display 0x709:0x0BF5, then 0x899:0 = terminate): the
 *     model returns ERROR_EXIT before the display call. Otherwise Move(block, object, 0x11D) (0xAF8:0x1685, an overlap-safe move);
 *     object[+0xC8/+0xCA], [+0x7F/+0x81], [+0x104/+0x106], [+0x108/+0x10A] := 0; the class byte [+0xC0] := 1 when the loaded byte at +0x2E is 1, 3 or 5,
 *     := 2 when it is 2, 4, 6 or 7, else it keeps the loaded byte; five rolls 0xB0:0x48(1, 6) store ((r + 8) << 4) + r (low byte) at
 *     [+0xC0 + i] for i = 1..3, then [+0xC4] := 0xC4, then i = 5..6; FreeMem(block, size); [0x557C] := 1
 *   block 2 (name = c0E98 + digit + c0EBC): size 0 skips; else 9-byte units chained from object[+0x7F/+0x81] (GetMem(9), Move 9, link at +5/+7,
 *     do-while off < size), FreeMem(block, size)
 *   block 3 (c0EC0): size 0 skips; else 0x3F-byte units chained from object[+0xC8/+0xCA] (link +0x2A/+0x2C); FreeMem
 *   [0x557C] := 0; 0x899:0x0104 (key flush); return
 *
 * Boundaries (explicit hooks): the block reader, GetMem/FreeMem, the roll, 0xAF8:0x1366 (numeric to string), 0x899:0x0104. The five code
 * constants are host data (Pascal strings; their text stays out of the repository). Objects are reached through an explicit host table.
 * Classification: bounded model of the routine's whole path with hooks. Not a strict translation. No game-function credit. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u25_entry5_0411.h"

#define PR_U6F_E6_RECORD 0x11Du
#define PR_U6F_E6_UNIT9 9u
#define PR_U6F_E6_UNIT3F 0x3Fu
#define PR_U6F_E6_STR_MAX 255u

typedef struct pr_u6f_e6_env pr_u6f_e6_env;

typedef struct {
    /* 0xAF8:0x1366 Str(value, 0, dest, 1): writes a Pascal string of length 0 or 1 into out[0..1]. */
    int (*str_af8_1366)(void *ctx, const pr_u6f_e6_env *env, uint32_t value, uint8_t out[2]);
    /* 0x802:0x0716(name, ext, id, &size, &ptr): *size = unpacked size (0 = missing), *ptr = the far block (an object in the table). */
    int (*block_802_0716)(void *ctx, const pr_u6f_e6_env *env, const uint8_t *name, const uint8_t *ext, uint8_t id, uint16_t *size, pr_far16 *ptr);
    /* 0xAF8:0x023F GetMem(size) (sizes 9 and 0x3F here). */
    int (*getmem_af8_023f)(void *ctx, const pr_u6f_e6_env *env, uint16_t size, pr_far16 *result);
    /* 0xAF8:0x0254 FreeMem(far block, size). */
    int (*free_af8_0254)(void *ctx, const pr_u6f_e6_env *env, pr_far16 ptr, uint16_t size);
    /* 0xB0:0x0048 (unit 0x9e entry 8): called with (count 1, range 6). */
    int (*roll_b0_0048)(void *ctx, const pr_u6f_e6_env *env, uint8_t count, uint8_t range, uint8_t *al);
    /* 0x899:0x0104: the trailing key-flush loop. */
    int (*flush_899_0104)(void *ctx, const pr_u6f_e6_env *env);
} pr_u6f_e6_hooks;

struct pr_u6f_e6_env {
    pr_u25_e5_object *objects;           /* the host object table: the record, the blocks and every GetMem result */
    size_t object_count;
    const uint8_t *c0e98, *c0e9c, *c0ea0, *c0ebc, *c0ec0;   /* unit constants (Pascal strings) */
    uint8_t b5376;                       /* DS byte [0x5376] (read only) */
    pr_u6f_e6_hooks hooks;
    void *ctx;
};

typedef struct { uint8_t b557c; } pr_u6f_e6_ds;     /* DS byte [0x557C]: := 1 after block 1, := 0 at the end */

typedef enum {
    PR_U6F_E6_OK = 0,
    PR_U6F_E6_ARGUMENT,     /* null state/env/constant/object table inconsistency: rejected before any effect */
    PR_U6F_E6_UNMAPPED,     /* a dereferenced far pointer is not in the table (report->bad_ptr) */
    PR_U6F_E6_VIEW,         /* an access runs past an object's view (report->bad_ptr) */
    PR_U6F_E6_HOOK,         /* a reached hook is NULL (report->failed_call) */
    PR_U6F_E6_CALLBACK,     /* a reached hook returned nonzero (report->callback_status, report->failed_call) */
    PR_U6F_E6_RANGE,        /* a unit loop's 16-bit offset would wrap before reaching the block size */
    PR_U6F_E6_ERROR_EXIT    /* block 1 had size 0: the original shows a message and terminates; stopped before the display call */
} pr_u6f_e6_status;

typedef enum {
    PR_U6F_E6_CALL_STR = 1, PR_U6F_E6_CALL_BLOCK, PR_U6F_E6_CALL_GETMEM, PR_U6F_E6_CALL_FREE, PR_U6F_E6_CALL_ROLL, PR_U6F_E6_CALL_FLUSH
} pr_u6f_e6_call;

typedef struct {
    uint16_t size[3];            /* the three block sizes (0 for a block not reached or missing) */
    unsigned blocks_loaded;      /* blocks with a non-zero size */
    unsigned units9, units3f;    /* chain units created */
    uint8_t rolls[5];            /* the five roll results in order */
    unsigned n_rolls;
    uint8_t class_byte;          /* object[+0xC0] after the class step (before the rolls overwrite the +0xC1.. bytes) */
    pr_u6f_e6_call failed_call;
    int callback_status;
    pr_far16 bad_ptr;
} pr_u6f_e6_report;

/* `object` = [bp+6], `id` = AL of [bp+0xA]. `report` may be NULL. On any non-OK status the effects made before the stop stand. */
pr_u6f_e6_status pr_u6f_entry6(pr_u6f_e6_ds *ds, const pr_u6f_e6_env *env, pr_far16 object, uint8_t id, pr_u6f_e6_report *report);

#endif
