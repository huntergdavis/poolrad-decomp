#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_READER_ADAPTER_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_READER_ADAPTER_H

/* Host adapter between the bounded unit 0x6f entry 6 model (overlay_u6f_entry6_0ec4.h: hooks block_802_0716 and free_af8_0254) and the
 * merged resident block reader model pr_rec_res_0802_0716 (resident_ecl_dax_block.h, issue #74). It adds no game behaviour: it converts
 * the Pascal strings, calls the reader, borrows the reader-owned block as an object-table view and releases it exactly once.
 *
 * Load: the Pascal name and extension become NUL-terminated C strings (a length byte followed by that many bytes; an embedded NUL is
 * rejected because a C string cannot carry it; the reader itself truncates each to 20 characters as the resident routine does). The id is
 * passed as its low byte. Reader status mapping:
 *   OK                       -> return 0; *size = the returned size, *ptr = a unique synthetic far key naming a reserved object-table slot whose
 *                               view is the reader-owned block at exactly that size (no copy, no padding: an access past the size is VIEW in
 *                               the u6f model); the slot stays live until released
 *   MISSING_FILE / MISSING_ID -> return 0 with *size = 0 and *ptr = nil (the DOS routine writes zero outputs and returns normally)
 *   PACKED_LIMIT_ERROR       -> return PR_U6F_RA_STOP_PACKED_LIMIT (the DOS path prints a message and terminates; no normal return)
 *   any other reader status  -> return PR_U6F_RA_STOP_HOST_BASE + status (host-only: never mapped to the zero-size path)
 * Release (the FreeMem hook, called with the block pointer and size the load returned): exactly once, with the original size; an unknown
 * key, an already released slot, or a size mismatch returns PR_U6F_RA_STOP_FREE_MISMATCH without releasing anything. After release the slot's
 * view is dropped (segment NULL, size 0) so a later access through the stale key is VIEW, never a use of freed memory.
 * Teardown releases blocks still live when the model stopped before its FreeMem (host cleanup, not a claim that the DOS path freed them).
 *
 * Classification: host adapter, synthetic tests only. No game-function credit, no parity claim. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u25_entry5_0411.h"
#include "poolrad/recovered/overlay_u6f_entry6_0ec4.h"
#include "poolrad/recovered/resident_ecl_dax_block.h"

#define PR_U6F_RA_MAX_SLOTS 8u

enum {
    PR_U6F_RA_OK = 0,
    PR_U6F_RA_STOP_PACKED_LIMIT = 1,   /* the DOS routine would print a message and terminate */
    PR_U6F_RA_STOP_NO_SLOT = 2,        /* every reserved slot is live */
    PR_U6F_RA_STOP_STRING = 3,         /* a Pascal string with an embedded NUL, or a NULL string */
    PR_U6F_RA_STOP_FREE_MISMATCH = 4,  /* unknown key, double release, or size mismatch */
    PR_U6F_RA_STOP_ZERO_OK = 5,        /* the reader reported OK with size 0 (not a DOS outcome): not guessed */
    PR_U6F_RA_STOP_ARGUMENT = 6,
    PR_U6F_RA_STOP_HOST_BASE = 0x100   /* + the reader's status for every other non-OK status */
};

typedef struct {
    const uint8_t *block;      /* the reader-owned block (NULL when the slot is free) */
    uint16_t size;
    unsigned live;
    unsigned released;         /* released once (the slot may be reused after release) */
} pr_u6f_ra_slot;

typedef struct {
    pr_rec_res_ecl_dax_io *io;             /* the reader's I/O (also the reader's ctx) */
    pr_u25_e5_object *objects;             /* the reserved slots inside the host's object table (stable storage) */
    size_t slot_count;                     /* at most PR_U6F_RA_MAX_SLOTS */
    pr_u6f_ra_slot slots[PR_U6F_RA_MAX_SLOTS];
    uint16_t key_segment_base;             /* slot i has the far key key_segment_base + i : 0x0000 (a nonzero segment, offset 0) */
    unsigned loads, releases, teardown_releases;
    int last_reader_status;
    void *user;                            /* free for the host's other u6f hooks */
} pr_u6f_ra;

/* Reserves `slot_count` entries of the host table starting at `objects`.
 * The nonzero key range must not wrap 16 bits. The block hook also verifies
 * that each slot belongs to env.objects and no other entry shadows its key. */
int pr_u6f_ra_init(pr_u6f_ra *ra, pr_rec_res_ecl_dax_io *io, pr_u25_e5_object *objects, size_t slot_count, uint16_t key_segment_base);

/* Hook-shaped entry points: bind `env.hooks.block_802_0716 = pr_u6f_ra_block_hook` and `env.hooks.free_af8_0254 = pr_u6f_ra_free_hook`
 * with env.ctx = the pr_u6f_ra (or a host struct whose first member is it). */
int pr_u6f_ra_block_hook(void *ctx, const pr_u6f_e6_env *env, const uint8_t *name, const uint8_t *ext, uint8_t id, uint16_t *size, pr_far16 *ptr);
int pr_u6f_ra_free_hook(void *ctx, const pr_u6f_e6_env *env, pr_far16 ptr, uint16_t size);

/* Releases every still-live block; returns how many. */
unsigned pr_u6f_ra_teardown(pr_u6f_ra *ra);

#endif
