#include "poolrad/recovered/resident_character_target_select.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { CHARACTER_SEG = 0x1111, LINK_SEG = 0x2222, SLOT_SEG = 0x3333 };

typedef struct {
    char kind;
    uint16_t segment, offset, value;
} event;

typedef struct {
    uint8_t character[65536], linked[65536], slot[65536], globals[65536];
    pr_rec_select_far_ptr character_ptr, link_ptr, slot_ptr;
    event events[18];
    unsigned event_count, successful_writes, fail_at;
} fixture;

static uint8_t *area(fixture *f, uint16_t segment)
{
    if (segment == CHARACTER_SEG) return f->character;
    if (segment == LINK_SEG) return f->linked;
    if (segment == SLOT_SEG) return f->slot;
    return NULL;
}

static void put16(uint8_t *mem, uint16_t offset, uint16_t value)
{
    mem[offset] = (uint8_t)value;
    mem[(uint16_t)(offset + 1)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *mem, uint16_t offset)
{
    return (uint16_t)(mem[offset] |
                      ((uint16_t)mem[(uint16_t)(offset + 1)] << 8));
}

static void put_far(uint8_t *mem, uint16_t offset, pr_rec_select_far_ptr p)
{
    put16(mem, offset, p.offset);
    put16(mem, (uint16_t)(offset + 2), p.segment);
}

static bool log_event(fixture *f, char kind, uint16_t segment,
                      uint16_t offset, uint16_t value)
{
    assert(f->event_count < 18);
    f->events[f->event_count++] = (event){kind, segment, offset, value};
    return f->event_count != f->fail_at;
}

static bool read_word(void *context, uint16_t segment, uint16_t offset,
                      uint16_t *value)
{
    fixture *f = context;
    uint8_t *mem = area(f, segment);
    if (!log_event(f, 'R', segment, offset, 0) || !mem) return false;
    *value = get16(mem, offset);
    return true;
}

static bool read_byte(void *context, uint16_t segment, uint16_t offset,
                      uint8_t *value)
{
    fixture *f = context;
    uint8_t *mem = area(f, segment);
    if (!log_event(f, 'B', segment, offset, 0) || !mem) return false;
    *value = mem[offset];
    return true;
}

static bool read_global(void *context, uint16_t offset, uint8_t *value)
{
    fixture *f = context;
    if (!log_event(f, 'G', 0, offset, 0)) return false;
    *value = f->globals[offset];
    return true;
}

static bool write_word(void *context, uint16_t segment, uint16_t offset,
                       uint16_t value)
{
    fixture *f = context;
    uint8_t *mem = area(f, segment);
    if (!log_event(f, 'W', segment, offset, value) || !mem) return false;
    put16(mem, offset, value);
    ++f->successful_writes;
    return true;
}

static pr_rec_target_select_io io_for(fixture *f)
{
    pr_rec_target_select_io io = {f, read_word, read_byte, read_global,
                                  write_word};
    return io;
}

static void prepare(fixture *f, uint8_t flags)
{
    memset(f, 0, sizeof(*f));
    f->character_ptr = (pr_rec_select_far_ptr){0x0100, CHARACTER_SEG};
    f->link_ptr = (pr_rec_select_far_ptr){0x0200, LINK_SEG};
    f->slot_ptr = (pr_rec_select_far_ptr){0x0300, SLOT_SEG};
    put_far(f->character, 0x01cc, f->link_ptr);
    put_far(f->character, 0x01f8, (pr_rec_select_far_ptr){0x0400, 0x4444});
    put_far(f->character, 0x01fc, (pr_rec_select_far_ptr){0x0500, 0x5555});
    f->linked[0x022e] = 7;
    f->globals[0x5600] = flags;
    put_far(f->slot, 0x0300, (pr_rec_select_far_ptr){0xdead, 0xbeef});
}

static void assert_slot(fixture *f, uint16_t offset, uint16_t segment)
{
    assert(get16(f->slot, f->slot_ptr.offset) == offset);
    assert(get16(f->slot, (uint16_t)(f->slot_ptr.offset + 2)) == segment);
}

static void assert_trace(fixture *f, const char *expected)
{
    size_t i;
    assert(f->event_count == strlen(expected));
    for (i = 0; i < f->event_count; ++i)
        assert(f->events[i].kind == expected[i]);
}

static void test_null_link_and_plain_flags(void)
{
    fixture f;
    pr_rec_target_select_io io;
    uint16_t ax;
    prepare(&f, 0xff);
    put_far(f.character, 0x01cc, (pr_rec_select_far_ptr){0, 0});
    io = io_for(&f);
    ax = 0xabcd;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0, 0);
    assert(ax == 0 && f.successful_writes == 2);
    assert_trace(&f, "RRWWRR");

    prepare(&f, 0x01);
    io = io_for(&f);
    ax = 0xabcd;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0, 0);
    assert(ax == 0 && f.successful_writes == 2);
    assert_trace(&f, "RRWWBGRR");
    assert(f.events[5].offset == 0x5600);
}

