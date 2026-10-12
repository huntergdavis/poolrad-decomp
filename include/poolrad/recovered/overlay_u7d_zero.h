#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_ZERO_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_ZERO_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x7d internal routine +0x0000
 * (file 0x23DCC, exactly 754 bytes +0x0000..+0x02F1, 248 instructions,
 * `retf 4`, no trailing data: the next prologue is +0x02F2). It is the
 * near callee merged entry 1 (overlay_u7d_entry1.h) calls last, with (n,
 * idx) as words. Evidence: docs/re/U7D_ZERO_CALLEE.md.
 *
 * What it does, in order:
 *   [0x49F3] != 2:  FillChar(DS:0x471E, 0x48, 1)                       (+0x0006..+0x001E)
 *   [0x49F3] == 2:  scan the bytes DS:0x471D + i for a nonzero one,
 *                   i = 1 .. window-1 word 0x67C (at least i = 1); if none
 *                   is set, return with nothing else done              (+0x0020..+0x0059)
 *   remaining = n; for b := idx downto 2: remaining *= limit[b-1]       (+0x005C..+0x008C)
 *                   (16-bit; word table at DS:0x363A; skipped for idx <= 1 as a signed byte)
 *   while remaining != 0: chunk = min(remaining, 10); one pass over the
 *     member list at DS far pointer [0x5D96] (next = far word pair at
 *     +0x104; i counts members from 1); for each member whose flag
 *     DS:0x471D + i is set: clear it and walk the member's item list
 *     (head = far pair at +0x7F; item = byte type at +0, word quantity at
 *     +1, far next at +5):
 *       quantity == 0: skip;  chunk < quantity: quantity -= chunk, re-flag the member;
 *       otherwise: call unit 0x9e entry 2 (stub 0xB0:0x2A) with (member, type byte,
 *       item) -- a host hook here -- then continue at the member's head if the
 *       predecessor snapshot equals the head, else at the saved next item.
 *     The walk stops once the snapshot tail has been visited; any items left
 *     are then scanned only to re-flag the member when a quantity is nonzero.
 *     remaining -= min(remaining, 10)                                  (+0x008E..+0x02E9)
 * Classification: bounded path-complete model of the 754 bytes. NOT strict
 * full C: the unit 0x9e entry 2 call is a host hook, the DS tables are host
 * state, far pointers are host structs, and two loops the original can run
 * without end are reported instead. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#define PR_U7D0_FLAG_COUNT 256u               /* DS:0x471D + i, i a byte */
#define PR_U7D0_FILL_FIRST 1u                 /* +0x000D: mov di, 0x471E */
#define PR_U7D0_FILL_COUNT 0x48u              /* +0x0012: mov ax, 0x48 */
#define PR_U7D0_MODE_SCAN 2u                  /* +0x0006: cmp byte [0x49F3], 2 */
#define PR_U7D0_WINDOW1_SCAN_WORD 0x67Cu      /* +0x004C: es:[di + 0x67C] */
#define PR_U7D0_WINDOW1_MIN_SIZE 0x67Eu
#define PR_U7D0_CHUNK_MAX 10u                 /* +0x0097 / +0x00A3 */
#define PR_U7D0_STEP_CAP 4000000u             /* host guard for the item loop */

typedef struct pr_u7d0_item {
    uint8_t type;                 /* byte at +0 */
    uint16_t quantity;            /* word at +1 */
    struct pr_u7d0_item *next;    /* far pointer at +5 (NULL = both words zero) */
    uint8_t flag4;                /* byte at +4: unit 0x9e entry 2 calls its handler when nonzero
                                     (not read by overlay_u7d_zero.c) */
} pr_u7d0_item;

typedef struct pr_u7d0_member {
    pr_u7d0_item *head;           /* far pointer at +0x7F */
    struct pr_u7d0_member *next;  /* far pointer at +0x104 */
} pr_u7d0_member;

/* lcall 0xB0:0x2A (unit 0x9e entry 2, retf 0xA): pushes (member far
 * pointer, type byte as a word with stale AH, item far pointer). The hook is
 * expected to unlink `item` from `member->head`'s chain; the model re-reads
 * the head and the chain afterwards, as the original does. */
typedef void (*pr_u7d0_remove_hook)(void *ctx, pr_u7d0_member *member, uint8_t type, pr_u7d0_item *item);

typedef struct {
    uint8_t mode;                      /* byte [0x49F3] */
    uint8_t flags[PR_U7D0_FLAG_COUNT]; /* DS:0x471D .. 0x481C; index 0 is read only by the byte wrap */
    const uint8_t *window1;            /* host memory for far [0x49D6]; word 0x67C read when mode == 2 */
    size_t window1_size;
    const uint16_t *limit;             /* host words from DS:0x363A */
    size_t limit_count;
    pr_u7d0_member *list_head;         /* far pointer [0x5D96] */
    pr_u7d0_remove_hook remove;        /* may be NULL (then nothing is unlinked) */
    void *ctx;
} pr_u7d0_state;

typedef enum {
    PR_U7D0_OK = 0,
    PR_U7D0_ARGUMENT,        /* null state; mode 2 with a window1 shorter than 0x67E; or a limit table
                                shorter than idx when idx is 2..127 and the multiply is reached. Nothing
                                written, hook not run. */
    PR_U7D0_NONTERMINATING,  /* mode 2, no flag set and window-1 word 0x67C >= 255: the byte counter wraps
                                and the original scans forever. Nothing written. */
    PR_U7D0_STEP_LIMIT       /* the item loop ran PR_U7D0_STEP_CAP steps (e.g. a hook that never unlinks);
                                state is partially updated */
} pr_u7d0_status;

typedef struct {
    unsigned filled;             /* mode != 2: the 72 flags were set */
    unsigned early_return;       /* mode 2 with no flag: nothing else done */
    uint16_t remaining_start;    /* n after the unit conversion */
    unsigned passes;             /* chunk passes over the member list */
    unsigned hook_calls;
    unsigned steps;              /* item-loop iterations */
} pr_u7d0_report;

/* The routine. `n` = [bp+8], `idx` = [bp+6], raw bytes (the original zero-
 * extends n and compares idx as a signed byte). `report` may be NULL. */
pr_u7d0_status pr_u7d0_run(pr_u7d0_state *state, uint8_t n, uint8_t idx, pr_u7d0_report *report);

#endif
