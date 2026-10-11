#include "poolrad/recovered/resident_dax_file_open.h"

#include <stddef.h>
#include <string.h>

static void copy_at_most_40(pr_rec_dax_open_string *target,
                            const pr_rec_dax_open_string *source)
{
    target->length = source->length < 40u ? source->length : 40u;
    memcpy(target->bytes, source->bytes, target->length);
}

/* 0xAF8:0x0B98 appends only the bytes that fit a 255-byte Pascal string. */
static void append(pr_rec_dax_open_string *target,
                   const pr_rec_dax_open_string *source)
{
    unsigned available = 255u - target->length;
    unsigned count = source->length < available ? source->length : available;
    memcpy(target->bytes + target->length, source->bytes, count);
    target->length = (uint8_t)(target->length + count);
}

pr_rec_dax_open_status pr_rec_res_0802_05cd(
    const pr_rec_dax_open_io *io,
    const pr_rec_dax_open_string *base_path,
    const pr_rec_dax_open_string *extension,
    uint16_t *header_word, uint8_t *al)
{
    pr_rec_dax_open_string name_ext, part, configured, one;
    uint8_t selector, character, suffix_byte, mode, open_al;
    uint16_t ignored_result = 0, actual_bytes = 0;

    if (io == NULL || base_path == NULL || extension == NULL ||
        header_word == NULL || al == NULL ||
        io->read_ds_byte == NULL || io->read_table_string == NULL ||
        io->read_code_byte == NULL || io->open_configured == NULL ||
        io->assign_second == NULL || io->reset_second == NULL ||
        io->io_result == NULL || io->read_second == NULL ||
        io->seek_first == NULL)
        return PR_REC_DAX_OPEN_ARGUMENT;

    /* 0x085F4..0x0862F: two capped copies and a retained-destination
     * assignment/append form the first name plus extension. */
    copy_at_most_40(&name_ext, base_path);
    copy_at_most_40(&part, extension);
    append(&name_ext, &part);

    /* 0x0863A..0x0866A: one 30-byte table entry is selected by a byte,
     * then a single global byte and a CS Pascal literal are appended. */
    if (!io->read_ds_byte(io->context, 0x5376u, &selector) ||
        !io->read_table_string(io->context,
                               (uint16_t)(0x2614u + 0x1eu * selector),
                               &configured) ||
        !io->read_ds_byte(io->context, 0x547du, &character))
        return PR_REC_DAX_OPEN_BOUNDARY;
    one.length = 1;
    one.bytes[0] = character;
    append(&configured, &one);
    if (!io->read_code_byte(io->context, 0x05ccu, &suffix_byte))
        return PR_REC_DAX_OPEN_BOUNDARY;
    one.bytes[0] = suffix_byte;
    append(&configured, &one);

    /* 0x0866F..0x0867E: the local callee receives both strings, the mode
     * byte, and the first file record. Its internal behavior is a boundary. */
    if (!io->read_ds_byte(io->context, 0x557cu, &mode) ||
        !io->open_configured(io->context, &name_ext, &configured,
                             mode, &open_al))
        return PR_REC_DAX_OPEN_BOUNDARY;
    if (open_al == 0) {
        *al = 0;
        return PR_REC_DAX_OPEN_OK;
    }

    /* 0x08680..0x0869F: the original rereads both source strings after
     * the local call and builds the second file record's name. */
    copy_at_most_40(&name_ext, base_path);
    copy_at_most_40(&part, extension);
    append(&name_ext, &part);
    if (!io->assign_second(io->context, &name_ext) ||
        !io->reset_second(io->context, 1u) ||
        !io->io_result(io->context, &ignored_result) ||
        !io->read_second(io->context, header_word, 2u, &actual_bytes) ||
        !io->io_result(io->context, &ignored_result))
        return PR_REC_DAX_OPEN_BOUNDARY;

    /* 0x086D4..0x086EF: add two in the caller's word, seek the first
     * record using a zero-extended 32-bit position, and ignore IOResult. */
    *header_word = (uint16_t)(*header_word + 2u);
    if (!io->seek_first(io->context, *header_word) ||
        !io->io_result(io->context, &ignored_result))
        return PR_REC_DAX_OPEN_BOUNDARY;

    *al = 1;
    return PR_REC_DAX_OPEN_OK;
}
