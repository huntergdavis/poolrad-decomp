#include "poolrad/recovered/resident_464_bitmap_release.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { SLOT_SEG = 0x1000, BITMAP_SEG = 0x2000 };

typedef struct {
    uint8_t slot[65536];
    uint8_t bitmap[65536];
    pr464_far_ptr freed[2];
    uint16_t sizes[2];
    unsigned reads, writes, free_calls;
    unsigned fail_free_at, fail_write_at;
    char events[32];
    unsigned event_count;
} fixture;

static void event(fixture *f, char kind)
{
    assert(f->event_count + 1 < sizeof(f->events));
    f->events[f->event_count++] = kind;
    f->events[f->event_count] = 0;
}

static uint8_t *cell(fixture *f, pr464_far_ptr p)
{
    if (p.segment == SLOT_SEG) return &f->slot[p.offset];
    if (p.segment == BITMAP_SEG) return &f->bitmap[p.offset];
    return NULL;
}

static bool read8(void *ctx, pr464_far_ptr p, uint8_t *value)
{
    fixture *f = ctx;
    uint8_t *src = cell(f, p);
    ++f->reads;
    event(f, 'R');
    if (!src) return false;
    *value = *src;
    return true;
}

static bool read16(void *ctx, pr464_far_ptr p, uint16_t *value)
{
    fixture *f = ctx;
    uint8_t *lo = cell(f, p);
    uint8_t *hi = cell(f, (pr464_far_ptr){(uint16_t)(p.offset + 1), p.segment});
    ++f->reads;
    event(f, 'R');
    if (!lo || !hi) return false;
    *value = (uint16_t)(*lo | ((uint16_t)*hi << 8));
    return true;
}

static bool write16(void *ctx, pr464_far_ptr p, uint16_t value)
{
    fixture *f = ctx;
    uint8_t *lo = cell(f, p);
    uint8_t *hi = cell(f, (pr464_far_ptr){(uint16_t)(p.offset + 1), p.segment});
    ++f->writes;
    event(f, 'W');
    if (f->writes == f->fail_write_at || !lo || !hi) return false;
    *lo = (uint8_t)value;
    *hi = (uint8_t)(value >> 8);
    return true;
}

static bool free_block(void *ctx, pr464_far_ptr p, uint16_t size)
{
    fixture *f = ctx;
    ++f->free_calls;
    event(f, 'F');
    if (f->free_calls <= 2) {
        f->freed[f->free_calls - 1] = p;
        f->sizes[f->free_calls - 1] = size;
    }
    return f->free_calls != f->fail_free_at;
}

static pr464_bitmap_memory memory_for(fixture *f)
{
    pr464_bitmap_memory m = {f, read8, read16, write16, free_block};
    return m;
}

static void set16(fixture *f, pr464_far_ptr p, uint16_t value)
{
    uint8_t *lo = cell(f, p);
    uint8_t *hi = cell(f, (pr464_far_ptr){(uint16_t)(p.offset + 1), p.segment});
    assert(lo && hi);
    *lo = (uint8_t)value;
    *hi = (uint8_t)(value >> 8);
}

static uint16_t get16(fixture *f, pr464_far_ptr p)
{
    uint16_t result = 0;
    assert(read16(f, p, &result));
    return result;
}

static void prepare(fixture *f, pr464_far_ptr slot, pr464_far_ptr bitmap,
                    uint8_t planes, uint16_t stride, pr464_far_ptr auxiliary)
{
    memset(f, 0, sizeof(*f));
    set16(f, slot, bitmap.offset);
    set16(f, (pr464_far_ptr){(uint16_t)(slot.offset + 2), slot.segment}, bitmap.segment);
    *cell(f, (pr464_far_ptr){(uint16_t)(bitmap.offset + 8), bitmap.segment}) = planes;
    set16(f, (pr464_far_ptr){(uint16_t)(bitmap.offset + 0x11), bitmap.segment}, stride);
    set16(f, (pr464_far_ptr){(uint16_t)(bitmap.offset + 0x13), bitmap.segment}, auxiliary.offset);
    set16(f, (pr464_far_ptr){(uint16_t)(bitmap.offset + 0x15), bitmap.segment}, auxiliary.segment);
}

static void test_null_pointer(void)
{
    fixture f;
    pr464_far_ptr slot = {0x100, SLOT_SEG};
    pr464_bitmap_memory m;
    memset(&f, 0, sizeof(f));
    m = memory_for(&f);
    assert(pr464_bitmap_release(&m, slot, 1) == PR464_BITMAP_OK);
    assert(f.reads == 2 && f.free_calls == 0 && f.writes == 0);
}

