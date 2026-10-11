#include "poolrad/recovered/resident_field_08ef.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    pr_rec_08ef_far node;
    uint8_t field, mutate_to;
    int mutate_after_copy;
    pr_rec_08ef_event events[6];
    unsigned event_count;
    char trace[16];
    unsigned calls, fail_at, reads;
    uint8_t display_al;
} fixture;

static int step(fixture *f, char code) {
    if (f->calls >= sizeof f->trace - 1) return 1;
    f->trace[f->calls++] = code;
    f->trace[f->calls] = '\0';
    return f->calls == f->fail_at;
}

static int read_byte(void *ctx, pr_rec_08ef_far node,
                     uint16_t displacement, uint8_t *value) {
    fixture *f = ctx;
    if (node.segment != f->node.segment || node.offset != f->node.offset ||
        displacement != 0x111) return 1;
    ++f->reads;
    if (step(f, 'R')) return 1;
    *value = f->field;
    return 0;
}

static int effect(void *ctx, const pr_rec_08ef_event *event, uint8_t *al) {
    fixture *f = ctx;
    const char codes[] = {'F', 'C', 'L', 'A', 'B', 'D'};
    if ((unsigned)event->kind >= sizeof codes ||
        f->event_count >= sizeof f->events / sizeof f->events[0]) return 1;
    f->events[f->event_count++] = *event;
    if (step(f, codes[event->kind])) return 1;
    if (f->mutate_after_copy && event->kind == PR_REC_08EF_COPY_FIRST)
        f->field = f->mutate_to;
    *al = event->kind == PR_REC_08EF_DISPLAY
        ? f->display_al : (uint8_t)f->event_count;
    return 0;
}

static void init(fixture *f, uint8_t field) {
    memset(f, 0, sizeof *f);
    f->node = (pr_rec_08ef_far){0x1234, 0x5678};
    f->field = field;
    f->display_al = 0xA5;
}

static pr_rec_08ef_io io_for(fixture *f) {
    pr_rec_08ef_io io = {f, read_byte, effect};
    return io;
}

static int test_short_path_and_frame(void) {
    fixture f;
    pr_rec_08ef_io io;
    uint8_t al = 0x77;
    init(&f, 0x3C);
    io = io_for(&f);
    CHECK(pr_rec_res_00ba_08ef(&io, f.node, 0x21, 0x7F, &al) ==
          PR_REC_08EF_OK);
    CHECK(strcmp(f.trace, "RFCRD") == 0 && f.reads == 2 &&
          f.event_count == 3 && al == 0xA5);
    CHECK(f.events[0].kind == PR_REC_08EF_FORMAT &&
          f.events[0].number == 0);
    CHECK(f.events[1].kind == PR_REC_08EF_COPY_FIRST &&
          f.events[1].limit == 0xFF);
    CHECK(f.events[2].kind == PR_REC_08EF_DISPLAY &&
          f.events[2].x == 0x21 && f.events[2].y == 0x7F &&
          f.events[2].style == 0x0A);
    CHECK(f.events[2].node.segment == 0x1234 &&
          f.events[2].node.offset == 0x5678);
    return 0;
}

static int test_distance_and_second_read_branch(void) {
    static const struct { uint8_t field; uint16_t distance; } values[] = {
        {0, 60}, {0x3B, 1}, {0x3C, 0}, {0x3D, 1}, {0xFF, 195}
    };
    unsigned i, branch;
    for (i = 0; i < sizeof values / sizeof values[0]; ++i) {
        for (branch = 0; branch < 2; ++branch) {
            fixture f;
            pr_rec_08ef_io io;
            uint8_t al = 0;
            init(&f, values[i].field);
            f.mutate_after_copy = 1;
            f.mutate_to = branch ? 0x3D : 0x3C;
            io = io_for(&f);
            CHECK(pr_rec_res_00ba_08ef(&io, f.node, 4, 9, &al) ==
                  PR_REC_08EF_OK);
            CHECK(f.events[0].number == values[i].distance);
            CHECK(f.reads == 2 && al == 0xA5);
            if (branch) {
                CHECK(strcmp(f.trace, "RFCRLABD") == 0 &&
                      f.event_count == 6);
                CHECK(f.events[2].kind == PR_REC_08EF_COPY_LITERAL &&
                      f.events[3].kind == PR_REC_08EF_APPEND_NUMBER &&
                      f.events[4].kind == PR_REC_08EF_COPY_FINAL &&
                      f.events[4].limit == 0xFF &&
                      f.events[5].kind == PR_REC_08EF_DISPLAY);
            } else {
                CHECK(strcmp(f.trace, "RFCRD") == 0 &&
                      f.event_count == 3);
            }
        }
    }
    return 0;
}

static int test_reached_boundaries(void) {
    unsigned fail;
    for (fail = 1; fail <= 8; ++fail) {
        fixture f;
        pr_rec_08ef_io io;
        uint8_t al = 0x77;
        init(&f, 0x3D);
        io = io_for(&f);
        f.fail_at = fail;
        CHECK(pr_rec_res_00ba_08ef(&io, f.node, 1, 2, &al) ==
              PR_REC_08EF_BOUNDARY);
        CHECK(f.calls == fail && al == 0x77);
    }
    {
        fixture f;
        pr_rec_08ef_io io;
        uint8_t al = 0x77;
        init(&f, 0x3C);
        io = io_for(&f);
        CHECK(pr_rec_res_00ba_08ef(NULL, f.node, 1, 2, &al) ==
              PR_REC_08EF_ARGUMENT);
        CHECK(pr_rec_res_00ba_08ef(&io, f.node, 1, 2, NULL) ==
              PR_REC_08EF_ARGUMENT);
        CHECK(f.calls == 0);
        io.read_byte = NULL;
        CHECK(pr_rec_res_00ba_08ef(&io, f.node, 1, 2, &al) ==
              PR_REC_08EF_BOUNDARY);
        CHECK(f.calls == 0 && al == 0x77);
        io = io_for(&f);
        io.effect = NULL;
        CHECK(pr_rec_res_00ba_08ef(&io, f.node, 1, 2, &al) ==
              PR_REC_08EF_BOUNDARY);
        CHECK(strcmp(f.trace, "R") == 0 && al == 0x77);
    }
    return 0;
}

int main(void) {
    if (test_short_path_and_frame() ||
        test_distance_and_second_read_branch() ||
        test_reached_boundaries()) return 1;
    puts("resident field 08EF tests passed");
    return 0;
}
