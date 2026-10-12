#include "poolrad/recovered/overlay_u68_record_clear.h"
#include "poolrad/recovered/overlay_u68_record_scan.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t first_segment[PR_U68_RECORD_SEGMENT_SIZE];
static uint8_t second_segment[PR_U68_RECORD_SEGMENT_SIZE];

static void put_word(uint8_t *segment, uint16_t offset, uint16_t word)
{
    segment[offset] = (uint8_t)word;
    segment[(uint16_t)(offset + 1u)] = (uint8_t)(word >> 8);
}

static void put_next(uint8_t *segment, pr_u68_far node, pr_u68_far next)
{
    put_word(segment, (uint16_t)(node.offset + 0x104u), next.offset);
    put_word(segment, (uint16_t)(node.offset + 0x106u), next.segment);
}

int main(void)
{
    const pr_u68_far zero = {0, 0};
    const pr_u68_far first = {0x0100u, 0x1111u};
    const pr_u68_far second = {0x0200u, 0x2222u};
    pr_u68_mutable_segment_view views[2] = {
        {0x1111u, first_segment, sizeof first_segment},
        {0x2222u, second_segment, sizeof second_segment}
    };
    pr_u68_segment_view read_views[2];
    pr_u68_far visited[4];
    uint16_t ax = 0xBEEFu;

    /* A null DS head returns false before any ES view or scratch is read. */
    CHECK(pr_u68_record_clear(zero, NULL, 0, NULL, 0, NULL, 0, &ax)
          == PR_U68_CLEAR_OK && ax == 0);

    /* First node, first matching byte: clear only that byte and return 1. */
    memset(first_segment, 0xA5, sizeof first_segment);
    first_segment[first.offset + 0x17u] = 0x1F;
    first_segment[first.offset + 0x18u] = 0x1F;
    ax = 0xBEEFu;
    CHECK(pr_u68_record_clear(first, views, 1, read_views, 2, visited, 4,
                               &ax) == PR_U68_CLEAR_OK && ax == 1);
    CHECK(first_segment[first.offset + 0x17u] == 0);
    CHECK(first_segment[first.offset + 0x18u] == 0x1F);

    /* A miss at the head follows +0x104 and clears index 20 in node two. */
    memset(first_segment, 0xA5, sizeof first_segment);
    memset(second_segment, 0xA5, sizeof second_segment);
    put_next(first_segment, first, second);
    put_next(second_segment, second, zero);
    second_segment[second.offset + 0x17u + 20u] = 0x1F;
    CHECK(pr_u68_record_clear(first, views, 2, read_views, 2, visited, 4,
                               &ax) == PR_U68_CLEAR_OK && ax == 1);
    CHECK(second_segment[second.offset + 0x17u + 20u] == 0);
    CHECK(first_segment[first.offset + 0x17u] == 0xA5);

    /* A finite all-miss list returns zero and leaves both views untouched. */
    second_segment[second.offset + 0x17u + 20u] = 0xA5;
    ax = 0xBEEFu;
    CHECK(pr_u68_record_clear(first, views, 2, read_views, 2, visited, 4,
                               &ax) == PR_U68_CLEAR_OK && ax == 0);
    CHECK(second_segment[second.offset + 0x17u + 20u] == 0xA5);

    /* Offset zero with nonzero segment is a valid node; wrapped byte write. */
    memset(first_segment, 0xA5, sizeof first_segment);
    first_segment[0x17u + 3u] = 0x1F;
    CHECK(pr_u68_record_clear((pr_u68_far){0, 0x1111u}, views, 1,
                               read_views, 2, visited, 4, &ax)
          == PR_U68_CLEAR_OK && ax == 1);
    CHECK(first_segment[0x17u + 3u] == 0);
    memset(first_segment, 0xA5, sizeof first_segment);
    first_segment[0x0000u] = 0x1F;
    CHECK(pr_u68_record_clear((pr_u68_far){0xFFE9u, 0x1111u}, views, 1,
                               read_views, 2, visited, 4, &ax)
          == PR_U68_CLEAR_OK && ax == 1);
    CHECK(first_segment[0] == 0);

    /* Host guards are explicit and preserve the prior result and bytes. */
    ax = 0xBEEFu;
    CHECK(pr_u68_record_clear(first, views, 2, read_views, 1, visited, 4,
                               &ax) == PR_U68_CLEAR_ARGUMENT && ax == 0xBEEFu);
    memset(first_segment, 0xA5, sizeof first_segment);
    put_next(first_segment, first, first);
    CHECK(pr_u68_record_clear(first, views, 1, read_views, 2, visited, 4,
                               &ax) == PR_U68_CLEAR_CYCLE && ax == 0xBEEFu);
    CHECK(first_segment[first.offset + 0x17u] == 0xA5);
    put_next(first_segment, first, second);
    put_next(second_segment, second, zero);
    CHECK(pr_u68_record_clear(first, views, 2, read_views, 2, visited, 1,
                               &ax) == PR_U68_CLEAR_CAPACITY && ax == 0xBEEFu);
    CHECK(pr_u68_record_clear(first, views, 1, read_views, 2, visited, 4,
                               &ax) == PR_U68_CLEAR_ARGUMENT && ax == 0xBEEFu);
    CHECK(pr_u68_record_clear(first, views, 1, read_views, 2, visited, 0,
                               &ax) == PR_U68_CLEAR_ARGUMENT && ax == 0xBEEFu);
    CHECK(pr_u68_record_clear(first, views, 1, read_views, 2, visited, 4,
                               NULL) == PR_U68_CLEAR_ARGUMENT);

    puts("overlay_u68_record_clear: ok");
    return 0;
}
