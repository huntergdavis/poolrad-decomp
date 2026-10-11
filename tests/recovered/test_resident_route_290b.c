#include "poolrad/recovered/resident_route_290b.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)
#define EVENT(kind, value) (((uint32_t)(kind) << 16) | (value))

typedef struct {
    uint8_t first, second, record_first, record_second;
    uint8_t selector[3], mode, index, length1, length2;
    uint16_t word_192, word_190, word_18e, word_1f6, other_flags;
    uint32_t events[128];
    uint16_t calls[40];
    unsigned event_count, call_count, fail_at, selector_reads;
    int bad;
} fixture;

static fixture setup(void)
{
    fixture f = {0};
    f.first = 0x31u;
    f.second = 0x42u;
    f.record_first = 250u;
    f.record_second = 0x72u;
    f.selector[0] = f.selector[1] = f.selector[2] = 0u;
    f.mode = 2u;
    f.index = 0x93u;
    f.length1 = f.length2 = 2u;
    f.word_192 = 0x88abu;
    f.word_190 = 7u;
    f.word_18e = 0xfffeu;
    f.word_1f6 = 1u;
    return f;
}

static int record(fixture *f, uint32_t code)
{
    if (f->event_count == sizeof f->events / sizeof f->events[0]) {
        f->bad = 1;
        return 1;
    }
    f->events[f->event_count++] = code;
    return f->event_count == f->fail_at;
}

static int read_ds_byte(void *context, uint16_t offset, uint8_t *value)
{
    fixture *f = context;
    switch (offset) {
    case 0x6aadu: *value = f->first; break;
    case 0x6aaeu: *value = f->second; break;
    case 0x49fau:
        if (f->selector_reads >= 3u) f->bad = 1;
        *value = f->selector[f->selector_reads < 3u ? f->selector_reads : 2u];
        ++f->selector_reads;
        break;
    case 0x6aafu: *value = f->index; break;
    case 0x49f3u: *value = f->mode; break;
    default: f->bad = 1; return 1;
    }
    return record(f, EVENT(1, offset));
}

static int read_ds_far_ptr(void *context, uint16_t offset,
                           pr_rec_290b_far_ptr *value)
{
    fixture *f = context;
    if (offset == 0x49d2u) {
        value->offset = 0xff00u;
        value->segment = 0xbeefu;
    } else if (offset == 0x49d6u) {
        value->offset = 0xfe00u;
        value->segment = 0xcafeu;
    } else {
        f->bad = 1;
        return 1;
    }
    return record(f, EVENT(2, offset));
}

static int read_far_byte(void *context, pr_rec_290b_far_ptr base,
                         uint16_t effective_offset, uint8_t *value)
{
    fixture *f = context;
    uint16_t delta = (uint16_t)(effective_offset - base.offset);
    if (base.offset != 0xff00u || base.segment != 0xbeefu) f->bad = 1;
    if (delta == 0x0186u) *value = f->record_first;
    else if (delta == 0x0188u) *value = f->record_second;
    else { f->bad = 1; return 1; }
    return record(f, EVENT(3, delta));
}

static int read_far_word(void *context, pr_rec_290b_far_ptr base,
                         uint16_t effective_offset, uint16_t *value)
{
    fixture *f = context;
    uint16_t delta = (uint16_t)(effective_offset - base.offset);
    if (delta == 0x0594u && base.offset == 0xfe00u &&
        base.segment == 0xcafeu) *value = f->other_flags;
    else if (base.offset == 0xff00u && base.segment == 0xbeefu) {
        switch (delta) {
        case 0x0192u: *value = f->word_192; break;
        case 0x0190u: *value = f->word_190; break;
        case 0x018eu: *value = f->word_18e; break;
        case 0x01f6u: *value = f->word_1f6; break;
        default: f->bad = 1; return 1;
        }
    } else { f->bad = 1; return 1; }
    return record(f, EVENT(4, delta));
}

static void expect_push(fixture *f, const pr_rec_290b_call *call,
                        unsigned index, pr_rec_290b_push_kind kind,
                        uint16_t value)
{
    if (call->push_count <= index || call->pushes[index].kind != kind ||
        call->pushes[index].value != value) f->bad = 1;
}

