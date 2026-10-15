#include "poolrad/recovered/overlay_u40c_group2_tile_plane.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define HEADER 0x17u
#define SOURCE_SIZE (HEADER + 70u * 32u)
#define MASK_SIZE (70u * 8u)
#define DEST_SIZE (HEADER + 168u * 21u * 4u)

static uint8_t source[SOURCE_SIZE], mask[MASK_SIZE], destination[DEST_SIZE];
static uint8_t expected[DEST_SIZE];

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static pr_u40c_group2_tile_plane_state fixture(void)
{
    pr_u40c_group2_tile_plane_state s = {0};
    unsigned row, plane;
    memset(source, 0, sizeof source);
    memset(mask, 0, sizeof mask);
    memset(destination, 0xAA, sizeof destination);
    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 0u);
    put_word(source + 0x15u, 0x1234u); /* zero offset, present segment */
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    for (row = 0; row < 8u; ++row) {
        mask[27u * 8u + row] = (uint8_t)(row == 7u ? 0x0Fu : 0xF0u);
        for (plane = 0; plane < 4u; ++plane)
            source[HEADER + 27u * 32u + row * 4u + plane] =
                (uint8_t)(1u << plane);
    }
    memcpy(expected, destination, sizeof expected);
    s.item_id = 143u;
    s.group2_remap = 116u;
    s.x = 2u;
    s.y = 6u;
    s.flag = 1u;
    s.video_state = 1u;
    s.source = source;
    s.source_size = sizeof source;
    s.aux_mask = mask;
    s.aux_mask_size = sizeof mask;
    s.destination = destination;
    s.destination_size = sizeof destination;
    return s;
}

static void test_index27_masked_writes_and_noop(void)
{
    pr_u40c_group2_tile_plane_state s = fixture();
    pr_u40c_group2_tile_plane_report r;
    size_t first = (6u * 8u * 21u + 2u) * 4u;
    unsigned row, plane;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OK);
    assert(r.item_index == 27u);
    assert(r.first_data_offset == first);
    assert(r.first_changed_offset == first);
    assert(r.bytes_written == 32u && r.bytes_changed == 32u);
    for (row = 0; row < 8u; ++row) {
        for (plane = 0; plane < 4u; ++plane) {
            size_t at = HEADER + first + row * 84u + plane;
            expected[at] = (uint8_t)((row == 7u ? 0x0Au : 0xA0u) |
                                     (1u << plane));
        }
    }
    assert(memcmp(destination, expected, sizeof destination) == 0);

    /* A second source/mask pair must read the first result, not the
     * original 0xAA destination. */
    memset(source + HEADER + 27u * 32u, 0, 32u);
    memset(mask + 27u * 8u, 0x0Fu, 8u);
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OK);
    for (row = 0; row < 8u; ++row) {
        for (plane = 0; plane < 4u; ++plane) {
            size_t at = HEADER + first + row * 84u + plane;
            expected[at] &= 0x0Fu;
        }
    }
    assert(r.bytes_written == 32u && r.bytes_changed == 28u);
    assert(memcmp(destination, expected, sizeof destination) == 0);

    memset(mask + 27u * 8u, 0xFFu, 8u);
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OK);
    assert(r.bytes_written == 32u && r.bytes_changed == 0u);
    assert(r.first_changed_offset == (size_t)-1);
    assert(memcmp(destination, expected, sizeof destination) == 0);
}

static void test_gates_are_atomic(void)
{
    pr_u40c_group2_tile_plane_state s;
    pr_u40c_group2_tile_plane_report r;
    assert(pr_u40c_entry2_group2_tile_plane(NULL, NULL) ==
           PR_U40C_GROUP2_TILE_ARGUMENT);

    s = fixture();
    s.item_id = 115u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OTHER_PATH);
    s.item_id = 186u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OTHER_PATH);
    s.item_id = 143u;
    s.flag = 0u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OTHER_PATH);
    s.flag = 1u;
    s.video_state = 0u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OTHER_PATH);
    s.video_state = 1u;
    s.y = 21u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OTHER_PATH);
    assert(memcmp(destination, expected, sizeof destination) == 0);

    s = fixture();
    put_word(source + 0x15u, 0u);
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_OTHER_PATH);
    assert(memcmp(destination, expected, sizeof destination) == 0);

    s = fixture();
    s.source_size = HEADER + 28u * 32u - 1u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_BOUNDS);
    assert(memcmp(destination, expected, sizeof destination) == 0);
    s.source_size = sizeof source;
    s.aux_mask_size = 28u * 8u - 1u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_BOUNDS);
    assert(memcmp(destination, expected, sizeof destination) == 0);
    s.aux_mask_size = sizeof mask;
    s.destination_size = DEST_SIZE - 1u;
    assert(pr_u40c_entry2_group2_tile_plane(&s, &r) ==
           PR_U40C_GROUP2_TILE_BOUNDS);
    assert(memcmp(destination, expected, sizeof destination) == 0);
}

int main(void)
{
    test_index27_masked_writes_and_noop();
    test_gates_are_atomic();
    return 0;
}
