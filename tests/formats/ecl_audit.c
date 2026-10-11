#include "poolrad/formats/dax.h"
#include "poolrad/formats/ecl.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t archives;
    size_t records;
    size_t entry_targets;
    size_t unpacked_bytes;
    size_t bounded_text_candidates;
} totals;

static int audit_one(const char *path, totals *sum) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL, *decoded = NULL;
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
    decoded = malloc(PR_ECL_MAX_RECORD);
    if (!bytes || !decoded || fread(bytes, 1, (size_t)length, file) != (size_t)length) {
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
        size_t written = 0, at;
        pr_ecl_record record;
        pr_dax_status dax_status = pr_dax_read(&archive, i, decoded,
                                               PR_ECL_MAX_RECORD, &written);
        pr_ecl_status ecl_status;
        if (dax_status != PR_DAX_OK) {
            fprintf(stderr, "%s: block %zu: %s\n", path, i,
                    pr_dax_status_string(dax_status));
            goto done;
        }
        ecl_status = pr_ecl_open(decoded, written, &record);
        if (ecl_status != PR_ECL_OK) {
            fprintf(stderr, "%s: block %zu: %s\n", path, i,
                    pr_ecl_status_string(ecl_status));
            goto done;
        }
        /* A raw 0x80 byte is only a candidate: this audit does not interpret
         * opcodes or claim that every candidate is a real text operand. */
        for (at = PR_ECL_HEADER_SIZE; at < record.size; ++at) {
            pr_ecl_text frame;
            char scratch[341]; /* floor(255 * 8 / 6) chars plus NUL */
            size_t chars;
            if (record.data[at] != 0x80 ||
                pr_ecl_text_at(&record, at, &frame) != PR_ECL_OK) continue;
            if (pr_ecl_text_decode(&record, &frame, scratch, sizeof scratch,
                                   &chars) != PR_ECL_OK) {
                fprintf(stderr, "%s: block %zu: text candidate decode failed\n",
                        path, i);
                goto done;
            }
            ++sum->bounded_text_candidates;
        }
        ++sum->records;
        sum->entry_targets += PR_ECL_ENTRY_COUNT;
        sum->unpacked_bytes += written;
    }
    ++sum->archives;
    ok = 1;
done:
    free(decoded);
    free(bytes);
    fclose(file);
    return ok ? 0 : -1;
}

int main(int argc, char **argv) {
    totals sum = {0};
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s ECL1.DAX [ECL2.DAX ...]\n", argv[0]);
        return 2;
    }
    for (i = 1; i < argc; ++i)
        if (audit_one(argv[i], &sum) != 0) return 1;
    printf("archives=%zu records=%zu entry_targets=%zu unpacked_bytes=%zu "
           "bounded_text_candidates=%zu errors=0\n", sum.archives, sum.records,
           sum.entry_targets, sum.unpacked_bytes, sum.bounded_text_candidates);
    return 0;
}