static pr_rec_290b_target expected_target(uint16_t site)
{
    switch (site) {
    case 0x3507u: case 0x3563u: return PR_REC_290B_LOCAL_1C13;
    case 0x35b9u: case 0x35d1u: return PR_REC_290B_LOCAL_1BD7;
    case 0x3513u: case 0x3541u: case 0x356fu: case 0x359du:
    case 0x35ecu: case 0x362cu: case 0x3668u: case 0x3697u:
    case 0x36d1u: return PR_REC_290B_SYSTEM_0B25;
    case 0x3529u: case 0x3585u: case 0x35fcu: case 0x363cu:
    case 0x367fu: case 0x36b9u: return PR_REC_290B_SYSTEM_0B0B;
    case 0x3533u: case 0x358fu: case 0x35c1u: case 0x35d4u:
    case 0x35deu: case 0x3614u: case 0x361eu: case 0x3646u:
    case 0x3650u: case 0x365au: case 0x3689u: case 0x36c3u:
        return PR_REC_290B_SYSTEM_0B98;
    case 0x36e2u: return PR_REC_290B_RES_03F1_0137;
    case 0x36f5u: return PR_REC_290B_RES_0709_0626;
    default: return (pr_rec_290b_target)-1;
    }
}

static int call_target(void *context, const pr_rec_290b_call *call,
                       uint8_t *observed_length)
{
    fixture *f = context;
    uint8_t expected_first = f->first;
    uint8_t expected_second = f->second;
    if (f->selector[0] > 1u) {
        expected_first = f->record_first;
        expected_second = f->record_second;
        if (f->selector[1] == 3u)
            expected_first = (uint8_t)(expected_first + 13u);
        if (f->selector[2] == 4u)
            expected_first = (uint8_t)(expected_first + 26u);
    }
    if (call->target != expected_target(call->image_site) ||
        f->call_count == sizeof f->calls / sizeof f->calls[0]) f->bad = 1;
    else f->calls[f->call_count++] = call->image_site;
    if ((call->image_site == 0x3513u || call->image_site == 0x356fu) !=
        (observed_length != NULL)) f->bad = 1;
    if (observed_length != NULL)
        *observed_length = call->image_site == 0x3513u ?
            f->length1 : f->length2;
    switch (call->image_site) {
    case 0x3507u:
        expect_push(f, call, 0, PR_REC_290B_SS_LOCAL, 0x0131u);
        expect_push(f, call, 1, PR_REC_290B_WORD, f->word_192);
        f->word_190 = 0x1111u; /* Later read must observe callee effects. */
        break;
    case 0x3563u:
        expect_push(f, call, 0, PR_REC_290B_SS_LOCAL, 0x0131u);
        expect_push(f, call, 1, PR_REC_290B_WORD,
                    (uint16_t)((uint32_t)f->word_190 * 10u + f->word_18e));
        break;
    case 0x35b9u:
        expect_push(f, call, 1, PR_REC_290B_AX_LOW_BYTE, expected_first);
        break;
    case 0x35d1u:
        expect_push(f, call, 1, PR_REC_290B_AX_LOW_BYTE, expected_second);
        break;
    case 0x35d4u:
        if (call->push_count != 0u) f->bad = 1;
        break;
    case 0x3614u:
        expect_push(f, call, 0, PR_REC_290B_DS_OFFSET,
                    (uint16_t)(0x25a8u + 3u * f->index));
        break;
    case 0x36e2u:
        expect_push(f, call, 0, PR_REC_290B_AX_LOW_BYTE, 0x11u);
        expect_push(f, call, 1, PR_REC_290B_AX_LOW_BYTE, 0x0fu);
        expect_push(f, call, 2, PR_REC_290B_AX_LOW_BYTE, 0x26u);
        expect_push(f, call, 3, PR_REC_290B_AX_LOW_BYTE, 0x0fu);
        break;
    case 0x36f5u:
        expect_push(f, call, 0, PR_REC_290B_AX_LOW_BYTE, 0x11u);
        expect_push(f, call, 1, PR_REC_290B_AX_LOW_BYTE, 0x0fu);
        expect_push(f, call, 2, PR_REC_290B_AX_LOW_BYTE, 0x0au);
        expect_push(f, call, 3, PR_REC_290B_SS_LOCAL, 0x0031u);
        break;
    default: break;
    }
    return record(f, EVENT(5, call->image_site));
}

static pr_rec_290b_io callbacks(fixture *f)
{
    pr_rec_290b_io io = {f, read_ds_byte, read_ds_far_ptr,
                         read_far_byte, read_far_word, call_target};
    return io;
}

static int check_calls(const fixture *f, const uint16_t *expected, size_t count)
{
    return !f->bad && f->call_count == count &&
        memcmp(f->calls, expected, count * sizeof expected[0]) == 0;
}

