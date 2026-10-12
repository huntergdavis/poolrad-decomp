#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_MONSTER_TAIL_0537_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_MONSTER_TAIL_0537_H

/* Bounded model of unit 0x1c +0x0537..+0x0816 of ECL opcode 0x0B (LOAD MONSTER): the party-list tail link, the per-copy record
 * clones and the two sidecar chain copies, through the return. File 0x127D..0x155C (unit base 0x0D46), 736 bytes, 222 instructions,
 * SHA-256 a31517ef9c75925d057e7140d269e7f68700c4c57672e7b410f6466b09149f39. It continues after the 0x3E9:0x34 call of
 * overlay_u1c_load_monster_04af.h. Evidence: docs/re/U1C_LOAD_MONSTER_TAIL_0537.md.
 *
 * In the original's order (n = byte [bp-4], initialised to 1 in the prefix; count = byte [bp-2]):
 *   cur := far [0x5D96]:[0x5D98]; while cur.link(+0x104/+0x106) <> nil do cur := cur.link          (+0x0537..+0x0559)
 *   cur.link := A; cur := A; A[+0xBF] := [0x6F83]; A.link := nil; [0x4435]++; n++                   (+0x055B..+0x059F)
 *   loop (+0x05A2): n > count (unsigned) or [0x4435] >= 0x3F -> finish
 *     P := GetMem(0x11D); cur.link := P; cur := P; Move(local record copy, P, 0x11D); P[+0xBF] := [0x6F83];
 *     P.link := nil; P[+0x7F..+0x82] := 0; P[+0xC8..+0xCB] := 0; n++; [0x4435]++                     (+0x05B7..+0x063A)
 *     for each node I of the B chain (link +0x2A/+0x2C), starting at the saved head B: M := GetMem(0x3F); cur[+0xC8] := M
 *       (the previous +0xC8 word pair is kept in M's link when it was non-nil, else M's link := nil); Move(I, M, 0x3F)  (+0x063E..+0x070D)
 *     for each node I of the C chain (link +5/+7) from the saved head C: the same with GetMem(9), the chain word pair
 *       at cur[+0x7F], the node link at +5/+7, Move(I, M, 9)                                           (+0x0710..+0x07EA)
 *   finish: [0x6F83]++; [0x84E7] := 1; [0x5D92]/[0x5D94] := the saved active-character pointer          (+0x07FD..+0x0816)
 * The copies are prepended to each chain, so the chain order is the reverse of the source.
 *
 * Boundaries: GetMem (0xAF8:0x023F, heap allocator 0xAF8:0x0308 not decoded) is an explicit hook; every far object is read through an
 * explicit host object table (an unmapped pointer, including nil, is UNMAPPED; a view too short for an access is VIEW). Cycles
 * in any walked list are guarded. Nothing about object contents, the party, or what the engine does after the opcode is invented.
 *
 * Classification: bounded model of a slice. Not a strict translation. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u25_entry5_0411.h"

#define PR_U1C_LMT_RECORD 0x11Du
#define PR_U1C_LMT_CAP 0x3Fu
#define PR_U1C_LMT_WALK_MAX 4096u

typedef struct pr_u1c_lmt_env pr_u1c_lmt_env;

typedef struct {
    /* 0xAF8:0x023F GetMem(size): *result is the far pointer (DX:AX). Nonzero return stops with PR_U1C_LMT_CALLBACK. */
    int (*getmem_af8_023f)(void *ctx, const pr_u1c_lmt_env *env, uint16_t size, pr_far16 *result);
} pr_u1c_lmt_hooks;

struct pr_u1c_lmt_env {
    pr_u25_e5_object *objects;        /* the host's object table: every far pointer the routine dereferences must be in it */
    size_t object_count;
    pr_far16 party_head;              /* far [0x5D96]:[0x5D98] (offset, segment): the party list head (read only) */
    pr_u1c_lmt_hooks hooks;
    void *ctx;
};

/* The DS bytes and words the slice reads and writes. */
typedef struct {
    uint8_t b4435;                    /* inc per monster; the cap test reads it */
    uint8_t b6f83;                    /* read for the +0xBF byte; inc at the end */
    uint8_t b84e7;                    /* := 1 at the end */
    uint16_t w5d92, w5d94;            /* := the saved pointer at the end */
} pr_u1c_lmt_ds;

/* The values the earlier slices leave in the frame. */
typedef struct {
    pr_far16 var_a;                   /* [bp-0x10]: the object the entry-5 wrapper filled */
    pr_far16 head_b, head_c;          /* the saved list heads ([bp-0x14A], [bp-0x156]) */
    uint8_t count;                    /* [bp-2] */
    uint8_t n;                        /* [bp-4], 1 after the prefix */
    uint8_t record_copy[PR_U1C_LMT_RECORD];   /* the local 0x11D-byte copy of A taken at +0x04AF */
    uint16_t saved_w5d92, saved_w5d94;        /* [bp-0x14], [bp-0x12] */
} pr_u1c_lmt_in;

typedef enum {
    PR_U1C_LMT_OK = 0,        /* returned (+0x0816) */
    PR_U1C_LMT_ARGUMENT,      /* null argument or object table inconsistency: rejected before any effect */
    PR_U1C_LMT_UNMAPPED,      /* a dereferenced far pointer is not in the table (report->bad_ptr) */
    PR_U1C_LMT_VIEW,          /* an access runs past the object's view (report->bad_ptr) */
    PR_U1C_LMT_CYCLE,         /* a walked list exceeded PR_U1C_LMT_WALK_MAX nodes */
    PR_U1C_LMT_HOOK,          /* the GetMem hook is NULL when reached */
    PR_U1C_LMT_CALLBACK       /* the GetMem hook returned nonzero (report->callback_status) */
} pr_u1c_lmt_status;

typedef struct {
    unsigned walked;          /* party nodes stepped over to reach the tail */
    pr_far16 tail;            /* the party tail before A was linked */
    unsigned monsters;        /* A plus the copies made (the +0x4435 increments) */
    unsigned copies;          /* GetMem(0x11D) records made */
    unsigned b_nodes, c_nodes;    /* sidecar nodes copied (0x3F-byte and 9-byte) */
    unsigned getmem_calls;
    pr_far16 bad_ptr;
    int callback_status;
    unsigned cap_stop;        /* finished because [0x4435] >= 0x3F rather than n > count */
} pr_u1c_lmt_report;

/* Applies the slice to `ds` and the host objects. `report` may be NULL. On any non-OK status the effects made before the stop stand. */
pr_u1c_lmt_status pr_u1c_load_monster_tail(pr_u1c_lmt_ds *ds, const pr_u1c_lmt_env *env, const pr_u1c_lmt_in *in, pr_u1c_lmt_report *report);

#endif
