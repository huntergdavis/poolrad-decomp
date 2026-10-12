#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_DIV5_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_DIV5_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x7f public entry 10
 * (+0x0130, file 0x24DB9, run-time stub 0x8F:0x0052): a leaf that writes
 * ten bytes of the record behind the active-character far pointer [0x5D92].
 * One pushed word, [bp+6]; no result (every caller jumps away without
 * reading AX); `retf 2`. 85 bytes, 31 instructions; three fixup-confirmed
 * far callers; no outgoing calls. The far pointer [0x5D92] is read by
 * `les di` at three sites (+0x0145, +0x0162, +0x0176) executed seven times
 * in all: five loop passes, then once before each of the two later stores.
 *
 * Paths, in the original's order:
 *   [bp-1] = 0; for i = 0..4 (`cmp [bp-1],4; jne`, do-while form):
 *     les di,[0x5D92]; word es:[di + 0x88 + 2*i] = 0      (+0x013C..+0x0156)
 *   AX = [bp+6]; DX = 0; CX = 5; div CX; les di,[0x5D92];
 *     word es:[di + 0x90] = AX (the unsigned quotient)      (+0x0158..+0x016A)
 *   AX = [bp+6]; DX = 0; CX = 5; div CX; xchg ax,dx;
 *     les di,[0x5D92]; word es:[di + 0x8E] = AX (remainder) (+0x016B..+0x017E)
 * Net effect: words +0x88, +0x8A, +0x8C = 0; +0x8E = value mod 5;
 * +0x90 = value div 5 (both unsigned; the loop's zeros at +0x8E/+0x90 are
 * overwritten). No other memory is written; nothing is read from the record.
 * Classification: full translation (FUNCTION_LEDGER.md §1 vocabulary):
 * every instruction path is reproduced with the same input and the same
 * ten output bytes. Evidence: docs/re/U7F_DIV5_ENTRY10.md.
 * "div5" names the mechanism only; what the five word slots hold, and why
 * the argument is split into a quotient and a remainder of 5, are not
 * asserted here. No game data is embedded. */

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_DIV5_RECORD_SIZE 285u        /* verified record frame (docs/formats/DOS_CHARACTER.md) */
#define PR_U7F_DIV5_CLEAR_BASE 0x88u        /* +0x014D: es:[di + 0x88 + 2*i] */
#define PR_U7F_DIV5_CLEAR_WORDS 5u          /* +0x0152: the loop counter runs 0..4 */
#define PR_U7F_DIV5_REMAINDER_OFFSET 0x8Eu  /* +0x017A: es:[di + 0x8E] = value mod 5 */
#define PR_U7F_DIV5_QUOTIENT_OFFSET 0x90u   /* +0x0166: es:[di + 0x90] = value div 5 */
#define PR_U7F_DIV5_DIVISOR 5u              /* +0x015D / +0x0170: mov cx, 5 */
#define PR_U7F_DIV5_WRITE_END 0x92u         /* one past the last byte written (+0x91) */

typedef enum {
    PR_U7F_DIV5_OK = 0,
    PR_U7F_DIV5_ARGUMENT        /* null record, or record_size < PR_U7F_DIV5_WRITE_END: checked
                                   on entry, nothing is written. The original performs no such
                                   check; a nil [0x5D92] would write through segment 0 at
                                   offsets 0x88..0x91. The size bound is a host-side guard, not
                                   an engine behaviour. */
} pr_u7f_div5_status;

typedef struct {
    unsigned cleared_words;     /* words zeroed by the loop at +0x013C..+0x0156: always 5 */
    uint16_t quotient;          /* AX after the first `div cx`: value / 5, stored at +0x90 */
    uint16_t remainder;         /* DX after the second `div cx`: value % 5, stored at +0x8E */
    unsigned first_offset;      /* 0x88: lowest record byte written */
    unsigned last_offset;       /* 0x91: highest record byte written */
} pr_u7f_div5_report;

/* The entry. `record` is host memory for the object at far [0x5D92] (the
 * engine's frame is PR_U7F_DIV5_RECORD_SIZE bytes; any size of at least
 * PR_U7F_DIV5_WRITE_END is accepted); `value` is the pushed word [bp+6].
 * On PR_U7F_DIV5_OK bytes 0x88..0x91 hold, little-endian: 0, 0, 0,
 * value % 5, value / 5; every other byte is untouched. `report` may be
 * NULL. */
pr_u7f_div5_status pr_u7f_entry10(uint8_t *record, size_t record_size, uint16_t value,
                                  pr_u7f_div5_report *report);

#endif
