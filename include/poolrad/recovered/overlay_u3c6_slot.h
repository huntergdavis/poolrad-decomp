#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_SLOT_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_SLOT_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x3c6 public entries
 * 12, 13, 14 and 15 (stub paragraph 0x3E0, stubs 0x3E0:0x5C, 0x61, 0x66,
 * 0x6B; unit code at file 198,458). Evidence: docs/re/U9E_0160.md.
 *
 * Entry 15 (+0x0C5F, 90 bytes, `retf 4`, one far-pointer argument
 * [bp+8]:[bp+6]): searches the data-segment far-pointer table at DS:0x65B9
 * (4-byte entries, slot i at 0x65B9 + 4i) for the argument, i = 1 up to the
 * byte [0x5F2A] with a byte counter, and returns the slot number, or 0.
 * Entries 12, 13 and 14 (+0x0BE7, +0x0C0F, +0x0C37, 40 bytes each, `retf 4`)
 * return the DS byte at 0x5F27, 0x5F28 or 0x5F2A plus 4 * entry15(arg).
 * Slot 0 is the same memory as [0x5F2A] (entry 14 with slot 0 reads the
 * count byte), which the flat byte array below reproduces.
 *
 * Classification: full translation of the four routines (no calls except
 * entries 12..14's near call to entry 15); the DS tables are explicit host
 * state and nothing else is read or written. No game data is embedded. */

#include <stdint.h>

#define PR_U3C6_SLOTS 256u
#define PR_U3C6_PTR_TABLE_DS 0x65B9u       /* `mov ax, [di + 0x65B9]` and `[di + 0x65BB]` */
#define PR_U3C6_BYTES_DS 0x5F27u           /* `mov al, [di + 0x5F27]` (entry 12) */
#define PR_U3C6_COUNT_DS 0x5F2Au           /* `cmp al, [0x5F2A]` and entry 14's base */

typedef struct {
    const void *slot_ptr[PR_U3C6_SLOTS];  /* far pointer at DS:0x65B9 + 4 * i, compared whole (offset and segment) */
    uint8_t bytes[4u * PR_U3C6_SLOTS];    /* DS:0x5F27 + k; bytes[3] is [0x5F2A], the slot count */
} pr_u3c6_state;

/* Entry 15. Returns AL. */
uint8_t pr_u3c6_entry15(const pr_u3c6_state *state, const void *far_ptr);
/* Entries 12, 13, 14: DS byte at 0x5F27 / 0x5F28 / 0x5F2A + 4 * entry15(far_ptr). */
uint8_t pr_u3c6_entry12(const pr_u3c6_state *state, const void *far_ptr);
uint8_t pr_u3c6_entry13(const pr_u3c6_state *state, const void *far_ptr);
uint8_t pr_u3c6_entry14(const pr_u3c6_state *state, const void *far_ptr);

#endif
