#ifndef POOLRAD_RECOVERED_RESIDENT_DAX_RETRY_DISPLAY_H
#define POOLRAD_RECOVERED_RESIDENT_DAX_RETRY_DISPLAY_H

#include <stdint.h>

#include "poolrad/recovered/resident_dax_nested_open.h"

/* Bounded model of RES_0802:0193 at loaded image 0x081B3.
 * display_command stands at the complete 0x08060 renderer and its global
 * effects. read_key stands at 0x0899:0059 and its runtime effects. A zero
 * callback result means the host boundary completed; the DOS AL value from
 * the final display command is supplied separately. */
typedef struct {
    void *ctx;
    int (*display_command)(void *ctx, uint8_t column, uint8_t row,
                           uint8_t code, uint8_t count, uint8_t glyph,
                           uint8_t *al);
    int (*read_key)(void *ctx, uint8_t *key);
} pr_rec_dax_retry_io;

typedef enum {
    PR_REC_DAX_RETRY_OK = 0,
    PR_REC_DAX_RETRY_ARGUMENT,
    PR_REC_DAX_RETRY_BOUNDARY
} pr_rec_dax_retry_status;

/* DOS frame: far Pascal string at [BP+6], code word at [BP+0xA],
 * RETF 6. Only the low code byte is used. The string is capped to 40 bytes.
 * One fill command, one command per copied byte, one key read, and a final
 * fill command occur in that order. The key byte is discarded. On success
 * *al receives AL from the final display command. On host callback failure
 * *al remains unchanged and any earlier callback effects remain. */
pr_rec_dax_retry_status pr_rec_res_0802_0193(
    const pr_rec_dax_retry_io *io,
    const pr_rec_dax_nested_string *message,
    uint16_t code, uint8_t *al);

#endif
