#include "poolrad/recovered/overlay_u40c_group3_tile_plane.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define HEADER 0x17u
#define SOURCE_SIZE (HEADER + 41u * 32u)
#define MASK_SIZE (41u * 8u)
#define DEST_SIZE (HEADER + 168u * 21u * 4u)

static uint8_t source[SOURCE_SIZE], mask[MASK_SIZE], destination[DEST_SIZE];
static uint8_t before[DEST_SIZE];

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static pr_u40c_group3_tile_plane_state fixture(void)
{
    pr_u40c_group3_tile_plane_state s = {0};
    unsigned row, plane;
    memset(source, 0, sizeof source);
    memset(mask, 0, sizeof mask);
    memset(destination, 0xA5, sizeof destination);
    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 0u);      /* live pointer case: zero offset */
    put_word(source + 0x15u, 0x1234u);/* but nonzero segment */
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    for (row = 0; row < 8u; ++row) {
        mask[40u * 8u + row] = (uint8_t)(0xF0u | row);
        for (plane = 0; plane < 4u; ++plane)
            source[HEADER + 40u * 32u + row * 4u + plane] =
                (uint8_t)(0x10u + row * 4u + plane);
    }
    memcpy(before, destination, sizeof before);
    s.item_id = 0xE2u;
    s.group3_remap = 0xBAu;
    s.x = 4u;
    s.y = 7u;
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

static void test_index_40_and_present_segment(void)
{
    pr_u40c_group3_tile_plane_state s = fixture();
    pr_u40c_group3_tile_plane_report r;
    size_t first = ((7u * 8u * 21u + 4u) * 4u);
    unsigned row, plane, changes = 0;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_OK);
    assert(r.item_index == 40u);
    assert(r.first_data_offset == first);
    assert(r.bytes_written == 32u);
    for (row = 0; row < 8u; ++row) {
        for (plane = 0; plane < 4u; ++plane) {
            size_t at = HEADER + first + row * 84u + plane;
            uint8_t expected = (uint8_t)((before[at] &
                                mask[40u * 8u + row]) |
                                source[HEADER + 40u * 32u +
                                       row * 4u + plane]);
            assert(destination[at] == expected);
            if (expected != before[at]) ++changes;
            before[at] = expected;
        }
    }
    assert(memcmp(destination, before, sizeof destination) == 0);
    assert(r.bytes_changed == changes);
    assert(r.first_changed_offset == first);
}

static void test_gates_and_bounds(void)
{
    pr_u40c_group3_tile_plane_state s;
    pr_u40c_group3_tile_plane_report r;
    assert(pr_u40c_entry2_group3_tile_plane(NULL, NULL) ==
           PR_U40C_GROUP3_TILE_ARGUMENT);

    s = fixture();
    s.item_id = 0xB9u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_OTHER_PATH);
    assert(memcmp(destination, before, sizeof destination) == 0);
    s.item_id = 0x100u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_OTHER_PATH);
    s.item_id = 0xE2u;
    s.video_state = 0u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_OTHER_PATH);
    s.video_state = 1u;
    s.flag = 0u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_OTHER_PATH);
    s.flag = 1u;
    s.y = 21u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_OTHER_PATH);

    s = fixture();
    put_word(source + 0x15u, 0u);
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_OTHER_PATH);
    assert(memcmp(destination, before, sizeof destination) == 0);

    s = fixture();
    s.source_size = SOURCE_SIZE - 1u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_BOUNDS);
    assert(memcmp(destination, before, sizeof destination) == 0);
    s.source_size = SOURCE_SIZE;
    s.aux_mask_size = MASK_SIZE - 1u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_BOUNDS);
    assert(memcmp(destination, before, sizeof destination) == 0);
    s.aux_mask_size = MASK_SIZE;
    s.destination_size = DEST_SIZE - 1u;
    assert(pr_u40c_entry2_group3_tile_plane(&s, &r) ==
           PR_U40C_GROUP3_TILE_BOUNDS);
    assert(memcmp(destination, before, sizeof destination) == 0);
}

int main(void)
{
    test_index_40_and_present_segment();
    test_gates_and_bounds();
    return 0;
}
