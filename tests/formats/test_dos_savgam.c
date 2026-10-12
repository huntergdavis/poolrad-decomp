#include "poolrad/formats/dos_savgam.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int test_framing(void) {
    uint8_t data[PR_DOS_SAVGAM_SIZE] = {0};
    pr_dos_savgam_record record, saved;
    pr_dos_savgam_span block, old_block;
    size_t i, end = 0;
    CHECK(pr_dos_savgam_open(data, sizeof data, &record) == PR_DOS_SAVGAM_OK);
    CHECK(record.data == data && record.size == sizeof data);
    saved = record;
    CHECK(pr_dos_savgam_open(data, sizeof data - 1, &record) ==
          PR_DOS_SAVGAM_BAD_SIZE);
    CHECK(pr_dos_savgam_open(data, sizeof data + 1, &record) ==
          PR_DOS_SAVGAM_BAD_SIZE);
    CHECK(record.data == saved.data && record.size == saved.size);
    CHECK(pr_dos_savgam_open(NULL, sizeof data, &record) ==
          PR_DOS_SAVGAM_ARGUMENT);
    CHECK(pr_dos_savgam_open(data, sizeof data, NULL) ==
          PR_DOS_SAVGAM_ARGUMENT);
    for (i = 0; i < PR_DOS_SAVGAM_BLOCK_COUNT; ++i) {
        CHECK(pr_dos_savgam_block(&record, i, &block) == PR_DOS_SAVGAM_OK);
        CHECK(block.source_offset == end);
        CHECK(block.end_offset > block.source_offset);
        end = block.end_offset;
    }
    CHECK(end == sizeof data);
    old_block = block;
    CHECK(pr_dos_savgam_block(&record, PR_DOS_SAVGAM_BLOCK_COUNT, &block) ==
          PR_DOS_SAVGAM_RANGE);
    CHECK(block.source_offset == old_block.source_offset &&
          block.end_offset == old_block.end_offset);
    CHECK(pr_dos_savgam_block(NULL, 0, &block) == PR_DOS_SAVGAM_ARGUMENT);
    CHECK(pr_dos_savgam_block(&record, 0, NULL) == PR_DOS_SAVGAM_ARGUMENT);
    return 0;
}

static int test_restored_fields(void) {
    uint8_t data[PR_DOS_SAVGAM_SIZE] = {0};
    pr_dos_savgam_record record;
    pr_dos_savgam_core core, saved;
    data[0] = 3;
    data[0x01E4] = 0xA5; /* previous byte is not the map ID */
    data[0x01E5] = 0x17;
    data[0x01E6] = 0xA6; /* high byte of the saved map word is not read */
    data[0x0E24] = 0xB6;
    data[0x0E25] = 4;
    data[0x3200] = 0xC7;
    data[0x3201] = 15;
    data[0x3202] = 4;
    data[0x3203] = 6;
    data[0x3204] = 0xD8;
    data[0x3207] = 4;
    data[0x3208] = 6;
    CHECK(pr_dos_savgam_open(data, sizeof data, &record) == PR_DOS_SAVGAM_OK);
    CHECK(pr_dos_savgam_read_core(&record, &core) == PR_DOS_SAVGAM_OK);
    CHECK(core.saved_ecl_file == 3 && core.restored_ecl_file == 4);
    CHECK(core.map_id == 0x17 && core.x == 15 && core.y == 4);
    CHECK(core.facing == 6 && core.mode == 4 && core.character_count == 6);
    saved = core;
    CHECK(pr_dos_savgam_read_core(NULL, &core) == PR_DOS_SAVGAM_ARGUMENT);
    CHECK(pr_dos_savgam_read_core(&record, NULL) == PR_DOS_SAVGAM_ARGUMENT);
    CHECK(memcmp(&core, &saved, sizeof core) == 0);
    record.size--;
    CHECK(pr_dos_savgam_read_core(&record, &core) == PR_DOS_SAVGAM_ARGUMENT);
    CHECK(memcmp(&core, &saved, sizeof core) == 0);
    return 0;
}

int main(void) {
    if (test_framing() || test_restored_fields()) return 1;
    puts("synthetic DOS world-save framing and restored-field tests passed");
    return 0;
}
