#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_PIECES_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_PIECES_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x3bd public entry 6
 * (+0x0E08, file 0x2F6FF, run-time stub 0x3D5:0x003E), the routine that the
 * merged class-1 store hook (unit 0x25 +0x0A74, cases 0x322/0x324/0x326)
 * and the LOAD FILES / LOAD PIECES handler (unit 0x1c +0x0C74, PR #97's
 * `load_pieces(ctx, id, slot)` callback) call with two pushed words:
 * [bp+8] = id (first pushed), [bp+6] = slot (second pushed); `retf 4`.
 * Only the low byte of each word is ever read.
 *
 * Classification: path-complete local-body model with callback
 * boundaries, not a full translation (FUNCTION_LEDGER.md vocabulary). The
 * routine's own paths are reproduced: slot gate, block read through the
 * resident DAX loader, the notice path (which ends in resident
 * 0x899:0x0000, a DOS terminate, so it never returns), copy of 780-byte
 * pieces into a four-slot buffer, per-piece index remap, per-piece
 * announcement, and the slot table writes. The runtime string construction
 * of the file name, the resident block loader, the notice display, the
 * program exit, FreeMem and the unit 0x40c call are callbacks. Sixteen
 * far-call sites to thirteen distinct targets. Evidence:
 * docs/re/U3BD_PIECES_ENTRY6.md. Names are addresses; no game mechanics
 * are asserted. No game data is embedded. */

#include <stddef.h>
#include <stdint.h>

#define PR_U3BD_PIECE_SIZE 780u          /* 0x30C bytes per piece */
#define PR_U3BD_PIECE_ROWS 5u            /* remap walks 5 rows of 0x9C = 156 bytes */
#define PR_U3BD_PIECE_ROW 156u
#define PR_U3BD_SLOTS 4u                 /* pieces buffer at far [0x6A58]: slots 1..4 */
#define PR_U3BD_BUFFER_SIZE (PR_U3BD_SLOTS * PR_U3BD_PIECE_SIZE)
#define PR_U3BD_NAME_PREFIX_LEN 7u       /* code-segment literal at +0x0DB4, 7 characters */
#define PR_U3BD_NAME_SUFFIX_LEN 4u       /* code-segment literal at +0x0DBC, 4 characters */

typedef struct {
    uint16_t w6aae;                      /* word [0x6AAE + 4*slot] */
    uint16_t w6ab0;                      /* word [0x6AB0 + 4*slot] */
} pr_u3bd_slot_words;

typedef struct {
    uint8_t file_number;                 /* byte [0x5376], the decimal digit(s) appended to the 7-character name literal */
    uint16_t table_2722[4];              /* words [0x2722], [0x2724], [0x2726], [0x2728]; indexed by slot (1..3 used) */
    uint16_t threshold_272c;             /* word [0x272C] */
    uint8_t *pieces;                     /* host memory for far [0x6A58]: PR_U3BD_BUFFER_SIZE bytes, slot k at (k-1)*780 */
    pr_u3bd_slot_words slots[5];         /* [0x6AAE..0x6AC1]: index 0 is never written by this routine */
} pr_u3bd_state;

typedef enum {
    PR_U3BD_OK = 0,
    PR_U3BD_ARGUMENT,        /* null state, pieces buffer or callouts struct */
    PR_U3BD_CALLOUT,         /* a required callback was missing at the point it was reached; earlier effects done */
    PR_U3BD_EXITED,          /* the notice path ran and reached 0x899:0x0000, which terminates the program
                                (runtime 0xAF8:0x00E9 -> int 21h, AH = 0x4C): nothing after it executes */
    PR_U3BD_BLOCK_BOUNDS     /* a 780-byte copy would read past the block's capacity (the original reads
                                beyond `size` into its heap block when size is not a multiple of 780) */
} pr_u3bd_status;

/* Call-outs. Each is required at the point its site is reached. */
typedef struct {
    void *ctx;
    /* Resident 0x802:0x0716 with (name string = 7-char literal + decimal file number, 4-char
     * extension literal, block id, &size, &pointer). Return the block and its size; size 0
     * means the loader failed (the original then takes the terminal notice path).
     * `capacity` is the readable length of `block` (>= size); the original reads whole
     * 780-byte pieces regardless of `size`. */
    void (*read_block)(void *ctx, uint8_t file_number, uint8_t block_id,
                       const uint8_t **block, uint16_t *size, size_t *capacity);
    /* +0x0E93..+0x0EB7 on size 0 or on slot + size/780 > 4: resident 0x464:0x00FF, runtime
     * 0xAF8:0x0919 and 0x084A with the 38-character literal at +0x0DC1 and [0x8608], key read
     * 0x899:0x0059 (result stored, never used), 0x464:0x00FF again. */
    void (*error_notice)(void *ctx);
    /* +0x0EBC: resident 0x899:0x0000, the program exit (DOS terminate). Must not return in
     * the original; the model returns PR_U3BD_EXITED right after calling it. */
    void (*exit_program)(void *ctx);
    /* Runtime 0xAF8:0x0254 (FreeMem shape) with (block, size), after the copies. */
    void (*release)(void *ctx, const uint8_t *block, uint16_t size);
    /* Overlay unit 0x40c entry 1 (+0x003B, stub 0x428:0x25, `retf 4`) with (arg, slot) as
     * pushed: arg = id when one piece was copied, else (id == 0 ? 10 : id) * 10 + piece index. */
    void (*piece_loaded)(void *ctx, uint16_t arg, uint8_t slot);
} pr_u3bd_callouts;

typedef struct {
    unsigned slot_ok;         /* 1 when 1 <= slot <= 3 (the gate at +0x0E0F) */
    unsigned read_size;       /* size returned by the loader */
    unsigned notice;          /* 1 when the notice path ran (the routine then exits the program) */
    unsigned exited;          /* 1 when exit_program was called */
    unsigned pieces_copied;   /* copies done (the do-while runs at least once on the non-notice path) */
    unsigned count;           /* pieces counted by the original: ceil(size / 780) */
    unsigned remapped;        /* pieces that passed the slot-set test {1,2,3} and were remapped */
    unsigned announced;       /* piece_loaded calls made */
    unsigned final_id;        /* value stored in slots[slot].w6aae (id, with 10 mapped to 0) */
} pr_u3bd_report;

/* The entry. `id` = [bp+8] low byte, `slot` = [bp+6] low byte. */
pr_u3bd_status pr_u3bd_entry6(pr_u3bd_state *state, uint8_t id, uint8_t slot,
                              const pr_u3bd_callouts *callouts, pr_u3bd_report *report);

/* The slot-set test the original performs through runtime 0xAF8:0x0DC0 against the
 * 32-byte set constant at +0x0DE8: exactly the members 1, 2 and 3. */
int pr_u3bd_slot_in_set(uint8_t slot);

#endif
