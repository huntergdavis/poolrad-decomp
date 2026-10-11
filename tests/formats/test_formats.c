#include "poolrad/formats/dax.h"
#include "poolrad/formats/geo.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static void sample_archive(uint8_t bytes[28]) {
    memset(bytes, 0, 28);
    bytes[0] = 18;             /* two nine-byte directory entries */
    bytes[2] = 7;              /* raw block, ID 7 */
    bytes[9] = 3;
    bytes[11] = 9;             /* compressed block, ID 9 */
    bytes[12] = 3;            /* relative offset from end of directory */
    bytes[16] = 5;            /* unpacked length */
    bytes[18] = 5;            /* stored length */
    memcpy(bytes + 20, "xyz", 3);
    bytes[23] = 1;            /* two literal bytes */
    bytes[24] = 'A';
    bytes[25] = 'B';
    bytes[26] = 0xfd;         /* three repeated bytes */
    bytes[27] = 'C';
}

static int test_dax(void) {
    uint8_t bytes[28], output[8] = {0};
    pr_dax_archive archive;
    size_t written = 0;
    sample_archive(bytes);
    CHECK(pr_dax_open(bytes, sizeof bytes, &archive) == PR_DAX_OK);
    CHECK(archive.count == 2 && archive.entries[0].id == 7 &&
          archive.entries[1].id == 9);
    CHECK(pr_dax_read(&archive, 0, output, 2, &written) == PR_DAX_OUTPUT_SIZE);
    CHECK(written == 3);
    CHECK(pr_dax_read(&archive, 0, output, sizeof output, &written) == PR_DAX_OK);
    CHECK(written == 3 && memcmp(output, "xyz", 3) == 0);
    CHECK(pr_dax_read(&archive, 1, output, sizeof output, &written) == PR_DAX_OK);
    CHECK(written == 5 && memcmp(output, "ABCCC", 5) == 0);
    CHECK(pr_dax_read(&archive, 2, output, sizeof output, &written) == PR_DAX_INDEX);
    CHECK(pr_dax_read(&archive, 1, NULL, 0, &written) == PR_DAX_OUTPUT_SIZE);
    CHECK(written == 5);
    bytes[26] = 0xfc;
    CHECK(pr_dax_read(&archive, 1, output, sizeof output, &written) == PR_DAX_RLE);
    bytes[26] = 0xfd;

    bytes[11] = 7;
    CHECK(pr_dax_open(bytes, sizeof bytes, &archive) == PR_DAX_DUPLICATE_ID);
    bytes[11] = 9;
    bytes[12] = 8;
    CHECK(pr_dax_open(bytes, sizeof bytes, &archive) == PR_DAX_RANGE);
    bytes[12] = 3;
    bytes[9] = 0;
    CHECK(pr_dax_open(bytes, sizeof bytes, &archive) == PR_DAX_RANGE);
    bytes[9] = 3;
    bytes[0] = 17;
    CHECK(pr_dax_open(bytes, sizeof bytes, &archive) == PR_DAX_INDEX);
    bytes[0] = 18;
    CHECK(pr_dax_open(bytes, 10, &archive) == PR_DAX_SIZE);
    CHECK(pr_dax_open(bytes, 19, &archive) == PR_DAX_INDEX);
    CHECK(pr_dax_open(bytes, PR_DAX_MAX_FILE + 1, &archive) == PR_DAX_SIZE);
    CHECK(pr_dax_open(NULL, sizeof bytes, &archive) == PR_DAX_ARGUMENT);
    CHECK(pr_dax_open(bytes, sizeof bytes, NULL) == PR_DAX_ARGUMENT);
    return 0;
}

