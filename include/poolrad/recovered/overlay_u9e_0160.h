#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_0160_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_0160_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x9e internal routine
 * +0x0160 (file 0x2B04A, exactly 410 bytes +0x0160..+0x02F9, 140
 * instructions, `retf 6`, no trailing data: the next prologue is +0x02FA).
 * The 32-byte constant at +0x0140..+0x015F that it uses is data of THIS
 * routine (pushed as cs:0x0140 at +0x01A0), stored before it. Evidence:
 * docs/re/U9E_0160.md.
 *
 * Pascal frame, arguments pushed left to right: character far pointer B
 * ([bp+0xA]:[bp+8]) and a type byte ([bp+6]); `retf 6`, no result. It asks
 * whether the sidecar list (head far pointer +0x7F, node = type byte at +0,
 * link at +5) of B, or in some cases of another party member, holds a node
 * with byte +0 equal to the type byte, and if so calls the handler for the
 * type with mode 0:
 *   found := 0; cell := lookup(B, type)  (resident 0xBA:0x2199)         (+0x0167..+0x0199)
 *   found -> skip to the call
 *   type not in the 256-bit set at cs:0x0140 (0xAF8:0x0DC0) -> no call   (+0x019C..+0x01AC)
 *   save DS:0x671B..0x67F2 (216 bytes) to the frame (0xAF8:0x1685)       (+0x01AF..+0x01BE)
 *   for each party member `cur` from far pointer [0x5D96], next at +0x104,
 *   until found:                                                         (+0x01C3..+0x02BD)
 *     cell := lookup(cur, type); not found -> next member
 *     [0x49F3] != 5 -> found := 1
 *     [0x49F3] == 5 -> a = unit 0x3c6 entry 12(cur), b = entry 13(cur),
 *       mode = (type == 0x31 ? 6 : 1), c = entry 14(cur); unit 0x3c2 entry 6
 *       (a, b, mode, 0xFF, c, far pointer [0x6716]:[0x6718]) fills the DS
 *       array; for k = 1 .. [0x671A]: if byte DS:0x6718 + 3k == entry 15(B)
 *       then found := 1
 *   restore DS:0x671B..0x67F2 from the frame                            (+0x02C0..+0x02CF)
 *   found -> near call +0x0000 (type, B, cell, 0) = unit 0x9e entry 1
 *            (overlay_u9e_entry2.h's pr_u9e_dispatch), mode byte 0       (+0x02D4..+0x02F1)
 *
 * Classification: bounded model of the 410 bytes. The party-list and
 * sidecar-list far pointers are host structs; unit 0x3c2 entry 6 (720
 * bytes, stub 0x3DB:0x3E), the handlers behind pr_u9e_dispatch and
 * (optionally) the resident lookup are host boundaries. Unit 0x3c6 entries
 * 12..15 are the translated overlay_u3c6_slot.c. Not claimed as strict
 * full C; no game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u3c6_slot.h"
#include "poolrad/recovered/overlay_u9e_entry2.h"

#define PR_U9E0160_SET_BYTES 32u              /* the constant at +0x0140: members 0x15, 0x2D, 0x2E, 0x31 */
#define PR_U9E0160_SAVE_BYTES 0xD8u           /* +0x01BA: mov ax, 0xD8 */
#define PR_U9E0160_SCRATCH_DS 0x6716u         /* far pointer words [0x6716] (offset), [0x6718] (segment) */
#define PR_U9E0160_COUNT_DS 0x671Au           /* byte [0x671A], the record count */
#define PR_U9E0160_RECORDS_DS 0x6718u         /* record k byte 0 is at 0x6718 + 3k, k = 1.. */
#define PR_U9E0160_SAVE_DS 0x671Bu            /* the saved/restored span starts here */
#define PR_U9E0160_SCRATCH_SIZE 0x306u        /* 0x6716 .. 0x6718 + 3 * 255 + 1 */
#define PR_U9E0160_MODE_COMBAT 5u             /* +0x01F8: cmp byte [0x49F3], 5 */
#define PR_U9E0160_SPECIAL_TYPE 0x31u         /* +0x0202: cmp byte [bp+6], 0x31 */
#define PR_U9E0160_STEP_CAP 65536u            /* host guard for a cyclic party or sidecar list */

/* The resident sidecar lookup 0xBA:0x2199 (translated as pr_rec_res_00ba_2199, whose semantics the
 * default below mirrors): searches `character`'s sidecar list for the first node whose byte +0 equals
 * `key`. Returns AL (1 = found); writes the node, or NULL when absent, to *cell. */
typedef uint8_t (*pr_u9e0160_lookup)(void *ctx, pr_u7d0_member *character, uint8_t key, pr_u7d0_item **cell);

/* unit 0x3c2 entry 6 (stub 0x3DB:0x3E, retf 0xE): seven words in push order, then the DS scratch
 * image (indexed from DS:0x6716) it fills: count at [4], records at 2 + 3k. */
typedef void (*pr_u9e0160_scan)(void *ctx, uint16_t a, uint16_t b, uint16_t range_mode, uint16_t ff,
                                uint16_t c, uint16_t far_off, uint16_t far_seg, uint8_t *scratch);

typedef struct {
    uint8_t mode;                             /* byte [0x49F3] */
    pr_u7d0_member *party_head;               /* far pointer [0x5D96] (next at +0x104) */
    uint8_t scratch[PR_U9E0160_SCRATCH_SIZE]; /* DS:0x6716 .. 0x6A1B */
    pr_u3c6_state slots;                      /* unit 0x3c6 tables for entries 12..15 */
    pr_u9e_state *handlers;                   /* the DS:0x6828 table for the final dispatch (may be NULL) */
    pr_u9e0160_lookup lookup;                 /* NULL = the built-in list search */
    pr_u9e0160_scan scan;                     /* may be NULL (the array is then left as is) */
    void *ctx;
} pr_u9e0160_state;

typedef enum {
    PR_U9E0160_OK = 0,
    PR_U9E0160_ARGUMENT,    /* null state or character */
    PR_U9E0160_STEP_LIMIT   /* more than PR_U9E0160_STEP_CAP list steps; state partly updated (the DS
                               span is restored first) */
} pr_u9e0160_status;

typedef enum {
    PR_U9E0160_CALL_LOOKUP = 1,   /* resident 0xBA:0x2199 */
    PR_U9E0160_CALL_SET_TEST,     /* 0xAF8:0x0DC0 */
    PR_U9E0160_CALL_SAVE,         /* 0xAF8:0x1685 into the frame */
    PR_U9E0160_CALL_SCAN,         /* 0x3DB:0x3E */
    PR_U9E0160_CALL_RESTORE,      /* 0xAF8:0x1685 back */
    PR_U9E0160_CALL_DISPATCH      /* near +0x0000 */
} pr_u9e0160_call;

typedef struct {
    unsigned found_on_character;  /* the first lookup found */
    unsigned in_set;              /* the type is in the 32-byte set */
    unsigned found;               /* the byte [bp-0xE1] at +0x02D4 */
    unsigned members_visited;
    unsigned scans;
    pr_u7d0_item *cell;           /* the node at the dispatch (valid when found) */
    unsigned calls_n;
    pr_u9e0160_call calls[160];   /* far/near calls in order (capped) */
} pr_u9e0160_report;

/* 0xAF8:0x0DC0 against the 32-byte set at cs:0x0140 (ZF clear = member). */
int pr_u9e0160_type_in_set(uint8_t type);

/* The routine. `character` = B, `type` = [bp+6]. `report` may be NULL. */
pr_u9e0160_status pr_u9e0160_run(pr_u9e0160_state *state, pr_u7d0_member *character, uint8_t type,
                                 pr_u9e0160_report *report);

#endif
