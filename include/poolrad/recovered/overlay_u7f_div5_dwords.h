#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_DIV5_DWORDS_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_DIV5_DWORDS_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x7f public entry 11
 * (+0x0185, file 0x24E0E, run-time stub 0x8F:0x0057): a leaf that writes
 * twenty bytes of the data-segment block at DS:0x67F4. One pushed word,
 * [bp+6]; no result (every caller jumps away without reading AX or DX);
 * `retf 2`. Reachable code is 86 bytes, 34 instructions, +0x0185..+0x01DA;
 * the ledger's 126-byte bound runs on into 40 bytes of non-code data
 * (+0x01DB..+0x0202, a length-prefixed string constant that the next
 * routine, +0x0203, references and this one never touches). Three
 * fixup-confirmed far callers; no outgoing calls.
 *
 * Paths, in the original's order:
 *   [bp-1] = 0; for i = 0..4 (`cmp [bp-1],4; jne`, do-while form):
 *     di = 4*i; word [di+0x67F4] = 0; word [di+0x67F6] = 0   (+0x0191..+0x01AC)
 *   AX = [bp+6]; DX = 0; CX = 5; div CX; DX = 0;
 *     word [0x6804] = AX (unsigned quotient); word [0x6806] = DX (0)  (+0x01AE..+0x01C0)
 *   AX = [bp+6]; DX = 0; CX = 5; div CX; xchg ax,dx; DX = 0;
 *     word [0x6800] = AX (remainder); word [0x6802] = DX (0)          (+0x01C1..+0x01D4)
 * Net effect on the block, as dwords from 0x67F4: slots 0, 1, 2 = 0;
 * slot 3 (0x6800) = value mod 5; slot 4 (0x6804) = value div 5; both
 * unsigned and zero-extended to 32 bits (the `xor dx,dx` after each
 * division discards the other half before the high word is stored). The
 * loop's zeros in slots 3 and 4 are overwritten. Nothing else is written;
 * nothing is read from the block.
 * Classification: full translation of the 86 code bytes (FUNCTION_LEDGER.md
 * §1 vocabulary). Game ownership: the classifier leaves this routine
 * unresolved (no string, no documented global, no verified callee); the
 * block it writes is zero-filled with length 0x1C by a game_verified
 * routine and read by game_evidenced routines, which is recorded as
 * evidence in docs/re/U7F_DIV5_DWORDS_ENTRY11.md for the PM to weigh. No
 * strict game-function credit is claimed here. "div5" names the mechanism;
 * what the slots hold is not asserted. No game data is embedded. */

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_DWORDS_BLOCK_DS 0x67F4u          /* DS address of slot 0 (the host block's byte 0) */
#define PR_U7F_DWORDS_BLOCK_SIZE 0x1Cu          /* the length the block's writers pass to the runtime fill
                                                   (unit 0x1c +0x1276, 0x1e +0xD26, 0x23 +0x55C) and entry 12
                                                   copies (+0x00B8): seven dwords; this routine writes five */
#define PR_U7F_DWORDS_CLEAR_SLOTS 5u            /* +0x01A8: the loop counter runs 0..4 */
#define PR_U7F_DWORDS_REMAINDER_OFFSET 0x0Cu    /* 0x6800 - 0x67F4: slot 3 = value mod 5 */
#define PR_U7F_DWORDS_QUOTIENT_OFFSET 0x10u     /* 0x6804 - 0x67F4: slot 4 = value div 5 */
#define PR_U7F_DWORDS_DIVISOR 5u                /* +0x01B3 / +0x01C6: mov cx, 5 */
#define PR_U7F_DWORDS_WRITE_END 0x14u           /* one past the last byte written (0x6807) */

typedef enum {
    PR_U7F_DWORDS_OK = 0,
    PR_U7F_DWORDS_ARGUMENT      /* null block, or block_size < PR_U7F_DWORDS_WRITE_END: checked on
                                   entry, nothing is written. The original addresses DS directly
                                   and cannot fail here; the guard is host-side only. */
} pr_u7f_dwords_status;

typedef struct {
    unsigned cleared_slots;     /* dwords zeroed by the loop at +0x0191..+0x01AC: always 5 */
    uint16_t quotient;          /* AX after the first `div cx`: value / 5, stored at 0x6804 */
    uint16_t remainder;         /* DX after the second `div cx`: value % 5, stored at 0x6800 */
    unsigned first_offset;      /* 0x00: lowest block byte written (DS 0x67F4) */
    unsigned last_offset;       /* 0x13: highest block byte written (DS 0x6807) */
} pr_u7f_dwords_report;

/* The entry. `block` is host memory whose byte 0 is DS:0x67F4 (the engine's
 * block is PR_U7F_DWORDS_BLOCK_SIZE bytes; any size of at least
 * PR_U7F_DWORDS_WRITE_END is accepted); `value` is the pushed word [bp+6].
 * On PR_U7F_DWORDS_OK bytes 0x00..0x13 hold, as little-endian dwords:
 * 0, 0, 0, value % 5, value / 5; every other byte is untouched. `report`
 * may be NULL. */
pr_u7f_dwords_status pr_u7f_entry11(uint8_t *block, size_t block_size, uint16_t value,
                                    pr_u7f_dwords_report *report);

#endif
