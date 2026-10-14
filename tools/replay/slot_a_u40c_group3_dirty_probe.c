#include "poolrad/recovered/resident_464_group3_dirty_metadata.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE 655360u
#define DS_BASE (0x14A0u * 16u)
#define DS_PREFIX 0x4EC0u
#define FRAME (0x1D16u * 16u + 0x3E78u)

static unsigned word(const uint8_t *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned farptr(const uint8_t *ram, unsigned at)
{
    return word(ram + at) + 16u * word(ram + at + 2u);
}

static uint8_t *load(const char *path)
{
    FILE *file = fopen(path, "rb");
    uint8_t *ram = malloc(RAM_SIZE);
    if (!file || !ram || fread(ram, 1u, RAM_SIZE, file) != RAM_SIZE ||
        fgetc(file) != EOF) {
        fprintf(stderr, "cannot read exact 640 KiB capture: %s\n", path);
        if (file) fclose(file);
        free(ram);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(ram);
        return NULL;
    }
    return ram;
}

int main(int argc, char **argv)
{
    uint8_t *pre, *post, ds[DS_PREFIX];
    unsigned source, destination;
    pr_464_group3_dirty_state state = {0};
    pr_464_group3_dirty_report report;
    int result = 1;

    if (argc != 3) {
        fprintf(stderr, "usage: %s PRE_RAM POST_RAM\n", argv[0]);
        return 2;
    }
    pre = load(argv[1]);
    post = load(argv[2]);
    if (!pre || !post) goto done;
    source = farptr(pre, DS_BASE + 0x5E3Au + 3u * 4u);
    destination = farptr(pre, DS_BASE + 0x4A20u);
    if (source + 0x17u > RAM_SIZE ||
        destination + 0x17u > RAM_SIZE ||
        farptr(post, DS_BASE + 0x4A20u) != destination ||
        word(pre + FRAME) != 1u ||
        word(pre + FRAME + 2u) != 1u ||
        word(pre + FRAME + 4u) != 0xE3u ||
        word(pre + FRAME + 6u) != 7u ||
        word(pre + FRAME + 8u) != 5u) {
        fprintf(stderr, "saved callback frame or object pointers differ\n");
        goto done;
    }
    memcpy(ds, pre + DS_BASE, sizeof ds);
    state.x = word(pre + FRAME + 8u);
    state.y = word(pre + FRAME + 6u);
    state.flag = (uint8_t)word(pre + FRAME + 2u);
    state.video_state = pre[DS_BASE + 0x4A1Eu];
    state.source = pre + source;
    state.source_size = RAM_SIZE - source;
    state.destination = pre + destination;
    state.destination_size = RAM_SIZE - destination;
    state.ds = ds;
    state.ds_size = sizeof ds;
    if (pr_464_group3_dirty_metadata(&state, &report) !=
            PR_464_GROUP3_DIRTY_OK ||
        memcmp(ds, post + DS_BASE, sizeof ds) != 0) {
        fprintf(stderr, "dirty metadata differs from native return\n");
        goto done;
    }
    printf("DS prefix parity %u/%u; rows=%u first=%u last=%u changed=%u\n",
           DS_PREFIX, DS_PREFIX, report.rows_marked,
           report.first_extent_updates, report.last_extent_updates,
           report.changed_bytes);
    result = 0;
done:
    free(pre);
    free(post);
    return result;
}
