#include "poolrad/recovered/resident_464_group3_dirty_metadata.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define DS_SIZE 0x4EC0u
#define ROWS 168u

static uint8_t ds[DS_SIZE], before[DS_SIZE];
static uint8_t source[0x17u], destination[0x17u];

static uint16_t word(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static pr_464_group3_dirty_state fixture(void)
{
    pr_464_group3_dirty_state s = {0};
    unsigned row;
    memset(ds, 0, sizeof ds);
    memset(source, 0, sizeof source);
    memset(destination, 0, sizeof destination);
    for (row = 0u; row < ROWS; ++row) {
        put_word(ds + 0x4AD0u + row * 2u, 0xFFFFu);
        put_word(ds + 0x4C20u + row * 2u, 0xFFFFu);
    }
    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x15u, 0x1234u); /* zero far offset, present segment */
    put_word(destination, ROWS);
    put_word(destination + 2u, 21u);
    s.x = 4u;
    s.y = 7u;
    s.flag = 1u;
    s.video_state = 1u;
    s.source = source;
    s.source_size = sizeof source;
    s.destination = destination;
    s.destination_size = sizeof destination;
    s.ds = ds;
    s.ds_size = sizeof ds;
    return s;
}

static void test_first_and_overlap(void)
{
    pr_464_group3_dirty_state s = fixture();
    pr_464_group3_dirty_report r;
    unsigned row;
    uint16_t first, last;

    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OK);
    assert(r.rows_marked == 8u && r.first_extent_updates == 8u &&
           r.last_extent_updates == 8u && r.changed_bytes > 0u);
    for (row = 56u; row < 64u; ++row) {
        first = (uint16_t)((row * 21u + 4u) * 4u);
        assert(ds[0x4A28u + row] == 1u);
        assert(word(ds + 0x4AD0u + row * 2u) == 4u);
        assert(word(ds + 0x4C20u + row * 2u) == first);
        assert(word(ds + 0x4D70u + row * 2u) == first + 3u);
    }
    assert(ds[0x4A28u + 55u] == 0u && ds[0x4A28u + 64u] == 0u);

    s.x = 5u;
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OK);
    assert(r.first_extent_updates == 0u && r.last_extent_updates == 8u);
    for (row = 56u; row < 64u; ++row) {
        first = (uint16_t)((row * 21u + 4u) * 4u);
        last = (uint16_t)((row * 21u + 5u) * 4u + 3u);
        assert(word(ds + 0x4AD0u + row * 2u) == 4u);
        assert(word(ds + 0x4C20u + row * 2u) == first);
        assert(word(ds + 0x4D70u + row * 2u) == last);
    }

    s.x = 3u;
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OK);
    assert(r.first_extent_updates == 8u && r.last_extent_updates == 0u);
    for (row = 56u; row < 64u; ++row) {
        assert(word(ds + 0x4AD0u + row * 2u) == 3u);
        assert(word(ds + 0x4C20u + row * 2u) ==
               (row * 21u + 3u) * 4u);
        assert(word(ds + 0x4D70u + row * 2u) ==
               (row * 21u + 5u) * 4u + 3u);
    }
    memcpy(before, ds, sizeof ds);
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OK);
    assert(r.changed_bytes == 0u && r.first_extent_updates == 0u &&
           r.last_extent_updates == 0u);
    assert(memcmp(ds, before, sizeof ds) == 0);
}

static void test_gates_and_bounds(void)
{
    pr_464_group3_dirty_state s = fixture();
    pr_464_group3_dirty_report r;
    memcpy(before, ds, sizeof ds);
    assert(pr_464_group3_dirty_metadata(NULL, &r) ==
           PR_464_GROUP3_DIRTY_ARGUMENT);
    s.video_state = 2u;
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OTHER_PATH);
    s.video_state = 1u;
    s.flag = 2u;
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OTHER_PATH);
    s.flag = 3u;
    s.x = 21u;
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OTHER_PATH);
    s.x = 4u;
    s.ds_size = DS_SIZE - 1u;
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_BOUNDS);
    s.ds_size = DS_SIZE;
    put_word(source + 0x15u, 0u);
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OTHER_PATH);
    assert(memcmp(ds, before, sizeof ds) == 0);
}

static void test_saturated_saved_shape(void)
{
    pr_464_group3_dirty_state s = fixture();
    pr_464_group3_dirty_report r;
    unsigned row;
    s.x = 5u;
    for (row = 56u; row < 64u; ++row) {
        ds[0x4A28u + row] = 1u;
        put_word(ds + 0x4AD0u + row * 2u, 2u);
        put_word(ds + 0x4C20u + row * 2u,
                 (uint16_t)((row * 21u + 2u) * 4u));
        put_word(ds + 0x4D70u + row * 2u,
                 (uint16_t)((row * 21u + 12u) * 4u + 3u));
    }
    memcpy(before, ds, sizeof ds);
    assert(pr_464_group3_dirty_metadata(&s, &r) ==
           PR_464_GROUP3_DIRTY_OK);
    assert(r.rows_marked == 8u && r.first_extent_updates == 0u &&
           r.last_extent_updates == 0u && r.changed_bytes == 0u);
    assert(memcmp(ds, before, sizeof ds) == 0);
}

int main(void)
{
    test_first_and_overlap();
    test_gates_and_bounds();
    test_saturated_saved_shape();
    return 0;
}
