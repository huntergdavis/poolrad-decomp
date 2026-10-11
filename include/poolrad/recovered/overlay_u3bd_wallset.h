#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_WALLSET_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_WALLSET_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x3bd public entry 7
 * (+0x10C7, file 0x2F9BE, run-time stub 0x3D5:0x0043), the routine the
 * merged LOAD FILES handler (unit 0x1c +0x0C74, opcode 0x21 wallset branch,
 * PR #97's `load_wallset(ctx, id)` boundary) and the saved-game loader
 * (unit 0x6f +0x1602) call with one pushed word: [bp+6] = id, read as a
 * byte; `retf 2`.
 *
 * Classification: path-complete local-body model with callback
 * boundaries, not a full translation (FUNCTION_LEDGER.md vocabulary). The
 * routine's own paths are reproduced: block read through the resident DAX
 * loader for a name built from the file-number byte, the size gate (only
 * a 1,026-byte block proceeds), the terminal notice path (ending in
 * resident 0x899:0x0000, a DOS terminate), four 256-byte copies from block
 * offset 2 into the 1,024-byte buffer at far [0x6A5C], FreeMem, and the
 * store of id into window-0 word 0xC5. The runtime string construction,
 * the loader, the notice display, the exit and FreeMem are callbacks.
 * Sixteen far-call sites to eleven distinct targets. Evidence:
 * docs/re/U3BD_WALLSET_ENTRY7.md. Names are addresses; "wallset" is the
 * handler's name for this branch, not a claim about the data. No game
 * data is embedded. */

#include <stddef.h>
#include <stdint.h>

#define PR_U3BD_WALLSET_BLOCK_SIZE 0x402u    /* the only accepted block size */
#define PR_U3BD_WALLSET_DATA_OFFSET 2u       /* copies start at block + 2 */
#define PR_U3BD_WALLSET_QUARTER 0x100u       /* four Move calls of 256 bytes */
#define PR_U3BD_WALLSET_BUFFER_SIZE 0x400u   /* far [0x6A5C] receives 1,024 bytes */
#define PR_U3BD_WALLSET_NAME_PREFIX_LEN 3u   /* code-segment literal at +0x109D */
#define PR_U3BD_WALLSET_NAME_SUFFIX_LEN 4u   /* code-segment literal at +0x10A1 */

typedef struct {
    uint8_t file_number;          /* byte [0x5376] */
    uint8_t *buffer;              /* host memory for far [0x6A5C], PR_U3BD_WALLSET_BUFFER_SIZE bytes */
    uint16_t window0_word_c5;     /* word at [0x49D2] + 0x18A, i.e. window 0 word 0xC5 */
} pr_u3bd_wallset_state;

typedef enum {
    PR_U3BD_WALLSET_OK = 0,
    PR_U3BD_WALLSET_ARGUMENT,     /* null state or callouts (checked on entry), or a null host
                                     buffer when the copies are reached (the original first
                                     touches [0x6A5C] there, after the read and the size gate) */
    PR_U3BD_WALLSET_CALLOUT,      /* a required callback was missing where reached; earlier effects done */
    PR_U3BD_WALLSET_EXITED,       /* size 0 or size != 0x402: notice, then 0x899:0x0000 terminates the program */
    PR_U3BD_WALLSET_BLOCK_BOUNDS  /* at the copies: no block pointer, or fewer than 0x402
                                     readable bytes; nothing is copied */
} pr_u3bd_wallset_status;

typedef struct {
    void *ctx;
    /* Resident 0x802:0x0716 with (name = 3-char literal + decimal file number, 4-char
     * extension literal, block id, &size, &pointer). `capacity` = readable bytes of `block`. */
    void (*read_block)(void *ctx, uint8_t file_number, uint8_t block_id,
                       const uint8_t **block, uint16_t *size, size_t *capacity);
    /* +0x1132..+0x1156: resident 0x464:0x00FF, runtime 0xAF8:0x0919 and 0x084A with the
     * 32-character literal at +0x10A6 and [0x8608], key read 0x899:0x0059 (stored, unused),
     * 0x464:0x00FF again. */
    void (*error_notice)(void *ctx);
    /* +0x115B: resident 0x899:0x0000, DOS terminate; never returns in the original. */
    void (*exit_program)(void *ctx);
    /* Runtime 0xAF8:0x0254 (FreeMem shape) with (block, size), after the four copies. */
    void (*release)(void *ctx, const uint8_t *block, uint16_t size);
} pr_u3bd_wallset_callouts;

typedef struct {
    unsigned read_size;
    unsigned notice;
    unsigned exited;
    unsigned copied;              /* 1 when the four quarters were copied */
    unsigned stored_id;           /* value written to window-0 word 0xC5 */
} pr_u3bd_wallset_report;

/* The entry. `id` = [bp+6] low byte. */
pr_u3bd_wallset_status pr_u3bd_entry7(pr_u3bd_wallset_state *state, uint8_t id,
                                      const pr_u3bd_wallset_callouts *callouts,
                                      pr_u3bd_wallset_report *report);

#endif
