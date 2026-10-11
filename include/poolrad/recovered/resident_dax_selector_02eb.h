#ifndef POOLRAD_RECOVERED_RESIDENT_DAX_SELECTOR_02EB_H
#define POOLRAD_RECOVERED_RESIDENT_DAX_SELECTOR_02EB_H

#include <stdint.h>

/* Bounded model of RES_0802:02EB, loaded at image 0x0830B. */
typedef struct {
    uint8_t length;
    uint8_t bytes[255];
} pr_rec_dax_selector_02eb_string;

typedef struct {
    void *context;
    /* These model the private stack file record. Zero means the host
     * callback completed; DOS I/O failure is conveyed by io_result. */
    int (*assign_temp)(void *context,
                       const pr_rec_dax_selector_02eb_string *name);
    int (*reset_temp)(void *context, uint16_t record_size);
    int (*close_temp)(void *context);
    int (*io_result)(void *context, uint16_t *result);
    int (*store_ds_word)(void *context, uint16_t offset, uint16_t value);
} pr_rec_dax_selector_02eb_io;

typedef enum {
    PR_REC_DAX_SELECTOR_02EB_OK = 0,
    PR_REC_DAX_SELECTOR_02EB_ARGUMENT,
    PR_REC_DAX_SELECTOR_02EB_BOUNDARY
} pr_rec_dax_selector_02eb_status;

/* DOS frame: one far Pascal-string pointer at [BP+6], RETF 4. The input is
 * copied at a 40-byte cap before any file callback. All five callbacks run
 * in order on a completed path, including when IOResult is nonzero.
 * AL is one iff IOResult is zero and the capped name is nonempty; AH is the
 * high byte of the IOResult word. On host callback failure *ax is unchanged;
 * effects of earlier successful callbacks remain. */
pr_rec_dax_selector_02eb_status pr_rec_res_0802_02eb(
    const pr_rec_dax_selector_02eb_io *io,
    const pr_rec_dax_selector_02eb_string *name, uint16_t *ax);

#endif
