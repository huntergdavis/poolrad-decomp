#ifndef POOLRAD_RECOVERED_RESIDENT_DAX_RLE_H
#define POOLRAD_RECOVERED_RESIDENT_DAX_RLE_H

#include <stddef.h>
#include <stdint.h>

/* Loaded START.EXE image 0x0852F, resident 0x0802:0x050F.
 * This is a bounded model of the complete DOS body, including its two
 * decoded runtime callees. It exposes the nonterminating and invalid-memory
 * cases as host statuses; these are not DOS return values. */
typedef struct {
    uint16_t segment;
    uint16_t offset;
} pr_rec_dax_far;

typedef struct {
    void *ctx;
    /* The callbacks address real-mode segment:offset memory. They must be
     * deterministic passive reads/writes; offset arithmetic wraps at 16 bits.
     * A nonzero result is a host memory fault, not a DOS error path. */
    int (*read8)(void *ctx, pr_rec_dax_far address, uint8_t *value);
    int (*write8)(void *ctx, pr_rec_dax_far address, uint8_t value);
} pr_rec_dax_memory;

typedef struct {
    /* All are far addresses of variables, not the contained buffer pointers.
     * DOS frame: source [BP+0x12], destination [BP+0x0E], packed size
     * [BP+0x0A], unused output size [BP+0x06]; RETF 0x10. */
    pr_rec_dax_far source_pointer;
    pr_rec_dax_far destination_pointer;
    pr_rec_dax_far packed_size;
    pr_rec_dax_far unused_output_size;
} pr_rec_dax_rle_args;

typedef struct {
    uint16_t source_index;
    uint16_t destination_index;
    size_t iterations;
    /* On entry: caller's DF value (0 or 1). On exit: DF left by the last
     * runtime Move/FillChar call, or unchanged when neither was called. */
    uint8_t direction_flag;
} pr_rec_dax_rle_state;

typedef enum {
    PR_REC_DAX_RLE_OK = 0,
    PR_REC_DAX_RLE_ARGUMENT,
    PR_REC_DAX_RLE_MEMORY,
    PR_REC_DAX_RLE_STALLED,
    PR_REC_DAX_RLE_ITERATION_LIMIT
} pr_rec_dax_rle_status;

/* The DOS body reads a first control before checking packed_size. It checks
 * the packed-size word after each iteration, and never reads output size.
 * max_iterations is a host bound on cyclic/mutable input; use at least 65536
 * for ordinary fixed-size DAX blocks. */
pr_rec_dax_rle_status pr_rec_res_0802_050f(
    const pr_rec_dax_memory *memory, const pr_rec_dax_rle_args *args,
    size_t max_iterations, pr_rec_dax_rle_state *state);

#endif
