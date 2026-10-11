#ifndef POOLRAD_FORMATS_DAX_H
#define POOLRAD_FORMATS_DAX_H

#include <stddef.h>
#include <stdint.h>

/* Format/RLE adapted from Gold Box Explorer, Copyright (c) 2018 Bil Simser.
 * MIT notice: docs/formats/GoldBoxExplorer-MIT.txt. */
#define PR_DAX_MAX_FILE (1024u * 1024u)
#define PR_DAX_MAX_BLOCKS 256u

typedef enum {
    PR_DAX_OK = 0,
    PR_DAX_ARGUMENT,
    PR_DAX_SIZE,
    PR_DAX_INDEX,
    PR_DAX_DUPLICATE_ID,
    PR_DAX_RANGE,
    PR_DAX_RLE,
    PR_DAX_OUTPUT_SIZE
} pr_dax_status;

typedef struct {
    uint8_t id;
    size_t offset;
    size_t stored_size;
    size_t unpacked_size;
} pr_dax_entry;

/* Borrows data. Keep it alive and unchanged while reading blocks. */
typedef struct {
    const uint8_t *data;
    size_t size;
    size_t count;
    pr_dax_entry entries[PR_DAX_MAX_BLOCKS];
} pr_dax_archive;

pr_dax_status pr_dax_open(const uint8_t *data, size_t size, pr_dax_archive *out);
/* index is a directory position, not a block ID. On OUTPUT_SIZE, written is
 * the required capacity. Output must not overlap the borrowed archive data. */
pr_dax_status pr_dax_read(const pr_dax_archive *archive, size_t index,
                          uint8_t *out, size_t capacity, size_t *written);
pr_dax_status pr_dax_rle(const uint8_t *packed, size_t packed_size,
                         uint8_t *out, size_t expected_size);
const char *pr_dax_status_string(pr_dax_status status);

#endif
