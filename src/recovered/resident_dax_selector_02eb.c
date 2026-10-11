#include "poolrad/recovered/resident_dax_selector_02eb.h"

#include <string.h>

pr_rec_dax_selector_02eb_status pr_rec_res_0802_02eb(
    const pr_rec_dax_selector_02eb_io *io,
    const pr_rec_dax_selector_02eb_string *name, uint16_t *ax)
{
    pr_rec_dax_selector_02eb_string local = {0};
    uint16_t result;
    uint8_t al;

    if (io == NULL || name == NULL || ax == NULL ||
        io->assign_temp == NULL || io->reset_temp == NULL ||
        io->close_temp == NULL || io->io_result == NULL ||
        io->store_ds_word == NULL)
        return PR_REC_DAX_SELECTOR_02EB_ARGUMENT;

    /* 0x08312..0x08320: copy the far input into [BP-0x2A] at cap 0x28. */
    local.length = name->length < 40u ? name->length : 40u;
    memcpy(local.bytes, name->bytes, local.length);

    /* 0x08325..0x08354: the private stack record is assigned, reset at
     * record size 0x80, closed, and followed by IOResult and a DS write. */
    if (io->assign_temp(io->context, &local) ||
        io->reset_temp(io->context, 0x80u) ||
        io->close_temp(io->context) ||
        io->io_result(io->context, &result) ||
        io->store_ds_word(io->context, 0x49ceu, result))
        return PR_REC_DAX_SELECTOR_02EB_BOUNDARY;

    /* 0x08357..0x08373: both branches write AL only. The IOResult high
     * byte remains in AH through the final RETF 4. */
    al = (uint8_t)(result == 0u && local.length != 0u);
    *ax = (uint16_t)((result & 0xff00u) | al);
    return PR_REC_DAX_SELECTOR_02EB_OK;
}
