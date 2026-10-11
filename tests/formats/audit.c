#include "poolrad/formats/dax.h"
#include "poolrad/formats/geo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int geo_name(const char *path) {
    const char *base = strrchr(path, '/');
    const char *p = base ? base + 1 : path;
    if (strncmp(p, "GEO", 3) != 0) return 0;
    p += 3;
    if (*p < '0' || *p > '9') return 0;
    do { ++p; } while (*p >= '0' && *p <= '9');
    return strcmp(p, ".DAX") == 0;
}

static int audit_one(const char *path, size_t *blocks, size_t *maps) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL, *decoded = NULL;
    pr_dax_archive archive;
    long length;
    size_t i;
    int ok = 0;
    if (!file) { perror(path); return -1; }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        (unsigned long)length > PR_DAX_MAX_FILE ||
        fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: invalid file size\n", path);
        goto done;
    }
    bytes = malloc((size_t)length ? (size_t)length : 1);
    decoded = malloc(65535);
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
        size_t written = 0;
        pr_dax_status status = pr_dax_read(&archive, i, decoded, 65535, &written);
        if (status != PR_DAX_OK) {
            fprintf(stderr, "%s: block %zu: %s\n", path, i,
                    pr_dax_status_string(status));
            goto done;
        }
        if (geo_name(path)) {
            pr_geo_map map;
            if (pr_geo_parse(decoded, written, &map) != 0) {
                fprintf(stderr, "%s: block %zu: invalid GEO size\n", path, i);
                goto done;
            }
            ++*maps;
        }
        ++*blocks;
    }
    ok = 1;
done:
    free(decoded);
    free(bytes);
    fclose(file);
    return ok ? 0 : -1;
}

int main(int argc, char **argv) {
    size_t archives = 0, blocks = 0, maps = 0;
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s archive.dax [archive.dax ...]\n", argv[0]);
        return 2;
    }
    for (i = 1; i < argc; ++i) {
        if (audit_one(argv[i], &blocks, &maps) != 0) return 1;
        ++archives;
    }
    printf("archives=%zu blocks=%zu geo_records=%zu\n", archives, blocks, maps);
    return 0;
}
