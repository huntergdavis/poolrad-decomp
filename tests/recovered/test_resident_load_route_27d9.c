#include "poolrad/recovered/resident_load_route_27d9.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    uint8_t mode;
    uint8_t byte_fd, byte_fe;
    uint16_t word_94, word_92;
    uint8_t code_bytes[3];
    char events[20];
    unsigned calls, fail_at;
    int unexpected;
} fixture;

static int event(fixture *f, char code)
{
    if (f->calls >= sizeof f->events - 1u) {
        f->unexpected = 1;
        return 1;
    }
    f->events[f->calls++] = code;
    f->events[f->calls] = '\0';
    return f->calls == f->fail_at;
}

static int read_ds_byte(void *context, uint16_t offset, uint8_t *value)
{
    fixture *f = context;
    if (offset == 0x49f3u) {
        *value = f->mode;
        return event(f, 'm');
    }
    if (offset == 0x49fdu) {
        *value = f->byte_fd;
        return event(f, 'a');
    }
    if (offset == 0x49feu) {
        *value = f->byte_fe;
        return event(f, 'b');
    }
    f->unexpected = 1;
    return 1;
}

static int read_ds_word(void *context, uint16_t offset, uint16_t *value)
{
    fixture *f = context;
    if (offset == 0x5d94u) {
        *value = f->word_94;
        return event(f, 'W');
    }
    if (offset == 0x5d92u) {
        *value = f->word_92;
        return event(f, 'w');
    }
    f->unexpected = 1;
    return 1;
}

static int read_cs_bytes(void *context, uint16_t offset,
                         uint8_t count, uint8_t *bytes)
{
    fixture *f = context;
    if (offset != 0x27d6u || count != 3u) f->unexpected = 1;
    memcpy(bytes, f->code_bytes, 3u);
    return event(f, 'S');
}

static void expect_no_args(fixture *f, const pr_rec_27d9_args *a)
{
    if (a->byte_count || a->word_count || a->has_ds_pointer || a->string)
        f->unexpected = 1;
}

static int call_target(void *context, pr_rec_27d9_target target,
                       const pr_rec_27d9_args *a)
{
    fixture *f = context;
    char code = '?';
    switch (target) {
    case PR_REC_27D9_RES_03F1_02E8:
        code = 'F';
        if (a->byte_count != 6u || a->word_count || a->has_ds_pointer ||
            !a->string || a->string->length != 0u ||
            a->byte_args[0] != 1u || a->byte_args[1] != 1u ||
            a->byte_args[2] != 0x26u || a->byte_args[3] != 0x16u ||
            a->byte_args[4] != 0u || a->byte_args[5] != 0x0fu)
            f->unexpected = 1;
        break;
    case PR_REC_27D9_LOCAL_32CA:
        code = 'L';
        if (a->byte_count != 1u || a->word_count || a->string ||
            a->has_ds_pointer ||
            a->byte_args[0] != (f->mode == 3u ? 0u : 1u))
            f->unexpected = 1;
        /* These reads occur after the local callback, not at entry. */
        f->byte_fd = 0xa1u;
        f->byte_fe = 0xb2u;
        break;
    case PR_REC_27D9_OVL_03D0_003E:
        code = 'P';
        if (a->byte_count != 2u || a->word_count || a->string ||
            a->has_ds_pointer || a->byte_args[0] != 0xa1u ||
            a->byte_args[1] != 0xb2u) f->unexpected = 1;
        break;
    case PR_REC_27D9_OVL_03D0_0039:
        code = 'Q';
        if (a->byte_count != 3u || a->word_count || a->string ||
            a->has_ds_pointer || a->byte_args[0] != 3u ||
            a->byte_args[1] != 3u || a->byte_args[2] != 1u)
            f->unexpected = 1;
        break;
    case PR_REC_27D9_OVL_03D0_002A:
        code = 'D';
        if (a->byte_count != 2u || a->word_count ||
            a->byte_args[0] != (f->mode == 2u ? 0x1du : 1u) ||
            a->byte_args[1] != 0u || !a->has_ds_pointer ||
            a->ds_offset != 0x6abeu || !a->string ||
            a->string->length != 3u ||
            memcmp(a->string->bytes, f->code_bytes, 3u) != 0)
            f->unexpected = 1;
        f->word_94 = 0x1357u;
        f->word_92 = 0x2468u;
        break;
    case PR_REC_27D9_OVL_03CA_0025:
        code = 'V';
        expect_no_args(f, a);
        f->word_94 = 0x1357u;
        f->word_92 = 0x2468u;
        break;
    case PR_REC_27D9_RES_00BA_0767:
        code = 'R';
        if (a->byte_count || a->word_count != 2u || a->string ||
            a->has_ds_pointer || a->word_args[0] != 0x1357u ||
            a->word_args[1] != 0x2468u) f->unexpected = 1;
        break;
    case PR_REC_27D9_RES_00BA_290B:
        code = 'U';
        expect_no_args(f, a);
        break;
    default:
        f->unexpected = 1;
        break;
    }
    return event(f, code);
}