static int test_rle(void) {
    uint8_t output[5];
    const uint8_t valid[] = {1, 'A', 'B', 0xfd, 'C'};
    const uint8_t short_literal[] = {2, 'A'};
    const uint8_t short_repeat[] = {0xff};
    const uint8_t long_repeat[] = {0x80, 'Q'};
    uint8_t long_output[128];
    CHECK(pr_dax_rle(valid, sizeof valid, output, sizeof output) == PR_DAX_OK);
    CHECK(memcmp(output, "ABCCC", 5) == 0);
    CHECK(pr_dax_rle(valid, sizeof valid, output, 4) == PR_DAX_RLE);
    CHECK(pr_dax_rle(valid, sizeof valid, output, 6) == PR_DAX_RLE);
    CHECK(pr_dax_rle(short_literal, sizeof short_literal, output, 3) == PR_DAX_RLE);
    CHECK(pr_dax_rle(short_repeat, sizeof short_repeat, output, 1) == PR_DAX_RLE);
    CHECK(pr_dax_rle(long_repeat, sizeof long_repeat, long_output,
                     sizeof long_output) == PR_DAX_OK);
    CHECK(long_output[0] == 'Q' && long_output[127] == 'Q');
    CHECK(pr_dax_rle(NULL, 1, output, 1) == PR_DAX_ARGUMENT);
    CHECK(pr_dax_rle(valid, sizeof valid, NULL, 5) == PR_DAX_ARGUMENT);
    CHECK(pr_dax_rle(NULL, 0, NULL, 0) == PR_DAX_OK);
    return 0;
}

static int test_geo(void) {
    uint8_t bytes[PR_GEO_SIZE] = {0}, value = 0;
    pr_geo_map map;
    const unsigned x = 3, y = 7, tile = y * PR_GEO_WIDTH + x;
    bytes[2 + tile] = 0xa3;
    bytes[258 + tile] = 0xb4;
    bytes[258 + tile + 1] = 0xf5; /* adjacent west edge differs from east */
    bytes[514 + tile] = 0x5a;
    bytes[770 + tile] = 0xe4;
    CHECK(pr_geo_parse(bytes, sizeof bytes, &map) == 0);
    CHECK(pr_geo_wall(&map, x, y, PR_GEO_NORTH, &value) == 0 && value == 10);
    CHECK(pr_geo_wall(&map, x, y, PR_GEO_EAST, &value) == 0 && value == 3);
    CHECK(pr_geo_wall(&map, x, y, PR_GEO_SOUTH, &value) == 0 && value == 11);
    CHECK(pr_geo_wall(&map, x, y, PR_GEO_WEST, &value) == 0 && value == 4);
    CHECK(pr_geo_wall(&map, x + 1, y, PR_GEO_WEST, &value) == 0 && value == 5);
    CHECK(pr_geo_event(&map, x, y, &value) == 0 && value == 0x5a);
    CHECK(pr_geo_door(&map, x, y, PR_GEO_NORTH, &value) == 0 && value == 0);
    CHECK(pr_geo_door(&map, x, y, PR_GEO_EAST, &value) == 0 && value == 1);
    CHECK(pr_geo_door(&map, x, y, PR_GEO_SOUTH, &value) == 0 && value == 2);
    CHECK(pr_geo_door(&map, x, y, PR_GEO_WEST, &value) == 0 && value == 3);
    CHECK(pr_geo_parse(bytes, sizeof bytes - 1, &map) == -1);
    CHECK(pr_geo_parse(NULL, sizeof bytes, &map) == -1);
    CHECK(pr_geo_wall(&map, 16, y, PR_GEO_NORTH, &value) == -1);
    CHECK(pr_geo_wall(&map, x, y, (pr_geo_direction)-1, &value) == -1);
    CHECK(pr_geo_door(&map, x, y, (pr_geo_direction)4, &value) == -1);
    CHECK(pr_geo_event(&map, x, y, NULL) == -1);
    return 0;
}

int main(void) {
    if (test_dax() || test_rle() || test_geo()) return 1;
    puts("synthetic DAX/RLE/GEO tests passed");
    return 0;
}
