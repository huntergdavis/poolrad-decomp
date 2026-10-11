#include "poolrad/recovered/resident_dax_retry_display.h"

#include <string.h>

pr_rec_dax_retry_status pr_rec_res_0802_0193(
    const pr_rec_dax_retry_io *io,
    const pr_rec_dax_nested_string *message,
    uint16_t code, uint8_t *al) {
    uint8_t local[40];
    uint8_t length;
    uint8_t display_al = 0;
    uint8_t key = 0;
    unsigned i;

    if (!io || !message || !al || !io->display_command || !io->read_key)
        return PR_REC_DAX_RETRY_ARGUMENT;

    /* 0x081B9..0x081C7: capped Pascal copy into the 41-byte stack slot. */
    length = message->length < 40 ? message->length : 40;
    memcpy(local, message->bytes, length);

    /* 0x081CC..0x081D9: rectangle fill through 0x08195 -> 0x08060. */
    if (io->display_command(io->ctx, 0, 24, 0, 40, 0xDB, &display_al))
        return PR_REC_DAX_RETRY_BOUNDARY;

    /* 0x081DC..0x081EC: 0x08125 copies the already capped string again
     * and emits one 0x08060 command per byte, incrementing column in an
     * eight-bit slot. The caller supplies its code in a word-sized slot. */
    for (i = 0; i < length; ++i) {
        if (io->display_command(io->ctx, (uint8_t)i, 24, (uint8_t)code,
                                1, local[i], &display_al))
            return PR_REC_DAX_RETRY_BOUNDARY;
    }

    /* 0x081EF..0x081F4: the key byte is saved locally but never read. */
    if (io->read_key(io->ctx, &key)) return PR_REC_DAX_RETRY_BOUNDARY;

    /* 0x081F7..0x0820A: second fill and RETF 6. The final 0x08060 call
     * supplies the returned AL, irrespective of the key byte. */
    if (io->display_command(io->ctx, 0, 24, 0, 40, 0xDB, &display_al))
        return PR_REC_DAX_RETRY_BOUNDARY;
    *al = display_al;
    return PR_REC_DAX_RETRY_OK;
}
