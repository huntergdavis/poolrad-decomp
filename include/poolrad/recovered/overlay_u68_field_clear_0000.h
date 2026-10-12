#ifndef POOLRAD_RECOVERED_OVERLAY_U68_FIELD_CLEAR_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U68_FIELD_CLEAR_0000_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x68 internal routine
 * +0x0000..+0x0108 (file 0x166CB, exactly 265 bytes, 112 instructions,
 * `retf 6`, no calls, no trailing data: the next routine is +0x0109).
 * Near-called only by entry 1 (+0x0D6D) at +0x0E8D and +0x0FCC, with the
 * party x, y and facing bytes. Evidence: docs/re/U68_FIELD_CLEAR_0000.md.
 *
 * Three pushed words, each read as a byte: [bp+0xA] = x (first), [bp+8] = y,
 * [bp+6] = selector. It clears one 2-bit field of the byte at
 * far[0x6A5C] + 0x300 + 16 * y + x (the fourth quarter of the 1,024-byte
 * buffer; the same byte and bit pairs unit 0x3bd entry 3 reads):
 *   selector 6: byte &= 0x3F   (bits 7..6)      (+0x0006..+0x0044)
 *   selector 4: byte &= 0xCF   (bits 5..4)      (+0x0047..+0x0085)
 *   selector 2: byte &= 0xF3   (bits 3..2)      (+0x0087..+0x00C5)
 *   selector 0: byte &= 0xFC   (bits 1..0)      (+0x00C7..+0x0100)
 *   any other selector: nothing is read or written (+0x00C9 jne +0x0105)
 * x and y are sign-extended (`cbw`), y is shifted left by 4 as a 16-bit
 * word, and the address is the original's 16-bit one: far offset + (y << 4)
 * + x wraps modulo 65536, and so does the +0x300 displacement. There is no
 * range check. The byte is read, masked into BL, and written back to the
 * same address (the address is computed twice with the same result).
 *
 * Classification: full translation of all 112 instructions: no calls, and
 * the only memory touched is that one byte. The far pointer is explicit host
 * state (a view of its segment plus the offset word); the only guard is the
 * host one for an address outside the view. No game data is embedded. */

#include <stddef.h>
#include <stdint.h>

#define PR_U68_0000_QUARTER 0x300u    /* es:[di + 0x300] */
#define PR_U68_0000_ROW_SHIFT 4u      /* mov cl, 4; shl ax, cl */

typedef struct {
    uint8_t *segment;     /* host bytes of far [0x6A5C]'s segment, from offset 0 (at most 64 KiB) */
    size_t segment_size;
    uint16_t offset;      /* the far pointer's offset word */
} pr_u68_0000_state;

typedef enum {
    PR_U68_0000_OK = 0,
    PR_U68_0000_ARGUMENT  /* null state or segment, segment larger than 64 KiB, or the (wrapped) address
                             at or past the view. Checked before any effect. */
} pr_u68_0000_status;

typedef struct {
    unsigned touched;       /* a selector in {0, 2, 4, 6}: the byte was read and written */
    uint16_t address;       /* effective offset of the byte (valid when touched) */
    uint8_t before, after;  /* the byte before and after (valid when touched) */
} pr_u68_0000_report;

/* The routine. `x` = [bp+0xA], `y` = [bp+8], `selector` = [bp+6], raw bytes. `report` may be NULL. */
pr_u68_0000_status pr_u68_0000(pr_u68_0000_state *state, uint8_t x, uint8_t y, uint8_t selector,
                               pr_u68_0000_report *report);

#endif
