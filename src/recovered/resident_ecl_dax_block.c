#include "poolrad/recovered/resident_ecl_dax_block.h"

#include <stdlib.h>
#include <string.h>

static uint16_t read_word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_dword(const uint8_t *p) {
    return (uint32_t)read_word(p) | ((uint32_t)read_word(p + 2) << 16);
}

static void copy_name(char out[PR_REC_RES_ECL_DAX_NAME_MAX + 1],
                      const char *input) {
    size_t i;
    for (i = 0; i < PR_REC_RES_ECL_DAX_NAME_MAX && input[i]; ++i)
        out[i] = input[i];
    out[i] = '\0';
}

static void *allocate(const pr_rec_res_ecl_dax_io *io, size_t size) {
    return io->allocate ? io->allocate(io->ctx, size) : malloc(size);
}

static void deallocate(const pr_rec_res_ecl_dax_io *io, void *ptr, size_t size) {
    if (!ptr) return;
    if (io->deallocate) io->deallocate(io->ctx, ptr, size);
    else free(ptr);
}

/* Image 0x0852F..0x085E8. The DOS routine neither checks output capacity nor
 * advances on control 0x7F/0x80. This host model rejects those cases. */
static int decode(const uint8_t *packed, size_t packed_size,
                  uint8_t *output, size_t output_size) {
    size_t source = 0, target = 0;
    while (source < packed_size) {
        uint8_t control = packed[source];
        size_t count;
        if (control == 0x7F || control == 0x80)
            return PR_REC_RES_ECL_DAX_RLE_STALL;
        if (control <= 0x7E) {
            count = (size_t)control + 1;
            if (count > packed_size - source - 1 || count > output_size - target)
                return PR_REC_RES_ECL_DAX_RLE;
            memcpy(output + target, packed + source + 1, count);
            source += count + 1;
        } else {
            count = 256u - (size_t)control;
            if (packed_size - source < 2 || count > output_size - target)
                return PR_REC_RES_ECL_DAX_RLE;
            memset(output + target, packed[source + 1], count);
            source += 2;
        }
        target += count;
    }
    return target == output_size ? PR_REC_RES_ECL_DAX_OK : PR_REC_RES_ECL_DAX_RLE;
}

int pr_rec_res_0802_0716(void *ctx, const char *name, const char *ext,
                         uint8_t block_id, uint16_t *out_size,
                         const uint8_t **out_block) {
    const pr_rec_res_ecl_dax_io *io = ctx;
    const uint8_t *file = NULL;
    size_t file_size = 0, header, count, i, packed_start, rounded = 0;
    uint16_t unpacked = 0, packed_size = 0;
    uint32_t offset = 0;
    uint8_t *packed = NULL, *output = NULL;
    char local_name[PR_REC_RES_ECL_DAX_NAME_MAX + 1];
    char local_ext[PR_REC_RES_ECL_DAX_NAME_MAX + 1];
    int status = PR_REC_RES_ECL_DAX_OK;

    if (!io || !name || !ext || !out_size || !out_block || !io->open_archive ||
        (!!io->allocate != !!io->deallocate))
        return PR_REC_RES_ECL_DAX_ARGUMENT;
    *out_size = 0;
    *out_block = NULL;
    copy_name(local_name, name);
    copy_name(local_ext, ext);
    if (io->open_archive(io->ctx, local_name, local_ext, &file, &file_size))
        return PR_REC_RES_ECL_DAX_MISSING_FILE;

    if (!file || file_size < 11 || file_size > PR_REC_RES_ECL_DAX_FILE_LIMIT) {
        status = PR_REC_RES_ECL_DAX_INDEX;
        goto close_file;
    }
    header = read_word(file);
    if (!header || header % 9 || header / 9 > 256 || header + 2 > file_size) {
        status = PR_REC_RES_ECL_DAX_INDEX;
        goto close_file;
    }
    count = header / 9;
    for (i = 0; i < count; ++i) {
        const uint8_t *entry = file + 2 + i * 9;
        packed_size = read_word(entry + 7);
        /* Image 0x0882B: this check precedes the ID comparison. */
        if (packed_size > PR_REC_RES_ECL_DAX_PACKED_LIMIT) {
            status = PR_REC_RES_ECL_DAX_PACKED_LIMIT_ERROR;
            goto close_file;
        }
        if (entry[0] == block_id) {
            offset = read_dword(entry + 1);
            unpacked = read_word(entry + 5);
            break;
        }
    }
    if (i == count) {
        status = PR_REC_RES_ECL_DAX_MISSING_ID;
        goto close_file;
    }
    packed_start = header + 2;
    if (!packed_size || !unpacked || offset > file_size - packed_start ||
        packed_size > file_size - packed_start - offset) {
        status = PR_REC_RES_ECL_DAX_RANGE;
        goto close_file;
    }
    packed_start += offset;
    /* Image 0x08040 rounds the packed allocation up to a multiple of eight. */
    rounded = ((size_t)packed_size + 7u) & ~(size_t)7u;
    packed = allocate(io, rounded);
    if (!packed) {
        status = PR_REC_RES_ECL_DAX_ALLOC;
        goto close_file;
    }
    output = allocate(io, unpacked);
    if (!output) {
        status = PR_REC_RES_ECL_DAX_ALLOC;
        goto close_file;
    }
    memcpy(packed, file + packed_start, packed_size);
    /* The DOS path closes both file records at 0x0893D before decoding. */
    if (io->close_archive) io->close_archive(io->ctx);
    status = decode(packed, packed_size, output, unpacked);
    deallocate(io, packed, rounded);
    if (status != PR_REC_RES_ECL_DAX_OK) {
        deallocate(io, output, unpacked);
        return status;
    }
    *out_size = unpacked;
    *out_block = output;
    return PR_REC_RES_ECL_DAX_OK;

close_file:
    if (io->close_archive) io->close_archive(io->ctx);
    deallocate(io, output, unpacked);
    deallocate(io, packed, rounded);
    return status;
}

void pr_rec_res_0802_0716_release(void *ctx, const uint8_t *block,
                                  uint16_t size) {
    const pr_rec_res_ecl_dax_io *io = ctx;
    if (!block) return;
    if (io && io->deallocate) io->deallocate(io->ctx, (void *)block, size);
    else free((void *)block);
}
