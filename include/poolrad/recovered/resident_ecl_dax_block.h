#ifndef POOLRAD_RECOVERED_RESIDENT_ECL_DAX_BLOCK_H
#define POOLRAD_RECOVERED_RESIDENT_ECL_DAX_BLOCK_H

#include <stddef.h>
#include <stdint.h>

/* Bounded model of DOS START.EXE RES_0802_0716, not a full translation.
 * The file-open helper and Pascal runtime operations remain callback boundaries.
 * The public load/release signatures fit the overlay block-load callbacks in
 * PR #72 without editing the overlay implementation. */
#define PR_REC_RES_ECL_DAX_NAME_MAX 20u
#define PR_REC_RES_ECL_DAX_PACKED_LIMIT 0x8CA0u
#define PR_REC_RES_ECL_DAX_FILE_LIMIT (1024u * 1024u)

typedef enum {
    PR_REC_RES_ECL_DAX_OK = 0,
    PR_REC_RES_ECL_DAX_ARGUMENT,
    PR_REC_RES_ECL_DAX_MISSING_FILE,
    PR_REC_RES_ECL_DAX_MISSING_ID,
    PR_REC_RES_ECL_DAX_INDEX,
    PR_REC_RES_ECL_DAX_PACKED_LIMIT_ERROR,
    PR_REC_RES_ECL_DAX_RANGE,
    PR_REC_RES_ECL_DAX_ALLOC,
    PR_REC_RES_ECL_DAX_RLE_STALL,
    PR_REC_RES_ECL_DAX_RLE
} pr_rec_res_ecl_dax_status;

typedef struct {
    void *ctx;
    /* The DOS helper at image 0x085ED also uses DS configuration state and
     * opens two runtime file records. This callback supplies one borrowed
     * archive view instead. A nonzero return means the file was not opened.
     * name and ext are each truncated to 20 characters as at image 0x0874B
     * and 0x0875E; both are NUL terminated for the host callback. */
    int (*open_archive)(void *ctx, const char *name, const char *ext,
                        const uint8_t **data, size_t *size);
    /* Called once after a successful open, including early error paths. */
    void (*close_archive)(void *ctx);
    /* Both callbacks must be present together, or both absent for malloc/free.
     * The size supplied to release is the DOS allocation/free size. */
    void *(*allocate)(void *ctx, size_t size);
    void (*deallocate)(void *ctx, void *ptr, size_t size);
} pr_rec_res_ecl_dax_io;

/* DOS frame: far name [BP+14], far extension [BP+10], byte id [BP+0E],
 * far out-size address [BP+0A], far out-pointer address [BP+06]; RETF 0x12.
 * On a valid call, outputs are zero/null on every non-OK status. Unlike DOS,
 * malformed input, stalled RLE, and allocation failure return host statuses.
 * The caller owns an OK block and releases it with the function below. */
int pr_rec_res_0802_0716(void *ctx, const char *name, const char *ext,
                         uint8_t block_id, uint16_t *out_size,
                         const uint8_t **out_block);

/* Fits PR #72's release callback; size must equal the returned out_size. */
void pr_rec_res_0802_0716_release(void *ctx, const uint8_t *block,
                                  uint16_t size);

#endif
