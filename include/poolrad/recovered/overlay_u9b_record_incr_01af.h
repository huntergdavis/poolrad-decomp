#ifndef POOLRAD_RECOVERED_OVERLAY_U9B_RECORD_INCR_01AF_H
#define POOLRAD_RECOVERED_OVERLAY_U9B_RECORD_INCR_01AF_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x9b +0x01AF..+0x0254 (file
 * 175,158..175,323, exactly 166 bytes, 49 instructions, `retf 4`, no calls). The unit's memory
 * segment is 0xAC, so the far call `0xAC:0x25` made at the end of the party insertion
 * (unit 0x6f +0x1574) is the overlay thunk for this function (thunk offset 0x25). Evidence:
 * docs/re/U9B_RECORD_INCR_01AF.md.
 *
 * One far-pointer argument (offset [bp+6], segment [bp+8]; `retf 4`) addresses a character
 * record; t is the byte at +0x12. The routine has five independent steps in this order, each
 * an unsigned byte compare and, only when taken, an increment modulo 256 of a counter byte
 * (the counter is re-read before each step, so an earlier step that wrapped it to 0 stops the
 * later one):
 *     t > 0x0C and b[+0xB2] <> 0: b[+0xB2] += 1
 *     t > 0x0D and b[+0xB2] <> 0: b[+0xB2] += 1
 *     t > 0x0E and b[+0xB3] <> 0: b[+0xB3] += 1
 *     t > 0x0F and b[+0xB3] <> 0: b[+0xB3] += 1
 *     t > 0x10 and b[+0xB4] <> 0: b[+0xB4] += 1
 * No other byte is read or written. Every access is es:[di + k] with di the far offset, so the
 * effective offset wraps modulo 65536. The record is explicit host state: a view of the
 * record's ES segment from offset 0 plus the far offset. An access at or past the view's size
 * (or a null view) is a host guard that stops the routine with the increments already done
 * retained. What the bytes mean is not asserted.
 *
 * Classification: full translation of all 49 instructions (no calls, no state besides the
 * record view). The only guards are host-side. No game data is embedded. */

#include <stddef.h>
#include <stdint.h>

#define PR_U9B_INCR_T 0x12u         /* es:[di + 0x12] */
#define PR_U9B_INCR_B2 0xB2u
#define PR_U9B_INCR_B3 0xB3u
#define PR_U9B_INCR_B4 0xB4u
#define PR_U9B_INCR_STEPS 5u

typedef struct {
    uint8_t *segment;               /* host bytes of the record's ES segment, from offset 0 */
    size_t segment_size;            /* at most 0x10000; accesses at or past it are a host guard */
    uint16_t offset;                /* the far pointer's offset word */
} pr_u9b_record;

typedef enum {
    PR_U9B_INCR_OK = 0,
    PR_U9B_INCR_ARGUMENT            /* null record, or a reached access outside the view (earlier increments stay) */
} pr_u9b_incr_status;

typedef struct {
    unsigned increments;            /* number of increments performed */
    unsigned steps_taken;           /* bit i set: step i (0..4) incremented its counter */
} pr_u9b_incr_report;

pr_u9b_incr_status pr_u9b_record_incr(const pr_u9b_record *record, pr_u9b_incr_report *report);

#endif
