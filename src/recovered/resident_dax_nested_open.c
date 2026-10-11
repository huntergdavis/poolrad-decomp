#include "poolrad/recovered/resident_dax_nested_open.h"

#include <string.h>

static void copy_capped(pr_rec_dax_nested_string *to,
                        const pr_rec_dax_nested_string *from) {
    to->length = from->length < 40 ? from->length : 40;
    memcpy(to->bytes, from->bytes, to->length);
}

pr_rec_dax_nested_status pr_rec_res_0802_025d(
    const pr_rec_dax_nested_io *io,
    const pr_rec_dax_nested_string *name,
    const pr_rec_dax_nested_string *extension,
    uint16_t mode, pr_rec_dax_nested_far first_record,
    size_t max_attempts, uint8_t *al) {
    pr_rec_dax_nested_string local_name = {0}, local_extension = {0};
    size_t attempt;
    if (!io || !name || !extension || !al || !max_attempts || !io->assign ||
        !io->reset || !io->close_temp || !io->io_result ||
        !io->store_ds_word || !io->retry_display)
        return PR_REC_DAX_NESTED_ARGUMENT;

    /* 0x08283..0x082A4: two calls to the Pascal copy with a 0x28 cap. */
    copy_capped(&local_name, name);
    copy_capped(&local_extension, extension);
    for (attempt = 0; attempt < max_attempts; ++attempt) {
        pr_rec_dax_nested_string selector_name = {0};
        pr_rec_dax_nested_far temp_record = {0, 0};
        uint16_t result;
        uint8_t opened;
        /* 0x0830B selector copies the first string again, then performs
         * Assign, Reset(0x80), Close, IOResult, and DS:0x49CE store. */
        copy_capped(&selector_name, &local_name);
        if (io->assign(io->ctx, PR_REC_DAX_NESTED_TEMP, temp_record,
                       &selector_name) ||
            io->reset(io->ctx, PR_REC_DAX_NESTED_TEMP, temp_record, 0x80) ||
            io->close_temp(io->ctx) ||
            io->io_result(io->ctx, &result) ||
            io->store_ds_word(io->ctx, 0x49CE, result))
            return PR_REC_DAX_NESTED_BOUNDARY;
        opened = (uint8_t)(result == 0 && selector_name.length != 0);
        if (!opened && (uint8_t)mode == 0) {
            /* 0x082C1 calls 0x081B3 with code 0x0E, then 0x082D7
             * branches back to the selector. The display/input side
             * effects remain a single explicit callback boundary. */
            if (io->retry_display(io->ctx, 0x0E, &local_extension))
                return PR_REC_DAX_NESTED_BOUNDARY;
            continue;
        }
        if (opened &&
            (io->assign(io->ctx, PR_REC_DAX_NESTED_FIRST, first_record,
                        &local_name) ||
             io->reset(io->ctx, PR_REC_DAX_NESTED_FIRST, first_record, 1)))
            return PR_REC_DAX_NESTED_BOUNDARY;
        *al = opened;
        return PR_REC_DAX_NESTED_OK;
    }
    return PR_REC_DAX_NESTED_RETRY_LIMIT;
}
