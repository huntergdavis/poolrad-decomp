#include "poolrad/recovered/resident_field_0973.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    pr_rec_0973_far node;
    uint8_t first, threshold, formatted, display_al;
    pr_rec_0973_event events[2];
    unsigned event_count, reads, calls, fail_at;
    char trace[8];
} fixture;

static void init(fixture *f, uint8_t first, uint8_t threshold,
                 uint8_t formatted) {
    memset(f, 0, sizeof *f);
    f->node = (pr_rec_0973_far){0x1234, 0x5678};
    f->first = first;
    f->threshold = threshold;
    f->formatted = formatted;
    f->display_al = 0xA5;
}

static int step(fixture *f, char code) {
    if (f->calls >= sizeof f->trace - 1) return 1;
    f->trace[f->calls++] = code;
    f->trace[f->calls] = '\0';
    return f->calls == f->fail_at;
}

static int read_byte(void *ctx, pr_rec_0973_far node,
                     uint16_t displacement, uint8_t *value) {
    fixture *f = ctx;
    const uint16_t expected[] = {0x11B, 0x32, 0x11B};
    const char code[] = {'1', '2', '1'};
    unsigned n = f->reads;
    if (node.segment != f->node.segment || node.offset != f->node.offset ||
        n >= 3 || displacement != expected[n]) return 1;
    ++f->reads;
    if (step(f, code[n])) return 1;
    *value = n == 0 ? f->first : n == 1 ? f->threshold : f->formatted;
    return 0;
}

static int effect(void *ctx, const pr_rec_0973_event *event, uint8_t *al) {
    fixture *f = ctx;
    const char code[] = {'F', 'D'};
    if ((unsigned)event->kind >= sizeof code || f->event_count >= 2)
        return 1;
    f->events[f->event_count++] = *event;
    if (step(f, code[event->kind])) return 1;
    *al = event->kind == PR_REC_0973_DISPLAY ? f->display_al : 0x3C;
    return 0;
}

static pr_rec_0973_io io_for(fixture *f) {
    pr_rec_0973_io io = {f, read_byte, effect};
    return io;
}

static int test_branches_frame_and_second_read(void) {
    static const struct {
        uint8_t first, threshold, flag, style;
    } cases[] = {
        {4, 5, 0, 0x0E}, {5, 5, 0, 0x0A}, {6, 5, 0, 0x0A},
        {0xFE, 0xFF, 0, 0x0E}, {0xFF, 0xFF, 0, 0x0A},
        {4, 5, 1, 0x0D}, {6, 5, 0xFF, 0x0D}
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        fixture f;
        pr_rec_0973_io io;
        uint8_t al = 0x77;
        init(&f, cases[i].first, cases[i].threshold, 0x91);
        io = io_for(&f);
        CHECK(pr_rec_res_00ba_0973(&io, f.node, 0x81, 0xFE,
                                     cases[i].flag, &al) == PR_REC_0973_OK);
        CHECK(strcmp(f.trace, "121FD") == 0 && f.reads == 3 &&
              f.event_count == 2 && al == 0xA5);
        CHECK(f.events[0].kind == PR_REC_0973_FORMAT_NUMBER &&
              f.events[0].number == 0x91); /* fresh third read */
        CHECK(f.events[1].kind == PR_REC_0973_DISPLAY &&
              f.events[1].x == 0x81 && f.events[1].y == 0xFE &&
              f.events[1].style == cases[i].style);
        CHECK(f.events[1].node.segment == 0x1234 &&
              f.events[1].node.offset == 0x5678);
    }
    return 0;
}

static int test_reached_failures(void) {
    static const char *const traces[] = {"", "1", "12", "121", "121F", "121FD"};
    static const unsigned read_counts[] = {0, 1, 2, 3, 3, 3};
    static const unsigned event_counts[] = {0, 0, 0, 0, 1, 2};
    unsigned fail;
    for (fail = 1; fail <= 5; ++fail) {
        fixture f;
        pr_rec_0973_io io;
        uint8_t al = 0x77;
        init(&f, 4, 5, 6);
        io = io_for(&f);
        f.fail_at = fail;
        CHECK(pr_rec_res_00ba_0973(&io, f.node, 1, 2, 0, &al) ==
              PR_REC_0973_BOUNDARY);
        CHECK(f.calls == fail && al == 0x77);
        CHECK(strcmp(f.trace, traces[fail]) == 0);
        CHECK(f.reads == read_counts[fail] &&
              f.event_count == event_counts[fail]);
    }
    {
        fixture f;
        pr_rec_0973_io io;
        uint8_t al = 0x77;
        init(&f, 4, 5, 6);
        io = io_for(&f);
        CHECK(pr_rec_res_00ba_0973(NULL, f.node, 1, 2, 0, &al) ==
              PR_REC_0973_ARGUMENT);
        CHECK(pr_rec_res_00ba_0973(&io, f.node, 1, 2, 0, NULL) ==
              PR_REC_0973_ARGUMENT);
        CHECK(f.calls == 0);
        io.read_byte = NULL;
        CHECK(pr_rec_res_00ba_0973(&io, f.node, 1, 2, 0, &al) ==
              PR_REC_0973_BOUNDARY);
        CHECK(f.calls == 0 && al == 0x77);
        io = io_for(&f);
        io.effect = NULL;
        CHECK(pr_rec_res_00ba_0973(&io, f.node, 1, 2, 0, &al) ==
              PR_REC_0973_BOUNDARY);
        CHECK(strcmp(f.trace, "121") == 0 && al == 0x77);
    }
    return 0;
}

int main(void) {
    if (test_branches_frame_and_second_read() || test_reached_failures())
        return 1;
    puts("resident field 0973 tests passed");
    return 0;
}
