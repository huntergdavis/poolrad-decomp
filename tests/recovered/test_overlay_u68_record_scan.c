#include "poolrad/recovered/overlay_u68_record_scan.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t segment[PR_U68_RECORD_SEGMENT_SIZE];

static uint16_t at(uint16_t record_offset, unsigned index)
{
    return (uint16_t)(record_offset + PR_U68_RECORD_SCAN_START + index);
}

int main(void)
{
    uint16_t ax;
    unsigned index;
    const uint16_t record_offset = 0x1234u;

    /* Every possible first hit, including index 0 and the last index 20. */
    for (index = 0; index < PR_U68_RECORD_SCAN_COUNT; ++index) {
        memset(segment, 0xA5, sizeof segment);
        segment[at(record_offset, index)] = 0x1F;
        ax = 0xBEEF;
        CHECK(pr_u68_record_scan(segment, sizeof segment, record_offset,
                                 0xAB1Fu, &ax) == PR_U68_RECORD_SCAN_OK);
        CHECK(ax == index); /* high argument byte is ignored; AH in result is zero */
        CHECK(segment[at(record_offset, index)] == 0x1F);
    }

    /* A missing value checks index 20 then returns AX=0x00FF. */
    memset(segment, 0xA5, sizeof segment);
    ax = 0;
    CHECK(pr_u68_record_scan(segment, sizeof segment, record_offset,
                             0x1Fu, &ax) == PR_U68_RECORD_SCAN_OK);
    CHECK(ax == PR_U68_RECORD_SCAN_MISS);

    /* The first equal byte wins, even if a later one also matches. */
    segment[at(record_offset, 3)] = 0x1F;
    segment[at(record_offset, 17)] = 0x1F;
    CHECK(pr_u68_record_scan(segment, sizeof segment, record_offset,
                             0x1Fu, &ax) == PR_U68_RECORD_SCAN_OK);
    CHECK(ax == 3u);

    /* Only 21 bytes are read; a match at index 21 is still a miss. */
    memset(segment, 0xA5, sizeof segment);
    segment[at(record_offset, 21)] = 0x1F;
    CHECK(pr_u68_record_scan(segment, sizeof segment, record_offset,
                             0x1Fu, &ax) == PR_U68_RECORD_SCAN_OK);
    CHECK(ax == PR_U68_RECORD_SCAN_MISS);

    /* DOS 16-bit effective offset wraps within ES, not into a new segment. */
    memset(segment, 0xA5, sizeof segment);
    segment[at(0xFFD8u, 20)] = 0xFF;
    CHECK(at(0xFFD8u, 20) == 3u);
    CHECK(pr_u68_record_scan(segment, sizeof segment, 0xFFD8u,
                             0x12FFu, &ax) == PR_U68_RECORD_SCAN_OK);
    CHECK(ax == 20u); /* match 0xFF is distinct from the 0x00FF miss result */

    /* Host-only guards leave the result untouched. */
    ax = 0xBEEF;
    CHECK(pr_u68_record_scan(NULL, sizeof segment, 0, 0x1Fu, &ax)
          == PR_U68_RECORD_SCAN_ARGUMENT && ax == 0xBEEF);
    CHECK(pr_u68_record_scan(segment, sizeof segment - 1u, 0, 0x1Fu, &ax)
          == PR_U68_RECORD_SCAN_ARGUMENT && ax == 0xBEEF);
    CHECK(pr_u68_record_scan(segment, sizeof segment, 0, 0x1Fu, NULL)
          == PR_U68_RECORD_SCAN_ARGUMENT);

    puts("overlay_u68_record_scan: ok");
    return 0;
}