static void test_primary_only_and_partial_null(void)
{
    fixture f;
    pr464_far_ptr slot = {0x100, SLOT_SEG};
    pr464_far_ptr bitmap = {0, BITMAP_SEG};
    pr464_bitmap_memory m;
    prepare(&f, slot, bitmap, 3, 1001, (pr464_far_ptr){0, 0});
    m = memory_for(&f);
    assert(pr464_bitmap_release(&m, slot, 1) == PR464_BITMAP_OK);
    assert(f.free_calls == 1 && f.sizes[0] == 3026);
    assert(f.freed[0].offset == 0 && f.freed[0].segment == BITMAP_SEG);
    assert(get16(&f, slot) == 0);
    assert(get16(&f, (pr464_far_ptr){0x102, SLOT_SEG}) == 0);
}

static void test_auxiliary_modes_and_wrap(void)
{
    fixture f;
    pr464_far_ptr slot = {0xfffe, SLOT_SEG};
    pr464_far_ptr bitmap = {0xfff0, BITMAP_SEG};
    pr464_far_ptr auxiliary = {0x1234, 0x5678};
    pr464_bitmap_memory m;
    prepare(&f, slot, bitmap, 255, 0xfe00, auxiliary);
    m = memory_for(&f);
    assert(pr464_bitmap_release(&m, slot, 1) == PR464_BITMAP_OK);
    assert(f.free_calls == 2 && f.sizes[0] == 128 && f.sizes[1] == 535);
    assert(strcmp(f.events, "RRRRRRRRRRFRRFWW") == 0);
    assert(f.freed[0].offset == auxiliary.offset &&
           f.freed[0].segment == auxiliary.segment);
    assert(f.freed[1].offset == bitmap.offset && f.freed[1].segment == bitmap.segment);
    assert(get16(&f, slot) == 0 && get16(&f, (pr464_far_ptr){0, SLOT_SEG}) == 0);

    prepare(&f, slot, bitmap, 3, 1001, auxiliary);
    m = memory_for(&f);
    assert(pr464_bitmap_release(&m, slot, 0) == PR464_BITMAP_OK);
    assert(f.sizes[0] == 3003 && f.sizes[1] == 3026);
    prepare(&f, slot, bitmap, 3, 1001, auxiliary);
    m = memory_for(&f);
    assert(pr464_bitmap_release(&m, slot, 2) == PR464_BITMAP_OK);
    assert(f.sizes[0] == 3003);
}

static void test_callback_failures(void)
{
    fixture f;
    pr464_far_ptr slot = {0x100, SLOT_SEG};
    pr464_far_ptr bitmap = {0x200, BITMAP_SEG};
    pr464_far_ptr auxiliary = {0x300, 0x4000};
    pr464_bitmap_memory m;

    prepare(&f, slot, bitmap, 2, 100, auxiliary);
    m = memory_for(&f);
    f.fail_free_at = 1;
    assert(pr464_bitmap_release(&m, slot, 1) == PR464_BITMAP_FREE_FAILED);
    assert(f.free_calls == 1 && f.writes == 0 && get16(&f, slot) == bitmap.offset);

    prepare(&f, slot, bitmap, 2, 100, auxiliary);
    m = memory_for(&f);
    f.fail_free_at = 2;
    assert(pr464_bitmap_release(&m, slot, 1) == PR464_BITMAP_FREE_FAILED);
    assert(f.free_calls == 2 && f.writes == 0 && get16(&f, slot) == bitmap.offset);

    prepare(&f, slot, bitmap, 2, 100, auxiliary);
    m = memory_for(&f);
    f.fail_write_at = 2;
    assert(pr464_bitmap_release(&m, slot, 1) == PR464_BITMAP_ACCESS_FAILED);
    assert(f.free_calls == 2 && f.writes == 2 && get16(&f, slot) == 0);
    assert(get16(&f, (pr464_far_ptr){0x102, SLOT_SEG}) == BITMAP_SEG);
    assert(pr464_bitmap_release(NULL, slot, 1) == PR464_BITMAP_BAD_ARGUMENT);
}

int main(void)
{
    test_null_pointer();
    test_primary_only_and_partial_null();
    test_auxiliary_modes_and_wrap();
    test_callback_failures();
    return 0;
}
