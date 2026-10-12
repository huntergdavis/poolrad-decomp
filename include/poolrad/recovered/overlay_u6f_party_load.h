#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_LOAD_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_LOAD_H

/* Host model of the party-building part of DOS v1.3 GAME.OVR unit 0x6f public entry 8
 * (+0x1602, the world-save loader): the character-record loop at +0x1926..+0x19F4, the
 * record fix-ups of the record loader (+0x0AEC) and the party insertion (+0x1446). It
 * does not read files, load sidecars, or run the unit's other post-load effects (the
 * 468-byte call at +0x02AE after each insertion is not modelled). The far call 0xAC:0x25 at
 * the end of each insertion is the overlay thunk for unit 0x9b +0x01AF (see
 * overlay_u9b_record_incr_01af.h) and is applied to the member's record.
 * Evidence: docs/re/SLOT_A_PARTY_INPUT.md.
 *
 * Plan: the loader reads the local character count (world-save byte 0x3208) and eight
 * 41-byte Pascal basename slots (0x3209..0x3350). For i = 1..count it takes slot i-1
 * (stride 0x29), composes "<basename>.sav", and, only when that file exists, allocates a
 * 0x11D-byte record, reads it, and appends it to the party list (head far [0x5D96]).
 * The count byte is a loop bound the original does not check against the eight slots;
 * the model rejects a count above eight (a host bound).
 *
 * Build: for each present record, in load order:
 *   - the 285 bytes are copied; the far-pointer slots +0x7F..+0x82, +0xC8..+0xCB and
 *     +0x104..+0x10B are cleared (the record loader, +0x0C54..+0x0C8A) and
 *   - byte +0xBF is the first index 0..7 not used by an earlier member (+0x1538..+0x1563);
 *   - the member is appended at the tail through the link at +0x104/+0x106 (the model's
 *     node chain; the raw link bytes stay zero);
 *   - the unit 0x9b +0x01AF routine then runs on the member's record (+0xB2/+0xB3/+0xB4 counters);
 *   - the window-1 counter (word [0x49D6]+0x67C) is zeroed before the loop and
 *     incremented once per present member (+0x1917, +0x1569): `loaded`.
 * A missing record (the existence test at +0x19A8 fails) is skipped and takes no slot.
 *
 * Classification: bounded host model. No game data is embedded. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u68_party_byte_0236.h"

#define PR_U6F_SAVGAM_SIZE 0x3351u            /* the world file length */
#define PR_U6F_PARTY_COUNT_OFFSET 0x3208u     /* local character count */
#define PR_U6F_PARTY_NAMES_OFFSET 0x3209u     /* eight 41-byte Pascal basename slots */
#define PR_U6F_PARTY_SLOTS 8u
#define PR_U6F_PARTY_NAME_STRIDE 0x29u        /* mov dx, 0x29 ; mul dx */
#define PR_U6F_PARTY_NAME_MAX 40u             /* Pascal length byte plus 40 text bytes per slot */
#define PR_U6F_PARTY_RECORD_SIZE 0x11Du       /* GetMem(0x11D) and the 285-byte read */
#define PR_U6F_PARTY_SLOT_BYTE 0xBFu          /* in-memory party slot */

typedef enum {
    PR_U6F_PARTY_OK = 0,
    PR_U6F_PARTY_ARGUMENT,   /* null argument or a world save that is not exactly 0x3351 bytes */
    PR_U6F_PARTY_COUNT,      /* count above eight (host bound; the original does not check) */
    PR_U6F_PARTY_NAME        /* a used slot's Pascal length exceeds 40 (host bound) */
} pr_u6f_party_status;

typedef struct {
    unsigned count;                                  /* world-save byte 0x3208 */
    uint8_t name_len[PR_U6F_PARTY_SLOTS];            /* Pascal length of slot k (valid for k < count) */
    const uint8_t *name_text[PR_U6F_PARTY_SLOTS];    /* borrowed text of slot k, into the caller's save */
} pr_u6f_party_plan;

/* Reads the count and the used basename slots. Slots at or after `count` are not examined. */
pr_u6f_party_status pr_u6f_party_plan_from_savgam(const uint8_t *savgam, size_t size, pr_u6f_party_plan *plan);

/* The built party. The nodes point into `record`, so the structure must not be copied or moved
 * after a build. `head` is the node chain for pr_u68_0236 and pr_u1c_op_party_strength. */
typedef struct {
    uint8_t record[PR_U6F_PARTY_SLOTS][PR_U6F_PARTY_RECORD_SIZE];
    pr_u68_0236_node node[PR_U6F_PARTY_SLOTS];
    const pr_u68_0236_node *head;                    /* NULL for an empty party */
    unsigned loaded;                                 /* present members; the window-1 counter value */
    uint8_t slot[PR_U6F_PARTY_SLOTS];                /* +0xBF of each loaded member in list order */
} pr_u6f_party;

/* `records[k]` is the 285-byte record for plan slot k (k < count), or NULL when its file is absent.
 * Entries at or after `count` are ignored. */
pr_u6f_party_status pr_u6f_party_build(const uint8_t *const records[PR_U6F_PARTY_SLOTS], unsigned count,
                                       pr_u6f_party *party);

#endif