static int test_branches(void)
{
    static const uint16_t short_path[] = {
        0x3507, 0x3513, 0x3563, 0x356f, 0x35fc, 0x3614,
        0x361e, 0x362c, 0x363c, 0x3646, 0x3650, 0x365a,
        0x3668, 0x367f, 0x3689, 0x3697, 0x36e2, 0x36f5
    };
    static const uint16_t rich_path[] = {
        0x3507, 0x3513, 0x3529, 0x3533, 0x3541, 0x3563,
        0x356f, 0x3585, 0x358f, 0x359d, 0x35b9, 0x35c1,
        0x35d1, 0x35d4, 0x35de, 0x35ec, 0x35fc, 0x3614,
        0x361e, 0x362c, 0x363c, 0x3646, 0x3650, 0x365a,
        0x3668, 0x36b9, 0x36c3, 0x36d1, 0x36e2, 0x36f5
    };
    fixture f = setup();
    pr_rec_290b_io io = callbacks(&f);
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_OK);
    CHECK(check_calls(&f, short_path, sizeof short_path / sizeof short_path[0]));
    CHECK(f.selector_reads == 1u);

    f = setup();
    f.selector[0] = f.selector[1] = f.selector[2] = 3u;
    f.length1 = f.length2 = 1u;
    f.word_1f6 = 0u;
    f.mode = 1u;
    f.other_flags = 1u;
    io = callbacks(&f);
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_OK);
    CHECK(check_calls(&f, rich_path, sizeof rich_path / sizeof rich_path[0]));
    CHECK(f.selector_reads == 3u);

    f = setup();
    f.selector[0] = f.selector[1] = f.selector[2] = 4u;
    f.length2 = 1u;
    f.word_1f6 = 0u;
    f.mode = 1u;
    io = callbacks(&f);
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_OK);
    CHECK(!f.bad && f.call_count == 24u && f.calls[f.call_count - 2u] == 0x36e2u);

    f = setup();
    f.selector[0] = 1u;
    f.word_1f6 = 0u;
    io = callbacks(&f);
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_OK);
    CHECK(!f.bad && f.selector_reads == 1u);

    f = setup();
    f.selector[0] = 2u;
    f.selector[1] = 3u;
    f.selector[2] = 4u;
    f.word_1f6 = 0u;
    io = callbacks(&f);
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_OK);
    CHECK(!f.bad && f.selector_reads == 3u);
    return 0;
}

static int test_each_reached_failure(void)
{
    fixture reference = setup();
    pr_rec_290b_io io;
    unsigned fail;
    reference.selector[0] = reference.selector[1] = reference.selector[2] = 3u;
    reference.length1 = reference.length2 = 1u;
    reference.word_1f6 = 0u;
    reference.mode = 1u;
    reference.other_flags = 1u;
    io = callbacks(&reference);
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_OK);
    CHECK(!reference.bad && reference.event_count > 50u);
    for (fail = 1; fail <= reference.event_count; ++fail) {
        fixture f = setup();
        f.selector[0] = f.selector[1] = f.selector[2] = 3u;
        f.length1 = f.length2 = 1u;
        f.word_1f6 = 0u;
        f.mode = 1u;
        f.other_flags = 1u;
        f.fail_at = fail;
        io = callbacks(&f);
        CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_BOUNDARY);
        CHECK(!f.bad && f.event_count == fail &&
              memcmp(f.events, reference.events, fail * sizeof f.events[0]) == 0);
    }
    return 0;
}

static int test_missing_callbacks(void)
{
    fixture f = setup();
    pr_rec_290b_io io = callbacks(&f);
    CHECK(pr_rec_res_00ba_290b(NULL) == PR_REC_290B_ARGUMENT);
    io.read_far_byte = NULL;
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_OK);
    CHECK(!f.bad && f.selector_reads == 1u);

    f = setup();
    io = callbacks(&f);
    io.read_ds_byte = NULL;
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_BOUNDARY);
    CHECK(f.event_count == 0u);

    f = setup();
    io = callbacks(&f);
    io.read_ds_far_ptr = NULL;
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_BOUNDARY);
    CHECK(f.event_count == 3u);

    f = setup();
    io = callbacks(&f);
    io.read_far_word = NULL;
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_BOUNDARY);
    CHECK(f.event_count == 4u);

    f = setup();
    f.selector[0] = 3u;
    io = callbacks(&f);
    io.read_far_byte = NULL;
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_BOUNDARY);
    CHECK(f.event_count == 4u);

    f = setup();
    io = callbacks(&f);
    io.call = NULL;
    CHECK(pr_rec_res_00ba_290b(&io) == PR_REC_290B_BOUNDARY);
    CHECK(f.event_count == 5u);
    return 0;
}

int main(void)
{
    if (test_branches() || test_each_reached_failure() ||
        test_missing_callbacks()) return 1;
    return 0;
}
