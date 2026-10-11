/* Read-only local audit. Arguments are ECL1.DAX through ECL8.DAX in order.
 * The program reports aggregate counts only; no input or output bytes. */
#include "poolrad/formats/dax.h"
#include "poolrad/recovered/resident_ecl_dax_block.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const uint8_t *data;
    size_t size;
    unsigned opens;
    unsigned closes;
    char expected_name[16];
} source;

static int open_archive(void *ctx, const char *name, const char *ext,
                        const uint8_t **data, size_t *size) {
    source *s = ctx;
    if (strcmp(name, s->expected_name) || strcmp(ext, ".dax")) return 1;
    ++s->opens;
    *data = s->data;
    *size = s->size;
    return 0;
}

static void close_archive(void *ctx) {
    ++((source *)ctx)->closes;
}

int main(int argc, char **argv) {
    unsigned files = 0, blocks = 0, matches = 0;
    size_t decoded_bytes = 0;
    int n;
    if (argc != 9) {
        fputs("usage: audit_resident_ecl_dax_block ECL1.DAX ... ECL8.DAX\n",
              stderr);
        return 2;
    }
    for (n = 1; n <= 8; ++n) {
        FILE *f = fopen(argv[n], "rb");
        long length;
        uint8_t *bytes;
        pr_dax_archive archive;
        source s = {0};
        pr_rec_res_ecl_dax_io io = {0};
        size_t i;
        if (!f || fseek(f, 0, SEEK_END) || (length = ftell(f)) < 0 ||
            fseek(f, 0, SEEK_SET)) return 3;
        bytes = malloc((size_t)length);
        if (!bytes || fread(bytes, 1, (size_t)length, f) != (size_t)length)
            return 4;
        fclose(f);
        if (pr_dax_open(bytes, (size_t)length, &archive) != PR_DAX_OK)
            return 5;
        s.data = bytes;
        s.size = (size_t)length;
        snprintf(s.expected_name, sizeof s.expected_name, "ECL%d", n);
        io.ctx = &s;
        io.open_archive = open_archive;
        io.close_archive = close_archive;
        for (i = 0; i < archive.count; ++i) {
            uint8_t reference[65536];
            size_t written = 0;
            uint16_t size = 0;
            const uint8_t *block = NULL;
            if (pr_dax_read(&archive, i, reference, sizeof reference,
                            &written) != PR_DAX_OK) return 6;
            if (pr_rec_res_0802_0716(&io, s.expected_name, ".dax",
                                      archive.entries[i].id, &size, &block) !=
                PR_REC_RES_ECL_DAX_OK) return 7;
            if (size != written || memcmp(block, reference, written)) return 8;
            ++blocks;
            ++matches;
            decoded_bytes += written;
            pr_rec_res_0802_0716_release(&io, block, size);
        }
        if (s.opens != archive.count || s.closes != archive.count) return 9;
        ++files;
        free(bytes);
    }
    printf("archives %u blocks %u byte_matches %u decoded_bytes %zu\n",
           files, blocks, matches, decoded_bytes);
    return 0;
}
