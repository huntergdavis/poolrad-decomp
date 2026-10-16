#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_032F_PICTURE_MERGE_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_032F_PICTURE_MERGE_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x6F + 0x032F: a near-called leaf (not a public entry),
 * executable [032F,0476), 327 bytes, 121 instructions, 15 branches, no
 * calls, two far-pointer arguments consumed by `retf 8`: the source bitmap
 * object at [bp+0xA] (pushed first) and the destination at [bp+6]. Its only
 * callers are the two sites in +0482 (+0546, +056A; the picture builder's
 * `merge_picture` boundary). Evidence: docs/re/U6F_032F_PICTURE_MERGE.md.
 * Bitmap objects (the resident 0x464 constructor's layout): the span word at
 * +0x11, the auxiliary far pointer at +0x13/+0x15, the primary data at +0x17.
 *   +0335  mode := DS:[0x4A1E] (read through the DS view at entry);
 *   mode 0 or 2 (+0343): n := src.span - 1 (16-bit); for i := 0 to n:
 *          dst.data[i] |= src.data[i]; dst.aux[i] &= src.aux[i];
 *   mode 1 (+03C8): n := src.span - 1; for i := 0 to n: dst.data[i] |= src.data[i];
 *          then (+0412, AFTER the OR pass, so an OR that aliased DS:0x5EB2
 *          changes it) base := the far pointer DS:[0x5EB2] (the slot-0x0B
 *          first picture, table entry DS:0x5E5A + 8*11), m := (base.span shr 2)
 *          - 1; for i := 0 to m: dst.aux[i] &= src.aux[i];
 *   other modes: nothing; +0470 frame restore; retf 8.
 * The loops are Turbo Pascal `for 0 to n` loops compiled with an unsigned
 * entry test (`ja`) that never fires, so a span of 0 (or a span below 4 in
 * the mode-1 auxiliary loop) runs the loop for all 65,536 offsets; the model
 * reproduces that. Every byte access goes through host segment views with
 * 16-bit offset wrap, so overlapping or aliased objects behave as in DOS and
 * an access outside the supplied views stops with BOUNDS, earlier writes
 * kept. This is a complete translation of the leaf over mapped memory; no
 * callbacks. No native differential has been run; no strict credit is claimed. */

typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_u6f_032f_far;

typedef struct {
    uint16_t segment;
    uint8_t *bytes;              /* the host view of the whole segment window used */
    size_t size;                 /* offsets below size are mapped */
} pr_u6f_032f_segment;

typedef struct {
    uint16_t ds_segment;         /* the DS paragraph; DS:0x4A1E and DS:0x5EB2 are read through its view */
    pr_u6f_032f_segment *segments;
    size_t segment_count;
} pr_u6f_032f_state;

typedef struct {
    uint8_t mode;                /* DS:[0x4A1E] as read at entry */
    uint8_t route;               /* 0: modes 0/2; 1: mode 1; 2: other modes (nothing) */
    pr_u6f_032f_far base;        /* DS:[0x5EB2] as read at +0412 (mode 1) */
    uint32_t data_bytes;         /* OR steps completed */
    uint32_t aux_bytes;          /* AND steps completed */
    uint16_t src_span;           /* src.span as read */
    uint16_t base_span;          /* DS:[0x5EB2]^.span as read (mode 1) */
    uint16_t next_offset;        /* source offset the model stopped at, or 0x0476 after retf */
} pr_u6f_032f_result;

typedef enum {
    PR_U6F_032F_OK = 0,
    PR_U6F_032F_ARGUMENT,        /* null state/output/segment table */
    PR_U6F_032F_BOUNDS           /* a DOS access (including the two DS reads) outside the supplied views */
} pr_u6f_032f_status;

pr_u6f_032f_status pr_u6f_032f_picture_merge(
    const pr_u6f_032f_state *state,
    pr_u6f_032f_far src, pr_u6f_032f_far dst,
    pr_u6f_032f_result *out);

#define PR_U6F_032F_DS_MODE 0x4A1Eu
#define PR_U6F_032F_DS_SLOT11_FIRST 0x5EB2u
#define PR_U6F_032F_OFF_SPAN 0x11u
#define PR_U6F_032F_OFF_AUX 0x13u
#define PR_U6F_032F_OFF_DATA 0x17u
#define PR_U6F_032F_RETF_OFFSET 0x0473u
#define PR_U6F_032F_END_OFFSET 0x0476u

#endif
