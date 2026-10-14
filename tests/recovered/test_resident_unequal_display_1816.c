#include "poolrad/recovered/resident_unequal_display_1816.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    char trace[16];
    unsigned count, fail_at, effects;
    uint8_t b10d, b10e, length, al_node, al_suffix;
    pr_rec_1816_effect seen[3];
    pr_rec_1816_far node;
    int mutate_length;
} fixture;

static int step(fixture *f, char code) {
    if (f->count >= sizeof f->trace - 1) return 1;
    f->trace[f->count++] = code;
    f->trace[f->count] = '\0';
    return f->count == f->fail_at;
}

static int read_byte(void *ctx, pr_rec_1816_far node,
                     uint16_t displacement, uint8_t *value) {
    fixture *f = ctx;
    char code;
    if (node.segment != f->node.segment || node.offset != f->node.offset)
        return 1;
    if (displacement == 0x10D) {
        code = 'D'; *value = f->b10d;
    } else if (displacement == 0x10E) {
        code = 'E'; *value = f->b10e;
    } else if (displacement == 0) {
        code = 'L'; *value = f->length;
    } else return 1;
    return step(f, code);
}

static int effect(void *ctx, const pr_rec_1816_effect *event, uint8_t *al) {
    fixture *f = ctx;
    const char codes[] = {'N', 'C', 'S'};
    if ((unsigned)event->kind >= sizeof codes || f->effects >= 3) return 1;
    f->seen[f->effects++] = *event;
    if (step(f, codes[event->kind])) return 1;
    if (event->kind == PR_REC_1816_PRINT_NODE) {
        if (f->mutate_length) f->length = 7;
        *al = f->al_node;
    } else if (event->kind == PR_REC_1816_PRINT_SUFFIX) {
        *al = f->al_suffix;
    }
    return 0;
}

static fixture make_fixture(void) {
    fixture f = {0};
    f.node = (pr_rec_1816_far){0x4567, 0x1234};
    f.length = 2;
    f.al_node = 0xA1;
    f.al_suffix = 0xB2;
    return f;
}

static pr_rec_1816_io make_io(fixture *f) {
    pr_rec_1816_io io = {f, read_byte, effect};
    return io;
}

static int test_color_and_flag_paths(void) {
    fixture f = make_fixture();
    pr_rec_1816_io io = make_io(&f);
    uint8_t al = 0;
    CHECK(pr_rec_res_00ba_1816(&io, f.node, 0, 7, 17, &al) ==
          PR_REC_1816_OK);
    CHECK(strcmp(f.trace, "DN") == 0 && al == 0xA1);
    CHECK(f.seen[0].kind == PR_REC_1816_PRINT_NODE &&
          f.seen[0].color == 0x0C && f.seen[0].row == 7 &&
          f.seen[0].column == 17);

    f = make_fixture(); io = make_io(&f); f.b10d = 1; f.b10e = 1;
    CHECK(pr_rec_res_00ba_1816(&io, f.node, 1, 9, 250, &al) ==
          PR_REC_1816_OK);
    CHECK(strcmp(f.trace, "DENLCS") == 0 && al == 0xB2);
    CHECK(f.seen[0].color == 0x0E && f.seen[1].kind == PR_REC_1816_COPY_SUFFIX &&
          f.seen[2].kind == PR_REC_1816_PRINT_SUFFIX &&
          f.seen[2].column == 252 && f.seen[2].row == 9 &&
          f.seen[2].color == 0x0E);

    f = make_fixture(); io = make_io(&f); f.b10d = 2; f.b10e = 2;
    f.mutate_length = 1;
    CHECK(pr_rec_res_00ba_1816(&io, f.node, 0x80, 0xFE, 0xFE, &al) ==
          PR_REC_1816_OK);
    CHECK(strcmp(f.trace, "DENLCS") == 0 && f.seen[0].color == 0x0B &&
          f.seen[2].column == 5 && f.seen[2].row == 0xFE);
    return 0;
}

static int test_boundary_failures(void) {
    unsigned fail;
    for (fail = 1; fail <= 6; ++fail) {
        fixture f = make_fixture();
        pr_rec_1816_io io = make_io(&f);
        uint8_t al = 0x5A;
        f.b10d = 1; f.fail_at = fail;
        CHECK(pr_rec_res_00ba_1816(&io, f.node, 1, 2, 3, &al) ==
              PR_REC_1816_BOUNDARY);
        CHECK(f.count == fail && al == 0x5A);
    }
    {
        fixture f = make_fixture();
        pr_rec_1816_io io = make_io(&f);
        uint8_t al = 0x5A;
        io.read_byte = NULL;
        CHECK(pr_rec_res_00ba_1816(&io, f.node, 0, 2, 3, &al) ==
              PR_REC_1816_BOUNDARY && f.count == 0 && al == 0x5A);
        io = make_io(&f); io.effect = NULL;
        CHECK(pr_rec_res_00ba_1816(&io, f.node, 0, 2, 3, &al) ==
              PR_REC_1816_BOUNDARY && strcmp(f.trace, "D") == 0);
        CHECK(pr_rec_res_00ba_1816(NULL, f.node, 0, 2, 3, &al) ==
              PR_REC_1816_ARGUMENT);
        CHECK(pr_rec_res_00ba_1816(&io, f.node, 0, 2, 3, NULL) ==
              PR_REC_1816_ARGUMENT);
    }
    return 0;
}

int main(void) {
    if (test_color_and_flag_paths() || test_boundary_failures()) return 1;
    puts("resident 1816 bounded display model: ok");
    return 0;
}
