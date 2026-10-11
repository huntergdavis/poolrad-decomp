#ifndef POOLRAD_RECOVERED_RESIDENT_DAX_NESTED_OPEN_H
#define POOLRAD_RECOVERED_RESIDENT_DAX_NESTED_OPEN_H

#include <stddef.h>
#include <stdint.h>

/* Bounded model of START.EXE 0x0802:0x025D, loaded at image 0x0827D.
 * The file/runtime calls and the retry display are callback boundaries. */
typedef struct {
    uint8_t length;
    uint8_t bytes[255];
} pr_rec_dax_nested_string;

typedef struct {
    uint16_t segment;
    uint16_t offset;
} pr_rec_dax_nested_far;

typedef enum {
    PR_REC_DAX_NESTED_TEMP = 0,
    PR_REC_DAX_NESTED_FIRST = 1
} pr_rec_dax_nested_record;

typedef struct {
    void *ctx;
    /* Each callback returns zero on host success. A runtime I/O error is
     * instead supplied through io_result's word, so later DOS steps still
     * occur. FIRST receives the caller's far file-record address. TEMP is
     * the selector's private stack record; its far address is not exported. */
    int (*assign)(void *ctx, pr_rec_dax_nested_record record,
                  pr_rec_dax_nested_far first_record,
                  const pr_rec_dax_nested_string *name);
    int (*reset)(void *ctx, pr_rec_dax_nested_record record,
                 pr_rec_dax_nested_far first_record, uint16_t record_size);
    int (*close_temp)(void *ctx);
    int (*io_result)(void *ctx, uint16_t *result);
    int (*store_ds_word)(void *ctx, uint16_t offset, uint16_t value);
    /* The 0x081B3 display/input routine is unresolved. It receives the
     * copied extension and code 0x0E on a zero-mode failed attempt. */
    int (*retry_display)(void *ctx, uint8_t code,
                         const pr_rec_dax_nested_string *extension);
} pr_rec_dax_nested_io;

typedef enum {
    PR_REC_DAX_NESTED_OK = 0,
    PR_REC_DAX_NESTED_ARGUMENT,
    PR_REC_DAX_NESTED_BOUNDARY,
    PR_REC_DAX_NESTED_RETRY_LIMIT
} pr_rec_dax_nested_status;

/* DOS frame: name [BP+0x10], extension [BP+0x0C], mode word [BP+0x0A],
 * first file record [BP+0x06]; RETF 0x0E. Only the low mode byte is tested.
 * A complete zero-mode failed attempt displays a retry message and repeats;
 * max_attempts bounds that host model without inventing a DOS exit. The
 * selector at 0x0830B returns one iff IOResult is zero and name is nonempty.
 * On completed return AL is 0 or 1. Host callback failure or retry limit
 * leaves *al unchanged, with prior callback effects preserved. */
pr_rec_dax_nested_status pr_rec_res_0802_025d(
    const pr_rec_dax_nested_io *io,
    const pr_rec_dax_nested_string *name,
    const pr_rec_dax_nested_string *extension,
    uint16_t mode, pr_rec_dax_nested_far first_record,
    size_t max_attempts, uint8_t *al);

#endif
