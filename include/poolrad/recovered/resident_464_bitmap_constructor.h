#ifndef POOLRAD_RECOVERED_RESIDENT_464_BITMAP_CONSTRUCTOR_H
#define POOLRAD_RECOVERED_RESIDENT_464_BITMAP_CONSTRUCTOR_H

#include <stdint.h>

#include "poolrad/recovered/resident_464_block_wrapper.h"

/* Bounded host model of RES_0464:248E; see the address-only evidence note.
 * The object type matches the merged RES_0464:0111 make_object hook. */
typedef enum {
    PR_R464_CTOR_OK = 0,
    PR_R464_CTOR_ARGUMENT,
    PR_R464_CTOR_HOOK,
    PR_R464_CTOR_CALLBACK,
    PR_R464_CTOR_BOUNDS
} pr_r464_ctor_status;

typedef enum {
    PR_R464_CTOR_READ_VIDEO_MAIN = 1,
    PR_R464_CTOR_ALLOC_MAIN,
    PR_R464_CTOR_FILL_MAIN,
    PR_R464_CTOR_READ_VIDEO_AUX,
    PR_R464_CTOR_ALLOC_AUX,
    PR_R464_CTOR_ATTACH_AUX,
    PR_R464_CTOR_FILL_AUX
} pr_r464_ctor_call;

typedef struct {
    void *ctx;
    /* Reads DS byte [0x4A1E] at each reached instruction site. */
    int (*read_video)(void *ctx, uint8_t *value);
    /* Models runtime GetMem. On success, *out has writable bytes and
     * capacity at least the requested allocation size. */
    int (*allocate)(void *ctx, uint16_t size, pr_r464_block_object **out);
    /* Models runtime FillChar(object, size, 0). */
    int (*fill_zero)(void *ctx, pr_r464_block_object *object,
                     uint16_t size);
    /* Maps the auxiliary host object to the DOS far-pointer field +0x13.
     * The model does not cast a host pointer into four DOS bytes. */
    int (*attach_aux)(void *ctx, pr_r464_block_object *primary,
                      pr_r464_block_object *auxiliary);
} pr_r464_ctor_env;

typedef struct {
    pr_r464_ctor_call calls[7];
    unsigned call_count;
    pr_r464_ctor_call stopped_at;
    int callback_status;
    uint8_t video_main, video_aux;
    uint16_t span_word, data_size, primary_size, auxiliary_size;
    /* Host retains ownership of allocations after any stop. */
    pr_r464_block_object *primary, *auxiliary;
} pr_r464_ctor_report;

/* DOS frame: far destination slot [BP+6], raw flag byte [BP+0x0A],
 * raw multiplicity byte [BP+0x0C], WORDs [BP+0x0E] and [BP+0x10],
 * RETF 0x0C. The wrapper passes block WORDs at +0/+2, byte +8, and its
 * [BP+0x0C] byte respectively. All arithmetic truncates at 16 bits like
 * the decoded AX operations. Bit 0 of arg_a enables the auxiliary branch.
 * Callbacks are required only when reached; prior allocations/writes stand
 * if a later host hook fails. */
pr_r464_ctor_status pr_r464_bitmap_construct(
    const pr_r464_ctor_env *env, uint16_t word_0, uint16_t word_2,
    uint8_t byte_8, uint8_t arg_a, pr_r464_block_object **destination,
    pr_r464_ctor_report *report);

#endif
