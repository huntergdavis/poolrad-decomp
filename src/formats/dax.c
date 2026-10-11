#include "poolrad/formats/dax.h"

#include <string.h>

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t dword(const uint8_t *p) {
    return (uint32_t)word(p) | ((uint32_t)word(p + 2) << 16);
}

pr_dax_status pr_dax_open(const uint8_t *data, size_t size, pr_dax_archive *out) {
    size_t header, start, count, i;
    uint8_t seen[256] = {0};
    pr_dax_archive result = {0};
    if (!data || !out) return PR_DAX_ARGUMENT;
    if (size < 11 || size > PR_DAX_MAX_FILE) return PR_DAX_SIZE;
    header = word(data);
    start = header + 2;
    if (!header || header % 9 || header / 9 > PR_DAX_MAX_BLOCKS || start > size)
        return PR_DAX_INDEX;
    count = header / 9;
    result.data = data;
    result.size = size;
    result.count = count;
    for (i = 0; i < count; ++i) {
        const uint8_t *p = data + 2 + i * 9;
        size_t offset = (size_t)dword(p + 1);
        size_t stored = word(p + 7);
        uint8_t id = p[0];
        if (seen[id]) return PR_DAX_DUPLICATE_ID;
        seen[id] = 1;
        if (!stored || offset > size - start || stored > size - start - offset)
            return PR_DAX_RANGE;
        result.entries[i].id = id;
        result.entries[i].offset = start + offset;
        result.entries[i].stored_size = stored;
        result.entries[i].unpacked_size = word(p + 5) ? word(p + 5) : stored;
    }
    *out = result;
    return PR_DAX_OK;
}

pr_dax_status pr_dax_rle(const uint8_t *packed, size_t packed_size,
                         uint8_t *out, size_t expected_size) {
    size_t source = 0, target = 0;
    if ((!packed && packed_size) || (!out && expected_size)) return PR_DAX_ARGUMENT;
    while (source < packed_size) {
        uint8_t control = packed[source++];
        size_t count = control < 128 ? (size_t)control + 1 : 256u - (size_t)control;
        if (count > expected_size - target) return PR_DAX_RLE;
        if (control < 128) {
            if (count > packed_size - source) return PR_DAX_RLE;
            memcpy(out + target, packed + source, count);
            source += count;
        } else {
            if (source == packed_size) return PR_DAX_RLE;
            memset(out + target, packed[source++], count);
        }
        target += count;
    }
    return target == expected_size ? PR_DAX_OK : PR_DAX_RLE;
}

pr_dax_status pr_dax_read(const pr_dax_archive *archive, size_t index,
                          uint8_t *out, size_t capacity, size_t *written) {
    const pr_dax_entry *entry;
    const uint8_t *packed;
    uint16_t encoded_size;
    if (!archive || !written || !archive->data) return PR_DAX_ARGUMENT;
    if (archive->count == 0 || archive->count > PR_DAX_MAX_BLOCKS ||
        archive->size < 2 + archive->count * 9 ||
        archive->size > PR_DAX_MAX_FILE) return PR_DAX_INDEX;
    if (index >= archive->count) return PR_DAX_INDEX;
    entry = &archive->entries[index];
    if (!entry->stored_size || entry->offset < 2 + archive->count * 9 ||
        entry->offset > archive->size ||
        entry->stored_size > archive->size - entry->offset) return PR_DAX_RANGE;
    encoded_size = word(archive->data + 2 + index * 9 + 5);
    if (entry->unpacked_size != (encoded_size ? encoded_size : entry->stored_size))
        return PR_DAX_INDEX;
    *written = entry->unpacked_size;
    if (!out || capacity < entry->unpacked_size) return PR_DAX_OUTPUT_SIZE;
    packed = archive->data + entry->offset;
    if (!encoded_size) {
        memcpy(out, packed, entry->stored_size);
        return PR_DAX_OK;
    }
    return pr_dax_rle(packed, entry->stored_size, out, entry->unpacked_size);
}

const char *pr_dax_status_string(pr_dax_status status) {
    switch (status) {
    case PR_DAX_OK: return "ok";
    case PR_DAX_ARGUMENT: return "invalid argument";
    case PR_DAX_SIZE: return "archive size out of range";
    case PR_DAX_INDEX: return "invalid directory";
    case PR_DAX_DUPLICATE_ID: return "duplicate block id";
    case PR_DAX_RANGE: return "block range outside archive";
    case PR_DAX_RLE: return "invalid RLE stream";
    case PR_DAX_OUTPUT_SIZE: return "output buffer too small";
    default: return "unknown error";
    }
}