static pr_rec_27d9_io callbacks(fixture *f)
{
    pr_rec_27d9_io io = {
        f, read_ds_byte, read_ds_word, read_cs_bytes, call_target
    };
    return io;
}

static fixture setup(uint8_t mode)
{
    fixture f = {0};
    f.mode = mode;
    f.byte_fd = 0x11u;
    f.byte_fe = 0x22u;
    f.word_94 = 0x3333u;
    f.word_92 = 0x4444u;
    f.code_bytes[0] = 0x12u;
    f.code_bytes[1] = 0x34u;
    f.code_bytes[2] = 0x56u;
    return f;
}

static int test_paths_and_failures(void)
{
    static const struct {
        uint8_t mode;
        const char *events;
    } cases[] = {
        {0u, "mF"}, {1u, "mLabPQ"}, {2u, "mLSDWwRU"},
        {3u, "mLVWwRU"}, {4u, "mLVWwRU"}, {6u, "mLSDWwR"},
        {5u, "m"}, {255u, "m"}
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        fixture f = setup(cases[i].mode);
        pr_rec_27d9_io io = callbacks(&f);
        size_t fail, count = strlen(cases[i].events);
        CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_OK);
        CHECK(!f.unexpected && strcmp(f.events, cases[i].events) == 0);
        for (fail = 1; fail <= count; ++fail) {
            f = setup(cases[i].mode);
            f.fail_at = (unsigned)fail;
            io = callbacks(&f);
            CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_BOUNDARY);
            CHECK(!f.unexpected && f.calls == fail &&
                  strncmp(f.events, cases[i].events, fail) == 0);
        }
    }
    return 0;
}

static int test_invalid_arguments(void)
{
    fixture f = setup(0);
    pr_rec_27d9_io io = callbacks(&f);
    CHECK(pr_rec_res_00ba_27d9(NULL) == PR_REC_27D9_ARGUMENT);
    io.read_ds_byte = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_BOUNDARY);
    CHECK(f.calls == 0u);
    return 0;
}

static int test_missing_callbacks_at_first_use(void)
{
    fixture f = setup(5);
    pr_rec_27d9_io io = callbacks(&f);
    io.read_ds_word = NULL;
    io.read_cs_bytes = NULL;
    io.call = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_OK);
    CHECK(!f.unexpected && strcmp(f.events, "m") == 0);

    f = setup(0);
    io = callbacks(&f);
    io.read_ds_word = NULL;
    io.read_cs_bytes = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_OK);
    CHECK(!f.unexpected && strcmp(f.events, "mF") == 0);

    f = setup(1);
    io = callbacks(&f);
    io.read_ds_word = NULL;
    io.read_cs_bytes = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_OK);
    CHECK(!f.unexpected && strcmp(f.events, "mLabPQ") == 0);

    f = setup(0);
    io = callbacks(&f);
    io.call = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_BOUNDARY);
    CHECK(!f.unexpected && strcmp(f.events, "m") == 0);

    f = setup(2);
    io = callbacks(&f);
    io.read_cs_bytes = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_BOUNDARY);
    CHECK(!f.unexpected && strcmp(f.events, "mL") == 0);

    f = setup(2);
    io = callbacks(&f);
    io.read_ds_word = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_BOUNDARY);
    CHECK(!f.unexpected && strcmp(f.events, "mLSD") == 0);

    f = setup(4);
    io = callbacks(&f);
    io.read_ds_word = NULL;
    CHECK(pr_rec_res_00ba_27d9(&io) == PR_REC_27D9_BOUNDARY);
    CHECK(!f.unexpected && strcmp(f.events, "mLV") == 0);
    return 0;
}

int main(void)
{
    if (test_paths_and_failures() || test_invalid_arguments() ||
        test_missing_callbacks_at_first_use()) return 1;
    return 0;
}
