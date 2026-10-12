#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_ENTRY1_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_ENTRY1_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x7d public entry 1 (+0x03D5,
 * file 0x241A1, run-time stub 0x8C:0x0025; exactly 165 bytes +0x03D5..+0x0479,
 * 67 instructions, `retf 4`, no trailing data: the next prologue is +0x047A)
 * together with its near callee +0x02F2 (177 bytes, 7-word carry
 * normalizer). Evidence: docs/re/U7D_ENTRY1.md.
 *
 * Two pushed words, each read as a byte: [bp+8] = n (count), [bp+6] = idx
 * (word index), `retf 4`. What it does:
 *   w[0..6] = the seven words at window 0 +0x018C..+0x0199     (+0x03DB..+0x0407)
 *   repeat n times (n = 0: none; a Pascal `for k := 1 to n`):
 *       w[idx] += 1; carry-normalize(w)                          (+0x0409..+0x0438)
 *   window 0 +0x018C.. = w[0..6]                                  (+0x043A..+0x0466)
 *   near call +0x0000 with (n, idx) pushed as words whose high byte is the
 *   stale AH 0x93 left by the write-back loop                     (+0x0468..+0x0471)
 * The carry normalizer (+0x02F2): for i = 0..6, if w[i] >= limit[i] (unsigned
 * word compare against the DS word table at 0x363A): i < 6 -> w[i+1] += 1,
 * w[i] -= limit[i]; i == 6 -> walk the linked list at DS far pointer
 * [0x5D96] (next pointer = far word pair at node+0x104) adding 1 to the
 * word at node+0x30, with w[6] left unreduced. Wrap is 16-bit.
 *
 * Classification: bounded path-complete model of the 165 bytes plus the
 * 177-byte normalizer. NOT strict full C: the near callee +0x0000 (754
 * bytes, a runtime fill and unit 0x9e entry 2) is a host hook, the limit
 * table is host state (its live bytes are not established), the window-0
 * and list host representations replace far pointers, and idx > 6 (which
 * the original lets write past its 14-byte local array) is rejected.
 * No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#define PR_U7D_WORDS 7u                     /* the loop bound `cmp byte [bp-0xF], 6` */
#define PR_U7D_WINDOW0_OFFSET 0x18Cu        /* (0x49C6 + i) * 2 + 0x6E00 mod 0x10000, far offset taken as 0 */
#define PR_U7D_WINDOW0_MIN_SIZE (PR_U7D_WINDOW0_OFFSET + 2u * PR_U7D_WORDS)
#define PR_U7D_LIMIT_TABLE_DS 0x363Au       /* word table read at [di + 0x363A] */
#define PR_U7D_NODE_COUNTER_OFFSET 0x30u    /* inc word es:[di + 0x30] */
#define PR_U7D_NODE_NEXT_OFFSET 0x104u      /* far next pointer at es:[di + 0x104] */
#define PR_U7D_NODE_MIN_SIZE (PR_U7D_NODE_COUNTER_OFFSET + 2u)
#define PR_U7D_PUSHED_HIGH_BYTE 0x93u       /* stale AH when +0x0000 is called */
#define PR_U7D_LIST_CAP 65536u              /* host guard: the original has no cycle check */

/* One list node: `record` stands for the far pointer's block (bytes at
 * +0x30 are written), `next` for the far pointer stored at +0x104. */
typedef struct pr_u7d_node {
    uint8_t *record;
    size_t record_size;
    struct pr_u7d_node *next;
} pr_u7d_node;

/* The near call +0x0000 (n, idx), made after window 0 is written back. */
typedef void (*pr_u7d_update_hook)(void *ctx, uint16_t n_word, uint16_t idx_word);

typedef struct {
    uint8_t *window0;                 /* host memory for far [0x49D2]; words +0x18C..+0x199 are read and written */
    size_t window0_size;
    uint16_t limit[PR_U7D_WORDS];     /* host words from DS:0x363A */
    pr_u7d_node *list_head;           /* far pointer [0x5D96]; NULL = the original's null test */
    pr_u7d_update_hook update;        /* may be NULL */
    void *ctx;
} pr_u7d_state;

typedef enum {
    PR_U7D_OK = 0,
    PR_U7D_ARGUMENT   /* null state/window0, window0 shorter than 0x19A, idx > 6, or a list node with a
                         null record shorter than 0x32 bytes or a chain longer than the cap. Checked
                         before any effect: nothing written, hook not run. */
} pr_u7d_status;

typedef struct {
    unsigned iterations;                          /* n */
    unsigned carries[PR_U7D_WORDS];               /* i < 6: w[i+1] += 1, w[i] -= limit */
    unsigned list_walks;                          /* times w[6] >= limit[6] */
    unsigned nodes_bumped;                        /* total node counters incremented */
    uint16_t update_n_word, update_idx_word;      /* 0x93nn, 0x93mm */
    uint16_t words_out[PR_U7D_WORDS];             /* what was written back */
} pr_u7d_report;

/* The near callee +0x02F2 on a host array. `limit` = the 7-word DS table,
 * `list_head` as in the state. `walks`/`bumped` and `carries` (7 entries)
 * may be NULL. Returns PR_U7D_ARGUMENT only for a null array/limit or a
 * list that fails the guards above (nothing written). */
pr_u7d_status pr_u7d_carry_normalize(uint16_t w[PR_U7D_WORDS], const uint16_t limit[PR_U7D_WORDS],
                                     pr_u7d_node *list_head, unsigned *carries, unsigned *walks,
                                     unsigned *bumped);

/* The entry. `n` = [bp+8], `idx` = [bp+6]. `report` may be NULL. */
pr_u7d_status pr_u7d_entry1(pr_u7d_state *state, uint8_t n, uint8_t idx, pr_u7d_report *report);

#endif
