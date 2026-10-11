#include "poolrad/formats/dax.h"
#include "poolrad/formats/ega.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t archives, blocks, image_blocks, unsupported_blocks;
    size_t items, decoded_pixels, dimension_checks;
} totals;

static const char *base_name(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int expected_dimension(const char *name, const pr_ega_block *block) {
    size_t width = 0, height = 0;
    if (strncmp(name, "8X8D", 4) == 0) { width = 8; height = 8; }
    else if (strncmp(name, "BODY", 4) == 0) { width = 88; height = 48; }
    else if (strncmp(name, "HEAD", 4) == 0) { width = 88; height = 40; }
    else if (strncmp(name, "TITLE", 5) == 0) { width = 320; height = 200; }
    else return 0;
    return block->width_pixels == width && block->height == height ? 1 : -1;
}

static int audit_one(const char *path, totals *sum) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL, *decoded = NULL, *pixels = NULL;
    pr_dax_archive archive;
    long length;
    size_t i;
    int ok = 0;
    if (!file) { perror(path); return -1; }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        (unsigned long)length > PR_DAX_MAX_FILE || fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: invalid file size\n", path);
        goto done;
    }
    bytes = malloc((size_t)length ? (size_t)length : 1);
    decoded = malloc(PR_EGA_MAX_BLOCK);
    pixels = malloc(PR_EGA_MAX_WIDTH * PR_EGA_MAX_HEIGHT);
    if (!bytes || !decoded || !pixels ||
        fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "%s: read or allocation failure\n", path);
        goto done;
    }
    {
        pr_dax_status status = pr_dax_open(bytes, (size_t)length, &archive);
        if (status != PR_DAX_OK) {
            fprintf(stderr, "%s: %s\n", path, pr_dax_status_string(status));
            goto done;
        }
    }
    for (i = 0; i < archive.count; ++i) {
        size_t size = 0, item;
        pr_ega_block block;
        pr_dax_status dax = pr_dax_read(&archive, i, decoded,
                                        PR_EGA_MAX_BLOCK, &size);
        pr_ega_status ega;
        if (dax != PR_DAX_OK) {
            fprintf(stderr, "%s: block %zu: %s\n", path, i,
                    pr_dax_status_string(dax));
            goto done;
        }
        ++sum->blocks;
        /* The audited tile font is a different format, not a bad EGA image. */
        if (strncmp(base_name(path), "8X8D", 4) == 0 &&
            archive.entries[i].id == 201) {
            ++sum->unsupported_blocks;
            continue;
        }
        ega = pr_ega_open(decoded, size, &block);
        if (ega != PR_EGA_OK) {
            fprintf(stderr, "%s: block %zu: %s\n", path, i,
                    pr_ega_status_string(ega));
            goto done;
        }
        {
            int check = expected_dimension(base_name(path), &block);
            if (check < 0) {
                fprintf(stderr, "%s: block %zu: unexpected dimensions\n", path, i);
                goto done;
            }
            if (check > 0) ++sum->dimension_checks;
        }
        ++sum->image_blocks;
        sum->items += block.item_count;
        for (item = 0; item < block.item_count; ++item) {
            size_t written = 0, pixel;
            if (pr_ega_decode_item(&block, item, pixels,
                                   PR_EGA_MAX_WIDTH * PR_EGA_MAX_HEIGHT,
                                   &written) != PR_EGA_OK) {
                fprintf(stderr, "%s: block %zu: pixel decode failed\n", path, i);
                goto done;
            }
            /* Independent direct nibble comparison with the upstream rule. */
            for (pixel = 0; pixel < written; ++pixel) {
                uint8_t packed = decoded[PR_EGA_HEADER_SIZE +
                                         item * block.item_bytes + pixel / 2];
                uint8_t expected = (uint8_t)(pixel & 1 ? packed & 15 : packed >> 4);
                if (pixels[pixel] != expected) {
                    fprintf(stderr, "%s: block %zu: pixel mismatch\n", path, i);
                    goto done;
                }
            }
            sum->decoded_pixels += written;
        }
    }
    ++sum->archives;
    ok = 1;
done:
    free(pixels);
    free(decoded);
    free(bytes);
    fclose(file);
    return ok ? 0 : -1;
}

int main(int argc, char **argv) {
    totals sum = {0};
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s art1.dax [art2.dax ...]\n", argv[0]);
        return 2;
    }
    for (i = 1; i < argc; ++i)
        if (audit_one(argv[i], &sum) != 0) return 1;
    printf("archives=%zu blocks=%zu image_blocks=%zu unsupported_blocks=%zu "
           "items=%zu decoded_pixels=%zu dimension_checks=%zu errors=0\n",
           sum.archives, sum.blocks, sum.image_blocks, sum.unsupported_blocks,
           sum.items, sum.decoded_pixels, sum.dimension_checks);
    return 0;
}