static void test_link_and_special_result(void)
{
    fixture f;
    pr_rec_target_select_io io;
    uint16_t ax;
    prepare(&f, 0x10);
    io = io_for(&f);
    ax = 0xabcd;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0x0200, LINK_SEG);
    assert(ax == 0x2201 && f.successful_writes == 4);
    assert_trace(&f, "RRWWBGWWRR");

    prepare(&f, 0x0a);
    io = io_for(&f);
    ax = 0xffff;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0, 0);
    assert(ax == 1 && f.successful_writes == 2);
    assert_trace(&f, "RRWWBGRR");

    prepare(&f, 0x08);
    io = io_for(&f);
    ax = 0xffff;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert(ax == 0);
}

static void test_replacements_and_priority(void)
{
    fixture f;
    pr_rec_target_select_io io;
    uint16_t ax;
    prepare(&f, 0x09);
    io = io_for(&f);
    ax = 0;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0x0400, 0x4444);
    assert(ax == 0x4401);
    assert_trace(&f, "RRWWBGRRWWRR");

    prepare(&f, 0x88);
    io = io_for(&f);
    ax = 0;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0x0500, 0x5555);
    assert(ax == 0x5501);
    assert_trace(&f, "RRWWBGRRWWRR");

    prepare(&f, 0x99);
    io = io_for(&f);
    ax = 0;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0x0500, 0x5555);
    assert(ax == 0x5501 && f.successful_writes == 8);
    assert_trace(&f, "RRWWBGWWRRWWRRWWRR");
    assert(f.events[6].value == 0x0200 && f.events[7].value == LINK_SEG);
    assert(f.events[10].value == 0x0400 && f.events[11].value == 0x4444);
    assert(f.events[14].value == 0x0500 && f.events[15].value == 0x5555);

    prepare(&f, 0x8a);
    put_far(f.character, 0x01fc, (pr_rec_select_far_ptr){0, 0});
    io = io_for(&f);
    ax = 0xffff;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0, 0);
    assert(ax == 0);
}

static void test_wrap_and_partial_link(void)
{
    fixture f;
    pr_rec_target_select_io io;
    uint16_t ax;
    prepare(&f, 0x10);
    f.character_ptr.offset = 0xff90;
    f.slot_ptr.offset = 0xfffe;
    f.link_ptr.offset = 0xfff0;
    put_far(f.character, 0x005c, f.link_ptr);
    put_far(f.slot, 0xfffe, (pr_rec_select_far_ptr){0xdead, 0xbeef});
    f.linked[0x001e] = 0xff;
    f.globals[0x6580] = 0x10;
    io = io_for(&f);
    ax = 0;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0xfff0, LINK_SEG);
    assert(ax == 0xff01);
    assert(f.events[0].offset == 0x005c && f.events[1].offset == 0x005e);
    assert(f.events[4].offset == 0x001e && f.events[5].offset == 0x6580);

    prepare(&f, 0x10);
    f.link_ptr.offset = 0;
    put_far(f.character, 0x01cc, f.link_ptr);
    f.linked[0x002e] = 7;
    io = io_for(&f);
    ax = 0;
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    assert_slot(&f, 0, LINK_SEG);
    assert(ax == 0x2201);
}

static void test_callback_boundaries(void)
{
    fixture f;
    pr_rec_target_select_io io;
    uint16_t ax;
    unsigned failed, i, prior_writes;
    static const char sequence[] = "RRWWBGWWRRWWRRWWRR";
    for (failed = 1; failed <= 18; ++failed) {
        prepare(&f, 0x99);
        f.fail_at = failed;
        io = io_for(&f);
        ax = 0xaffe;
        assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, &ax) ==
               PR_REC_TARGET_SELECT_MEMORY);
        assert(ax == 0xaffe && f.event_count == failed);
        prior_writes = 0;
        for (i = 0; i + 1 < failed; ++i)
            prior_writes += sequence[i] == 'W';
        assert(f.successful_writes == prior_writes);
    }
    prepare(&f, 0);
    io = io_for(&f);
    ax = 0;
    assert(pr_rec_res_00ba_2e8b(NULL, f.character_ptr, f.slot_ptr, &ax) ==
           PR_REC_TARGET_SELECT_ARGUMENT);
    assert(pr_rec_res_00ba_2e8b(&io, f.character_ptr, f.slot_ptr, NULL) ==
           PR_REC_TARGET_SELECT_ARGUMENT);
}

int main(void)
{
    test_null_link_and_plain_flags();
    test_link_and_special_result();
    test_replacements_and_priority();
    test_wrap_and_partial_link();
    test_callback_boundaries();
    return 0;
}
