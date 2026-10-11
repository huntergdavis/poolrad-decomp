#include "poolrad/recovered/resident_character_target_reset.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char kind;
    uint16_t segment, offset;
    uint16_t value;
} event;

typedef struct {
    pr_rec_reset_far_ptr character;
    pr_rec_reset_far_ptr targets[4];
    uint8_t bytes[4][65536];
    event events[12];
    unsigned event_count, successful_writes, current_target, fail_at;
    bool retarget_after_write;
} fixture;

static bool record_event(fixture *f, char kind, uint16_t segment,
                         uint16_t offset, uint16_t value)
{
    assert(f->event_count < 12);
    f->events[f->event_count++] = (event){kind, segment, offset, value};
    return f->event_count != f->fail_at;
}

static bool read_word(void *context, uint16_t segment, uint16_t offset,
                      uint16_t *value)
{
    fixture *f = context;
    uint16_t link_offset = (uint16_t)(f->character.offset + 0x108u);
    uint16_t link_segment = (uint16_t)(f->character.offset + 0x10au);
    if (!record_event(f, 'R', segment, offset, 0)) return false;
    if (segment != f->character.segment) return false;
    if (offset == link_offset) {
        *value = f->targets[f->current_target].offset;
        return true;
    }
    if (offset == link_segment) {
        *value = f->targets[f->current_target].segment;
        return true;
    }
    return false;
}

static bool write_byte(void *context, uint16_t segment, uint16_t offset,
                       uint8_t value)
{
    fixture *f = context;
    unsigned target = f->current_target;
    if (!record_event(f, 'W', segment, offset, value)) return false;
    if (segment != f->targets[target].segment || value != 0) return false;
    f->bytes[target][offset] = value;
    ++f->successful_writes;
    if (f->retarget_after_write && target < 3) ++f->current_target;
    return true;
}

static pr_rec_target_reset_io io_for(fixture *f)
{
    pr_rec_target_reset_io io = {f, read_word, write_byte};
    return io;
}

static void prepare(fixture *f)
{
    unsigned i;
    memset(f, 0, sizeof(*f));
    f->character = (pr_rec_reset_far_ptr){0x0100, 0x1111};
    for (i = 0; i < 4; ++i) {
        f->targets[i] = (pr_rec_reset_far_ptr){0x0200, 0x2222};
        memset(f->bytes[i], 0xa5, sizeof(f->bytes[i]));
    }
}

static void assert_order(fixture *f, unsigned count)
{
    static const uint8_t fields[4] = {3, 0, 7, 6};
    unsigned i;
    assert(f->event_count == count);
    for (i = 0; i < count; ++i) {
        event *e = &f->events[i];
        assert(e->kind == (i % 3 == 2 ? 'W' : 'R'));
        if (i % 3 != 2) {
            assert(e->segment == f->character.segment);
            assert(e->offset == (uint16_t)(f->character.offset +
                                           (i % 3 == 0 ? 0x108u : 0x10au)));
        } else {
            unsigned target = f->retarget_after_write ? i / 3 : 0;
            assert(e->segment == f->targets[target].segment);
            assert(e->offset == (uint16_t)(f->targets[target].offset + fields[i / 3]));
            assert(e->value == 0);
        }
    }
}

static void test_order_and_return(void)
{
    fixture f;
    pr_rec_target_reset_io io;
    uint16_t ax = 0xa5fe;
    prepare(&f);
    io = io_for(&f);
    assert(pr_rec_res_00ba_2637(&io, f.character, &ax) == PR_REC_TARGET_RESET_OK);
    assert(ax == 0xa501 && f.successful_writes == 4);
    assert_order(&f, 12);
    assert(f.bytes[0][0x203] == 0 && f.bytes[0][0x200] == 0);
    assert(f.bytes[0][0x207] == 0 && f.bytes[0][0x206] == 0);
    assert(f.bytes[0][0x202] == 0xa5);
}

static void test_retarget_and_wrap(void)
{
    fixture f;
    pr_rec_target_reset_io io;
    uint16_t ax = 0x7f00;
    unsigned i;
    static const uint8_t fields[4] = {3, 0, 7, 6};
    prepare(&f);
    f.character.offset = 0xfffc;
    f.retarget_after_write = true;
    for (i = 0; i < 4; ++i) {
        f.targets[i].offset = (uint16_t)(0xfffc + i);
        f.targets[i].segment = (uint16_t)(0x2222 + i);
    }
    io = io_for(&f);
    assert(pr_rec_res_00ba_2637(&io, f.character, &ax) == PR_REC_TARGET_RESET_OK);
    assert(ax == 0x7f01 && f.successful_writes == 4);
    assert_order(&f, 12);
    for (i = 0; i < 4; ++i) {
        uint16_t at = (uint16_t)(f.targets[i].offset + fields[i]);
        assert(f.bytes[i][at] == 0);
        assert(f.bytes[i][(uint16_t)(at + 1)] == 0xa5);
    }
}

static void test_partial_null_and_failures(void)
{
    fixture f;
    pr_rec_target_reset_io io;
    uint16_t ax;
    unsigned failed_event;
    prepare(&f);
    f.targets[0].offset = 0;
    io = io_for(&f);
    ax = 0x9900;
    assert(pr_rec_res_00ba_2637(&io, f.character, &ax) == PR_REC_TARGET_RESET_OK);
    assert(ax == 0x9901 && f.bytes[0][3] == 0);

    for (failed_event = 1; failed_event <= 12; ++failed_event) {
        prepare(&f);
        f.fail_at = failed_event;
        io = io_for(&f);
        ax = 0x12fe;
        assert(pr_rec_res_00ba_2637(&io, f.character, &ax) ==
               PR_REC_TARGET_RESET_MEMORY);
        assert(ax == 0x12fe);
        assert(f.event_count == failed_event);
        assert(f.successful_writes == (failed_event - 1) / 3);
    }
    prepare(&f);
    io = io_for(&f);
    ax = 0;
    assert(pr_rec_res_00ba_2637(NULL, f.character, &ax) ==
           PR_REC_TARGET_RESET_ARGUMENT);
    assert(pr_rec_res_00ba_2637(&io, f.character, NULL) ==
           PR_REC_TARGET_RESET_ARGUMENT);
}

int main(void)
{
    test_order_and_return();
    test_retarget_and_wrap();
    test_partial_null_and_failures();
    return 0;
}
