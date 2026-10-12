#include "poolrad/recovered/overlay_u68_list_search.h"
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

static void reset_segments(void)
{
    memset(first_segment, 0xA5, sizeof first_segment);
    memset(second_segment, 0xA5, sizeof second_segment);
}

int main(void)
{
    const pr_u68_far zero = {0, 0};
    const pr_u68_far first = {0x0100u, 0x1111u};
    const pr_u68_far second = {0x0200u, 0x2222u};
    pr_u68_segment_view views[2] = {
        {0x1111u, first_segment, sizeof first_segment},
        {0x2222u, second_segment, sizeof second_segment}
    };
    pr_u68_far visited[4];
    pr_u68_far result = {0xBEEFu, 0xCAFEu};

    /* Original OR-of-two-words null check returns 0:0 without reading ES. */
    CHECK(pr_u68_list_search(zero, NULL, 0, 0x1Fu, &result, NULL, 0)
          == PR_U68_LIST_OK);
    CHECK(result.offset == 0 && result.segment == 0);

    /* First-node hit returns the exact far pointer before reading +0x104. */
    reset_segments();
    first_segment[first.offset + 0x17u + 7u] = 0x1F;
    result = zero;
    CHECK(pr_u68_list_search(first, views, 1, 0xAB1Fu, &result, visited, 4)
          == PR_U68_LIST_OK);
    CHECK(result.offset == first.offset && result.segment == first.segment);

    /* Miss on first node follows the far next pointer, then index-20 hit. */
    reset_segments();
    put_next(first_segment, first, second);
    put_next(second_segment, second, zero);
    second_segment[second.offset + 0x17u + 20u] = 0x1F;
    result = zero;
    CHECK(pr_u68_list_search(first, views, 2, 0x1Fu, &result, visited, 4)
          == PR_U68_LIST_OK);
    CHECK(result.offset == second.offset && result.segment == second.segment);

    /* Both nodes miss: the returned AX:DX is the null pointer. */
    second_segment[second.offset + 0x17u + 20u] = 0xA5;
    result = first;
    CHECK(pr_u68_list_search(first, views, 2, 0x1Fu, &result, visited, 4)
          == PR_U68_LIST_OK);
    CHECK(result.offset == 0 && result.segment == 0);

    /* A nonzero segment with a zero offset is a valid far pointer. */
    reset_segments();
    first_segment[0x17u] = 0x1F;
    result = zero;
    CHECK(pr_u68_list_search((pr_u68_far){0, 0x1111u}, views, 1, 0x1Fu,
                             &result, visited, 4) == PR_U68_LIST_OK);
    CHECK(result.offset == 0 && result.segment == 0x1111u);

    /* Next pointer's +0x104/+0x106 effective offsets wrap within ES. */
    reset_segments();
    put_next(first_segment, (pr_u68_far){0xFF00u, 0x1111u}, second);
    second_segment[second.offset + 0x17u] = 0x1F;
    result = zero;
    CHECK(pr_u68_list_search((pr_u68_far){0xFF00u, 0x1111u}, views, 2,
                             0x1Fu, &result, visited, 4) == PR_U68_LIST_OK);
    CHECK(result.offset == second.offset && result.segment == second.segment);

    /* Host-only cycle detection stops the DOS infinite loop. */
    reset_segments();
    put_next(first_segment, first, first);
    result = second;
    CHECK(pr_u68_list_search(first, views, 1, 0x1Fu, &result, visited, 4)
          == PR_U68_LIST_CYCLE);
    CHECK(result.offset == second.offset && result.segment == second.segment);
    put_next(first_segment, first, second);
    put_next(second_segment, second, first);
    CHECK(pr_u68_list_search(first, views, 2, 0x1Fu, &result, visited, 4)
          == PR_U68_LIST_CYCLE);
    CHECK(result.offset == second.offset && result.segment == second.segment);

    /* Finite scratch capacity and invalid host mappings reject safely. */
    CHECK(pr_u68_list_search(first, views, 2, 0x1Fu, &result, visited, 1)
          == PR_U68_LIST_CAPACITY);
    CHECK(result.offset == second.offset && result.segment == second.segment);
    views[0].size = sizeof first_segment - 1u;
    CHECK(pr_u68_list_search(first, views, 2, 0x1Fu, &result, visited, 4)
          == PR_U68_LIST_ARGUMENT);
    views[0].size = sizeof first_segment;
    CHECK(pr_u68_list_search(first, views, 1, 0x1Fu, &result, visited, 4)
          == PR_U68_LIST_ARGUMENT); /* second selector not mapped */
    reset_segments();
    CHECK(pr_u68_list_search((pr_u68_far){0xFEFBu, 0x1111u}, views, 1,
                             0x1Fu, &result, visited, 4) == PR_U68_LIST_ARGUMENT);
    CHECK(pr_u68_list_search((pr_u68_far){0xFEF9u, 0x1111u}, views, 1,
                             0x1Fu, &result, visited, 4) == PR_U68_LIST_ARGUMENT);
    CHECK(pr_u68_list_search(first, views, 2, 0x1Fu, NULL, visited, 4)
          == PR_U68_LIST_ARGUMENT);
    CHECK(result.offset == second.offset && result.segment == second.segment);

    puts("overlay_u68_list_search: ok");
    return 0;
}
