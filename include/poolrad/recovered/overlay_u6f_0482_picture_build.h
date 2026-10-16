#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_0482_PICTURE_BUILD_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_0482_PICTURE_BUILD_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x6F + 0x0482: public entry 3 (run-time stub 0x7C:0x2F),
 * executable [0482,0674), 498 bytes, 183 instructions, 16 call sites, one
 * word argument (its low byte is the recolor flag) consumed by `retf 2`.
 * Evidence and offsets: docs/re/U6F_0482_PICTURE_BUILD.md. It builds the
 * active character's picture pair from the character record at DS:0x5D92:
 *   +0489  resident 0x802:0x094A with the pushed byte 3 (the archive-number
 *          push: DS:0x5377 := DS:0x5376, DS:0x5376 := 3);
 *   +0491  rec := DS:[0x5D92] (the active character, far pointer);
 *   +049B  name := CS:0x0476 (5 characters) + Chr(DS:[0x08C5 + rec[0xC0]])
 *          (0xAF8:0x0B0B, 0x0C3B, 0x0B98); unit 0x3CE entry 4 (0x3E9:0x34,
 *          the picture loader) with (name, p3 = rec[0xBD], slot 0x0B);
 *   +04DD  name := CS:0x047C (5 characters) + the same letter; the loader
 *          with (name, p3 = rec[0xBE], slot = rec[0xBF]);
 *   +0525  near +032F (the picture merge leaf) with (DS:[0x5EB2], the slot's
 *          first picture DS:[0x5E5A + 8*rec[0xBF]]); again with (DS:[0x5EB6],
 *          the slot's second picture at +4);
 *   +056D  a zero argument byte skips to +065C;
 *   +0576  map_a[0..15] := 0..15; map_b[0..15] := 0..15;
 *   +059F  for k := 1 to 6: i := DS:[0x3CF5 + k]; map_b[i] := rec[0xC0+k] & 0xF;
 *          map_b[i+8] := rec[0xC0+k] >> 4;
 *   +0602  resident 0x464:0x1E5C (the palette remap) with (map_a, map_b, word 0,
 *          word 0, the slot's first picture); again with the second picture;
 *   +065C  unit 0x3CE entry 3 (0x3E9:0x2F, the slot release) with 0x0B;
 *   +0664  resident 0x802:0x095D (DS:0x5376 := DS:0x5377);
 *   +0669  resident 0x899:0x0104 (flush the keyboard); +066E retf 2.
 * The push helper, the loader, the merge leaf, the remap, the release, the
 * restore and the flush are boundaries supplied as callbacks; the string
 * composition and the map construction are translated inline; the two
 * private CS literals and the two DS tables are supplied by the caller.
 * The two loops have fixed trip counts (16 and 6); no host guard is needed. */

#define PR_U6F_0482_MAP_SIZE 16u
#define PR_U6F_0482_PREFIX_LENGTH 5u
#define PR_U6F_0482_TEMP_SLOT 0x0Bu
#define PR_U6F_0482_COLOR_COUNT 6u
#define PR_U6F_0482_RECORD_MIN 0xC1u          /* bytes 0xBD..0xC0 are read on every path */
#define PR_U6F_0482_RECORD_RECOLOR_MIN 0xC7u  /* bytes 0xC1..0xC6 on the recolor path */
#define PR_U6F_0482_OFF_P3_FIRST 0xBDu
#define PR_U6F_0482_OFF_P3_SECOND 0xBEu
#define PR_U6F_0482_OFF_SLOT 0xBFu
#define PR_U6F_0482_OFF_LETTER 0xC0u

typedef struct {
    void *context;
    int (*selector_push)(void *context, uint8_t low_byte);                    /* 0x802:0x094A; AH is inherited, not modelled */
    int (*load_picture)(void *context, const uint8_t *pascal_name, size_t span,
                        uint8_t p3, uint8_t slot);                            /* unit 0x3CE entry 4 */
    int (*merge_picture)(void *context, uint8_t slot, uint8_t which);         /* +032F: which 0 = base DS:0x5EB2 into the
                                                                               * slot's first picture, 1 = DS:0x5EB6 into its second */
    int (*recolor_picture)(void *context, const uint8_t *map_a, const uint8_t *map_b,
                           uint8_t slot, uint8_t which);                      /* 0x464:0x1E5C with the two zero words */
    int (*release_slot)(void *context, uint8_t slot);                         /* unit 0x3CE entry 3 */
    int (*restore_archive_number)(void *context);                             /* 0x802:0x095D */
    int (*flush_keys)(void *context);                                         /* 0x899:0x0104 */
} pr_u6f_0482_hooks;

typedef struct {
    size_t loads;                /* loader calls completed (0..2) */
    size_t merges;               /* merge calls completed (0..2) */
    size_t recolors;             /* remap calls completed (0..2) */
    uint8_t released;            /* the release was reached */
    uint8_t restored;            /* the restore was reached */
    uint8_t flushed;             /* the flush was reached */
    uint8_t letter;              /* DS:[0x08C5 + rec[0xC0]] */
    uint8_t map_a[PR_U6F_0482_MAP_SIZE];   /* SS:BP-0x13, valid when recolors > 0 or next_offset >= 0x0602 */
    uint8_t map_b[PR_U6F_0482_MAP_SIZE];   /* SS:BP-0x23 */
    uint16_t next_offset;        /* source offset the model stopped at, or 0x0674 after retf */
} pr_u6f_0482_result;

typedef enum {
    PR_U6F_0482_OK = 0,
    PR_U6F_0482_ARGUMENT,        /* spans, literal lengths, table index past a span, a map index above 7 */
    PR_U6F_0482_NEED_HOOK,       /* a reached boundary has no callback; next_offset = the call instruction */
    PR_U6F_0482_HOST_FAILURE     /* a callback returned non-zero; earlier effects kept */
} pr_u6f_0482_status;

/* record: the active character record (DS:[0x5D92]); its span must hold byte
 * 0xC0 (and 0xC6 when recolor is non-zero). letter_table: DS:0x08C5 with its
 * span (rec[0xC0] must index inside it). index_table: DS:0x3CF5 with its span
 * (at least 7 bytes; entries 1..6 must be at most 7, as the engine's 16-byte
 * local allows). prefix_a / prefix_b: the CS:0x0476 and CS:0x047C constants
 * (5 characters each). recolor: the low byte of the word argument. */
pr_u6f_0482_status pr_u6f_0482_picture_build(
    uint8_t recolor,
    const uint8_t *record, size_t record_size,
    const uint8_t *letter_table, size_t letter_table_size,
    const uint8_t *index_table, size_t index_table_size,
    const uint8_t *prefix_a, size_t prefix_a_size,
    const uint8_t *prefix_b, size_t prefix_b_size,
    const pr_u6f_0482_hooks *hooks,
    pr_u6f_0482_result *out);

#define PR_U6F_0482_RETF_OFFSET 0x0671u
#define PR_U6F_0482_END_OFFSET 0x0674u

#endif
